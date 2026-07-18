// ============================================================
// std::iota    (C++11 起)
// 分類 (Category): Numeric operations (數值演算法)
// 標頭檔 (Header):  <numeric>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/iota
//   * https://cplusplus.com/reference/numeric/iota/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::iota 解的問題:
//
//   「把這段範圍填入連續遞增的值: value, value+1, value+2, ...」
//
// 名稱來自 APL 語言的「ι (iota)」 — 該語言用這個小寫希臘字母表示「索引」。
//
// 等價的手寫迴圈:
//
//   T x = value;
//   for (auto it = first; it != last; ++it) {
//       *it = x;
//       ++x;
//   }
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、最常見用途 — 「argsort 模式 (依索引排序)」              │
// └────────────────────────────────────────────────────────────┘
//
// 「argsort」是統計/資料處理常見的概念 — 不直接排序資料,而是排序「索引」,
// 之後用索引去取值。這在「需要保留原始位置」的情境很有用。
//
//   std::vector<int> idx(n);
//   std::iota(idx.begin(), idx.end(), 0);     // idx = 0, 1, 2, ..., n-1
//   std::sort(idx.begin(), idx.end(),
//             [&](int a, int b){ return scores[a] < scores[b]; });
//   // 現在 idx 就是「依分數排序後的原始索引」
//
// LC 「依分數排名」「依時間戳排序事件 (保留原 ID)」等都是這個模式。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、其他典型應用                                           │
// └────────────────────────────────────────────────────────────┘
//
//   * 產生連號 ID:1001, 1002, 1003, ...
//   * 產生字母序列:'A', 'B', 'C', ...
//   * 對抗測試的「期望值序列」:[1..n] 與輸入比對
//   * DP 表格的初始索引
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   void iota(FwdIt first, FwdIt last, T value);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、需求與複雜度                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次寫入與 N 次 ++value
//   空間: O(1)
//   需求: T 必須支援 ++ 運算元,且能指派給元素型別。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 容器要先 resize 到所需大小 — iota 不會自動擴充。
//   2. 浮點 ++ 會有精度問題 — 對 double 不太常用 iota。
//   3. 使用前確認「該型別 ++ 的語意」 — 自訂類別要重載 operator++。
//
// ============================================================

/*
補充筆記：std::iota
  - iota 從初始值開始遞增填滿範圍，常用來建立索引陣列。
  - 它使用 ++value，因此型別只要支援遞增即可，不限整數。
  - 容器大小要先準備好；iota 不會 push_back。
  - std::iota 屬於數值演算法，常用來把迴圈中的累加、差分、內積或前綴和寫成標準形式。
  - accumulate 的初始值型別會決定累加結果型別；用 0 累加 double 或 long long 時可能不小心截斷或溢位。
  - reduce 可重新排序運算順序以利平行化，因此 binary operation 必須接近結合律且沒有副作用；浮點數結果可能和 accumulate 不完全相同。
  - partial_sum 和 adjacent_difference 互為常見搭配，可用來建立前綴和與差分陣列。
  - gcd/lcm 對整數工作；lcm 可能溢位，即使最後型別是 long long 也要注意輸入大小。
  - 數值演算法最容易出錯的是型別和溢位，不是語法；工作程式應刻意選擇初始值型別。
*/
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 0..9 ---
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 0);
    std::cout << "iota 0..9: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 從 100 開始 ---
    std::vector<int> w(5);
    std::iota(w.begin(), w.end(), 100);
    std::cout << "from 100: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 字元序列 ---
    std::string s(7, ' ');
    std::iota(s.begin(), s.end(), 'A');
    std::cout << "letters: " << s << '\n';

    // --- 範例 4: argsort 模式 — 對 score 排序得到原始索引 ---
    std::vector<int> score{30, 10, 20, 50, 40};
    std::vector<int> idx(score.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&score](int a, int b){ return score[a] < score[b]; });
    std::cout << "indices sorted by score:";
    for (int i : idx)
        std::cout << " (" << i << ":" << score[i] << ")";
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_287_find_duplicate_iota_helper();
    void leetcode_argsort_pattern();
    void practical_generate_user_ids();
    void leetcode_2overlap_missing_via_iota();
    void practical_seat_numbers();
    leetcode_287_find_duplicate_iota_helper();
    leetcode_argsort_pattern();
    practical_generate_user_ids();
    leetcode_2overlap_missing_via_iota();
    practical_seat_numbers();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 287 概念:用 iota 產生「期望基準」
