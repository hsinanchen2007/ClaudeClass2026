// ============================================================
// std::fill / std::fill_n
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/fill
//   * https://en.cppreference.com/w/cpp/algorithm/fill_n
//   * https://cplusplus.com/reference/algorithm/fill/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::fill 系列就是「把一段區間裡每個位置都寫入同一個值」 —
// 這是最常被用來「清空」「歸零」「初始化」的 STL 演算法。
//
// 兩個成員的差別只在於「範圍如何描述」:
//
//   ┌──────────────┬─────────────────────────────────────────┐
//   │ fill(f, l, v) │ 範圍 [first, last) 全部填 v              │
//   │ fill_n(f, n, v)│ 從 first 開始的 n 個位置全部填 v        │
//   └──────────────┴─────────────────────────────────────────┘
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、跟「建構填值」的差別                                   │
// └────────────────────────────────────────────────────────────┘
//
// 容易搞混的兩件事:
//
//   std::vector<int> v(5, 0);   // 「建構」5 個 0,容器才剛產生
//   std::fill(v.begin(), v.end(), 0);  // 「重設」已存在的 5 個位置都為 0
//
// 第一行是建構期間就把 5 個位置都填 0;第二行是「容器已存在,把每格內容覆蓋」。
// 兩者效果在「值」上等價,但第一行包含了配置記憶體 + 初始化,第二行只是寫入。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼 fill 通常很快?                                 │
// └────────────────────────────────────────────────────────────┘
//
// 對 trivially copyable 的型別 (int、float、struct of POD ...),
// 編譯器/標準函式庫常會把 std::fill 最佳化為 `memset` 或 SIMD 指令,
// 速度遠快於手寫 for 迴圈。對效能敏感的初始化情境很有用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   void fill(FwdIt first, FwdIt last, const T& value);
//
//   template <class OutputIt, class Size, class T>
//   OutputIt fill_n(OutputIt first, Size n, const T& value);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   fill   : void
//   fill_n : 指向「最後寫入位置的下一個」(即 first + n)
//            n <= 0 時回傳 first (什麼都不寫)。
//
// fill_n 配合 back_inserter 可以「動態追加 n 個值」 — 常見搭配。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次指派 — O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 容器必須先有空間 — fill 不會幫你 resize。
//   2. fill_n 對 n <= 0 不寫入,直接回傳 first。
//   3. value 的型別必須能轉型為元素型別 — 否則會編譯錯誤。
//   4. 對自訂型別:fill 是逐元素 operator= 指派,不是 memcpy。
//
// ============================================================

