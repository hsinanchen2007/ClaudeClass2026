// =============================================================================
// 檔名: find.cpp
// 主題: std::string::find (從前往後找子字串/字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/find
//   - https://cplusplus.com/reference/string/string/find/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type find(const string& str, size_type pos = 0)               const noexcept; // (1)
//   size_type find(const char*   s,   size_type pos = 0)               const;          // (2)
//   size_type find(const char*   s,   size_type pos, size_type count)  const;          // (3)
//   size_type find(char          ch,  size_type pos = 0)               const noexcept; // (4)
//   size_type find(string_view   sv,  size_type pos = 0)               const noexcept; // (5) C++17
//
// 回傳: 第一個出現位置的 index;沒找到則回傳 std::string::npos。
//
// 【詳細解釋 Explanation】
// find 是 std::string 中最常用、最容易理解的搜尋函式 — 「從 pos 起往右掃,找出
// 子字串(或字元)第一次出現的位置」。但它的細節遠比表面複雜,涉及 npos 的設計、
// size_t 的選擇、字串搜尋演算法的取捨,以及 STL 的歷史包袱。
//
// 【1. npos:為什麼用 size_t(-1) 而不是 -1】
// std::string 的所有「位置」型別是 size_type(通常 alias 自 size_t,即無號型別)。
// 為了表示「沒找到」,STL 採用「最大值」當哨兵值:
//   static constexpr size_type npos = static_cast<size_type>(-1);
// 在 64-bit 系統上,這個值是 18446744073709551615。為什麼不用 ssize_t 並回傳 -1?
//   * 一致性:string 的 size、index、pos 全用 size_type,回傳值也用同型別。
//   * 歷史:STL 在 C++98 設計時,signed/unsigned 的 idiom 比現在保守,
//          所有「容量類別」一律 unsigned。
//   * 效能:無號比較較單純(不需考慮負數),CPU 上稍快一點點。
// 但這帶來了知名陷阱:
//   if (s.find("x") != -1) ...        // 在某些編譯器下會警告 signed/unsigned 比較
//   if (s.find("x") >= 0) ...         // 永遠成立!size_type 不可能 < 0
// 標準寫法只有一種:
//   if (s.find("x") != std::string::npos) { ... }
//
// 【2. 演算法:naive、KMP、Boyer-Moore 的取捨】
// 標準 並未強制 規定 find 的演算法,只規定行為與複雜度上界(O(N*M) 是允許的)。
// 大多數實作使用 naive 算法:
//     for (i in [pos, N - M]):
//         if memcmp(haystack + i, needle, M) == 0: return i
//     return npos
// naive 的最壞 O(N*M),平均接近 O(N) — 對短字串非常快(cache-friendly,
// memcmp 還能用 SIMD)。
//
// 為什麼 STL 不直接用 KMP 或 Boyer-Moore?
//   * KMP 需要先對 needle 做 O(M) 預處理建立 failure table,對 needle 較短
//     (常見情況)反而變慢。
//   * Boyer-Moore 需要 O(M + Σ) 預處理(Σ 是字元集大小),對 short string 不划算。
//   * 對極長 haystack + 極長 needle,的確 KMP/BM 更快,但這種情境少見。
// C++17 起提供 std::search + std::boyer_moore_searcher / boyer_moore_horspool_searcher
// 讓使用者「自選」演算法:
//     auto it = std::search(s.begin(), s.end(),
//                           std::boyer_moore_searcher(p.begin(), p.end()));
//
// 【3. 各重載的細節】
// (1) find(const string&, pos)
//     傳 string 物件,大小由其 size() 決定;支援含內嵌 '\0' 的字串。
// (2) find(const char*, pos)
//     傳 C-string,結束於第一個 '\0'。若 needle 含 '\0' 會被截斷,要小心。
// (3) find(const char*, pos, count)
//     明確指定 needle 長度,可搜尋含內嵌 '\0' 的二進位 needle。
// (4) find(char, pos)
//     單字元搜尋,實作時通常退化為 std::memchr(很快)。
// (5) find(string_view, pos) — C++17
//     避免額外複製;傳 const char* 或 std::string 都會隱式轉成 string_view。
//
// 【4. pos 與邊界規則】
//   * pos 起點包含;比對範圍是 [pos, size())。
//   * pos > size():直接回傳 npos(不丟例外)。
//   * pos == size() 且 needle 為空:回傳 pos(這是「空字串到處都在」的規範)。
//   * needle 為空字串:永遠回傳 pos(若 pos ≤ size())。
//
// 【5. 時間複雜度】
//   * 最壞:O(N * M)(naive 實作)
//   * 平均:O(N)(典型 needle 與隨機 haystack)
//   * 單字元 find(char):O(N),通常用 memchr 加速
//
// 【6. find vs C-style strstr / memmem】
//   * strstr  : 只能處理 NUL-terminated 字串,無法搜尋含 '\0' 的二進位。
//   * memmem  : POSIX 擴充(非標準 C),可處理二進位資料,有時用 KMP/BM。
//   * std::string::find: 由 STL 實作決定演算法;支援內嵌 '\0';跨平台。
//
// 【概念補充 Concept Deep Dive】
// (A) 為何用 size_type 而不是 ssize_t
//   size_type 反映「容器大小」這個概念,本質上是非負的。STL 採用「同型別 +
//   sentinel value」的模式,代價是必須記得 npos 慣用法。Google C++ Style
//   Guide 與一些近代專案(LLVM)其實偏好 signed index,因為易讀且不會掉進
//   unsigned underflow 的坑(例如 size() - 1 在 size 為 0 時是天文數字)。
//
// (B) KMP vs Boyer-Moore vs naive 的複雜度比較
//   * naive (memcmp loop):
//     - 預處理:O(0)
//     - 搜尋:O(N*M) 最壞,但常數小,SIMD 友善
//   * KMP (Knuth-Morris-Pratt):
//     - 預處理:O(M)
//     - 搜尋:O(N + M),最壞也是 O(N + M)
//     - 適合 needle 中有大量重複 pattern 的情境
//   * Boyer-Moore:
//     - 預處理:O(M + Σ),Σ 為字元集大小
//     - 搜尋:O(N/M) 平均(可跳躍多個位置),最壞 O(N*M)
//     - 適合 needle 長且字元集大(如英文文件)
//   * Rabin-Karp:
//     - 用 hash 比對,適合「同時找多個 needle」
//
// (C) 為何 STL 沒用 KMP 作預設
//   1) 大多數 needle 短(< 16 char),naive + memcmp 已快到 cache-friendly。
//   2) KMP 需要在 heap 配置 failure table,對「一次性」搜尋反而 overhead。
//   3) STL 提供 std::boyer_moore_searcher,讓真的需要的人自選。
//   4) 標準避免「強制最佳」 — 不同實作可在 ABI 穩定下優化。
//
// (D) string_view 的影響
//   C++17 之後,find 接受 string_view 重載讓「傳 const char*、std::string、
//   string_view」都不會多複製,效能與彈性都好。建議新程式碼盡量用 string_view 重載。
//
// 【注意事項 Pay Attention】
// 1. 比較不存在時請用 == npos:
//      if (s.find("X") != std::string::npos) { ... }
// 2. pos 起點;pos > size() 直接回傳 npos (不會丟例外)。
// 3. 找空字串永遠回傳 pos (即起點)。
// 4. find vs strstr:find 支援含內嵌 '\0' 的搜尋(用 (s, pos, count) 重載)。
// 5. 大量重複搜尋同一個 needle 用 KMP / Boyer-Moore 自寫會更快;
//    或用 std::search + std::boyer_moore_searcher (C++17)。
// 6. 在迴圈內反覆 s.find(...) 並 erase/insert 的 pattern,要記得每次更新 pos,
//    否則可能無窮迴圈或漏掉位置。
//
// =============================================================================

