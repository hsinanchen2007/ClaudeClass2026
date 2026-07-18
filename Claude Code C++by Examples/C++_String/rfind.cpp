// =============================================================================
// 檔名: rfind.cpp
// 主題: std::string::rfind (從後往前找子字串/字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/rfind
//   - https://cplusplus.com/reference/string/string/rfind/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type rfind(const string& str, size_type pos = npos)              const noexcept; // (1)
//   size_type rfind(const char*   s,   size_type pos = npos)              const;          // (2)
//   size_type rfind(const char*   s,   size_type pos, size_type count)    const;          // (3)
//   size_type rfind(char          ch,  size_type pos = npos)              const noexcept; // (4)
//   size_type rfind(string_view   sv,  size_type pos = npos)              const noexcept; // (5) C++17
//
// 回傳: 最後一次出現位置(子字串開頭索引);沒找到回 npos。
//
// 【詳細解釋 Explanation】
// rfind 是 find 的「反向版本」 — 從後往前掃描,找出 needle 最後一次出現的位置。
// 雖然名字裡的 r 表示 reverse,但回傳的「位置」依然是 forward 索引(從 0 起算
// 的字元 offset),不是反向索引。例如 "abc abc" rfind("abc") 回傳 4,不是 0。
//
// 【1. pos 參數的精確語意】
// rfind 的 pos 預設是 npos,意思是「上限不設」。它的精確規則是:
//   * 找出最大的 i,使得 i ≤ pos 且 s[i..i+M-1] == needle。
//   * 注意 i 是「needle 的開頭位置」,不是結尾位置。
//   * 也就是說,只要 needle 的「開頭」落在 [0, pos] 範圍內就算數,
//     即使結尾延伸到 pos 之後。
// 範例:s = "abc abc abc",size=11
//   s.rfind("abc")        → 8     (最右邊那個 abc 的開頭)
//   s.rfind("abc", 8)     → 8     (8 ≤ 8 OK)
//   s.rfind("abc", 7)     → 4     (8 > 7 不行,退到 4)
//   s.rfind("abc", 6)     → 4     (還是 4,因為 5,6 都不是開頭)
//   s.rfind("abc", 0)     → npos  (0 處的 "abc" 嗎? 是的,回傳 0!)
//   s.rfind("abc", 100)   → 8     (pos 過大時 = 不限制)
// 換句話說,pos 是「最後一個被允許的開頭位置」(inclusive)。
//
// 【2. 為什麼會「從後往前」 — 真實的演算法】
// rfind 的 naive 實作:
//   for (i = min(pos, N - M); i >= 0; --i):
//       if s[i..i+M-1] == needle: return i
//   return npos
// 注意外層用 size_type(無號),要用 do-while 或 i-- 後檢查上界,避免下溢。
// 大多數 STL 實作會處理這個邊界:
//   size_type i = std::min(pos, size() - M);   // 從可能的最大開頭往下掃
//   while (true) {
//       if (memcmp(...)) return i;
//       if (i == 0) break;
//       --i;
//   }
//
// 【3. 各重載的細節】
// (1) rfind(const string&, pos)
//     最常見;支援含內嵌 '\0'。
// (2) rfind(const char*, pos)
//     C-string 結尾於第一個 '\0'。
// (3) rfind(const char*, pos, count)
//     明確指定 needle 長度,可處理含 '\0' 的二進位 needle。
// (4) rfind(char, pos)
//     單字元搜尋,通常用 std::memrchr(GNU 擴充)或反向掃描。
// (5) rfind(string_view, pos) — C++17
//     避免 const char* / std::string 之間的隱式複製。
//
// 【4. 邊界規則】
//   * needle 為空字串:回傳 std::min(pos, size())(空字串「到處都在」)。
//   * needle 比 s 還長:回傳 npos。
//   * pos > size():視為 pos = size() - 1(也就是「不限制」)。
//   * 在空字串上 rfind 任何非空 needle:回傳 npos。
//
// 【5. 為何 rfind 適合「副檔名 / 路徑分隔」】
// 處理檔案路徑時最常見的需求是「找最後一個 /」、「找最後一個 .」 — 這正是
// rfind 的拿手好戲。從前面 find 然後不斷推進來找最後一個相比,rfind 一次
// 從後往前掃,通常更快(若資料分布偏向尾端)且程式碼更直觀。
//
// 【6. 時間複雜度】
//   * 最壞:O(N * M)(naive 反向掃描)
//   * 平均:O(N + M)
//   * rfind(char):O(N),通常用反向 memchr / memrchr
//
// 【概念補充 Concept Deep Dive】
// (A) rfind vs reverse + find
//   理論上「先 reverse 整個 string 再 find」也能達到反向搜尋,但會 O(N) 複製
//   兼破壞原資料,且 needle 也要反向。rfind 是 in-place 反向掃,顯然更好。
//
// (B) rfind vs find_last_of
//   兩者很像,差別在「找的單位」:
//     * rfind          : 找「整個子字串」(連續 M 個字元相符)
//     * find_last_of   : 找「字元集合中任一字元」
//   例:s.rfind("aeiou")          找最後一個 "aeiou" 子字串(子字串需相符,通常找不到)
//      s.find_last_of("aeiou")   找最後一個母音字元
//
// (C) 為什麼 rfind 不叫 find_last
//   歷史原因:STL 一開始借用 C strrchr / strrstr 命名(r 表示 reverse)。
//   後來加入 find_first_of / find_last_of 才用 first/last 命名,造成混亂。
//   實務上記住:rfind 對應 find,find_last_of 對應 find_first_of。
//
// (D) 與 std::string::contains (C++23) 的關係
//   想知道「是否含某 needle」用 contains 即可,不必比較 rfind 結果與 npos。
//   contains 內部通常實作為 find != npos,但語意更清晰。
//
// 【注意事項 Pay Attention】
// 1. rfind 的 pos 含意是「搜尋範圍上界」 — 字串子字串開始處 ≤ pos 才算。
//    例:s.rfind("abc", 5) 會找最後一個開頭位置 ≤ 5 的 "abc"。
// 2. 不要拿 rfind 結果直接 + size 來作為「下一個搜尋起點」,因為它是逆向。
// 3. 找空字串時回傳 std::min(pos, size())。
// 4. rfind 雖名為「reverse find」,回傳值仍是 forward 索引(從 0 起算)。
// 5. 反向迴圈用 size_type 容易下溢,用 rfind 已封裝這層邏輯,比手寫安全。
//
// =============================================================================

