// ============================================================
// std::all_of / std::any_of / std::none_of   (C++11 起)
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/all_any_none_of
//   * https://cplusplus.com/reference/algorithm/all_of/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// 程式裡常常出現這種句子:
//
//   * 「所有員工的薪資都大於 0 嗎?」
//   * 「有任何訂單金額為負嗎?」
//   * 「沒有任何使用者帳號被停用吧?」
//
// 這三類句子分別對應數理邏輯中的三個量詞 (quantifier):
//
//   ∀ x ∈ S, p(x)  → 所有 (all)
//   ∃ x ∈ S, p(x)  → 存在 (any)
//   ¬∃ x ∈ S, p(x) → 不存在 (none)
//
// C++11 把這三個量詞直接收進 STL,讓程式碼能像數學一樣讀:
//
//   std::all_of (v.begin(), v.end(), p)
//   std::any_of (v.begin(), v.end(), p)
//   std::none_of(v.begin(), v.end(), p)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼不用 for 迴圈?                                  │
// └────────────────────────────────────────────────────────────┘
//
//   * 意圖一目了然:看到 all_of 立刻知道在做「全稱判斷」,
//     不必去看裡面 break 是哪個方向、回 true 還是 false。
//   * 自動 short-circuit:any_of 找到一個就停;all_of 找到反例就停;
//     none_of 找到反例就停。效能與手寫 for 相同。
//   * 述詞可組合:p 是 lambda 或自由函式,容易測試與重用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、空範圍 (vacuous truth) — 最容易踩坑的點                 │
// └────────────────────────────────────────────────────────────┘
//
// 對「空集合」做量詞判斷時,結果如下,務必記牢:
//
//   all_of (空) → true   ← 「對空集合的所有元素都成立」恆真
//   any_of (空) → false  ← 「空集合裡沒有任何元素」
//   none_of(空) → true   ← 「空集合裡找不到反例」
//
// 這就是邏輯學的「空真 (vacuous truth)」 —
//   「凡是粉紅色的大象都會飛」這句話之所以是真的,
//   就是因為世上沒有粉紅色的大象,你舉不出反例。
//
// 寫程式時最常踩的坑是:
//   if (std::all_of(v.begin(), v.end(), is_valid)) { ... }
// 結果 v 是空,進到 if 分支才發現「沒有任何資料」 — 要先檢查 !v.empty()。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class UnaryPred>
//   bool all_of (InputIt first, InputIt last, UnaryPred p);
//   template <class InputIt, class UnaryPred>
//   bool any_of (InputIt first, InputIt last, UnaryPred p);
//   template <class InputIt, class UnaryPred>
//   bool none_of(InputIt first, InputIt last, UnaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//   * C++20 引入 std::ranges 對應版本,可帶投影 (projection)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度與短路 (Complexity & Short-circuit)              │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 O(n) — 找到結論立刻停。
//   any_of  : 找到第一個 true 即停。
//   all_of  : 找到第一個 false 即停。
//   none_of : 找到第一個 true 即停 (回傳 false)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 空範圍的真值表 (見上),別憑直覺。
//   2. 三者僅回 bool。若還想知道「是哪一個元素讓判斷失敗」,改用 find_if。
//   3. 與 count_if 的差別:count_if 一定走完整個範圍,
//      只在乎個數;all/any/none 在乎布林結論,可短路。
//   4. 述詞 p 必須是「無副作用」的純函式 (理論上;實務上盡量遵守)。
//
// ============================================================

