// =============================================================================
// 檔名: comparison_operators.cpp
// 主題: std::string 的比較運算子 (== != < > <= >= <=>)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/operator_cmp
//   - cplusplus.com: https://cplusplus.com/reference/string/string/operators/
// =============================================================================
//
// 【函式資訊 Information】
//   // C++17 之前 (傳統):
//   bool operator==(const string& lhs, const string& rhs) noexcept;
//   bool operator!=(const string& lhs, const string& rhs) noexcept;
//   bool operator< (const string& lhs, const string& rhs) noexcept;
//   bool operator<=(const string& lhs, const string& rhs) noexcept;
//   bool operator> (const string& lhs, const string& rhs) noexcept;
//   bool operator>=(const string& lhs, const string& rhs) noexcept;
//
//   // C++20 起改為三向比較 (spaceship):
//   bool operator==(const string& lhs, const string& rhs) noexcept;
//   auto operator<=>(const string& lhs, const string& rhs) noexcept
//       -> /* 通常是 std::strong_ordering */;
//   // 其他 5 個運算子 (!= < > <= >=) 由編譯器自動從 <=> 合成
//
// 也存在 string vs const char* 的混合重載 (避免每次比較都建構暫時 string)。
//
// 所屬類別: 非成員函式 (non-member functions),屬於 std 命名空間
// 標準:    C++98 起 (傳統 6 個運算子) / C++20 起 (spaceship 改寫)
//
// 【詳細解釋 Explanation】
//
// (1) 比較的本質 ── 字典序 (lexicographical order):
//     std::string 的所有比較運算子採用「字典序逐字元比較」:
//       a. 從位置 0 開始,逐字元比較
//       b. 第一個不相等的字元決定結果 (字元值小者「較小」)
//       c. 若一邊先用完字元 (是另一邊的前綴),較短的那邊「較小」
//     這個語意與 std::lexicographical_compare 一致,等同呼叫 compare 後判斷正負/零。
//
//     範例:
//       "apple"  <  "banana"   // 第 0 位 'a' < 'b'
//       "abc"    <  "abcd"     // 前 3 字相同,左邊先結束
//       "abc"    == "abc"      // 完全一致
//       "Apple"  <  "apple"    // 'A' (65) < 'a' (97);大小寫敏感!
//       "10"     <  "2"        // '1' < '2';字典序不是數值排序!
//
// (2) C++20 之前 vs 之後 ── 從 6 個運算子到 spaceship:
//
//     【C++17 及以前】
//     必須提供 6 個獨立的運算子,且它們之間的「一致性」需自己保證:
//       == 與 != 的關係、< 與 > 的對稱性、<= 與 >= 的補集關係...
//     若漏寫或寫錯,容易發生 a == b 但 (a < b || a > b) 卻為 true 的怪事。
//
//     【C++20 spaceship】
//     只要實作:
//       a. operator==  (回 bool,告知「是否相等」)
//       b. operator<=> (回 ordering type,告知「順序關係」)
//     編譯器會「自動合成」!=, <, >, <=, >= 共 4 個運算子。
//     這稱為 "rewritten candidate":a < b 編譯器會嘗試 (a <=> b) < 0、(b <=> a) > 0 等。
//     大幅減少樣板程式碼,也保證一致性。
//
// (3) <=> 的回傳型別 ── 三種 ordering:
//     - std::strong_ordering  : 完全有序,a==b 時兩者「不可區分」(int、string)
//     - std::weak_ordering    : 有序但 a==b 不代表「不可區分」(忽略大小寫的字串)
//     - std::partial_ordering : 部分有序,可能無法比較 (NaN 的 float)
//
//     std::string 的 <=> 回傳 std::strong_ordering ── 字串的字典序是完全有序。
//
// (4) 各種重載 ── 避免暫時 string:
//     除了 string-vs-string,還有:
//       - string vs const char*  (避免 "abc" 必先建構 std::string)
//       - const char* vs string  (對稱)
//       - C++17 起:string vs string_view
//     讓 if (s == "hello") 不會配置記憶體。
//
// (5) 時間複雜度: O(min(N1, N2))
//     最壞情況 (兩字串完全相同) 仍要走完整段;最好情況 (首字元就不同) 即時返回。
//     若兩字串長度不同,兩者皆「相同前綴」時也只需比短的那邊。
//
// 【注意事項 Pay Attention】
// 1. 比較是 unsigned 字元比較,不是「文字邏輯排序」(例如 "10" < "2",因為 '1' < '2')。
// 2. 比較不會處理 locale(例如重音字母順序)。需要 locale-aware 比較
//    請改用 std::collate 或 ICU。
// 3. 別用 == 比較 std::string 與 nullptr(會 UB / 編譯錯)。
// 4. 對於排序大量字串,std::sort 自動使用 < 運算子,效能合理。
// 5. 大小寫敏感: "Apple" != "apple"。要 case-insensitive 比較請自行 tolower 或用 ICU。
// 6. 中文/UTF-8: 比較是 byte-by-byte;UTF-8 byte-wise 比較通常與「Unicode 碼點順序」
//    一致 (UTF-8 設計如此),所以對相同編碼的字串通常還算合理。
// 7. C++20 spaceship 在 ABI 上是新的 symbol,連結舊編譯的物件檔可能出問題。
//
// 【概念補充 Concept Deep Dive】
//
// (A) C++20 spaceship 自動產生 6 個運算子 ── 以使用者自訂類別為例:
//
//         struct Version {
//             int major, minor, patch;
//             auto operator<=>(const Version&) const = default;  // 全部自動產生!
//         };
//         // 之後可直接使用 ==, !=, <, >, <=, >=
//
//     對 std::string 也同理 ── 標準庫 C++20 起就用這寫法。
//
// (B) Strong / Weak / Partial Ordering ── 三種「順序強度」:
//
//     【strong_ordering】 a == b 表示 a 與 b「在所有層面上等價」
//       - int 比較 (5 == 5,完全相同)
//       - std::string 比較 (字元序完全相同)
//       - 比較結果取值: less / equal / greater
//
//     【weak_ordering】 a == b 表示「在排序意義下相等」,但仍可能有差異
//       - case-insensitive 字串比較 ("Apple" 與 "apple" 排序上相同,但拼寫不同)
//       - 浮點數的「絕對值排序」 (5 與 -5 相等?)
//       - 比較結果取值: less / equivalent / greater
//
//     【partial_ordering】 兩元素可能無法比較
//       - float / double (NaN 與任何值都不可比)
//       - 偏序集 (集合的子集關係)
//       - 比較結果取值: less / equivalent / greater / unordered
//
// (C) 字典序 vs 數字序 ── 經典坑:
//     檔名排序時 "file10.txt" < "file2.txt",因為 '1' < '2'。
//     若要 natural sort (人類直覺),需自己實作或用第三方函式庫 (如 boost::natural_sort)。
//
// (D) 與 std::lexicographical_compare 的關係:
//     std::string::operator< 等同 std::lexicographical_compare(s1.begin(), s1.end(),
//                                                              s2.begin(), s2.end())。
//     對 vector、deque、其他 sequence 比較也適用同樣邏輯。
//
// (E) 用 == 比較字串的效能:
//     好的實作會先比 size() ── size() 不同直接 false,免除整段比對。
//     對 hash-based 容器,建議實作 hash 函式而非直接用 == 找重複。
//
// =============================================================================

