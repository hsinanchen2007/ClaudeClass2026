/*=============================================================================
 * 檔名：21_SharedPtr.cpp
 * 主題：std::shared_ptr 與 std::weak_ptr - 共享所有權的智慧指標
 * 適合：學完 unique_ptr，遇到「同一份資源要被多個人共用」場景的人
 *
 * 【課題介紹】
 *   unique_ptr 強調「獨占」，但有些情境下：
 *
 *     - 同一個物件被「多個邏輯擁有者」一起持有
 *     - 「最後一個擁有者離開時」才釋放
 *
 *   例如圖 (Graph)、樹有複雜引用、observer 模式、cache 有外部使用者...
 *   這時用 std::shared_ptr：
 *
 *       「shared_ptr 透過『引用計數 (reference count)』追蹤目前有幾個
 *        shared_ptr 還持有資源；計數歸 0 時自動 delete。」
 *
 *   每次複製 shared_ptr → 計數 +1
 *   每個 shared_ptr 解構 → 計數 -1
 *   計數變 0 → 釋放資源
 *
 * 【常用 API】
 *   #include <memory>
 *
 *   auto p = std::make_shared<Foo>(args...);   // 推薦的建立方式
 *   p.use_count()                              // 看目前有幾個共享者
 *   *p / p->member                             // 跟 unique_ptr 一樣使用
 *   p.reset()                                  // 自己放棄；計數 -1
 *
 * 【為什麼推薦 make_shared 而不是 shared_ptr<T>(new T)】
 *   make_shared 會把「物件」與「控制塊 (引用計數)」一次配置在連續記憶體，
 *   省一次配置、cache locality 也好。
 *
 * 【shared_ptr 的死穴：循環引用 (cycle)】
 *   如果 A 持有 B 的 shared_ptr，B 又持有 A 的 shared_ptr，
 *   兩邊計數永遠 ≥ 1 → 永遠不會被釋放 → 記憶體洩漏。
 *   解法：把其中一邊改成 std::weak_ptr (弱參考)，不增加計數。
 *
 *       「weak_ptr 是『不增加計數的觀察者』，要使用時用 .lock() 取出
 *        一個 shared_ptr (若資源還在的話)。」
 *
 * 【日常實用範例】
 *   1. 多個物件共享一份設定 (Config)：示範引用計數行為。
 *   2. 樹節點：parent 用 weak_ptr 指向上層，避免循環。
 *
 * 【對應 Leetcode】938. Range Sum of BST
 *   題目簡述：給一棵 BST 與區間 [low, high]，回傳所有 low <= val <= high 的節點和。
 *   為什麼選這題：剛好可以用 shared_ptr 建一棵樹來練習。
 *   雖然 LeetCode 上的官方型別是 raw pointer (TreeNode*)，但用 shared_ptr 自建
 *   樹節點時，整棵樹會被「自動正確釋放」，初學者完全不用煩惱 delete。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/memory/shared_ptr
 *   https://en.cppreference.com/w/cpp/memory/weak_ptr
 *   https://cplusplus.com/reference/memory/shared_ptr/
 *=============================================================================*/


/*
補充筆記：SharedPtr
  - std::shared_ptr 表示共享所有權，內部有 control block 保存 reference count；最後一個 shared_ptr 消失時才刪除物件。
  - shared_ptr 可以複製，但每次複製都代表所有權關係變複雜；不要因為方便就把所有指標都做成 shared_ptr。
  - std::make_shared<T>() 通常一次配置 control block 和物件，效率好且例外安全。
  - 循環引用是 shared_ptr 最大陷阱：A 持有 B，B 也持有 A，reference count 永遠不歸零；解法通常是其中一邊改用 weak_ptr。
  - shared_ptr 的引用計數操作本身是 thread-safe，但被指向的物件內容不是自動 thread-safe；多執行緒讀寫物件仍需要同步。
  - get() 取得裸指標只應借用，不可 delete；真正所有權仍由 shared_ptr 管理。
  - 不要從同一個裸指標建立兩個獨立 shared_ptr，會產生兩個 control block 並導致 double delete。
  - API 設計上，只有函式需要延長生命週期或共享所有權時才收 shared_ptr；只是讀取物件應收 T&、const T& 或 T*。
*/

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <functional>     // std::function (for the LC 938 lambda recursion)

// -----------------------------------------------------------------------------
// 範例 1：shared_ptr 引用計數示範
// -----------------------------------------------------------------------------
class Config {
public:
    explicit Config(const std::string& name) : name_(name) {
        std::cout << "  Config(" << name_ << ") 建構\n";
    }
    ~Config() { std::cout << "  ~Config(" << name_ << ") 解構\n"; }
    void show() const { std::cout << "  使用 Config: " << name_ << std::endl; }
private:
    std::string name_;
};

