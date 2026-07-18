// ============================================================
// std::next_permutation
// 分類 (Category): Permutation operations (排列)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/next_permutation
//   * https://cplusplus.com/reference/algorithm/next_permutation/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::next_permutation 解的問題:
//
//   「給我一個排列,把它變成『字典序的下一個』排列。」
//
// 也就是「把陣列當數字看,加 1 取得下一個更大的字典序」。
//
// 範例 (字典序由小到大):
//
//   {1, 2, 3}  →  {1, 3, 2}  →  {2, 1, 3}  →  {2, 3, 1}  →  {3, 1, 2}  →  {3, 2, 1}
//
// 已是最大 (遞減) → 重排回最小 (遞增) 並回傳 false。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼這個函式很特別?                                 │
// └────────────────────────────────────────────────────────────┘
//
// 大多數 STL 函式只做一件事;next_permutation 是「狀態機」 —
// 它把容器當作「目前狀態」,呼叫一次就推進一格。
//
// 配合 do-while 即可枚舉所有排列:
//
//   std::sort(v.begin(), v.end());           // 從最小排列開始
//   do {
//       process(v);                            // 處理目前排列
//   } while (std::next_permutation(v.begin(), v.end()));
//
// 這個 idiom 比手寫遞迴回溯短得多 — 沒有遞迴呼叫、沒有回溯堆疊。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、演算法原理 (字典序下一個排列)                          │
// └────────────────────────────────────────────────────────────┘
//
// 標準演算法:
//   1. 從右往左找「第一個 a[i] < a[i+1]」 — 稱為 pivot。
//      (若找不到,代表已是遞減序列,直接 reverse 回到最小,return false。)
//   2. 從右往左找「第一個 a[j] > a[i]」 — 該位置的元素「剛好大於 pivot」。
//   3. swap(a[i], a[j])。
//   4. reverse a[i+1 .. end] (這段原本遞減,反轉後變遞增)。
//
// 結果就是字典序的下一個排列。整體 O(N)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、處理重複元素 (相異排列)                                 │
// └────────────────────────────────────────────────────────────┘
//
// 若容器含重複元素,next_permutation 只會枚舉「相異」排列 —
// 例如 {1, 1, 2} 只有 3 種相異排列 (1,1,2 / 1,2,1 / 2,1,1),不是 3! = 6。
// 這正是因為演算法依「字典序」推進,自動跳過重複。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class BidirIt>
//   bool next_permutation(BidirIt first, BidirIt last);
//
//   template <class BidirIt, class Compare>
//   bool next_permutation(BidirIt first, BidirIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與注意事項                                       │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 平均 O(N),最壞 O(N) 次比較與 swap
//   空間: O(1) (原地)
//
//   1. 從「未排序」開始只能枚舉「目前位置之後」的排列。
//      要全部 N! → 先 sort 再開始。
//   2. 含重複元素 → 只枚舉相異排列 (N!/(重複數!))。
//   3. comp 必須符合嚴格弱序,且與 sort 用的相同。
//
// ============================================================

