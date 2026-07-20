// =============================================================================
// 檔名: getline.cpp
// 主題: std::getline (從輸入串流讀一整行到 string)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/getline
//   - https://cplusplus.com/reference/string/string/getline/
// =============================================================================
//
// 【函式資訊 Information】
//   template<class CharT, class Traits, class Alloc>
//   basic_istream<CharT,Traits>& getline(basic_istream<CharT,Traits>& input,
//                                        basic_string<CharT,Traits,Alloc>& str,
//                                        CharT delim);
//   // 預設 delim = '\n'
//   basic_istream& getline(basic_istream& input, basic_string& str);
//
//   - 所屬:`<string>` 標頭內的 std namespace 自由函式
//   - 參數:input (輸入串流);str (輸出 string);delim (分隔字元,預設 '\n')
//   - 回傳:input 自身的 reference (可串接、可作為 while 條件)
//   - noexcept:否,可能丟例外 (若 stream 設了 exceptions())
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// 【1. 基本流程 (簡化版規格)】
//   1. 呼叫 sentry 物件 (做必要的 setup,例如 tied stream flush)
//   2. 清空 str (str.clear())
//   3. 反覆從 input 讀字元並 append 到 str:
//        - 遇到 EOF → 設定 eofbit,結束
//        - 遇到 delim → 不放入 str,結束
//        - 字元數超過 str.max_size() → 設 failbit,結束
//   4. 若一個字元都沒讀到 (空行 + EOF) → 設 failbit
//   5. 回傳 input
//
// 【2. getline vs operator>>】
//
//   行為           operator>>                     getline
//   ──────────────  ─────────────────────────────  ─────────────────────────────
//   分隔            任意空白 (space/tab/\n)        delim (預設 '\n')
//   保留空白        否,token 模式                  是,讀一整行包含內部空白
//   skipws          會跳過前導空白                  不會跳過 (連空行都讀得到 "")
//   讀到分隔        分隔字元留在 buffer             分隔字元被吃掉但不放入 str
//   典型用途        讀一個欄位/單字                 讀一整行 / 讀 CSV 欄位
//
// 【3. 與 cin 的著名陷阱:殘留 '\n' 問題】
//   常見錯誤:
//       int n;
//       std::cin >> n;                    // 假設輸入 "42\n",讀走 42 留下 '\n'
//       std::string line;
//       std::getline(std::cin, line);     // 立刻拿到空字串!
//
//   原因:operator>> 不消耗結尾 '\n',getline 一見到 '\n' 就馬上結束 (空 line)。
//
//   解法 (任選):
//     (a) std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     (b) std::cin >> std::ws;            // 略過空白 (含 '\n') 再 getline
//     (c) 全程改用 getline + stringstream 解析
//
// 【4. 自訂 delim 的威力】
//   - getline(ss, field, ',')  → 簡單 CSV 解析
//   - getline(ss, token, '/')  → URL/路徑切片
//   - getline(ss, log, ';')    → log 結尾用 ';' 的格式
//   - 注意:不支援多字元 delim (要自己手寫狀態機或用 std::regex)
//
// 【5. while (std::getline(in, line)) 範式】
//   - getline 回傳 stream reference,而 stream 有 operator bool 轉型
//     (回傳 !fail()),因此可直接放在 while 條件裡。
//   - 結束條件:讀到 EOF 之後最後一次 getline 設了 eofbit + failbit → 跳出迴圈
//   - 如果檔案最後一行沒換行符:最後一次仍會讀到該行內容 (eofbit 但非 failbit
//     ── 取決於是否讀到任何字元)
//
// 【6. 例外與錯誤處理】
//   - 預設 stream 不丟例外,只設 failbit。
//   - 若呼叫過 stream.exceptions(failbit | badbit),getline 失敗會丟 ios_base::failure。
//   - bad_alloc 仍可能在 str append 時拋出 (記憶體不足)。
//
// 【7. 效能考量】
//   - 一次一個字元讀取:每次都過 sentry / sync,對非常大檔案稍慢。
//     若效能敏感,改用 read 大塊 buffer 自己 split。
//   - 但 getline 已經是「正確 + 易讀」的標準方案,絕大多數情境足夠快。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 【std::ws 是什麼?】
//   - 一個 stream manipulator,從 input 串流跳過所有空白字元 (含 '\n')。
//     用法:`std::cin >> std::ws;`
//   - 在 cin >> n 與 getline 之間插入 `cin >> std::ws` 可優雅消化殘留 '\n'。
//
// 【cin.ignore() 與 ignore(N, delim)】
//   - cin.ignore() 默認丟掉一個字元。
//   - cin.ignore(N, '\n') 丟掉至多 N 個字元,直到遇到 '\n' 為止 (含 '\n')。
//   - 經典慣用法:
//       std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     表達「丟到行尾」,需包含 <limits>。
//
// 【while (std::getline(...)) 為何能當條件?】
//   - basic_istream 有 explicit operator bool() (C++11 起;之前是 void* 轉換)
//   - 對應 !this->fail()。讀失敗 (含 EOF 後再讀) 會回傳 false,迴圈結束。
//
// 【getline 與 std::string::npos 無關】
//   - 別把 getline 和 find 系列混淆;getline 失敗用 stream 狀態 (eofbit/failbit) 偵測,
//     不是 npos。
//
// 【自訂 delim 不能用字串】
//   - getline(ss, line, "AB")  ← 編譯錯誤
//   - 想用多字元分隔得自己迴圈 + find,或用 std::regex_token_iterator。
//
// 【tied stream 與互動式輸入】
//   - std::cin 預設與 std::cout tie:讀之前會 flush cout。
//   - 命令列工具的 prompt 可順利顯示。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. operator>> 後再 getline,getline 會「立刻」讀到一個空字串(因為 \n 還在 buffer)。
//    解法:先 std::cin.ignore() 或 cin >> ws 後再 getline。
// 2. 自訂 delim 例如 ',' 可解析簡單 CSV。但別忘了引號內逗號的處理 (這時要用 csv parser)。
// 3. 例外: 若 stream 設了 exceptions(),失敗會丟例外。
// 4. getline 回傳 stream 的參考,可在 while 條件中使用:
//      while (std::getline(in, line)) { ... }
// 5. Windows CRLF 檔案在 binary 模式下讀進來會殘留 '\r' 在每行尾,可用
//      if (!line.empty() && line.back() == '\r') line.pop_back();
//
// =============================================================================

