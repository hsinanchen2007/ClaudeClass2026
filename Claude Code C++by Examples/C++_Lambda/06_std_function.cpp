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