/*
補充筆記：std::next_permutation
  - std::next_permutation 處理排列順序或字典序比較；它關心元素相對排列，不一定關心容器型別。
  - next_permutation/prev_permutation 會直接修改範圍，並回傳是否成功產生下一個排列。
  - 若一開始不是排序後的最小排列，next_permutation 只會從目前排列往後走，不會列出全部排列。
  - lexicographical_compare 像字典查單字一樣逐元素比較，常用於自訂排序或比較序列。
  - is_permutation 判斷兩段資料是否由相同元素組成，重複值也會納入計算。
  - 排列數量成長極快，n! 很快不可接受；教材範例小，工作上要先估算資料量。
  - std::next_permutation 處理的是序列排列，不是集合；相同元素出現多次時，排列與比較都會把重複值算進去。
  - std::next_permutation 常和字典序有關，字典序比較會從第一個不同元素決定大小，前面元素相同才比較長度。
  - std::next_permutation 可能直接重排原範圍；呼叫後若還需要原順序，應先保留副本或改用不修改輸入的判斷函式。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 枚舉所有排列 ---
    std::vector<int> v{1, 2, 3};
    std::cout << "all permutations of {1,2,3}:\n";
    do {
        std::cout << "  ";
        for (int x : v) std::cout << x << ' ';
        std::cout << '\n';
    } while (std::next_permutation(v.begin(), v.end()));

    // --- 範例 2: 含重複元素 — 只枚舉「相異」排列 ---
    std::vector<int> w{1, 1, 2};
    std::cout << "permutations of {1,1,2}:\n";
    do {
        std::cout << "  ";
        for (int x : w) std::cout << x << ' ';
        std::cout << '\n';
    } while (std::next_permutation(w.begin(), w.end()));

    // --- 範例 3: 已是最大 → 回 false 並重排為最小 ---
    std::vector<int> u{3, 2, 1};
    bool more = std::next_permutation(u.begin(), u.end());
    std::cout << "after np on {3,2,1}: more=" << std::boolalpha << more << ", v=";
    for (int x : u) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_31_next_permutation();
    void leetcode_46_permutations();
    void practical_id_combinations();
    void leetcode_60_kth_permutation();
    void practical_enumerate_seat_assignments();
    std::cout << "\n--- LeetCode / Practical Examples ---\n";
    leetcode_31_next_permutation();
    leetcode_46_permutations();
    practical_id_combinations();
    leetcode_60_kth_permutation();
    practical_enumerate_seat_assignments();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 31: 下一個排列 (Next Permutation)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,將其重排為「下一個字典序更大的排列」(原地修改)。
//      若已是最大排列,則重排回最小。
//
// 為什麼用 std::next_permutation:
//   題目就是這個函式的定義 — 一行解決,不必自己手刻演算法。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_31_next_permutation() {
    std::vector<int> nums{1, 2, 3};
    std::next_permutation(nums.begin(), nums.end());
    std::cout << "LC31 ({1,2,3} -> next): ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';

    std::vector<int> nums2{3, 2, 1};
    bool more = std::next_permutation(nums2.begin(), nums2.end());
    std::cout << "LC31 ({3,2,1} -> next): more=" << std::boolalpha << more << ", v=";
    for (int x : nums2) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 46: 全排列 (Permutations)
// ----------------------------------------------------------------
// 題目:給「不重複」的整數陣列 nums,回傳所有可能的全排列。
//
// 為什麼用 std::next_permutation:
//   sort 後 do-while 一段就完成,不必寫遞迴回溯。N! 個排列依字典序排出。
//
// 複雜度:時間 O(n! × n);空間 O(n) (不計輸出)。
void leetcode_46_permutations() {
    std::vector<int> nums{1, 2, 3};
    std::sort(nums.begin(), nums.end());
    std::vector<std::vector<int>> ans;
    do {
        ans.push_back(nums);
    } while (std::next_permutation(nums.begin(), nums.end()));
    std::cout << "LC46 ({1,2,3}): total=" << ans.size() << '\n';
    for (const auto& p : ans) {
        std::cout << "  ";
        for (int x : p) std::cout << x << ' ';
        std::cout << '\n';
    }
}

// ----------------------------------------------------------------
// 實務範例:依字典序枚舉 ID 組合
// ----------------------------------------------------------------
// 場景:系統需要產生由 {A, B, C} 組成的所有不重複序列代碼,並按字典序輸出
//      (例如測試用例 ID、樂透球號排列等)。
//
// 為什麼用 std::next_permutation:
//   排序後反覆呼叫即可依字典序輸出全部排列,程式碼極短。
void practical_id_combinations() {
    std::string s = "ABC";
    std::sort(s.begin(), s.end());
    std::cout << "Practical (ID combos of \"ABC\"):\n";
    do {
        std::cout << "  " << s << '\n';
    } while (std::next_permutation(s.begin(), s.end()));
}

// ----------------------------------------------------------------
// LeetCode 60: 第 k 個排列 (Permutation Sequence)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給整數 n 與 k,回傳 [1..n] 的第 k 個字典序排列 (1-based)。
//
// 為什麼用 std::next_permutation:
//   最直觀寫法:從 [1..n] 開始,呼叫 k-1 次 next_permutation。
//   不是最佳 (O(k × n)),但夠簡單,適合教學。
//      最佳解 O(n²) 用「Cantor's encoding」。
//
// 複雜度:時間 O(k × n);空間 O(n)。
void leetcode_60_kth_permutation() {
    int n = 4, k = 9;
    std::string s;
    for (int i = 1; i <= n; ++i) s.push_back('0' + i);
    for (int i = 1; i < k; ++i) std::next_permutation(s.begin(), s.end());
    std::cout << "LC60 n=4 k=9: " << s << '\n';
}

// ----------------------------------------------------------------
// 實務範例:枚舉「N 個學生 vs N 個座位」的所有指派
// ----------------------------------------------------------------
// 場景:小規模的安排所有可能性 (例如班級座位、考場分組),
//      用 next_permutation 一行枚舉所有 N! 種。
void practical_enumerate_seat_assignments() {
    std::vector<int> seats{1, 2, 3};
    int count = 0;
    do {
        ++count;
    } while (std::next_permutation(seats.begin(), seats.end()));
    std::cout << "total assignments: " << count << '\n';
}

// === 預期輸出 (Expected output) ===
// all permutations of {1,2,3}:
//   1 2 3
//   1 3 2
//   2 1 3
//   2 3 1
//   3 1 2
//   3 2 1
// permutations of {1,1,2}:
//   1 1 2
//   1 2 1
//   2 1 1
// after np on {3,2,1}: more=false, v=1 2 3
//
// --- LeetCode / Practical Examples ---
// LC31 ({1,2,3} -> next): 1 3 2
// LC31 ({3,2,1} -> next): more=false, v=1 2 3
// LC46 ({1,2,3}): total=6
//   1 2 3
//   1 3 2
//   2 1 3
//   2 3 1
//   3 1 2
//   3 2 1
// Practical (ID combos of "ABC"):
//   ABC
//   ACB
//   BAC
//   BCA
//   CAB
//   CBA
// LC60 n=4 k=9: 2314
// total assignments: 6
