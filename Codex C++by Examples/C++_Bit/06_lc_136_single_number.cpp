// ============================================================================
// LeetCode 136：Single Number
// ============================================================================
//
// 條件：除一個數出現一次，其餘每個數恰出現兩次。利用 XOR：
//   x^x=0、x^0=x，且可交換/結合，因此配對值不論順序都抵消。
// 時間 O(N)、額外空間 O(1)，不需 sort/hash table。
//
// 前置條件不可忽略：若其他值不是恰兩次（例如三次），這個結果不代表 single number。
// XOR 對 signed int 的 bitwise operation 有明確結果，但若要跨平台解釋 raw bits，使用
// fixed-width unsigned 會更直觀。
// ============================================================================

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vector>

int single_number(const std::vector<int>& nums)
{
    if (nums.empty()) throw std::invalid_argument("input must not be empty");
    int result = 0;
    for (const int value : nums) result ^= value;
    return result;
}

// 基礎示範：XOR 的交換律與結合律讓成對資料不必相鄰。
void basic_example()
{
    assert((42 ^ 42) == 0);
    assert((0 ^ 17) == 17);
    assert((9 ^ 4 ^ 9) == 4);
    assert((4 ^ 9 ^ 9) == 4);
    std::cout << "[基礎] 相同值 XOR 抵消，順序不影響結果\n";
}

void leetcode_example()
{
    assert(single_number({2, 2, 1}) == 1);
    assert(single_number({4, 1, 2, 1, 2}) == 4);
    assert(single_number({-3, 7, 7}) == -3);
    std::cout << "[LeetCode 136] answers 1, 4, -3\n";
}

// 實務：兩份相同 ID 記錄成對抵消，找出只出現在其中一邊的 transaction ID。
// 這只適用「恰有一個 unmatched ID」；一般資料 reconciliation 應用 set/count map。
int unmatched_transaction(const std::vector<int>& ids)
{
    return single_number(ids);
}

void practical_example()
{
    const std::vector<int> combined{101, 205, 101, 309, 205};
    assert(unmatched_transaction(combined) == 309);
    std::cout << "[實務] unmatched transaction id=309\n";
}

int main()
{
    basic_example();
    leetcode_example();
    practical_example();
}

// 易錯與面試：
//   * XOR 解法不是「找出現次數最少者」的通用演算法；它精確依賴其餘值出現偶數次。
//   * 若有兩個 unmatched values，總 XOR 只得到 a^b，不能直接分別知道 a、b。
//   * 空輸入在 LeetCode 契約不會出現；production API 要像本例明確拒絕。
//
// 延伸解法：LC 260 可取 xor_all 的最低 set bit，把資料分兩組再各自 XOR；LC 137 則
// 需要逐 bit modulo 3 或有限狀態 bit masks。這些變化都來自「出現次數契約」不同。
//
// 複雜度：一次走訪 O(N)、額外 O(1)；sort 解法 O(NlogN)，hash map 平均 O(N) 但 O(N)
// 空間。面試不只要背 XOR，還要先向面試官確認輸入是否真的滿足配對條件。
// 生命週期：函式以 const reference 借用 vector，只在呼叫期間讀取，不保存 iterator/reference。
// API 契約：production 若不能信任「恰成對」前置條件，應改回傳 optional/result 並先驗頻次。
// 練習：LC 137 中其他值出現三次，需累計每個 bit modulo 3，單純 XOR 已不適用。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_lc_136_single_number.cpp' -o '/tmp/codex_cpp_C_Bit_06_lc_136_single_number' && '/tmp/codex_cpp_C_Bit_06_lc_136_single_number'
//
// === 預期輸出（節錄）===
// [實務] unmatched transaction id=309
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
