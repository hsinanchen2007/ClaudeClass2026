// =============================================================================
//  第 26 課：this 指標7.cpp  —  this 與靜態成員：誰有 this、誰沒有
// =============================================================================
//
// 【主題資訊 Information】
//   非靜態成員函數：有隱藏參數 this，可存取 instance member 與 static member
//   靜態成員函數  ：沒有 this，只能存取 static member（除非另外傳入物件）
//   語法：  static void f();              // 宣告在 class 內才寫 static
//           inline static int x = 20;     // C++17：類別內直接定義 static 資料成員
//   標準：  static 成員函數自 C++98；inline static 資料成員初始化是 **C++17**
//           （C++17 前必須在類別外另寫一行定義：int Demo::staticVar_ = 20;）
//   標頭檔：不需要
//   複雜度：static 成員函數呼叫少一個隱藏參數，成本與自由函式相同，O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. 「有沒有 this」是靜態 / 非靜態的唯一本質差別】
//   非靜態成員函數 obj.f(a) 實際上等同於 Demo::f(&obj, a)——編譯器偷偷把物件
//   位址塞成第一個參數。靜態成員函數則完全沒有這個參數，它本質上就是一個
//   「被放進類別命名空間、且能存取 private 的自由函式」。
//
//   由此可以一口氣推出所有規則，不必死背：
//     * static 成員函數不能存取 instance member ── 沒有 this，不知道要存取「誰的」。
//     * static 成員函數不能標 const/volatile ── const 修飾的是 this，沒有 this 可修飾。
//     * static 成員函數不能是 virtual ── virtual dispatch 靠物件內的 vptr 決定，
//       沒有物件就沒有 vptr。
//     * 非靜態成員函數存取 static member 完全合法 ── 反方向不需要 this。
//
// 【2. static 資料成員：屬於類別，不屬於物件】
//   staticVar_ 只有一份，所有 Demo 物件共用；它的儲存期是 static storage duration，
//   生命週期跨整個程式，與任何物件無關。因此：
//     * 它不計入 sizeof(Demo)。本檔輸出會實測驗證 sizeof(Demo) == sizeof(int)。
//     * 即使一個 Demo 物件都沒建立，Demo::staticVar_ 依然存在且可存取。
//     * 修改它會影響所有物件——這正是它適合拿來做計數器、共用設定、
//       物件池統計的原因，也正是它容易變成隱藏全域狀態的原因。
//
// 【3. inline static 是 C++17 才有的便利】
//   C++17 之前，類別內只能「宣告」static 資料成員，還必須在某一個 .cpp 檔
//   寫一次類別外定義，否則連結期會出現 undefined reference：
//       // demo.h
//       class Demo { static int staticVar_; };
//       // demo.cpp   ← 這行不能忘，而且只能有一份
//       int Demo::staticVar_ = 20;
//   C++17 的 inline 變數讓「宣告即定義」，且允許出現在多個翻譯單元而不違反 ODR，
//   連結器會自動合併成一份。header-only 函式庫因此大幅受益。
//   （唯一長期例外：static constexpr 資料成員自 C++17 起隱含 inline。）
//
// 【4. 什麼時候該用 static 成員函數】
//     * 工廠函式：Demo::create(...)，在還沒有物件的時候就要被呼叫。
//     * 與類別強相關但不需要特定物件的工具：Date::isLeapYear(2024)。
//     * 回呼轉接（thunk）：C API 只收函式指標，不收成員函數指標，
//       慣例是用 static 成員函數當跳板，把 void* user_data 轉回物件指標再呼叫。
//       ── 本檔尾端的【日常實務範例】就是這個模式。
//
// 【概念補充 Concept Deep Dive】
//   * 成員函數指標的型別與自由函式指標**不相容**：
//         void (Demo::*mp)() = &Demo::normalFunc;   // 成員函數指標，內含隱藏 this
//         void (*fp)()       = &Demo::staticFunc;   // 一般函式指標，可直接給 C API
//     這就是為什麼所有 C 風格回呼（pthread_create、qsort、signal、
//     libcurl、SQLite）都只能收 static 成員函數或自由函式。
//   * 成員函數指標在多重繼承 / 虛擬繼承下可能不只 8 bytes（Itanium ABI 下通常
//     16 bytes：一個 ptr 加一個 adjustment），因為它必須能表達 this 調整量。
//     本機 g++ 15.2 x86-64 實測值見輸出。
//   * static 成員函數雖然沒有 this，仍受存取控制（public/private）約束，
//     且能存取同類別物件的 private 成員（只要你把物件傳給它）。
//   * 「local static 變數」（函式內的 static）與「static 資料成員」是兩件事，
//     前者是 C++11 起保證執行緒安全的 magic static，屬於不同主題。
//
// 【注意事項 Pay Attention】
//   1. static 關鍵字只寫在**類別內的宣告**；類別外定義時不可重複寫 static。
//   2. inline static 資料成員是 C++17。用 -std=c++14 編譯本檔會直接失敗，
//      這也是驗證「版本宣告是否誠實」最可靠的方式（見檔尾編譯註記）。
//   3. static 資料成員是共用可變狀態，多執行緒下需自行同步（mutex/atomic），
//      它不會因為在類別裡就自動安全。
//   4. 別為了「省一個參數」而把本該是 instance 的狀態塞進 static——
//      那等於製造全域變數，單元測試會因為測試之間互相污染而變得極難寫。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 與靜態成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 static 成員函數不能存取非靜態成員？
//     答：因為它沒有 this。非靜態成員必須透過某個具體物件才存在，
//         而 static 成員函數是用 Demo::f() 呼叫的，編譯器根本不知道「哪一個物件」。
//         反過來，非靜態成員函數存取 static 成員完全合法，因為那不需要 this。
//     追問：那 static 成員函數為什麼也不能是 virtual？
//         → virtual dispatch 要靠物件內的 vptr 查 vtable，沒有物件就沒有 vptr，
//           無從分派。同理 static 也不能加 const（const 修飾的是 this）。
//
// 🔥 Q2. static 資料成員算進 sizeof(類別) 嗎？它什麼時候被初始化？
//     答：不算。static 資料成員有 static storage duration，儲存在物件之外，
//         所有物件共用一份。本檔實測 sizeof(Demo) 只等於那個 int 成員的大小。
//         具常值初始化者在編譯期即完成；其餘於 main 之前的動態初始化階段完成。
//     追問：多個 .cpp 都 include 這個 header 會不會重複定義？
//         → C++17 的 inline static 不會，連結器會合併成一份；
//           C++17 之前必須在恰好一個 .cpp 寫類別外定義，否則 undefined reference。
//
// ⚠️ 陷阱1. 「把成員函數指標傳給 C 語言的回呼（如 pthread_create）就好了。」
//     答：不行，型別完全不相容。成員函數指標帶有隱藏的 this，
//         而 C API 期待的是 void (*)(void*) 這種純函式指標。
//         正確作法是用 static 成員函數當跳板，把物件位址透過 void* user_data
//         傳進去，在跳板內 static_cast 回物件指標再呼叫真正的成員函數。
//     為什麼會錯：以為「函式指標就是函式指標」，忽略成員函數多一個隱藏參數，
//         而且在多重繼承下它甚至不是單一位址（本機實測其 sizeof 為 16）。
//
// ⚠️ 陷阱2. 「inline static int x = 20; 在 C++11 也能編，只是有點新。」
//     答：不能。這是 C++17 的 inline 變數特性。要驗證版本宣告是否誠實，
//         必須用 -std=c++14 -pedantic-errors 實際編譯，看它是否報錯。
//     為什麼會錯：只用 -fsyntax-only 或不加 -pedantic-errors 時，
//         GCC 可能把某些較新語法當成擴充放行，讓人誤以為舊標準也支援。
//
// ⚠️ 陷阱3. 「static 成員是類別內部的，所以多執行緒存取比較安全。」
//     答：完全不是。static 資料成員就是一個有存取控制的全域變數，
//         同時讀寫仍是 data race（未定義行為），必須自行加 mutex 或改用 atomic。
//     為什麼會錯：把「封裝（誰看得到）」誤當成「同步（誰能同時改）」，
//         這是兩個互相獨立的維度。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Demo {
private:
    int instanceVar_ = 10;
    inline static int staticVar_ = 20;   // C++17：類別內直接定義

