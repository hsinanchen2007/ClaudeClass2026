/*
 * C++17 面試前總複習：語法可讀性、compile-time 分流與零配置 view
 * ============================================================================
 * C++17 是現代 C++ 的重要實務基線。本總章不是檔名列表，而是面試前可直接快速回想
 * 13 個主題的選擇準則、複雜度、生命週期、陷阱與回答框架。
 * 【LeetCode】下方 Two Sum 為可執行題；實務整合 header view、encode、invoke 與 byte wire format。
 *
 * 【一、控制流程與解構】
 * ┌─────────────────────┬─────────────────────────────────────────────────────┐
 * │ 功能                │ 必記規則                                            │
 * ├─────────────────────┼─────────────────────────────────────────────────────┤
 * │ structured binding  │ value element 常見為複製；reference element 仍可 alias。│
 * │ if (init; cond)     │ init 變數涵蓋 then+else，整段結束才析構。            │
 * │ switch(init; value) │ condition 仍須 integral/enum；case 內注意 scope。    │
 * │ [[fallthrough]]     │ 只表明刻意貫穿，不會產生 jump 或安全檢查。          │
 * └─────────────────────┴─────────────────────────────────────────────────────┘
 *
 * 快問快答：
 * Q: `for (auto [k,v] : map)` 能修改 map value 嗎？ A: 不能，拆的是元素副本。
 * Q: if-init iterator 在 else 可用嗎？ A: 可以；離開整個 if 才失效。
 * Q: 多個 case 共用完全相同 body 需要 fallthrough 嗎？ A: 不要，直接堆疊 case labels。
 *
 * 【二、compile-time 與 ODR】
 * - if constexpr：condition 在 instantiation 時決定，未選 branch 被 discarded；普通 if
 *   兩邊都需合法。適合 template local type dispatch，不是 runtime optimization switch。
 * - inline variables：允許 header 多 TU 相同定義，核心是 ODR/linkage，不是 inline 效能。
 * - CTAD：由 constructor arguments 推導 class template arguments；必要時寫 deduction guide。
 *   它與 auto variable deduction 是不同層次，initializer_list/decay 仍可能令人意外。
 * - nested namespace `a::b` 只縮短語法；不提供 private access，也不自動做 API versioning。
 *
 * 【三、attributes】
 * - [[nodiscard]]：忽略回傳值應診斷；適合 error/result/validation，不保證 caller 真處理。
 * - [[maybe_unused]]：允許 build variant 合理未使用；不要掩蓋 dead code。
 * - [[fallthrough]]：刻意從一個 case 落到下一 case；只放必要位置。
 * Attributes 可被 implementation 依規範處理，但不能當 runtime security boundary。
 *
 * 【四、string_view 生命週期】
 * - non-owning pointer+size；由 pointer+length、string 或另一個 view 建構及 substr 為
 *   O(1)。只給 null-terminated `const char*` 的 constructor 必須找 `\0`，是 O(N)。
 * - 不保證 null terminated；`.data()` 不能無條件交給 strlen/printf("%s")。
 * - underlying string destruct/reallocate/modify 可能讓 view dangling 或內容改變。
 * - 只在 call 期間讀取的 parameter 很適合；要保存就 copy 成 std::string。
 * 面試必抓：`std::string_view v = std::string("temporary");` statement 後 v dangling。
 *
 * 【五、raw bytes 與 callable】
 * - std::byte：raw storage，支援 bitwise/shift，不支援 arithmetic；用 to_integer 讀數值。
 * - byte 不處理 endian/padding/alignment/object lifetime；wire format 應逐欄 encode。
 * - std::invoke：統一普通 callable、member function pointer、data member pointer。
 * - invoke 是 compile-time generic dispatch，沒有要求 type erasure；std::function 才擦除型別。
 *
 * 【複雜度/成本速查】
 * ┌────────────────────────────┬──────────────────────────────────────────────┐
 * │ 操作                       │ 複雜度/成本                                  │
 * ├────────────────────────────┼──────────────────────────────────────────────┤
 * │ string_view 建構/substr    │ 已知長度 O(1)；僅 const char* 建構 O(N)       │
 * │ string_view find           │ 依內容/實作，不能宣稱一律 O(n)              │
 * │ structured binding by value│ 元素複製成本；大型 mapped value 可能很昂貴  │
 * │ if/switch init             │ 無額外抽象成本；只縮短 scope                │
 * │ if constexpr discarded arm│ specialization 中無 runtime branch          │
 * │ std::invoke                │ 通常可 inline；語意依 underlying callable   │
 * └────────────────────────────┴──────────────────────────────────────────────┘
 *
 * 【高頻陷阱】
 * 1. structured binding 忘記 &，修改只落在副本。
 * 2. if-init 中持有 mutex lock，else 也仍在臨界區，造成鎖太久。
 * 3. if constexpr condition 偷用 runtime variable；或以為 parser 不必看 discarded branch。
 * 4. inline variable 有動態初始化，仍踩跨 TU initialization order。
 * 5. CTAD 推到 const char* 而非 std::string，owner/view 語意錯誤。
 * 6. nodiscard warning 被 static_cast<void> 全面壓掉，錯誤仍未處理。
 * 7. string_view 指向 temporary/local 或原 string reallocation 後繼續使用。
 * 8. memcpy struct 當 network packet，忽略 padding/endian/ABI。
 * 9. overloaded member function 取址後交 invoke，未 static_cast 指定簽名。
 *
 * 【面試問答】
 * Q1: if constexpr 與 SFINAE 的差別？
 * A1: if constexpr 在 function body 內分支；SFINAE 可從 overload set 排除 candidate。
 * Q2: inline variable 是否每個 TU 都有一份？
 * A2: ODR 語意上是同一 entity/address；implementation 負責合併。
 * Q3: string_view 為何通常應 by value 傳？
 * A3: 它本身很小，複製 pointer+length；const& 反而增加 indirection，且不增加 lifetime。
 * Q4: std::byte 可以直接 +1 嗎？
 * A4: 不行，刻意不提供 arithmetic；先 to_integer，明確決定語意後再轉回。
 *
 * 【練習】
 * 1. 為下方 parse_header 加 trim，仍不得配置新 string。
 * 2. 將 encode 改成 big-endian 16-bit length，不能 memcpy struct。
 * 3. 寫 overload member function，使用 static_cast + invoke 選定 const 版本。
 */

