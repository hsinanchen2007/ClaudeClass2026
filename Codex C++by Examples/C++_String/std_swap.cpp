/*
 * std::swap(string,string)：泛型程式中的交換介面
 *
 * 對 std::string 有適當 overload，效果呼叫 member swap。泛型模板常使用：
 *   using std::swap; swap(a,b);
 * 讓 ADL 能找到自訂型別專屬 swap，又保留 std::swap fallback。不要在 std namespace
 * 為使用者型別任意新增 overload（標準允許的特化規則非常有限）。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>

namespace {

void basic_demo() {
    std::string first = "left";
    std::string second = "right";
    std::swap(first, second);
    assert(first == "right" && second == "left");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：原地反轉字元序列且只使用常數額外空間；hello 變成 olleh。
// 為何使用本章主題：std::swap 對兩個 char reference 交換兩端元素，讓泛型交換意圖比手動 temporary 清楚；
//       本檔把原題 vector<char> 改為 string，演算法相同。
// 思路：1. left 從 0、right 從 size；2. 先遞減 right；3. 尚未交錯就 swap；4. left 前進。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：空字串不可先算 size()-1；此處 swap 的是 char，不涉及兩個 string allocator 契約。
// -----------------------------------------------------------------------------
void leetcode_reverse_string(std::string& text) {
    for (std::size_t left = 0U, right = text.size(); left < right; ++left) {
        --right;
        if (left >= right) break;
        std::swap(text[left], text[right]);
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】無向邊 canonical key
// 情境：nodeA-nodeB 與 nodeB-nodeA 必須產生同一 cache key，因此較小字串固定放在左側。
// 為何使用本章主題：條件成立時 std::swap 交換兩個 owning string，較另做兩份複製直接；
//       固定排序後再串接可讓 hash/cache lookup 穩定。
// 設計：1. 以字典序比較兩端；2. second 較小就 swap；3. 依固定箭頭格式串接。
// 成本：比較與輸出時間 O(A+B)、額外空間 O(A+B)，A、B 是兩端字串長度；合法 string swap 為 O(1)。
// 上線注意：字典序是 byte/code-unit 規則；節點名稱含 delimiter 時需 escaping，且自訂 allocator
//       不相等時必須先確認 swap 前置條件。
// -----------------------------------------------------------------------------
std::string practical_canonical_edge(std::string first, std::string second) {
    if (second < first) std::swap(first, second);
    return first + "->" + second;
}

}  // namespace

int main() {
    basic_demo();
    std::string word = "hello";
    leetcode_reverse_string(word);
    assert(word == "olleh");
    assert(practical_canonical_edge("nodeB", "nodeA") == "nodeA->nodeB");
    std::cout << "std::swap: tests passed\n";
}

/*
 * 【面試】為何泛型程式先 `using std::swap` 再無限定呼叫？同時啟用 std fallback 與 ADL。
 * 【陷阱】`std::swap(a,b)` 寫死可能略過自訂型別在自己 namespace 的高效 swap。
 * 【練習】定義含大型 string 的 Record，提供 noexcept member swap 與 free swap。
 */

/*
 * 【泛型 swap 慣用法】
 * template<class T>
 * void algorithm(T& a, T& b) {
 *     using std::swap;
 *     swap(a,b); // unqualified，讓 ADL 找到 T namespace 的 overload
 * }
 *
 * 【為何重要】大型自訂型別若只走 generic copy/move swap，可能較慢或 noexcept 條件較差；
 * 專屬 swap 可直接交換成員。std::string 已由標準庫提供合適 overload。
 *
 * 【陷阱】不要替一般使用者型別在 namespace std 隨意新增 overload；在型別自己的
 * namespace 定義 free swap，內部呼叫 member swap。
 * 【面試題】member swap 與 std::swap(string) 結果相同，前者直接；後者適合泛型語境。
 */

/*
 * 【教科書補充：string swap 與 allocator】
 * - 在合法 allocator 前置條件下，basic_string::swap 為常數複雜度，不逐字元交換內容。
 * - 若 allocator 不會隨 swap 傳播且兩 allocator 不相等，呼叫 swap 是 UB，不存在自動線性 fallback。
 * - noexcept 性質與 allocator traits/標準版本相關；泛型 code 應查 `is_nothrow_swappable_v<T>`。
 * - swap 後不要依賴舊 iterator/view/pointer 所屬的物件身分；最清楚的做法是重新取得 handle。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'std_swap.cpp' -o '/tmp/codex_cpp_C_String_std_swap' && '/tmp/codex_cpp_C_String_std_swap'
//
// === 預期輸出（節錄）===
// std::swap: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
