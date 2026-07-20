/*
 * std::clamp（C++17）：把值限制在閉區間 [low, high]
 * ==================================================
 * value<low 回 low；high<value 回 high；否則回 value。三參數版回 const T&，
 * 因此不要長期保存由 temporary low/high 產生的 reference，通常直接複製結果。
 * 前置條件 low 不可大於 high；否則行為未定義。複雜度至多兩次比較。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

// LeetCode-style：將影像像素裁切到合法 8-bit 範圍。
std::vector<int> leetcode_clamp_pixels(const std::vector<int>& pixels) {
    std::vector<int> result;
    result.reserve(pixels.size());
    for (int value : pixels) {
        result.push_back(std::clamp(value, 0, 255));
    }
    return result;
}

// 實務：API page size 可由使用者指定，但服務限制為 [1,100]。
int practical_normalize_page_size(int requested) {
    return std::clamp(requested, 1, 100);
}

int main() {
    assert(std::clamp(7, 0, 10) == 7);
    assert(std::clamp(-3, 0, 10) == 0);
    assert(std::clamp(99, 0, 10) == 10);

    assert((leetcode_clamp_pixels({-10, 0, 128, 300}) ==
            std::vector<int>{0, 0, 128, 255}));
    assert(practical_normalize_page_size(0) == 1);
    assert(practical_normalize_page_size(20) == 20);
    assert(practical_normalize_page_size(500) == 100);

    std::cout << "clamp：像素與 API page size 邊界測試通過\n";
}

/*
 * 易錯點：clamp 不是循環 wrap，也不是輸入驗證；把 500 靜默改成 100 是否符合產品
 * 契約要先決定。浮點 NaN 不會按直覺被限制。比較自訂型別時 comparator 需一致。
 * 面試：如何手寫？return v<lo?lo:(hi<v?hi:v)，注意不是用 <=。
 * 練習：對音訊 sample 做 clamp，統計有多少筆真的被 clipping。
 *
 * 【LeetCode-style 複雜度】
 * 每個 pixel 至多兩次比較，總時間 O(N)、輸出空間 O(N)。若允許原地改輸入，
 * 可用 transform(pixels.begin(),pixels.end(),pixels.begin(),...) 降低額外空間。
 *
 * 【實務 API 設計】
 * page size 超界可選 clamp 或回 400 error。clamp 對人機輸入較寬容，對程式間 API
 * 可能掩蓋 client bug。實務上常同時回正規化值與 warning/metric。
 *
 * 【生命週期陷阱】
 * `const int& r=std::clamp(x,0,100);` 的 low/high 是 temporary；若命中邊界，r 在
 * statement 後懸空。使用 `const int r=...` 複製值即可避免。
 *
 * 面試追問：clamp 與 min(max(v,lo),hi) 是否完全等價？在一般全序值上結果相同，
 * 但直接 clamp 更清楚表達前置條件，且標準只要求至多兩次比較。
 *
 * 測試至少包含 lo-1、lo、區間內、hi、hi+1。若使用 unsigned，不能直接建立
 * lo-1；測試資料型別本身也是邊界設計的一部分。
 *
 * LeetCode-style pixel 題若輸入可達 INT_MAX，clamp 本身仍不做算術所以安全；
 * 若先做亮度加法再 clamp，必須先用較寬型別避免加法已溢位。
 * 練習：實作回傳 {normalized,was_clamped} 的 page-size API。
 */