/*
==============================================================================
【面試深挖：C++17】

M17-1｜structured binding 有哪三種路徑？
答：array、tuple-like（tuple_size/tuple_element/get）、以及符合規則的 public data members。
不是所有 structured binding 都「底層依賴 tuple_size」。

M17-2｜`auto [a,b]=obj` 與 `auto& [a,b]=obj`？
答：前者先建立 hidden object 的 copy，再綁其 subobjects；後者綁原 object。
`decltype(a)` 又有 structured-binding 特殊規則，不能只看表面是否像 reference。

M17-3｜`if constexpr` 與普通 if？
答：在 template instantiation 中未選 branch 是 discarded statement，可避免該 branch
對此 specialization 的無效程式被 instantiation；非 template context 仍需基本語法/語意有效。

M17-4｜CTAD 解決什麼，不能做什麼？
答：由 constructor arguments/deduction guides 推 class template arguments，例如 pair p(1,2)。
它不是 function return type inference，且錯誤 deduction guide 可導致 surprising type。

M17-5｜`optional<T>` 與 pointer/哨兵值如何選？
答：optional 表「可能沒有一個 T value」且 inline 儲存，不表 polymorphism/ownership；
reference optional 到 C++26 前不是一般 `optional<T&>`。若需要失敗原因，用 expected 類型。

M17-6｜`variant` 與 union？
答：variant 是 tagged union，知道 active alternative、管理 lifetime 並可 visit；仍可能
`valueless_by_exception`。duplicate alternative types 合法，但 get<T> 要求 T 唯一。

M17-7｜`any`、`variant`、virtual interface 怎麼選？
答：variant 型別集合封閉且 compile-time exhaustive；any 完全 open 但 runtime cast/type erasure；
virtual 適合 open set 且共享行為 contract。不要只比較 syntax。

M17-8｜C++17 guaranteed copy elision 保證什麼？
答：某些 prvalue 直接初始化 destination，不先建立 temporary，因此即使 copy/move deleted 也可。
NRVO 對具名 local 仍是允許優化，不是同一類 mandatory guarantee。

M17-9｜inline variable 解決什麼？
答：允許 header 中同一 inline variable definition 出現在多個 translation units 並指向同一 entity，
類似 inline function 的 ODR 模型；不是「編譯器一定把變數 inline 到指令」。

M17-10｜fold expression 四種形式的陷阱？
答：unary left/right 與 binary left/right；空 parameter pack 只有少數 operator 有 identity，
binary fold 可由 init 定義空包結果。減法/stream 等非結合 operation 的左右方向很重要。

M17-11｜`string_view` 為何是 C++17 面試高頻？
答：它展現 vocabulary type、zero-copy 與 lifetime trade-off。正確答案必須同時說
non-owning、可含 NUL、不保證 null-terminated，以及 temporary/reallocation dangling。

M17-12｜C++17 如何處理 over-aligned type 的動態配置？
答：若型別需要超過一般 new 保證的 alignment，new-expression 可選帶 `std::align_val_t` 的
allocation function，delete 必須走相配的 aligned deallocation overload。自訂 allocator/operator new
若漏掉 alignment，建構 SIMD/cache-line aligned object 可能形成 UB；「通常很整齊」不是標準證明。
==============================================================================
*/

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace review {
struct HeaderView {
    std::string_view name;
    std::string_view value;
};

HeaderView practical_parse_header(std::string_view line) {
    if (const std::size_t colon = line.find(':'); colon != std::string_view::npos) {
        std::string_view value = line.substr(colon + 1U);
        if (!value.empty() && value.front() == ' ') value.remove_prefix(1U);
        return {line.substr(0U, colon), value};
    }
    return {line, {}};
}

std::pair<int, int> leetcode_two_sum(const std::vector<int>& nums, int target) {
    std::unordered_map<long long, int> seen;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        const long long needed = static_cast<long long>(target) - nums[i];
        if (const auto found = seen.find(needed); found != seen.end()) {
            return {found->second, static_cast<int>(i)};
        }
        seen.emplace(static_cast<long long>(nums[i]), static_cast<int>(i));
    }
    return {-1, -1};
}

