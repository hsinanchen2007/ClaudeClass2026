// =============================================================================
//  第八課：函數物件 10  —  Lambda 的捕獲：值捕獲 vs 參考捕獲
// =============================================================================
//
// 【主題資訊 Information】
//   捕獲列表的六種寫法：
//       []          什麼都不捕獲（可轉成函數指標）
//       [x]         以**值**捕獲 x（複製一份存進 closure）
//       [&x]        以**參考**捕獲 x（存的是位址，可改到外部）
//       [=]         預設全部以值捕獲（不建議，見注意事項）
//       [&]         預設全部以參考捕獲（不建議）
//       [x = expr]  init capture / 廣義捕獲，**C++14**
//   標準版本：基本捕獲 C++11；init capture 與 [*this] 分別是 C++14 / C++17。
//   本質：捕獲的變數會變成 closure 類別的**成員變數**——
//         值捕獲存的是複本，參考捕獲存的是參考（實作上通常是指標）。
//
// 【詳細解釋 Explanation】
//
// 【1. 捕獲就是「決定 closure 的成員變數長什麼樣」】
//   回到第 9 檔的心智模型：lambda 是編譯器生成的類別。
//       int t = 5;
//       auto f = [t](int n) { return n > t; };
//   展開後大致是：
//       class __lambda { int t;  public: __lambda(int t_) : t(t_) {}
//                        bool operator()(int n) const { return n > t; } };
//   捕獲列表就是在寫這個類別的**成員變數與建構子**。
//   [t] 產生 int t;（複本）；[&t] 產生 int& t;（參考）。
//   理解這一點，捕獲的所有行為都變得理所當然。
//
// 【2. 捕獲的時機是「定義 lambda 的那一刻」，不是呼叫時】
//   這是值捕獲最重要、也最常被誤解的一點：
//       int x = 1;
//       auto f = [x] { return x; };     // 這一行就把 1 複製進去了
//       x = 100;
//       f();                            // 仍然回傳 1，不是 100
//   值捕獲是**快照**。想要「呼叫時才取值」就得用參考捕獲，
//   或者乾脆把它改成參數傳進去。
//
// 【3. 參考捕獲的代價：生命週期】
//   參考捕獲存的是外部變數的位址。
//   **一旦那個變數死了，lambda 就握著懸空參考**，用它就是未定義行為。
//   這在下面三種情境特別致命：
//     (a) 把 lambda 存起來、稍後才呼叫（callback、event handler）
//     (b) 把 lambda 回傳出函式 —— 區域變數已經銷毀
//     (c) 把 lambda 丟給另一個執行緒 / std::async —— 原函式可能已經返回
//   準則：**lambda 只在當下用完就丟（例如傳給演算法）→ 參考捕獲安全；
//         lambda 會活得比當前作用域久 → 一律值捕獲**。
//
// 【4. 為什麼不建議用 [=] 和 [&]】
//   兩個理由：
//     * **看不出捕獲了什麼**。讀者得逐行掃 lambda 本體才知道它依賴哪些外部狀態，
//       而那正是 review 時最需要一眼看清的資訊。
//     * [=] 給人「全部複製、很安全」的錯覺，其實**不會複製指標指向的東西**，
//       更關鍵的是它在成員函式中會**捕獲 this 指標**（見概念補充 B）。
//   明確列出要捕獲的變數，是主流風格指南（Google、LLVM、Core Guidelines）
//   一致的建議。
//
// 【概念補充 Concept Deep Dive】
//
// (A) init capture（C++14）解決了兩個難題
//       auto f = [p = std::move(bigVector)] { ... };    // 把只能移動的東西搬進去
//       auto g = [n = compute()] { return n; };          // 存「算好的值」而非變數
//     C++11 的捕獲只能抓「已存在的變數」，無法移動、也無法存運算結果。
//     init capture 讓 closure 的成員可以任意初始化——
//     這是把 std::unique_ptr 放進 lambda 的唯一辦法。
//     ★ 但捕獲 unique_ptr 後 closure 就變成 **move-only**，
//       塞不進 std::function（它要求可複製建構，實測會是
//       「std::function target must be copy-constructible」的靜態斷言失敗）。
//       要放進 std::function 得改用 shared_ptr，
//       或用 C++23 的 std::move_only_function。
//
// (B) 在成員函式裡，[=] 捕獲的是 this，不是成員變數
//     struct S {
//         int v = 42;
//         auto get() { return [=] { return v; }; }    // 實際捕獲的是 this！
//     };
//     [=] 看起來「複製了 v」，實際上複製的是 **this 指標**，
//     v 是透過 this->v 存取的。所以物件銷毀後這個 lambda 就懸空了。
//     C++20 起 [=] 隱式捕獲 this 已被**棄用**，要明寫 [this] 或 [*this]。
//     C++17 的 [*this] 才是真正「複製整個物件」。
//
// (C) 值捕獲的變數預設是 const 的
//     因為 operator() 預設帶 const，成員變數在其中不可改。
//     要改必須加 mutable（第 13 檔）——但改的仍然是副本。
//
// (D) 捕獲會影響 sizeof 與「能否轉成函數指標」
//     無捕獲       sizeof = 1，可以轉成函數指標
//     捕獲一個 int  sizeof = 4，不能轉
//     捕獲兩個 int  sizeof = 8，不能轉
//     這再次印證「捕獲 = closure 的成員變數」。
//
// 【注意事項 Pay Attention】
//   1. 值捕獲是**定義當下的快照**，之後改外部變數不影響它。
//   2. 參考捕獲要確保被捕獲的變數**活得比 lambda 久**，
//      否則是懸空參考（未定義行為）。
//   3. 存起來稍後才呼叫、回傳出函式、跨執行緒 → 一律值捕獲。
//   4. 避免 [=] / [&]，明確列出捕獲項目。
//   5. 成員函式中的 [=] 捕獲的是 this（C++20 已棄用這個隱式行為），
//      要複製物件本身請用 C++17 的 [*this]。
//   6. 值捕獲的變數在 lambda 內是 const，要改需 mutable（改的是副本）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Lambda 的捕獲機制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 值捕獲和參考捕獲，各自在什麼時候「取得」變數的值？
//     答：值捕獲在**定義 lambda 的那一刻**就把值複製進 closure，之後
//         外部變數怎麼變都不影響它——它是個快照。
//         參考捕獲存的是位址，每次呼叫時才去讀外部變數的**當前值**。
//         這也代表參考捕獲有生命週期風險：外部變數死了，
//         lambda 就握著懸空參考。
//     追問：那什麼時候該用哪個？
//         → lambda 當下用完就丟（傳給演算法）→ 參考捕獲安全又省複製。
//           lambda 會活得比當前作用域久（存起來、回傳、跨執行緒）
//           → 一律值捕獲。
//
// 🔥 Q2. 這段程式有什麼問題？
//        std::function<int()> makeCounter() {
//            int count = 0;
//            return [&count] { return ++count; };
//        }
//     答：回傳的 lambda 以參考捕獲了區域變數 count，
//         但函式一返回 count 就銷毀了，lambda 握著**懸空參考**，
//         呼叫它是未定義行為。
//         修法是改成值捕獲並加 mutable：
//             return [count]() mutable { return ++count; };
//         這樣 count 的複本存活在 closure 物件裡，跟著 lambda 一起活。
//     追問：如果要捕獲的是 std::unique_ptr 這種只能移動的東西呢？
//         → 用 C++14 的 init capture：[p = std::move(ptr)] { ... }。
//           C++11 的捕獲語法做不到，這正是 init capture 被加入的原因之一。
//
// ⚠️ 陷阱. 「我在成員函式裡寫 [=]，它會把成員變數 v 複製一份，
//         所以物件銷毀後 lambda 還是安全的吧？」
//     答：**不安全**。在成員函式中，[=] 捕獲的是 **this 指標**，
//         不是成員變數本身。lambda 裡的 v 實際上是 this->v。
//         物件一銷毀，this 就懸空，lambda 再被呼叫就是未定義行為。
//         這個陷阱特別危險，因為 [=] 三個字看起來就是「全部複製」。
//         C++20 起這個隱式捕獲 this 的行為已被**棄用**，
//         必須明寫 [this]（捕獲指標）或 [*this]（C++17，真正複製整個物件）。
//     為什麼會錯：把 [=] 讀成「複製所有用到的東西」。
//         正確的規則是「複製所有用到的**區域變數與參數**」，
//         而成員變數不屬於這兩者——存取它們一律要透過 this，
//         所以能被捕獲的只有 this 這個指標。
//         同理，[=] 也不會深複製指標指向的資料。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <algorithm>
#include <numeric>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：捕獲的難點在**生命週期**——lambda 存活得比被捕獲的變數久時會怎樣。
//   LeetCode 的解法幾乎都是「在單一函式內建 lambda、當場傳給演算法、
//   函式結束就一起消失」，這個結構下值捕獲與參考捕獲**行為完全相同**，
//   永遠不會踩到懸空參考。
//   換句話說，LeetCode 的環境剛好把本檔最重要的風險完全隱藏起來，
//   拿它當範例反而會給讀者「捕獲怎麼寫都沒差」的錯誤印象。
//   下面的實務範例改用「回呼註冊」情境，那才是這個 bug 真正會發生的地方。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】事件回呼註冊：捕獲方式選錯就是懸空參考
//   情境：UI 或伺服器框架常見的「註冊一個 callback，事件發生時才呼叫」。
//         註冊函式返回後，區域變數就銷毀了，但 callback 還活著。
//   為什麼用到本主題：這是「lambda 活得比被捕獲的變數久」的典型場景，
//     也是參考捕獲最容易出事的地方。
//     下面示範**安全的做法**（值捕獲 / init capture 搬移所有權），
//     並在註解說明錯誤版本為什麼危險——
//     但**不實際執行**懸空參考的版本，因為那是未定義行為，
//     它的輸出不可預測，不該被當成「預期輸出」印出來。
// -----------------------------------------------------------------------------
class EventBus {
public:
    void subscribe(std::string topic, std::function<void(const std::string&)> handler) {
        handlers_.emplace_back(std::move(topic), std::move(handler));
    }