// ----------------------------------------------------------------
// 題目:nums 長度 n+1,值域 [1, n],恰有一個重複。找出重複數。
//
// 為什麼用 std::iota (作為輔助):
//   產生「期望的值域 [1..n]」用以對照輸入 — iota 一行解決。
void leetcode_287_find_duplicate_iota_helper() {
    std::vector<int> nums{1, 3, 4, 2, 2};
    int n = static_cast<int>(nums.size()) - 1;
    std::vector<int> expect(n);
    std::iota(expect.begin(), expect.end(), 1);
    std::sort(nums.begin(), nums.end());
    int dup = -1;
    for (std::size_t i = 1; i < nums.size(); ++i)
        if (nums[i] == nums[i - 1]) { dup = nums[i]; break; }
    std::cout << "LC287 expect=[";
    for (std::size_t i = 0; i < expect.size(); ++i)
        std::cout << expect[i] << (i + 1 == expect.size() ? "" : ",");
    std::cout << "] duplicate=" << dup << '\n';
}

// ----------------------------------------------------------------
// LeetCode argsort 模式 (LC 1331 等):依某 key 排序「索引」
// ----------------------------------------------------------------
// 題目:給 nums,得到「依值排序後的原始索引」。
//
// 為什麼用 std::iota:
//   * 先 iota 生成索引 0..n-1。
//   * 對「索引」排序,比較器用 nums[a] < nums[b]。
//   * 結果是 argsort — 不破壞原 nums 的順序。
void leetcode_argsort_pattern() {
    std::vector<int> nums{40, 10, 20, 30};
    std::vector<int> idx(nums.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(),
              [&nums](int a, int b){ return nums[a] < nums[b]; });
    std::cout << "argsort: ";
    for (int i : idx) std::cout << i << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:批量產生連續使用者 ID
// ----------------------------------------------------------------
// 場景:系統初始化要建立 N 個 user 並指派 1001..1000+N 的 ID。
//
// 為什麼用 std::iota:
//   一行就能產生連續整數序列,語意明確。
void practical_generate_user_ids() {
    const int N = 5;
    std::vector<int> user_ids(N);
    std::iota(user_ids.begin(), user_ids.end(), 1001);
    std::cout << "user_ids: ";
    for (int id : user_ids) std::cout << id << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 268: 找缺失的數字 (Missing Number) — 用 iota 產生期望集合
// ----------------------------------------------------------------
// 題目:給 [0, n] 中漏掉一個數的陣列,找出缺失的那個。
//
// 為什麼用 std::iota:
//   * 用 iota 生 [0..n] 的期望總和;
//   * missing = sum_expected - sum_actual。
//
// 複雜度:時間 O(n);空間 O(n) (期望陣列)。
void leetcode_2overlap_missing_via_iota() {
    std::vector<int> nums{3, 0, 1};
    int n = nums.size();
    std::vector<int> expect(n + 1);
    std::iota(expect.begin(), expect.end(), 0);
    int sum_e = std::accumulate(expect.begin(), expect.end(), 0);
    int sum_a = std::accumulate(nums.begin(), nums.end(), 0);
    std::cout << "LC268 missing: " << (sum_e - sum_a) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:座位編號生成 (依排數產生)
// ----------------------------------------------------------------
// 場景:電影院某排座位編號從 N1 到 Nk,要產生 vector<int>{N1, N1+1, ..., Nk}。
//      iota 一行完成。
void practical_seat_numbers() {
    int row_start = 21;
    int seats = 8;
    std::vector<int> nums(seats);
    std::iota(nums.begin(), nums.end(), row_start);
    std::cout << "seats:";
    for (int n : nums) std::cout << ' ' << n;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// iota 0..9: 0 1 2 3 4 5 6 7 8 9
// from 100: 100 101 102 103 104
// letters: ABCDEFG
// indices sorted by score: (1:10) (2:20) (0:30) (4:40) (3:50)
// LC287 expect=[1,2,3,4] duplicate=2
// argsort: 1 2 3 0
// user_ids: 1001 1002 1003 1004 1005
// LC268 missing: 2
// seats: 21 22 23 24 25 26 27 28
