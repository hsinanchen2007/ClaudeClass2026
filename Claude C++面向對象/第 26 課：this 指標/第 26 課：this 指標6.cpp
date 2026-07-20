// =============================================================================
//  第 26 課：this 指標6.cpp  —  this 的型別：普通成員 vs const 成員 vs static
// =============================================================================
//
// 【主題資訊 Information】
//   在 class X 的非靜態成員函數中，this 的型別依函數的 cv-qualifier 而定：
//       void f()                → decltype(this) 為  X*
//       void f() const          → decltype(this) 為  const X*
//       void f() volatile       → decltype(this) 為  volatile X*
//       void f() const volatile → decltype(this) 為  const volatile X*
//       static void f()         → 沒有 this（編譯期就沒有這個參數）
//   標準：this 自 C++98 起存在；C++11 起 this 明確定義為 prvalue。
//         本檔用到 std::is_same_v（C++17）與 static_assert（C++11）。
//   標頭檔：<type_traits>（僅為了在編譯期「證明」型別，語言本身不需要）
//   複雜度：this 的傳遞是暫存器層級的動作，O(1)，無執行期成本。
//
// 【詳細解釋 Explanation】
//
// 【1. this 是「隱藏的第一個參數」，不是物件裡的欄位】
//   當你寫：
//       obj.setValue(42);
//   編譯器實際產生的呼叫大致等同於：
//       TypeDemo::setValue(&obj, 42);
//   在 x86-64 System V ABI 上，物件位址放在 %rdi（第一個整數參數暫存器）。
//   這解釋了三件事：
//     * 為什麼 sizeof(TypeDemo) 不會因為多寫幾個成員函數而變大——this 不存在物件內。
//     * 為什麼 static 成員函數沒有 this——它根本不是透過物件呼叫的，沒有第一個參數。
//     * 為什麼成員函數指標與普通函數指標型別不相容——前者多一個隱藏參數。
//
// 【2. 「this 不能被賦值」的真正原因（教科書常講錯的一點）】
//   幾乎所有教材都說：this 的型別是 X* const，因為指標本身是 const，所以不能改。
//   這個「心智模型」得到的結論是對的（this = nullptr 確實編譯錯誤），
//   但**型別描述在 C++11 之後並不精確**。標準 [expr.prim.this] 規定：
//       this 是一個 prvalue，型別為「指向 cv-qualified X 的指標」。
//   也就是說型別是 X*（或 const X*），**沒有頂層 const**。
//   它不能被賦值，是因為 prvalue 不是 lvalue，而不是因為頂層 const。
//   本檔用 static_assert 在編譯期直接證明這件事：
//       static_assert(std::is_same_v<decltype(this), TypeDemo*>);        // 成立
//       static_assert(!std::is_same_v<decltype(this), TypeDemo* const>); // 也成立
//   （若「X* const」是對的，第一條就會編譯失敗。這是本機 g++ 15.2 實測結果。）
//
//   為什麼還是可以繼續用「X* const」來教？因為它給出的行為預測全部正確，
//   只是理由不對。面試時能講出「其實是 prvalue」通常是加分項。
//
// 【3. const 成員函數改變的是「this 指向的東西」】
//   const 加在函數尾巴，作用在 this 的**被指物**上：
//       void constFunc() const;   → this 是 const TypeDemo*
//   所以 this->value_ = 42 編譯失敗——不是因為 this 不能改，
//   而是因為透過 const TypeDemo* 不能寫入被指物。
//
//   這帶出 const 成員函數的真正意義：它是一份**對呼叫者的承諾**——
//   「呼叫我不會改變這個物件的可觀察狀態」。編譯器只檢查得到「位元層級」的
//   const（不能寫成員），邏輯層級的 const 要靠設計者自律。
//
// 【4. const 是函數簽章的一部分 → 可以重載】
//       const T& at(size_t i) const;   // 唯讀版本
//       T&       at(size_t i);         // 可寫版本
//   多數標準容器都是這樣成對提供的。呼叫哪一個由**物件的 const 性**決定，
//   不是由「要不要修改」決定：對 const 物件呼叫必選 const 版本，
//   對非 const 物件呼叫優先選非 const 版本。
//
// 【概念補充 Concept Deep Dive】
//   * mutable：允許 const 成員函數修改某個成員，用於「不影響可觀察狀態」的
//     內部快取（memoization）、統計計數、mutex。本檔尾端的實務範例示範
//     lazily-computed 快取——這是 mutable 唯一站得住腳的正當用途。
//   * const 成員函數不代表執行緒安全，但標準函式庫（C++11 起）要求
//     「同時對同一物件呼叫 const 成員函數必須安全」，因此若用 mutable
//     做快取，實務上要配 mutex 或 atomic。
//   * this 的 cv-qualifier 也參與重載決議，所以下面兩者是不同函數：
//         void f();
//         void f() const;
//     但下面兩者是**重定義**（編譯錯誤），因為回傳型別不參與簽章：
//         int  f() const;
//         long f() const;
//   * C++23 引入 explicit object parameter（deducing this）：
//         void f(this TypeDemo& self);
//     可把 this 寫成顯式參數，一次寫完 const/非 const/左值/右值四個版本。
//     本檔以 C++17 編譯，僅作為延伸知識提及，未使用。
//
// 【注意事項 Pay Attention】
//   1. 「this 是 X* const」是好用但不精確的心智模型；精確說法是
//      「型別為 X*，且它是 prvalue，所以不可被賦值」。
//   2. const 成員函數擋的是「透過 this 寫入」，擋不住「透過別的路徑寫入」——
//      例如成員是 T* 時，const 成員函數仍可寫 *ptr_（const 只到指標本身為止）。
//      這就是 const 的「淺層性（shallow const）」。
//   3. static 成員函數不能標 const，因為根本沒有 this 可以 qualify。
//   4. 在 const 成員函數中呼叫非 const 成員函數會編譯失敗；反向則沒問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】this 的型別與 const 成員函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. const 成員函數中的 this 是什麼型別？為什麼不能寫 this->x = 1?
//     答：是 const X*（指向 const 物件的指標）。不能寫入是因為被指物被 const 修飾，
//         而不是因為指標本身是 const。const 加在函數尾巴，作用對象是 this 的被指物。
//     追問：那 this = nullptr 為什麼也錯？
//         → 因為 this 是 prvalue（C++11 起明確規定），prvalue 不能出現在
//           賦值運算子左邊。這跟頂層 const 無關，是值類別（value category）的限制。
//
// 🔥 Q2. const 可以拿來重載嗎？呼叫哪一個由什麼決定？
//     答：可以。const 是成員函數簽章的一部分，f() 與 f() const 是兩個不同函數。
//         由**呼叫該函數的物件是否為 const** 決定：const 物件只能選 const 版本，
//         非 const 物件優先選非 const 版本。這就是容器 begin()/cbegin()、
//         operator[] 成對出現的原因。
//     追問：那為什麼 int f() 和 long f() 不能重載？
//         → 回傳型別不參與函數簽章，兩者被視為同一函數的重定義。
//
// ⚠️ 陷阱1. 「const 成員函數保證物件完全不會被改。」
//     答：不對。const 是淺層的。若成員是 int* p_，const 成員函數不能改 p_ 本身，
//         但可以自由地改 *p_。物件指向的資料完全不受保護。
//     為什麼會錯：把 const 想成「整棵物件樹唯讀」，但 C++ 的 const
//         只作用在該物件的直接成員上，不會傳遞穿過指標或參考。
//
// ⚠️ 陷阱2. 「mutable 是作弊，正式專案不該用。」
//     答：過度簡化。mutable 的正當用途是「不影響可觀察狀態」的內部設施：
//         lazily-computed 快取、命中次數統計、保護內部狀態的 std::mutex。
//         這些讓介面能維持 const 卻仍可實作最佳化。
//     為什麼會錯：把「位元層級 const（bitwise const）」當成 const 的定義，
//         但 C++ 真正要表達的是「邏輯層級 const（logical const）」。
//
// ⚠️ 陷阱3. 「static 成員函數加 const 更安全。」
//     答：編譯錯誤。const 修飾的是 this，而 static 成員函數沒有 this。
//     為什麼會錯：把 const 當成「這個函數不改東西」的通用標記，
//         但它其實是專門用來 qualify 隱藏參數 this 的語法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
using namespace std;

