// =============================================================================
// 檔名: compare.cpp
// 主題: std::string::compare (字典序比較)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/compare
//   - https://cplusplus.com/reference/string/string/compare/
// =============================================================================
//
// 【函式資訊 Information - 主要重載】
//   int compare(const string& str) const noexcept;
//   int compare(size_type pos1, size_type count1, const string& str) const;
//   int compare(size_type pos1, size_type count1,
//               const string& str, size_type pos2, size_type count2 = npos) const;
//   int compare(const char* s) const;
//   int compare(size_type pos1, size_type count1, const char* s) const;
//   int compare(size_type pos1, size_type count1,
//               const char* s, size_type count2) const;
//   // C++17 起加上 string_view 重載
//
// 回傳 (Return value):
//   負值 (< 0) : *this 字典序在 str 之前 (this < str)
//   零   (= 0) : 兩者完全相等 (this == str)
//   正值 (> 0) : *this 字典序在 str 之後 (this > str)
//
// 【詳細解釋 Explanation】
//
// 一、底層運作
//   compare 內部會呼叫 std::char_traits<char>::compare,效果等同
//   memcmp 的「逐 unsigned char 比較」。注意是 unsigned ── 即使你的 char
//   是有號型別,比較時也會視為 0~255 的整數。
//
//   流程:
//     1. 逐字元比較直到「找到第一個不同字元」或「其中一方用完」。
//     2. 若找到不同字元 → 回傳該位置兩字元的差 (a[i] - b[i] 視為 unsigned char)。
//     3. 若全部前綴相同,長度短者較小;回傳 (len_a - len_b)。
//   實作通常先 memcmp(min(N1,N2)) 再判斷長度,效能極佳。
//
// 二、為何回傳 int 而不是 bool?
//   一個 bool 只能告訴你「相不相等」或「a 是否小於 b」,排序需求要呼叫兩次。
//   回傳 int (三態) 一次就能分辨 <, ==, > 三種情況,
//   這是 C 時代 strcmp/memcmp 的傳統,也是 sorted container 的核心介面 ──
//   排序演算法可少呼叫一次比較器,大幅提升效能。
//
//   注意:回傳值「只」保證「正/負/零」這三類,別當成「實際差距」!
//   實作可能回傳 1 / -1,也可能回傳 32 / -32 (字元差)。
//
// 三、與 operator<、operator==、operator<=> 的關係
//   - operator==    : 用 char_traits::eq + 長度檢查,專門做相等性。最快。
//   - operator<     : 內部就是 compare() < 0 (C++20 之前由非成員函式提供)。
//   - operator<=>   : C++20 三向比較 (spaceship);回傳 std::strong_ordering,
//                     可同時得到 <、=、> 結果,語意上與 compare 等價但型別更安全。
//   - 比較字元數值大小寫敏感;若要不分大小寫請自己寫 (見下方實務範例)。
//
// 四、子字串比較重載的優勢
//   想比較 a 的某個 substring 與 b 時,有兩種寫法:
//     a.substr(pos, n) == b           // 會配置新字串、拷貝資料
//     a.compare(pos, n, b) == 0       // 直接在原字串比較,零配置
//   後者明顯較佳,尤其在 hot path 或處理大字串時。
//
// 五、版本演進
//   - C++11 : 加上 noexcept (對 const string& 重載)。
//   - C++17 : 加入 string_view 重載。
//   - C++20 : operator<=> 出現,提供類型安全的三向比較。
//   - C++20 : constexpr。
//
// 時間複雜度: O(min(N1, N2));若長度不同會多一次長度比較,仍是 O(min(N1,N2))。
//
// 【概念補充 Concept Deep Dive】
//
// (A) Lexicographic Ordering (字典序排序)
//   定義:逐位置比較,第一個不同的位置決定大小;若一方是另一方的「真前綴」,
//   短者較小。例如:"app" < "apple" < "banana"。
//   注意這不是「自然數字排序」── "10" < "2" (因為 '1' < '2'),
//   要做數字排序需先轉成 int 比較。
//
// (B) char vs unsigned char 的陷阱
//   C++ 標準規定 compare 把字元視為 unsigned 處理。為什麼?
//   非 ASCII 字元 (例如中文 UTF-8 的 byte 值 0x80~0xFF) 在有號 char 下會是負數,
//   若用「有號比較」,中文字會排在 ASCII 之前 ── 違反直覺。
//   因此標準強制用 unsigned char 比較,確保字典序與「byte 值」一致。
//
// (C) C++20 三向比較 (Spaceship Operator)
//   auto r = s1 <=> s2;
//   r 是 std::strong_ordering,可以與 0 比較:
//     if (r < 0)  ...   if (r == 0) ...   if (r > 0) ...
//   優點:
//     1. 型別安全 (不會被當成 bool 或 int 隨意運算)。
//     2. 編譯器可一次比較,自動產生 <、<=、>、>= 全部運算子。
//   推薦新程式碼優先使用 <=>,compare 留給需要「(pos, count) 子字串」時用。
//
// (D) 為何不直接用 std::lexicographical_compare?
//   <algorithm> 中的 lexicographical_compare 是泛型版本,效能比 compare 慢
//   (沒有針對連續記憶體做 memcmp 優化)。string::compare 是專為字串設計的快取友善版本。
//
// 【注意事項 Pay Attention】
// 1. 不要用 strcmp 比較 std::string;有更安全的 operator==、operator<=>。
// 2. compare 回傳 int,但僅保證「正/負/零」三類,別假設它是字元差。
// 3. C++20 起可用 三向比較 operator<=>:
//      auto r = s1 <=> s2;  // r 是 std::strong_ordering
// 4. 若只需相等性比較,operator== / != 比 compare()==0 略快,且更直觀。
// 5. 在 substring 比較時,(pos, count) 變種比先 substr 再比較快(避免拷貝)。
// 6. compare 是「位元組級」比較,不處理 Unicode collation;中文/排序語系需 ICU。
// 7. 大小寫敏感;不分大小寫請手寫 (tolower) 或用 Boost iequals。
//
// =============================================================================

