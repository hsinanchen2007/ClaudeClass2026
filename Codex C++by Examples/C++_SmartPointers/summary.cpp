// ============================================================================
// C++ Smart Pointers 總複習：ownership 是設計，pointer 只是表示
// ============================================================================
//
// 【本章地圖：對應 01～05】
//   unique_ptr / shared_ptr / weak_ptr / enable_shared_from_this / custom deleter
//
// 【先畫 ownership graph，再選型別】
//   語意                                  表示
//   object/value 直接屬於 parent           member value（通常最佳）
//   heap object 恰有一個 owner              unique_ptr<T>
//   多方共同決定何時銷毀                    shared_ptr<T>
//   觀察 shared object、不延長 lifetime     weak_ptr<T>
//   暫時借用且必須存在                      T& / const T&
//   可為空的暫時借用                        T* / const T*（文件明訂 non-owning）
//   member 要安全交出「既有」shared owner   enable_shared_from_this<T>
//   資源不是由 delete 釋放                  unique_ptr<T, Deleter> 或專用 RAII class
//
// 【unique_ptr】
//   make_unique<T>(args...)     單次配置、exception safe，預設建立方式
//   move                       轉移 ownership；來源變 null（unique_ptr 明確保證）
//   reset/release              reset 會刪舊物件；release 只放棄 ownership，呼叫者必須接手
//   get/operator*/operator->   borrow，不延長 lifetime；不可另建 owner
//   unique_ptr<T[]>            array 特化；通常 vector<T> 更能攜帶 size
//   copy 禁止、move O(1)；deleter 若帶 state，pointer object 大小可能增加。
//
// 【shared_ptr 與 control block】
//   control block 通常保存 strong count、weak count、deleter/allocator；object 可同區配置。
//   make_shared 一般一次 allocation，cache locality 好；但 weak_ptr 尚存時整塊 allocation
//   不會釋放，且 custom deleter/特殊 allocation 場景可改直接 constructor/allocate_shared。
//   copy shared_ptr 是 O(1) refcount 操作，常需 atomic；不代表被指物件 thread-safe。
//   `use_count()` 只適合診斷，不可用來做同步決策：讀完數字立刻可能改變。
//
// 【weak_ptr】
//   weak 不增加 strong count，打破 parent<->child/callback/cache cycle。
//   `expired()` 後再 `lock()` 有 TOCTOU；直接 `if (auto p = weak.lock())` 取得原子式結果。
//   lock 成功得到 shared_ptr，讓物件至少活到該 local shared_ptr 結束。
//
// 【enable_shared_from_this】
//   object 必須已由 shared_ptr control block 管理，才能在 member 呼叫 shared_from_this()。
//   對 stack object 或尚未被 shared owner 接管的 object 呼叫會丟 bad_weak_ptr。
//   constructor 內不可使用 shared_from_this；採 static create factory + 建構後初始化。
//   C++17 weak_from_this() 可不 throw 地取得 weak observer（仍可能 expired）。
//
// 【custom deleter】
//   FILE* -> fclose、socket -> close、C library handle -> destroy；deleter 必須 noexcept。
//   `unique_ptr<FILE, decltype(&fclose)>` 的 pointer type 仍是 FILE*；函式指標 deleter 會佔空間。
//   incomplete type 可放 unique_ptr member，但擁有類別的 destructor 通常需在 T 完整處定義。
//
// 【最危險的錯法】
//   1. `shared_ptr<T>(raw)` 兩次：建立兩個 control blocks，最後 double delete。
//   2. shared cycle：A/B 各 shared 彼此，strong count 永不歸零；一邊應是 weak。
//   3. `.get()` 傳給會保存/刪除 pointer 的 legacy API；先確認 ownership contract。
//   4. release 後忘記接手；或把同一 raw pointer 同時交給 unique_ptr 與手動 delete。
//   5. 為「避免 lifetime 思考」到處 shared_ptr：語意模糊、atomic 成本、cycle 風險。
//
// 【生命週期與複雜度】
//   unique move/reset       O(1) + destructor cost
//   shared copy/reset      O(1) refcount + 最後 owner 的 destructor cost
//   weak lock              O(1) control-block synchronization
//   custom deleter         成本由 release API 決定
//   smart pointer 本身不使 iterator/reference 安全；borrow 仍不得活過 owner。
//
// 【面試快問快答】
//   Q: unique_ptr 為何優先？ A: ownership 最清楚、無 refcount、可零成本 move。
//   Q: shared_ptr 是否 thread-safe？ A: 不同 shared_ptr instances 對同 control block 的計數安全；
//      pointee data 仍需自己的同步，同一 shared_ptr object 併發改寫亦需 atomic/shared mutex。
//   Q: make_shared 缺點？ A: object 與 control block 同 allocation，weak 尚存會延後記憶體釋放；
//      也不能直接提供任意 custom deleter。
//   Q: raw pointer 是否永遠錯？ A: 否；作為短期 nullable borrow 很清楚，錯的是未註明 ownership。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

