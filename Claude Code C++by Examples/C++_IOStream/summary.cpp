/*
================================================================================
【C++_IOStream/summary.cpp】

本目錄主題：iostream / fstream / stringstream（C++ I/O 串流，課件版）

你要把 iostream 當成「狀態機（state machine）」來理解：
  - 讀取/寫入不是每次都一定成功；成功/失敗會改變 stream 的狀態位元
  - 一旦 failbit/badbit 被設起來，後續讀取就會直接失敗，直到你 clear()

本檔目標（課件式）：
  - 逐一示範最常用的 member functions：good/eof/fail/bad/clear/rdstate/exceptions
  - 逐一示範常用 I/O 操作：operator>>、getline、格式化 manipulators、fstream、stringstream
  - 附錄提供 cppreference 風格速查表（C++20+ 或很少用的放附錄提示）

本 summary 原則：
  - 不加入 題庫 類範例
  - C++17 可編譯（demo 不寫檔到 repo 內，避免污染；需要檔案示範用臨時檔名）

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_IOStream/C++_IOStream summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_IOStream/C++_IOStream summary 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_IOStream/C++_IOStream summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iostream 總覽（狀態機與緩衝）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼說 iostream 要當成「狀態機」來理解？
//     答：每次讀寫都可能失敗，而失敗會改變 stream 的狀態位元（eofbit / failbit /
//     badbit），一旦設定，後續所有操作都會被忽略直到 clear()。所以正確的用法永遠是
//     「把操作本身當條件」：while (f >> x)、while (std::getline(f, line))，而不是先問
//     eof() 再讀。這條原則同時涵蓋 eof、格式錯誤與裝置錯誤三種結束原因。
//     追問：讀失敗後目標變數會變成什麼？（C++11 起被設為 0；C++11 之前保持原值）
//
// 🔥 Q2. 緩衝區在哪些時機會被 flush？
//     答：① 緩衝區滿 ② 顯式 flush()／std::flush／std::endl ③ 該 stream 設了 unitbuf
//     （如 cerr）④ 關閉檔案（close() 或 fstream 解構）⑤ 程式正常結束（static 解構、
//     exit()）⑥ tie 機制——cin 預設 tie 到 cout，每次從 cin 讀取前會先 flush cout。
//     反過來說，abort()、_exit() 或被 signal 終止時都不會 flush。
//
// Q3. printf 和 cout 各有什麼優劣？現代的答案是什麼？
//     答：printf 的格式字串集中易讀但沒有型別安全（不匹配就是 UB，-Wformat 只在格式
//     字串是字面量時才幫得上忙），也無法支援自訂型別；cout 由 operator<< 重載決議，
//     型別安全、可擴充，但 setw / setprecision 冗長且有黏性陷阱。現代答案是 C++20 的
//     std::format（與 C++23 的 std::print）：兼具可讀的格式字串、編譯期型別檢查與可擴充性。
// ═══════════════════════════════════════════════════════════════════════════

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static void header(const char* title) { std::cout << "\n[" << title << "]\n"; }

// -----------------------------------------------------------------------------
// 【重點 1】格式化輸出：manipulators（iomanip）
// -----------------------------------------------------------------------------
static void demo_cout_format() {
    header("demo_cout_format");
    double pi = 3.1415926535;

    std::cout << "  default: " << pi << "\n";
    std::cout << "  fixed(2): " << std::fixed << std::setprecision(2) << pi << "\n";
    std::cout << "  reset to defaultfmt: " << std::defaultfloat << pi << "\n";

    std::cout << "  setw/setfill:\n";
    for (int i = 1; i <= 3; ++i) {
        std::cout << "    [" << std::setw(4) << std::setfill('0') << i << "]\n";
    }
    std::cout << std::setfill(' ');
}

// -----------------------------------------------------------------------------
// 【重點 2】token 讀取 vs 整行讀取
// -----------------------------------------------------------------------------
static void demo_cin_token_vs_line() {
    header("demo_cin_token_vs_line");
    std::cout << "  NOTE: operator>> 讀 token；getline 讀整行（含空白）\n";
    std::cout << "  NOTE: getline 常和 std::ws 搭配，先吃掉前導換行/空白。\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】stringstream：把字串當成 stream 來解析
// -----------------------------------------------------------------------------
static void demo_stringstream_parse() {
    header("demo_stringstream_parse");
    std::string line = "id=42 name=alice score=98.5";

    // 用 stringstream 當成「字串版的 cin」
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        std::cout << "  token: " << token << "\n";
    }

    // 更實用：解析 "key=value"
    auto parse_kv = [](const std::string& s) {
        auto pos = s.find('=');
        if (pos == std::string::npos) return std::pair<std::string, std::string>{s, ""};
        return std::pair<std::string, std::string>{s.substr(0, pos), s.substr(pos + 1)};
    };

    std::istringstream iss2(line);
    while (iss2 >> token) {
        auto kv = parse_kv(token);
        std::cout << "  " << kv.first << " => " << kv.second << "\n";
    }

    // ostringstream：把輸出「組裝」成字串（例如 log/訊息/格式化）
    std::ostringstream oss;
    oss << "user=" << "alice" << ", score=" << std::fixed << std::setprecision(1) << 98.5;
    std::cout << "  ostringstream => " << oss.str() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】fstream：文字檔讀寫（RAII：離開 scope 自動關檔）
// -----------------------------------------------------------------------------
static void demo_fstream_text() {
    header("demo_fstream_text");

    const char* filename = "tmp_iostream_demo.txt";

    // (1) 寫檔
    {
        std::ofstream out(filename);
        out << "line1\n";
        out << "line2 with spaces\n";
    }

    // (2) 讀檔（逐行）
    {
        std::ifstream in(filename);
        std::string line;
        while (std::getline(in, line)) {
            std::cout << "  read: " << line << "\n";
        }
    }

    // (3) 清理（不一定成功：例如檔案被鎖；此處只做示範）
    std::remove(filename);
}

// -----------------------------------------------------------------------------
// 【重點 5】stream 狀態：good/eof/fail/bad/rdstate/clear
// -----------------------------------------------------------------------------
static void demo_error_state() {
    header("demo_error_state");

    std::istringstream iss("10 20 xx 30");
    int x = 0;
    while (true) {
        iss >> x;
        if (iss.good()) {
            std::cout << "  got " << x << "\n";
            continue;
        }

        // 讀取失敗時，先觀察狀態
        std::cout << "  state: eof=" << iss.eof()
                  << " fail=" << iss.fail()
                  << " bad=" << iss.bad()
                  << " rdstate=" << iss.rdstate()
                  << "\n";

        if (iss.eof()) {
            std::cout << "  reached EOF\n";
            break;
        }

        if (iss.fail()) {
            // fail：格式不符合（例如想讀 int 卻遇到 "xx"）
            std::cout << "  fail => clear() then skip one token\n";
            iss.clear(); // 清 failbit，否則後續 >> 都會立刻失敗
            std::string junk;
            iss >> junk; // 吃掉壞 token（這步很常見）
            continue;
        }

        if (iss.bad()) {
            // bad：更嚴重的 I/O 錯誤（例如底層裝置故障）
            std::cout << "  bad => stop\n";
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// 【重點 6】exceptions()：把狀態轉成例外（依需求選用）
// -----------------------------------------------------------------------------
static void demo_exceptions_mode() {
    header("demo_exceptions_mode");

    std::istringstream iss("1 xx");
    // 設定：只要 failbit 被設起來就丟例外
    iss.exceptions(std::ios::failbit);

    try {
        int a = 0, b = 0;
        iss >> a;
        std::cout << "  a=" << a << "\n";
        iss >> b; // 這裡會丟 ios_base::failure
        std::cout << "  b=" << b << "\n";
    } catch (const std::ios_base::failure& e) {
        std::cout << "  caught ios_base::failure: " << e.what() << "\n";
    }
}

int main() {
    // 若你追求速度（大量 I/O），可啟用下列兩行：
    // std::ios::sync_with_stdio(false);
    // std::cin.tie(nullptr);
    // 但要記住：啟用後不要再混用 printf/scanf，避免緩衝不同步。

    demo_cout_format();
    demo_cin_token_vs_line();
    demo_stringstream_parse();
    demo_fstream_text();
    demo_error_state();
    demo_exceptions_mode();

    std::cout << "\n[done]\n";
    return 0;
}

/*
================================================================================
【附錄：cppreference 風格速查（常用）】

std::basic_ios / std::ios_base（狀態）
  - good(), eof(), fail(), bad()
  - rdstate(), clear()
  - exceptions() / exceptions(mask)

std::istream
  - operator>>（formatted input）
  - getline（非 member：std::getline(istream, string)）
  - get(), peek(), ignore()（需要時再查）

std::ostream
  - operator<<（formatted output）
  - flush()

iomanip manipulators
  - std::setw, std::setfill, std::setprecision, std::fixed, std::defaultfloat

fstream
  - ifstream/ofstream/fstream：open/close/is_open
  - 文字 vs 二進位：openmode 設定（ios::binary）

stringstream
  - istringstream/ostringstream/stringstream
  - str() 取得/設定底層字串
================================================================================
*/