/*
補充筆記：std::getline
  - std::getline 是 <string> 提供的自由函式，目標是把輸入串流中的字元讀進 std::string；它不同於 istream::getline(char*, size)，後者是讀進 C-style char buffer。
  - 呼叫 getline(input, str) 時，str 會先被清空，再把讀到的內容放進去；它不是 append，所以不要期待原本的字串內容會被保留。
  - 預設分隔字元是 '\n'；讀到分隔字元時，分隔字元會從 stream 被取走，但不會放進 str，下一次讀取會從分隔字元後面開始。
  - 若檔案最後一行沒有換行，只要 getline 有讀到任何字元，這一行仍然會成功取得；之後再讀一次才會因 EOF/fail 狀態讓 while 條件結束。
  - getline 不會自動跳過前導空白，空行會得到空字串；這和 operator>> 會略過空白的 token 讀取模式完全不同。
  - cin >> n 之後直接 getline 常讀到空字串，因為數字後面的 '\n' 還留在輸入緩衝區；若只想丟掉該行剩餘內容，用 ignore(max, '\n')，若想略過所有前導空白才讀下一段，用 std::ws。
  - std::ws 會吃掉所有空白，包括多個換行；如果空白行本身有語意，例如設定檔允許空值或使用者可以輸入空行，就不要盲目使用 std::ws。
  - 自訂 delimiter 只能是一個 CharT，例如 ',' 或 '/'；真正 CSV 的引號、跳脫與內嵌逗號不是 getline(field, ',') 能完整處理的，需要專門 parser 或狀態機。
  - getline 的失敗要看 stream 狀態，不是看 std::string::npos；逐行處理時使用 while (std::getline(in, line))，迴圈結束後再用 bad()/eof()/fail() 區分錯誤與正常 EOF。
  - Windows CRLF 文字若以某些模式或來源讀入，line 尾端可能留下 '\r'；跨平台處理文字檔時常需要檢查 !line.empty() && line.back() == '\r'。
  - 讀很長的行時，std::string 可能多次重新配置；若知道行長上限，可先 line.reserve(n) 減少配置，但不要為了微小輸入提早複雜化。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】getline
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::getline 與 cin >> s 讀字串差在哪？
//     答：cin >> s 以空白為分隔——先跳過前導空白，讀到下一個空白就停，
//         而且把那個空白「留在」緩衝區裡。
//         getline(cin, s) 一路讀到換行為止，並且「吃掉且丟棄」那個換行，不存進 s。
//         所以 >> 讀不到含空白的一整行，getline 可以。
//     追問：getline 能換分隔字元嗎？→ 可以，第三參數，例如 getline(is, s, ',')。
//
// ⚠️ 陷阱. 為什麼 cin >> n 之後接 getline(cin, line) 會拿到空字串？
//     答：>> n 讀完數字就停，把使用者按下的那個 '\n' 留在緩衝區。
//         接著的 getline 立刻碰到這個殘留換行，於是回傳空字串就結束——
//         它並沒有失敗，是「正確地讀到了一個空行」。
//         修法：cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 丟掉殘留。
//     為什麼會錯：多數人以為 >> 會順手把分隔符消掉，但 >> 只「跳過前導空白」，
//         結尾的空白與換行一律原封不動留著，這個不對稱正是 bug 的根源。
//
// 🔥 Q2. while (getline(cin, line)) 為什麼可以當迴圈條件？
//     答：getline 回傳 istream& 自身，而 istream 有 explicit operator bool，
//         在 boolean context 下轉成「串流狀態是否良好」，遇到 EOF 或錯誤才為 false。
//     追問：那迴圈結束後怎麼區分正常 EOF 與真錯誤？→ 再查 eof() / fail() / bad()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <sstream>

void demoGetline() {
    std::stringstream ss("Hello, World!\nSecond line\nThird,line");

    std::string line;
    while (std::getline(ss, line)) {                     // 經典 while-getline 範式
        std::cout << "[" << line << "]\n";
    }

    // 自訂分隔
    std::stringstream csv("apple,banana,cherry");
    std::string field;
    while (std::getline(csv, field, ',')) {
        std::cout << "field=" << field << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【實務範例】簡單 CSV 解析
// 為何用 getline: 配合 stringstream 與自訂 delim,優雅切分欄位。
//                 注意:這是最簡單版本,不支援引號 / 跳脫 / 內嵌逗號。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::vector<std::string>> parseCSV(const std::string& text) {
    std::vector<std::vector<std::string>> rows;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        std::vector<std::string> cols;
        std::stringstream ls(line);
        std::string col;
        while (std::getline(ls, col, ',')) cols.push_back(col);
        rows.push_back(cols);
    }
    return rows;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 71. Simplify Path 的 token 切分
// 題目: 簡化 Unix 路徑 ("/a/./b/../../c/" → "/c")。
// 為何用 getline: 配合 '/' delim 自然切出 path token,無需手寫 split。
// -----------------------------------------------------------------------------
std::string simplifyPath(const std::string& path) {
    std::vector<std::string> stack;
    std::stringstream ss(path);
    std::string token;
    while (std::getline(ss, token, '/')) {
        if (token.empty() || token == ".") continue;       // 連續 // 或 . 都略過
        if (token == "..") {
            if (!stack.empty()) stack.pop_back();          // 回上一層
        } else {
            stack.push_back(token);
        }
    }
    std::string out = "/";
    for (size_t i = 0; i < stack.size(); ++i) {
        out += stack[i];
        if (i + 1 < stack.size()) out += '/';
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】載入 .env 設定檔 (KEY=VALUE 每行一筆)
// 為何用 getline: docker-compose、CI/CD 環境變數設定的標準格式。
//                 用 getline 一次讀一行,跳過註解 (#) 與空行。
// -----------------------------------------------------------------------------
#include <unordered_map>
std::unordered_map<std::string, std::string> loadEnvFile(const std::string& text) {
    std::unordered_map<std::string, std::string> env;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        // 處理 Windows CRLF (如果是文字模式讀檔通常已自動轉換,但保險起見)
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // 跳過空行與註解
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;

        size_t eq = line.find('=', start);
        if (eq == std::string::npos) continue;

        std::string key = line.substr(start, eq - start);
        std::string val = line.substr(eq + 1);
        env[key] = val;
    }
    return env;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】逐行掃描 log,找出 ERROR 行
// 為何用 getline: log 檔常以「行」為單位,getline 是天生的 log scanner。
//                 真實場景:tail -f 之類的監控工具底層也是 line-oriented。
// -----------------------------------------------------------------------------
std::vector<std::string> grepErrors(const std::string& logText) {
    std::vector<std::string> hits;
    std::stringstream ss(logText);
    std::string line;
    size_t lineno = 0;
    while (std::getline(ss, line)) {
        ++lineno;
        if (line.find("ERROR") != std::string::npos) {
            hits.push_back("L" + std::to_string(lineno) + ": " + line);
        }
    }
    return hits;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1page. 1page0. Number of Lines To Write String - 簡化版
// 題目: LeetCode 1location7. 1location0. Count Words in a Sentence (此處示範:給多行文字,計算總單字數)
// 為何用 getline: 一次處理一行,易讀;先 getline 切行,再對每行計單字。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int countTotalWordsInLines(const std::string& text) {
    std::stringstream ss(text);
    std::string line;
    int total = 0;
    while (std::getline(ss, line)) {
        std::stringstream ls(line);
        std::string w;
        while (ls >> w) ++total;
    }
    return total;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 3】從 multi-line input 抽出 HTTP headers 成 key/value map
// 為何用 getline: HTTP 報文以 \r\n 分行,getline 一次抓一行,再依 ':' 分鍵值。
// -----------------------------------------------------------------------------
std::vector<std::pair<std::string,std::string>> parseHttpHeaders(const std::string& blob) {
    std::vector<std::pair<std::string,std::string>> out;
    std::stringstream ss(blob);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();    // 去 CR
        if (line.empty()) break;                                       // header 結束
        size_t c = line.find(':');
        if (c == std::string::npos) continue;
        std::string k = line.substr(0, c);
        std::string v = line.substr(c + 1);
        // trim left space on value
        size_t vs = v.find_first_not_of(' ');
        out.push_back({k, vs == std::string::npos ? "" : v.substr(vs)});
    }
    return out;
}

int main() {
    demoGetline();

    std::cout << "\n=== parseCSV ===\n";
    auto rows = parseCSV("a,b,c\n1,2,3\nx,y,z");
    for (auto& r : rows) {
        for (auto& c : r) std::cout << "[" << c << "] ";
        std::cout << "\n";
    }

    std::cout << "\n=== LeetCode 71 ===\n";
    std::cout << simplifyPath("/home//foo/")          << "\n";   // /home/foo
    std::cout << simplifyPath("/a/./b/../../c/")      << "\n";   // /c

    std::cout << "\n=== 日常實務: 載入 .env ===\n";
    auto env = loadEnvFile(
        "# database settings\n"
        "DB_HOST=localhost\n"
        "DB_PORT=5432\n"
        "\n"
        "API_KEY=secret-token");
    for (auto& [k, v] : env) std::cout << k << "=" << v << "\n";

    std::cout << "\n=== 日常實務: 掃描 log 抓 ERROR ===\n";
    auto errs = grepErrors(
        "2026-05-04 10:00 INFO  start\n"
        "2026-05-04 10:01 ERROR db conn failed\n"
        "2026-05-04 10:02 WARN  retry\n"
        "2026-05-04 10:03 ERROR auth invalid\n");
    for (auto& e : errs) std::cout << e << "\n";

    std::cout << "\n=== LeetCode 1location0 (countTotalWordsInLines) ===\n";
    std::cout << countTotalWordsInLines("hello world\nfoo bar baz\nx") << "\n";   // 6

    std::cout << "\n=== 日常實務: parseHttpHeaders ===\n";
    auto hs = parseHttpHeaders(
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: curl/8.0\r\n"
        "Accept: */*\r\n"
        "\r\n");
    for (auto& [k, v] : hs) std::cout << "[" << k << "]=[" << v << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:cin >> n 後直接呼叫 getline 為什麼立刻拿到空字串?
    //    A:operator>> 不消耗結尾 '\n',它留在 buffer 裡。getline 一見到 '\n'
    //       就馬上停 (空行)。解法:cin.ignore(numeric_limits<streamsize>::max(), '\n')
    //       或 cin >> std::ws 跳掉空白後再 getline。
    //
    //  Q2:getline 的 delim 字元最後跑哪去了?能用多字元 delim 嗎?
    //    A:delim 從 stream 取出但「不」放入 str (與 operator>> 把分隔字元留在
    //       buffer 不同)。delim 只能是單一 CharT,不支援 "AB" 這種多字元;
    //       多字元分隔得自己迴圈 + find,或用 std::regex_token_iterator。
    //
    //  Q3:while (std::getline(in, line)) 為什麼能當條件?何時跳出?
    //    A:getline 回傳 stream&,basic_istream 有 explicit operator bool()
    //       (= !this->fail())。讀到 EOF 後再讀會設 failbit → 條件變 false 跳出。
    //       Windows CRLF 在 binary 模式會殘留 '\r' 在 line 尾,需 line.pop_back()。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra getline.cpp -o getline

// === 預期輸出 (節錄) ===
// === LeetCode 1location0 (countTotalWordsInLines) ===
// 6
// === 日常實務: parseHttpHeaders ===
// [Host]=[example.com]
// [User-Agent]=[curl/8.0]
// [Accept]=[*/*]
