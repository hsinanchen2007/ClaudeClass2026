// ============================================================
// std::swap_ranges
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/swap_ranges
//   * https://cplusplus.com/reference/algorithm/swap_ranges/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::swap_ranges 解的問題:
//
//   「兩段資料,逐元素一對一交換。」
//
// 等同於:
//
//   for i = 0 .. N-1:
//       std::iter_swap(first1 + i, first2 + i);
//
// 適用情境:
//   * 矩陣的「列交換」(對 vector<vector<int>> 的兩個 row 段交換)
//   * 兩個 buffer 的某個區段對換 (signal 處理)
//   * 任何「同長度區段對調」的需求
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、跟「整個容器交換」的差別                               │
// └────────────────────────────────────────────────────────────┘
//
//   * 若是「兩個容器整體互換」 → 直接用 a.swap(b) 是 O(1) (容器特化)
//   * 若是「兩段區間互換」     → 用 swap_ranges 是 O(N)
//
// 整體交換通常更快 — 但只有「想交換的就是整個容器」才適用。
// 區段交換是更彈性的工具。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt1, class FwdIt2>
//   FwdIt2 swap_ranges(FwdIt1 first1, FwdIt1 last1, FwdIt2 first2);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   指向第二範圍中「最後被交換元素的下一個」(first2 + (last1 - first1))。
//   類似於 std::copy 的回傳值,可以用來串接後續操作。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次 swap — O(N)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 兩段「不可重疊」 — 重疊時行為未定義 (UB)。
//   2. 第二範圍長度必須 >= 第一範圍 — 標準不檢查,自己要確保。
//   3. 兩個迭代器型別可以不同 (跨容器交換 OK)。
//   4. 整體容器交換改用容器自己的 .swap() 才是 O(1)。
//
// ============================================================

/*
補充筆記：std::swap_ranges
  - swap_ranges 逐一交換兩段範圍的元素。
  - 第二段必須至少和第一段一樣長，演算法不會替你檢查容器邊界。
  - 兩段重疊時要非常小心，因為逐步交換可能不是你想像的整段交換。
  - std::swap_ranges 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> b{10, 20, 30, 40, 50};

    // --- 範例 1: 全範圍交換 (兩個 vector 整段對換) ---
    std::swap_ranges(a.begin(), a.end(), b.begin());
    std::cout << "a: "; for (int x : a) std::cout << x << ' '; std::cout << '\n';
    std::cout << "b: "; for (int x : b) std::cout << x << ' '; std::cout << '\n';

    // --- 範例 2: 部分範圍交換 — a 的前 3 個 vs b 的尾 3 個 ---
    std::vector<int> v{1, 2, 3, 4, 5};
    std::vector<int> w{6, 7, 8, 9, 10};
    std::swap_ranges(v.begin(), v.begin() + 3, w.begin() + 2);
    std::cout << "v: "; for (int x : v) std::cout << x << ' '; std::cout << '\n';
    std::cout << "w: "; for (int x : w) std::cout << x << ' '; std::cout << '\n';

    // --- 範例 3: 兩個 vector 整體交換 (改用 .swap() 是 O(1)) ---
    std::vector<int> p{1, 2, 3};
    std::vector<int> q{9, 8};
    p.swap(q);
    std::cout << "p.swap(q): p has " << p.size()
              << " items, q has " << q.size() << '\n';

    // === 實務範例 ===
    void practical_swap_matrix_rows();
    void practical_swap_segments();
    void leetcode_swap_first_last_half();
    void practical_left_right_camera_swap();
    practical_swap_matrix_rows();
    practical_swap_segments();
    leetcode_swap_first_last_half();
    practical_left_right_camera_swap();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// swap_ranges 在 LeetCode 上題目稀少,主要是實務工具。

// ----------------------------------------------------------------
// 實務範例 1:矩陣的「列交換」
// ----------------------------------------------------------------
// 場景:做高斯消去 (Gaussian elimination) 或矩陣列簡化時,
//      常需要交換兩個「列段」(row range)。
//
// 為什麼用 std::swap_ranges:
//   * 一行交換兩列的所有元素,意圖明確。
//   * 對 vector<vector<int>> 也可以直接 std::swap(m[i], m[j]) (O(1)),
//     但若只是「列內某段」(部分欄位) 對換,swap_ranges 才是適合工具。
void practical_swap_matrix_rows() {
    std::vector<std::vector<int>> m{
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9,10,11,12},
    };
    std::swap_ranges(m[0].begin(), m[0].end(), m[2].begin());
    std::cout << "Swap rows 0 and 2: ";
    for (auto& row : m) {
        std::cout << '[';
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << row[i] << (i + 1 < row.size() ? "," : "");
        }
        std::cout << "] ";
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例 2:同一個 vector 內前後段對調
// ----------------------------------------------------------------
// 場景:處理環狀緩衝/段對換的情境 — 把前 k 筆與後 k 筆對換。
//
// 為什麼用 std::swap_ranges:
//   兩段在同一個 vector 內,但只要「不重疊」就 OK。
//   一行完成,不必自己手寫雙端 swap 迴圈。
void practical_swap_segments() {
    std::vector<int> data{1, 2, 3, 0, 0, 7, 8, 9};
    std::swap_ranges(data.begin(), data.begin() + 3, data.end() - 3);
    std::cout << "Segments swapped: ";
    for (int x : data) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 概念題:把陣列前半與後半交換 (旋轉的特例)
// ----------------------------------------------------------------
// 題目:給陣列 (長度 2n),把前 n 個與後 n 個互換 — 等價於 std::rotate(begin, begin+n, end)。
//      但若想「保留兩半各自的相對順序、僅互換位置」,std::swap_ranges 更直接。
//
// 為什麼用 std::swap_ranges:
//   一行表達「兩個不重疊的相同長度區間互換」 — 比手寫 for + temp 變數簡潔。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_swap_first_last_half() {
    std::vector<int> nums{1, 2, 3, 7, 8, 9};
    int n = nums.size() / 2;
    std::swap_ranges(nums.begin(), nums.begin() + n, nums.begin() + n);
    std::cout << "swap halves:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:雙鏡頭直播 — 把左右鏡頭的「最近 N 秒影格」互換
// ----------------------------------------------------------------
// 場景:某直播間支援左右鏡頭分屏,使用者按下「切換鏡頭」時,
//      要把最後 N 秒緩衝影格的左右兩個 ring buffer 互換。
//      用 swap_ranges 一次互換整段,程式碼簡潔。
void practical_left_right_camera_swap() {
    std::vector<int> left{101, 102, 103, 104, 105};   // 影格 ID
    std::vector<int> right{201, 202, 203, 204, 205};
    std::swap_ranges(left.end() - 3, left.end(), right.end() - 3);
    std::cout << "left:";
    for (int x : left) std::cout << ' ' << x;
    std::cout << "\nright:";
    for (int x : right) std::cout << ' ' << x;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// a: 10 20 30 40 50
// b: 1 2 3 4 5
// v: 8 9 10 4 5
// w: 6 7 1 2 3
// p.swap(q): p has 2 items, q has 3 items
// Swap rows 0 and 2: [9,10,11,12] [5,6,7,8] [1,2,3,4]
// Segments swapped: 7 8 9 0 0 1 2 3
// swap halves: 7 8 9 1 2 3
// left: 101 102 203 204 205
// right: 201 202 103 104 105
