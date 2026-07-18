// =============================================================================
//  05_fstream_text.cpp  —  ifstream / ofstream 文字檔案讀寫
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/io/basic_ifstream
//    https://en.cppreference.com/w/cpp/io/basic_ofstream
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、三個 stream class                                      │
//  └────────────────────────────────────────────────────────────┘
//
//      std::ifstream   讀檔
//      std::ofstream   寫檔
//      std::fstream    雙向（讀+寫）
//
//  全部支援 RAII — 物件析構時自動關檔，不需要手動 close()（但需要顯式
//  flush 時可以呼叫 close 或 flush()）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、open mode（C++17 起也接受 std::filesystem::path）     │
//  └────────────────────────────────────────────────────────────┘
//
//      std::ios::in        讀
//      std::ios::out       寫（預設 ofstream 是 out | trunc）
//      std::ios::app       append（每次寫都跳到尾端）
//      std::ios::ate       開啟時跳到尾端（但仍可以隨意 seek 寫）
//      std::ios::trunc     開啟時清空檔案
//      std::ios::binary    二進位模式（見 06_fstream_binary.cpp）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、檔案開啟失敗的處理                                     │
//  └────────────────────────────────────────────────────────────┘
//
//      std::ifstream f{"data.txt"};
//      if (!f) {                       // contextually convertible to bool
//          std::cerr << "open failed\n";
//          return 1;
//      }
//
//  也可以用 f.is_open()。預設情況下「失敗不會 throw 例外」 — 想丟例外
//  要 f.exceptions(std::ios::failbit | std::ios::badbit)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：寫一個檔案（覆寫）
//   * Demo 2：把檔案內容讀回來（一行一行）
//   * Demo 3：append 模式 — 不會清空原內容
//   * Demo 4：把整檔讀成一個 string（常見技巧）
// =============================================================================

/*
補充筆記：std::fstream_text
  - std::fstream_text 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - 文字模式讀寫會受平台換行轉換影響；Windows 可能在文字模式處理 CRLF。
  - 開檔後要檢查 stream 是否成功，否則後續讀寫只會讓錯誤狀態持續存在。
  - 文字檔解析要考慮編碼；iostream 不會自動理解 UTF-8 字元邊界。
*/
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    const std::string path = "tmp_text.txt";

    // ─────────────────────────────────────────────────────────
    // Demo 1：寫檔（預設會覆寫）
    // ─────────────────────────────────────────────────────────
    {
        std::ofstream out{path};            // 預設 std::ios::out | std::ios::trunc
        if (!out) { std::cerr << "open failed\n"; return 1; }
        out << "hello\n"
            << "world\n"
            << 42 << ' ' << 3.14 << '\n';
        // RAII：out 析構時自動關檔
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：逐行讀
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path};
        if (!in) { std::cerr << "open failed\n"; return 1; }
        std::string line;
        int lineno = 1;
        while (std::getline(in, line)) {
            std::cout << "[Demo2] line " << lineno++ << ": " << line << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：append — 不清空原內容，從尾端接著寫
    // ─────────────────────────────────────────────────────────
    {
        std::ofstream out{path, std::ios::app};
        out << "appended line\n";
    }
    {
        std::ifstream in{path};
        std::string line;
        std::cout << "[Demo3] after append:\n";
        while (std::getline(in, line)) std::cout << "  | " << line << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 4：把整個檔讀成一個 string（常見技巧）
    //   方式 A：用 stringstream 接 rdbuf()
    //   方式 B：iterator 迭代 (相對慢，不示範)
    //   方式 C：reserve + read，C++17 std::filesystem::file_size 算大小
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path};
        std::ostringstream buf;
        buf << in.rdbuf();             // 把整個檔案串流接過來
        std::string content = buf.str();
        std::cout << "[Demo4] file size = " << content.size() << " bytes\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：解析簡易 config 檔（key=value）
    //   工作上常用：讀 .env 或 .ini 風格設定。重點：getline 取行、find '='
    //   切 key/value，跳過註解行（#）與空行。
    // ─────────────────────────────────────────────────────────
    {
        const std::string cfgPath = "tmp_demo.cfg";
        {
            std::ofstream out{cfgPath};
            out << "# comment line\n"
                << "host=localhost\n"
                << "port=8080\n"
                << "\n"
                << "user=admin\n";
        }

        std::ifstream cfg{cfgPath};
        std::string line;
        std::cout << "[config] parsed:\n";
        while (std::getline(cfg, line)) {
            if (line.empty() || line.front() == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            std::cout << "  [" << k << "] = [" << v << "]\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：統計檔案的「行數 / 字數 / 字元數」(類 wc)
    //   getline 計行；對每行 stringstream 拆 token 計字數。
    // ─────────────────────────────────────────────────────────
    {
        std::ifstream in{path};
        size_t lines = 0, words = 0, chars = 0;
        std::string line;
        while (std::getline(in, line)) {
            ++lines;
            chars += line.size() + 1;          // 加上換行
            std::istringstream is{line};
            std::string tok;
            while (is >> tok) ++words;
        }
        std::cout << "[wc] lines=" << lines
                  << " words=" << words
                  << " chars=" << chars << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼時候要 flush？
    //    A：fstream 析構或 close() 都會自動 flush；但程式 crash 前 buffer
    //       可能來不及寫出。重要 log 寫完可以 out << std::flush 強制立即
    //       寫到磁碟層（不過 OS page cache 還是另一層，要絕對落盤要 fsync）。
    //
    //  Q2：rdbuf() 是什麼？
    //    A：rdbuf 取得 stream 底下的 streambuf 指標。`out << in.rdbuf()`
    //       是「整個 streambuf 接到 out」的高效寫法（內部不會逐字 buffer）。
    //
    //  Q3：iostream 的中文 / UTF-8 處理？
    //    A：iostream 預設「按 byte 處理」 — 對 UTF-8 多數情況沒問題（因
    //       為它是 ASCII-superset）。但若要做 codecvt（GB2312/UTF-16 互轉）
    //       要設 imbue + locale，較複雜，建議用 ICU 等專業函式庫。
    //
    return 0;
}
