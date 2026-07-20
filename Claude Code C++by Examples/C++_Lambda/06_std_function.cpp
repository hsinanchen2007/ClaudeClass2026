// =============================================================================
//  06_std_function.cpp  —  std::function 包裝可呼叫物件
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/utility/functional/function
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要 std::function？                               │
//  └────────────────────────────────────────────────────────────┘
//
//  問題：兩個「看起來一樣」的 lambda 是「不同型別」 — 沒辦法塞進同一個
//  容器、也沒辦法當成同一個 function pointer 傳。
//
//      auto a = [](int x) { return x + 1; };
//      auto b = [](int x) { return x + 2; };
//      // a 跟 b 是兩個 unique 的 closure type，不能 a = b;
//      // std::vector<???> 不知道該裝什麼型別
//
//  std::function 是個「型別擦除 (type erasure) 容器」，可以包裝：
//   * 一般函式
//   * 函式指標
//   * 成員函式（搭配物件）
//   * lambda（含捕獲）
//   * 任何具 operator() 的物件
//
//  只要它們的 (參數型別 → 回傳型別) 一致：
//
//      std::function<int(int)> f;     // 簽名是 int(int)
//      f = a;                         // OK
//      f = b;                         // OK，動態替換
//      std::vector<std::function<int(int)>> v{a, b};   // OK 同型別
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、成本                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  std::function 用 type erasure 實作，內部會：
//   * 對小 closure 做 small buffer optimization (SBO，免 heap)
//   * 對大 closure 做 heap allocation
//   * 透過 virtual / function pointer 派發 → 多一次間接呼叫
//
//  熱迴圈裡頻繁建立大量 std::function 可能變慢；如果不需要「型別統一」的
//  好處，能直接用 lambda 模板就直接用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、最重要應用                                             │
//  └────────────────────────────────────────────────────────────┘
//
//   1. callback 註冊（事件、handler 容器）
//   2. 遞迴 lambda — lambda 不能在自己的 body 裡引用自己（因為自己還沒命名
//      完成），用 std::function 包就可以
//   3. 跨模組函式傳遞（API 邊界）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔範例與 LeetCode                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  * Demo 1：std::function 容器
//  * Demo 2：遞迴 lambda 的標準寫法
//  * LeetCode 700. Search in a Binary Search Tree
// =============================================================================

