// =====================================================================
// 主題 01: std::unique_ptr  (獨占擁有權的智慧指標)
// =====================================================================
// 參考來源:
//   https://en.cppreference.com/w/cpp/memory/unique_ptr
//   https://cplusplus.com/reference/memory/unique_ptr/
//
// ---------------------------------------------------------------------
// 一、什麼是 unique_ptr?
// ---------------------------------------------------------------------
// std::unique_ptr 是 C++11 引入的智慧指標, 定義於 <memory> 標頭檔。
// 它的核心語意是「獨占擁有權 (exclusive ownership)」:
//   - 同一個原生指標 (raw pointer) 在任何時間點, 只能被「一個」
//     unique_ptr 所擁有。
//   - 當 unique_ptr 離開 scope (作用域) 時, 它會自動 delete 所擁有的
//     物件, 無須手動釋放。
//
// 換言之, unique_ptr 就是一個「會自動清理的指標」, 並且禁止複製,
// 只能搬移 (move)。
//
// ---------------------------------------------------------------------
// 二、為什麼要用 unique_ptr?
// ---------------------------------------------------------------------
// 1. 避免記憶體洩漏 (memory leak): 不必再寫 new/delete 配對。
// 2. 避免 double free: 因為「獨占」, 不會發生兩個指標都嘗試刪除。
// 3. 例外安全 (exception safety): 即使中途丟出例外, 解構子仍會被
//    呼叫, 物件會被正確釋放 (RAII 原則)。
// 4. 零成本抽象: unique_ptr 在 release 模式下與裸指標效能幾乎相同,
//    它沒有引用計數的額外成本 (這點與 shared_ptr 不同)。
//
// ---------------------------------------------------------------------
// 三、基本用法
// ---------------------------------------------------------------------
//   std::unique_ptr<T> p1(new T(...));         // 較舊的寫法
//   auto p2 = std::make_unique<T>(...);        // C++14 推薦寫法 (例外安全)
//
//   p1.get()       -> 取得底層原生指標 (不轉移擁有權)
//   p1.release()   -> 釋放擁有權, 回傳原生指標 (使用者需自行管理)
//   p1.reset(q)    -> 釋放原本物件, 改持有 q
//   p1.swap(p2)    -> 兩個 unique_ptr 互換內容
//   *p1, p1->x     -> 與裸指標一樣的解參照語法
//   if (p1) {...}  -> bool 轉換: 是否持有物件
//
// ---------------------------------------------------------------------
// 四、為什麼禁止複製?
// ---------------------------------------------------------------------
// 因為「獨占」語意, 複製會造成兩個 unique_ptr 都認為自己擁有同一物件,
// 解構時會發生 double free。所以 copy constructor / copy assignment
// 都被刪除 (= delete), 只能用 std::move 來搬移擁有權。
//
//   std::unique_ptr<int> a = std::make_unique<int>(42);
//   std::unique_ptr<int> b = a;             // ❌ 編譯錯誤
//   std::unique_ptr<int> c = std::move(a);  // ✅ 搬移後 a 變成 nullptr
//
// ---------------------------------------------------------------------
// 五、何時該用 unique_ptr?
// ---------------------------------------------------------------------
// - 工廠函式 (factory function) 的回傳型別: 表示「呼叫者取得擁有權」。
// - PIMPL idiom (隱藏實作細節)。
// - 容器中持有「動態配置且獨占」的物件, 例如 std::vector<unique_ptr<T>>。
// - 任何「明確獨占」的資源管理: 檔案 handle、socket、自訂資源 (配合
//   custom deleter)。
//
// ---------------------------------------------------------------------
// 六、常見陷阱
// ---------------------------------------------------------------------
// 1. 不要把同一個 raw pointer 同時交給兩個 unique_ptr:
//      int* raw = new int(10);
//      std::unique_ptr<int> a(raw);
//      std::unique_ptr<int> b(raw);  // ❌ double free
// 2. 函式參數若只是「使用」物件, 應傳遞 raw pointer 或 reference,
//    不需要再包一層 unique_ptr&。
// 3. 陣列版本要用 std::unique_ptr<T[]>, 它會呼叫 delete[] 而非 delete。
//
// =====================================================================

