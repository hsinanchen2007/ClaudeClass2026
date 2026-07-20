// ============================================================
// std::lexicographical_compare
// 分類 (Category): Comparison operations (字典序比較)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/lexicographical_compare
//   * https://cplusplus.com/reference/algorithm/lexicographical_compare/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 什麼是「字典序」?                          │
// └────────────────────────────────────────────────────────────┘
//
// 「字典序 (lexicographical order)」就是「像翻字典」的比較方式:
//
//   1. 從左到右逐位置比。
//   2. 第一個不同的位置,誰小誰排前面。
//   3. 若全部相同,長度短的算小。
//
// 範例:
//   "apple" vs "banana"  → 'a' < 'b' → apple 小
//   "apple" vs "apply"   → 前 4 個相同,第 5 個 'e' < 'y' → apple 小
//   "app"   vs "apple"   → 前 3 個相同,但 "app" 較短 → app 小
//
// std::lexicographical_compare 就是把這個概念變成函式 — 不只能用在字串,
// 任何序列 (vector<int>、list<string> ...) 都能用字典序比。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、跟 std::string::compare 有什麼差別?                    │
// └────────────────────────────────────────────────────────────┘
//
//   * std::string::compare       回傳 int (負/零/正),三向比較
//   * std::lexicographical_compare 回傳 bool ( a < b 與否)
//
// 字串比較通常用 string::compare 或直接 <、==;但對「任意序列」
// (例如 vector<int> 兩個 vector 比較) 就需要 lexicographical_compare。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼字典序 ≠ 數值序? (常見陷阱)                     │
// └────────────────────────────────────────────────────────────┘
//
// 字典序對「字元」逐個比,這跟「數值大小」是兩件事:
//
//   "9"  vs "10"  字典序:'9' > '1' → "9" > "10"
//                數值序:9 < 10
//
//   "1.2.10" vs "1.2.9"  字典序: 在 "..1." 後,'1' < '9' → "1.2.10" < "1.2.9"
//                        實際版本: 1.2.10 > 1.2.9
//
// 寫「版本比較」「LeetCode 179 最大數」時要特別小心 — 不能直接用字典序。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt1, class InputIt2>
//   bool lexicographical_compare(InputIt1 first1, InputIt1 last1,
//                                InputIt2 first2, InputIt2 last2);
//
//   template <class InputIt1, class InputIt2, class Compare>
//   bool lexicographical_compare(InputIt1 first1, InputIt1 last1,
//                                InputIt2 first2, InputIt2 last2,
//                                Compare comp);
//
//   * C++20 起為 constexpr。
//   * C++20 引入 std::lexicographical_compare_three_way (回 strong_ordering)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值與複雜度                                         │
// └────────────────────────────────────────────────────────────┘
//
//   回傳: 第一範圍 < 第二範圍 (字典序) → true;否則 (含等於) → false
//   時間: 最多 2 × min(N1, N2) 次比較
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. comp 必須符合嚴格弱序。
//   2. 前綴相同但長度不同 — 「短的」較小。
//   3. 字典序 ≠ 數值序 — 版本比較、最大數題要轉成數字 vector 再比。
//   4. C++20 三向比較 lexicographical_compare_three_way 更精確。
//
// ============================================================

