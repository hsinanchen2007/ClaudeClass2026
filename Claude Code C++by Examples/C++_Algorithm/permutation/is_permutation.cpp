// ============================================================
// std::is_permutation    (C++11 起)
// 分類 (Category): Permutation operations (排列)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/is_permutation
//   * https://cplusplus.com/reference/algorithm/is_permutation/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::is_permutation 解的問題:
//
//   「兩個範圍裡的元素是否完全相同 (含出現次數),只是順序可能不同?」
//
// 換句話說,它問的是:「兩個 multiset 是否相等?」
//
// 經典應用:
//   * 異位詞 (anagram) 判定:LC 242
//   * 兩份資料集是否包含相同元素 (如稽核兩台機器的訂單)
//   * 字串 / 容器內容比對 (不在乎順序)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、與其他「比對」函式的差別                                │
// └────────────────────────────────────────────────────────────┘
//
//   ┌────────────────┬─────────────────────────────────────┐
//   │ equal          │ 「逐位置」相等 — 順序也要一樣         │
//   │ is_permutation │ 「multiset」相等 — 順序不重要         │
//   │ includes       │ 子集關係 (multiset)                   │
//   └────────────────┴─────────────────────────────────────┘
//
// equal 在意「順序」;is_permutation 不在意。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、三參數版 vs 四參數版                                   │
// └────────────────────────────────────────────────────────────┘
//
// 跟 std::equal 一樣:
//
//   * 三參數 (C++98):假設第二範圍 ≥ 第一範圍長度,否則 UB!
//   * 四參數 (C++14):自動檢查長度,長度不同直接回 false。
//
// **永遠用四參數版本** — 安全且更清楚。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、複雜度的細節                                           │
// └────────────────────────────────────────────────────────────┘
//
// is_permutation 的演算法不是「sort + ==」 — 它沒去動兩個容器:
//
//   1. 先跳過兩端「前綴相同」的部分。
//   2. 對剩餘段,對每個元素「在另一段中計數」 — 平方時間。
//
// 因此最壞 O(N²)。對小 N 沒問題,大 N 想加速:
//   * 雙方先 sort + std::equal → O(N log N)
//   * 用 unordered_map 計次比對 → O(N) 平均
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt1, class FwdIt2>
//   bool is_permutation(FwdIt1 first1, FwdIt1 last1, FwdIt2 first2);
//
//   // C++14 起:四參數版,推薦
//   template <class FwdIt1, class FwdIt2>
//   bool is_permutation(FwdIt1 first1, FwdIt1 last1,
//                       FwdIt2 first2, FwdIt2 last2);
//
//   + 帶 BinaryPred 的版本 (自訂等價判斷)
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 永遠用「四參數版」 — 三參數版有 UB 風險。
//   2. 是 multiset 比對 (重複次數要一致),不是 set 比對。
//   3. 大 N 想加速 → 自己用 sort+equal 或 unordered_map。
//   4. 自訂 BinaryPred 必須是「等價關係」(自反、對稱、傳遞)。
//
// ============================================================

