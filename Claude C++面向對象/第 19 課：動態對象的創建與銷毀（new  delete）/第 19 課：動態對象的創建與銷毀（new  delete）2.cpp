// =============================================================================
//  第 19 課：動態對象的創建與銷毀 2  —  new vs malloc 的本質差異
// =============================================================================
//
// 【主題資訊 Information】
//   對照：
//     malloc(size)  → 只配置原始位元組，回傳 void*，失敗回傳 NULL
//     new T(args)   → 配置記憶體 + 呼叫建構函式，回傳 T*，失敗拋 std::bad_alloc
//     free(p)       → 只歸還記憶體，不呼叫任何解構函式
//     delete p      → 呼叫解構函式 + 歸還記憶體
//   標頭檔：<cstdlib>（malloc/free）、<new>（placement new、bad_alloc）
//   標準版本：placement new 自 C++98 起即在 <new> 中。
//
// 【詳細解釋 Explanation】
//
// 【1. 四個關鍵差異】
//   ┌────────────┬──────────────────────┬─────────────────────────┐
//   │            │ malloc / free        │ new / delete            │
//   ├────────────┼──────────────────────┼─────────────────────────┤
//   │ 建構/解構  │ 完全不呼叫           │ 一定呼叫                │
//   │ 型別安全   │ 回傳 void*，要轉型   │ 回傳 T*，不需轉型       │
//   │ 大小計算   │ 手動 sizeof(T)*n     │ 編譯器自動算            │
//   │ 失敗處理   │ 回傳 NULL            │ 拋 std::bad_alloc       │
//   └────────────┴──────────────────────┴─────────────────────────┘
//   其中「建構/解構」是本質差異，其餘三項只是便利性。
//
// 【2. 為什麼對類別物件用 malloc 是災難？】
//   Widget* w = (Widget*)malloc(sizeof(Widget));
//   此時 w 指向一塊「大小正確、但內容是垃圾」的記憶體：
//     ● 成員 std::string name 沒有被建構——它內部的指標是垃圾值，
//       一旦使用（甚至只是離開作用域）就是未定義行為。
//     ● 成員 int* data 沒有被配置。
//     ● 若類別有虛擬函式，vptr 也沒被設定，呼叫虛擬函式會跳到亂七八糟的位址。
//   而 free(w) 同樣不呼叫解構函式，內部資源全部洩漏。
//   ★ 所以規則很簡單：C++ 的類別物件一律用 new/delete（或更好：智慧指標）。
//     malloc/free 只在「與 C 介接」或「自己寫配置器」時才會出現。
//
// 【3. 但兩者其實可以拆開組合——placement new】
//   new 做的兩件事（配置 + 建構）可以分離：
//       void* mem = std::malloc(sizeof(Widget));   // ① 只配置
//       Widget* w = new (mem) Widget("...");       // ② 只建構（placement new）
//       w->~Widget();                              // ③ 只解構（明確呼叫）
//       std::free(mem);                            // ④ 只釋放
//   這是完全合法且正確的——本檔會實際執行這個流程給你看。
//   ★ 這正是 std::vector 的運作方式：一次配置一大塊（capacity），
//     再依需要逐一 placement new 建構元素（size）。
//     這也解釋了為什麼 vector 的 capacity 與 size 是兩個不同的概念。
//   ★ 注意 ③：用 placement new 建構的物件，
//     絕對不可以用 delete 釋放（記憶體不是 operator new 給的），
//     必須「明確呼叫解構函式」再用對應的方式釋放記憶體。
//
// 【4. new 底層其實會呼叫 operator new，而它通常用 malloc 實作】
//   所以 new 並不是「malloc 的競爭者」，而是「建立在它之上的一層」：
//       new T(args)
//         → operator new(sizeof(T))     ← 多半內部呼叫 malloc
//         → T 的建構函式
//   你甚至可以多載 operator new / operator delete 來換掉配置策略
//   （記憶體池、統計配置次數、偵錯用的邊界檢查等），
//   而完全不影響上層的 new T 語法。
//
// 【概念補充 Concept Deep Dive】
//   ● 不可以混用：malloc 配置的必須 free，new 的必須 delete，
//     new[] 的必須 delete[]。混用是未定義行為，因為兩套系統的
//     記錄方式（標頭、大小資訊）不同。
//   ● realloc 對 C++ 物件是危險的：它可能「按位元組搬移」記憶體，
//     這對「有自訂複製建構函式或內部自我指標」的型別是錯的。
//     C++ 中要調整大小應該用 std::vector。
//   ● C++ 的 new 也可以被多載成「不拋例外」的版本：
//       new (std::nothrow) T;   // 失敗回傳 nullptr，行為較接近 malloc
//     詳見 5.cpp。
//   ● calloc 會把記憶體歸零，但「全零的位元組」不等於「已正確建構的物件」——
//     對 std::string 而言，全零仍然不是一個合法的 string。
//
// 【注意事項 Pay Attention】
//   1. C++ 類別物件不可用 malloc/free 管理——建構與解構都不會發生。
//   2. malloc 失敗回傳 NULL，new 失敗拋 std::bad_alloc；
//      對 new 寫 `if (p == nullptr)` 是無效的檢查（除非用 nothrow 版）。
//   3. 配置與釋放必須同一套：malloc↔free、new↔delete、new[]↔delete[]。
//   4. placement new 建構的物件，必須明確呼叫解構函式，且不可用 delete。
//   5. 「malloc 之後不碰它就 free」是安全的（本檔如此示範）；
//      一旦讀寫未建構的成員就是未定義行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】new vs malloc
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. new 和 malloc 的差別是什麼？哪一個是本質差異？
//     答：四點——建構/解構、型別安全、大小自動計算、失敗處理方式。
//         其中「會不會呼叫建構與解構函式」是本質差異，其餘三項只是便利性。
//         malloc 只給你一塊大小正確的原始位元組，物件從未被建構；
//         free 也不會呼叫解構函式，內部資源必然洩漏。
//     追問：new 底層是不是就是 malloc？
//         → new 會呼叫 operator new，而它「通常」以 malloc 實作，
//           但這不是標準規定。重點是 new = operator new + 建構函式，
//           它是建立在配置函式之上的一層，不是 malloc 的替代品。
//
// 🔥 Q2. 什麼是 placement new？什麼時候會用到？
//     答：在「已經配置好的記憶體」上呼叫建構函式，不做任何配置：
//         Widget* w = new (mem) Widget(args);
//         它把 new 的兩件事拆開，讓配置與建構可以分別控制。
//         最典型的用途是 std::vector——先配置 capacity 這麼大的原始記憶體，
//         再依需要逐一建構元素，這就是 capacity 與 size 不同的原因。
//         記憶體池、自訂配置器也都靠它。
//     追問：placement new 建出來的物件怎麼釋放？
//         → 必須明確呼叫解構函式 w->~Widget();，
//           再用「與當初配置方式對應」的方法釋放記憶體（如 free）。
//           絕對不可以用 delete——那塊記憶體不是 operator new 給的。
//
// ⚠️ 陷阱. Widget* w = (Widget*)malloc(sizeof(Widget)); 之後 free(w);
//          中間完全沒用過 w。這樣有問題嗎？
//     答：這樣沒問題。配置了一塊原始記憶體又原封不動地歸還，
//         全程沒有任何物件被建立，也就沒有任何物件需要解構。
//         真正的未定義行為始於「把它當成 Widget 來讀寫」的那一刻。
//     為什麼會錯：很多人以為「(Widget*) 這個轉型本身就是 UB」，
//         或以為「malloc 之後就必須解構」。實際上轉型與配置都無害，
//         問題出在「型別聲稱有一個 Widget，但那裡從來沒有 Widget 被建構」。
//         分清楚「記憶體」與「物件」是兩件事——記憶體只是位元組，
//         物件是在其上由建構函式賦予的生命——這題就不會答錯，
//         也才能真正理解 placement new 為什麼是合法的。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cstdlib>
#include <new>        // placement new
using namespace std;