template <class T>
std::string practical_encode(const T& value) {
    if constexpr (std::is_integral<T>::value) {
        return std::to_string(value);
    } else {
        return std::string(value);
    }
}

struct Transformer {
    int factor;
    int apply(int value) const { return factor * value; }
};

std::array<std::byte, 2> practical_encode_length(unsigned length) {
    if (length > 0xFFFFU) {
        throw std::out_of_range("16-bit length overflow");
    }
    return {std::byte{static_cast<unsigned char>((length >> 8U) & 0xFFU)},
            std::byte{static_cast<unsigned char>(length & 0xFFU)}};
}

void integrated_test() {
    const auto [left, right] = leetcode_two_sum({2, 7, 11, 15}, 9);
    assert(left == 0 && right == 1);
    const auto extreme = leetcode_two_sum(
        {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}, -1);
    assert(extreme.first == 0 && extreme.second == 1);

    const std::string line = "Content-Type: text/plain";
    const auto [name, value] = practical_parse_header(line);
    assert(name == "Content-Type" && value == "text/plain");

    assert(practical_encode(42) == "42");
    assert(practical_encode("ok") == "ok");

    const Transformer transformer{3};
    assert(std::invoke(&Transformer::apply, transformer, 7) == 21);

    const auto bytes = practical_encode_length(0x1234U);
    assert(std::to_integer<unsigned>(bytes[0]) == 0x12U);
    assert(std::to_integer<unsigned>(bytes[1]) == 0x34U);
    const auto largest = practical_encode_length(0xFFFFU);
    assert(std::to_integer<unsigned>(largest[0]) == 0xFFU);
    bool rejected = false;
    try {
        static_cast<void>(practical_encode_length(0x10000U));
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    assert(rejected);
}
}  // namespace review

int main() {
    review::integrated_test();
    std::cout << "C++17 面試總複習：scope、推導、attributes、view、byte、invoke 皆通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Cpp17_summary' && '/tmp/codex_cpp_C_Cpp17_summary'
//
// === 預期輸出（節錄）===
// C++17 面試總複習：scope、推導、attributes、view、byte、invoke 皆通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
