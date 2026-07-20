// ============================================================
// std::replace / std::replace_if / std::replace_copy / std::replace_copy_if
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/replace
//   * https://cplusplus.com/reference/algorithm/replace/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::replace 解的問題:
//
//   「在這段資料裡,把所有符合條件的元素換成新值。」
//
// 注意「換」這個字 —
//   * 數量「不變」、位置「不變」,只是值被改寫。
//   * 想真的「移除」(改變數量) 請用 remove / erase-remove idiom。
//
// 四個成員的差異是「條件如何描述」與「結果寫到哪裡」:
//
//   ┌──────────────────────┬───────────────────────────────────┐
//   │ replace              │ 原地,條件 = (== old_value)        │
//   │ replace_if           │ 原地,條件 = 述詞 p                │
//   │ replace_copy         │ 拷貝到輸出,條件 = (== old_value)  │
//   │ replace_copy_if      │ 拷貝到輸出,條件 = 述詞 p          │
//   └──────────────────────┴───────────────────────────────────┘
//
// _copy 版本不修改原範圍,適合「想保留原資料 + 同時要替換版」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、replace vs transform vs remove 的選擇                  │
// └────────────────────────────────────────────────────────────┘
//
//   * replace      把「特定元素」換成「特定新值」 (條件式覆蓋)
//   * transform    對「每個元素」套一個函式產生新值 (一對一映射)
//   * remove       「過濾掉」元素 (改變邏輯長度)
//
// 三者各有對應的場景:
//   要替換 → replace
//   要逐元素轉換 → transform
//   要刪除 → remove + erase
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class T>
//   void replace(FwdIt first, FwdIt last,
//                const T& old_value, const T& new_value);
//
//   template <class FwdIt, class UnaryPred, class T>
//   void replace_if(FwdIt first, FwdIt last,
//                   UnaryPred p, const T& new_value);
//
//   template <class InputIt, class OutputIt, class T>
//   OutputIt replace_copy(InputIt first, InputIt last, OutputIt d_first,
//                         const T& old_value, const T& new_value);
//
//   template <class InputIt, class OutputIt, class UnaryPred, class T>
//   OutputIt replace_copy_if(InputIt first, InputIt last, OutputIt d_first,
//                            UnaryPred p, const T& new_value);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   replace / replace_if            : void
//   replace_copy / replace_copy_if  : 寫入結束位置的下一個 (d_first + N)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 N 次「比較或述詞呼叫」
//   空間: O(1) (in-place 版);_copy 版另需 O(N) 輸出。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. replace 不會改變元素「個數」,只改值。
//   2. _copy 版的目的端必須有足夠空間,或用 back_inserter。
//   3. 對自訂型別:replace 用 operator== 比對;沒有就改用 replace_if。
//   4. 字串 (std::string) 也是字元 sequence,可直接用 std::replace 替換字元。
//
// ============================================================

