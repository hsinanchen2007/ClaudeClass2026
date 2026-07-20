// ============================================================================
// C++ OOP 總複習：value、invariant、ownership、polymorphism 與常見設計模式
// ============================================================================
//
// 【本章地圖：完整覆蓋同目錄 1～28】
//   1  class/object          型別是藍圖，object 有獨立 state/lifetime
//   2  member/method        data 與操作共同維持 invariant
//   3  encapsulation        private 隱藏表示；public 提供合法狀態轉換
//   4  constructor          完成即應建立合法物件；失敗就 throw/factory error
//   5  destructor           lifetime 結束釋放資源；不可讓例外逃出
//   6  this                 指向目前 object；不要回傳會超過 object lifetime 的 this/reference
//   7  member initializer   members 依宣告順序建構，不依 initializer list 書寫順序
//   8/9 copy ctor/assign    建立新物件 vs 改既有物件；self-assignment 與 exception safety
//   10 const member         const object 只能呼叫 const overload；logical constness
//   11 static member        屬於 class，不屬個別 object；inline static 可避免另一定義
//   12 friend               精準授權，不是 inheritance；過多 friend 暗示封裝邊界不佳
//   13 operator overload    保留直覺語意；比較/算術通常由 compound operator 推導
//   14/15 inheritance       public=is-a；protected/private inheritance 多半可改 composition
//   16 virtual function     runtime dispatch；override/final 防簽名錯誤
//   17 abstract class       至少一個 pure virtual，定義 runtime interface
//   18 virtual destructor   會經 base pointer delete 時必須 virtual
//   19 RAII                resource acquisition is initialization
//   20/21 unique/shared     單一 ownership 預設；真正共同 lifetime 才 shared
//   22 move semantics       轉移資源；moved-from 仍 valid 但值通常 unspecified
//   23 Rule 3/5/0           優先 Rule of Zero；手管資源才考慮五個 special members
//   24 class template       compile-time family；implementation 通常放 header
//   25 STL custom class     equality/order/hash 必須符合容器契約
//   26 Singleton            global lifetime/test coupling；能 dependency injection 就別用
//   27 Factory              把 construction 與使用端分離，常回 unique_ptr<Base>
//   28 LRU Cache            list + unordered_map，O(1) get/put；兩容器 invariant/例外安全
//
// 【設計順序：先問這五題】
//   1. invariant 是什麼？每個 public operation 後都成立嗎？
//   2. 誰擁有資源？value / unique_ptr / shared_ptr / borrow 哪個最誠實？
//   3. 需要 value semantics 還是 identity semantics？copy 是否有合理意義？
//   4. 變化在 compile time（template）還是 runtime（virtual/type erasure）？
//   5. composition 能否取代 inheritance？若只想重用實作，通常答案是能。
//
// 【special member 函式表】
//   default ctor        T()                         建立初始合法 state
//   destructor          ~T()                        清理；通常 noexcept
//   copy ctor           T(const T&)                 從另一 object 建立
//   copy assignment     T& operator=(const T&)      替換既有 state
//   move ctor           T(T&&) noexcept             從另一 object 轉移建立
//   move assignment     T& operator=(T&&) noexcept  釋放舊 state 後接收資源
//   宣告 destructor/copy/move 之一會影響其餘隱式生成規則；不要死背零碎規則，
//   優先讓 vector/string/unique_ptr 成為 members 並 `= default` 或完全不宣告。
//
// 【copy / move / lifetime】
//   - copy 後兩物件應可獨立使用；raw owning pointer 的 memberwise copy 會 double delete。
//   - memberwise copy 也可能「能編譯卻語意錯」：若 member 保存另一 member 的 iterator，
//     iterator 仍指向來源容器。此類型要禁止 copy，或 copy 資料後重建所有內部索引。
//   - move 後來源仍可析構與重新賦值；不要依賴其內容，除非型別明訂 postcondition。
//   - return by value 通常靠 copy elision/NRVO；不要為「優化」亂回 reference 或 std::move(local)。
//   - base constructor 先、members 按宣告順序、derived constructor body；析構完全反向。
//
// 【inheritance / virtual dispatch 選型】
//   public inheritance 表示每個 Derived 都可替代 Base（Liskov substitution）。
//   non-virtual call 靜態綁定；virtual call 依 dynamic type。constructor/destructor 中 virtual
//   dispatch 不會進入尚未建構/已析構的 derived 部分。
//   interface destructor：`virtual ~Base() = default;`。若 base 不允許 polymorphic delete，
//   destructor 可設 protected non-virtual，但需清楚限制。
//   object slicing：把 Derived by value 放入 Base 會只剩 Base；改 reference/pointer 或 variant。
//
// 【operator 契約】
//   `==` 應為 equivalence relation；strict weak ordering 必須滿足 irreflexive/transitive。
//   `a+b` 通常 copy a 後 `a += b`；assignment/operator<< 回 reference 便於 chaining。
//   不可改變 operator precedence/arity；不要讓 `+` 做 I/O 或其他驚奇副作用。
//
// 【複雜度速查】
//   virtual dispatch          O(1)，通常一次間接呼叫；設計清楚比微小成本重要
//   unique_ptr move/delete    O(1)（deleter 本身成本另計）
//   shared_ptr copy/reset     O(1) atomic refcount（object destructor 成本另計）
//   vector custom object     push_back amortized O(1)，reallocation 會搬移/複製全部
//   LRU get/put              unordered_map average O(1) + list splice O(1)，worst O(n)
//
// 【面試快問快答】
//   Q: struct 與 class 唯一語言差異？ A: 預設 member/inheritance access（public vs private）。
//   Q: override 為何重要？ A: 編譯器驗證真的覆寫，抓 const/參數簽名不一致。
//   Q: virtual destructor 何時需要？ A: object 可能透過 base pointer/reference ownership 刪除時。
//   Q: Rule of Zero 為何最好？ A: 把資源交給已正確實作 copy/move/destruct 的 member。
//   Q: Singleton 的問題？ A: 隱藏 dependency、全域 mutable state、初始化/測試/併發困難。
// ============================================================================

