/*
 * std::iota：以遞增運算產生連續值
 * =================================
 * 把 value 寫入每個位置，再執行 ++value。時間 O(N)，不額外配置空間。
 * 輸出範圍必須可寫；value 只需能指派給元素並支援前置 ++，不限定整數。
 *
 * 常見用途不是「填 1..N」而已，而是產生索引後排序索引，保留原資料不動。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// LeetCode 1920：Build Array from Permutation；iota 先建立可追蹤的索引集合。
std::vector<int> leetcode_build_array(const std::vector<int>& nums) {
    std::vector<std::size_t> indices(nums.size());
    std::iota(indices.begin(), indices.end(), std::size_t{0});
    std::vector<int> answer(nums.size());
    std::transform(indices.begin(), indices.end(), answer.begin(),
                   [&nums](std::size_t index) {
                       return nums[static_cast<std::size_t>(nums[index])];
                   });
    return answer;
}

struct Job {
    std::string name;
    int priority;
};

// 實務：排序 index 而非搬動大型 Job；同優先度以原始順序穩定決勝。
std::vector<std::size_t> practical_priority_order(const std::vector<Job>& jobs) {
    std::vector<std::size_t> order(jobs.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::stable_sort(order.begin(), order.end(), [&jobs](std::size_t lhs,
                                                        std::size_t rhs) {
        return jobs[lhs].priority > jobs[rhs].priority;
    });
    return order;
}

int main() {
    std::vector<int> sequence(5);
    std::iota(sequence.begin(), sequence.end(), -2);
    assert((sequence == std::vector<int>{-2, -1, 0, 1, 2}));

    assert((leetcode_build_array({0, 2, 1, 5, 3, 4}) ==
            std::vector<int>{0, 1, 2, 4, 5, 3}));

    const std::vector<Job> jobs{{"backup", 2}, {"alert", 5}, {"report", 2}};
    assert((practical_priority_order(jobs) ==
            std::vector<std::size_t>{1U, 0U, 2U}));
    assert(jobs[0].name == "backup");  // 原資料沒有被排序搬動。

    std::cout << "iota：連續值、索引排序與排列建構測試通過\n";
}

/*
 * 易錯與邊界：
 * - vector 必須先有 size；`reserve(10)` 只有容量，begin()==end()，iota 不會寫入。
 * - signed/unsigned 起點混用可能繞回；索引建議使用 std::size_t{0}。
 * - 產生超出元素型別範圍的序列會截斷或溢位，不能把 iota 當範圍檢查器。
 * - C++23 `views::iota` 是 lazy range，不需先配置；std::iota 是立即寫入既有範圍。
 *
 * 面試：何時排序索引？當 payload 大、不可搬、需保留原順序或需要多種 view 時。
 * 代價是之後每次存取多一次間接尋址，cache locality 可能較差，必須依 workload 衡量。
 *
 * 實務延伸：資料庫結果頁可用 index permutation 建立不同排序視圖；ML batching
 * 可先 iota 再 shuffle。練習：以索引完成 top-k，且不複製原始 Job。
 */