struct LifetimeProbe {
    explicit LifetimeProbe(int& destructions) : destructions_(&destructions) {}
    ~LifetimeProbe() { ++*destructions_; }
    LifetimeProbe(const LifetimeProbe&) = delete;
    LifetimeProbe& operator=(const LifetimeProbe&) = delete;
private:
    int* destructions_; // 測試借用；counter lifetime 明確比 probe 長
};

void basic_ownership_demo()
{
    int destructions = 0;
    {
        auto owner = std::make_unique<LifetimeProbe>(destructions);
        LifetimeProbe* borrowed = owner.get();
        assert(borrowed != nullptr);

        auto moved = std::move(owner);
        assert(owner == nullptr && moved != nullptr);
    }
    assert(destructions == 1);

    auto shared = std::make_shared<const std::string>("immutable config");
    std::weak_ptr<const std::string> observer = shared;
    assert(observer.lock() != nullptr);
    shared.reset();
    assert(observer.lock() == nullptr);
}

// ---------------------------------------------------------------------------
// LeetCode 102：Binary Tree Level Order Traversal
// Tree 用 unique_ptr 明確擁有 children；BFS queue 只借用 const Node*，不轉移 ownership。
// 每個 node 入隊一次：O(n) time；queue 最寬層 O(w) space；析構遞迴自動釋放整棵樹。
// ---------------------------------------------------------------------------
class Tree {
public:
    struct Node {
        explicit Node(int node_value) : value(node_value) {}
        int value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };

    explicit Tree(std::unique_ptr<Node> root) : root_(std::move(root)) {}

    [[nodiscard]] std::vector<std::vector<int>> level_order() const
    {
        std::vector<std::vector<int>> result;
        if (root_ == nullptr) return result;

        std::queue<const Node*> pending;
        pending.push(root_.get());
        while (!pending.empty()) {
            const std::size_t width = pending.size();
            std::vector<int> level;
            level.reserve(width);
            for (std::size_t index = 0U; index < width; ++index) {
                const Node* node = pending.front();
                pending.pop();
                level.push_back(node->value);
                if (node->left != nullptr) pending.push(node->left.get());
                if (node->right != nullptr) pending.push(node->right.get());
            }
            result.push_back(std::move(level));
        }
        return result;
    }

private:
    std::unique_ptr<Node> root_;
};

void leetcode_102_demo()
{
    auto root = std::make_unique<Tree::Node>(3);
    root->left = std::make_unique<Tree::Node>(9);
    root->right = std::make_unique<Tree::Node>(20);
    root->right->left = std::make_unique<Tree::Node>(15);
    root->right->right = std::make_unique<Tree::Node>(7);
    const Tree tree(std::move(root));
    assert((tree.level_order() == std::vector<std::vector<int>>{
        {3}, {9, 20}, {15, 7}}));
}

// ---------------------------------------------------------------------------
// 實務：async callback 不可捕捉延命用的 shared_ptr 形成 cycle。
// Session factory 建立 shared owner；callback 只保存 weak_ptr，觸發時 lock。
// ---------------------------------------------------------------------------
class Session : public std::enable_shared_from_this<Session> {
public:
    static std::shared_ptr<Session> create(std::string id)
    {
        return std::shared_ptr<Session>(new Session(std::move(id)));
    }

    [[nodiscard]] std::weak_ptr<Session> observer() noexcept
    {
        return weak_from_this();
    }

    void record_message() noexcept { ++message_count_; }
    [[nodiscard]] int message_count() const noexcept { return message_count_; }
    [[nodiscard]] const std::string& id() const noexcept { return id_; }

private:
    explicit Session(std::string id) : id_(std::move(id)) {}
    std::string id_;
    int message_count_{0};
};

struct FakeHandle {
    int value;
};

struct HandleCloser {
    int* closed_count;
    void operator()(FakeHandle* handle) const noexcept
    {
        ++*closed_count;
        delete handle;
    }
};

void practical_callback_and_deleter_demo()
{
    auto session = Session::create("client-7");
    const std::weak_ptr<Session> callback_target = session->observer();
    if (auto locked = callback_target.lock()) {
        locked->record_message();
        assert(locked->id() == "client-7");
    }
    assert(session->message_count() == 1);
    session.reset();
    assert(callback_target.expired());

    int closes = 0;
    {
        std::unique_ptr<FakeHandle, HandleCloser> handle(
            new FakeHandle{42}, HandleCloser{&closes});
        assert(handle->value == 42);
    }
    assert(closes == 1);
}

int main()
{
    basic_ownership_demo();
    leetcode_102_demo();
    practical_callback_and_deleter_demo();
    std::cout << "SmartPointers summary: all assertions passed\n";
}
