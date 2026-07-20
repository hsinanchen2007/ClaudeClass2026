/*
 * C++11 面試前總複習：15 個語言核心功能
 * ============================================================================
 * 這不是目錄索引，而是「只剩十分鐘時」可單檔複習的速讀章。讀完應能回答選擇準則、
 * 型別/生命週期陷阱，並看懂下方整合範例。
 *
 * 【一、型別推導與宣告】
 * ┌────────────────────┬──────────────────────────────────────────────────────┐
 * │ 功能               │ 面試必記                                             │
 * ├────────────────────┼──────────────────────────────────────────────────────┤
 * │ auto               │ 傳值推導去 top-level const/ref；保留參考要 auto&。   │
 * │ decltype(x)        │ 未括號 name 取宣告型別；decltype((x)) 依 value cat。 │
 * │ trailing return    │ 參數進 scope 後才能寫 decltype(param expression)。  │
 * │ using alias        │ 不建立新型別；alias template 是 typedef 做不到的。   │
 * └────────────────────┴──────────────────────────────────────────────────────┘
 * 快問快答：
 * Q: const int x; auto y=x 的型別？ A: int。
 * Q: decltype((x))？ A: const int&，因括號運算式是 lvalue。
 * Q: using UserId=int 能防 OrderId 傳錯嗎？ A: 不能，要 wrapper strong type。
 *
 * 【二、初始化與字面值】
 * - brace init：阻止 narrowing；但 initializer_list constructor 優先。
 *   vector<int>{3,9}=兩元素；vector<int>(3,9)=三個 9。
 * - initializer_list：唯讀 view；不要保存其 iterator 到呼叫結束後。
 * - user-defined literal：suffix 需 `_name`；適合單位，不應隱藏 I/O。
 * - raw string：解決 escape 可讀性，不會自動防 SQL injection 或驗 JSON。
 *
 * 【三、型別安全與 compile-time 契約】
 * - nullptr：std::nullptr_t，不是整數；解參考空指標是 UB，delete nullptr 是 no-op。
 * - enum class：scoped、無隱式 int conversion；跨 wire 要顯式 encode。
 * - constexpr：有機會 compile time，也可 runtime；const 只代表不可修改。
 * - static_assert：編譯期條件；runtime input 要另外驗證。不要把 ABI padding 當序列格式。
 *
 * 【四、類別與多型】
 * - =default 明確要求 compiler 生成；=delete 讓危險呼叫在 compile time 失敗。
 * - 優先 Rule of Zero；手動管理資源才考慮 Rule of Five。
 * - override 讓 compiler 驗證真正覆寫；final 阻止再覆寫/繼承。
 * - polymorphic base destructor 必須 virtual，否則透過 base pointer delete 是 UB。
 *
 * 【五、迭代】
 * range-for 的 auto 是複製、auto& 修改、const auto& 只讀免複製。迴圈內造成
 * vector reallocation/erase 可能讓 iterator/reference 失效。
 * 【複雜度】這些語言功能本身多為零成本；演算法仍依容器與操作決定，例如本檔
 * running sum 是 O(n) time/O(n) output space，多型呼叫則可能有一次間接 dispatch。
 *
 * 【選擇題速查】
 * - 冗長 iterator 型別但 initializer 明顯 -> auto。
 * - 泛型回傳精確 expression type -> decltype + trailing return。
 * - optional raw observer -> nullptr；擁有權 -> smart pointer。
 * - 一組封閉 domain states -> enum class。
 * - compile-time 常數演算法 -> constexpr；不合法型別 -> static_assert。
 * - runtime plugin interface -> virtual + override + virtual destructor。
 *
 * 【高頻陷阱】
 * 1. auto 複製導致修改沒回原容器；或意外複製大型物件。
 * 2. decltype((local)) 回傳 local reference，函式結束後 dangling。
 * 3. vector 的 initializer_list overload 與 size/value constructor 混淆。
 * 4. 保存 initializer_list 元素地址、temporary string 的 string_view/pointer。
 * 5. base destructor 非 virtual；override 漏 const；移動後仍假設舊值不變。
 * 6. enum class 強轉任意整數後直接 switch，未處理非法值。
 *
 * 【實務/面試練習】
 * 1. 說明 Rule of Zero 比手寫 copy/move 更可靠的原因。
 * 2. 替下方 Job 加 cancelled 狀態並讓 switch 保持 exhaustive。
 * 3. 將 running_total 泛化為 long long/double，確認 decltype 回傳型別。
 * 4. 找出 `const auto value : rows` 為何仍會複製，改成正確宣告。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace review {
using UserId = std::string;

enum class JobState { queued, running, succeeded, failed };

constexpr long long kibibytes(int count) noexcept {
    return static_cast<long long>(count) * 1024LL;
}
static_assert(kibibytes(4) == 4096, "容量換算必須在 compile time 正確");

class Reporter {
public:
    virtual ~Reporter() = default;
    virtual std::string report(JobState state) const = 0;
};

class TextReporter final : public Reporter {
public:
    std::string report(JobState state) const override {
        switch (state) {
            case JobState::queued: return "queued";
            case JobState::running: return "running";
            case JobState::succeeded: return "succeeded";
            case JobState::failed: return "failed";
        }
        return "invalid";
    }
};

template <class Left, class Right>
auto add(Left left, Right right) -> decltype(left + right) {
    return left + right;
}

// LeetCode 1480 整合題：range-for + auto/decltype + brace initialization。
template <class Container>
auto leetcode_running_sum(const Container& input)
    -> std::vector<typename Container::value_type> {
    using Value = typename Container::value_type;
    std::vector<Value> output;
    output.reserve(input.size());
    Value total{};
    for (const auto& value : input) {
        total += value;
        output.push_back(total);
    }
    return output;
}

struct Job {
    UserId owner;
    JobState state;
    std::vector<int> durations_ms;
};

// 實務整合：const auto& 避免複製 Job/string/vector；polymorphic reporter 可替換輸出格式。
std::string practical_summarize(const Job& job, const Reporter& reporter) {
    // accumulate 的累加型別由 init 決定；0 會讓合法的大型總和在 int 中溢位。
    const auto total = std::accumulate(job.durations_ms.begin(),
                                       job.durations_ms.end(), 0LL);
    return job.owner + ":" + reporter.report(job.state) + ":" + std::to_string(total);
}

void test_all() {
    static_assert(std::is_same<decltype(add(1, 0.5)), double>::value,
                  "整合範例的回傳型別必須是 double");
    const auto sums = leetcode_running_sum(std::vector<int>{1, 2, 3, 4});
    assert(sums == std::vector<int>({1, 3, 6, 10}));

    std::unique_ptr<Reporter> reporter(new TextReporter());
    const Job job{"alice", JobState::succeeded, {10, 20, 30}};
    assert(practical_summarize(job, *reporter) == "alice:succeeded:60");
    const Job large{"bob", JobState::running,
                    {std::numeric_limits<int>::max(), 1}};
    assert(practical_summarize(large, *reporter) == "bob:running:2147483648");

    const int* observer = nullptr;
    assert(observer == nullptr);
}
}  // namespace review

int main() {
    review::test_all();
    std::cout << "C++11 面試總複習：型別、初始化、契約、多型、整合案例皆通過\n";
}
