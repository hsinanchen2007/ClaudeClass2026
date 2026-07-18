// =============================================================================
// 檔名: io_operators.cpp
// 主題: std::string 的 I/O 運算子 (operator<< / operator>>)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/operator_ltltgtgt
//   - https://cplusplus.com/reference/string/string/operator%3C%3C/
//   - https://cplusplus.com/reference/string/string/operator%3E%3E/
// =============================================================================
//
// 【函式資訊 Information】
//   template<class CharT, class Traits, class Alloc>
//   basic_ostream<CharT,Traits>&
//       operator<<(basic_ostream<CharT,Traits>& os,
//                  const basic_string<CharT,Traits,Alloc>& str);
//
//   template<class CharT, class Traits, class Alloc>
//   basic_istream<CharT,Traits>&
//       operator>>(basic_istream<CharT,Traits>& is,
//                  basic_string<CharT,Traits,Alloc>& str);
//
//   - 所屬:`<string>` 標頭內 std namespace 的非成員函式
//   - 回傳:os/is 自身的 reference (可串接 cout << a << b)
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// 【1. operator<< 行為】
//   - 把 string 的字元寫入 output stream。
//   - 受目前 stream 的 fmtflags 影響:
//       (a) width()  : 最小欄位寬度 (不足以填充字元 fill 補足)
//       (b) fill()   : 填充字元,預設空白
//       (c) left/right/internal : 對齊方向
//       (d) write 後 width() 自動歸零 (sticky 例外)
//   - 不會在中間加任何空白或換行 ── 全部由你掌控。
//
// 【2. operator>> 行為 (預設)】
//   1. sentry 物件啟動 → flush 綁定的 ostream
//   2. 若 skipws 旗標 (預設啟用),跳過所有前導空白
//   3. 從第一個非空白字元開始讀
//   4. 持續讀,直到:
//        (a) 遇到下一個空白
//        (b) 達到 width() 限制 (若有設定)
//        (c) EOF
//   5. 把讀到的字元 assign 進 str (會 clear str)
//   6. 若一個字元都沒讀到 → 設 failbit
//   7. 回傳 stream
//
// 【3. operator>> vs getline 一覽】
//
//   面向          operator>>                       std::getline
//   ────────────  ──────────────────────────────  ──────────────────────────────
//   讀取單位      一個 token (空白分隔)            一整行 (delim 分隔)
//   skipws        是 (預設啟用)                    否
//   保留空白      否,讀到空白就停                 是,內部空白原封保留
//   換行符        留在 buffer                      被消化但不放入 str
//   常見用途      cin >> name >> age               cin.getline / 讀檔逐行
//
// 【4. width() 用於限制讀取長度】
//   char buf[16];          // 假設用 char[];對 std::string 沒有 buffer overflow 問題
//   std::cin.width(10);
//   std::cin >> str;       // 至多讀 10 個字元 (對 string 是安全的;
//                           // 但對 char* 必須設,否則 buffer overflow!)
//   注意:width() 是 sticky-once,讀完一次就歸零。
//
// 【5. 為何「cin >> str」對非預期輸入仍安全 (vs char[])】
//   - 對 char buf[16];     cin >> buf;   ← 沒設 width 會 overflow,危險!
//   - 對 std::string str;  cin >> str;   ← string 自動 grow,沒有 buffer overflow,
//                                          但仍可能讓記憶體用量爆炸 (DoS),所以
//                                          對外部輸入仍應用 width() 限制。
//
// 【6. std::quoted 處理含空白字串 (C++14)】
//   - 寫入時自動加上引號並 escape:
//       cout << std::quoted(s);                // → "hello world"
//   - 讀取時自動處理引號與 escape:
//       cin  >> std::quoted(s);                // 可讀「整個帶空白字串」
//   - 對 CSV、JSON 字串、shell 參數等場景非常有用。
//
// 【7. 格式化 manipulator (在 <iomanip>)】
//   - std::setw(n)        : 設定 width() (sticky-once)
//   - std::setfill(c)     : 設定填充字元 (sticky)
//   - std::left/right     : 對齊方向 (sticky)
//   - std::setprecision(n): 浮點數精度 (sticky)
//   - std::fixed/scientific: 浮點數格式 (sticky)
//   - std::hex/dec/oct    : 整數基底 (sticky)
//   - std::boolalpha      : bool → "true"/"false" (sticky)
//
// 【8. 為何 ostringstream 可優雅取代「+」拼接】
//   - 字串 + 不同型別需要先 to_string,語法繁瑣。
//   - ostringstream 一視同仁,且對 const char* 不需轉換。
//   - 對於少量 + 兩三個 string,直接 + 較快;但混型別、長表達式建議改 ostringstream。
//   - C++20 引入 std::format,語法更現代 (Python f-string 風)。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 【fmtflags 是什麼?如何 RAII 還原?】
//   - basic_ios 內部維護一組 std::ios_base::fmtflags (位元旗標) 與 width / precision / fill。
//   - 改了之後若不還原,會「污染」後續輸出。
//   - 標準 RAII guard:
//       struct StreamGuard {
//           std::ios::fmtflags f;
//           std::streamsize    p;
//           std::ostream&      os;
//           StreamGuard(std::ostream& s) : f(s.flags()), p(s.precision()), os(s) {}
//           ~StreamGuard() { os.flags(f); os.precision(p); }
//       };
//   - 函式內暫時設了 fixed + setprecision,離開時自動恢復。
//
// 【為何 cin >> str 對外部輸入「不夠安全」?】
//   - 不會 buffer overflow (string 自動成長)。
//   - 但攻擊者送 1 GB 的不含空白字串,你的程序記憶體爆掉 → DoS。
//   - 防禦:`cin.width(N); cin >> str;` 限制最大讀入長度。
//
// 【sentry 物件 (RAII) 在串流的角色】
//   - 每個 stream IO 操作開頭都會建一個 sentry,負責:
//       (a) flush tied stream
//       (b) 檢查 stream 狀態 (是否 good)
//       (c) 視 skipws 設定跳過前導空白
//   - 你寫 `cin >> x` 看似一行,底層是 sentry → ws skip → 解析 → 設 state。
//
// 【tied stream】
//   - std::cin.tie(&std::cout) 是預設行為:任何 cin 讀取前,自動 flush cout。
//   - 互動式 prompt 才能正確顯示。
//   - 競賽程式碼為了效能會解綁:cin.tie(nullptr); ios::sync_with_stdio(false);
//
// 【std::format (C++20) 與 printf-style 的 trade-off】
//
//   寫法                 安全          locale       效能       表達力
//   ───────────────────  ────────────  ─────────────  ────────  ──────────
//   printf("%d %s", ...) 編譯器警告    yes           最快      最簡潔
//   ostringstream        type-safe     yes           中等      中等
//   std::format (C++20)  type-safe     可選          快        最佳
//   std::print  (C++23)  type-safe     可選          最快      最簡潔
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. operator>> 與 getline 混用時要清掉換行符 (cin.ignore() 或 cin >> ws)。
// 2. width() 可限制最大讀入字元數,防止惡意大字串造成記憶體爆炸。
// 3. operator<< 對齊 / 寬度可用 std::setw / std::left / std::right 控制 (來自 <iomanip>)。
// 4. 從 stringstream 讀整段文字:常與 std::ostringstream 一起使用。
// 5. 改了 fmtflags / precision 記得還原 (RAII guard 或記下 old state)。
// 6. cin.fail() 後別忘了 cin.clear() + cin.ignore(),否則後續讀取永遠失敗。
//
// =============================================================================

