// =============================================================================
//  第 2.7 章 - 2  —  重載組合爆炸：2^N 個建構子，或 1 個轉發建構子
// =============================================================================
//
// 【主題資訊 Information】
//   本檔要示範的核心式子：
//       template<typename S, typename V>
//       Service(S&& prefix, V&& data)
//           : logger_(std::forward<S>(prefix))
//           , data_(std::forward<V>(data)) {}
//
//   標準版本：forwarding reference / std::forward 為 C++11。
//             本檔另示範的 fold expression 與 if constexpr 為 **C++17**
//             （不是 C++11，實測 -std=c++14 -pedantic-errors 會直接報錯）。
//             本檔以 C++17 編譯。
//   標頭檔：<utility>（std::forward）、<vector>、<string>
//   成本：完美轉發是純編譯期機制，執行期不產生任何中介物件。
//
// 【詳細解釋 Explanation】
//
// 【1. 組合爆炸：為什麼「多寫幾個重載」不是辦法】
//   Service 要收兩個參數，每個參數都可能是左值或右值。若不用完美轉發、
//   而是老老實實把每種情況寫出來，就是 2 個參數 × 2 種值類別 = 4 個建構子：
//
//       Service(const std::string&,  const std::vector<int>&)   // 左, 左
//       Service(const std::string&,  std::vector<int>&&)        // 左, 右
//       Service(std::string&&,       const std::vector<int>&)   // 右, 左
//       Service(std::string&&,       std::vector<int>&&)        // 右, 右
//
//   參數變三個就要 8 個，四個就要 16 個——**2^N**。而且這 4 份程式碼
//   內容幾乎一模一樣，只差在有沒有 std::move，是維護災難：
//   哪天多一個成員，四個地方都要改，漏改一個就是一個效能 bug（而且不會編譯錯，
//   只會默默變慢，最難抓）。
//
//   一個轉發建構子就取代了全部 2^N 個。這是完美轉發最直接的價值。
//
// 【2. 為什麼不能退一步「全部用 const&」就好】
//   全用 const& 只要寫 1 個建構子，看起來也解決了爆炸問題——代價是
//   **右值再也移動不了**。
//   本檔的 Logger 刻意寫了兩個建構子並各自印出訊息，就是要讓這件事被看見：
//       Logger(const std::string&)  → 印「複製 prefix」
//       Logger(std::string&&)       → 印「移動 prefix」
//   如果 Service 收的是 const&，那麼不管呼叫端傳什麼，Logger 永遠只會印
//   「複製」。std::vector<int> 的差別更大：複製要配置新記憶體並逐一搬元素，
//   移動只是搬三個指標。
//
//   所以三種寫法的取捨是：
//       寫法              建構子數量    右值能移動嗎    move-only 型別能傳嗎
//       2^N 個重載            2^N          ✅               ✅
//       全部 const&            1           ❌               ❌
//       完美轉發               1           ✅               ✅
//
// 【3. 成員初始化順序的坑（與轉發一起出現時特別危險）】
//   成員的初始化順序由**宣告順序**決定，不是由初始化列表的書寫順序決定。
//   本檔 Service 宣告順序是 logger_、data_，所以先初始化 logger_。
//   這在完美轉發下要特別小心：如果兩個成員都從「同一個」被轉發的參數取值，
//   先初始化的那個會把內容搬走，後初始化的那個就拿到空的。
//   本檔兩個成員各自吃不同參數，所以安全。
//
// 【4. 轉發建構子的副作用：它會搶走複製建構】
//   一旦寫了 template<class S, class V> Service(S&&, V&&)，這個模板對
//   「兩個參數」的呼叫非常貪心。本檔因為需要兩個參數，剛好不會跟
//   單參數的複製建構子 Service(const Service&) 打架。
//   但只要是**單參數**的轉發建構子，就一定會搶走複製建構——
//   那是本章第 3、4 檔的主題，也是完美轉發最有名的陷阱。
//
// 【概念補充 Concept Deep Dive】
//
// ── reference collapsing：為什麼一份程式碼能同時是四份 ──────────────
//   關鍵在於 S 與 V 各自獨立推導，各自記住自己那一邊是左值還是右值：
//
//     Service s3(name, std::vector<int>{100, 200});
//       name 是左值   → S 推導為 std::string&
//                      S&& = std::string& &&  --摺疊--> std::string&
//                      forward<std::string&> 回傳左值 → Logger 走「複製」
//       {100,200} 是右值 → V 推導為 std::vector<int>
//                      V&& = std::vector<int>&&（不需摺疊）
//                      forward<std::vector<int>> 回傳右值 → vector 走「移動」
//
//   摺疊規則（[dcl.ref]/6）：**有左值引用就是左值引用，全右值才是右值引用**
//        T& &  → T&      T&& &  → T&
//        T& && → T&      T&& && → T&&
//
//   兩個參數各走各的，就自然覆蓋了 4 種組合中的第 3 種。
//   編譯器實際上會為每一種用到的組合各實例化一份程式碼——
//   也就是說 2^N 份程式碼還是存在，只是由編譯器生成，不必人來維護。
//
// ── 這件事的代價：程式碼膨脹與錯誤訊息 ──────────────────────────────
//   完美轉發把「人工重載」換成「編譯器實例化」，因此：
//     * 二進位檔可能變大（每種引數組合一份）
//     * 編譯變慢
//     * 型別錯誤會在**實例化時**才爆，錯誤訊息又長又難讀
//       （C++20 concepts 就是為了讓錯誤提早在介面處被擋下）
//   這是完美轉發真實的取捨，不是白吃的午餐。
//
// ── 一次只能轉發一次 ────────────────────────────────────────────────
//   被轉發的參數要當成「許可只發一張」。若一個參數要餵給多個下游，
//   只有**最後一個**使用點可以 forward，前面都要用具名左值。
//   本檔的 broadcast 實務範例會把這條規則實際跑出來。
//
// 【注意事項 Pay Attention】
//   1. 被移動走的物件處於 **valid but unspecified**（有效但未指定）狀態：
//      可以安全解構、可以重新賦值，但不可假設它還留著原值，也不可假設
//      它一定變成空的。本檔輸出中「移動後長度」只作為本機實測觀察，
//      不是標準保證的結果。
//   2. 成員初始化順序看宣告順序，不看初始化列表順序。
//   3. 轉發建構子與複製建構子的競爭：單參數版一定要用 enable_if / concepts
//      約束（第 3、4 檔）。
//   4. fold expression、if constexpr 是 C++17，不是 C++11。寫 C++11 專案時
//      要改用遞迴展開 + 標籤分派或 std::initializer_list 展開技巧。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】重載組合爆炸與完美轉發
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個吃 N 個參數的建構子，若不用完美轉發要寫幾個重載？為什麼？
//     答：2^N 個。每個參數都可能是左值或右值，要讓每種情況都能走到最佳路徑
//         （左值複製、右值移動），就得把所有組合列出來。兩個參數 4 個、
//         三個參數 8 個，且每份程式碼幾乎相同，只差 std::move，維護成本極高。
//         一個 template<class... A> C(A&&...) 就全部取代。
//     追問：那編譯器是不是也產生了 2^N 份程式碼？→ 是，但只針對**實際用到的**
//           組合實例化。差別在於這 2^N 份由編譯器維護，不是由人維護。
//
// 🔥 Q2. 全部參數都用 const& 收，不是也只要寫一個建構子嗎？差在哪？
//     答：差在右值再也移動不了。const& 會把值類別資訊抹掉，臨時物件被綁成
//         const& 之後，下游只剩複製一條路。另外 move-only 型別
//         （std::unique_ptr、std::thread）根本無法用 const& 傳遞。
//     追問：那什麼時候 const& 反而比較好？→ 當函式只是「讀取」引數、
//           不打算儲存或轉交時。完美轉發只在「要把引數交給下游建構/儲存」
//           時才有意義；純讀取用 const& 更簡單，也不會有模板膨脹。
//
// 🔥 Q3. Service 有兩個成員，成員初始化的順序由什麼決定？
//     答：由**成員在類別中的宣告順序**決定，與建構子初始化列表的書寫順序無關。
//         本檔宣告順序是 logger_、data_，所以 logger_ 一定先初始化。
//         GCC 開 -Wreorder（-Wall 已含）會在兩者不一致時警告。
//     追問：這跟完美轉發有什麼關係？→ 若兩個成員都從同一個被轉發的參數取值，
//           先初始化的會把內容搬走，後初始化的拿到的是 valid but unspecified
//           的殘骸——這種 bug 只在特定宣告順序下出現，極難察覺。
//
// ⚠️ 陷阱. 這個「廣播給多個 handler」的寫法錯在哪？
//         template<class Event, class... Handlers>
//         void broadcast(Event&& e, Handlers&&... hs) {
//             (hs(std::forward<Event>(e)), ...);   // C++17 fold
//         }
//     答：同一個 e 被轉發給每一個 handler。若呼叫端傳入右值，第一個 handler
//         就可能把內容搬走，第二個以後拿到的是 valid but unspecified 的物件。
//         正確做法：前面的 handler 都收具名左值 e，只有**最後一個**才 forward
//         （或乾脆全部都不 forward，用左值廣播）。
//     為什麼會錯：直覺把 std::forward 當成「原封不動地傳遞」，
//         但它真正的意思是「把可以被搬走的許可交出去」。
//         許可發一張叫轉發，發 N 張叫重複移動。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Logger {
    std::string prefix_;