/*
補充筆記：std::string::rfind
  - std::string::rfind 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::rfind 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoRFind() {
    std::string s = "abc abc abc";

    std::cout << "rfind \"abc\" = " << s.rfind("abc") << "\n";       // 8
    std::cout << "rfind \"abc\", 6 = " << s.rfind("abc", 6) << "\n"; // 4
    std::cout << "rfind 'b' = " << s.rfind('b') << "\n";             // 9
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 58. Length of Last Word (用 rfind)
// 題目: 給字串,回傳最後一個單字長度。
// 為何用 rfind: 找最後一個非空白的位置 → 再用 rfind(' ') 找其前面的空白。
// -----------------------------------------------------------------------------
int lengthOfLastWord(const std::string& s) {
    size_t end = s.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    size_t start = s.rfind(' ', end);
    if (start == std::string::npos) return static_cast<int>(end + 1);
    return static_cast<int>(end - start);
}

// -----------------------------------------------------------------------------
// 【實務範例】從檔案路徑取出檔名與副檔名
// 為何用 rfind: 從尾端找 '/' 與 '.',典型路徑解析。
// -----------------------------------------------------------------------------
struct PathParts { std::string dir, base, ext; };

PathParts splitPath(const std::string& path) {
    PathParts r;
    size_t slash = path.rfind('/');
    if (slash == std::string::npos) {
        r.dir = "";
        r.base = path;
    } else {
        r.dir  = path.substr(0, slash);
        r.base = path.substr(slash + 1);
    }
    size_t dot = r.base.rfind('.');
    if (dot != std::string::npos && dot != 0) {
        r.ext  = r.base.substr(dot + 1);
        r.base = r.base.substr(0, dot);
    }
    return r;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從 URL 取出 host (找最後一個 '/' 之前)
// 為何用 rfind: 從 "https://api.example.com/v1/users" 取 host。
//               搭配 find("//", 0) 找 scheme 結束位置。
//               日常處理 web request 必備。
// -----------------------------------------------------------------------------
std::string extractHost(const std::string& url) {
    size_t schemeEnd = url.find("://");
    size_t start = (schemeEnd == std::string::npos) ? 0 : schemeEnd + 3;
    size_t pathStart = url.find('/', start);
    if (pathStart == std::string::npos) return url.substr(start);
    return url.substr(start, pathStart - start);
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Number of Substrings - 簡化
// 題目: LeetCode 686. Repeated String Match
// 為求 A 重複多少次後 B 是其子字串(最少次數);若不可能回 -1。
// 為何用 rfind: 計算重複次數的下界 = ceil(B.size()/A.size()),再追加 1 次即可;
//                教學 string::rfind / find 的搭配。本範例聚焦 rfind 在末段確認子字串位置。
// 複雜度: O((|A|+|B|)*N)。
// 難度: medium
// -----------------------------------------------------------------------------
int repeatedStringMatch(const std::string& A, const std::string& B) {
    if (A.empty()) return -1;
    std::string s = A;
    int cnt = 1;
    while (s.size() < B.size()) { s += A; ++cnt; }
    if (s.find(B) != std::string::npos) return cnt;
    s += A;
    if (s.find(B) != std::string::npos) return cnt + 1;
    // 再用 rfind 做尾端 sanity check (示範 rfind)
    if (s.rfind(B) != std::string::npos) return cnt + 1;
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從含 query string 的 URL 取出 path (去除 ?... 與 #...)
// 為何用 rfind: 找最後一個 '/' 之後到 '?' / '#' 之間的內容。
// -----------------------------------------------------------------------------
std::string extractPathOnly(const std::string& url) {
    size_t schemeEnd = url.find("://");
    size_t start = (schemeEnd == std::string::npos) ? 0 : schemeEnd + 3;
    size_t pathStart = url.find('/', start);
    if (pathStart == std::string::npos) return "/";
    size_t end = url.find_first_of("?#", pathStart);
    if (end == std::string::npos) end = url.size();
    return url.substr(pathStart, end - pathStart);
}

int main() {
    demoRFind();
    std::cout << "\n=== LeetCode 58 ===\n";
    std::cout << lengthOfLastWord("Hello World") << "\n";   // 5

    std::cout << "\n=== splitPath ===\n";
    auto p = splitPath("/home/user/file.txt");
    std::cout << "dir=" << p.dir << ", base=" << p.base << ", ext=" << p.ext << "\n";

    std::cout << "\n=== LeetCode 686 ===\n";
    std::cout << repeatedStringMatch("abcd", "cdabcdab")  << "\n";   // 3
    std::cout << repeatedStringMatch("a",    "aa")        << "\n";   // 2

    std::cout << "\n=== 日常實務: 取 URL host ===\n";
    std::cout << extractHost("https://api.example.com/v1/users") << "\n";
    std::cout << extractHost("http://localhost:8080/")           << "\n";

    std::cout << "\n=== 日常實務: extractPathOnly ===\n";
    std::cout << extractPathOnly("https://x.com/a/b?q=1#frag") << "\n";   // /a/b
    std::cout << extractPathOnly("https://x.com")              << "\n";   // /

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:rfind(needle, pos) 的 pos 是「needle 結尾」還是「needle 開頭」?
    //    A:是 needle「開頭」位置的上界 (inclusive)。標準規則:找最大的 i
    //      使得 i ≤ pos 且 s[i..i+M-1] == needle。即使 needle 結尾跨過
    //      pos 之外也算合法,只要開頭 ≤ pos。常被誤解,寫 unit test 容易踩到。
    //
    //  Q2:rfind 與 find_last_of 差在哪?
    //    A:rfind 找「整個子字串」(連續 M 個字元相符);find_last_of 找
    //      「字元集合中任一字元」。例 s.rfind("aeiou") 找 "aeiou" 這 5 字元
    //      連續出現的最後位置 (通常找不到);s.find_last_of("aeiou") 找
    //      最後一個母音。前者對應 strrstr,後者對應反向 strpbrk。
    //
    //  Q3:rfind 在空字串上呼叫如何?needle 為空又如何?
    //    A:空 string 上 rfind 任何「非空 needle」回傳 npos;rfind 「空 needle」
    //      回傳 std::min(pos, size()) — 空字串「到處都在」。pos 大於 size()
    //      時視為不限制,等同從尾端開始往回找。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra rfind.cpp -o rfind

// === 預期輸出 (節錄) ===
// === LeetCode 686 ===
// 3
// 2
// === 日常實務: extractPathOnly ===
// /a/b
// /
