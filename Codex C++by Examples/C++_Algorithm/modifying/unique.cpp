/*
 * std::unique / unique_copy：移除「相鄰」重複的邏輯壓縮
 * =====================================================
 * unique 將每段相鄰等價元素保留第一個，回 new_end；不改容器 size，尾端 unspecified。
 * 完整刪除使用 erase(new_end,end)。若要刪除所有重複值，資料通常要先 sort；但 sort
 * 會改順序。時間 O(N)，保留每段代表的順序。
 *
 * predicate 表示兩元素是否等價，應是 equivalence relation。若 predicate 不可傳遞，
 * 結果難以推理。unique_copy 保留來源、寫到目的 iterator。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// LeetCode 26：Remove Duplicates from Sorted Array。
int leetcode_remove_duplicates(std::vector<int>& nums) {
    const auto new_end = std::unique(nums.begin(), nums.end());
    const int size = static_cast<int>(std::distance(nums.begin(), new_end));
    nums.erase(new_end, nums.end());
    return size;
}

// 實務：壓縮連續重複狀態，保留真正的狀態轉換序列。
std::vector<std::string> practical_compress_states(
    const std::vector<std::string>& states) {
    std::vector<std::string> result;
    std::unique_copy(states.begin(), states.end(), std::back_inserter(result));
    return result;
}

int main() {
    std::vector<int> adjacent{1, 1, 2, 2, 1};
    const auto new_end = std::unique(adjacent.begin(), adjacent.end());
    adjacent.erase(new_end, adjacent.end());
    assert((adjacent == std::vector<int>{1, 2, 1}));  // 最後 1 非相鄰，會保留。

    std::vector<int> nums{0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    assert(leetcode_remove_duplicates(nums) == 5);
    assert((nums == std::vector<int>{0, 1, 2, 3, 4}));

    const auto compressed = practical_compress_states(
        {"idle", "idle", "running", "running", "idle"});
    assert((compressed == std::vector<std::string>{"idle", "running", "idle"}));
    std::cout << "unique：LeetCode 26 與實務狀態壓縮測試通過\n";
}

/*
 * 易錯陷阱：unique 不是 set 去重；未排序的 {1,2,1} 不會變 {1,2}。只呼叫 unique
 * 不 erase，size 仍不變。尾端元素可銷毀/指定，但不可依賴其數值。
 *
 * 面試：sort+unique+erase 的複雜度 O(N log N)，會失去原順序；若要保序全域去重，
 * 可用 unordered_set 記 seen 並 copy_if，平均 O(N) 但多 O(N) 空間。
 * 實務狀態壓縮故意只消除相鄰 heartbeat，稍後再次 idle 是重要轉換不能刪。
 * 練習：以自訂 predicate 忽略大小寫壓縮相鄰狀態，先定義 Unicode 策略。
 *
 * 【LeetCode 不變量】
 * sorted input 使相同值必相鄰，所以 unique 就等同全域去重；若題目取消排序條件，
 * 同一解法立刻失效。回傳 k 後只保證前 k 格，本文 erase 是一般容器完整版本。
 *
 * 【實務資料語意】
 * telemetry 的連續重複狀態可壓縮，但若每筆 timestamp/計數不同，直接只比較 state
 * 會丟資訊；可改成 run-length encoding，保存 state、first/last time、count。
 *
 * 面試延伸：unique predicate 通常比較相鄰兩值；若它不是對稱/傳遞的等價關係，
 * 代表選擇可能依走訪順序。測試需含 A,A,B,A，確認最後 A 應保留。
 */