/*
補充筆記：std::unique_ptr
  - unique_ptr 表示唯一所有權；它不能拷貝，只能移動。
  - make_unique 是建立 unique_ptr 的優先寫法，避免裸 new 暴露在程式中。
  - 函式回傳 unique_ptr 可以清楚表示所有權移交，參數使用 unique_ptr 則表示函式可能接管生命週期。
  - std::unique_ptr 屬於智慧指標；先判斷所有權是獨占、共享，還是只是觀察。
  - unique_ptr 表達獨占所有權，不可複製但可 move；shared_ptr 表達共享所有權，weak_ptr 打破循環引用。
  - make_unique/make_shared 通常比裸 new 更安全；它們把配置和建構包在一起，降低例外中途洩漏風險。
  - get() 只取得借用指標，不轉移所有權；不要 delete get() 回傳值。
  - shared_ptr 的 control block 很重要；不要用同一裸指標建立多個 shared_ptr。
  - custom deleter 用於 FILE*、C API handle、特殊釋放函式；deleter 型別也會影響 unique_ptr 型別大小。
*/
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// =====================================================================
// 範例 (1): 最基本的 unique_ptr 用法
// =====================================================================
// 觀念: 透過 make_unique 建立, 離開 scope 自動釋放。
void example_basic() {
    std::cout << "--- 範例 1: 基本用法 ---\n";
    // make_unique<T>(args...) 會把參數轉發給 T 的建構子
    auto p = std::make_unique<int>(42);
    std::cout << "*p = " << *p << "\n";
    std::cout << "p.get() = " << p.get() << "\n";
    // 離開函式時, p 自動釋放, 不需 delete
}

// =====================================================================
// 範例 (2): 搬移擁有權 (move semantics)
// =====================================================================
// 觀念: unique_ptr 不能複製, 只能 std::move。
//       搬移後, 原本的指標變為 nullptr。
void example_move() {
    std::cout << "--- 範例 2: 搬移擁有權 ---\n";
    auto a = std::make_unique<std::string>("hello");
    auto b = std::move(a);                   // 把擁有權交給 b
    std::cout << "a is " << (a ? *a : "(null)") << "\n";  // a 已變空
    std::cout << "b = " << *b << "\n";
}

// =====================================================================
// 範例 (3): 工廠函式 - 日常工作中最常見的用法
// =====================================================================
// 觀念: 函式回傳 unique_ptr 表示「呼叫者拿到擁有權」。
//       這是現代 C++ 取代「new + 回傳 raw pointer」的標準作法。
struct Config {
    std::string host;
    int port;
    Config(std::string h, int p) : host(std::move(h)), port(p) {}
};

std::unique_ptr<Config> create_default_config() {
    // 不必擔心 caller 忘了 delete
    return std::make_unique<Config>("localhost", 8080);
}

void example_factory() {
    std::cout << "--- 範例 3: 工廠函式 ---\n";
    auto cfg = create_default_config();
    std::cout << "host=" << cfg->host << ", port=" << cfg->port << "\n";
}

// =====================================================================
// 範例 (4): 容器中持有獨占物件
// =====================================================================
// 觀念: vector<unique_ptr<T>> 是常見的多型容器寫法。
//       每個元素獨占其所指物件, 容器釋放時自動清理。
struct Shape {
    virtual ~Shape() = default;
    virtual void draw() const = 0;
};
struct Circle : Shape { void draw() const override { std::cout << "Circle\n"; } };
struct Square : Shape { void draw() const override { std::cout << "Square\n"; } };

