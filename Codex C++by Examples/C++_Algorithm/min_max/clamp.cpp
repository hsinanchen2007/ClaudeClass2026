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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 661. Image Smoother（圖片平滑器）
// 題目：原題輸入 m*n 灰階影像並輸出每格及其鄰居平均值；本 helper 僅示範延伸影像
// 管線例如將中間像素 [-10,0,128,300] 裁成合法 8-bit 的 [0,0,128,255]，不是完整題解。
// 為何使用本章主題：std::clamp 適合在亮度或濾鏡計算後統一限制 [0,255]；原題官方
// 輸入與平均本來就在此範圍，因此這是誠實標示的教學後處理改寫。
// 思路：1. 預留與輸入同大小的輸出；2. 逐像素 clamp 到閉區間 [0,255]；3. 保留順序回傳。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為傳入像素數。
// 易錯點：clamp 不會防止前置亮度運算先溢位；正式 LC661 還必須計算二維鄰域平均。
// -----------------------------------------------------------------------------
std::vector<int> leetcode_clamp_pixels(const std::vector<int>& pixels) {
    std::vector<int> result;
    result.reserve(pixels.size());
    for (int value : pixels) {
        result.push_back(std::clamp(value, 0, 255));
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】API 分頁大小正規化
// 情境：使用者可傳 requested page size，但服務每頁只允許 1 到 100 筆；0 要調成 1，
// 500 要調成 100。
// 為何使用本章主題：std::clamp 以固定兩側界線直接表達寬容式正規化，比巢狀 if 或
// min/max 組合更清楚；若契約要求拒絕錯值，則不該使用靜默裁切。
// 設計：1. 將 requested 與閉區間 [1,100] 比較；2. 低於下限取 1；3. 高於上限取 100。
// 成本：時間 O(1)、額外空間 O(1)，至多兩次整數比較。
// 上線注意：需記錄被裁切的異常請求；直接以值接收結果，避免保存 clamp 對 temporary 邊界的 reference。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'clamp.cpp' -o '/tmp/codex_cpp_C_Algorithm_min_max_clamp' && '/tmp/codex_cpp_C_Algorithm_min_max_clamp'
//
// === 預期輸出（節錄）===
// clamp：像素與 API page size 邊界測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