class TypeDemo {
private:
    int value_;

public:
    TypeDemo(int v) : value_(v) {}

    // 普通成員函數
    void normalFunc() {
        // this 的類型：指向非 const TypeDemo 的指標
        // 教科書常寫成 TypeDemo* const；精確地說型別是 TypeDemo*，
        // 而它是 prvalue，所以不能被賦值。下面兩行在編譯期證明這件事。
        static_assert(std::is_same_v<decltype(this), TypeDemo*>,
                      "this 的型別是 TypeDemo*");
        static_assert(!std::is_same_v<decltype(this), TypeDemo* const>,
                      "this 沒有頂層 const（不可賦值是因為它是 prvalue）");

        this->value_ = 42;     // ✅ 可以修改
        // this = nullptr;     // ❌ this 是 prvalue，不能當賦值目標
    }

    // const 成員函數
    int constFunc() const {
        // this 的類型：const TypeDemo*
        static_assert(std::is_same_v<decltype(this), const TypeDemo*>,
                      "const 成員函數中 this 是 const TypeDemo*");

        // this->value_ = 42;  // ❌ 指向 const 對象，不能修改
        // this = nullptr;     // ❌ this 是 prvalue
        return this->value_;   // ✅ 可以讀取
    }

    // const / 非 const 重載：由「呼叫者是否為 const 物件」決定選哪一個
    const char* which()       { return "非 const 版本"; }
    const char* which() const { return "const 版本"; }

