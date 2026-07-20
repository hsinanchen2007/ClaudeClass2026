// =============================================================================
//  03_manipulators.cpp  —  IO 格式化工具 (manipulators)
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/io/manip
//    https://cplusplus.com/reference/iomanip/
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、manipulator 是什麼？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  manipulator 是「插進 stream 的特殊物件」，效果是改變 stream 後續輸出的
//  格式設定。寫起來像：
//
//      cout << std::hex << 255 << '\n';      // ff
//      cout << std::setw(8) << 42 << '\n';   // "      42"
//
//  它們其實是 ostream& operator<<(ostream&, manipulator) 的 overload；插入
//  時改 stream 內部 flag，之後輸出就用新的設定。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最常用的 manipulators                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  通用：
//    boolalpha / noboolalpha    bool 印 true/false 還是 1/0
//    showpos / noshowpos         正數要不要 +
//    uppercase / nouppercase     hex 用大寫
//    showpoint                   永遠帶小數點
//
//  整數進制：
//    dec / hex / oct
//    showbase / noshowbase       要不要印 0x / 0
//
//  浮點：
//    fixed                       固定小數位（123.45）
//    scientific                  科學記號（1.2345e+02）
//    defaultfloat                預設（C++11，介於兩者之間）
//    setprecision(n)             小數位數（fixed/scientific 模式下）
//                                或「總有效位數」（defaultfloat 模式）
//
//  寬度與對齊（要 <iomanip>）：
//    setw(n)                     下一個輸出的最小寬度（單次效）
//    setfill(c)                  寬度不足時填什麼字
//    left / right / internal     對齊方向（持續效）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、setw 是「單次效」 — 別忘了                             │
//  └────────────────────────────────────────────────────────────┘
//
//  setw 只影響「下一次的輸出」，不會持續。setfill 跟其他 flags 是「設定
//  到改變為止」。
//
//      cout << setw(5) << 1 << 2 << 3 << '\n';   // "    123"
//                                                  ↑
//                                       只有 1 用 setw(5)，2、3 緊接著
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：std::manipulators
  - std::manipulators 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - manipulator 如 std::hex、std::fixed 會改變 stream 格式狀態，影響之後的輸出直到被改回。
  - std::setw 只影響下一次格式化輸出，std::setprecision 則會持續保存。
  - 格式化輸出若要局部設定，實務上可使用 stringstream 或保存/還原 flags。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】IO 格式化 manipulators
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. setw 和 setprecision 的「黏性」差在哪？
//     答：std::setw(n) 只作用於「下一次」輸出，之後自動失效，所以每個欄位都要重設；
//     setprecision、fixed、hex、left、setfill 等則是黏性（sticky），設定後一直有效到
//     下次改變。所以 cout << setw(10) << "a" << "b"; 中的 "b" 沒有寬度 10，而
//     cout << setprecision(3) << x << y; 中 x 與 y 都是 3 位精度。
//     追問：怎麼還原格式？（存下 std::ios_base::fmtflags f = cout.flags(); 事後
//     cout.flags(f)，或寫一個 RAII wrapper；C++20 的 std::format 沒有這個問題，
//     因為每次呼叫的格式是獨立的）
//
// 🔥 Q2. setprecision(n) 的 n 到底是什麼意思？
//     答：依模式而異——預設模式下是「有效位數」，在 std::fixed 或 std::scientific 之下
//     才是「小數點後位數」。這是這組 manipulator 最常被答錯的地方。
//
// ⚠️ 陷阱. cout << 0.1; 印出 0.1，是不是代表 double 精確存下了 0.1？
//     答：不是。cout 預設精度是 6 位有效位數，把值四捨五入顯示成 0.1 而已，實際儲存的
//     double 是 0.1000000000000000055511... 要看到真實值可寫
//     cout << std::setprecision(17) << 0.1;。17 這個數字來自
//     std::numeric_limits<double>::max_digits10，它保證 double → 文字 → double 無損
//     來回轉換；要做這種 round-trip，C++17 的 std::to_chars 是更好的現代選擇。
//     為什麼會錯：把「印出來好看」當成「存得精確」，忽略了輸出端預設就在做四捨五入。
// ═══════════════════════════════════════════════════════════════════════════

