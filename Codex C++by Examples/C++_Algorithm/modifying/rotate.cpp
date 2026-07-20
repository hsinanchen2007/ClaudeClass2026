/*
 * std::rotate：把 [first,middle,last) 旋轉成 [middle,last)+[first,middle)
 * ==================================================================
 * rotate 不等於 reverse：它保留兩個子區間內部順序，只交換哪一段在前。
 * 回傳 iterator 指向「原本 first 元素」旋轉後的位置。時間 O(N)，交換次數 O(N)，
 * 額外空間由實作決定但標準演算法可原地完成。至少需要 forward iterator。
 *
 * middle 可等於 first 或 last，此時不改內容。演算法不改容器 size；元素位置改變，
 * 原 iterator 仍指位置而非跟著原元素。自訂型別需可 swap/move。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 189：Rotate Array，向右旋轉 k。
void leetcode_rotate_right(std::vector<int>& nums, std::size_t k) {
    if (nums.empty()) {
        return;
    }
    k %= nums.size();
    const auto middle = nums.end() - static_cast<std::ptrdiff_t>(k);
    std::rotate(nums.begin(), middle, nums.end());
}

// 實務：round-robin 排程每輪把已服務的第一位移到尾端。
void practical_advance_round_robin(std::vector<std::string>& workers) {
    if (workers.size() > 1U) {
        std::rotate(workers.begin(), workers.begin() + 1, workers.end());
    }
}

int main() {
    std::vector<int> values{1, 2, 3, 4, 5};
    const auto old_first = std::rotate(values.begin(), values.begin() + 2,
                                       values.end());
    assert((values == std::vector<int>{3, 4, 5, 1, 2}));
    assert(old_first == values.begin() + 3 && *old_first == 1);

    std::vector<int> nums{1, 2, 3, 4, 5, 6, 7};
    leetcode_rotate_right(nums, 3U);
    assert((nums == std::vector<int>{5, 6, 7, 1, 2, 3, 4}));
    leetcode_rotate_right(nums, 7U);
    assert((nums == std::vector<int>{5, 6, 7, 1, 2, 3, 4}));

    std::vector<std::string> workers{"A", "B", "C"};
    practical_advance_round_robin(workers);
    assert((workers == std::vector<std::string>{"B", "C", "A"}));
    std::cout << "rotate：區段旋轉、LeetCode 189、round-robin 測試通過\n";
}

/*
 * 【LeetCode 複雜度】時間 O(N)、額外空間 O(1) 的語意；空 vector 必須先處理，
 * 否則 k % nums.size() 會除以 0。k 大於 N 要先取餘數。
 *
 * 【實務陷阱】round-robin 只改本機順序，不處理 worker offline、權重或併發更新；
 * 真正 scheduler 需要鎖或 immutable snapshot。rotate 期間若其他 thread 同時讀寫
 * 同一 vector 是 data race。
 *
 * 面試：三次 reverse 也可解 LC189；rotate 表意更直接。若要產生新輸出而保留
 * 原範圍，使用 rotate_copy。練習：左旋 k，並驗證回傳 iterator 的含義。
 *
 * 測試補充：單元素與 k=0 應完全不變；k=N、2N 也不變。LeetCode 若 k 是 signed
 * 且可能負數，要先定義負值代表左旋或拒絕，不可直接轉 size_t。
 * 實務 weighted round-robin 不能只 rotate，還需按權重重複/計算配額。
 * 練習後以 std::is_permutation 驗證旋轉沒有遺失或重複元素。
 */
