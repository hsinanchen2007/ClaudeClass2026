// =====================================================================
// 主題 03: std::weak_ptr  (弱引用智慧指標)
// =====================================================================
// 參考來源:
//   https://en.cppreference.com/w/cpp/memory/weak_ptr
//   https://cplusplus.com/reference/memory/weak_ptr/
//
// ---------------------------------------------------------------------
// 一、什麼是 weak_ptr?
// ---------------------------------------------------------------------
// std::weak_ptr 是 C++11 引入的智慧指標, 定義於 <memory>。
// 它的核心語意是「弱引用 (non-owning reference)」:
//   - weak_ptr 「不」擁有物件, 「不」增加 shared 引用計數。
//   - 它總是搭配 shared_ptr 一起使用, 用來「觀察 (observe)」
//     由 shared_ptr 管理的物件。
//   - 想要使用該物件時, 必須先呼叫 .lock() 取得一個 shared_ptr,
//     若物件還活著就能用, 否則回傳空。
//
// ---------------------------------------------------------------------
// 二、為什麼需要 weak_ptr?
// ---------------------------------------------------------------------
// 主要解決兩大問題:
//
// (A) 循環引用 (cycle): shared_ptr 互指會導致引用計數永遠不歸零,
//     造成記憶體洩漏。把其中一邊改成 weak_ptr 即可打破循環。
//
//     例子: 雙向連結串列、樹的「parent 指標」、觀察者模式。
//        Parent  --shared_ptr-->  Child
//        Child   --weak_ptr   -->  Parent   (打破循環)
//
// (B) 觀察可能已失效的物件 (observer):
//     在快取、回呼註冊、事件系統中, 我們想記住某物件,
//     但「不延長它的生命週期」, 也想偵測它是否已被釋放。
//
// ---------------------------------------------------------------------
// 三、與 control block 的關聯
// ---------------------------------------------------------------------
// 回顧 shared_ptr 的 control block, 它有兩個計數:
//   - shared count: 影響「物件本身」是否釋放
//   - weak count  : 影響「control block」是否釋放
//
// 當 shared_count -> 0, 物件被 delete, 但只要還有 weak_ptr 存在
// (weak_count > 0), control block 仍會保留, 用來告訴 weak_ptr
// 「物件已死」。
//
// 因此 weak_ptr.expired() 與 .lock() 才能安全運作:
//   - expired(): 檢查 shared_count 是否為 0
//   - lock()   : 嘗試把 weak_ptr 升級成 shared_ptr (atomic, 安全)
//
// ---------------------------------------------------------------------
// 四、基本用法
// ---------------------------------------------------------------------
//   std::shared_ptr<T> s = std::make_shared<T>(...);
//   std::weak_ptr<T>   w = s;             // 不增加 shared count
//
//   w.expired()      -> bool, 物件是否已死
//   w.use_count()    -> 目前 shared 計數
//   w.lock()         -> 回傳 shared_ptr<T>; 若已死則為空
//   w.reset()        -> 放棄觀察
//
// 注意: weak_ptr 「不能」直接解參照, 一定要先 .lock()。
//
// ---------------------------------------------------------------------
// 五、何時該用 weak_ptr?
// ---------------------------------------------------------------------
// 1. 樹/圖中的 back-reference (parent / 反向邊)
// 2. 觀察者模式: 訂閱者不擁有發布者
// 3. 快取需要自動失效
// 4. 不確定物件是否還活著, 但想「試試看能不能用」
//
// ---------------------------------------------------------------------
// 六、常見陷阱
// ---------------------------------------------------------------------
// 1. 忘了 .lock() 就直接使用 -> 編譯錯誤 (weak_ptr 沒有 operator*)。
// 2. 在 .lock() 後立刻 . 操作而不檢查是否為空 -> nullptr 解參照崩潰。
// 3. 在多執行緒中, .expired() 的結果在下一行可能就過期; 想「檢查
//    + 使用」必須用 .lock() (它是 atomic)。
//
// =====================================================================