#include <iomanip>
#include <iostream>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：bool / 進制 / showbase
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo1] bool default      = " << true << '\n';
    std::cout << "[Demo1] bool boolalpha    = " << std::boolalpha << true << '\n';
    std::cout << std::noboolalpha;            // 復原（其它 demo 不受影響）

    std::cout << "[Demo1] dec 255           = " << 255 << '\n';
    std::cout << "[Demo1] hex 255           = " << std::hex << 255 << '\n';
    std::cout << "[Demo1] oct 255           = " << std::oct << 255 << '\n';
    std::cout << "[Demo1] hex+showbase      = " << std::hex << std::showbase << 255 << '\n';
    std::cout << std::dec << std::noshowbase; // 復原

    // ─────────────────────────────────────────────────────────
    // Demo 2：浮點精度
    // ─────────────────────────────────────────────────────────
    double pi = 3.141592653589793;
    std::cout << "[Demo2] default           = " << pi << '\n';
    std::cout << "[Demo2] setprecision(3)   = " << std::setprecision(3) << pi << '\n';
    std::cout << "[Demo2] fixed prec(3)     = " << std::fixed << pi << '\n';
    std::cout << "[Demo2] scientific prec(3)= " << std::scientific << pi << '\n';
    std::cout.unsetf(std::ios::floatfield);   // 復原 floatfield
    std::cout << std::setprecision(6);

    // ─────────────────────────────────────────────────────────
    // Demo 3：寬度與對齊
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo3] setw(8)|" << std::setw(8) << 42 << "|\n";
    std::cout << "[Demo3] left   |" << std::left  << std::setw(8) << 42 << "|\n";
    std::cout << "[Demo3] fill 0 |" << std::right << std::setw(8) << std::setfill('0') << 42 << "|\n";
    std::cout << std::setfill(' '); // 復原

    // ─────────────────────────────────────────────────────────
    // Demo 4：實用範例 — 印一個對齊的表格
    // ─────────────────────────────────────────────────────────
    struct Row { std::string name; int age; double score; };
    Row rows[] = {{"Alice", 30, 92.5}, {"Bob", 9, 100.0}, {"Charlie", 12, 7.25}};

    std::cout << "[Demo4]\n";
    std::cout << std::left  << std::setw(10) << "Name"
              << std::right << std::setw(5)  << "Age"
              << std::right << std::setw(10) << "Score" << '\n';
    std::cout << std::string(25, '-') << '\n';
    std::cout << std::fixed << std::setprecision(2);
    for (const auto& r : rows) {
        std::cout << std::left  << std::setw(10) << r.name
                  << std::right << std::setw(5)  << r.age
                  << std::right << std::setw(10) << r.score << '\n';
    }
    // 復原格式（避免影響後續輸出）
    std::cout.unsetf(std::ios::floatfield);
    std::cout << std::setprecision(6);

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：hex dump — 把一段 bytes 印成「位址 + 16 個 hex bytes」
    //   工作上 debug 二進位協定、檔案格式時常用；展示 hex + setw + setfill 組合。
    // ─────────────────────────────────────────────────────────
    {
        unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F,
                                0x72, 0x6C, 0x64, 0x21, 0x0A, 0x00, 0xFF, 0x7E};
        std::cout << "[hexdump]\n";
        std::cout << std::hex << std::setfill('0');
        std::cout << std::setw(4) << 0 << ":";
        for (unsigned char b : data) {
            std::cout << ' ' << std::setw(2) << static_cast<int>(b);
        }
        std::cout << '\n';
        // 復原 dec / fill
        std::cout << std::dec << std::setfill(' ');
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：用 RAII 把 stream 格式狀態自動還原
    //   工作中常見坑：函式內改了 cout flag、沒還原 → 後續輸出格式錯亂。
    //   把「flag 還原」做成 RAII 物件，scope 結束自動還原。
    // ─────────────────────────────────────────────────────────
    {
        struct FmtGuard {
            std::ios& s;
            std::ios saved{nullptr};
            explicit FmtGuard(std::ios& st) : s(st) { saved.copyfmt(s); }
            ~FmtGuard() { s.copyfmt(saved); }
        };

        std::cout << "[FmtGuard] before: " << 255 << '\n';     // dec
        {
            FmtGuard g{std::cout};
            std::cout << std::hex << std::showbase
                      << "[FmtGuard] inside: " << 255 << '\n';  // 0xff
        }
        std::cout << "[FmtGuard] after:  " << 255 << '\n';     // dec 還原
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 setw 寫一次就被「重置」？
    //    A：標準明文：width() 在每次輸出後會被重設為 0。其他 flags（fill、
    //       precision、進制、對齊）是「黏著」的，要明確改回來。
    //
    //  Q2：怎麼把 stream 設定「存起來、之後復原」？
    //    A：可以這樣做：
    //         std::ios state(nullptr);
    //         state.copyfmt(std::cout);    // 存
    //         /* ... 改 cout 設定 ... */
    //         std::cout.copyfmt(state);    // 復原
    //       或 Boost.IO/io_state_savers 的 RAII 包裝。
    //
    //  Q3：std::format 是什麼關係？
    //    A：C++20 的 std::format 把「格式化邏輯」獨立出來、與 stream 解耦，
    //       語法類似 Python f-string；對於格式化是更現代的選擇。
    //       見 C++_Format 專題（之後若補建）。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_manipulators.cpp -o 03_manipulators

// === 預期輸出 ===
// [Demo1] bool default      = 1
// [Demo1] bool boolalpha    = true
// [Demo1] dec 255           = 255
// [Demo1] hex 255           = ff
// [Demo1] oct 255           = 377
// [Demo1] hex+showbase      = 0xff
// [Demo2] default           = 3.14159
// [Demo2] setprecision(3)   = 3.14
// [Demo2] fixed prec(3)     = 3.142
// [Demo2] scientific prec(3)= 3.142e+00
// [Demo3] setw(8)|      42|
// [Demo3] left   |42      |
// [Demo3] fill 0 |00000042|
// [Demo4]
// Name        Age     Score
// -------------------------
// Alice        30     92.50
// Bob           9    100.00
// Charlie      12      7.25
// [hexdump]
// 0000: 48 65 6c 6c 6f 20 57 6f 72 6c 64 21 0a 00 ff 7e
// [FmtGuard] before: 255
// [FmtGuard] inside: 0xff
// [FmtGuard] after:  255