/*
補充筆記：std::fill
  - fill 把指定值寫入整段輸出範圍，常用來重設 vector 或 array 內容。
  - 它不會改變容器大小；範圍長度由 iterator 決定。
  - 對大型物件 fill 會重複賦值，成本不是常數。
  - std::fill 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 將既有容器全部設為 0 ---
    std::vector<int> v{1, 2, 3, 4, 5};
    std::fill(v.begin(), v.end(), 0);
    std::cout << "fill all zero: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 將前半段設為 -1 (子範圍) ---
    std::vector<int> w(8, 0);
    std::fill(w.begin(), w.begin() + 4, -1);
    std::cout << "first 4 set to -1: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: fill_n 寫入 5 個 7 到既有容器 ---
    std::vector<int> u(5);
    auto it = std::fill_n(u.begin(), 5, 7);
    std::cout << "fill_n 5 sevens (it == end? "
              << std::boolalpha << (it == u.end()) << "): ";
    for (int x : u) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: fill_n 配合 back_inserter 動態擴充 ---
    std::vector<int> grow;
    std::fill_n(std::back_inserter(grow), 4, 42);
    std::cout << "back_inserter fill: ";
    for (int x : grow) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1295_dp_init();
    void practical_dp_matrix_init();
    void leetcode_303_range_sum_prefix_reset();
    void practical_clear_pixel_buffer();
    leetcode_1295_dp_init();
    practical_dp_matrix_init();
    leetcode_303_range_sum_prefix_reset();
    practical_clear_pixel_buffer();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:DP 表格初始化 (常見於 LC 322 零錢兌換、LC 198 打家劫舍)
// ----------------------------------------------------------------
// 題目簡化:很多動態規劃題會需要把 dp 陣列「初始化為某個 sentinel 值」
//          代表「尚未計算」或「不可達」。例如 LC 322 用 amount+1 當 sentinel。
//
// 為什麼用 std::fill:
//   這正是「重設整段為某個值」 — 一行 fill 即可,意圖比手寫 for 清晰。
void leetcode_1295_dp_init() {
    int amount = 11;
    std::vector<int> dp(amount + 1);
    std::fill(dp.begin(), dp.end(), amount + 1);   // sentinel = "不可達"
    dp[0] = 0;                                     // base case
    std::cout << "LC322-init dp: ";
    for (int x : dp) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:二維表格初始化
// ----------------------------------------------------------------
// 場景:DP / Floyd-Warshall / 棋盤類問題常需要建立 n × m 表格並
//      初始化為某預設值 (例如 -1 代表「未訪問」、INT_MAX 代表「無路徑」)。
void practical_dp_matrix_init() {
    const int rows = 3, cols = 4;
    std::vector<std::vector<int>> dp(rows, std::vector<int>(cols));
    for (auto& row : dp) {
        std::fill(row.begin(), row.end(), -1);   // -1 代表尚未計算
    }
    std::cout << "DP init: ";
    for (auto& row : dp) {
        for (int x : row) std::cout << x << ' ';
        std::cout << "| ";
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 303: 區域與檢索 - 陣列不可變 (Range Sum Query - 前綴和重置)
// ----------------------------------------------------------------
// 題目:給陣列 nums,設計 sumRange(i, j) 回傳 nums[i..j] 區間和。
//      解法:預先計算 prefix[i] = nums[0..i-1] 累加。
//      這裡用 fill 把 prefix 預先清零再填寫,展示「清零再計算」模式。
//
// 為什麼用 std::fill:
//   初始化 prefix 陣列為 0 — 一行表達,意圖明顯。
//
// 複雜度:預處理 O(n);查詢 O(1)。
void leetcode_303_range_sum_prefix_reset() {
    std::vector<int> nums{-2, 0, 3, -5, 2, -1};
    std::vector<int> prefix(nums.size() + 1);
    std::fill(prefix.begin(), prefix.end(), 0);
    for (size_t i = 0; i < nums.size(); ++i)
        prefix[i+1] = prefix[i] + nums[i];
    int i = 0, j = 2;   // sumRange(0,2)
    std::cout << "LC303 sumRange(0,2)=" << (prefix[j+1] - prefix[i]) << '\n';
    i = 2; j = 5;
    std::cout << "LC303 sumRange(2,5)=" << (prefix[j+1] - prefix[i]) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:清空 pixel buffer (圖形系統)
// ----------------------------------------------------------------
// 場景:每幀繪製前,要先把 frame buffer 全部清為背景色 (例如黑色 = 0)。
//      fill 是最直接的寫法,某些編譯器/標準函式庫會 vectorize 成 memset。
void practical_clear_pixel_buffer() {
    std::vector<unsigned char> buf(16);   // 16 pixel buffer (簡化)
    // 初始有資料
    for (size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<unsigned char>(i * 10);
    // 每幀開始:清空成黑色
    std::fill(buf.begin(), buf.end(), 0);
    int sum = 0;
    for (unsigned char c : buf) sum += c;
    std::cout << "buffer cleared, sum=" << sum << '\n';
}

// === 預期輸出 (Expected output) ===
// fill all zero: 0 0 0 0 0
// first 4 set to -1: -1 -1 -1 -1 0 0 0 0
// fill_n 5 sevens (it == end? true): 7 7 7 7 7
// back_inserter fill: 42 42 42 42
// LC322-init dp: 0 12 12 12 12 12 12 12 12 12 12 12
// DP init: -1 -1 -1 -1 | -1 -1 -1 -1 | -1 -1 -1 -1 |
// LC303 sumRange(0,2)=1
// LC303 sumRange(2,5)=-1
// buffer cleared, sum=0