void example_polymorphic_container() {
    std::cout << "--- 範例 4: 多型容器 ---\n";
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>());
    shapes.push_back(std::make_unique<Square>());
    for (const auto& s : shapes) s->draw();
    // shapes 解構時, 每個 unique_ptr 自動 delete 各自的 Shape
}

// =====================================================================
// Leetcode 21. Merge Two Sorted Lists  (Easy)
// =====================================================================
// 題目:
//   給兩個已排序的單向鏈結串列 list1 與 list2,
//   合併它們成一個新的已排序鏈結串列並回傳串列頭。
//
// 為什麼這題適合 unique_ptr?
//   - 鏈結串列節點是動態配置的, 傳統解法用 new ListNode(...) 之後
//     極易忘記 delete, 造成記憶體洩漏。
//   - 每個節點只屬於一個串列 (獨占擁有權), 完全符合 unique_ptr 語意。
//   - 用 unique_ptr 後, 整個函式就具備自動清理能力, 即使中間 throw
//     例外也不會洩漏。
//
// 解題思路:
//   1. 建立一個 dummy (假) 節點當作合併後串列的「前哨」。
//   2. 維持一個 tail 指標 (raw pointer 即可, 因為不擁有, 只是觀察)
//      指向目前已合併串列的最後一個節點。
//   3. 比較 list1 與 list2 的首節點, 較小者接到 tail->next, 並前進。
//   4. 任一串列走完後, 把另一串列剩餘部分接上。
//   5. 回傳 dummy->next 的擁有權。
//
// 註: Leetcode 原本的 ListNode 是裸指標版本, 此處改用 unique_ptr 教學。
// ---------------------------------------------------------------------
struct ListNode {
    int val;
    std::unique_ptr<ListNode> next;   // 每個節點獨占下一個節點
    explicit ListNode(int v) : val(v), next(nullptr) {}
};

// 把 std::initializer_list 轉成串列, 便於建構測資
std::unique_ptr<ListNode> build_list(std::initializer_list<int> vals) {
    std::unique_ptr<ListNode> head;
    ListNode* tail = nullptr;
    for (int v : vals) {
        auto node = std::make_unique<ListNode>(v);
        if (!head) {
            head = std::move(node);
            tail = head.get();             // 觀察用, 不擁有
        } else {
            tail->next = std::move(node);
            tail = tail->next.get();
        }
    }
    return head;
}

// 合併兩個已排序串列 (Leetcode 21 解答)
// 重點: l1, l2 以「值傳遞」的 unique_ptr 接收, 表示函式取得擁有權。
//       回傳的 unique_ptr 把擁有權交還給呼叫者。
std::unique_ptr<ListNode> merge_two_lists(std::unique_ptr<ListNode> l1,
                                          std::unique_ptr<ListNode> l2) {
    auto dummy = std::make_unique<ListNode>(0);
    ListNode* tail = dummy.get();

    while (l1 && l2) {
        // 選定要接的那一側 (用 ref 指向參與方)
        std::unique_ptr<ListNode>& pick = (l1->val <= l2->val) ? l1 : l2;

        // 步驟:
        //   1. 把 pick 的下一個暫存起來 (next_node)
        //   2. pick->next 設為空, 切斷與後續節點的連結
        //   3. 把 pick 整顆搬去 tail->next
        //   4. tail 前進
        //   5. pick 重新指向 next_node
        std::unique_ptr<ListNode> next_node = std::move(pick->next);
        tail->next = std::move(pick);
        tail = tail->next.get();
        pick = std::move(next_node);
    }

    // 把剩下的串列整段接上 (擁有權直接搬移, O(1))
    tail->next = l1 ? std::move(l1) : std::move(l2);

    // 回傳 dummy->next 的擁有權; dummy 自身會被釋放
    return std::move(dummy->next);
}

// 印出整條串列, 用於驗證結果
void print_list(const ListNode* head) {
    std::cout << "[";
    for (const ListNode* p = head; p; p = p->next.get()) {
        std::cout << p->val;
        if (p->next) std::cout << " -> ";
    }
    std::cout << "]\n";
}