#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// Value type：constructor 建立 invariant，const observer，operator== 保留自然語意。
class Money {
public:
    explicit Money(long long cents) : cents_(cents) {}

    [[nodiscard]] long long cents() const noexcept { return cents_; }
    Money& operator+=(const Money& other)
    {
        if ((other.cents_ > 0 && cents_ > std::numeric_limits<long long>::max() - other.cents_) ||
            (other.cents_ < 0 && cents_ < std::numeric_limits<long long>::lowest() - other.cents_)) {
            throw std::overflow_error("Money addition overflow");
        }
        cents_ += other.cents_;
        return *this;
    }
    friend Money operator+(Money left, const Money& right)
    {
        left += right;
        return left;
    }
    friend bool operator==(const Money&, const Money&) = default;

private:
    long long cents_;
};

template <typename T>
concept HasEmptyObserver = requires(const T& value) {
    { value.empty() } -> std::convertible_to<bool>;
};

// 型別名稱宣告 invariant，就必須在 construction boundary 驗證；只把值包起來不算封裝。
template <HasEmptyObserver T>
class NonEmptyBox {
public:
    explicit NonEmptyBox(T value) : value_(std::move(value))
    {
        if (value_.empty()) throw std::invalid_argument("NonEmptyBox requires a non-empty value");
    }

    [[nodiscard]] const T& get() const noexcept { return value_; }
private:
    T value_;
};