/*
補充筆記：std::weak_ptr
  - weak_ptr 不擁有物件，只觀察 shared_ptr 管理的控制區塊。
  - 使用前要 lock 成 shared_ptr；lock 失敗代表物件已被釋放。
  - 它最常用來打破 shared_ptr 循環引用，或在 callback 中避免延長物件生命。
  - std::weak_ptr 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
*/
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// =====================================================================
// 範例 (1): 基本用法 - 觀察物件是否還活著
// =====================================================================
void example_basic() {
    std::cout << "--- 範例 1: 基本用法 ---\n";
    std::weak_ptr<int> w;
    {
        auto s = std::make_shared<int>(99);
        w = s;                                          // 觀察 s
        std::cout << "expired? " << w.expired() << "\n"; // 0
        if (auto sp = w.lock()) {                       // 升級回 shared_ptr
            std::cout << "value via lock = " << *sp << "\n";
        }
    } // s 離開 scope, 物件被釋放, 但 w 還在 (control block 也還在)
    std::cout << "expired? " << w.expired() << "\n";    // 1
    std::cout << "lock returns null? " << (w.lock() == nullptr) << "\n"; // 1
}

// =====================================================================
// 範例 (2): 打破循環 - 樹的 parent 指標
// =====================================================================
// 觀念: 父節點以 shared_ptr 持有子節點 (擁有), 子節點以 weak_ptr
//       回指父節點 (觀察), 兩者就不會形成循環。
struct TreeNode {
    int val;
    std::vector<std::shared_ptr<TreeNode>> children;   // 父擁有子
    std::weak_ptr<TreeNode> parent;                    // 子弱觀察父
    explicit TreeNode(int v) : val(v) {}
    ~TreeNode() { std::cout << "  [destroy node " << val << "]\n"; }
};

void example_tree_parent() {
    std::cout << "--- 範例 2: 樹的 parent (打破循環) ---\n";
    auto root  = std::make_shared<TreeNode>(1);
    auto child = std::make_shared<TreeNode>(2);
    root->children.push_back(child);
    child->parent = root;                               // weak, 不增加計數

    std::cout << "root  use_count = " << root.use_count()  << "\n"; // 1
    std::cout << "child use_count = " << child.use_count() << "\n"; // 2 (root->children 與 child)

    if (auto p = child->parent.lock()) {
        std::cout << "child 的父節點 val = " << p->val << "\n";
    }
    // 函式結束時, root 與 child 都能正確被釋放, 沒有 leak
}

// =====================================================================
// 範例 (3): 觀察者模式 - 不延長被觀察者壽命
// =====================================================================
// 觀念: Observer 訂閱了 Subject, 但 Observer 不該因為被訂閱而存活。
//       Subject 用 weak_ptr 記住 Observer; 通知時 lock() 取得有效者。
struct Observer {
    std::string name;
    explicit Observer(std::string n) : name(std::move(n)) {}
    void on_event(int e) const { std::cout << "  " << name << " got " << e << "\n"; }
};

class Subject {
    std::vector<std::weak_ptr<Observer>> observers_;
public:
    void subscribe(const std::shared_ptr<Observer>& o) { observers_.push_back(o); }
    void notify(int event) {
        // 一面通知, 一面清掉已過期的觀察者
        auto it = observers_.begin();
        while (it != observers_.end()) {
            if (auto sp = it->lock()) {
                sp->on_event(event);
                ++it;
            } else {
                it = observers_.erase(it);              // 已死, 移除
            }
        }
    }
};

void example_observer() {
    std::cout << "--- 範例 3: 觀察者模式 ---\n";
    Subject subject;
    auto a = std::make_shared<Observer>("A");
    {
        auto b = std::make_shared<Observer>("B");
        subject.subscribe(a);
        subject.subscribe(b);
        subject.notify(1);                              // A, B 都收到
    } // b 離開 scope, 自動釋放 (因為 Subject 只是 weak_ptr)
    subject.notify(2);                                  // 只剩 A
}

// =====================================================================
// 範例 (4): 自動失效快取 - 日常工作可能用到
// =====================================================================
// 觀念: 快取「弱持有」資源, 只要還有 caller 在用就重複利用,
//       沒人用了就自動消失, 不會佔記憶體。
class WeakCache {
    std::unordered_map<std::string, std::weak_ptr<std::string>> map_;
public:
    std::shared_ptr<std::string> get_or_load(const std::string& key) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            if (auto sp = it->second.lock()) return sp; // 還活著, 直接用
        }
        auto data = std::make_shared<std::string>("loaded:" + key);
        map_[key] = data;                               // 弱持有
        return data;
    }
};

