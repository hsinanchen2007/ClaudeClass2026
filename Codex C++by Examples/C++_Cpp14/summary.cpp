/*
 * C++14 面試前總複習：小版本、但補齊泛型與編譯期實用性
 * ============================================================================
 * 本章可獨立作為面試前速讀，不需先開其他檔。C++14 的主軸不是大改語言，而是讓
 * C++11 的 auto、constexpr、lambda 與 ownership 寫法真正適合日常工程。
 * 【LeetCode】下方 Running Sum 是可執行整合題；【實務】則整合 ownership、reference
 * 與 permission mask。【複雜度】Running Sum 為 O(n) time/O(n) output space，其餘語法
 * 抽象通常無額外 runtime 成本，但 std::unique_ptr 配置仍取決於建立的物件。
 *
 * 【功能選擇表】
 * ┌────────────────────────┬───────────────────────────────────────────────────┐
 * │ 功能                   │ 選擇準則 / 一句話記憶                            │
 * ├────────────────────────┼───────────────────────────────────────────────────┤
 * │ auto function return   │ 回 value，像模板傳值推導，會去 const/reference。 │
 * │ decltype(auto)         │ 精確保留 decltype 規則；強大但要審 lifetime。     │
 * │ relaxed constexpr      │ 可用 local/if/loop 寫正常 compile-time algorithm。│
 * │ binary literal         │ 0b 直接顯示 bit fields；搭 unsigned 操作。        │
 * │ digit separator        │ 只改善 source 可讀性，完全沒有 runtime cost。     │
 * │ make_unique            │ RAII 單一 ownership 的預設 factory。             │
 * │ [[deprecated]]         │ 漸進 API migration；仍能呼叫，不是 enforcement。 │
 * └────────────────────────┴───────────────────────────────────────────────────┘
 *
 * 【auto return vs decltype(auto)】
 * const int& source();
 * auto a() { return source(); }            // int，複製
 * decltype(auto) b() { return source(); }  // const int&，保留 reference
 *
 * 面試快問：
 * Q1: 多個 return 可以是 int 與 double 嗎？
 * A1: auto return deduction 不會找 common_type；必須同型，或自行明確轉換/寫回傳型別。
 * Q2: `decltype(auto) f(){ int x=1; return (x); }` 有何問題？
 * A2: 回 int& 指向 local，函式結束後 dangling；括號改變 decltype 規則。
 * Q3: 公開 library API 是否都改 auto？
 * A3: 不必。明確型別可表達契約並穩定 ABI；auto 適合型別依實作自然產生的情境。
 *
 * 【constexpr 演進】
 * - C++11：function body 幾乎只容許一個 return expression。
 * - C++14：local variable、branch、loop 可進 constexpr，iterative DP/查表變自然。
 * - constexpr function 可 runtime 執行；只有 static_assert、array bound 等 context 強制常數求值。
 * - compile-time 不是免費：大型表與複雜模板會增加 build time。
 *
 * 【位元與數字 literal】
 * - `0b1111'0000U`：binary 表 pattern，separator 按 nibble 分組。
 * - set: flags |= mask；clear: flags &= ~mask；toggle: flags ^= mask；test: flags & mask。
 * - 優先 unsigned：signed left shift、overflow 與 sign bit 容易踩 UB。
 * - separator 不能放在 literal 開頭/結尾、不能亂放在 decimal point/exponent 旁。
 *
 * 【ownership 速查】
 * ┌──────────────────┬─────────────────────────────────────────────────────────┐
 * │ 表達             │ 用途                                                    │
 * ├──────────────────┼─────────────────────────────────────────────────────────┤
 * │ unique_ptr<T>    │ 唯一 owner；可 move 不可 copy；預設首選。               │
 * │ make_unique<T>   │ 配置並立刻接管；constructor throw 時不洩漏。            │
 * │ T* / T&          │ non-owning observer；文件需說 lifetime。                 │
 * │ shared_ptr<T>    │ 真正共享 lifetime；control block/atomic 有成本。         │
 * └──────────────────┴─────────────────────────────────────────────────────────┘
 * move 後 unique_ptr 保證為 null；標準函式庫型別通常依 library requirements 保持
 * valid but unspecified。使用者自訂型別則完全取決於自己的 move 實作與契約。
 * polymorphic unique_ptr<Base> 要求 Base virtual destructor。
 *
 * 【API 淘汰流程】
 * 1. 先實作 replacement 並提供 migration message。
 * 2. 舊 API 加 [[deprecated("use new_api")]]，CI 可逐步把 warning 升 error。
 * 3. 修 call sites、觀測使用量、公告 removal release。
 * 4. major version 才刪除，避免 silent behavior change。
 * deprecated 不等於 deleted：前者有相容期，後者直接禁止 overload。
 *
 * 【高頻陷阱清單】
 * 1. auto return 意外複製 reference-returning API。
 * 2. decltype(auto) 因多一層括號回傳 local reference。
 * 3. constexpr function 寫了 loop 卻用 C++11 mode 編譯。
 * 4. `1 << 31` 使用 signed int；改用正確寬度 unsigned constant。
 * 5. digit separator 改善閱讀但沒有 range checking，巨大 literal 仍可能型別不合。
 * 6. release()/裸 new 讓 ownership 斷裂；優先 make_unique 與自然 scope。
 * 7. deprecated 訊息沒有替代 API，團隊只能忽略 warning。
 *
 * 【整合面試題】
 * - 為何 make_unique 在 C++14 才進標準，而 unique_ptr 在 C++11 已存在？
 * - 描述 auto 與 decltype(auto) 在 proxy type（vector<bool>）上的差異。
 * - 寫一個 constexpr popcount，說明在哪些 input 下真的 compile time。
 * - API 已 deprecated 三年仍不能刪，反映了什麼 release/process 問題？
 *
 * 【練習】
 * 1. 使用下方 admin=0b1000，測試「同時需要 read+admin」。
 * 2. 將 leetcode_running_sum 泛化成 vector<long long>，觀察 auto return。
 * 3. 故意把 practical_scale 回傳 `(value)` 的 reference，解釋為何不能這樣做。
 */

