// ============================================================================
// multiset：排序且允許重複值的 bag
// ============================================================================
// multiset 保留每次插入，即使值相同。insert/find/bounds 為 O(log n)，count 為
// O(log n + 相等元素數)。erase(pos) 是 amortized O(1)，erase(value) 為 O(log n + count)，
// range erase 為 O(log n + N)。erase(value) 會刪除所有相等值；只刪一份要先 find 再
// erase(iterator)。它適合需要排序、重複計數且會動態增刪的資料。

#include <cassert>
#include <iostream>
#include <iterator>
#include <set>
#include <vector>

void basic_demo()
{
    std::multiset<int> readings{5, 2, 5, 9};
    assert(readings.count(5) == 2U);
    assert(*readings.begin() == 2);

    const auto one_five = readings.find(5);
    assert(one_five != readings.end());
    readings.erase(one_five);  // 只刪一份。
    assert(readings.count(5) == 1U);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 350. Intersection of Two Arrays II（兩個陣列的交集 II）
// 題目：回傳兩陣列含重複次數的交集；例如 [4,9,5,4] 與 [9,4,9,8,4] 得 [4,9,4]（順序不限）。
// 為何使用本章主題：multiset 同時保存值與重複份數，find 後 erase(iterator) 恰好消耗一份匹配。
// 思路：以 right 建立可用 bag；掃描 left；找到即輸出並刪除該 iterator；缺少則跳過。
// 複雜度：時間 O((N+M) log M)、額外空間 O(M)，N/M 分別為左右陣列長度。
// 易錯點：erase(value) 會刪光全部同值項；題目不要求輸出順序；hash 計數方案平均可達 O(N+M)。
// -----------------------------------------------------------------------------
std::vector<int> intersect(const std::vector<int>& left,
                           const std::vector<int>& right)
{
    std::multiset<int> available(right.begin(), right.end());
    std::vector<int> result;
    for (const int value : left) {
        const auto found = available.find(value);
        if (found != available.end()) {
            result.push_back(value);
            available.erase(found);
        }
    }
    return result;
}

void leetcode_demo()
{
    auto result = intersect({4, 9, 5, 4}, {9, 4, 9, 8, 4});
    assert((result == std::vector<int>{4, 9, 4}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】延遲滑動窗口的中位數
// 情境：監控窗口保存可重複的 latency 樣本，樣本進出後需計算中位數以降低極端值影響。
// 為何使用本章主題：multiset 在動態增刪時維持排序與重複值，比每次重排整個 vector 更直觀。
// 設計：從 begin 前進到中點；奇數筆取中點；偶數筆取中點及前一筆平均。
// 成本：插入/刪除 O(log W)，本教學版查中位數 O(W)、額外空間 O(1)，W 為窗口大小。
// 上線注意：空集合不可取中位數；刪單筆必須 erase(iterator)；高吞吐場景應維護中點或改用雙 heap。
// -----------------------------------------------------------------------------
double median(const std::multiset<int>& values)
{
    assert(!values.empty());
    auto middle = values.begin();
    std::advance(middle, static_cast<std::ptrdiff_t>(values.size() / 2U));
    if (values.size() % 2U == 1U) {
        return static_cast<double>(*middle);
    }
    const int upper = *middle;
    const int lower = *std::prev(middle);
    return (static_cast<double>(lower) + static_cast<double>(upper)) / 2.0;
}

void practical_demo()
{
    std::multiset<int> latency{10, 20, 20, 100};
    assert(median(latency) == 20.0);
    latency.erase(latency.find(10));
    latency.insert(30);
    assert(median(latency) == 25.0);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "multiset：重複值、交集與中位數測試通過\n";
}

// 【陷阱】multiset iterator 的元素同樣不可原地改 key。
// 【面試】只需 frequency 時 unordered_map<T,count> 通常更直接；需要 order statistic
//         才考慮 multiset，但標準 multiset 不提供 O(log n) 第 k 個元素。
// 【練習】實作固定長度 3 的窗口，每滑一步刪一值、加一值並輸出 median。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'multiset.cpp' -o '/tmp/codex_cpp_C_Container_multiset' && '/tmp/codex_cpp_C_Container_multiset'
//
// === 預期輸出（節錄）===
// multiset：重複值、交集與中位數測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
