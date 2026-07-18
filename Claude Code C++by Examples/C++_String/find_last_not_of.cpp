// =============================================================================
// 檔名: find_last_not_of.cpp
// 主題: std::string::find_last_not_of (從後往前找「不在字元集」的位置)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/find_last_not_of
//   - cplusplus.com: https://cplusplus.com/reference/string/string/find_last_not_of/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type find_last_not_of(const string& str, size_type pos = npos) const noexcept;
//   size_type find_last_not_of(const char* s, size_type pos = npos) const;
//   size_type find_last_not_of(const char* s, size_type pos, size_type count) const;
//   size_type find_last_not_of(char ch, size_type pos = npos) const noexcept;
//   // C++17 起加入 string_view 重載
//   size_type find_last_not_of(std::string_view sv, size_type pos = npos) const noexcept;
//
// 所屬類別: std::basic_string<CharT, Traits, Allocator>
// 回傳型別: size_type (typedef 為 std::size_t,通常為 unsigned long)
// 回傳值:   字串中「最後一個」不屬於指定字元集合的字元位置 (0-based);
//           若每個字元都屬於該集合 (或字串為空),回傳 npos (= -1 cast 成 unsigned)。
//
// 【詳細解釋 Explanation】
//
// (1) 函式定位:
//     find_last_not_of 是「字元集合搜尋」家族中的最後一員。整個家族包含:
//       - find_first_of      : 從前往後找「屬於」集合的字元
//       - find_first_not_of  : 從前往後找「不屬於」集合的字元
//       - find_last_of       : 從後往前找「屬於」集合的字元
//       - find_last_not_of   : 從後往前找「不屬於」集合的字元 (本檔)
//     四者組合可解決幾乎所有「逐字元篩選」的字串前置處理需求。
//
// (2) 與 find_first_not_of 的搭配 ── trim 標準寫法:
//     在 C++ 中沒有內建 trim() 函式,實務上絕大多數的 trim 實作都長這樣:
//
//       std::string trim(const std::string& s, std::string_view ws = " \t\r\n") {
//           auto first = s.find_first_not_of(ws);
//           if (first == std::string::npos) return "";   // 全部都是空白 → 空字串
//           auto last  = s.find_last_not_of(ws);
//           return s.substr(first, last - first + 1);
//       }
//
//     這個寫法的關鍵:
//       a. find_first_not_of 找到「左側第一個非空白」── 即 trim 後字串的起點
//       b. find_last_not_of  找到「右側最後一個非空白」── 即 trim 後字串的終點
//       c. substr(first, last - first + 1) 把中間段抽出來
//     若兩者其中一個回傳 npos,代表整個字串都是要被 trim 掉的字元,結果為空。
//
// (3) pos 參數的語意 (常見誤解):
//     pos 是「最大允許的搜尋位置 (含)」,並不像 find 那樣是「起始位置」。
//     換言之,find_last_not_of 會在 [0, pos] 區間中由右往左搜尋。
//     例如:
//       std::string s = "abc!def!";   // 長度 8,索引 0..7
//       s.find_last_not_of('!');                 // 7?不,'!' 在 7,跳過後找到 'f' 在 6
//       s.find_last_not_of('!', 6);              // 從位置 6 (含) 往左,'f' → 6
//       s.find_last_not_of('!', 3);              // 從位置 3 (含) 往左,'!' 在 3,跳過,'c' → 2
//       s.find_last_not_of('!', npos);           // 等同預設,搜整個字串
//
// (4) 「字元集合」是「OR」關係,不是「整段子字串」:
//     最常見的初學者錯誤是把 find_last_not_of("abc") 誤解成「找最後一個不是 abc 子字串」。
//     實際上是「找最後一個既不是 'a'、也不是 'b'、也不是 'c' 的位置」。
//     如果要找子字串,該用 rfind。
//
// (5) 時間複雜度:
//     O(N * |chars|),N 是搜尋範圍長度,|chars| 是集合大小。
//     STL 實作通常會建一個 256 大小的 bitset (假設 char) 來加速到 O(N + |chars|)。
//
// (6) noexcept 標註的差異:
//     接受 string、char、string_view 的版本標 noexcept;接受 const char* 的版本不是
//     (因為對 nullptr 的處理理論上會 UB,但傳統上實作不會丟例外)。
//
// 【注意事項 Pay Attention】
// 1. 對於只想去尾端空白的場景,這比手寫迴圈簡潔很多。
// 2. 全部都在字元集 → npos;若想 trim 後變空字串,要先檢查 npos。
// 3. 別把 pos 當成「起始 index」:它是「上限 index (含)」,從那位置往左搜。
// 4. 字元集合是 OR 關係,不是子字串比對。
// 5. 中文/UTF-8 多位元組字元不能逐 byte 比較;若是 UTF-8 trim 中文,
//    建議用 ICU 或 std::wstring + locale。
//
// 【概念補充 Concept Deep Dive】
//
// (A) trim_right 的標準寫法 ── 現實世界處理日誌、輸入時的必備:
//
//       std::string trim_right(const std::string& s, std::string_view ws = " \t\r\n") {
//           auto last = s.find_last_not_of(ws);
//           return last == std::string::npos ? "" : s.substr(0, last + 1);
//       }
//
//     為什麼業界這麼依賴它?
//       - 輸入來源 (subprocess、socket、檔案讀行) 常帶 \r\n、tab、尾端空白
//       - 寫入欄位前 (DB、CSV) 必須先把尾巴清乾淨
//       - 比對前 (key 比對、URL 比對) 必須避免「看不見」的字元造成 mismatch
//     若用迴圈寫:
//       while (!s.empty() && std::isspace(s.back())) s.pop_back();
//     雖可,但每次 pop_back 都要呼叫一次 isspace 並修改 string;
//     find_last_not_of + substr 一次計算位置、一次配置新字串,通常更快也更易讀。
//
// (B) trim_left + trim_right 結合:
//
//       std::string trim(const std::string& s, std::string_view ws = " \t\r\n") {
//           auto a = s.find_first_not_of(ws);
//           if (a == std::string::npos) return "";
//           auto b = s.find_last_not_of(ws);
//           return s.substr(a, b - a + 1);
//       }
//
//     C++ 標準庫一直沒收錄 trim;C++20 加入 starts_with/ends_with、C++23 加入 contains,
//     但 trim 仍然要自己寫。Boost 有 boost::algorithm::trim,但不依賴 boost 時這個寫法
//     就是最 idiomatic 的方式。
//
// (C) 實作 chomp (移除單一尾端換行,Perl 常見):
//
//       std::string chomp(std::string s) {
//           auto last = s.find_last_not_of("\r\n");
//           s.resize(last == std::string::npos ? 0 : last + 1);
//           return s;
//       }
//
// (D) 對 C++17 之後,把字元集合用 std::string_view 傳是更高效的選擇 ──
//     避免不必要的 const char* → std::string 隱式轉換。
//
// =============================================================================

