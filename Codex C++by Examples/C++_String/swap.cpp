/*
 * string::swap：交換兩個字串的內容
 *
 * 在前置條件成立時，標準保證複雜度為常數時間。若
 * allocator_traits<Allocator>::propagate_on_container_swap 為 false，兩邊 allocator
 * 必須相等；不相等仍呼叫 swap 是前置條件違反，不存在改做逐字元交換的線性 fallback。
 * 這項限制與 noexcept 的 allocator traits 條件是不同問題。
 * 交換後，變數名稱不變但其內容互換。
 * iterator/reference 的歸屬語意較細，實務最安全做法是交換後重新取得。
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {

// 不使用 assert，讓測試在 -DNDEBUG 建置中仍會執行。
void expect(const bool condition) {
    if (!condition) std::abort();
}

void basic_demo() {
    using Allocator = std::string::allocator_type;
    static_assert(std::allocator_traits<Allocator>::is_always_equal::value);

    std::string active = "v1";
    std::string staging = "v2";
    active.swap(staging);
    expect(active == "v2" && staging == "v1");
}

// LeetCode 1790（Check if One String Swap Can Make Strings Equal）。
bool leetcode_one_swap_equal(std::string first, const std::string& second) {
    if (first.size() != second.size()) return false;
    std::size_t left = first.size();
    std::size_t right = first.size();
    for (std::size_t i = 0U; i < first.size(); ++i) {
        if (first[i] != second[i]) {
            if (left == first.size()) left = i;
            else if (right == first.size()) right = i;
            else return false;
        }
    }
    if (left == first.size()) return true;
    if (right == first.size()) return false;
    const char saved = first[left];
    first[left] = first[right];
    first[right] = saved;
    return first == second;
}

// 實務：先完整建 staging，驗證後才 swap 發佈，提供強烈的狀態切換邊界。
bool practical_publish_if_valid(std::string& active, std::string staging) {
    if (staging.empty() || staging.find("INVALID") != std::string::npos) return false;
    active.swap(staging);
    return true;
}

}  // namespace

int main() {
    basic_demo();
    expect(leetcode_one_swap_equal("bank", "kanb"));
    expect(!leetcode_one_swap_equal("attack", "defend"));
    std::string config = "old";
    expect(practical_publish_if_valid(config, "new") && config == "new");
    expect(!practical_publish_if_valid(config, "INVALID") && config == "new");
    std::cout << "member swap: tests passed\n";
}

/*
 * 【面試】copy assignment 與 swap 發佈差異？先建 staging 再 swap 可把昂貴/會失敗的工作
 * 放在 active 之外，驗證完成才切換。
 * 【練習】讓 practical_publish_if_valid 回傳被替換的舊值，思考 copy 與 move 成本。
 */

/*
 * 【陷阱】swap 後變數名稱沒有變，但內容/資源歸屬已交換；不要以舊名稱推斷內容。
 * 【考前速查】member swap 適合已知 string；泛型程式採 using std::swap + unqualified swap。
 * string::swap 的標準複雜度固定是 O(1)；allocator 不相容不是 O(n) fallback，而是不能呼叫。
 * move assignment 是另一份契約：不 propagate 且 allocator 不等時仍有定義，但可能線性搬移。
 * 交換後重新取得 iterator/view/pointer，是避免歸屬誤判的簡單規則。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'swap.cpp' -o '/tmp/codex_cpp_C_String_swap' && '/tmp/codex_cpp_C_String_swap'
//
// === 預期輸出（節錄）===
// member swap: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