// -----------------------------------------------------------------------------
// 範例 2：循環引用 + weak_ptr 修補 - 樹節點
// -----------------------------------------------------------------------------
struct Node {
    int                                  value;
    std::vector<std::shared_ptr<Node>>   children;     // 父→子：強參考 (擁有)
    std::weak_ptr<Node>                  parent;       // 子→父：弱參考 (不擁有)

    explicit Node(int v) : value(v) {
        std::cout << "  Node(" << value << ") 建構\n";
    }
    ~Node() {
        std::cout << "  ~Node(" << value << ") 解構\n";
    }
};

// 把 child 加到 parent 的 children 中，並設好 child.parent
void addChild(std::shared_ptr<Node> parent, std::shared_ptr<Node> child) {
    parent->children.push_back(child);
    child->parent = parent;            // weak_ptr，不會增加計數，避免循環
}

int main() {
    std::cout << "===== 範例 1：shared_ptr 引用計數 =====" << std::endl;
    {
        std::shared_ptr<Config> a = std::make_shared<Config>("MyApp.cfg");
        std::cout << "建立後 a.use_count() = " << a.use_count() << "\n";   // 1
        {
            std::shared_ptr<Config> b = a;     // 複製 → 計數 +1
            std::shared_ptr<Config> c = a;     // 複製 → 計數 +1
            std::cout << "三個共享者 use_count = " << a.use_count() << "\n";   // 3
            b->show();
        }
        // 離開內層大括號 → b, c 解構 → 計數 -2
        std::cout << "回到外層 use_count = " << a.use_count() << "\n";   // 1
    }
    // 離開外層 → a 解構 → 計數歸 0 → ~Config 跑

    std::cout << "===== 範例 2：weak_ptr 避免循環 =====" << std::endl;
    {
        auto root  = std::make_shared<Node>(1);
        auto left  = std::make_shared<Node>(2);
        auto right = std::make_shared<Node>(3);
        addChild(root, left);
        addChild(root, right);

        std::cout << "root.use_count = " << root.use_count() << "\n";    // 1 (只有自己 main)
        std::cout << "left.use_count = " << left.use_count() << "\n";    // 2 (我們 + root.children)

        // 用 weak_ptr 拿父親
        if (auto p = left->parent.lock()) {     // .lock() 把 weak_ptr 升級成 shared_ptr
            std::cout << "  left 的爸爸是 " << p->value << "\n";
        } else {
            std::cout << "  父親已經消失\n";
        }
    }
    // 離開大括號 → root 計數歸 0 → root 解構 → 它 children 也跟著解構 → left/right 解構

    std::cout << "===== 結尾：weak_ptr 觀察『資源是否還活著』 =====" << std::endl;
    std::weak_ptr<Config> obs;
    {
        auto cfg = std::make_shared<Config>("Temp");
        obs = cfg;             // 弱參考觀察，不影響計數
        std::cout << "  obs.expired() = " << obs.expired() << "\n";        // 0
    }
    // 離開區塊 → cfg 解構 → ~Config 跑 → obs 觀察的對象消失
    std::cout << "  obs.expired() = " << obs.expired() << "\n";            // 1

    std::cout << "===== Leetcode 938 - Range Sum of BST (用 shared_ptr 建樹) =====" << std::endl;
    // BSTNode 用 shared_ptr 持有左右子節點，整棵樹的釋放完全交給 RAII。
    // 注意：BST 是樹結構不是圖，不會出現循環，所以全部用 shared_ptr 沒問題。
    struct BSTNode {
        int val;
        std::shared_ptr<BSTNode> left;
        std::shared_ptr<BSTNode> right;
        BSTNode(int v) : val(v) {}
    };
    auto N = [](int v, std::shared_ptr<BSTNode> l = nullptr,
                       std::shared_ptr<BSTNode> r = nullptr) {
        auto n = std::make_shared<BSTNode>(v);
        n->left = l; n->right = r;
        return n;
    };
    //          10
    //         /  \.
    //        5    15
    //       / \    \.
    //      3   7    18
    auto root = N(10, N(5, N(3), N(7)), N(15, nullptr, N(18)));

    // 遞迴解法：靠 BST 性質剪枝，不必遍歷整棵樹
    std::function<int(const std::shared_ptr<BSTNode>&, int, int)> rangeSum =
        [&](const std::shared_ptr<BSTNode>& node, int lo, int hi) -> int {
            if (!node) return 0;
            if (node->val < lo) return rangeSum(node->right, lo, hi);
            if (node->val > hi) return rangeSum(node->left,  lo, hi);
            return node->val + rangeSum(node->left,  lo, hi)
                             + rangeSum(node->right, lo, hi);
        };
    std::cout << "rangeSum(7, 15) = " << rangeSum(root, 7, 15)
              << "  (預期 7+10+15 = 32)" << std::endl;
    // root 在 main 結束時被解構 → 整棵樹遞迴自動釋放，零洩漏。

    std::cout << "===== Leetcode 104 - 二元樹最大深度 (用 shared_ptr 建樹) =====" << std::endl;
    // 題目簡述：給一棵二元樹，回傳最深一條路徑的節點數。
    // 用 shared_ptr 持有節點，整棵樹自動釋放。
    std::function<int(const std::shared_ptr<BSTNode>&)> maxDepth =
        [&](const std::shared_ptr<BSTNode>& node) -> int {
            if (!node) return 0;
            int l = maxDepth(node->left);
            int r = maxDepth(node->right);
            return 1 + (l > r ? l : r);
        };
    std::cout << "maxDepth = " << maxDepth(root) << "  (預期 3)" << std::endl;

    std::cout << "===== 日常實用：多人共享 Cache =====" << std::endl;
    // 工作上常見：同一份 Cache 被多個 Service 共享，最後一個離開才釋放。
    class Cache {
    public:
        std::string name;
        explicit Cache(const std::string& n) : name(n) {
            std::cout << "  Cache(" << name << ") 建構\n";
        }
        ~Cache() { std::cout << "  ~Cache(" << name << ") 解構\n"; }
    };

    {
        auto cache = std::make_shared<Cache>("imageCache");
        std::cout << "  use_count = " << cache.use_count() << " (1)\n";
        {
            auto serviceA = cache;     // 服務 A 共享
            auto serviceB = cache;     // 服務 B 共享
            std::cout << "  服務 A/B 加入後 use_count = " << cache.use_count() << " (3)\n";
        }
        std::cout << "  服務 A/B 離開後 use_count = " << cache.use_count() << " (1)\n";
        // cache 解構 → use_count 歸 0 → 釋放
    }
    return 0;
}