/*
補充筆記：std::all_any_none_of
  - all_of、any_of、none_of 都會短路，predicate 不一定對每個元素執行。
  - 空範圍上 all_of 和 none_of 為 true，any_of 為 false，這是邏輯上的 vacuous truth。
  - predicate 不應依賴被呼叫次數，否則短路會造成隱藏行為。
  - std::all_any_none_of 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::all_of / std::any_of / std::none_of
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對「空區間」,all_of / any_of / none_of 各回傳什麼?
//     答:all_of → true、any_of → false、none_of → true。
//         all_of 為真的理由是 vacuous truth(空真):空集合上舉不出任何反例,
//         所以全稱命題自動成立。這題考的是邊界條件的直覺,不是背答案。
//     追問:業務邏輯上怎麼避免踩坑?(先明確檢查 !v.empty(),
//           別讓「沒有任何資料」和「全部都合格」走進同一個分支)
//
// 🔥 Q2. 三者都會 short-circuit 嗎?和 count_if 差在哪?
//     答:會。any_of 找到第一個 true 就停、all_of 找到第一個 false 就停、
//         none_of 找到第一個 true 就停(並回 false);最壞才是 O(n)。
//         count_if 則一定走完整個區間,因為它必須算出確切個數。
//         所以只想知道「有沒有」時,別寫 count_if(...) > 0 — 用 any_of。
//     追問:predicate 可以有副作用嗎?(不該有 — 因為短路,p 不保證對每個元素都被呼叫,
//           依賴呼叫次數的寫法會產生隱藏行為)
//
// Q3. 只知道 true/false 不夠,想知道「是哪個元素不合格」該用什麼?
//     答:改用 find_if / find_if_not,它回傳的是 iterator,可以直接指出違規元素。
//         all_of(first, last, p) 在語意上等價於 find_if_not(first, last, p) == last。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{2, 4, 6, 8, 10};
    auto is_even = [](int x){ return x % 2 == 0; };
    auto is_neg  = [](int x){ return x < 0; };

    std::cout << std::boolalpha;
    // --- 範例 1: 全部偶數? ---
    std::cout << "all even: "  << std::all_of (v.begin(), v.end(), is_even) << '\n';
    // --- 範例 2: 有任何負數? ---
    std::cout << "any neg : "  << std::any_of (v.begin(), v.end(), is_neg)  << '\n';
    // --- 範例 3: 沒有任何負數? ---
    std::cout << "none neg: "  << std::none_of(v.begin(), v.end(), is_neg)  << '\n';

    // --- 範例 4: 含有奇數的情況 ---
    std::vector<int> w{2, 4, 5, 8};
    std::cout << "w all even: " << std::all_of(w.begin(), w.end(), is_even) << '\n';
    std::cout << "w any odd : " << std::any_of(w.begin(), w.end(),
                                               [](int x){ return x % 2; }) << '\n';

    // --- 範例 5: 空範圍 — 真值表演示 ---
    std::vector<int> e;
    std::cout << "empty all_of  = " << std::all_of (e.begin(), e.end(), is_even) << '\n';
    std::cout << "empty any_of  = " << std::any_of (e.begin(), e.end(), is_even) << '\n';
    std::cout << "empty none_of = " << std::none_of(e.begin(), e.end(), is_even) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_2overlap_check();
    void leetcode_1742_max_balls();
    void practical_form_validation();
    void leetcode_2553_separate_digits_all_positive();
    void practical_pre_flight_checks();
    leetcode_2overlap_check();
    leetcode_1742_max_balls();
    practical_form_validation();
    leetcode_2553_separate_digits_all_positive();
    practical_pre_flight_checks();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// 選題原則:挑「布林結論型」題目,凸顯 all/any/none 的語意優勢。

// ----------------------------------------------------------------
// LeetCode 1346: 檢查整數及其兩倍是否存在
//                (Check If N and Its Double Exist)
// ----------------------------------------------------------------
// 題目:給陣列 arr,是否存在 i ≠ j 使得 arr[i] == 2 * arr[j]?
//
// 為什麼用 std::any_of:
//   題目要的就是「存在 ∃」 — 直接對應 any_of。
//   找到第一組就可以結束,不需要走完整個陣列。
//
// 解法步驟:
//   1. 對每個位置 j,目標是「另一個位置 i,使 arr[i] == 2 * arr[j]」。
//   2. 用 any_of 在整個陣列搜尋,排除 i == j 的情況。
//
// 複雜度:時間 O(n^2),空間 O(1)。
//        (進階解法可用 unordered_set 降到 O(n);這裡示範 any_of 語意。)
void leetcode_2overlap_check() {
    std::vector<int> arr{10, 2, 5, 3};
    // ⚠️ 踩雷（本檔曾經寫錯）：不可以用 (&x - &arr[0]) 反推索引。
    //    lambda 的參數是【傳值】的 int x,&x 指向的是 lambda 自己那份【堆疊上的副本】,
    //    跟 vector 的元素不在同一個 array object 裡 → 兩個不相關指標相減是 UB,
    //    算出來的「索引」也毫無意義（ASan 的 invalid-pointer-pair 就是在抓這個）。
    //    要索引就老老實實用索引迴圈。
    bool ok = false;
    for (size_t i = 0; i < arr.size() && !ok; ++i) {
        const int x = arr[i];
        // 對每個 x,在陣列中找「值 == 2*x 且不是同一個位置」
        for (size_t j = 0; j < arr.size(); ++j) {
            if (j != i && arr[j] == 2 * x) { ok = true; break; }
        }
    }
    std::cout << "LC1346: " << std::boolalpha << ok << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1742: 盒子中小球的最大數量 (簡化:檢查所有元素皆 > 0)
// ----------------------------------------------------------------
// 題目簡化:給一組計數,確認「所有計數都嚴格大於 0」。
//
// 為什麼用 std::all_of:
//   「所有 ∀」直接對應 all_of。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_1742_max_balls() {
    std::vector<int> counts{3, 1, 4, 1, 5, 9, 2, 6};
    bool all_pos = std::all_of(counts.begin(), counts.end(),
                               [](int c){ return c > 0; });
    std::cout << "LC1742: all positive = " << std::boolalpha << all_pos << '\n';
}

// ----------------------------------------------------------------
// 實務範例:表單欄位驗證
// ----------------------------------------------------------------
// 場景:後端收到註冊表單,要同時驗證:
//   1. 所有「必填」欄位都有值        ← all_of
//   2. 沒有任何欄位含禁用字 <script> ← none_of
//
// 為什麼用 all_of + none_of:
//   把「驗證規則」表達成兩個量詞,程式碼極接近自然語言,維護容易。
struct Field {
    std::string name;
    std::string value;
    bool required;
};
void practical_form_validation() {
    std::vector<Field> form{
        {"username", "alice",       true},
        {"email",    "a@b.c",       true},
        {"comment",  "hello world", false}
    };
    bool required_filled = std::all_of(form.begin(), form.end(),
        [](const Field& f){ return !f.required || !f.value.empty(); });
    bool safe = std::none_of(form.begin(), form.end(),
        [](const Field& f){ return f.value.find("<script>") != std::string::npos; });
    std::cout << "form ok = "
              << std::boolalpha << (required_filled && safe) << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2553: 確認所有元素都是正整數
// ----------------------------------------------------------------
// 題目簡化:給陣列 nums,判斷「所有元素是否都是正整數」。
//      (取自 LC 2553「拆分數字後的位數和」的前置驗證步驟。)
//
// 為什麼用 std::all_of:
//   題意直接對應 all_of,一行寫完。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2553_separate_digits_all_positive() {
    std::vector<int> nums{13, 25, 83, 77};
    bool all_pos = std::all_of(nums.begin(), nums.end(),
                               [](int x){ return x > 0; });
    std::cout << "LC2553 all positive: " << std::boolalpha << all_pos << '\n';
}

// ----------------------------------------------------------------
// 實務範例:飛機起飛前的多項檢查 (Pre-flight checks)
// ----------------------------------------------------------------
// 場景:飛機起飛前要確認所有檢查項目都通過,且沒有任何「紅燈」 —
//      all_of 直接表達「所有都 OK」,程式碼極清晰。
void practical_pre_flight_checks() {
    struct Check { std::string name; bool ok; };
    std::vector<Check> checks{
        {"engine",  true},
        {"flaps",   true},
        {"radio",   true},
        {"weather", true},
    };
    bool ready = std::all_of(checks.begin(), checks.end(),
                             [](const Check& c){ return c.ok; });
    std::cout << "pre-flight ready: " << std::boolalpha << ready << '\n';
}

// === 預期輸出 (Expected output) ===
// all even: true
// any neg : false
// none neg: true
// w all even: false
// w any odd : true
// empty all_of  = true
// empty any_of  = false
// empty none_of = true
// LC1346: true
// LC1742: all positive = true
// form ok = true
// LC2553 all positive: true
// pre-flight ready: true