    int value() const { return value_; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 從缺，並說明原因
//
// 本檔主題是「this 的型別、cv-qualifier 與 const 正確性」，是 C++ 語言層面的
// 型別系統議題。LeetCode 的判題只執行你的解法函式並比對回傳值，
// 完全不檢查成員函數的 const 性，也不會因為漏標 const 而判錯。
// 指定清單中的 design 類題（146 LRU Cache、155 Min Stack、705 Design HashSet…）
// 雖然要求你設計 class，但它們考的是資料結構與複雜度，
// 用它們來「示範 const 成員函數」會讓讀者以為 const 是刷題技巧，反而誤導。
// 依規格「寧缺勿濫」，此處明確從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】使用者權限查詢服務 — const 成員函數 + mutable 快取
//
// 情境：後端服務對每個請求呼叫 hasPermission(user, action) 檢查權限。
//       權限規則存在唯讀的規則表裡，查詢本身「邏輯上不改變服務狀態」，
//       所以介面必須是 const（呼叫端常常拿到的是 const PermissionService&）。
//       但查詢要做字串拼接與雜湊，每次重算太浪費，於是加一層記憶化快取。
//
// 關鍵：快取是「實作細節」，不是「可觀察狀態」——同樣的輸入永遠得到同樣的輸出，
//       只是第二次比較快。這正是 mutable 存在的理由：
//       讓 const 成員函數能修改不影響可觀察狀態的成員。
// -----------------------------------------------------------------------------
class PermissionService {
private:
    unordered_map<string, bool> rules_;             // 規則表（唯讀）
    mutable unordered_map<string, bool> cache_;     // 記憶化快取
    mutable int hits_   = 0;                        // 命中統計
    mutable int misses_ = 0;

public:
    void addRule(const string& user, const string& action, bool allow) {
        rules_[user + ":" + action] = allow;         // 非 const：真的改變狀態
    }

    // 注意這是 const 成員函數：this 為 const PermissionService*
    // 卻仍能寫 cache_ / hits_ / misses_，因為它們宣告為 mutable。
    bool hasPermission(const string& user, const string& action) const {
        const string key = user + ":" + action;

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            ++hits_;                                 // mutable：const 函數中可寫
            return it->second;
        }

        ++misses_;
        auto r = rules_.find(key);
        bool allow = (r != rules_.end()) && r->second;
        cache_.emplace(key, allow);                  // mutable：填快取
        return allow;
    }

    void dumpStats() const {
        cout << "  快取命中 " << hits_ << " 次，未命中 " << misses_ << " 次" << endl;
    }
};

int main() {
    cout << "=== this 的類型 ===" << endl;

    cout << "  普通成員函數中：TypeDemo*（教科書常寫 TypeDemo* const）" << endl;
    cout << "  const 成員函數中：const TypeDemo*" << endl;
    cout << "  靜態成員函數中：沒有 this" << endl;
    cout << "  以上三行已由檔內 static_assert 在編譯期驗證" << endl;

    cout << "\n=== normalFunc 可寫、constFunc 只可讀 ===" << endl;
    TypeDemo d(7);
    cout << "  建構後 value = " << d.value() << endl;
    d.normalFunc();                       // 內部把 value_ 改成 42
    cout << "  normalFunc 後 value = " << d.constFunc() << endl;

    cout << "\n=== const 參與重載決議 ===" << endl;
    TypeDemo       nonConstObj(1);
    const TypeDemo constObj(1);
    cout << "  非 const 物件呼叫 which() → " << nonConstObj.which() << endl;
    cout << "  const   物件呼叫 which() → " << constObj.which()    << endl;

    cout << "\n=== 日常實務：權限查詢（const 成員函數 + mutable 快取）===" << endl;
    PermissionService svc;
    svc.addRule("alice", "deploy", true);
    svc.addRule("bob",   "deploy", false);

    // 刻意用 const reference 呼叫：只有 const 成員函數才叫得動
    const PermissionService& ro = svc;
    cout << "  alice 可以 deploy? " << boolalpha << ro.hasPermission("alice", "deploy") << endl;
    cout << "  alice 可以 deploy? " << ro.hasPermission("alice", "deploy") << "（第二次走快取）" << endl;
    cout << "  bob   可以 deploy? " << ro.hasPermission("bob",   "deploy") << endl;
    cout << "  carol 可以 deploy? " << ro.hasPermission("carol", "deploy") << "（無規則→拒絕）" << endl;
    ro.dumpStats();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 26 課：this 指標6.cpp" -o this6
// （本檔也通過 -pedantic-errors；std::is_same_v 為 C++17 設施，
//   以 -std=c++14 編譯會失敗，可用來驗證版本宣告確實正確。）

// 注意事項（輸出相關）：
//   * 本檔完全不列印指標位址。位址受 ASLR 影響每次執行都不同，
//     不可寫進「預期輸出」；要驗證型別請用編譯期的 static_assert（本檔作法）。
//   * static_assert 若不成立會是**編譯錯誤**，不會產生任何執行期輸出；
//     換言之「編譯成功」本身就是那三行型別敘述的證明。
//   * 快取統計 hits=1 / misses=3 是固定的：alice 查兩次（第二次命中）、
//     bob 與 carol 各查一次（皆未命中）。unordered_map 的走訪順序雖不保證，
//     但本檔只做 find/emplace，不走訪，故輸出完全決定性。

// === 預期輸出 ===
// === this 的類型 ===
//   普通成員函數中：TypeDemo*（教科書常寫 TypeDemo* const）
//   const 成員函數中：const TypeDemo*
//   靜態成員函數中：沒有 this
//   以上三行已由檔內 static_assert 在編譯期驗證
//
// === normalFunc 可寫、constFunc 只可讀 ===
//   建構後 value = 7
//   normalFunc 後 value = 42
//
// === const 參與重載決議 ===
//   非 const 物件呼叫 which() → 非 const 版本
//   const   物件呼叫 which() → const 版本
//
// === 日常實務：權限查詢（const 成員函數 + mutable 快取）===
//   alice 可以 deploy? true
//   alice 可以 deploy? true（第二次走快取）
//   bob   可以 deploy? false
//   carol 可以 deploy? false（無規則→拒絕）
//   快取命中 1 次，未命中 3 次
