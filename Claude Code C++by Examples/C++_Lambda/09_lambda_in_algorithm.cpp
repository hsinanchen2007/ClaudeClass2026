// =============================================================================
//  09_lambda_in_algorithm.cpp  —  Lambda 與 STL 演算法（含三個經典 LeetCode）
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/algorithm
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼 lambda + 演算法 = 工作金組合？                      │
//  └────────────────────────────────────────────────────────────┘
//
//  STL 演算法（sort、partial_sort、nth_element、find_if、count_if、
//  transform、accumulate、partition、stable_sort、unique、remove_if、
//  upper_bound、lower_bound...）幾乎都允許吃一個「比較器 / predicate /
//  函式物件」。Lambda 是給這個位置最方便的工具。
//
//  本檔示範 3 題經典 LeetCode，每題的核心都是「定義對的 lambda」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ LeetCode 56. Merge Intervals                              │
//  └────────────────────────────────────────────────────────────┘
//
//  題意：給多組 [start, end] 區間，把重疊的合併成一個。
//  解法：先用 lambda 「按起點」排序；接著線性掃過合併。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ LeetCode 215. Kth Largest Element in an Array              │
//  └────────────────────────────────────────────────────────────┘
//
//  題意：找第 k 大的元素。
//  簡單做法：用 std::nth_element（O(n) 平均）配 lambda 寫「降序」比較器。
//  std::nth_element 把第 k 大的元素「移到該在的位置」、左邊比它大、右邊比
//  它小（不必整個排序）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ LeetCode 347. Top K Frequent Elements                      │
//  └────────────────────────────────────────────────────────────┘
//
//  題意：找出現頻率最高的 k 個元素。
//  解法：unordered_map 統計頻率 → 把 (元素, 次數) 倒進 vector → 用 lambda
//  比較器配 partial_sort，只排前 k 個（更快）。
//
// =============================================================================