/*
補充筆記：std::string::find_last_not_of
  - std::string::find_last_not_of 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::find_last_not_of 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoFLNO() {
    std::string s = "Hello, World!   ";
    auto p = s.find_last_not_of(' ');
    std::cout << "last non-space at " << p << "\n";    // 12 ('!')

    std::string ws = "  \t\n";
    std::cout << "all whitespace: " << (ws.find_last_not_of(" \t\n") == std::string::npos ? "yes" : "no") << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1614 之輔助 — 移除右側雜訊字元
// 為何用 find_last_not_of: 只去尾端的特定字元(例如尾端 0)。
// -----------------------------------------------------------------------------
std::string trimRight(const std::string& s, const std::string& cs) {
    auto end = s.find_last_not_of(cs);
    if (end == std::string::npos) return "";
    return s.substr(0, end + 1);
}

// -----------------------------------------------------------------------------
// 【實務範例】處理浮點數字串,移除尾端 0 與多餘小數點
// 為何用 find_last_not_of: "12.3400" → "12.34"
// -----------------------------------------------------------------------------
std::string trimTrailingZeros(const std::string& s) {
    if (s.find('.') == std::string::npos) return s;
    auto end = s.find_last_not_of('0');
    if (end != std::string::npos && s[end] == '.') --end;     // 把點也去掉
    return s.substr(0, end + 1);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Log line 規整: 同時去除尾端 CR、LF、空白、tab
// 為何用 find_last_not_of: 從外部 source (subprocess、socket) 取得的字串
//                          常含混亂的尾端字元;一個函式呼叫處理掉全部。
// -----------------------------------------------------------------------------
std::string normalizeLogLine(const std::string& s) {
    auto end = s.find_last_not_of(" \t\r\n\v\f");
    if (end == std::string::npos) return "";
    return s.substr(0, end + 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 1】LeetCode 58. Length of Last Word
// 題目: 給字串含空白,回傳最後一個單字的長度。
// 為何用 find_last_not_of: 一行找出「最後一個非空白字元位置」,再從那位置往左找空白。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int lengthOfLastWord(const std::string& s) {
    size_t end = s.find_last_not_of(' ');
    if (end == std::string::npos) return 0;
    size_t start = s.find_last_of(' ', end);
    return static_cast<int>(end - (start == std::string::npos ? -1 : static_cast<long long>(start)));
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從路徑取最後一段 (basename),不含尾端 '/'
// 為何用 find_last_not_of: 先去掉尾端的 '/' 再 rfind('/') 找最後分隔符。
//                          處理 "/var/log/" 這類含尾斜線的路徑很常見。
// -----------------------------------------------------------------------------
std::string basename(const std::string& path) {
    size_t end = path.find_last_not_of('/');
    if (end == std::string::npos) return "/";
    size_t slash = path.find_last_of('/', end);
    return (slash == std::string::npos) ? path.substr(0, end + 1)
                                        : path.substr(slash + 1, end - slash);
}

int main() {
    demoFLNO();

    std::cout << "\n=== trimRight ===\n";
    std::cout << "[" << trimRight("hello!!!!!", "!") << "]\n";

    std::cout << "\n=== trimTrailingZeros ===\n";
    std::cout << trimTrailingZeros("12.3400") << "\n";   // "12.34"
    std::cout << trimTrailingZeros("12.0000") << "\n";   // "12"
    std::cout << trimTrailingZeros("1234")    << "\n";   // "1234"

    std::cout << "\n=== LeetCode 58 ===\n";
    std::cout << lengthOfLastWord("Hello World")          << "\n";  // 5
    std::cout << lengthOfLastWord("   fly me   to   the moon  ") << "\n";  // 4

    std::cout << "\n=== 日常實務: 規整 log line ===\n";
    std::cout << "[" << normalizeLogLine("hello world\r\n  ") << "]\n";
    std::cout << "[" << normalizeLogLine("\t\t  trailing\t\n") << "]\n";

    std::cout << "\n=== 日常實務: basename ===\n";
    std::cout << "[" << basename("/var/log/app.log") << "]\n";   // app.log
    std::cout << "[" << basename("/var/log/")       << "]\n";   // log
    std::cout << "[" << basename("file.txt")        << "]\n";   // file.txt

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:find_last_not_of 的 pos 參數是「起始位置」還是「上限位置」?
    //    A:是「最大允許 index (含)」 — 從 pos 往左掃。預設 npos 等同
    //       size()-1 (整串)。常被誤解成「起始位置」就會在小字串上意外回 npos。
    //
    //  Q2:用 find_last_not_of 寫的 trim_right,為何比 while + pop_back 快?
    //    A:while + pop_back 每次都要重複呼叫 isspace + 修改 size 寫 '\0';
    //       find_last_not_of 一次掃描算出最終 index,再用 substr 一次 build
    //       新字串 (或 resize 一次)。對長尾巴字串差異尤其明顯。
    //
    //  Q3:處理浮點字串去尾 0 (如 "12.3400" → "12.34") 為什麼可以用它?
    //    A:把字元集設為 "0",find_last_not_of('0') 就回傳「最後一個非 0 位置」。
    //       但要記得若該位置剛好是 '.',還要再退一格才不會留下孤立小數點 — 範例
    //       中的 trimTrailingZeros 就示範了這個收尾處理。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra find_last_not_of.cpp -o find_last_not_of

// === 預期輸出 (節錄) ===
// === LeetCode 58 ===
// 5
// 4
// === 日常實務: basename ===
// [app.log]
// [log]
// [file.txt]