/*
==============================================================================
【面試深挖：C++14】

M14-1｜generic lambda 本質？
答：參數寫 auto，closure 的 operator() 成為 function template；每種 argument type 各自
instantiation。它不是 type erasure，也不等同把所有型別裝進同一 runtime function。

M14-2｜init-capture 解決什麼？
答：可在 closure 內建立新 member，例如 `[p=std::move(ptr)]` 捕獲 move-only ownership，
或 `[count=0]` 建 state。捕獲名稱的 initializer 在外部 scope 評估。

M14-3｜`decltype(auto)` 為何危險也有用？
答：它完整套用 decltype，能保留 reference/value category，適合 forwarding wrapper；
但 `return (local);` 會推得 reference 並 dangling。plain auto return 通常去 reference。

M14-4｜auto return type deduction 的限制？
答：所有非 discarded return statements 必須推得相容同一型別；definition 通常要可見才能
呼叫端得知 return type。recursive call 在 deduction 前也可能無法使用。

M14-5｜C++14 constexpr 放寬了什麼？
答：允許較一般 function body，包括 local variables、loops、branches 與修改局部 state，
使 compile-time algorithm 更像普通 C++；仍須在 constant evaluation 路徑遵守限制。

M14-6｜`make_unique` 為何到 C++14 才加入？
答：補齊 make_shared 對應的 exception-safe、少寫 raw new 的 factory；對 array 也有適當
overload。它不支援 custom deleter 參數，需直接建 unique_ptr。

M14-7｜variable template 的用途？
答：讓一族值依 template arguments 定義，例如 `pi_v<T>` 或 type trait 的 `_v` helper；
每個 specialization 仍受 linkage/ODR 規則，不是 preprocessor constant。

M14-8｜heterogeneous lookup 為何算 C++14 實務亮點？
答：ordered associative containers 使用 transparent comparator（如 less<>）後，可用
string_view/char pointer 查 string key，不先建 temporary string；比較規則必須跨型別一致。

M14-9｜digit separator 與 binary literal 有 runtime 成本嗎？
答：沒有，只是 token syntax；separator 不可任意放在小數點、prefix 等位置。
面試價值在版本辨識與可讀性，不應誇大成效能特性。
==============================================================================
*/

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace review {
enum Permission : std::uint8_t {
    read = 0b001,
    write = 0b010,
    execute = 0b100,
    admin = 0b1000
};

constexpr int popcount(std::uint32_t value) {
    int count = 0;
    while (value != 0U) {
        value &= value - 1U;
        ++count;
    }
    return count;
}

static_assert(popcount(0b1011U) == 3, "constexpr popcount 必須正確");

auto leetcode_running_sum(const std::vector<int>& nums) {
    std::vector<int> output;
    output.reserve(nums.size());
    int total = 0;
    for (const int number : nums) {
        total += number;
        output.push_back(total);
    }
    return output;
}

class Scale {
public:
    explicit Scale(int factor) : factor_(factor) {}
    int apply(int value) const { return value * factor_; }
private:
    int factor_;
};

auto practical_make_scale(int factor) {
    return std::make_unique<Scale>(factor);
}

decltype(auto) practical_first(std::vector<int>& values) {
    if (values.empty()) {
        throw std::out_of_range("practical_first requires a nonempty vector");
    }
    return (values.front());
}

bool practical_has_all(std::uint8_t granted, std::uint8_t needed) {
    return (granted & needed) == needed;
}

void integrated_test() {
    const auto sums = leetcode_running_sum({1, 2, 3, 4});
    assert(sums == std::vector<int>({1, 3, 6, 10}));

    std::unique_ptr<Scale> scaler = practical_make_scale(3);
    assert(scaler->apply(7) == 21);

    std::vector<int> values{5, 6};
    static_assert(std::is_same<decltype(practical_first(values)), int&>::value,
                  "decltype(auto) 應保留 lvalue reference");
    practical_first(values) = 9;
    assert(values.front() == 9);

    const std::uint8_t developer = static_cast<std::uint8_t>(read | write);
    const std::uint8_t administrator = static_cast<std::uint8_t>(read | admin);
    assert(practical_has_all(developer, read));
    assert(!practical_has_all(developer, execute));
    assert(practical_has_all(administrator, static_cast<std::uint8_t>(read | admin)));
}
}  // namespace review

int main() {
    review::integrated_test();
    std::cout << "C++14 面試總複習：推導、constexpr、bits、ownership、遷移皆通過\n";
}