void basic_value_demo()
{
    const Money lunch{1'250};
    const Money coffee{450};
    assert(lunch + coffee == Money{1'700});
    [[maybe_unused]] bool money_overflow_rejected = false;
    try {
        static_cast<void>(Money{std::numeric_limits<long long>::max()} + Money{1});
    } catch (const std::overflow_error&) {
        money_overflow_rejected = true;
    }
    assert(money_overflow_rejected);

    const NonEmptyBox<std::string> label{"GPU job"};
    assert(label.get() == "GPU job");

    [[maybe_unused]] bool rejected_empty = false;
    try {
        const NonEmptyBox<std::string> invalid{""};
    } catch (const std::invalid_argument& error) {
        rejected_empty = std::string{error.what()} == "NonEmptyBox requires a non-empty value";
    }
    assert(rejected_empty);
}

// ---------------------------------------------------------------------------
// LeetCode 146：LRU Cache
// list front=最近使用，back=最久未用；map 儲存 key -> list iterator。
// list::splice 不配置、不複製 node，iterator 仍有效，因此 get/put average O(1)。
// ---------------------------------------------------------------------------
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity)
    {
        if (capacity_ == 0U) throw std::invalid_argument("capacity must be positive");
    }

    // index_ 的 iterator 指向 entries_。compiler-generated copy 會複製 iterator 值，
    // 使副本 index_ 仍指向來源 list；本 cache 採 identity semantics，明確禁止 copy/move。
    LruCache(const LruCache&) = delete;
    LruCache& operator=(const LruCache&) = delete;
    LruCache(LruCache&&) = delete;
    LruCache& operator=(LruCache&&) = delete;

    [[nodiscard]] std::optional<int> get(int key)
    {
        const auto found = index_.find(key);
        if (found == index_.end()) return std::nullopt;
        touch(found->second);
        return found->second->second;
    }

    void put(int key, int value)
    {
        const auto found = index_.find(key);
        if (found != index_.end()) {
            found->second->second = value;
            touch(found->second);
            return;
        }

        entries_.emplace_front(key, value);
        try {
            // 不用 operator[]：emplace 直接建立完整 mapping。若 allocation/rehash 丟例外，
            // catch 會移除剛加入的 list node，恢復「每個 node 恰有一筆 index」的 invariant。
            const auto insertion = index_.emplace(key, entries_.begin());
            if (!insertion.second) {
                throw std::logic_error("duplicate key after cache miss");
            }
        } catch (...) {
            entries_.pop_front();
            throw;
        }
        if (entries_.size() > capacity_) {
            const int victim = entries_.back().first;
            index_.erase(victim);
            entries_.pop_back();
        }
    }

private:
    using Entry = std::pair<int, int>;
    using Iterator = std::list<Entry>::iterator;

    void touch(Iterator iterator)
    {
        entries_.splice(entries_.begin(), entries_, iterator);
    }

    std::size_t capacity_;
    std::list<Entry> entries_;
    std::unordered_map<int, Iterator> index_;
};

static_assert(!std::is_copy_constructible_v<LruCache>);
static_assert(!std::is_copy_assignable_v<LruCache>);

void leetcode_146_demo()
{
    LruCache cache(2U);
    cache.put(1, 1);
    cache.put(2, 2);
    // get 命中時會調整 LRU 次序；NDEBUG 會移除整個 assert expression，必須先呼叫。
    [[maybe_unused]] const auto key_1_first = cache.get(1); // key 1 變成 MRU
    assert(key_1_first.value() == 1);
    cache.put(3, 3);                  // 淘汰 key 2
    [[maybe_unused]] const auto key_2 = cache.get(2);
    assert(!key_2.has_value());
    cache.put(4, 4);                  // 淘汰 key 1
    [[maybe_unused]] const auto key_1_evicted = cache.get(1);
    [[maybe_unused]] const auto key_3 = cache.get(3);
    [[maybe_unused]] const auto key_4 = cache.get(4);
    assert(!key_1_evicted.has_value());
    assert(key_3.value() == 3);
    assert(key_4.value() == 4);
}

// ---------------------------------------------------------------------------
// 實務：runtime interface + Factory + composition + unique ownership。
// 呼叫端依賴 Reporter abstraction；factory 封裝具體 constructor，回 unique_ptr 表示單一 owner。
// ---------------------------------------------------------------------------
class Reporter {
public:
    virtual ~Reporter() = default;
    [[nodiscard]] virtual std::string render(const std::string& message) const = 0;
};

class TextReporter final : public Reporter {
public:
    [[nodiscard]] std::string render(const std::string& message) const override
    {
        return "TEXT:" + message;
    }
};

class JsonReporter final : public Reporter {
public:
    [[nodiscard]] std::string render(const std::string& message) const override
    {
        // 教學輸入不含需 escape 字元；正式 JSON 請用成熟 library。
        return "{\"message\":\"" + message + "\"}";
    }
};

std::unique_ptr<Reporter> make_reporter(const std::string& kind)
{
    if (kind == "text") return std::make_unique<TextReporter>();
    if (kind == "json") return std::make_unique<JsonReporter>();
    throw std::invalid_argument("unknown reporter: " + kind);
}

class ReportService {
public:
    explicit ReportService(std::unique_ptr<Reporter> reporter)
        : reporter_(std::move(reporter))
    {
        if (reporter_ == nullptr) throw std::invalid_argument("missing reporter");
    }

    [[nodiscard]] std::vector<std::string> run(const std::vector<std::string>& messages) const
    {
        std::vector<std::string> output;
        output.reserve(messages.size());
        for (const std::string& message : messages) {
            output.push_back(reporter_->render(message));
        }
        return output;
    }

private:
    std::unique_ptr<Reporter> reporter_;
};

void practical_factory_demo()
{
    ReportService service(make_reporter("json"));
    const auto output = service.run({"compile", "test"});
    assert((output == std::vector<std::string>{
        "{\"message\":\"compile\"}", "{\"message\":\"test\"}"}));

    // unique_ptr 讓 service 不可 copy、可 move；ownership 不會被不小心共享。
    ReportService moved = std::move(service);
    [[maybe_unused]] const auto published = moved.run({"publish"});
    assert(published.front() == "{\"message\":\"publish\"}");
}

int main()
{
    basic_value_demo();
    leetcode_146_demo();
    practical_factory_demo();
    std::cout << "OOP summary: all assertions passed\n";
}