/*
補充筆記：std::string::io_operators
  - std::string::io_operators 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::io_operators 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <sstream>
#include <iomanip>

void demoIO() {
    // operator<<
    std::string s = "Hello";
    std::cout << "[" << s << "]\n";
    std::cout << std::setw(10) << s << "|\n";              // 右對齊 (預設)
    std::cout << std::left << std::setw(10) << s << "|\n";  // 左對齊
    std::cout << std::right;                                // 還原對齊

    // operator>> 從 stringstream 讀 token (空白分隔)
    std::stringstream ss("foo bar baz");
    std::string tok;
    while (ss >> tok) std::cout << "tok: " << tok << "\n";

    // std::quoted 處理含空白字串
    std::stringstream qs("\"hello world\" rest");
    std::string q;
    qs >> std::quoted(q);
    std::cout << "quoted parsed: [" << q << "]\n";          // [hello world]
}

// -----------------------------------------------------------------------------
// 【實務範例】用 ostringstream 拼字串(替代多個 +)
// 為何用 operator<<: 對於混合不同型別 (int/double/string),這比 + 更直觀。
// -----------------------------------------------------------------------------
std::string buildMessage(const std::string& name, int age, double height) {
    std::ostringstream os;
    os << "Name: " << name
       << ", Age: " << age
       << ", Height: " << std::fixed << std::setprecision(2) << height;
    return os.str();
}

// -----------------------------------------------------------------------------
// 【LeetCode 1078. Occurrences After Bigram】
// 題目: 給文字 text 與兩個 word first / second,找所有 "first second THIRD"
//       中的 THIRD。
// 為何用 operator>>: 用 stringstream 一個 token 一個 token 讀過去,
//                   完美利用 operator>> 「以空白分隔」的天性。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> findOcurrences(const std::string& text,
                                        const std::string& first,
                                        const std::string& second) {
    std::stringstream ss(text);
    std::vector<std::string> tokens;
    std::string w;
    while (ss >> w) tokens.push_back(w);                   // 空白切詞

    std::vector<std::string> out;
    for (size_t i = 2; i < tokens.size(); ++i) {
        if (tokens[i - 2] == first && tokens[i - 1] == second) {
            out.push_back(tokens[i]);
        }
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 14. Longest Common Prefix (用 stringstream 切詞做變體)】
// 題目原版: 找一組字串的最長共同前綴。
// 這裡示範:接收一行 "flower flow flight",用 operator>> 切詞後再求 LCP。
// 為何用 operator>>: CLI 工具常常一行給多個字串,用 stringstream 是最簡單做法。
// -----------------------------------------------------------------------------
std::string longestCommonPrefixLine(const std::string& line) {
    std::stringstream ss(line);
    std::vector<std::string> words;
    std::string w;
    while (ss >> w) words.push_back(w);
    if (words.empty()) return "";

    std::string prefix = words[0];
    for (size_t i = 1; i < words.size(); ++i) {
        size_t j = 0;
        while (j < prefix.size() && j < words[i].size() && prefix[j] == words[i][j]) ++j;
        prefix.resize(j);
        if (prefix.empty()) break;
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】Exception message 加上 file:line 上下文
// 為何用 ostringstream + operator<<: 在大型專案中,只看到 "invalid input"
//                                    根本不知道哪裡丟出來。配上 file 與 line
//                                    可大幅縮短除錯時間。常封裝成 macro。
// -----------------------------------------------------------------------------
std::string makeContextMessage(const char* file, int line, const std::string& msg) {
    std::ostringstream os;
    os << "[" << file << ":" << line << "] " << msg;
    return os.str();
}
#define CTX_MSG(msg) makeContextMessage(__FILE__, __LINE__, (msg))

// -----------------------------------------------------------------------------
// 【日常實務範例 2】CLI 表格輸出 (對齊欄位)
// 為何用 setw + left: 命令列工具常需印出對齊整齊的表格 (例如 docker ps、ls -l),
//                    setw + left 是最簡單實現方式。
// -----------------------------------------------------------------------------
void printRow(const std::string& name, int port, const std::string& status) {
    std::cout << std::left
              << std::setw(20) << name
              << std::setw(10) << port
              << std::setw(15) << status
              << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 (補充)】LeetCode 1page. 1page0. Reorder Data in Log Files - 簡化版
// 題目: LeetCode 824. Goat Latin
// 為單字加後綴 "ma" + 重複 a 個 'a' (依索引)。
// 為何用 stringstream + operator>>: 用 ss >> word 切詞最直觀;再用 ostringstream 組合輸出。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string toGoatLatin(const std::string& sentence) {
    std::istringstream is(sentence);
    std::ostringstream os;
    std::string w;
    int idx = 1;
    bool first = true;
    const std::string vowels = "aeiouAEIOU";
    while (is >> w) {
        if (vowels.find(w[0]) == std::string::npos) {
            w = w.substr(1) + w[0];               // 子音字首移到尾
        }
        w += "ma";
        w += std::string(static_cast<size_t>(idx), 'a');
        if (!first) os << ' ';
        os << w;
        first = false;
        ++idx;
    }
    return os.str();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 3】從一段含多筆「key=value」的字串解析鍵值
// 為何用 stringstream + getline: 先 ss + ' ' 切 token,再 stringstream + '=' 切 key/val。
// -----------------------------------------------------------------------------
std::vector<std::pair<std::string,std::string>> parseKvLine(const std::string& line) {
    std::vector<std::pair<std::string,std::string>> out;
    std::istringstream ss(line);
    std::string tok;
    while (ss >> tok) {
        size_t eq = tok.find('=');
        if (eq != std::string::npos) {
            out.push_back({tok.substr(0, eq), tok.substr(eq + 1)});
        }
    }
    return out;
}

int main() {
    demoIO();

    std::cout << "\n=== buildMessage ===\n";
    std::cout << buildMessage("Alice", 30, 1.6789) << "\n";

    std::cout << "\n=== LeetCode 1078 ===\n";
    auto r = findOcurrences("alice is a good girl she is a good student", "a", "good");
    for (auto& s : r) std::cout << s << " ";
    std::cout << "\n";

    std::cout << "\n=== LeetCode 14 (line variant) ===\n";
    std::cout << longestCommonPrefixLine("flower flow flight") << "\n";    // "fl"
    std::cout << longestCommonPrefixLine("dog racecar car") << "\n";       // ""

    std::cout << "\n=== 日常實務: 帶 file:line 的訊息 ===\n";
    std::cout << CTX_MSG("config not found") << "\n";

    std::cout << "\n=== 日常實務: CLI 表格輸出 ===\n";
    printRow("SERVICE",  8080, "STATUS");
    printRow("nginx",    8080, "running");
    printRow("postgres", 5432, "running");
    printRow("redis",    6379, "stopped");

    std::cout << "\n=== LeetCode 824 ===\n";
    std::cout << toGoatLatin("I speak Goat Latin") << "\n";

    std::cout << "\n=== 日常實務: parseKvLine ===\n";
    for (auto& [k, v] : parseKvLine("user=alice port=8080 debug=1"))
        std::cout << "[" << k << "]=[" << v << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:cin >> str 與 std::getline(cin, str) 對「前導空白」與「換行」處理有什麼不同?
    //    A:operator>> 預設啟用 skipws,自動跳過前導空白 (含 \n),讀到下一個空白
    //       就停且把空白留在 buffer。getline 不跳前導空白 (空行可讀到 ""),
    //       讀到 delim 才停且把 delim 吃掉。混用兩者要記得清 '\n'。
    //
    //  Q2:cin >> str 對 std::string 還會 buffer overflow 嗎?
    //    A:不會 (string 自動 grow),但攻擊者送超大不含空白字串可耗盡記憶體 → DoS。
    //       對外部不可信輸入應 cin.width(N) 限長。對 char buf[N];cin >> buf;
    //       則會真的 overflow,務必設 width 或改用 string。
    //
    //  Q3:改了 std::cout.precision / setw / fill / hex 之後為什麼後續輸出怪怪的?
    //    A:這些 fmtflags 多半是 sticky (setw 例外是 sticky-once)。函式內動了
    //       就要還原,標準 idiom 是寫一個 RAII guard 在 dtor 還原 flags()
    //       與 precision()。或避免動 cout,改用 std::format / std::print (C++23)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra io_operators.cpp -o io_operators

// === 預期輸出 (節錄) ===
// === LeetCode 824 ===
// Imaa peaksmaaa oatGmaaaa atinLmaaaaa
// === 日常實務: parseKvLine ===
// [user]=[alice]
// [port]=[8080]
// [debug]=[1]