void example_weak_cache() {
    std::cout << "--- 範例 4: 自動失效快取 ---\n";
    WeakCache cache;
    {
        auto a = cache.get_or_load("X");
        auto b = cache.get_or_load("X");
        std::cout << "a == b ? " << (a.get() == b.get()) << "\n";   // 1
        std::cout << "use_count = " << a.use_count() << "\n";       // 2
    } // a, b 都離開, "X" 被釋放; 快取裡的 weak_ptr 自動 expired

    auto c = cache.get_or_load("X");                    // 重新載入
    std::cout << "c = " << *c << "\n";
}

// =====================================================================
// Leetcode 133. Clone Graph  (Medium, 但循環引用是 weak_ptr 經典)
// =====================================================================
// 題目:
//   給一個無向圖中的某個節點 (含 val 與 neighbors 列表), 請深拷貝
//   整張圖並回傳新的對應節點。
//
// 為什麼這題適合 weak_ptr?
//   - 無向圖 = neighbors 互相指 = 「循環引用」!
//     如果 neighbors 全用 shared_ptr, 任一節點都不會被釋放 (cycle leak)。
//   - 標準解法: neighbors 用 weak_ptr (弱觀察); 由「圖容器」用
//     shared_ptr 統一擁有所有節點。
//   - 與 unique_ptr 在 Leetcode 21 的角色相反: 那邊獨占, 這邊共享。
//
// 解題思路 (DFS + map):
//   - map: 原節點裸指標 -> 新節點 shared_ptr
//   - 每次進入 dfs(orig):
//       1. 若 orig 已在 map 中, 回傳對應的新節點 (避免無限遞迴)
//       2. 否則建立新節點, 放入 map
//       3. 對每個 neighbor 遞迴, 把回傳結果以 weak_ptr 加進 neighbors
//
// 時間複雜度 O(V + E), 空間複雜度 O(V)。
// ---------------------------------------------------------------------
struct GraphNode {
    int val;
    std::vector<std::weak_ptr<GraphNode>> neighbors;    // 弱引用避免循環
    explicit GraphNode(int v) : val(v) {}
};

// 為了示範, 把所有節點集中由一個 vector 擁有, 確保壽命
class Graph {
public:
    std::vector<std::shared_ptr<GraphNode>> nodes;      // 統一擁有
    std::shared_ptr<GraphNode> add(int v) {
        nodes.push_back(std::make_shared<GraphNode>(v));
        return nodes.back();
    }
};

std::shared_ptr<GraphNode> clone_dfs(
    const std::shared_ptr<GraphNode>& orig,
    Graph& out,
    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>>& mp)
{
    if (!orig) return nullptr;
    auto it = mp.find(orig.get());
    if (it != mp.end()) return it->second;              // 已複製過

    auto copy = out.add(orig->val);
    mp[orig.get()] = copy;

    for (const auto& w : orig->neighbors) {
        if (auto n = w.lock()) {
            // 遞迴複製鄰居, 並把結果以 weak_ptr 接上
            auto cloned = clone_dfs(n, out, mp);
            copy->neighbors.push_back(cloned);          // weak_ptr <- shared_ptr 自動轉
        }
    }
    return copy;
}

std::shared_ptr<GraphNode> clone_graph(const std::shared_ptr<GraphNode>& node,
                                       Graph& out_graph) {
    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>> mp;
    return clone_dfs(node, out_graph, mp);
}

// 簡單 BFS 印出 (val: neighbors)
void print_graph(const std::shared_ptr<GraphNode>& start) {
    if (!start) return;
    std::unordered_map<const GraphNode*, bool> seen;
    std::vector<std::shared_ptr<GraphNode>> queue{start};
    seen[start.get()] = true;
    while (!queue.empty()) {
        auto cur = queue.front();
        queue.erase(queue.begin());
        std::cout << "  node " << cur->val << " -> [";
        for (size_t i = 0; i < cur->neighbors.size(); ++i) {
            if (auto n = cur->neighbors[i].lock()) {
                std::cout << n->val;
                if (i + 1 < cur->neighbors.size()) std::cout << ", ";
                if (!seen[n.get()]) { seen[n.get()] = true; queue.push_back(n); }
            }
        }
        std::cout << "]\n";
    }
}

