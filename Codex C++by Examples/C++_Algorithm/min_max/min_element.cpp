/*
 * std::min_element：回傳範圍中第一個最小元素的 iterator
 * ====================================================
 * 空範圍回 last；非空範圍做 N-1 次比較，時間 O(N)、額外空間 O(1)。相同最小值
 * 回第一個。演算法不修改元素，但回傳 iterator 的有效期由容器決定。
 *
 * 解參考前永遠先檢查 it != end。自訂 comparator 比較的是 element/element。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 153. Find Minimum in Rotated Sorted Array（尋找旋轉排序陣列中的最小值）
// 題目：輸入由嚴格升冪陣列旋轉而成的 nums，回最小值；例如 [3,4,5,1,2] 回 1。
// 為何使用本章主題：本例刻意用 min_element 建立 O(N) 正確性基線，沒有利用旋轉陣列
// 的單調結構；正式最佳解應以二分搜尋達 O(log N)。
// 思路：1. 驗證非空；2. 線性掃描取得最小 iterator；3. 解參考回傳最小值。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 nums 的元素數。
// 易錯點：空範圍會回 end 而不可解參考；面試需主動說明這不是題目要求的最佳複雜度。
// -----------------------------------------------------------------------------
int leetcode_find_min_linear(const std::vector<int>& nums) {
    assert(!nums.empty());
    return *std::min_element(nums.begin(), nums.end());
}

struct Server {
    int id;
    int active_requests;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】最低負載伺服器選擇
// 情境：負載平衡器取得 server id 與 active_requests 快照，要挑目前請求數最少者；
// 同負載時沿用輸入中的第一台。
// 為何使用本章主題：min_element 一次線性掃描即可取得最低負載物件，無需完整排序，
// 並提供明確的第一個最小值 tie 規則。
// 設計：1. comparator 比較 active_requests；2. 找第一個最小 server；3. 回其 id。
// 成本：時間 O(N)、額外空間 O(1)，N 為 server 數。
// 上線注意：輸入不可空且應先過濾 unhealthy server；併發負載變動若要求一致決策需使用 snapshot。
// -----------------------------------------------------------------------------
int practical_least_loaded_server(const std::vector<Server>& servers) {
    assert(!servers.empty());
    const auto it = std::min_element(
        servers.begin(), servers.end(),
        [](const Server& lhs, const Server& rhs) {
            return lhs.active_requests < rhs.active_requests;
        });
    return it->id;
}

int main() {
    const std::vector<int> values{8, 2, 5, 2};
    const auto it = std::min_element(values.begin(), values.end());
    assert(it == values.begin() + 1);
    assert(std::min_element(values.end(), values.end()) == values.end());

    assert(leetcode_find_min_linear({3, 4, 5, 1, 2}) == 1);
    assert(practical_least_loaded_server({{1, 10}, {2, 3}, {3, 3}}) == 2);

    std::cout << "min_element：位置、LC153 基線、負載平衡測試通過\n";
}

/*
 * 面試追問：既然 rotated sorted 可 O(log N)，為何實務仍可能先寫 O(N)？它是簡單
 * 可驗證的 oracle，可用於 property test 驗證最佳化版本；但正式大資料仍應二分。
 * 練習：比較 server 時加入 id 作第二鍵，明確定義 tie-breaking。
 *
 * 【LeetCode 最佳化方向】
 * LC153 利用 rotated sorted 的單調結構可二分到 O(log N)；min_element 完全不利用
 * 排序資訊，所以是 O(N)。面試應先交付正確解，再說明如何利用題目條件最佳化。
 *
 * 【實務一致性】
 * active_requests 若會被其他 thread 同時更新，掃描期間可能看見不同時間點資料。
 * 要求嚴格決策時需 snapshot 或同步；容許近似負載平衡時可接受 eventual consistency。
 *
 * 易錯陷阱：把 comparator 寫成 `lhs.load-lhs.load` 或回 int 差值再轉 bool，不僅
 * 拼字易錯，也可能溢位；直接使用 `<`。LeetCode 二分版則要另測無 rotation。
 * 面試若問 tie，明確回答 min_element 回第一個最小值。
 * 實務回傳 server id 前，也要確認該 server 沒有被健康檢查標為 unavailable。
 * 練習：回傳 optional<int> 取代空表 assert，並加入 health 欄位過濾。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'min_element.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_min_element' && '/tmp/codex_cpp_C_Algorithm_min_max_min_element'
//
// === 預期輸出（節錄）===
// min_element：位置、LC153 基線、負載平衡測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
