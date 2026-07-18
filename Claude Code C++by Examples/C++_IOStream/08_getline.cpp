// =============================================================================
//  08_getline.cpp  —  std::getline 行讀取與常見陷阱
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/string/basic_string/getline
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼需要 getline？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  cin >> string 只讀「一個 token」(遇空白停)，無法讀整行。getline 可以：
//
//      std::string line;
//      std::getline(std::cin, line);    // 讀整行（不含結尾 '\n'）
//
//  也可以指定其它分隔符：
//      std::getline(in, token, ',');    // 讀到 ',' 為止
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最常見陷阱：cin >> 之後接 getline                     │
//  └────────────────────────────────────────────────────────────┘
//
//  情境：
//
//      int n;
//      std::string line;
//      std::cin >> n;
//      std::getline(std::cin, line);    // ← 預期讀「下一行」
//
//  問題：cin >> n 讀完數字，「換行符 '\n'」還留在 buffer 裡。getline 會立
//  刻看到 '\n' 而回傳「空字串」，沒讀到真正想要的那行。
//
//  解法：在 getline 之前先吃掉那個 '\n'：
//
//      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//      std::getline(std::cin, line);
//
//  或用 ignore 的簡化版：std::cin.ignore() — 預設只 ignore 一個字元。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、getline 跟 EOF                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  while (std::getline(in, line)) { ... }
//
//  在每次成功讀完一行後 stream 還是 good；讀到 EOF（沒更多輸入）時 getline
//  會把 stream 設為 fail 並 break 迴圈 — 標準輸入循環。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、Windows '\r\n' 問題                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  Linux / macOS 行尾就是 '\n'。Windows 文字檔行尾是 "\r\n" — 在文字模式
//  下 fstream 自動把 "\r\n" 轉成 '\n'，沒問題；但若用 binary 模式或從
//  Windows 機器拷檔過來在 Linux 處理，每行最後會多一個 '\r'。
//
//  解法：讀完後 if (!line.empty() && line.back() == '\r') line.pop_back();
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：getline 基本用法
//   * Demo 2：cin >> + getline 的陷阱（重現 + 修法）
//   * Demo 3：getline 自訂 delim
//   * Demo 4：「來自 Windows 的 \r」清除
// =============================================================================

/*
補充筆記：std::getline
  - std::getline 讀到分隔符為止，預設分隔符是換行，分隔符本身不放進結果字串。
  - 和 operator>> 混用時，前一次格式化讀取留下的 newline 會讓 getline 讀到空行。
  - 檔案逐行處理時，用 while (getline(in, line)) 讓 stream 狀態控制迴圈。
  - std::getline 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
*/
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：getline 基本（用 istringstream 模擬輸入）
    // ─────────────────────────────────────────────────────────
    std::istringstream input1{"hello world\nthis is line 2\n"};
    std::string line;
    int no = 1;
    while (std::getline(input1, line)) {
        std::cout << "[Demo1] line " << no++ << " = [" << line << "]\n";
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：>> + getline 的經典陷阱
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"42\nthe answer\n"};
        int n;
        std::string s;
        in >> n;                  // 讀到 42，'\n' 留在 buffer
        std::getline(in, s);      // ❌ 讀到「空字串」
        std::cout << "[Demo2-bug]  n=" << n << " s=[" << s << "]\n";
    }
    {
        std::istringstream in{"42\nthe answer\n"};
        int n;
        std::string s;
        in >> n;
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // 修法
        std::getline(in, s);
        std::cout << "[Demo2-fix]  n=" << n << " s=[" << s << "]\n";
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：自訂 delim — 解析 CSV
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"alice,bob,charlie"};
        std::vector<std::string> names;
        std::string token;
        while (std::getline(in, token, ',')) names.push_back(token);
        std::cout << "[Demo3]";
        for (auto& s : names) std::cout << " [" << s << "]";
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 4：清掉 \r（Windows 來源檔常見）
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"hello\r\nworld\r\n"};   // 模擬 \r\n 結尾
        std::string ln;
        while (std::getline(in, ln)) {
            if (!ln.empty() && ln.back() == '\r') ln.pop_back();
            std::cout << "[Demo4] cleaned = [" << ln << "]\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：讀檔時跳過空行與註解（#），並印行號
    //   工作上看 config / 自訂 DSL 都會這樣處理。
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{
            "# header\n"
            "first line\n"
            "\n"
            "# another comment\n"
            "second line\n"
            "third line\n"};
        std::string ln;
        int rawNo = 0, effNo = 0;
        std::cout << "[skip-blank-comment]\n";
        while (std::getline(in, ln)) {
            ++rawNo;
            if (ln.empty() || ln.front() == '#') continue;
            ++effNo;
            std::cout << "  raw#" << rawNo << " eff#" << effNo
                      << " = " << ln << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：分析「多行 token」 — 每行先 getline，再對每行 split
    //   常見場景：log 檔每行是 "level message"，要拆 level + message。
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{
            "INFO server started\n"
            "WARN  disk almost full\n"
            "ERROR connection lost  abruptly\n"};
        std::string ln;
        std::cout << "[per-line-split]\n";
        while (std::getline(in, ln)) {
            std::istringstream ls{ln};
            std::string level, rest;
            ls >> level;                     // 拿第一個 token
            std::getline(ls, rest);          // 拿剩下的整段
            // 砍掉前導空白
            size_t s = rest.find_first_not_of(' ');
            if (s != std::string::npos) rest = rest.substr(s);
            std::cout << "  level=" << level << " msg=[" << rest << "]\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：getline 為什麼不把 '\n' 包在結果裡？
    //    A：標準設計如此 — 結果就是「不含 delim 的內容」。要重建一行需自
    //       己 + '\n'。
    //
    //  Q2：getline 對極長行會怎樣？
    //    A：std::string 會自動成長，沒有 buffer overflow 風險。但若行真
    //       的是 GB 級，要考慮分塊讀取（in.read(buf, N)）以控管記憶體。
    //
    //  Q3：getline 跟 std::cin >> ws 結合？
    //    A：`std::cin >> std::ws` 跳掉前導空白（包括 '\n'）。在 cin >> n;
    //       之後接 std::cin >> std::ws; std::getline(...) 是另一個常用修法。
    //
    return 0;
}
