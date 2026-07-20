/*
 * std::max_element：回傳第一個最大元素 iterator
 * ==============================================
 * 空範圍回 end，非空做 N-1 次比較；相同最大值回第一個。需要 value 用 *it，
 * 需要 index 用 distance(begin,it)。對 vector 為 O(1) distance，list 為 O(N)。
 * 回傳 iterator 不擁有容器，erase/reallocate 後不可繼續使用。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// LeetCode 747：Largest Number At Least Twice of Others。
int leetcode_dominant_index(const std::vector<int>& nums) {
    assert(!nums.empty());
    const auto max_it = std::max_element(nums.begin(), nums.end());
    const int maximum = *max_it;
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        if (it != max_it && maximum < 2 * *it) {
            return -1;
        }
    }
    return static_cast<int>(std::distance(nums.begin(), max_it));
}

struct Endpoint {
    int id;
    double success_rate;
};

// 實務：選成功率最高 endpoint；空表用 -1 明確表示沒有候選。
int practical_best_endpoint(const std::vector<Endpoint>& endpoints) {
    const auto it = std::max_element(
        endpoints.begin(), endpoints.end(),
        [](const Endpoint& lhs, const Endpoint& rhs) {
            return lhs.success_rate < rhs.success_rate;
        });
    return it == endpoints.end() ? -1 : it->id;
}

int main() {
    const std::vector<int> values{4, 9, 2, 9};
    assert(std::max_element(values.begin(), values.end()) == values.begin() + 1);
    assert(leetcode_dominant_index({3, 6, 1, 0}) == 1);
    assert(leetcode_dominant_index({1, 2, 3, 4}) == -1);

    assert(practical_best_endpoint({{10, 0.95}, {20, 0.99}, {30, 0.97}}) == 20);
    assert(practical_best_endpoint({}) == -1);
    std::cout << "max_element：位置、LC747、endpoint 選擇測試通過\n";
}

/*
 * 易錯點：2 * value 可能整數溢位；正式處理任意 int 時可轉 long long 後比較。
 * comparator 應表達「lhs 是否排在 rhs 前」，不是直接回差值轉 bool。
 * 練習：同成功率時選 id 較小者，並寫 tie 測試。
 *
 * 【LeetCode 邊界】
 * 題目通常限制非負數；若允許負數，「最大值至少是其他值兩倍」的語意需要重新
 * 檢查。乘 2 應轉 long long 防溢位，本短例依題目小範圍。
 *
 * 【實務健康檢查】
 * success_rate 相同時目前選第一筆；若輸入順序不穩定，結果會在重啟後改變。
 * 正式 routing 可加入 latency、id 等第二鍵，並排除熔斷或樣本數不足 endpoint。
 *
 * 面試追問：若只需 top-k endpoint，反覆 max_element 為 O(kN)，應考慮
 * partial_sort、nth_element 或 heap。LeetCode 小資料可線性掃，實務要依查詢頻率選型。
 * 易錯：空 endpoints 時絕不能解參考；本例以 -1 表示缺席，型別更完整可用 optional。
 */