/*
補充筆記：std_function
  - std::function 是 type-erased callable wrapper，可保存不同型別的函式物件。
  - 它可能配置記憶體，也可能讓呼叫無法 inline；熱路徑要評估成本。
  - 只接收 callable 參數而不保存時，模板參數通常比 std::function 更輕。
  - std_function 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::function 與型別擦除
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lambda（用 auto／模板參數接）、std::function、函式指標三者怎麼選？
//     答：函式指標不能帶狀態、通常難以 inline；lambda 以 auto 或模板參數傳遞時型別確定，
//     可完全 inline，開銷最小；std::function 是型別擦除的統一型別，代價是間接呼叫、
//     難以 inline，而且可能有堆配置。原則：能用 auto／模板參數就用；只有在需要把「不同
//     型別的可呼叫物」放進同一個變數、容器或成員時，才用 std::function。
//     追問：為什麼 std::sort 傳 lambda 通常比傳函式指標快？（lambda 的型別是模板參數，
//     operator() 可被 inline；函式指標對 sort 只是一個不透明的值）
//
// 🔥 Q2. std::function 的型別擦除是怎麼做的？為什麼有開銷？
//     答：它要把任意可呼叫物存進同一個型別，做法是內部保留一塊儲存區，加上一組操作
//     （呼叫／複製／解構／取 type_info），透過虛擬函式或函式指標表分派。骨架大致是：
//     struct Base { virtual R call(Args...) = 0; virtual Base* clone() const = 0; ... };
//     template<class F> struct Impl : Base { F f; R call(Args... a) override { ... } };
//     代價是多一層間接呼叫、難以 inline，且可呼叫物太大時要堆配置。
//
// Q3. std::function 什麼時候會發生堆配置？
//     答：實作通常有 small buffer optimization：可呼叫物夠小（且一般要求 nothrow-move）
//     就就地存放，否則配置到堆上。libstdc++ 的 buffer 約 16 bytes，但這是
//     implementation-defined、標準未規定門檻甚至未規定一定要有 SBO。實務重點是：在熱
//     路徑上把 lambda 塞進 std::function，可能引入看不見的 malloc。
//
// ⚠️ 陷阱. 呼叫一個空的 std::function 會怎樣？它能存捕獲 unique_ptr 的 lambda 嗎？
//     答：呼叫空的 std::function 會拋 std::bad_function_call，不是 UB、不是直接崩潰
//     （這與「呼叫空的函式指標是 UB」不同）。而 std::function 要求可呼叫物是
//     CopyConstructible，所以捕獲了 unique_ptr 的 lambda 存不進去；C++23 的
//     std::move_only_function 才補上這個缺口。
//     為什麼會錯：大家把 std::function 想成「聰明的函式指標」，於是既預期空呼叫是 UB，
//     又預期它能裝任何 lambda。
// ═══════════════════════════════════════════════════════════════════════════

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：std::function 容器 — 收集多個 callback 一次跑
    // ─────────────────────────────────────────────────────────
    using Callback = std::function<void(const std::string&)>;
    std::vector<Callback> handlers;

    handlers.emplace_back(
        [](const std::string& s) { std::cout << "[log]   " << s << '\n'; });
    handlers.emplace_back(
        [prefix = std::string{"!!ALERT!! "}](const std::string& s) {
            std::cout << "[alert] " << prefix << s << '\n';
        });

    auto fire = [&](const std::string& msg) {
        for (auto& h : handlers) h(msg);
    };
    fire("door opened");

    // ─────────────────────────────────────────────────────────
    // Demo 2：遞迴 lambda — 用 std::function 解決「自己引用自己」
    //   Lambda 在 body 裡無法直接寫自己的名字（因為還沒指派完成），
    //   把它包成 std::function 之後，name 在 init 階段就先存在了。
    // ─────────────────────────────────────────────────────────
    std::function<long long(int)> fact = [&fact](int n) -> long long {
        return n <= 1 ? 1LL : n * fact(n - 1);
    };
    std::cout << "[Demo2] fact(10) = " << fact(10) << '\n'; // 3628800

    // ─────────────────────────────────────────────────────────
    // LeetCode 700. Search in a Binary Search Tree
    //   題意：BST 中找值為 val 的節點，回傳該節點 (含子樹)；找不到回傳 null。
    //
    //   為什麼放這？經典「遞迴 lambda」場景 — 在 main / leetcode solution
    //   裡不想寫成全域函式，就用 std::function 包成可遞迴的 lambda。
    // ─────────────────────────────────────────────────────────
    struct Node {
        int val;
        std::unique_ptr<Node> left, right;
        explicit Node(int v) : val(v) {}
    };
    // 建一棵簡單 BST：
    //          4
    //         / \
    //        2   7
    //       / \
    //      1   3
    auto root = std::make_unique<Node>(4);
    root->left  = std::make_unique<Node>(2);
    root->right = std::make_unique<Node>(7);
    root->left->left  = std::make_unique<Node>(1);
    root->left->right = std::make_unique<Node>(3);

    std::function<const Node*(const Node*, int)> search =
        [&](const Node* n, int v) -> const Node* {
            if (!n) return nullptr;
            if (n->val == v) return n;
            return v < n->val ? search(n->left.get(),  v)
                              : search(n->right.get(), v);
        };

    auto* hit  = search(root.get(), 2);
    auto* miss = search(root.get(), 5);
    std::cout << "[LC700] search(2)  -> " << (hit  ? "found" : "null") << '\n';
    std::cout << "[LC700] search(5)  -> " << (miss ? "found" : "null") << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 std::function 建立「指令名 → 行為」的 dispatcher
    //   工作上常見：簡單 CLI / 工具命令分派；不同 lambda 都包成 std::function
    //   就能塞進同一個 std::map。
    // ─────────────────────────────────────────────────────────
    {
        using Action = std::function<void(const std::string&)>;
        // 不同 closure type、不同捕獲，但都符合 void(const string&)
        std::vector<std::pair<std::string, Action>> commands;
        commands.emplace_back("hello",
            [](const std::string& arg) {
                std::cout << "  hello, " << arg << "!\n";
            });
        commands.emplace_back("echo",
            [prefix = std::string{"[echo] "}](const std::string& arg) {
                std::cout << "  " << prefix << arg << '\n';
            });

        auto dispatch = [&](const std::string& cmd, const std::string& arg) {
            for (auto& [name, action] : commands) {
                if (name == cmd) { action(arg); return; }
            }
            std::cout << "  unknown command: " << cmd << '\n';
        };
        std::cout << "[dispatcher]\n";
        dispatch("hello", "world");
        dispatch("echo",  "test 123");
        dispatch("nope",  "x");
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::function 跟 function pointer 差在哪？
    //    A：function pointer 只能指向「無捕獲的 lambda 或一般函式」，沒有
    //       狀態。std::function 能裝任何狀態 — 所以是「胖指標」。
    //
    //  Q2：std::function 為什麼會 throw bad_function_call？
    //    A：空的 std::function（沒有指派任何可呼叫物件）被呼叫時會丟。
    //       使用前可先用 if (f) 檢查。
    //
    //  Q3：替代品有哪些？
    //    A：C++23 新增 std::move_only_function（可裝 unique_ptr 等不可拷
    //       貝的物件）；template + auto 參數也能避開 std::function 的成本，
    //       缺點是不能塞進同質容器。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 06_std_function.cpp -o 06_std_function

// === 預期輸出 ===
// [log]   door opened
// [alert] !!ALERT!! door opened
// [Demo2] fact(10) = 3628800
// [LC700] search(2)  -> found
// [LC700] search(5)  -> null
// [dispatcher]
//   hello, world!
//   [echo] test 123
//   unknown command: nope