/* 預期輸出：
 * ===== 範例 1：shared_ptr 引用計數 =====
 *   Config(MyApp.cfg) 建構
 * 建立後 a.use_count() = 1
 * 三個共享者 use_count = 3
 *   使用 Config: MyApp.cfg
 * 回到外層 use_count = 1
 *   ~Config(MyApp.cfg) 解構
 * ===== 範例 2：weak_ptr 避免循環 =====
 *   Node(1) 建構
 *   Node(2) 建構
 *   Node(3) 建構
 * root.use_count = 1
 * left.use_count = 2
 *   left 的爸爸是 1
 *   ~Node(1) 解構
 *   ~Node(2) 解構
 *   ~Node(3) 解構
 * ===== 結尾：weak_ptr 觀察『資源是否還活著』 =====
 *   Config(Temp) 建構
 *   obs.expired() = 0
 *   ~Config(Temp) 解構
 *   obs.expired() = 1
 * ===== Leetcode 938 - Range Sum of BST (用 shared_ptr 建樹) =====
 * rangeSum(7, 15) = 32  (預期 7+10+15 = 32)
 * ===== Leetcode 104 - 二元樹最大深度 (用 shared_ptr 建樹) =====
 * maxDepth = 3  (預期 3)
 * ===== 日常實用：多人共享 Cache =====
 *   Cache(imageCache) 建構
 *   use_count = 1 (1)
 *   服務 A/B 加入後 use_count = 3 (3)
 *   服務 A/B 離開後 use_count = 1 (1)
 *   ~Cache(imageCache) 解構
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. shared_ptr 透過引用計數共享所有權，計數歸 0 才釋放。
 *   2. 用 std::make_shared 建立比 shared_ptr<T>(new T) 更有效率。
 *   3. 兩個物件互相用 shared_ptr 指對方 → 循環引用 → 永遠不釋放。
 *   4. 解法：其中一邊改用 weak_ptr (弱參考)，要用時 .lock() 拿 shared_ptr。
 *   5. 預設選 unique_ptr，真的需要共享才用 shared_ptr — 計數有額外成本。
 *
 * 【下一篇預告】
 *   22_MoveSemantics.cpp
 *   移動語意 (Move Semantics) — 為什麼 std::move(a) 不是「真的搬走」？
 *   它讓「拷貝大型物件」變得幾乎免費。
 *=============================================================================*/