/*
補充筆記：std::lexicographical_compare
  - std::lexicographical_compare 處理排列順序或字典序比較；它關心元素相對排列，不一定關心容器型別。
  - next_permutation/prev_permutation 會直接修改範圍，並回傳是否成功產生下一個排列。
  - 若一開始不是排序後的最小排列，next_permutation 只會從目前排列往後走，不會列出全部排列。
  - lexicographical_compare 像字典查單字一樣逐元素比較，常用於自訂排序或比較序列。
  - is_permutation 判斷兩段資料是否由相同元素組成，重複值也會納入計算。
  - 排列數量成長極快，n! 很快不可接受；教材範例小，工作上要先估算資料量。
  - std::lexicographical_compare 處理的是序列排列，不是集合；相同元素出現多次時，排列與比較都會把重複值算進去。
  - std::lexicographical_compare 常和字典序有關，字典序比較會從第一個不同元素決定大小，前面元素相同才比較長度。
  - std::lexicographical_compare 可能直接重排原範圍；呼叫後若還需要原順序，應先保留副本或改用不修改輸入的判斷函式。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::lexicographical_compare
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是「字典序」?兩段長度不同的序列怎麼比?
//     答:像翻字典 — 從左到右逐位置比,第一個不同的位置就決定大小;若一路比到
//         其中一段結束都相同,則「較短的那段較小」,也就是前綴小於較長的序列
//         ("ab" < "abc")。複雜度 O(min(N1, N2)) — 標準規定最多
//         2 × min(N1, N2) 次比較,只要 input iterator 即可。
//     追問:回傳型別是什麼?怎麼判斷「相等」?
//           (答:回傳 bool,只回答 a < b;兩段相等時回 false。
//            要區分「小於 / 等於 / 大於」得比較兩個方向,或改用三向比較)
//
// 🔥 Q2. 我直接用 s1 < s2 比較 std::string、用 v1 < v2 比較 std::vector,
//        跟這個演算法有什麼關係?
//     答:那就是同一套語意 — 標準容器的 operator< 定義成字典序比較,
//         lexicographical_compare 只是把這個規則抽出來,讓「任意一對迭代器範圍」
//         都能用,包含來源型別不同的兩段(例如 vector<int> 對 list<int>),
//         以及需要自訂 comp 的情況(如不分大小寫)。
//     追問:自訂 comp 有什麼要求?(答:必須構成嚴格弱序,和 std::sort 同一套要求)
//
// ⚠️ 陷阱. 字典序可以拿來比版本號或數字字串嗎?
//     答:不行。字典序是逐「元素」比,不是逐「數值」比:字串 "9" 與 "10" 比,
//         第一個字元 '9' > '1',所以字典序上 "9" > "10";同理 "1.2.10" 字典序
//         小於 "1.2.9"。正解是先把版本切成 std::vector<int>,再對整數序列做
//         lexicographical_compare — 這時「逐段比、短的較小」剛好就是版本語意。
//     為什麼會錯:腦中把「字典序」和「由小到大的自然順序」畫上等號,忘了比較的
//         單位是字元;而字元 '1' 的順序和整數 10 的大小是兩回事。
//
//    Q3. C++20 之後有更好的寫法嗎?
//     答:有 std::lexicographical_compare_three_way,回傳的是三向比較結果
//         (ordering)而不是 bool,一次就能區分小於 / 等於 / 大於,不必為了拿到
//         「等於」而正反各比一次。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::cout << std::boolalpha;

    // --- 範例 1: 字串比較 ---
    std::string a = "apple";
    std::string b = "banana";
    std::cout << "apple < banana: "
              << std::lexicographical_compare(a.begin(), a.end(),
                                              b.begin(), b.end()) << '\n';

    // --- 範例 2: 整數向量字典序比 ---
    std::vector<int> v1{1, 2, 3};
    std::vector<int> v2{1, 2, 4};
    std::cout << "{1,2,3} < {1,2,4}: "
              << std::lexicographical_compare(v1.begin(), v1.end(),
                                              v2.begin(), v2.end()) << '\n';

    // --- 範例 3: 前綴相同,長度不同 (短的較小) ---
    std::vector<int> v3{1, 2};
    std::vector<int> v4{1, 2, 0};
    std::cout << "{1,2} < {1,2,0}: "
              << std::lexicographical_compare(v3.begin(), v3.end(),
                                              v4.begin(), v4.end()) << '\n';

    // --- 範例 4: 自訂 comp — 大小寫不敏感 ---
    std::string s1 = "Apple";
    std::string s2 = "banana";
    bool ci = std::lexicographical_compare(s1.begin(), s1.end(),
                                           s2.begin(), s2.end(),
                                           [](char x, char y){
                                               return std::tolower(x) < std::tolower(y);
                                           });
    std::cout << "case-insensitive Apple < banana: " << ci << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1370_lex_order();
    void leetcode_179_largest_number_concept();
    void practical_version_compare();
    void leetcode_2299_strong_password_lex();
    void practical_compare_byte_buffers();
    std::cout << "\n--- LeetCode / Practical Examples ---\n";
    leetcode_1370_lex_order();
    leetcode_179_largest_number_concept();
    practical_version_compare();
    leetcode_2299_strong_password_lex();
    practical_compare_byte_buffers();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1370 概念:依字典序排序字串清單
// ----------------------------------------------------------------
// 題目:LC 1370 是「字串重排」題,但「依字典序排序字串」是它的核心子步驟。
//
// 為什麼用 std::lexicographical_compare:
//   作為 std::sort 的比較器,把字串依字典序排序。
//   ASCII 中大寫 < 小寫,所以 "Apple" 會排在小寫 "apple" 之前。
void leetcode_1370_lex_order() {
    std::vector<std::string> words{"banana", "apple", "cherry", "Apple"};
    std::sort(words.begin(), words.end(),
              [](const std::string& x, const std::string& y) {
                  return std::lexicographical_compare(x.begin(), x.end(),
                                                      y.begin(), y.end());
              });
    std::cout << "LC1370 (lex sort):";
    for (const auto& w : words) std::cout << ' ' << w;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 179 概念:字典序 ≠ 數值序的陷阱示範
// ----------------------------------------------------------------
// 題目:LC 179 「最大數」要把整數陣列重排接成最大的數,
//      不能用單純的字典序比較 — 必須用「a+b vs b+a」的拼接比較。
//
// 為什麼示範 std::lexicographical_compare:
//   先示範字典序中 "9" > "10" (因為 '9' > '1') — 這是常見陷阱。
//   再示範正解的「a+b > b+a」拼接比較。
void leetcode_179_largest_number_concept() {
    std::string a = "9";
    std::string b = "10";
    bool a_lt_b = std::lexicographical_compare(a.begin(), a.end(),
                                               b.begin(), b.end());
    std::cout << "LC179 (\"9\" < \"10\" lexicographically): "
              << std::boolalpha << a_lt_b << '\n';   // false ('9' > '1')

    std::vector<std::string> nums{"3", "30", "34", "5", "9"};
    std::sort(nums.begin(), nums.end(),
              [](const std::string& x, const std::string& y) {
                  return x + y > y + x;
              });
    std::string ans;
    for (const auto& s : nums) ans += s;
    std::cout << "LC179 (largest number from {3,30,34,5,9}): " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:版本字串比較 — 字典序錯,要轉整數 vector
// ----------------------------------------------------------------
// 場景:軟體版本如 "1.2.10" 不能用字典序直接比 — "1.2.10" 字典序小於 "1.2.9"!
//      正確做法:把版本字串切成 vector<int>,再用 lexicographical_compare 比較整數序列。
//
// 為什麼用 std::lexicographical_compare (對 vector<int>):
//   vector<int> 用字典序比,等同「逐段、不夠長者較小」 — 完全符合版本比較語意。
static std::vector<int> parse_version(const std::string& v) {
    std::vector<int> out;
    std::stringstream ss(v);
    std::string token;
    while (std::getline(ss, token, '.')) {
        out.push_back(std::stoi(token));
    }
    return out;
}
void practical_version_compare() {
    auto v1 = parse_version("1.2.10");
    auto v2 = parse_version("1.2.9");
    bool naive  = std::lexicographical_compare(std::string("1.2.10").begin(),
                                               std::string("1.2.10").end(),
                                               std::string("1.2.9").begin(),
                                               std::string("1.2.9").end());
    bool proper = std::lexicographical_compare(v1.begin(), v1.end(),
                                               v2.begin(), v2.end());
    std::cout << "Practical version compare:\n";
    std::cout << "  naive   string \"1.2.10\" < \"1.2.9\": "
              << std::boolalpha << naive  << "  (wrong!)\n";
    std::cout << "  parsed  ints   {1,2,10} < {1,2,9} : "
              << proper << "  (correct: 1.2.10 > 1.2.9)\n";
}

// ----------------------------------------------------------------
// LeetCode 概念題:依字典序選最小字串 (Strong Password tiebreaker)
// ----------------------------------------------------------------
// 題目:多個候選字串,要選「字典序最小」的那一個 (例如同分時當 tiebreaker)。
//
// 為什麼用 std::lexicographical_compare:
//   min_element + lexicographical_compare 作 comparator,一行得到結果。
//
// 複雜度:時間 O(n × m);空間 O(1)。
void leetcode_2299_strong_password_lex() {
    std::vector<std::string> candidates{"Pass!1aA", "Aa1!ssss", "Bb2@cccc"};
    auto it = std::min_element(candidates.begin(), candidates.end(),
        [](const std::string& a, const std::string& b){
            return std::lexicographical_compare(a.begin(), a.end(),
                                                b.begin(), b.end());
        });
    std::cout << "lex smallest: " << *it << '\n';
}

// ----------------------------------------------------------------
// 實務範例:比較兩個 byte buffer 的字典序 (例如 hash 比對)
// ----------------------------------------------------------------
// 場景:DHT (分散式雜湊表) 或 Git 物件比對,常需要對 byte buffer
//      做字典序比較,而非數值比較。lexicographical_compare 直接適用。
void practical_compare_byte_buffers() {
    std::vector<unsigned char> a{0x12, 0x34, 0x56, 0x78};
    std::vector<unsigned char> b{0x12, 0x34, 0x57, 0x00};
    bool a_lt_b = std::lexicographical_compare(a.begin(), a.end(),
                                               b.begin(), b.end());
    std::cout << "hash A < hash B: " << std::boolalpha << a_lt_b << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra lexicographical_compare.cpp -o lexicographical_compare

// === 預期輸出 ===
// apple < banana: true
// {1,2,3} < {1,2,4}: true
// {1,2} < {1,2,0}: true
// case-insensitive Apple < banana: true
//
// --- LeetCode / Practical Examples ---
// LC1370 (lex sort): Apple apple banana cherry
// LC179 ("9" < "10" lexicographically): false
// LC179 (largest number from {3,30,34,5,9}): 9534330
// Practical version compare:
//   naive   string "1.2.10" < "1.2.9": true  (wrong!)
//   parsed  ints   {1,2,10} < {1,2,9} : false  (correct: 1.2.10 > 1.2.9)
// lex smallest: Aa1!ssss
// hash A < hash B: true
