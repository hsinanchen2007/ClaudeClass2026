// =============================================================================
//  01_overview.cpp  —  iostream 體系總覽
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/io
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、iostream 在做什麼？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ 的 IO 模型把「資料來源 / 去處」抽象成 stream，並把「格式化」與
//  「實際 IO」分兩層：
//
//      格式化層 (formatting)          ← cin/cout/ifstream/ostringstream...
//          ↑
//      streambuf 層 (raw bytes)        ← filebuf / stringbuf / cinbuf
//          ↑
//      作業系統 IO                     ← read/write/fread/fwrite
//
//  這個分層讓「同一份格式化邏輯」可以套到不同 backend：
//   * 寫到螢幕 → cout（背後是 stdout 的 streambuf）
//   * 寫到檔案 → ofstream（背後是 filebuf）
//   * 寫到字串 → ostringstream（背後是 stringbuf）
//
//  所以你會看到大家把「格式化邏輯」做成接受 std::ostream& 的函式 — 之後
//  cout、ofstream、ostringstream 全都能用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、八個基本 stream 名字                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  (header <iostream>)
//      std::cin    標準輸入   (stdin)
//      std::cout   標準輸出   (stdout)
//      std::cerr   標準錯誤   (stderr) — 不 buffered，立刻輸出
//      std::clog   標準 log   (stderr 但 buffered)
//
//  (header <fstream>)
//      std::ifstream  讀檔
//      std::ofstream  寫檔
//      std::fstream   雙向檔案
//
//  (header <sstream>)
//      std::istringstream / std::ostringstream / std::stringstream
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、operator<< 與 operator>> 是什麼？                     │
//  └────────────────────────────────────────────────────────────┘
//
//  cout、cin 並不是「特殊語法」 — `<<` 和 `>>` 是普通的二元運算子，標準函
//  式庫對 std::ostream / std::istream overload 它們而已：
//
//      std::ostream& operator<<(std::ostream&, int);
//      std::ostream& operator<<(std::ostream&, const std::string&);
//      ...
//
//  回傳的是「自己」(stream ref)，所以可以鏈起來：
//
//      cout << "x = " << x << '\n';
//      // 等價於 ((cout << "x = ") << x) << '\n';
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：cout / cerr / clog 差異
//   * Demo 2：自訂型別 overload operator<<
//   * Demo 3：把同一個格式化函式套用到 cout 與 ostringstream
// =============================================================================