    void publish(const std::string& topic, const std::string& payload) const {
        for (const auto& h : handlers_)
            if (h.first == topic) h.second(payload);
    }

    std::size_t count() const { return handlers_.size(); }

private:
    std::vector<std::pair<std::string, std::function<void(const std::string&)>>> handlers_;
};

// ✓ 安全：需要的狀態全部以「值」捕獲，複本活在 closure 裡
void registerOrderHandler(EventBus& bus, int retryLimit) {
    std::string prefix = "[order]";      // 區域變數：函式返回後就銷毀

    // 值捕獲 → prefix 與 retryLimit 各複製一份進 closure，
    // 之後 publish 時仍然有效
    bus.subscribe("order.created", [prefix, retryLimit](const std::string& payload) {
        std::cout << "    " << prefix << " 收到訂單 " << payload
                  << "（重試上限 " << retryLimit << "）" << std::endl;
    });

    // ✗ 危險版本（刻意不啟用）：
    //   bus.subscribe("order.created", [&prefix](const std::string& p) {
    //       std::cout << prefix << p;      // 函式返回後 prefix 已死 → 懸空參考
    //   });
    //   這是未定義行為：可能印出亂碼、可能崩潰、也可能「看起來正常」。
}

// ✓ C++14 init capture：把資源搬進 closure
//
// ★ 這裡有個必須知道的限制（實際編譯時踩到的）：
//   std::function 要求它包裝的可呼叫物件是**可複製建構**的。
//   若用 [p = std::move(uniquePtr)] 捕獲，closure 就變成 move-only，
//   塞進 std::function 會編譯失敗：
//       error: static assertion failed:
//              std::function target must be copy-constructible
//   兩種解法：
//     (a) 改用 std::shared_ptr（可複製）—— 本例採用
//     (b) 用 C++23 的 std::move_only_function（本課基準是 C++17，暫不使用）
//   下面 main 裡另有一個 unique_ptr 的 init capture 示範，
//   那個用 auto 直接持有 closure，沒有經過 std::function，所以完全合法。
void registerAuditHandler(EventBus& bus) {
    auto logFile = std::make_shared<std::string>("audit.log");

    // [p = logFile] 是 C++14 的 init capture；shared_ptr 可複製，
    // 所以這個 closure 塞得進 std::function
    bus.subscribe("order.created", [p = logFile](const std::string& payload) {
        std::cout << "    [audit -> " << *p << "] 記錄 " << payload << std::endl;
    });
}