/*
補充筆記：std::is_permutation
  - std::is_permutation 處理排列順序或字典序比較；它關心元素相對排列，不一定關心容器型別。
  - next_permutation/prev_permutation 會直接修改範圍，並回傳是否成功產生下一個排列。
  - 若一開始不是排序後的最小排列，next_permutation 只會從目前排列往後走，不會列出全部排列。
  - lexicographical_compare 像字典查單字一樣逐元素比較，常用於自訂排序或比較序列。
  - is_permutation 判斷兩段資料是否由相同元素組成，重複值也會納入計算。
  - 排列數量成長極快，n! 很快不可接受；教材範例小，工作上要先估算資料量。
  - std::is_permutation 處理的是序列排列，不是集合；相同元素出現多次時，排列與比較都會把重複值算進去。
  - std::is_permutation 常和字典序有關，字典序比較會從第一個不同元素決定大小，前面元素相同才比較長度。
  - std::is_permutation 是 non-modifying 演算法，【不會】改動輸入範圍（會改的是 next_permutation /
    prev_permutation）。它只做比較，代價在時間複雜度：最壞 O(n^2)。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::is_permutation
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::is_permutation 的複雜度是多少?資料量大時該怎麼辦?
//     答:最壞 O(N²) — 它不排序、也不配置額外空間,而是跳過共同前綴後,
//         對剩餘每個元素「在另一段中計數」。若兩個範圍「已經排序」,直接用
//         std::equal 即可 O(N);若可接受排序成本,sort + equal 是 O(N log N);
//         想要平均 O(N) 則自己用 unordered_map 計次比對。
//     追問:那為什麼標準不乾脆內部先排序?(答:它只拿到 forward iterator,
//           不保證能改動輸入,也不保證元素可排序 — 只要求可用 == 判等價)
//
// 🔥 Q2. 三參數版和四參數版差在哪?該用哪一個?
//     答:三參數版 is_permutation(first1, last1, first2) 假設第二個範圍
//         「至少和第一個一樣長」,若較短就會越界讀取 — UB。C++14 起新增四參數版
//         is_permutation(first1, last1, first2, last2),會先檢查兩段長度,
//         長度不同直接回 false。永遠優先用四參數版。
//     追問:還有哪些演算法有同樣的雙範圍安全版本?
//           (答:std::equal 與 std::mismatch,同樣是 C++14 加上的)
//
// ⚠️ 陷阱. is_permutation 會不會動到我的資料?它和 next_permutation 是同一類嗎?
//     答:不會。is_permutation 是「唯讀的判斷函式」,不重排任何一個範圍;
//         真正會原地修改容器的是 next_permutation / prev_permutation。
//         兩者都在 <algorithm> 的 permutation operations 分類下,但一個是查詢、
//         一個是變換。
//     為什麼會錯:很多人看到同分類就假設「排列相關 = 會重排」,於是多做了一次
//         不必要的備份;或反過來以為 next_permutation 也只是回傳而不改容器。
//
//    Q3. 它比較的是 set 還是 multiset?
//     答:multiset — 出現次數必須一致。{1,1,2} 與 {1,2,2} 元素種類相同,
//         但次數不同,回傳 false。想做 set 語意的比較,要自己先去重。
//     追問:自訂 BinaryPredicate 有什麼要求?
//           (答:必須是等價關係 — 自反、對稱、傳遞;不是 sort 用的嚴格弱序)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::cout << std::boolalpha;

    // --- 範例 1: 互為排列 ---
    std::vector<int> a{1, 2, 3, 4};
    std::vector<int> b{4, 3, 2, 1};
    std::cout << "{1,2,3,4} ~ {4,3,2,1}: "
              << std::is_permutation(a.begin(), a.end(),
                                     b.begin(), b.end()) << '\n';

    // --- 範例 2: 重複次數不同 → false ---
    std::vector<int> c{1, 1, 2, 3};
    std::vector<int> d{1, 2, 2, 3};
    std::cout << "{1,1,2,3} ~ {1,2,2,3}: "
              << std::is_permutation(c.begin(), c.end(),
                                     d.begin(), d.end()) << '\n';

    // --- 範例 3: 長度不同 → false ---
    std::vector<int> e{1, 2, 3};
    std::vector<int> f{1, 2, 3, 4};
    std::cout << "size mismatch: "
              << std::is_permutation(e.begin(), e.end(),
                                     f.begin(), f.end()) << '\n';

    // --- 範例 4: 自訂等價判斷 — 大小寫不敏感 ---
    std::string s1 = "AbC";
    std::string s2 = "cab";
    bool p = std::is_permutation(s1.begin(), s1.end(),
                                 s2.begin(), s2.end(),
                                 [](char x, char y){
                                     return std::tolower(x) == std::tolower(y);
                                 });
    std::cout << "case-insensitive perm: " << p << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_242_valid_anagram();
    void leetcode_567_permutation_in_string();
    void practical_dataset_match();
    void leetcode_1207_unique_occurrences();
    void practical_inventory_match();
    std::cout << "\n--- LeetCode / Practical Examples ---\n";
    leetcode_242_valid_anagram();
    leetcode_567_permutation_in_string();
    practical_dataset_match();
    leetcode_1207_unique_occurrences();
    practical_inventory_match();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 242: 有效的字母異位詞 (Valid Anagram)
// ----------------------------------------------------------------
// 題目:給兩字串 s 與 t,判斷 t 是否為 s 的字母異位詞 (anagram)。
//
// 為什麼用 std::is_permutation:
//   anagram 的定義就是「兩字串包含完全相同的字母 (含次數),只是順序不同」 —
//   就是 multiset 相等 — 直接呼叫 is_permutation 一行解決。
//
// 複雜度:時間 O(n²) 最壞 (對短字串足夠);空間 O(1)。
static bool isAnagram(const std::string& s, const std::string& t) {
    if (s.size() != t.size()) return false;
    return std::is_permutation(s.begin(), s.end(), t.begin(), t.end());
}
void leetcode_242_valid_anagram() {
    std::cout << "LC242 (\"anagram\" vs \"nagaram\"): "
              << std::boolalpha << isAnagram("anagram", "nagaram") << '\n';
    std::cout << "LC242 (\"rat\" vs \"car\"): "
              << isAnagram("rat", "car") << '\n';
}

// ----------------------------------------------------------------
// LeetCode 567: 字串的排列 (Permutation in String)
// ----------------------------------------------------------------
// 題目:給 s1 與 s2,判斷 s2 是否包含 s1 的某個排列作為子字串。
//
// 為什麼用 std::is_permutation:
//   滑動視窗 — 對每個長度為 |s1| 的視窗,呼叫 is_permutation 與 s1 比對。
//   注意:O(n) 最佳解需要計次表;這裡示範 is_permutation 的簡潔性。
//
// 複雜度:時間 O((|s2| - |s1| + 1) × |s1|²) (示範用)。
static bool checkInclusion(const std::string& s1, const std::string& s2) {
    if (s1.size() > s2.size()) return false;
    const std::size_t n = s1.size();
    for (std::size_t i = 0; i + n <= s2.size(); ++i) {
        if (std::is_permutation(s1.begin(), s1.end(),
                                s2.begin() + i, s2.begin() + i + n)) {
            return true;
        }
    }
    return false;
}
void leetcode_567_permutation_in_string() {
    std::cout << "LC567 (s1=\"ab\", s2=\"eidbaooo\"): "
              << std::boolalpha << checkInclusion("ab", "eidbaooo") << '\n';
    std::cout << "LC567 (s1=\"ab\", s2=\"eidboaoo\"): "
              << checkInclusion("ab", "eidboaoo") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:server vs client 兩份 ID 是否一致 (順序不限)
// ----------------------------------------------------------------
// 場景:後端與前端各自收集一組 ID,驗證「元素集合 + 次數完全相同」(順序不限)。
//
// 為什麼用 std::is_permutation:
//   不必先 sort 也不必建 map,一行直接比對 multiset。
void practical_dataset_match() {
    std::vector<int> server_ids   {101, 205, 309, 401, 205};
    std::vector<int> client_ids   {205, 101, 401, 205, 309};
    std::vector<int> tampered_ids {205, 101, 401, 309, 999};
    std::cout << "Practical (server vs client): "
              << std::boolalpha
              << std::is_permutation(server_ids.begin(), server_ids.end(),
                                     client_ids.begin(), client_ids.end())
              << '\n';
    std::cout << "Practical (server vs tampered): "
              << std::is_permutation(server_ids.begin(), server_ids.end(),
                                     tampered_ids.begin(), tampered_ids.end())
              << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1207 概念:利用 is_permutation 驗證頻次表
// ----------------------------------------------------------------
// 題目:給陣列 arr,判斷其每個元素的「出現次數」彼此都不相同。
//      簡化用法:把計次結果與 [1, 2, ..., k] 比較是否為排列。
//
// 為什麼用 std::is_permutation (作為輔助):
//   雖然主要要先計次,但 is_permutation 適合「比對 multiset」的驗證 step。
//
// 複雜度:時間 O(n²) (示範);空間 O(n)。
void leetcode_1207_unique_occurrences() {
    std::vector<int> arr{1, 2, 2, 1, 1, 3};
    std::map<int, int> cnt;
    for (int x : arr) ++cnt[x];
    std::vector<int> freqs;
    for (auto& [k, v] : cnt) freqs.push_back(v);
    std::set<int> uniq(freqs.begin(), freqs.end());
    bool unique = (uniq.size() == freqs.size());
    std::cout << "LC1207: " << std::boolalpha << unique << '\n';
}

// ----------------------------------------------------------------
// 實務範例:庫存盤點 — 系統紀錄與實際清點是否一致 (順序不限)
// ----------------------------------------------------------------
// 場景:倉庫盤點時,實際擺放順序與系統紀錄順序不同,但若要視為「庫存一致」,
//      條件是「兩集合的 multiset 相等」 — is_permutation 一行驗證。
void practical_inventory_match() {
    std::vector<std::string> system{"A101", "A102", "A101", "B205", "C300"};
    std::vector<std::string> actual{"A101", "C300", "B205", "A102", "A101"};
    bool ok = std::is_permutation(system.begin(), system.end(),
                                  actual.begin(), actual.end());
    std::cout << "inventory match: " << std::boolalpha << ok << '\n';
}

// === 預期輸出 (Expected output) ===
// {1,2,3,4} ~ {4,3,2,1}: true
// {1,1,2,3} ~ {1,2,2,3}: false
// size mismatch: false
// case-insensitive perm: true
//
// --- LeetCode / Practical Examples ---
// LC242 ("anagram" vs "nagaram"): true
// LC242 ("rat" vs "car"): false
// LC567 (s1="ab", s2="eidbaooo"): true
// LC567 (s1="ab", s2="eidboaoo"): false
// Practical (server vs client): true
// Practical (server vs tampered): false
// LC1207: true
// inventory match: true
