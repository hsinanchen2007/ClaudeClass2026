// ============================================================
// std::adjacent_find
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/adjacent_find
//   * https://cplusplus.com/reference/algorithm/adjacent_find/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// adjacent_find 解決的問題很單純,但極其常用:
//
//   「在一段資料裡,找出第一對『緊鄰且符合某關係』的兩個元素」
//
// 預設的「關係」是 operator== — 找第一對相鄰相等。
// 但你也可以自訂二元述詞,把「關係」換成「大於」「小於」「字首相同」等等,
// 它就變成「找第一個下降點」「找第一個上升點」「找第一個分組邊界」...
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼名稱叫 adjacent (相鄰)?                         │
// └────────────────────────────────────────────────────────────┘
//
// 「相鄰」是這個函式最重要的字眼。它只比較
//
//   *(it) 與 *(it+1)
//
// 兩個位置相鄰的元素。它「不是」拿元素去和「整個容器」找重複!
//
// 常見誤解:
//   ✘ 用 adjacent_find 想找「容器中所有重複的元素」 → 錯了,
//     它只能找「位置相鄰」的重複。要找全域重複請先排序,或用 set/map。
//
// 經典應用模式:
//   * 偵測「有序資料」是否含重複 (排序 → adjacent_find)
//   * 找第一個「轉折點」(自訂述詞 a > b 或 a < b)
//   * 找第一個「分組邊界」(述詞 a.group != b.group)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt>
//   FwdIt adjacent_find(FwdIt first, FwdIt last);
//
//   template <class FwdIt, class BinaryPred>
//   FwdIt adjacent_find(FwdIt first, FwdIt last, BinaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載 (可平行)。
//   * C++20 引入 std::ranges::adjacent_find。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、參數與回傳值 (Parameters & Return value)               │
// └────────────────────────────────────────────────────────────┘
//
//   first, last : 搜尋範圍 [first, last)
//   p           : 二元述詞,傳入 (前者, 後者) 應回傳 bool
//                 預設未提供 p 時等同 operator==
//
//   回傳:
//     * 找到 → 指向「相鄰一對中前者」的迭代器 it,
//              該對為 (*it, *(it+1))
//     * 找不到 → last
//     * 範圍少於 2 個元素 → last
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 最多 (last - first - 1) 次比較,O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、需求 (Requirements)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   FwdIt 至少滿足 LegacyForwardIterator (因為要看 it 和 it+1 兩個位置,
//   InputIterator 不夠用 — InputIterator 走過去就回不來了)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「相鄰」 — 找全域重複需先排序,別期待它能跨位置比對。
//   2. 回傳的是「前者」的迭代器,要拿後者請用 *(it+1)。
//   3. 述詞的「方向」: p(前者, 後者) — 寫 a > b 是「前者 > 後者」即下降。
//   4. 範圍只有 0 或 1 個元素時直接回傳 last,不要解參考。
//
// ============================================================

/*
補充筆記：std::adjacent_find
  - adjacent_find 找第一組相鄰且相等的元素。
  - 自訂 predicate 可以定義「相鄰關係」，例如差值小於某個門檻。
  - 找不到時回傳 last，找到時回傳第一個元素的位置。
  - std::adjacent_find 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::adjacent_find
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. adjacent_find 找什麼?回傳的是哪一個 iterator?
//     答:找「第一對『位置相鄰』且滿足關係的元素」— 預設關係是 operator==。
//         回傳的是這一對中「前者」的 iterator,後者要自己取 *(std::next(it))。
//         找不到、或區間元素少於 2 個,都回傳 last。
//     追問:述詞的參數順序是什麼?(p(前者, 後者) — 所以寫 a > b 表示「前者大於後者」,
//           即找第一個下降點)
//
// ⚠️ 陷阱. 可以用 adjacent_find 找出「容器裡所有重複的元素」嗎?
//     答:不行。它只比較 *it 與 *(it+1) 這種位置相鄰的一對,
//         不會拿一個元素去和整個容器比對。
//         {1, 2, 1} 用 adjacent_find 會回 last,因為兩個 1 並不相鄰。
//         要偵測全域重複,得先 sort 再 adjacent_find,或改用 set / unordered_set。
//     為什麼會錯:看到 "find" 加上「相等」就直覺理解成「找重複值」,
//         忽略了 adjacent 這個字才是這個演算法的全部限制條件。
//
// Q2. 為什麼 adjacent_find 要求 ForwardIterator,而 std::find 只要 InputIterator?
//     答:因為它必須同時看到「相鄰的兩個位置」。
//         InputIterator 是單次走訪的,走過去就回不來、也不保證能保留前一個位置;
//         ForwardIterator 才支援多次走訪與保存 iterator 副本。
//
// Q3. 有哪些典型用法是靠自訂述詞達成的?
//     答:把「相鄰關係」換掉即可 —
//         p = (a > b) 找第一個下降點(可用來驗證是否遞增)、
//         p = (a.group != b.group) 找第一個分組邊界、
//         p = (std::abs(a - b) < eps) 找第一對過於接近的值。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 預設 operator== 找第一對相鄰相等 ---
    std::vector<int> v{1, 2, 3, 3, 4, 5, 5};
    auto it = std::adjacent_find(v.begin(), v.end());
    if (it != v.end())
        std::cout << "first equal pair at index " << (it - v.begin())
                  << ", value=" << *it << '\n';

    // --- 範例 2: 自訂述詞找第一個「下降點」(a > b 即遞減) ---
    // 對檢查序列是否單調遞增很有用。
    std::vector<int> w{1, 3, 5, 4, 7, 2};
    auto down = std::adjacent_find(w.begin(), w.end(),
                                   [](int a, int b){ return a > b; });
    if (down != w.end())
        std::cout << "first descent: " << *down << " > " << *(down + 1) << '\n';

    // --- 範例 3: 找不到的情況 (嚴格遞增,沒有任何相鄰相等) ---
    std::vector<int> sorted{1, 2, 3, 4, 5};
    auto none = std::adjacent_find(sorted.begin(), sorted.end());
    std::cout << "no equal pair: " << (none == sorted.end() ? "OK" : "FAIL") << '\n';

    // --- 範例 4: 邊界情況 — 容器只有 1 個元素必回 end() ---
    std::vector<int> single{42};
    auto s = std::adjacent_find(single.begin(), single.end());
    std::cout << "single element returns end: "
              << (s == single.end() ? "yes" : "no") << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_217_contains_duplicate();
    void leetcode_896_monotonic_array();
    void practical_detect_consecutive_login();
    void leetcode_1909_remove_one_strictly_increasing();
    void practical_thermometer_glitch();
    leetcode_217_contains_duplicate();
    leetcode_896_monotonic_array();
    practical_detect_consecutive_login();
    leetcode_1909_remove_one_strictly_increasing();
    practical_thermometer_glitch();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================
//
// 選題原則:adjacent_find 真正自然的應用是
//   * 已排序資料找重複 (LC 217)
//   * 檢查單調性 (LC 896)
//   * 在事件流中找連續重複行為 (實務)

// ----------------------------------------------------------------
// LeetCode 217: 是否存在重複元素 (Contains Duplicate)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,若任何元素出現兩次以上回傳 true,否則 false。
//
// 為什麼用 adjacent_find:
//   排序後,所有相同的元素就會緊靠在一起。
//   只要還有任何重複,排序後就「至少有一對相鄰相等」 → adjacent_find 抓得到。
//
// 解法步驟:
//   1. std::sort 把同值元素湊到一起。
//   2. std::adjacent_find 看是否存在相鄰相等對。
//   3. 不等於 end() 即代表有重複。
//
// 複雜度:時間 O(n log n) (排序主導);空間 O(1)。
//        若記憶體不是問題,unordered_set 解法是 O(n) 平均。
void leetcode_217_contains_duplicate() {
    std::vector<int> nums{1, 2, 3, 1};
    std::sort(nums.begin(), nums.end());
    bool dup = std::adjacent_find(nums.begin(), nums.end()) != nums.end();
    std::cout << "LC217: containsDuplicate = "
              << (dup ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 896: 單調數列 (Monotonic Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums,判斷它是否「整體單調」(全部非遞減 或 全部非遞增)。
//
// 為什麼用 adjacent_find:
//   「不是非遞減」⇔「存在某對相鄰 a > b」 — 這正是 adjacent_find + 述詞的事。
//   同理「不是非遞增」⇔「存在某對相鄰 a < b」。
//
// 解法步驟:
//   1. 用 adjacent_find 配 (a > b) 看是否「不是非遞減」。
//   2. 用 adjacent_find 配 (a < b) 看是否「不是非遞增」。
//   3. 兩種至少有一種成立 → 單調。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_896_monotonic_array() {
    std::vector<int> nums{1, 2, 2, 3};
    bool not_inc = std::adjacent_find(nums.begin(), nums.end(),
                       [](int a, int b){ return a > b; }) != nums.end();
    bool not_dec = std::adjacent_find(nums.begin(), nums.end(),
                       [](int a, int b){ return a < b; }) != nums.end();
    bool monotonic = !not_inc || !not_dec;
    std::cout << "LC896: " << (monotonic ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:偵測連續重複登入
// ----------------------------------------------------------------
// 場景:安全稽核要找出「同一個 user 連續兩次登入」的位置 —
//      這常常是腳本自動重送或機器人攻擊的徵兆。
//
// 為什麼用 adjacent_find:
//   事件流已照時間排序,我們只想找「相鄰兩筆使用者相同」的那一刻,
//   不關心整體有多少個 user 重複。adjacent_find 配述詞剛好對應這種需求。
struct LoginEvent {
    long timestamp;
    std::string user;
};
void practical_detect_consecutive_login() {
    std::vector<LoginEvent> events{
        {100, "alice"},
        {110, "bob"},
        {120, "carol"},
        {130, "carol"},   // ← 連續兩次 carol → 應被偵測
        {140, "dave"}
    };
    auto it = std::adjacent_find(events.begin(), events.end(),
        [](const LoginEvent& a, const LoginEvent& b){
            return a.user == b.user;
        });
    if (it != events.end())
        std::cout << "consecutive login: " << it->user
                  << " at ts=" << it->timestamp << '\n';
    else
        std::cout << "no consecutive login\n";
}

// ----------------------------------------------------------------
// LeetCode 1909: 移除一個元素使陣列嚴格遞增
// 難度: medium
// ----------------------------------------------------------------
// 題目:給陣列 nums,判斷是否能「移除最多一個元素」後變為嚴格遞增。
//
// 為什麼用 std::adjacent_find:
//   先用 adjacent_find 找出第一個「a >= b」(違反嚴格遞增) 的位置;
//   若找到,嘗試把 *it 或 *(it+1) 移掉,再檢查是否剩餘嚴格遞增。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1909_remove_one_strictly_increasing() {
    auto strict_inc = [](const std::vector<int>& v) {
        return std::adjacent_find(v.begin(), v.end(),
                   [](int a, int b){ return a >= b; }) == v.end();
    };
    auto try_remove = [&](std::vector<int> v) {
        auto it = std::adjacent_find(v.begin(), v.end(),
                       [](int a, int b){ return a >= b; });
        if (it == v.end()) return true;
        // 嘗試刪掉 *it 或 *(it+1)
        auto idx = it - v.begin();
        std::vector<int> a = v, b = v;
        a.erase(a.begin() + idx);
        b.erase(b.begin() + idx + 1);
        return strict_inc(a) || strict_inc(b);
    };
    std::vector<int> ok{1, 2, 10, 5, 7};
    std::vector<int> bad{1, 2, 1, 2};
    std::cout << std::boolalpha;
    std::cout << "LC1909 ok=" << try_remove(ok)
              << " bad=" << try_remove(bad) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:溫度感測器毛刺 (glitch) 偵測
// ----------------------------------------------------------------
// 場景:溫度讀數每秒一筆,若兩筆相鄰讀數差距 > 10 度,
//      表示感測器毛刺;用 adjacent_find + 自訂謂詞快速找出第一個毛刺位置。
void practical_thermometer_glitch() {
    std::vector<double> temps{23.1, 23.3, 23.5, 35.0, 35.1, 35.2};
    auto it = std::adjacent_find(temps.begin(), temps.end(),
        [](double a, double b){ return std::abs(b - a) > 10.0; });
    if (it != temps.end())
        std::cout << "glitch between idx=" << (it - temps.begin())
                  << " and " << (it - temps.begin() + 1)
                  << " (" << *it << " -> " << *(it+1) << ")\n";
    else
        std::cout << "no glitch\n";
}

// === 預期輸出 (Expected output) ===
// first equal pair at index 2, value=3
// first descent: 5 > 4
// no equal pair: OK
// single element returns end: yes
// LC217: containsDuplicate = true
// LC896: true
// consecutive login: carol at ts=120
// LC1909 ok=true bad=false
// glitch between idx=2 and 3 (23.5 -> 35)