/*
補充筆記：std::string::find
  - find 找不到時回傳 npos，不是 -1；npos 是 size_type 的最大值。
  - 使用 find 結果做 substr 前一定要先判斷 npos，否則位置計算會溢位。
  - 搜尋單一字元、字串、或從指定 pos 開始，成本都和被搜尋長度相關。
  - std::string::find 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoFind() {
    std::string s = "Hello, World!";

    auto p1 = s.find("World");
    std::cout << "find World = " << p1 << "\n";  // 7
    auto p2 = s.find('o', 5);                    // 從 pos=5 開始找
    std::cout << "find 'o' from 5 = " << p2 << "\n";  // 8
    auto p3 = s.find("xyz");
    std::cout << "find xyz = " << (p3 == std::string::npos ? "npos" : std::to_string(p3)) << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 28. Find the Index of the First Occurrence
// 題目: 在 haystack 找 needle 第一個出現位置;沒找到回 -1。
// 為何用 find: 一行解。
// -----------------------------------------------------------------------------
int strStr(const std::string& haystack, const std::string& needle) {
    auto p = haystack.find(needle);
    return p == std::string::npos ? -1 : static_cast<int>(p);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 387. First Unique Character in a String
// 題目: 找第一個只出現一次的字元位置。
// 為何用 find: 用 find + rfind 確認某字元是否只出現一次(p == rfind(c)). 但
//              這做法 O(N^2);這裡示範完備寫法,不是最優。
// -----------------------------------------------------------------------------
int firstUniqCharByFind(const std::string& s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s.find(s[i]) == i && s.rfind(s[i]) == i) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 1768 之延伸 — 移除子字串所有出現
// 題目: LeetCode 1910. Remove All Occurrences of a Substring
// 反覆把 part 從 s 中移除,直到不再出現。
// 為何用 find: 標準的「找到 → erase → 再找」流程。
// -----------------------------------------------------------------------------
std::string removeOccurrences(std::string s, const std::string& part) {
    size_t p;
    while ((p = s.find(part)) != std::string::npos) {
        s.erase(p, part.size());
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 HTTP Content-Type 取出 charset
// 為何用 find: 從 "text/html; charset=utf-8" 之類的 header 取出 charset 值。
//               日常 HTTP 解析、HTML 內容處理一定會碰到。
// -----------------------------------------------------------------------------
std::string extractCharset(const std::string& contentType) {
    const std::string key = "charset=";
    size_t p = contentType.find(key);
    if (p == std::string::npos) return "";
    p += key.size();
    size_t end = contentType.find_first_of("; \t", p);
    if (end == std::string::npos) end = contentType.size();
    return contentType.substr(p, end - p);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 4】LeetCode 771. Jewels and Stones
// 題目: jewels 內含哪些字元算珠寶,計算 stones 內有多少珠寶字元。
// 為何用 find: 對 stones 每個字元用 jewels.find(c),命中 (!= npos) 計數+1。
//              示範 find 找「單一字元」的用法,直觀又簡單。
// 複雜度: O(|stones| * |jewels|)。
// -----------------------------------------------------------------------------
int numJewelsInStones(const std::string& jewels, const std::string& stones) {
    int count = 0;
    for (char c : stones) {
        if (jewels.find(c) != std::string::npos) ++count;
    }
    return count;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 log 行抽取 [ERROR]/[WARN] 等等級標籤
// 為何用 find: log 通常格式 "2024-05-18 10:00:00 [ERROR] something happened"。
//              用 find 找 '[' 與 ']',再取中間就是 level。
// 場景: 監控系統、log 過濾、警報觸發都常需要先抽 level。
// -----------------------------------------------------------------------------
std::string extractLogLevel(const std::string& line) {
    size_t l = line.find('[');
    if (l == std::string::npos) return "";
    size_t r = line.find(']', l + 1);
    if (r == std::string::npos) return "";
    return line.substr(l + 1, r - l - 1);
}

int main() {
    demoFind();
    std::cout << "\n=== LeetCode 28 ===\n";
    std::cout << strStr("hello", "ll") << "\n";          // 2
    std::cout << strStr("aaaaa", "bba") << "\n";         // -1

    std::cout << "\n=== LeetCode 387 ===\n";
    std::cout << firstUniqCharByFind("leetcode") << "\n";   // 0

    std::cout << "\n=== LeetCode 1910 ===\n";
    std::cout << removeOccurrences("daabcbaabcbc", "abc") << "\n";  // "dab"

    std::cout << "\n=== LeetCode 771 ===\n";
    std::cout << numJewelsInStones("aA", "aAAbbbb") << "\n";  // 3
    std::cout << numJewelsInStones("z", "ZZ")       << "\n";  // 0

    std::cout << "\n=== 日常實務: 解析 charset ===\n";
    std::cout << "[" << extractCharset("text/html; charset=utf-8")     << "]\n";
    std::cout << "[" << extractCharset("application/json")             << "]\n"; // empty

    std::cout << "\n=== 日常實務: 抽 log level ===\n";
    std::cout << "[" << extractLogLevel("2024-05-18 10:00:00 [ERROR] oops")  << "]\n";
    std::cout << "[" << extractLogLevel("2024-05-18 10:00:00 [INFO]  hello") << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 if (s.find("x") >= 0) 永遠成立、是個 bug?
    //    A:find 回傳 size_type (= size_t,unsigned),npos 也是這個型別的最大值
    //       (0xFFFF...FFF)。任何 size_type 必 >= 0,因此判斷找不到只能寫
    //       != std::string::npos。寫 != -1 雖然在常見實作上湊巧能跑,但會被
    //       編譯器警告 signed/unsigned 比較,且非 portable。
    //
    //  Q2:standard 強制規定 find 採用什麼演算法 (KMP / Boyer-Moore)?
    //    A:沒有強制。標準只規定行為與 O(N*M) 上界。多數實作走 naive memcmp
    //       (對短 needle cache-friendly、SIMD 友善),反而比 KMP 預處理 O(M)
    //       更划算。需要保證 O(N) 的場景請用 std::search + std::boyer_moore_searcher。
    //
    //  Q3:在迴圈內反覆 s.find(needle, pos) 找所有出現位置時,pos 該怎麼前進?
    //    A:命中後若是「找重疊」應 pos = p + 1;若是「不重疊」應 pos = p + needle.size()。
    //       兩者都不能寫成 ++pos 之外又忘記更新 — 否則會無窮迴圈或漏掉位置。
    //       erase + 再 find 的 pattern 也要記得每次重設 pos。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra find.cpp -o find

// === 預期輸出 ===
// find World = 7
// find 'o' from 5 = 8
// find xyz = npos
//
// === LeetCode 28 ===
// 2
// -1
//
// === LeetCode 387 ===
// 0
//
// === LeetCode 1910 ===
// dab
//
// === LeetCode 771 ===
// 3
// 0
//
// === 日常實務: 解析 charset ===
// [utf-8]
// []
//
// === 日常實務: 抽 log level ===
// [ERROR]
// [INFO]