/*
補充筆記：std::replace
  - replace 會把範圍內符合舊值的元素改成新值，容器大小不變。
  - replace_if 用 predicate 決定哪些元素要改，適合條件比對。
  - 若需要保留原資料並輸出新資料，應看 replace_copy。
  - std::replace 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::replace / replace_if / replace_copy / replace_copy_if
// ---------------------------------------------------------------------------
// 🔥 Q1. replace 會改變容器大小嗎?它和 remove、transform 怎麼分工?
//     答:不會。replace 只改「值」,個數與位置都不變,複雜度恰好 N 次比較。
//         分工:要把特定值換成另一個特定值用 replace;要對每個元素套函式產生新值用
//         transform;要過濾掉元素(改變邏輯長度)用 remove + erase。
//
// 🔥 Q2. replace 和 replace_if 各用在什麼時候?
//     答:replace(first, last, old, new) 以 operator== 比對舊值,適合「值相等」的簡單情形;
//         replace_if(first, last, pred, new) 以述詞決定,適合條件式(範圍、屬性、
//         或自訂型別沒有 operator== 的情況)。自訂型別若沒定義 operator==,
//         replace 根本編不過,這時就得改用 replace_if。
//
// Q3. 為什麼有 _copy 版本?它們的回傳值是什麼?
//     答:_copy 版不修改原範圍,而是把結果寫到另一段輸出,適合「原資料要保留、
//         同時要一份替換後的版本」。回傳值是寫入結束位置的下一個(d_first + N)。
//         注意目的端必須先有足夠空間,或改用 std::back_inserter——
//         和其他寫入型演算法一樣,它不會替你擴容。
//
// Q4. std::string 可以直接用 std::replace 嗎?
//     答:可以,但要分清楚層次:std::replace 把 string 當成「字元的 sequence」,
//         做的是「單一字元換單一字元」。要換「子字串」得用成員函式 std::string::replace,
//         兩者名字相同但完全是不同的東西。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 2, 5, 2};

    // --- 範例 1: replace (把所有 2 換成 99) ---
    std::replace(v.begin(), v.end(), 2, 99);
    std::cout << "replace 2->99: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: replace_if (把負數改為 0) ---
    std::vector<int> w{1, -2, 3, -4, 5};
    std::replace_if(w.begin(), w.end(),
                    [](int x){ return x < 0; }, 0);
    std::cout << "replace_if(<0)->0: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: replace_copy (不變動原始資料) ---
    std::vector<int> a{1, 2, 3, 2, 4};
    std::vector<int> b(a.size());
    std::replace_copy(a.begin(), a.end(), b.begin(), 2, 0);
    std::cout << "src unchanged: ";
    for (int x : a) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "replace_copy:  ";
    for (int x : b) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 4: replace_copy_if + back_inserter ---
    std::vector<int> c{1, 2, 3, 4, 5};
    std::vector<int> d;
    std::replace_copy_if(c.begin(), c.end(), std::back_inserter(d),
                         [](int x){ return x % 2 == 0; }, -1);
    std::cout << "replace_copy_if(even)->-1: ";
    for (int x : d) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1351_replace_negatives();
    void practical_replace_sensitive();
    void leetcode_2455_average_even_replace();
    void practical_normalize_missing_data();
    leetcode_1351_replace_negatives();
    practical_replace_sensitive();
    leetcode_2455_average_even_replace();
    practical_normalize_missing_data();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 子題:把矩陣中所有「負數」替換為 0
// ----------------------------------------------------------------
// 題目:給矩陣 grid,將所有負值替換為 0 (做為 LC 1351「計負數」的衍生)。
//
// 為什麼用 std::replace_if:
//   條件 (x < 0) + 換成新值 (0) — 完美對應 replace_if。
//
// 解法步驟:
//   對每一列 row 呼叫 std::replace_if(row, x<0, 0)。
//
// 複雜度:時間 O(rows × cols);空間 O(1)。
void leetcode_1351_replace_negatives() {
    std::vector<std::vector<int>> grid{
        { 4, 3, 2,-1},
        { 3, 2, 1,-1},
        { 1, 1,-1,-2},
        {-1,-1,-2,-3},
    };
    for (auto& row : grid) {
        std::replace_if(row.begin(), row.end(),
                        [](int x){ return x < 0; }, 0);
    }
    std::cout << "LC1351-variant: ";
    for (auto& row : grid) {
        std::cout << '[';
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << row[i] << (i + 1 < row.size() ? "," : "");
        }
        std::cout << "] ";
    }
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:字串敏感字元統一替換
// ----------------------------------------------------------------
// 場景:留言系統把所有星號 '*' 統一換成井號 '#' (例如統一遮罩字元樣式)。
//
// 為什麼用 std::replace:
//   std::string 也是字元的 sequence,直接 replace 即可,
//   一行寫完且效率與手寫迴圈相同。
void practical_replace_sensitive() {
    std::string msg = "He said: ****, then **** again!";
    std::replace(msg.begin(), msg.end(), '*', '#');
    std::cout << "Sanitized: " << msg << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2455 概念:把所有偶數元素替換為其除以 2 (一輪)
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums,把所有偶數 x 替換為 x/2 (示範條件替換),
//          再回傳平均值。
//
// 為什麼用 std::replace_if 並不直接夠用:
//   replace_if 要求「替換為一個固定值」,本題替換值依 x 變化 —
//   所以實際應該用 std::transform。這裡示範:當「目標值與 x 有關時」,
//   replace_if 不適用,要改用 transform 或 for 迴圈。
//
// 教學重點:辨識何時不該用 replace_if。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2455_average_even_replace() {
    std::vector<int> nums{2, 7, 4, 9, 6};
    // replace_if 不適用 (新值依 x 變化) → 改用 for + 條件賦值
    for (int& x : nums) if (x % 2 == 0) x /= 2;
    int sum = 0;
    for (int x : nums) sum += x;
    std::cout << "LC2455 nums:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << " avg=" << (double(sum) / nums.size()) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把資料集中的「缺失值 sentinel」標準化為 0
// ----------------------------------------------------------------
// 場景:從外部來源讀進的數值常用 -1 或 -999 表示「缺失」,
//      做統計前要統一把缺失值替換為 0 (或其他預設值)。
//      std::replace 一行寫完。
void practical_normalize_missing_data() {
    std::vector<int> data{12, -999, 5, 8, -999, 14, 7};
    std::replace(data.begin(), data.end(), -999, 0);
    std::cout << "normalized:";
    for (int x : data) std::cout << ' ' << x;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra replace.cpp -o replace

// === 預期輸出 ===
// replace 2->99: 1 99 3 99 5 99
// replace_if(<0)->0: 1 0 3 0 5
// src unchanged: 1 2 3 2 4
// replace_copy:  1 0 3 0 4
// replace_copy_if(even)->-1: 1 -1 3 -1 5
// LC1351-variant: [4,3,2,0] [3,2,1,0] [1,1,0,0] [0,0,0,0]
// Sanitized: He said: ####, then #### again!
// LC2455 nums: 1 7 2 9 3 avg=4.4
// normalized: 12 0 5 8 0 14 7
