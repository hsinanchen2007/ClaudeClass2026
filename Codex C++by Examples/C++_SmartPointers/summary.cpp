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

/*
==============================================================================
【面試深挖：Smart Pointers 與 Ownership】

SP1｜三種 smart pointer 的 ownership 語意？
答：unique_ptr 單一 owner、可 move 不可 copy；shared_ptr 共同 lifetime；weak_ptr non-owning
觀察 shared control block。預設 unique，只有真實共同所有權才升級 shared。

SP2｜raw pointer/reference 是否都該消失？
答：不該。它們適合表 non-owning borrow；關鍵是 API 清楚誰擁有、borrow 不超過 owner lifetime。
把每個 observer 都改 shared_ptr 會隱藏 ownership、增加 refcount 並延長不必要 lifetime。

SP3｜`make_shared` 的 allocation 模型？
答：通常 object 與 control block 一次配置，locality/配置次數較好；直接 shared_ptr(new T)
通常兩次。代價是只要 weak_ptr 還在，合併配置的整塊 storage 可能較晚釋放。

SP4｜control block 裡通常有什麼？
答：strong/weak counts、deleter、allocator 與 managed object/pointer 資訊；shared_ptr object
本身通常有 stored pointer 與 control-block pointer。這是常見實作，不要把大小當標準固定。

SP5｜shared_ptr「thread-safe」精確指什麼？
答：不同 shared_ptr instances 共享同 control block 時可並行 copy/reset，refcount 安全；
不代表 pointed object thread-safe，也不代表同一 shared_ptr object 可無同步讀寫。後者可用
`atomic<shared_ptr<T>>` 或鎖。

SP6｜如何打破 shared_ptr cycle？
答：ownership graph 中至少一條反向/觀察 edge 用 weak_ptr。先畫出誰決定誰的 lifetime；
不是看到兩個物件互指就隨機把其中一個改 weak。

SP7｜`weak_ptr::lock` 為何比先 `expired()` 再建 shared 好？
答：expired 與後續取得間有 race；lock 原子地嘗試增加 strong count，成功回 shared_ptr，
失敗回空。多執行緒與 callback registry 都應用 lock 結果作唯一判斷。

SP8｜為何不能寫 `shared_ptr<T>(this)`？
答：若 this 已由另一 control block 管理，會建立第二 control block，最後 double delete。
用 public `enable_shared_from_this<T>` 且確保物件先由 shared_ptr 建立；否則 shared_from_this
會丟 bad_weak_ptr。

SP9｜aliasing constructor 做什麼？
答：shared_ptr<U>(owner, subobject_ptr) 的 stored pointer 指 subobject，但 ownership/control
block 跟 owner。它可安全延長 member lifetime；get() 與真正被 deleter 釋放的 pointer 可不同。

SP10｜custom deleter 對 unique_ptr/shared_ptr 大小的影響？
答：unique_ptr 的 deleter 是型別的一部分，stateful/function-pointer deleter 可能增加物件大小；
stateless deleter 常可 empty-base/no_unique_address 優化。shared_ptr deleter type-erased 放 control block。

SP11｜PImpl 為何常用 `unique_ptr<Impl>`，destructor 要放 .cpp？
答：header 可只 forward-declare Impl；但 default_delete 在真正析構處需要 complete type。
把 owning class destructor out-of-line 定義於看得到 Impl definition 的 .cpp。

SP12｜smart pointer 應如何傳參？
答：要轉移 unique ownership 就按值 unique_ptr；只 borrow 用 T&/T*；要共享 lifetime 才按值
shared_ptr；只觀察且允許消失用 weak_ptr。不要僅為呼叫 member 就傳 shared_ptr const&。

SP13｜`auto_ptr` 為何被移除？
答：它用看似 copy 的語法偷偷轉移 ownership，來源被清空，不滿足正常 CopyConstructible
直覺且不適合容器。move semantics 讓 unique_ptr 以明確 `std::move` 表達轉移；C++17 移除 auto_ptr。

SP14｜`unique_ptr<T[]>` 與 `unique_ptr<T>` 可混用嗎？
答：不可；array specialization 使用 delete[] 並提供 operator[]，單物件版本使用 delete。
更常見的動態陣列應優先 vector，因為它同時保存 size 並提供完整 container contract。
==============================================================================
*/

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

// 【章末自測】從 ownership graph 判定 unique/shared/weak/raw observer，並找出可能的 cycle。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_SmartPointers_summary' && '/tmp/codex_cpp_C_SmartPointers_summary'
//
// === 預期輸出（節錄）===
// SmartPointers summary: all assertions passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
