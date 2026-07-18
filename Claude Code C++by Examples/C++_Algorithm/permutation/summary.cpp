/*
================================================================================
【C++_Algorithm/permutation/summary.cpp】
本章末總整理 — 排列 (Permutation) 家族
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Permutation_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、什麼是「排列 (Permutation)」?
--------------------------------------------------------------------------------
  「排列」= 一個多重集 (multiset) 的元素重新排序得到的另一個序列。
  例如 {1,2,3} 有 3! = 6 種排列。

  ★ 排列是有「順序」的概念,因此可以排成「字典序 (lexicographic order)」:
       123, 132, 213, 231, 312, 321

--------------------------------------------------------------------------------
二、本章涵蓋的 API
--------------------------------------------------------------------------------
  std::next_permutation(first, last [, comp])        → bool
    → 把 range 改成「字典序下一個排列」。
    → 若已是最大排列 (例如 321),回 false 並把 range 重排回最小 (123)。

  std::prev_permutation(first, last [, comp])        → bool
    → 把 range 改成「字典序上一個排列」。
    → 若已是最小排列,回 false 並重排回最大。

  std::is_permutation(first1, last1, first2 [, last2] [, pred])  → bool
    → 判斷兩個 range 是否「互為排列」(多重集相等)。

  std::lexicographical_compare(first1, last1, first2, last2 [, comp])  → bool
    → 比較兩個 range 的字典序 (a < b)。

--------------------------------------------------------------------------------
三、複雜度
--------------------------------------------------------------------------------
  API                            時間              空間   迭代器
  ─────────────────────────────  ──────────────    ────   ─────────────
  next_permutation                O(N) 最壞         O(1)   Bidirectional
  prev_permutation                O(N) 最壞         O(1)   Bidirectional
  is_permutation                  O(N²) 最壞        O(1)   Forward
                                  O(N) (若 sorted)
  lexicographical_compare         O(min(N1, N2))    O(1)   Input

  ★ N! 個排列總走訪總時間是 O(N · N!) — 因此只能對小 N 使用。

--------------------------------------------------------------------------------
四、next_permutation 演算法解析
--------------------------------------------------------------------------------
  cppreference 給的演算法:
    1. 從右往左找第一個 a[i] < a[i+1] 的位置 i;若不存在 → 已是最大排列。
    2. 從右往左找第一個 a[j] > a[i] 的位置 j。
    3. swap(a[i], a[j])。
    4. reverse(a[i+1 .. end])。

  這個演算法保證每次取得「字典序最近的下一個」排列。

  完整列舉模式:
      std::sort(v.begin(), v.end());     // 先從最小排列開始
      do {
          process(v);
      } while (std::next_permutation(v.begin(), v.end()));

--------------------------------------------------------------------------------
五、is_permutation 的關鍵細節
--------------------------------------------------------------------------------
  - 比較的是「多重集相等」,順序無關。
  - 預設用 operator==。可帶自訂 BinaryPredicate (例如不分大小寫)。
  - 4-iterator 版本 (C++14+) 先檢查長度,長度不同直接 false。
  - 3-iterator 版本不檢查 last2,要求第二條長度至少 == 第一條,否則 UB。

  常見替代:對小資料先 sort 再用 equal:
      auto a = v1; std::sort(a.begin(), a.end());
      auto b = v2; std::sort(b.begin(), b.end());
      bool ok = (a == b);
  這個寫法 O(N log N),但常數小,且容易讀。

--------------------------------------------------------------------------------
六、lexicographical_compare 重點
--------------------------------------------------------------------------------
  - 等價於「逐元素比較 + 短的優先」邏輯:
       "ab" < "abc"  → true   (短的為前綴)
       "abc" < "abd" → true   (第一個不同處 c < d)
       "abcd" < "abc" → false (反過來)

  - 是 std::sort、std::set、std::map 預設用來比較容器型元素的方式。

  - C++20 起,容器類別 (vector / string 等) 內建 operator<=>;新程式碼
    建議直接用 a < b,內部會走 spaceship 規則。

--------------------------------------------------------------------------------
七、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. next_permutation 起始狀態必須是「最小排列」才能列舉全部:
     - 若資料未排序,只會列出「從目前位置到最大排列」的後續排列。

  2. 含重複元素時 next_permutation 仍能正確處理:
     - {1,1,2} → {1,2,1} → {2,1,1} → false
     - 但「不同排列數」變少 (= N!/重複數的階乘乘積)。

  3. 元素數 N = 20 已是 2.4×10¹⁸,根本枚舉不完;只有小 N 才用 next_permutation。

  4. is_permutation 對大資料 O(N²) 太慢:
     - 預先 sort 後比 equal,或用 hash map 計次,通常更實用。

  5. 自訂 comp 必須與資料的等值關係一致:
     - 若 comp 把 'A' 與 'a' 視為等價,is_permutation 也要用同一 pred。

  6. lexicographical_compare 不是「< 比較容器」的唯一方式:
     - C++20 操作符 <=> 對容器是預設 lexicographical,但若元素類型沒
       三向比較運算子可能編譯失敗。

--------------------------------------------------------------------------------
八、選擇準則
--------------------------------------------------------------------------------
  需求                                          選擇
  ─────────────────────────────────────────  ─────────────────────────
  小 N 列舉所有排列                              sort + do { } while next_permutation
  反向列舉                                      sort(>) + do { } while prev_permutation
  判斷兩 range 是否互為排列                      is_permutation (或 sort + equal)
  字典序比較字串/容器                            lexicographical_compare 或 < (C++20)

--------------------------------------------------------------------------------
九、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 小規模窮舉測試案例 (例如 6 個 ID 的所有排列)。
  - 字典序版本比較 (例如 SemVer 字串拆解後比較)。
  - 判斷兩個多重集相等 (例如 anagram 檢查)。
  - 字典序生成測試輸入,確認 deterministic 結果。
================================================================================
*/
#include <algorithm>
#include <iostream>
#include <string>

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_next_prev_permutation() {
    std::cout << "\n[demo_next_prev_permutation]\n";

    // 起點:已排序的最小排列
    std::string s = "123";
    std::sort(s.begin(), s.end());

    std::cout << "  all permutations of \"123\":\n";
    do {
        std::cout << "    " << s << "\n";
    } while (std::next_permutation(s.begin(), s.end()));
    // 結束後 s 變回最小排列 "123"

    // prev_permutation:反向走幾步
    std::string t = "321";
    std::cout << "  prev_permutation steps from \"321\":\n";
    for (int i = 0; i < 3; ++i) {
        std::cout << "    " << t << "\n";
        std::prev_permutation(t.begin(), t.end());
    }
}

static void demo_is_permutation() {
    std::cout << "\n[demo_is_permutation]\n";
    std::string a = "aabc";
    std::string b = "abca";  // 是 a 的排列
    std::string c = "abcc";  // 不是 a 的排列 (字元計數不同)

    std::cout << "  a vs b: "
              << std::is_permutation(a.begin(), a.end(), b.begin(), b.end()) << "\n";
    std::cout << "  a vs c: "
              << std::is_permutation(a.begin(), a.end(), c.begin(), c.end()) << "\n";
}

static void demo_lex_compare() {
    std::cout << "\n[demo_lex_compare]\n";
    std::string x = "abc";
    std::string y = "abd";
    std::cout << "  \"abc\" < \"abd\" ? "
              << std::lexicographical_compare(x.begin(), x.end(),
                                              y.begin(), y.end()) << "\n";
}

int main() {
    demo_next_prev_permutation();
    demo_is_permutation();
    demo_lex_compare();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