/*
補充筆記：std::overview
  - std::overview 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - iostream 把輸入輸出抽象成 stream，資料來源可以是 console、file 或 string，但狀態模型相同。
  - stream 保存格式設定，例如進位、寬度、精度；格式狀態會持續影響後續輸出。
  - 讀寫 stream 後要檢查狀態，因為失敗不一定丟例外，更多時候是設定 failbit。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iostream 體系總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. cout、cerr、clog 差在哪？該怎麼選？
//     答：cout 接 stdout、有緩衝；cerr 接 stderr、預設 unitbuf（每次輸出後自動 flush）；
//     clog 也接 stderr 但有緩衝。所以：緊急錯誤用 cerr（即使程式隨後崩潰也保證看得到）、
//     大量診斷 log 用 clog（有緩衝、效能好）、正常輸出用 cout。重導向時
//     ./prog > out.txt 2> err.txt 可以把資料與 log 分離。
//     追問：cerr 需要 endl 嗎？（不用，它本來就每次都 flush）
//
// 🔥 Q2. 怎麼讓自訂型別支援 cout <<？
//     答：重載非成員的 operator<<：
//     std::ostream& operator<<(std::ostream& os, const Point& p);
//     重點四項：① 必須是非成員（左運算元是 ostream，不是你的型別）② 參數與回傳都用
//     std::ostream&（不是 cout），才能同時支援檔案流、stringstream 與鏈式呼叫
//     ③ 需要存取私有成員時宣告為 friend ④ 不要在裡面加 endl，換行由呼叫者決定。
//     追問：C++20 有更現代的做法嗎？（特化 std::formatter<Point>，讓 std::format 支援）
//
// Q3. iostream 的繼承體系為什麼用虛擬繼承？
//     答：basic_istream 與 basic_ostream 都虛擬繼承自 basic_ios，basic_iostream 同時
//     繼承兩者。若非虛擬繼承就會有兩份 basic_ios 子物件（菱形問題），狀態位元與緩衝區
//     指標會分裂成兩套。這是實務上少數合理使用虛擬繼承的正面教材。實際做 I/O 的是
//     std::streambuf（透過 rdbuf() 取得），stream 類別只負責格式化。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <sstream>
#include <string>

// 一個自訂型別
struct Point { int x, y; };

// overload operator<< 讓 Point 能直接被 stream 印
// 慣例：operator<< 要接受 std::ostream&，回傳同一個 ref（鏈式呼叫）
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}

// 「格式化邏輯」函式 — 不關心 backend 是 cout 還是 stringstream
static void describe(std::ostream& os, const std::string& name, const Point& p) {
    os << "name=" << name << " pos=" << p << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：cout / cerr / clog
    //   執行時把 stderr 重導向就能看到差異：./01_overview 2> err.txt
    // ─────────────────────────────────────────────────────────
    std::cout << "[cout] normal info (buffered, line-buffered to terminal)\n";
    std::cerr << "[cerr] error message (unbuffered)\n";
    std::clog << "[clog] log message (buffered to stderr)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：自訂型別 + operator<<
    // ─────────────────────────────────────────────────────────
    Point p{3, 4};
    std::cout << "[Demo2] " << p << '\n';   // (3, 4)

    // ─────────────────────────────────────────────────────────
    // Demo 3：同一個 describe 套到 cout 與 ostringstream
    //   實務應用：unit test 中把輸出送到 stringstream，斷言內容
    // ─────────────────────────────────────────────────────────
    describe(std::cout, "alice", {1, 2});

    std::ostringstream oss;
    describe(oss, "bob", {7, 8});
    std::cout << "[Demo3] captured = " << oss.str();   // 已含換行

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：用「相同 describe」測試輸出格式 — 單元測試最常見的姿勢
    //   寫格式化函式時讓它吃 std::ostream&，測試時把 ostringstream 接進去，
    //   就可以 assert 字串內容。
    // ─────────────────────────────────────────────────────────
    {
        std::ostringstream oss;
        describe(oss, "carol", {0, 0});
        const std::string actual = oss.str();
        const std::string expected = "name=carol pos=(0, 0)\n";
        std::cout << "[unit-test] expected == actual? "
                  << std::boolalpha << (actual == expected) << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：把同一份輸出同時寫到 cout 與字串
    //   工作中常見：log 寫螢幕又留一份；用兩個 ostream 一起寫即可。
    // ─────────────────────────────────────────────────────────
    {
        std::ostringstream buf;
        auto writeBoth = [&](const std::string& msg) {
            std::cout << msg;
            buf      << msg;
        };
        writeBoth("[fan-out] hello\n");
        writeBoth("[fan-out] world\n");
        std::cout << "[fan-out] captured size = " << buf.str().size() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 cerr 預設不 buffer？
    //    A：錯誤訊息要「即時看到」 — 程式 crash 前 buffered 的內容會丟失。
    //       cerr 不 buffer 確保你看到最後一條 log。clog 是 buffered 版的
    //       錯誤輸出（一般 log 用）。
    //
    //  Q2：endl 跟 '\n' 差在哪？
    //    A：'\n' 只插入換行字元；endl 插入換行 + flush buffer。Flush 對
    //       cerr 沒差（本來就不 buffer），對 cout / file 是強制立即寫出。
    //       熱迴圈裡用 endl 會明顯變慢，多數情況 '\n' 即可。
    //
    //  Q3：operator<< 為什麼回傳 ostream& 而非 void？
    //    A：為了鏈式：cout << a << b << c; 拆解成
    //         ((cout << a) << b) << c;
    //       每個 << 必須回傳能接著被下一個 << 吃進去的 ostream&。
    //
    return 0;
}