void example_leetcode_21() {
    std::cout << "--- Leetcode 21: Merge Two Sorted Lists ---\n";
    auto l1 = build_list({1, 2, 4});
    auto l2 = build_list({1, 3, 4});
    auto merged = merge_two_lists(std::move(l1), std::move(l2));
    print_list(merged.get());
    // merged 離開作用域時, 整條串列遞迴自動釋放 (因為 next 也是 unique_ptr)
}

// =====================================================================
// 實用範例: PIMPL idiom (pointer to implementation)
// =====================================================================
// 觀念: 把實作藏起來, header 只暴露 unique_ptr<Impl>。
//       好處: 修改 Impl 內部不會強迫 header 使用者重編譯;
//             unique_ptr 自動釋放, 不必手寫解構子內 delete impl_。
//
// 注意: 因為 unique_ptr<Impl> 在 header 看不到 Impl 完整定義,
//       解構子要在「.cpp 看得到 Impl 完整定義的地方」實作 (本檔示範)。
// =====================================================================
class Database {
public:
    Database();
    ~Database();                                // 注意: 必須在 cpp 內定義
    void put(const std::string& key, const std::string& value);
    std::string get(const std::string& key) const;
private:
    struct Impl;                                // 前向宣告
    std::unique_ptr<Impl> impl_;
};

// Impl 完整定義 (放在 .cpp 等同的「實作位置」)
struct Database::Impl {
    std::unordered_map<std::string, std::string> store;
};

Database::Database() : impl_(std::make_unique<Impl>()) {}
Database::~Database() = default;               // 在這裡 unique_ptr 看得到 Impl

void Database::put(const std::string& k, const std::string& v) {
    impl_->store[k] = v;
}
std::string Database::get(const std::string& k) const {
    auto it = impl_->store.find(k);
    return (it == impl_->store.end()) ? "" : it->second;
}

void example_pimpl() {
    std::cout << "--- 實用範例: PIMPL ---\n";
    Database db;
    db.put("user", "alice");
    db.put("role", "admin");
    std::cout << "user=" << db.get("user") << ", role=" << db.get("role") << "\n";
    // 離開 scope 時 unique_ptr 自動解構 Impl, store 跟著釋放
}

// =====================================================================
// main
// =====================================================================
int main() {
    example_basic();
    example_move();
    example_factory();
    example_polymorphic_container();
    example_leetcode_21();
    example_pimpl();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unique_ptr 跟 C++98 的 std::auto_ptr 差在哪?
    //    A：auto_ptr 的「複製」其實是搬移 (修改 RHS 變 null), 害容器
    //       與泛型程式碼遭遇詭異 bug, C++11 已 deprecated, C++17 移除。
    //       unique_ptr 直接禁止 copy, 只能用 std::move 顯式搬移, 還支援
    //       custom deleter 與 T[] 特化, 是徹底取代品。
    //
    //  Q2：sizeof(unique_ptr<T>) 一定等於 sizeof(T*) 嗎?
    //    A：預設 deleter (std::default_delete) 是 stateless, 透過 EBO
    //       不佔空間, 所以等於一個指標。一旦改成「有狀態的 deleter」
    //       (例如綁定 fclose 的函式指標), sizeof 就會變大, 因為要存
    //       deleter 物件。stateless functor 才能保持零成本。
    //
    //  Q3：可以把 unique_ptr<T> 傳給接受 unique_ptr<T>& 的函式嗎?
    //    A：可以但通常是壞 API 設計。「by value」表示轉移擁有權, 「by
    //       const T&」或「raw pointer」表示借用, 「by unique_ptr<T>&」
    //       則代表「我可能 reset 你」, 語意混亂。Herb Sutter 的 GotW #91
    //       建議:能用 raw pointer / reference 借用就不要包 unique_ptr&。
    //
    return 0;
}
