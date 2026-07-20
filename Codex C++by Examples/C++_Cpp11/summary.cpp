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

/*
==============================================================================
【面試深挖：C++11】

M11-1｜C++11 最重要的改變如何分組回答？
答：語言：move/value categories、auto/decltype、lambda、variadic templates、constexpr、
uniform init；library：smart pointers、thread/atomic、chrono/random、unordered containers。
不要只背名單，要說 ownership、generic programming、concurrency 三個設計轉折。

M11-2｜左值、xvalue、prvalue 如何快速說清楚？
答：先分 identity：glvalue 有 identity，prvalue 通常用於初始化；再分可移動：
xvalue 是有 identity 且資源可被重用的 expiring value。具名 T&& expression 本身仍是 lvalue。

M11-3｜`std::move` 真的移動了什麼？
答：它只是把 expression cast 成 xvalue，讓 overload resolution 有機會選 move operation；
真正資源轉移由目標型別 constructor/assignment 完成，const object 常仍只能 copy。

M11-4｜`auto` 會保留 const/reference 嗎？
答：plain auto 類似 template by-value deduction，通常去掉 top-level cv/reference；
`auto&`、`const auto&`、`auto&&` 規則不同。braced-init-list 是與一般 template deduction
著名不同點，必須單獨回答。

M11-5｜`decltype(x)` 與 `decltype((x))` 為何不同？
答：unparenthesized id-expression 有特殊規則，直接給 declared type；一般 expression 依 value
category 產 T&/T&&/T。具名變數 x 是 lvalue，因此 decltype((x)) 常為 T&。

M11-6｜`nullptr` 比 0/NULL 好在哪？
答：型別是 std::nullptr_t，可轉任意 pointer/member pointer，但不會被當 int overload；
解決 `f(int)` 與 `f(char*)` 的歧義/誤選。它仍不是「所有空 handle」的通用值。

M11-7｜brace initialization 的優點與陷阱？
答：防多數 narrowing、語法統一；但 initializer_list constructor 在 overload resolution
具有強偏好，`vector<int>{10,20}` 與 `vector<int>(10,20)` 意義不同。

M11-8｜move constructor 為何不等於 memcpy？
答：它要接管 resource 並讓 source 維持可析構 invariant；self-referential members、
allocator、mutex 或外部 registration 可能使 defaulted move 不符合語意。

M11-9｜range-for 有何 lifetime/拷貝陷阱？
答：loop variable 寫 `auto x` 會 copy，修改不回原容器；用 auto&/const auto&。
對 temporary range 的 lifetime 規則跨標準演進，子表達式 temporary 仍需小心，不要回 dangling view。

M11-10｜`=default` / `=delete` 與 private undeclared function 比較？
答：delete 在 overload resolution 時給明確診斷，能禁止特定 conversion/copy；default 要求
compiler 產生標準 special member，且定義位置會影響 triviality/ABI。

M11-11｜C++11 `constexpr` 的定位？
答：讓函式/物件可在條件允許時 constant-evaluate，不代表每次都編譯期執行。
C++11 constexpr function body 限制嚴，後續標準逐步放寬。
==============================================================================
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列動態和）
// 題目：對輸入序列產生逐項前綴和；例如 [1,2,3,4] 回傳 [1,3,6,10]。
// 為何使用本章主題：整合 range-for、auto、using 與 trailing return，依容器 value_type 建立結果；屬泛型教學改寫。
// 思路：1. 以 Value{} 建立累加器；2. const auto& 逐項讀取；3. 累加後依序 push 到預留容量的輸出。
// 複雜度：N 為輸入元素數；時間 O(N)，輸出空間 O(N)、額外工作空間 O(1)。
// 易錯點：累加型別由 value_type 決定，可能溢位；const auto& 對 scalar 雖非必要，但可泛化到較重元素。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】背景工作執行摘要
// 情境：將工作 owner、狀態與多段耗時組成單行摘要，並允許替換狀態輸出格式。
// 為何使用本章主題：UserId alias、enum class、override/final、const auto 與 0LL 累加共同表達型別與多型契約。
// 設計：1. Job 擁有 owner/state/durations；2. Reporter 將狀態格式化；3. accumulate 以 long long 加總後串成字串。
// 成本：N 為耗時筆數；時間 O(N)，字串結果空間 O(L)，其中 L 為輸出長度。
// 上線注意：Reporter 必須活過呼叫；仍需限制總和與輸出長度，且不得把敏感 owner 直接寫入未受控 log。
// -----------------------------------------------------------------------------
struct Job {
    UserId owner;
    JobState state;
    std::vector<int> durations_ms;
};

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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Cpp11_summary' && '/tmp/codex_cpp_C_Cpp11_summary'
//
// === 預期輸出（節錄）===
// C++11 面試總複習：型別、初始化、契約、多型、整合案例皆通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