class Widget {
private:
    string name;
    int* data;

public:
    Widget(const string& n) : name(n) {
        data = new int[10];
        cout << "  [建構] " << name << "：分配了內部資源" << endl;
    }
    
    ~Widget() {
        delete[] data;
        cout << "  [解構] " << name << "：釋放了內部資源" << endl;
    }

    const string& getName() const { return name; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】自訂配置策略：統計配置次數與總位元組數
//   情境：想知道某段程式到底做了幾次堆積配置（效能調校的第一步）。
//   多載全域 operator new / operator delete 就能攔截所有 new，
//   而完全不必修改任何使用 new 的程式碼——這正是「new 是分層設計」的價值。
//   註：這裡刻意只統計「次數」與「位元組數」這類確定性的量，
//       不量測耗時（耗時每次執行都不同，無法寫成預期輸出）。
// -----------------------------------------------------------------------------
namespace alloc_stats {
    size_t newCount   = 0;
    size_t deleteCount = 0;
    size_t totalBytes = 0;
    bool   tracking   = false;

    void reset() { newCount = 0; deleteCount = 0; totalBytes = 0; }
}

void* operator new(size_t sz) {
    if (alloc_stats::tracking) {
        ++alloc_stats::newCount;
        alloc_stats::totalBytes += sz;
    }
    void* p = std::malloc(sz);           // operator new 通常就是這樣實作的
    if (p == nullptr) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    if (alloc_stats::tracking && p != nullptr) ++alloc_stats::deleteCount;
    std::free(p);
}
// C++14 起編譯器可能改呼叫 sized delete，一併提供以免統計漏計
void operator delete(void* p, size_t) noexcept {
    if (alloc_stats::tracking && p != nullptr) ++alloc_stats::deleteCount;
    std::free(p);
}

// 註：本檔不加 LeetCode 範例。
//     「new 與 malloc 的差異」是語言與執行期模型的議題，
//     LeetCode 的題目不會因為改用哪種配置方式而改變解法或複雜度，
//     硬掛一題只會失焦，故從缺。本課的 LeetCode 範例放在 1.cpp（707）
//     與 3.cpp（705），那裡的動態配置才是解題的必要成分。

int main() {
    cout << "=== new vs malloc ===" << endl;
    
    // 正確：使用 new, 會調用建構函數，內部資源被正確初始化
    // 錯誤：使用 malloc, 只分配了記憶體，但建構函數沒被調用，內部資源未初始化
    cout << "\n--- 使用 new ---" << endl;
    Widget* w1 = new Widget("正確的 Widget");
    delete w1;   // 解構函數被調用，內部資源被釋放
    
    // 錯誤示範（概念上）：如果用 malloc, 會導致未定義行為
    cout << "\n--- 如果用 malloc（概念說明）---" << endl;
    cout << "  Widget* w2 = (Widget*)malloc(sizeof(Widget));" << endl;
    cout << "  // 記憶體分配了，但建構函數沒被調用！" << endl;
    cout << "  // name 沒被初始化，data 沒被分配" << endl;
    cout << "  // 使用 w2 會導致未定義行為！" << endl;
    cout << "  // free(w2) 也不會調用解構函數，data 洩漏！" << endl;
    
    // 我們不實際執行 malloc，因為會崩潰

    // ====== 安全示範：配置了但不當成物件使用，再原封不動歸還 ======
    cout << "\n--- 安全示範：malloc 之後不碰它，直接 free ---" << endl;
    {
        void* raw = std::malloc(sizeof(Widget));   // 只是一塊原始位元組
        if (raw != nullptr) {
            cout << "  已配置 " << sizeof(Widget) << " bytes 原始記憶體" << endl;
            cout << "  （本機 g++ 15.2 實測值，屬實作定義）" << endl;
            cout << "  全程沒有建構任何 Widget，因此也不需要解構" << endl;
            std::free(raw);
            cout << "  已歸還記憶體，沒有未定義行為" << endl;
        }
    }

    // ====== placement new：把「配置」與「建構」拆開 ======
    cout << "\n--- placement new：正確地在既有記憶體上建構物件 ---" << endl;
    {
        void* mem = std::malloc(sizeof(Widget));   // ① 只配置
        if (mem != nullptr) {
            Widget* w = new (mem) Widget("placement new 建的");  // ② 只建構
            cout << "  物件名稱 = " << w->getName() << endl;
            w->~Widget();                          // ③ 只解構（不可用 delete！）
            std::free(mem);                        // ④ 只釋放
            cout << "  完整走完 配置→建構→解構→釋放 四步，沒有洩漏" << endl;
        }
    }

    // ====== 實務：統計配置次數 ======
    cout << "\n=== 日常實務：攔截 operator new 統計配置次數 ===" << endl;
    {
        // 長名稱：19 bytes，超過 libstdc++ 的 SSO 門檻（15 字元）
        alloc_stats::reset();
        alloc_stats::tracking = true;
        Widget* a = new Widget("受監控的 Widget");
        delete a;
        alloc_stats::tracking = false;

        size_t longNew = alloc_stats::newCount;
        size_t longDel = alloc_stats::deleteCount;

        // 短名稱：2 bytes，落在 SSO 內，字串不需配置
        alloc_stats::reset();
        alloc_stats::tracking = true;
        Widget* b = new Widget("ab");
        delete b;
        alloc_stats::tracking = false;

        cout << "  [長名稱 \"受監控的 Widget\"，19 bytes]" << endl;
        cout << "    operator new    = " << longNew << " 次" << endl;
        cout << "    operator delete = " << longDel << " 次" << endl;
        cout << "    來源：① 由字面量建的臨時 string  ② Widget 物件本身" << endl;
        cout << "          ③ 複製到成員 name          ④ 內部的 new int[10]" << endl;
        cout << "  [短名稱 \"ab\"，2 bytes]" << endl;
        cout << "    operator new    = " << alloc_stats::newCount << " 次" << endl;
        cout << "    operator delete = " << alloc_stats::deleteCount << " 次" << endl;
        cout << "    少掉的 2 次，是因為短字串走 SSO（小字串最佳化）不需配置" << endl;
        cout << "  → 配置與釋放次數相等，代表沒有洩漏" << endl;
        cout << "  → 教訓：一行 new Widget(...) 背後的配置次數，遠比表面上多" << endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 19 課：動態對象的創建與銷毀（new  delete）2.cpp" -o newdel2

// 【輸出說明】
//   1. sizeof(Widget) = 40 是本機 g++ 15.2 / x86-64 的實測值
//      （std::string 32 bytes + int* 8 bytes），屬實作定義，非標準保證。
//   2. 本檔不讀取任何未建構的記憶體，故無未定義行為，輸出完全可重現。
//   3. libstdc++ 的 SSO（小字串最佳化）門檻實測為 15 個字元：
//      長度 15 以內不配置，16 起才配置。此為實作定義，
//      libc++ 與 MSVC 的門檻不同。

// === 預期輸出 ===
// === new vs malloc ===
//
// --- 使用 new ---
//   [建構] 正確的 Widget：分配了內部資源
//   [解構] 正確的 Widget：釋放了內部資源
//
// --- 如果用 malloc（概念說明）---
//   Widget* w2 = (Widget*)malloc(sizeof(Widget));
//   // 記憶體分配了，但建構函數沒被調用！
//   // name 沒被初始化，data 沒被分配
//   // 使用 w2 會導致未定義行為！
//   // free(w2) 也不會調用解構函數，data 洩漏！
//
// --- 安全示範：malloc 之後不碰它，直接 free ---
//   已配置 40 bytes 原始記憶體
//   （本機 g++ 15.2 實測值，屬實作定義）
//   全程沒有建構任何 Widget，因此也不需要解構
//   已歸還記憶體，沒有未定義行為
//
// --- placement new：正確地在既有記憶體上建構物件 ---
//   [建構] placement new 建的：分配了內部資源
//   物件名稱 = placement new 建的
//   [解構] placement new 建的：釋放了內部資源
//   完整走完 配置→建構→解構→釋放 四步，沒有洩漏
//
// === 日常實務：攔截 operator new 統計配置次數 ===
//   [建構] 受監控的 Widget：分配了內部資源
//   [解構] 受監控的 Widget：釋放了內部資源
//   [建構] ab：分配了內部資源
//   [解構] ab：釋放了內部資源
//   [長名稱 "受監控的 Widget"，19 bytes]
//     operator new    = 4 次
//     operator delete = 4 次
//     來源：① 由字面量建的臨時 string  ② Widget 物件本身
//           ③ 複製到成員 name          ④ 內部的 new int[10]
//   [短名稱 "ab"，2 bytes]
//     operator new    = 2 次
//     operator delete = 2 次
//     少掉的 2 次，是因為短字串走 SSO（小字串最佳化）不需配置
//   → 配置與釋放次數相等，代表沒有洩漏
//   → 教訓：一行 new Widget(...) 背後的配置次數，遠比表面上多