public:
    Logger(const std::string& p) : prefix_(p) {
        std::cout << "  Logger: 複製 prefix\n";
    }
    Logger(std::string&& p) : prefix_(std::move(p)) {
        std::cout << "  Logger: 移動 prefix\n";
    }

    void log(const std::string& msg) const {
        std::cout << "  [" << prefix_ << "] " << msg << "\n";
    }
};

class Service {
    Logger logger_;
    std::vector<int> data_;

public:
    // 不用完美轉發：需要寫多個建構子
    // Service(const std::string& prefix, const std::vector<int>& data)
    //     : logger_(prefix), data_(data) {}
    // Service(std::string&& prefix, std::vector<int>&& data)
    //     : logger_(std::move(prefix)), data_(std::move(data)) {}
    // Service(const std::string& prefix, std::vector<int>&& data) ...
    // Service(std::string&& prefix, const std::vector<int>& data) ...
    // → 4 個建構子！

    // 用完美轉發：一個搞定
    template<typename S, typename V>
    Service(S&& prefix, V&& data)
        : logger_(std::forward<S>(prefix))
        , data_(std::forward<V>(data))
    {}

    void run() const {
        logger_.log("Service started with " +
                    std::to_string(data_.size()) + " items");
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】事件分發器：為什麼「廣播」不可以轉發、「單播」才可以
//
// 情境：一個訂閱／發佈系統。事件物件內含 payload（會配置記憶體），
//       希望盡量避免複製。但事件要送給**多個** handler。
//
// 規則：一個引數的「可被移動」許可只能發一次。
//       * dispatch_one（單播）：只有一個下游 → 可以放心 forward
//       * broadcast（廣播）  ：多個下游     → 前面全部用左值，
//                                             只有最後一個能 forward
// -----------------------------------------------------------------------------
struct Event {
    std::string topic;
    std::string payload;

    Event(std::string t, std::string p)
        : topic(std::move(t)), payload(std::move(p)) {}

    Event(const Event& o) : topic(o.topic), payload(o.payload) {
        std::cout << "    (Event 被複製)\n";
    }
    Event(Event&& o) noexcept : topic(std::move(o.topic)), payload(std::move(o.payload)) {
        std::cout << "    (Event 被移動)\n";
    }
};

// 單播：只有一個下游，可以安全轉發
template<typename Handler, typename E>
void dispatch_one(Handler&& h, E&& e) {
    std::forward<Handler>(h)(std::forward<E>(e));
}

// 廣播：N 個下游。前 N-1 個必須用左值，否則第二個以後拿到的是被搬走的殘骸。
// 這裡示範最安全的做法——全部用 const 左值廣播，一次都不移動。
template<typename E, typename... Handlers>
void broadcast(const E& e, Handlers&&... hs) {
    // C++17 fold expression：對每個 handler 展開一次呼叫，用逗號串起來
    // （C++11/14 沒有這個語法，實測 -std=c++14 -pedantic-errors 直接報錯）
    (std::forward<Handlers>(hs)(e), ...);
}

int main() {
    std::string name = "MyApp";
    std::vector<int> nums = {1, 2, 3, 4, 5};

    std::cout << "=== 全部左值 ===\n";
    Service s1(name, nums);
    s1.run();

    std::cout << "\n=== 全部右值 ===\n";
    Service s2(std::string("Worker"), std::vector<int>{10, 20, 30});
    s2.run();

    std::cout << "\n=== 混合（左值 + 右值，一個建構子全包）===\n";
    Service s3(name, std::vector<int>{100, 200});
    s3.run();

    std::cout << "\n=== 日常實務：單播可以轉發 ===\n";
    {
        auto archive = [](Event ev) {
            std::cout << "    archive 收到 [" << ev.topic << "] "
                      << ev.payload << "\n";
        };
        Event e{"order.created", "{\"id\":1001}"};
        std::cout << "  以右值單播（會走移動）:\n";
        dispatch_one(archive, std::move(e));
        // e 現在處於 valid but unspecified 狀態，不再讀取它的內容
    }

    std::cout << "\n=== 日常實務：廣播只能用左值 ===\n";
    {
        auto audit  = [](const Event& ev) {
            std::cout << "    audit  收到 [" << ev.topic << "]\n";
        };
        auto notify = [](const Event& ev) {
            std::cout << "    notify 收到 [" << ev.topic << "] "
                      << ev.payload << "\n";
        };
        auto metric = [](const Event& ev) {
            std::cout << "    metric 收到 [" << ev.topic << "]\n";
        };

        Event e{"order.paid", "{\"id\":1001,\"amt\":250}"};
        broadcast(e, audit, notify, metric);
        // 全程沒有複製也沒有移動：三個 handler 都收 const&
        std::cout << "  廣播後 e 仍完好: [" << e.topic << "] " << e.payload << "\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.7 章：完美轉發 (Perfect Forwarding) — 泛型程式設計的關鍵技術2.cpp" -o pf2

// === 預期輸出 ===
// === 全部左值 ===
//   Logger: 複製 prefix
//   [MyApp] Service started with 5 items
//
// === 全部右值 ===
//   Logger: 移動 prefix
//   [Worker] Service started with 3 items
//
// === 混合（左值 + 右值，一個建構子全包）===
//   Logger: 複製 prefix
//   [MyApp] Service started with 2 items
//
// === 日常實務：單播可以轉發 ===
//   以右值單播（會走移動）:
//     (Event 被移動)
//     archive 收到 [order.created] {"id":1001}
//
// === 日常實務：廣播只能用左值 ===
//     audit  收到 [order.paid]
//     notify 收到 [order.paid] {"id":1001,"amt":250}
//     metric 收到 [order.paid]
//   廣播後 e 仍完好: [order.paid] {"id":1001,"amt":250}