/*
補充筆記：lambda_in_algorithm
  - algorithm 中的 lambda 通常扮演 predicate、projection 或 transformation。
  - predicate 應回傳穩定布林值，不要在排序比較器裡修改外部狀態。
  - 捕獲大型物件時優先考慮 const reference，但要確保 algorithm 執行期間物件仍存活。
  - lambda_in_algorithm 要從 closure object 理解：lambda 不是神祕語法，而是編譯器產生的匿名函式物件。
  - 捕獲 by value 是在建立 lambda 時複製，by reference 是保存別名；延後執行時 reference 捕獲最容易 dangling。
  - mutable 只讓 by value 捕獲的副本可修改，不會修改外部原變數。
  - generic lambda 的 auto 參數本質上是 function call operator template，錯誤可能在呼叫時才出現。
  - std::function 可保存不同 callable，但可能有型別抹除成本和配置成本；效能敏感處可優先用 template 接 callable。
  - lambda 放進 algorithm 時應讓 predicate 無副作用或副作用明確，否則演算法意圖會變難讀。
*/
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // LeetCode 56. Merge Intervals
    // ─────────────────────────────────────────────────────────
    {
        std::vector<std::vector<int>> intervals{{1,3},{2,6},{8,10},{15,18}};

        // 1) 按起點升序排序 — 用 lambda
        std::sort(intervals.begin(), intervals.end(),
                  [](const auto& a, const auto& b) { return a[0] < b[0]; });

        // 2) 線性合併
        std::vector<std::vector<int>> merged;
        for (const auto& cur : intervals) {
            if (!merged.empty() && cur[0] <= merged.back()[1]) {
                merged.back()[1] = std::max(merged.back()[1], cur[1]);
            } else {
                merged.push_back(cur);
            }
        }
        std::cout << "[LC56] merged =";
        for (auto& iv : merged)
            std::cout << " [" << iv[0] << ',' << iv[1] << ']';
        std::cout << '\n';
        // 預期：[1,6] [8,10] [15,18]
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 215. Kth Largest Element in an Array
    //   nth_element 把第 (k-1) 索引位置「就位」 — 左邊不一定排序，但都是
    //   比它大的；右邊都是比它小的。用降序比較器 → 索引 (k-1) 上就是第 k 大。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> nums{3, 2, 1, 5, 6, 4};
        int k = 2; // 預期答案 5

        // 用 lambda 提供降序比較器（greater）
        std::nth_element(nums.begin(), nums.begin() + (k - 1), nums.end(),
                         [](int a, int b) { return a > b; });
        std::cout << "[LC215] " << k << "-th largest = " << nums[k - 1] << '\n';
        // 預期：5
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 347. Top K Frequent Elements
    //   1) 統計頻率
    //   2) 把 (元素,次數) 放進 vector
    //   3) 用 partial_sort 只排前 k 個 → 比 sort 快
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> nums{1, 1, 1, 2, 2, 3};
        int k = 2;

        std::unordered_map<int, int> freq;
        for (int n : nums) ++freq[n];

        std::vector<std::pair<int, int>> bag(freq.begin(), freq.end());

        // 比較器：次數高的排在前面
        auto cmp = [](const auto& a, const auto& b) {
            return a.second > b.second;
        };
        std::partial_sort(bag.begin(), bag.begin() + k, bag.end(), cmp);

        std::cout << "[LC347] top " << k << " =";
        for (int i = 0; i < k; ++i) std::cout << ' ' << bag[i].first;
        std::cout << '\n';
        // 預期：1 2（順序視 unordered_map iterator 而定，但前兩名一定是 1 與 2）
    }

    // ─────────────────────────────────────────────────────────
    // LeetCode 451. Sort Characters By Frequency
    // 難度: medium
    //   題意：依字元出現次數重排字串，次數高的排前面。
    //   解法：用 unordered_map 統計頻率，再 sort 時用 lambda 把 freq 透過
    //         capture 傳進比較器 — 經典「lambda 捕獲外部 lookup table」用法。
    // ─────────────────────────────────────────────────────────
    {
        std::string s = "tree";
        std::unordered_map<char, int> freq;
        for (char c : s) ++freq[c];

        // 比較器：以 freq 為主鍵降序，相同則 char 本身穩定（這裡簡單比 char）
        std::sort(s.begin(), s.end(), [&freq](char a, char b) {
            if (freq[a] != freq[b]) return freq[a] > freq[b];
            return a < b;  // 同次數時用字元本身定序，避免不穩定輸出
        });
        std::cout << "[LC451] sorted = \"" << s << "\"\n";
        // 預期：eert（e 出現 2 次擺前面；r、t 各 1 次）
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例：用 partition + lambda 把資料切成「合格 / 不合格」兩堆
    //   工作上常見：批次處理前先把資料分流（valid vs invalid），不必額外配置
    //   兩個 vector — std::partition 原地完成、回傳「分界 iterator」。
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> ages{15, 18, 21, 17, 30, 16, 65};
        // 「成年人」往前移；it 為「第一個未成年」位置
        auto it = std::partition(ages.begin(), ages.end(),
                                 [](int a) { return a >= 18; });

        std::cout << "[partition] adults: ";
        for (auto p = ages.begin(); p != it; ++p) std::cout << *p << ' ';
        std::cout << "| minors: ";
        for (auto p = it; p != ages.end(); ++p) std::cout << *p << ' ';
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：sort vs partial_sort vs nth_element 怎麼選？
    //    A：
    //      sort         O(n log n)：要整個排序時用
    //      partial_sort O(n log k)：要前 k 個（且要排序）時用
    //      nth_element  O(n) 平均：只想找第 k 名（不在乎順序）時用 — 最快
    //
    //  Q2：lambda 比較器一定要回傳 bool 嗎？
    //    A：要。標準要求是 strict weak ordering — 回傳 bool，且要滿足
    //       「不可同時 (a<b) 與 (b<a)」。回傳 int (像 qsort 的 cmp) 會錯。
    //
    //  Q3：為什麼用 const auto& 而不是 const std::vector<int>&？
    //    A：generic lambda 寫起來短、且 LeetCode 題目經常切換型別
    //       (vector<int> vs vector<vector<int>>)；const auto& 兩者都通。
    //
    return 0;
}