/*
補充筆記：std::string::comparison_operators
  - std::string::comparison_operators 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::comparison_operators 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void demoCompare() {
    std::string a = "apple";
    std::string b = "banana";
    std::cout << std::boolalpha;
    std::cout << (a == b) << "\n";
    std::cout << (a <  b) << "\n";
    std::cout << (a >  b) << "\n";

    // C++20 三向比較
    auto r = a <=> b;
    std::cout << (r < 0 ? "a<b" : r > 0 ? "a>b" : "a==b") << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram
// 題目敘述:
//   給兩個字串 s, t,判斷 t 是否為 s 的「易位構詞 (anagram)」── 即由相同字元
//   重新排列而成。例如 "anagram" 與 "nagaram" 是 anagram。
// 為何用 operator==:
//   把兩字串各自排序後,若 anagram 它們會完全相同 ── 直接用 == 一行判斷。
//   先比 size() 可早退出 (== 內部其實也會做)。
// 解題思路:
//   1) 長度不同直接 false
//   2) std::sort 排序兩字串
//   3) operator== 一次比對
// 複雜度: O(N log N) (排序),空間 O(1) (in-place sort)。
// -----------------------------------------------------------------------------
bool isAnagram(std::string s, std::string t) {
    if (s.size() != t.size()) return false;
    std::sort(s.begin(), s.end());
    std::sort(t.begin(), t.end());
    return s == t;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 49. Group Anagrams
// 題目: 把易位構詞分群。
// 為何用 operator<: std::map 用 < 排序,我們用排序後的字串作 key。
// -----------------------------------------------------------------------------
#include <map>
std::vector<std::vector<std::string>> groupAnagrams(const std::vector<std::string>& strs) {
    std::map<std::string, std::vector<std::string>> mp;
    for (const auto& s : strs) {
        std::string key = s;
        std::sort(key.begin(), key.end());
        mp[key].push_back(s);
    }
    std::vector<std::vector<std::string>> res;
    for (auto& [k, v] : mp) res.push_back(std::move(v));
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依字串排序的 leaderboard / 排行榜
// 為何用 operator<: std::sort 用 < 排序 string-key 元素。
//                   遊戲名次、字典順序、報表輸出都會這樣做。
// -----------------------------------------------------------------------------
struct Score { std::string player; int points; };

void sortByPlayerName(std::vector<Score>& scores) {
    std::sort(scores.begin(), scores.end(),
              [](const Score& a, const Score& b) {
                  return a.player < b.player;
              });
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1370. Increasing Decreasing String
// 題目: 反覆地:取最小未取字元 → 大於上次的最小 → ...,再反向。
// 為何用 ==: 我們會把字元 push 到結果並用 == 檢查是否與上次取的字元相同 (避免重複)。
//            主要是教學「兩個 char 用 == 比較」的搭配 string 構建。
// 解法: 用桶排 (count[26]) 模擬。
// 複雜度: O(N)。
// 難度: easy
// -----------------------------------------------------------------------------
std::string sortString(const std::string& s) {
    int cnt[26] = {0};
    for (char c : s) ++cnt[c - 'a'];
    std::string out;
    out.reserve(s.size());
    while (out.size() < s.size()) {
        for (int i = 0; i < 26; ++i) if (cnt[i] > 0) { out.push_back('a' + i); --cnt[i]; }
        for (int i = 25; i >= 0; --i) if (cnt[i] > 0) { out.push_back('a' + i); --cnt[i]; }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】比對 SemVer 中的 minor / major 是否一致
// 為何用 ==: 直接 string == 一行解,比 strcmp 安全且不需 strlen。
// 場景: 套件管理工具判斷 major.minor 是否符合相容性要求。
// -----------------------------------------------------------------------------
bool sameMajorMinor(const std::string& a, const std::string& b) {
    auto pick = [](const std::string& v) -> std::string {
        size_t p1 = v.find('.');
        if (p1 == std::string::npos) return v;
        size_t p2 = v.find('.', p1 + 1);
        return v.substr(0, p2);
    };
    return pick(a) == pick(b);
}

int main() {
    demoCompare();

    std::cout << "\n=== LeetCode 242 ===\n";
    std::cout << std::boolalpha << isAnagram("anagram", "nagaram") << "\n"; // true
    std::cout << std::boolalpha << isAnagram("rat",     "car")     << "\n"; // false

    std::cout << "\n=== LeetCode 49 ===\n";
    auto groups = groupAnagrams({"eat","tea","tan","ate","nat","bat"});
    for (auto& g : groups) {
        for (auto& s : g) std::cout << s << " ";
        std::cout << "\n";
    }

    std::cout << "\n=== LeetCode 1370 ===\n";
    std::cout << sortString("aaaabbbbcccc") << "\n";   // abccbaabccba
    std::cout << sortString("leetcode")     << "\n";   // cdelotee

    std::cout << "\n=== 日常實務: 依玩家名排序 ===\n";
    std::vector<Score> ss{{"Charlie", 30}, {"Alice", 50}, {"Bob", 40}};
    sortByPlayerName(ss);
    for (auto& s : ss) std::cout << s.player << ":" << s.points << "\n";

    std::cout << "\n=== 日常實務: sameMajorMinor ===\n";
    std::cout << sameMajorMinor("1.2.3", "1.2.4") << "\n";   // true
    std::cout << sameMajorMinor("1.2.3", "1.3.0") << "\n";   // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：C++20 的 spaceship operator (<=>) 對 std::string 帶來什麼改變?
    //    A：標準改為只實作 operator== 與 operator<=>,其餘 4 個運算子
    //       (!= < > <= >=) 由編譯器從 <=> 自動合成 (rewritten candidate)。
    //       字串的 <=> 回傳 std::strong_ordering。好處:標準庫不再需要寫
    //       6 個獨立 operator,且保證一致性。
    //
    //  Q2：operator== 與 operator< 在效能上有差別嗎?
    //    A：有!operator== 可以先比 size():長度不同直接 false,O(1) 早退;
    //       長度相同才走 memcmp,O(N)。operator< / <=> 則必須逐字比對才能
    //       決定順序,沒有早退捷徑。所以「只想知道是否相等」永遠用 ==,
    //       不要用 !(a < b || b < a)。
    //
    //  Q3：std::string 與 const char* 比較會建構暫時 string 嗎?
    //    A：不會。標準提供 string vs const char* 的混合重載,直接走 strlen
    //       + memcmp 路徑,不配置記憶體。所以 if (s == "foo") 是零成本的
    //       常見寫法。但要避開 (char*)nullptr 比較 ── 那是 UB (走 strlen
    //       會 SEGV)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra comparison_operators.cpp -o comparison_operators

// === 預期輸出 (節錄) ===
// === LeetCode 1370 ===
// abccbaabccba
// cdelotee
// === 日常實務: sameMajorMinor ===
// true
// false