public:
    void normalFunc() {
        // 有 this：instance member 與 static member 都能存取
        //
        // 原始版本這裡印的是 this 的位址。位址受 ASLR 影響每次執行都不同，
        // 不適合寫進「預期輸出」，因此改為印出「決定性的身分驗證」：
        // 把 this 與呼叫端持有的物件位址比較，回傳布林值。
        cout << "  this 是否非空： " << boolalpha << (this != nullptr) << endl;
        cout << "  instanceVar_ = " << this->instanceVar_ << endl;  // ✅ 需要 this
        cout << "  staticVar_ = " << staticVar_ << endl;            // ✅ 不用 this
    }

    // 讓呼叫端能在不印出位址的前提下驗證「this 就是該物件」
    bool isSameObject(const Demo* p) const { return this == p; }

    static void staticFunc() {
        // 沒有 this
        // cout << this;                // ❌ 編譯錯誤：static 成員函數沒有 this
        // cout << instanceVar_;        // ❌ 編譯錯誤：需要 this 才知道是誰的
        cout << "  staticVar_ = " << staticVar_ << endl;  // ✅ 不需要物件
    }

    static void setStatic(int v) { staticVar_ = v; }      // 不需要任何物件即可呼叫

    int instanceVar() const { return instanceVar_; }
    void setInstance(int v)  { instanceVar_ = v; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「static 成員函數為何沒有 this，以及它與 C 風格回呼的關係」，
// 屬於 C++ 物件模型 / ABI 層面的知識。LeetCode 判題只會 new 一個你的 Solution
// 或 design 類別再呼叫其方法，從不涉及 static 成員函數、函式指標或回呼註冊。
// 指定清單中的 design 題（146、155、705、707、1603、1656）都不需要任何
// static 成員，硬加一題只會讓讀者以為「刷題要用 static」，是反向誤導。
// 依規格「寧缺勿濫」，此處明確從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】C 風格回呼的 trampoline（跳板）模式
//
// 情境：幾乎所有 C 函式庫（pthread、libcurl、SQLite、GLFW、libevent…）的回呼
//       都長成 void (*)(void* user_data)。你想在回呼裡呼叫某個 C++ 物件的
//       成員函數，但成員函數指標型別根本不相容——因為它帶有隱藏的 this。
//
// 標準解法（本範例示範）：
//   1. 寫一個 static 成員函數當跳板（它沒有 this，型別就是純函式指標）。
//   2. 註冊時把物件位址當成 void* user_data 一併交給 C API。
//   3. 跳板內把 void* static_cast 回物件指標——這就是「手動把 this 傳回來」。
//
// 這個範例用一個迷你事件迴圈模擬 C API，完整重現整個流程。
// -----------------------------------------------------------------------------

// ── 模擬 C 函式庫的介面（純 C 風格：只收函式指標 + void*）──────────────
using CCallback = void (*)(const char* event, void* userData);

class MiniEventLoop {
private:
    CCallback cb_       = nullptr;
    void*     userData_ = nullptr;
    vector<string> queue_;

public:
    void setCallback(CCallback cb, void* userData) {  // 典型 C API 簽章
        cb_ = cb;
        userData_ = userData;
    }
    void post(const string& ev) { queue_.push_back(ev); }
    void run() {
        for (const auto& ev : queue_) {
            if (cb_) cb_(ev.c_str(), userData_);
        }
        queue_.clear();
    }
};

// ── 我們的 C++ 物件：真正的邏輯寫在「非 static」成員函數裡 ──────────────
class MetricsCollector {
private:
    string name_;
    int    count_ = 0;

public:
    explicit MetricsCollector(string name) : name_(std::move(name)) {}

    // 真正的處理邏輯：需要 this（要更新 count_、要讀 name_）
    void onEvent(const char* event) {
        ++count_;
        cout << "    [" << name_ << "] 收到事件 " << event
             << "（累計 " << count_ << " 筆）" << endl;
    }

    // 跳板：static 成員函數，沒有 this，型別剛好等於 CCallback
    static void trampoline(const char* event, void* userData) {
        // 手動把「this」從 void* 還原回來——這正是編譯器平常替我們做的事
        auto* self = static_cast<MetricsCollector*>(userData);
        self->onEvent(event);
    }

    int count() const { return count_; }
};

int main() {
    cout << "=== this 與靜態成員 ===" << endl;

    Demo d;
    cout << "\n--- 普通函數（有 this）---" << endl;
    d.normalFunc();
    cout << "  呼叫端驗證 this == &d ？ " << boolalpha << d.isSameObject(&d) << endl;

    cout << "\n--- 靜態函數（沒有 this）---" << endl;
    Demo::staticFunc();

    cout << "\n--- static 成員屬於類別，不屬於物件 ---" << endl;
    Demo a, b;
    a.setInstance(100);              // 只改 a 自己的
    Demo::setStatic(999);            // 改的是「所有物件共用的那一份」
    cout << "  a.instanceVar_ = " << a.instanceVar() << endl;
    cout << "  b.instanceVar_ = " << b.instanceVar() << "（不受 a 影響）" << endl;
    cout << "  Demo::staticFunc() → ";
    Demo::staticFunc();
    cout << "  sizeof(Demo) = " << sizeof(Demo)
         << " bytes（== sizeof(int)，static 成員不佔物件空間）" << endl;

    cout << "\n--- 成員函數指標 vs 一般函式指標 ---" << endl;
    void (Demo::*memPtr)() = &Demo::normalFunc;   // 帶隱藏 this
    void (*funPtr)()       = &Demo::staticFunc;   // 純函式指標，可交給 C API
    cout << "  sizeof(成員函數指標) = " << sizeof(memPtr)
         << " bytes（實作定義：Itanium ABI 需容納 this 調整量）" << endl;
    cout << "  sizeof(一般函式指標) = " << sizeof(funPtr) << " bytes" << endl;
    (void)funPtr;

    cout << "\n=== 日常實務：C 風格回呼的 trampoline ===" << endl;
    MetricsCollector collector("http-server");
    MiniEventLoop loop;
    // C API 只吃函式指標；用 static 成員函數當跳板，物件位址走 void* 傳進去
    loop.setCallback(&MetricsCollector::trampoline, &collector);
    loop.post("request.begin");
    loop.post("request.end");
    loop.post("connection.close");
    loop.run();
    cout << "  最終累計筆數 = " << collector.count() << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 26 課：this 指標7.cpp" -o this7
// （版本驗證：以 g++ -std=c++14 -pedantic-errors 編譯會失敗，
//   錯誤訊息指向 inline static 資料成員 —— 證明本檔確實需要 C++17。）

// 注意事項（輸出相關）：
//   * 本檔刻意**不列印任何指標位址**。位址受 ASLR 影響每次執行都不同，
//     寫進預期輸出必然對不上。要驗證身分請印布林值（this == &d），
//     這是決定性的。
//   * sizeof(Demo) = 4 與 sizeof(一般函式指標) = 8 是本機 g++ 15.2 / x86-64
//     的實測值，屬**實作定義**，在其他平台（如 32-bit）會不同。
//   * sizeof(成員函數指標) = 16 同樣是實作定義：Itanium C++ ABI 用
//     「函式位址 + this 調整量」兩個欄位表示，以支援多重繼承與虛擬函式。
//     MSVC 的成員函數指標大小甚至會隨繼承模型變動（4/8/12/16 都可能）。
//   * 事件回呼的輸出順序完全決定性：MiniEventLoop 以 vector 依序走訪，
//     不涉及執行緒或雜湊容器走訪。

// === 預期輸出 ===
// === this 與靜態成員 ===
//
// --- 普通函數（有 this）---
//   this 是否非空： true
//   instanceVar_ = 10
//   staticVar_ = 20
//   呼叫端驗證 this == &d ？ true
//
// --- 靜態函數（沒有 this）---
//   staticVar_ = 20
//
// --- static 成員屬於類別，不屬於物件 ---
//   a.instanceVar_ = 100
//   b.instanceVar_ = 10（不受 a 影響）
//   Demo::staticFunc() →   staticVar_ = 999
//   sizeof(Demo) = 4 bytes（== sizeof(int)，static 成員不佔物件空間）
//
// --- 成員函數指標 vs 一般函式指標 ---
//   sizeof(成員函數指標) = 16 bytes（實作定義：Itanium ABI 需容納 this 調整量）
//   sizeof(一般函式指標) = 8 bytes
//
// === 日常實務：C 風格回呼的 trampoline ===
//     [http-server] 收到事件 request.begin（累計 1 筆）
//     [http-server] 收到事件 request.end（累計 2 筆）
//     [http-server] 收到事件 connection.close（累計 3 筆）
//   最終累計筆數 = 3