// =====================================================================
// 實用範例: 雙向鏈結串列 (Doubly Linked List)
// =====================================================================
// 觀念: 雙向 next/prev 若都用 shared_ptr 就會循環引用、永遠不釋放。
//       傳統解法: next 走擁有 (shared_ptr), prev 走觀察 (weak_ptr)。
// =====================================================================
struct DListNode {
    int val;
    std::shared_ptr<DListNode> next;
    std::weak_ptr<DListNode>   prev;             // 弱引用打破循環
    explicit DListNode(int v) : val(v) {}
    ~DListNode() { std::cout << "  [destroy DListNode " << val << "]\n"; }
};

void example_doubly_linked_list() {
    std::cout << "--- 實用範例: 雙向鏈結串列 (weak_ptr 作 prev) ---\n";
    auto a = std::make_shared<DListNode>(1);
    auto b = std::make_shared<DListNode>(2);
    auto c = std::make_shared<DListNode>(3);
    a->next = b; b->prev = a;
    b->next = c; c->prev = b;

    // 從 c 回頭走訪 (需要 lock weak_ptr)
    std::cout << "  反向走訪: " << c->val;
    auto p = c->prev.lock();
    while (p) { std::cout << " <- " << p->val; p = p->prev.lock(); }
    std::cout << "\n";
    // a, b, c 離開 scope 時, 因 prev 是 weak_ptr, 三個節點都能正確釋放
}

void example_leetcode_133() {
    std::cout << "--- Leetcode 133: Clone Graph ---\n";
    // 建立原圖: 1 - 2, 1 - 3, 2 - 3 (三角形)
    Graph g;
    auto n1 = g.add(1);
    auto n2 = g.add(2);
    auto n3 = g.add(3);
    n1->neighbors = { n2, n3 };
    n2->neighbors = { n1, n3 };
    n3->neighbors = { n1, n2 };

    std::cout << "原圖:\n";
    print_graph(n1);

    Graph cloned_graph;
    auto cloned = clone_graph(n1, cloned_graph);
    std::cout << "深拷貝後:\n";
    print_graph(cloned);

    std::cout << "n1.get() == cloned.get() ? "
              << (n1.get() == cloned.get()) << " (應為 0)\n";
}

// =====================================================================
// main
// =====================================================================
int main() {
    example_basic();
    example_tree_parent();
    example_observer();
    example_weak_cache();
    example_doubly_linked_list();
    example_leetcode_133();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼一定要 lock() 才能用, 不能直接 expired() 後解參照?
    //    A：在多執行緒下, expired() 回傳 false 的下一奈秒, 物件可能就
    //       被別人釋放了 (TOCTOU race)。lock() 是 atomic 的「檢查 +
    //       提升」單一動作, 拿到非空 shared_ptr 就保證物件在這個 scope
    //       內不會死, 這是唯一安全的存取方式。
    //
    //  Q2：weak_ptr 會延長物件壽命嗎? control block 呢?
    //    A：weak_ptr 「不」延長物件壽命 (shared count 才管物件生死),
    //       但「會」延長 control block 壽命 (weak count 管 control
    //       block 生死)。所以 make_shared 的「物件 + control block 同一
    //       塊」, 在 shared count 歸零後, 物件記憶體得等 weak_ptr 全死
    //       才能歸還, 這是大物件搭配大量 weak_ptr 時的隱性成本。
    //
    //  Q3：weak_ptr 跟 raw pointer 看起來都「不擁有」, 何時用哪個?
    //    A：raw pointer 沒辦法偵測物件是否還活著, 適合「呼叫者保證壽命
    //       一定夠長」的函式參數借用。weak_ptr 適合「跨 scope 觀察、
    //       存進容器、等等可能用到」的場景, 它能告訴你「物件已死」並
    //       安全失敗, 代價是要分配 control block 與一次 atomic 操作。
    //
    return 0;
}
