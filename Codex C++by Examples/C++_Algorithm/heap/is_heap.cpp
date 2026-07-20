/*
 * std::is_heap / std::is_heap_until：驗證 heap 不變量
 * =================================================
 * is_heap 回 bool；is_heap_until 回第一個破壞 heap 性質的 iterator，若完全合法
 * 則回 last。它們唯讀、O(N)，適合 assert、資料匯入驗證與除錯，不宜每次正式
 * push/pop 後都在熱路徑掃全陣列。
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

// LeetCode 2558：Take Gifts From the Richest Pile。
long long leetcode_pick_gifts(std::vector<int> gifts, int k) {
    std::make_heap(gifts.begin(), gifts.end());
    for (int round = 0; round < k; ++round) {
        std::pop_heap(gifts.begin(), gifts.end());
        const int largest = gifts.back();
        gifts.back() = static_cast<int>(std::sqrt(static_cast<double>(largest)));
        std::push_heap(gifts.begin(), gifts.end());
        assert(std::is_heap(gifts.begin(), gifts.end()));
    }
    long long total = 0;
    for (int value : gifts) {
        total += value;
    }
    return total;
}

// 實務：接收序列化 priority queue 後，在使用前驗證格式。
std::size_t practical_first_invalid_heap_index(const std::vector<int>& data) {
    const auto it = std::is_heap_until(data.begin(), data.end());
    return static_cast<std::size_t>(std::distance(data.begin(), it));
}

int main() {
    const std::vector<int> valid{10, 7, 9, 1, 3};
    const std::vector<int> invalid{10, 7, 12, 1, 3};
    assert(std::is_heap(valid.begin(), valid.end()));
    assert(!std::is_heap(invalid.begin(), invalid.end()));
    assert(practical_first_invalid_heap_index(valid) == valid.size());
    assert(practical_first_invalid_heap_index(invalid) == 2U);

    assert(leetcode_pick_gifts({25, 64, 9, 4, 100}, 4) == 29LL);
    std::cout << "is_heap：不變量診斷與 LC2558 測試通過\n";
}

/*
 * 陷阱：is_heap(data, greater<>{}) 驗的是 min-heap；忘了 comparator 會把合法資料
 * 誤報為壞。is_heap_until 指出的節點是第一個違規子節點，可用 parent=(i-1)/2
 * 找其父節點。練習：輸出違規 child/parent 的 index 與數值。
 *
 * 【LeetCode 數值細節】
 * sqrt 接受 double，再向下轉 int；題目 gifts 為非負，因此 truncation 等同 floor。
 * 一般程式若可有負數，sqrt 產生 NaN，轉整數不是可接受的資料契約。
 *
 * 【實務診斷】
 * index==size 表示完整合法；不要把它當成可解參考位置。正式匯入外部 heap 時，
 * 驗證失敗可選擇拒絕，或呼叫 make_heap 修復並記錄告警，取決於資料可信度。
 *
 * 面試追問：is_heap_until 為何比單純 bool 更利於除錯？它給出第一個違規 child，
 * 可回推 parent 並印出 comparator 兩側值。這仍不能證明資料來源可信，只能證明
 * 當下排列符合 heap 不變量。
 *
 * LeetCode 正式提交不應保留每輪 O(N) assert；本例用它教學驗證，release 熱路徑
 * 應移除或只在抽樣診斷時開啟。實務監控可另外計數修復次數。
 * 練習：回傳包含 child index、parent index 與兩值的診斷 struct。
 */
