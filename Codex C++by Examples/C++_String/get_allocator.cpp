/*
 * get_allocator()：取得 string 使用的 allocator 副本
 *
 * 一般程式幾乎不需呼叫它；allocator-aware 容器、記憶體資源或泛型函式才常用。
 * 回傳的是 allocator 物件副本，不是字串內部 buffer。標準 std::allocator<char>
 * 通常 stateless 且 always_equal；自訂 allocator 則可能帶 arena 身分。比較 allocator
 * 可用來檢查操作前置條件，但不能把不合法的 swap 解讀成「自動改走線性版本」。
 */

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <string>
#include <type_traits>
#include <utility>

namespace {

// 不使用 assert，讓測試在 -DNDEBUG 建置中仍會執行。
void expect(const bool condition) {
    if (!condition) std::abort();
}

void basic_demo() {
    const std::string text = "allocator";
    auto allocator = text.get_allocator();
    using Allocator = decltype(allocator);
    static_assert(std::is_same_v<typename Allocator::value_type, char>);
    static_assert(std::allocator_traits<Allocator>::is_always_equal::value);

    // 示範 allocator protocol；實務優先讓容器管理，不要手寫 raw allocation。
    char* memory = std::allocator_traits<Allocator>::allocate(allocator, 4U);
    std::copy_n("abc", 4U, memory);
    expect(std::string(memory) == "abc");
    std::allocator_traits<Allocator>::deallocate(allocator, memory, 4U);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram（有效的字母異位詞）
// 題目：判斷兩個小寫英文字串是否由完全相同的字母與次數組成；anagram/nagaram 為 true。
// 為何使用本章主題：此題不需要 get_allocator；固定 26 格 array 比額外配置 map/string 更合適，
//       這是刻意展示 allocator 介面不應為了章節而硬塞進一般演算法。
// 思路：1. 長度不同直接失敗；2. 第一字串累加 26 格計數；3. 第二字串遞減；4. 檢查全為零。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是字串長度，字母表固定為 26。
// 易錯點：原題保證小寫 a..z；一般輸入必須先驗證，否則 `ch-'a'` 可能越界。
// -----------------------------------------------------------------------------
bool leetcode_is_anagram(const std::string& first, const std::string& second) {
    if (first.size() != second.size()) {
        return false;
    }
    std::array<int, 26U> counts{};
    for (const char ch : first) ++counts[static_cast<std::size_t>(ch - 'a')];
    for (const char ch : second) --counts[static_cast<std::size_t>(ch - 'a')];
    return std::all_of(counts.begin(), counts.end(), [](const int value) { return value == 0; });
}

// -----------------------------------------------------------------------------
// 【日常實務範例】PMR 字串交換前置條件稽核
// 情境：兩個 pmr::string 可能來自不同 memory_resource，呼叫 swap 前要確認 allocator 契約成立。
// 為何使用本章主題：get_allocator() 可取得各容器 allocator 副本，搭配 allocator_traits 的
//       propagate_on_container_swap 判定；相較直接 swap，可避免不相等 allocator 下的前置條件違反。
// 設計：1. 編譯期讀 propagation trait；2. 可傳播則允許；3. 否則比較兩 allocator；4. 範例改用
//       有定義但可能線性搬移的 move assignment。
// 成本：前置條件檢查 O(1)；allocator 不相等的 move assignment 可能 O(N) 並向目的 resource 配置。
// 上線注意：memory_resource 必須活得比所有容器久；不能把 `swap 不合法` 解讀成自動線性 fallback，
//       也不要保存 get_allocator 回傳值來假裝替換容器內 allocator。
// -----------------------------------------------------------------------------
template <typename String>
bool member_swap_precondition_holds(const String& first, const String& second) {
    using Allocator = typename String::allocator_type;
    using Traits = std::allocator_traits<Allocator>;
    if constexpr (Traits::propagate_on_container_swap::value) {
        return true;
    }
    return first.get_allocator() == second.get_allocator();
}

void practical_allocator_contracts() {
    std::pmr::monotonic_buffer_resource destination_resource;
    std::pmr::monotonic_buffer_resource source_resource;
    std::pmr::string destination{"old value", &destination_resource};
    std::pmr::string source{"replacement value that exceeds a typical small-string buffer",
                            &source_resource};

    using Allocator = std::pmr::string::allocator_type;
    using Traits = std::allocator_traits<Allocator>;
    static_assert(!Traits::propagate_on_container_swap::value);
    static_assert(!Traits::propagate_on_container_move_assignment::value);

    expect(!member_swap_precondition_holds(destination, source));
    // destination.swap(source);  // 前置條件違反：不得用這行期待線性 fallback。

    // move assignment 是不同操作：allocator 不等仍有定義，但可能配置並線性搬移內容。
    destination = std::move(source);
    expect(destination == "replacement value that exceeds a typical small-string buffer");
    expect(destination.get_allocator().resource() == &destination_resource);
}

}  // namespace

int main() {
    basic_demo();
    expect(leetcode_is_anagram("anagram", "nagaram"));
    expect(!leetcode_is_anagram("rat", "car"));
    practical_allocator_contracts();
    std::cout << "get_allocator: tests passed\n";
}

/*
 * 【陷阱】allocate(n) 只給 raw storage；對非平凡型別還要 construct/destroy。此例 char 平凡。
 * 【面試】allocator 不等時 swap 與 move assignment 有何不同？若 swap 不 propagate，
 * 不等 allocator 會違反 swap 前置條件；move assignment 不 propagate 時仍可逐元素搬移。
 * 【練習】閱讀 std::pmr::string，說明 polymorphic_allocator 如何把配置導向 memory_resource。
 */

/*
 * 【allocator 面試速查】
 * - get_allocator 回 value，不是 container 內部 allocator 的可任意替換 reference。
 * - allocator_traits 統一 allocate/deallocate/construct 等 protocol。
 * - swap 的複雜度固定為 O(1)；不 propagate 且 allocator 不等時不能呼叫，沒有線性 fallback。
 * - move assignment 的規則不同：不 propagate 且 allocator 不等仍有定義，但可能是線性操作。
 * - `std::pmr::string` 是使用 polymorphic_allocator<char> 的別名，資源生命週期必須
 *   長於所有仍使用它配置記憶體的容器。
 * - 初學者不應為一般字串手動 allocate；本例只是解釋 container 底層契約。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'get_allocator.cpp' -o '/tmp/codex_cpp_C_String_get_allocator' && '/tmp/codex_cpp_C_String_get_allocator'
//
// === 預期輸出（節錄）===
// get_allocator: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