/*
補充筆記：std::string::compare
  - std::string::compare 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::compare 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoCompare() {
    std::string a = "apple";
    std::string b = "banana";
    std::string c = "apple";

    std::cout << "a.compare(b) = " << a.compare(b) << "\n";  // 負
    std::cout << "b.compare(a) = " << b.compare(a) << "\n";  // 正
    std::cout << "a.compare(c) = " << a.compare(c) << "\n";  // 0

    // 子字串比較: 比較 a 的 [0,3) 與 "app"
    std::cout << "a.compare(0,3,\"app\") = " << a.compare(0, 3, "app") << "\n";

    // 兩端都子字串
    std::cout << "a.compare(0,2, b, 0, 2) = " << a.compare(0, 2, b, 0, 2) << "\n";

    // 比 "==" 慢一點點但更靈活
    if (a == c) std::cout << "a == c\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 242. Valid Anagram
// 題目: 判斷字串 t 是否為 s 的字母重排 (anagram)。
// 為何用 compare: 把兩字串排序後直接 compare;若回傳 0 即為 anagram。
//                 示範 compare 用於相等性判斷的常見組合 (sort + compare)。
// 思路: 先排序兩字串,然後 compare;O(N log N) 時間。
//       (更快的解法是用 26 個字元計數陣列 O(N),但這裡示範 compare 用法。)
// -----------------------------------------------------------------------------
#include <algorithm>
bool isAnagram(std::string s, std::string t) {
    if (s.size() != t.size()) return false;
    std::sort(s.begin(), s.end());
    std::sort(t.begin(), t.end());
    return s.compare(t) == 0;       // 排序後相等 → anagram
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 165. Compare Version Numbers
// 題目: 比較版本字串 "1.01" 與 "1.001";相等回 0,前者大回 1,後者大回 -1。
// 為何用 compare: 雖然這題拆 token 後用 stoi + 數值比較較簡單,
//                 但我們示範也可以用 compare 對齊長度後字典序比較。
//                 主要重點是熟悉 compare 的子字串重載。
// 複雜度: O(N1 + N2)。
// -----------------------------------------------------------------------------
#include <vector>
#include <sstream>
int compareVersion(const std::string& v1, const std::string& v2) {
    auto split = [](const std::string& s) {
        std::vector<std::string> r;
        std::stringstream ss(s);
        std::string tok;
        while (std::getline(ss, tok, '.')) r.push_back(tok);
        return r;
    };
    auto a = split(v1);
    auto b = split(v2);
    size_t n = std::max(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        int x = (i < a.size() ? std::stoi(a[i]) : 0);
        int y = (i < b.size() ? std::stoi(b[i]) : 0);
        if (x < y) return -1;
        if (x > y) return  1;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】不分大小寫比較 (HTTP header / 副檔名)
// 為何用 compare: HTTP header 名稱不分大小寫 (Content-Type vs content-type)。
//                 副檔名也類似 (.PNG vs .png)。日常解析請求一定會碰到。
// -----------------------------------------------------------------------------
#include <cctype>
bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 459. Repeated Substring Pattern
// 題目: 判斷 s 是否可由它的某個子字串重複組成 (子字串長度 < s.size())。
// 為何用 compare: 對候選長度 len (s.size() 的因數),驗證 s.compare(0, len, s, k*len, len)
//                 是否對所有 k 都為 0。零配置零拷貝的 substring 比對。
// 思路: 列舉 len = 1..s.size()/2;只考慮 size 是 len 的倍數;比對所有區段。
// 複雜度: 最壞 O(N^1.5)。
// 難度: easy
// -----------------------------------------------------------------------------
bool repeatedSubstringPattern(const std::string& s) {
    size_t n = s.size();
    for (size_t len = 1; len * 2 <= n; ++len) {
        if (n % len != 0) continue;
        bool ok = true;
        for (size_t k = 1; k < n / len && ok; ++k) {
            if (s.compare(0, len, s, k * len, len) != 0) ok = false;
        }
        if (ok) return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】檢查 path 是否屬於某個 base 目錄 (path traversal 防護)
// 為何用 compare: 用 compare(0, base.size(), base) 比對前綴是否為 base,零拷貝。
//                 防止 "/app/../etc/passwd" 之類的攻擊。
// -----------------------------------------------------------------------------
bool isUnderBaseDir(const std::string& path, const std::string& base) {
    if (path.size() < base.size()) return false;
    return path.compare(0, base.size(), base) == 0;
}

int main() {
    demoCompare();

    std::cout << "\n=== LeetCode 242 ===\n";
    std::cout << std::boolalpha
              << isAnagram("anagram", "nagaram") << "\n"   // true
              << isAnagram("rat",     "car")     << "\n";  // false

    std::cout << "\n=== LeetCode 165 ===\n";
    std::cout << compareVersion("1.01", "1.001") << "\n";  // 0
    std::cout << compareVersion("1.0",  "1.0.0") << "\n";  // 0
    std::cout << compareVersion("0.1",  "1.1")   << "\n";  // -1

    std::cout << "\n=== LeetCode 459 ===\n";
    std::cout << repeatedSubstringPattern("abab")    << "\n";   // true
    std::cout << repeatedSubstringPattern("aba")     << "\n";   // false
    std::cout << repeatedSubstringPattern("abcabcabcabc") << "\n";   // true

    std::cout << "\n=== 日常實務: 不分大小寫比較 ===\n";
    std::cout << std::boolalpha
              << equalsIgnoreCase("Content-Type", "content-type") << "\n"  // true
              << equalsIgnoreCase(".PNG",         ".png")         << "\n"  // true
              << equalsIgnoreCase("foo",          "bar")          << "\n"; // false

    std::cout << "\n=== 日常實務: 路徑越界檢查 ===\n";
    std::cout << isUnderBaseDir("/app/data/log.txt", "/app/") << "\n";   // true
    std::cout << isUnderBaseDir("/etc/passwd",       "/app/") << "\n";   // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：compare 回傳的「正負/零」可以當作字元差距使用嗎?
    //    A：不可。標準只保證符號 (sign):負代表 this < other,零代表相等,
    //       正代表 this > other。實作可能回傳 1 / -1,也可能回傳實際字元差
    //       (例如 'A' - 'a' = -32)。永遠只用 < 0、== 0、> 0 判斷,別寫
    //       compare(s) == -1。
    //
    //  Q2：a.substr(p, n) == b 與 a.compare(p, n, b) == 0 哪個好?
    //    A：後者好太多!substr 會配置新的 std::string + 一次資料拷貝,完全
    //       多餘。a.compare(p, n, b) 直接在原 buffer 比對,零配置零拷貝,
    //       hot path 或大字串時差距明顯。starts_with / ends_with 內部也是
    //       走 compare 路徑。
    //
    //  Q3：compare 是按字典序還是數值比?中文字怎麼比?
    //    A：純字典序,逐 byte 視為 unsigned char 比較 (即使 char 是 signed)。
    //       對 UTF-8 文字會比「位元組順序」,剛好等同 Unicode code point 順序
    //       (UTF-8 的設計巧思)。但若要文化敏感的排序 (e.g. 中文筆畫、注音),
    //       必須用 std::collate 或 ICU,compare 不夠用。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra compare.cpp -o compare

// === 預期輸出 (節錄) ===
// === LeetCode 459 ===
// true
// false
// true
// === 日常實務: 路徑越界檢查 ===
// true
// false
