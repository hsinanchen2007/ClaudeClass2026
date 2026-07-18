#include <iostream>
#include <vector>
#include <deque>
using namespace std;

vector<int> maxSlidingWindow(const vector<int>& nums, int k) {
    deque<int> dq;       // 存放索引，維持對應值的單調遞減
    vector<int> result;

    for (int i = 0; i < (int)nums.size(); i++) {
        // ① 移除已經滑出視窗的頭端元素
        if (!dq.empty() && dq.front() <= i - k) {
            dq.pop_front();
        }

        // ② 從尾端移除所有比 nums[i] 小的元素
        //    因為它們不可能再成為任何未來視窗的最大值
        while (!dq.empty() && nums[dq.back()] <= nums[i]) {
            dq.pop_back();
        }

        // ③ 把當前索引加入尾端
        dq.push_back(i);

        // ④ 當視窗已經形成（i >= k-1），記錄最大值
        if (i >= k - 1) {
            result.push_back(nums[dq.front()]);
        }
    }
    return result;
}

int main() {
    vector<int> nums = {1, 3, -1, -3, 5, 3, 6, 7};
    int k = 3;

    vector<int> res = maxSlidingWindow(nums, k);
    for (int val : res) cout << val << " ";
    cout << endl;
    // 輸出：3 3 5 5 6 7

    return 0;
}