int main() {
    int threshold = 5;
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 值捕獲 [threshold] 或 [=]
    std::cout << "=== 值捕獲 ===" << std::endl;
    int count1 = std::count_if(vec.begin(), vec.end(),
        [threshold](int n) { return n > threshold; });
    std::cout << "大於 " << threshold << " 的個數: " << count1 << std::endl;

    // 值捕獲是「複製」，Lambda 內部的修改不影響外部
    auto by_value = [threshold]() mutable {  // mutable 允許修改捕獲的副本
        threshold = 100;  // 修改的是副本
        return threshold;
    };
    by_value();
    std::cout << "外部 threshold 仍然是: " << threshold << std::endl;

    // 參考捕獲 [&threshold] 或 [&]
    std::cout << "\n=== 參考捕獲 ===" << std::endl;
    int sum = 0;
    std::for_each(vec.begin(), vec.end(),
        [&sum](int n) { sum += n; });  // sum 以參考捕獲
    std::cout << "總和: " << sum << std::endl;

    // 參考捕獲可以修改外部變數
    int multiplier = 2;
    auto by_ref = [&multiplier]() {
        multiplier = 10;  // 修改外部變數
    };
    by_ref();
    std::cout << "multiplier 被修改為: " << multiplier << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 捕獲的時機：定義當下，不是呼叫時 ===" << std::endl;
    {
        int x = 1;
        auto byVal = [x]  { return x; };      // 這一行就把 1 複製進去了
        auto byRef = [&x] { return x; };      // 存的是位址

        x = 100;                               // 定義之後才改

        std::cout << "定義 lambda 後把 x 改成 100：" << std::endl;
        std::cout << "  值捕獲   [x]  回傳 " << byVal()
                  << "   ← 快照，仍是定義當下的 1" << std::endl;
        std::cout << "  參考捕獲 [&x] 回傳 " << byRef()
                  << " ← 每次呼叫才讀，看到最新的 100" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 捕獲 = closure 的成員變數 ===" << std::endl;
    {
        int a = 1, b = 2;
        auto none    = []             { return 0; };
        auto one     = [a]            { return a; };
        auto two     = [a, b]         { return a + b; };
        auto refOne  = [&a]           { return a; };

        std::cout << "sizeof(無捕獲)        = " << sizeof(none) << std::endl;
        std::cout << "sizeof(捕獲一個 int)  = " << sizeof(one) << std::endl;
        std::cout << "sizeof(捕獲兩個 int)  = " << sizeof(two) << std::endl;
        std::cout << "sizeof(參考捕獲一個)  = " << sizeof(refOne)
                  << "（存的是位址，本機為指標大小）" << std::endl;
        std::cout << "→ 捕獲什麼，closure 就長出什麼成員變數。" << std::endl;

        // 只有無捕獲的能轉成函數指標
        int (*fp)() = none;
        std::cout << "無捕獲可轉成函數指標: fp() = " << fp() << std::endl;
        std::cout << "有捕獲的不行 —— 一個位址放不下狀態。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 為什麼不建議 [=] 和 [&] ===" << std::endl;
    {
        int used1 = 10, used2 = 20, unrelated = 30;

        // 明確捕獲：一眼看出這個 lambda 依賴哪兩個變數
        auto explicitCap = [used1, used2] { return used1 + used2; };
        // [=]：得逐行掃本體才知道它用了什麼
        auto implicitCap = [=] { return used1 + used2; };

        std::cout << "明確捕獲 [used1, used2] = " << explicitCap() << std::endl;
        std::cout << "隱式捕獲 [=]            = " << implicitCap() << std::endl;
        std::cout << "  兩者結果相同，但前者讓 review 時一眼看出依賴。" << std::endl;
        std::cout << "  （unrelated=" << unrelated
                  << " 沒被用到，所以實際上也不會被捕獲）" << std::endl;
        std::cout << "→ 更關鍵的是：在成員函式裡 [=] 捕獲的是 **this 指標**，"
                  << std::endl;
        std::cout << "  不是成員變數的複本。物件銷毀後 lambda 就懸空了。"
                  << std::endl;
        std::cout << "  C++20 已棄用 [=] 隱式捕獲 this；"
                  << "要複製物件請用 C++17 的 [*this]。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 生命週期：回傳 lambda 時必須值捕獲 ===" << std::endl;
    {
        // ✗ 危險（刻意不執行）：
        //   auto makeBad = []{ int c = 0; return [&c]{ return ++c; }; };
        //   回傳的 lambda 參考了已銷毀的區域變數 c → 懸空參考 → 未定義行為

        // ✓ 正解：值捕獲 + mutable，複本活在 closure 物件裡
        auto makeCounter = [] {
            int count = 0;
            return [count]() mutable { return ++count; };
        };

        auto c1 = makeCounter();
        auto c2 = makeCounter();      // 各自獨立的計數器
        std::cout << "c1(): " << c1() << " " << c1() << " " << c1() << std::endl;
        std::cout << "c2(): " << c2() << " " << c2() << std::endl;
        std::cout << "→ 值捕獲的複本跟著 closure 物件一起活，"
                  << "所以回傳出去也安全。" << std::endl;
        std::cout << "  兩個計數器互不干擾——各自有自己的成員變數。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== init capture（C++14）：搬移只能移動的資源 ===" << std::endl;
    {
        auto data = std::make_unique<std::vector<int>>(
            std::initializer_list<int>{3, 1, 4, 1, 5});

        // C++11 的 [data] 會嘗試複製 unique_ptr → 編譯失敗
        // C++14 的 init capture 可以把所有權搬進 closure
        auto owner = [p = std::move(data)] {
            return std::accumulate(p->begin(), p->end(), 0);
        };

        std::cout << "把 unique_ptr 搬進 lambda 後計算總和: " << owner() << std::endl;
        std::cout << "原本的 data 還持有資源嗎? " << std::boolalpha
                  << (data != nullptr) << "（已被 move 走）" << std::endl;
        std::cout << "★ 但注意：捕獲了 unique_ptr 的 closure 是 **move-only**，"
                  << std::endl;
        std::cout << "  塞進 std::function 會編譯失敗（它要求可複製建構）。"
                  << std::endl;
        std::cout << "  這裡用 auto 直接持有才沒問題；" << std::endl;
        std::cout << "  要放進 std::function 請改用 shared_ptr，"
                  << "或 C++23 的 std::move_only_function。" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：事件回呼註冊 ===" << std::endl;
    {
        EventBus bus;

        registerOrderHandler(bus, 3);     // 內部的 prefix 是區域變數
        registerAuditHandler(bus);        // 內部用 init capture 搬 unique_ptr

        std::cout << "已註冊 " << bus.count() << " 個 handler" << std::endl;
        std::cout << "  （註冊函式**都已經返回**，區域變數全部銷毀了）" << std::endl;

        std::cout << "發布事件 order.created:" << std::endl;
        bus.publish("order.created", "ORD-20260719-001");
        bus.publish("order.created", "ORD-20260719-002");

        std::cout << "→ 因為用值捕獲 / init capture，"
                  << "狀態的複本活在 closure 裡，" << std::endl;
        std::cout << "  註冊函式返回後仍然有效。" << std::endl;
        std::cout << "  若當初寫成 [&prefix]，這裡就是懸空參考——" << std::endl;
        std::cout << "  未定義行為，可能印亂碼、可能崩潰、"
                  << "也可能「看起來正常」而潛伏很久。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探10.cpp -o demo10
//
// ⚠️ 本檔用到 init capture [p = std::move(x)]（C++14），最低需要 -std=c++14。

// === 預期輸出 ===
// === 值捕獲 ===
// 大於 5 的個數: 5
// 外部 threshold 仍然是: 5
//
// === 參考捕獲 ===
// 總和: 55
// multiplier 被修改為: 10
//
// === 捕獲的時機：定義當下，不是呼叫時 ===
// 定義 lambda 後把 x 改成 100：
//   值捕獲   [x]  回傳 1   ← 快照，仍是定義當下的 1
//   參考捕獲 [&x] 回傳 100 ← 每次呼叫才讀，看到最新的 100
//
// === 捕獲 = closure 的成員變數 ===
// sizeof(無捕獲)        = 1
// sizeof(捕獲一個 int)  = 4
// sizeof(捕獲兩個 int)  = 8
// sizeof(參考捕獲一個)  = 8（存的是位址，本機為指標大小）
// → 捕獲什麼，closure 就長出什麼成員變數。
// 無捕獲可轉成函數指標: fp() = 0
// 有捕獲的不行 —— 一個位址放不下狀態。
//
// === 為什麼不建議 [=] 和 [&] ===
// 明確捕獲 [used1, used2] = 30
// 隱式捕獲 [=]            = 30
//   兩者結果相同，但前者讓 review 時一眼看出依賴。
//   （unrelated=30 沒被用到，所以實際上也不會被捕獲）
// → 更關鍵的是：在成員函式裡 [=] 捕獲的是 **this 指標**，
//   不是成員變數的複本。物件銷毀後 lambda 就懸空了。
//   C++20 已棄用 [=] 隱式捕獲 this；要複製物件請用 C++17 的 [*this]。
//
// === 生命週期：回傳 lambda 時必須值捕獲 ===
// c1(): 1 2 3
// c2(): 1 2
// → 值捕獲的複本跟著 closure 物件一起活，所以回傳出去也安全。
//   兩個計數器互不干擾——各自有自己的成員變數。
//
// === init capture（C++14）：搬移只能移動的資源 ===
// 把 unique_ptr 搬進 lambda 後計算總和: 14
// 原本的 data 還持有資源嗎? false（已被 move 走）
// ★ 但注意：捕獲了 unique_ptr 的 closure 是 **move-only**，
//   塞進 std::function 會編譯失敗（它要求可複製建構）。
//   這裡用 auto 直接持有才沒問題；
//   要放進 std::function 請改用 shared_ptr，或 C++23 的 std::move_only_function。
//
// === 日常實務：事件回呼註冊 ===
// 已註冊 2 個 handler
//   （註冊函式**都已經返回**，區域變數全部銷毀了）
// 發布事件 order.created:
//     [order] 收到訂單 ORD-20260719-001（重試上限 3）
//     [audit -> audit.log] 記錄 ORD-20260719-001
//     [order] 收到訂單 ORD-20260719-002（重試上限 3）
//     [audit -> audit.log] 記錄 ORD-20260719-002
// → 因為用值捕獲 / init capture，狀態的複本活在 closure 裡，
//   註冊函式返回後仍然有效。
//   若當初寫成 [&prefix]，這裡就是懸空參考——
//   未定義行為，可能印亂碼、可能崩潰、也可能「看起來正常」而潛伏很久。
