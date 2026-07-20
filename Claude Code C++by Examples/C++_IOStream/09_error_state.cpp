// =============================================================================
//  09_error_state.cpp  —  Stream 的錯誤狀態：good / eof / fail / bad
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/io/basic_ios
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、stream 的四個狀態 bit                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  每個 stream 內部有一個 iostate（一組 bit flag）：
//
//      goodbit   = 0       一切正常
//      eofbit    = 1<<0    已經讀到檔案 / 串流尾端
//      failbit   = 1<<1    上次操作邏輯失敗（型別解析錯、找不到內容等）
//      badbit    = 1<<2    底層 IO 嚴重錯誤（罕見：磁碟壞、reader 死掉）
//
//  常用查詢：
//      stream.good()    iostate == 0
//      stream.eof()     eofbit set
//      stream.fail()    failbit OR badbit set
//      stream.bad()     badbit set
//
//  contextual conversion to bool：
//      if (stream)      ↔ !stream.fail()
//      if (!stream)     ↔ stream.fail()
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、典型誤用：用 eof() 當迴圈條件                          │
//  └────────────────────────────────────────────────────────────┘
//
//  錯誤寫法：
//
//      while (!cin.eof()) {
//          int x;
//          cin >> x;
//          process(x);             // ← 最後一次可能讀到「失敗的 x」
//      }
//
//  原因：eof 是「讀到尾」之後才被 set；最後一次讀失敗、x 還是舊值，但
//  process 已經被呼叫一次。
//
//  正確寫法：
//
//      int x;
//      while (cin >> x) {           // 讀成功才進迴圈
//          process(x);
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、發生 fail 後要 clear() 才能繼續用                      │
//  └────────────────────────────────────────────────────────────┘
//
//  fail bit set 之後，所有後續 IO 都會「立刻失敗、不前進」。要繼續用：
//
//      stream.clear();              // 清掉狀態 bits
//      // 然後可能要 ignore 掉造成失敗的內容
//      stream.ignore(...);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：印出 stream 在各種情況下的 4 個狀態
//   * Demo 2：壞輸入 → fail → clear + ignore 復原
//   * Demo 3：用 exceptions() 改成「失敗就 throw」
// =============================================================================

/*
補充筆記：std::error_state
  - std::error_state 屬於 iostream；stream 同時保存資料流位置、格式設定和錯誤狀態。
  - operator>> 會跳過空白並遇空白停止，getline 會讀到換行；混用時常需要先處理殘留 newline。
  - failbit、badbit、eofbit 代表不同狀態；讀取失敗後要 clear() 才能繼續使用 stream。
  - fstream 開檔模式要明確：in/out/app/binary/trunc 組合不同會影響是否覆蓋、追加或以二進位處理。
  - sync_with_stdio(false) 和 cin.tie(nullptr) 可提升競賽輸入速度，但混用 C stdio 和 iostream 時要小心順序。
  - stringstream 適合字串解析與格式化，但大量資料轉換時可考慮 charconv 降低 locale 和配置成本。
  - eofbit 只表示到達檔尾，不一定是錯誤；failbit 表示格式或讀取失敗，badbit 表示更底層 I/O 問題。
  - 常見正確讀法是 while (stream >> value) 處理成功讀到的資料，而不是先用 eof() 判斷。
  - clear() 只清狀態，不會移動讀取位置；若要重新讀檔還要 seekg。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】stream 錯誤狀態（good / eof / fail / bad）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. goodbit、eofbit、failbit、badbit 各代表什麼？
//     答：goodbit 表示沒有任何錯誤位元；eofbit 表示已到達輸入結尾（注意是在「嘗試讀取
//     越過結尾之後」才設）；failbit 表示格式或邏輯錯誤，未發生資料遺失，可用 clear()
//     復原；badbit 表示嚴重錯誤，通常代表資料遺失或裝置故障，通常不可復原。查詢函式是
//     good()、eof()、fail()（等於 failbit || badbit）、bad()。stream 可轉 bool，
//     if (cin >> x) 等價於檢查 !fail()。
//     追問：可以讓 stream 出錯時拋例外嗎？（可以：
//     cin.exceptions(std::ios::failbit | std::ios::badbit)，但要小心 eofbit 也會觸發，
//     於是正常讀到檔尾也變成例外）
//
// 🔥 Q2. cin >> x 失敗後為什麼要 clear() 再 ignore()？順序可以顛倒嗎？
//     答：失敗後 failbit 被設定，且導致失敗的字元仍留在緩衝區。必須先 clear() 讓 stream
//     回到可用狀態，再 ignore() 丟掉那些壞字元——順序不能顛倒，因為 failbit 還在的時候
//     ignore() 這類操作會被忽略，等於什麼都沒做，於是下次讀取又失敗，形成無窮迴圈。
//
// ⚠️ 陷阱. while (f >> x) 結束之後，怎麼知道是正常讀完還是出錯了？
//     答：迴圈條件只告訴你「停了」，要區分原因得看狀態位元：f.eof() 為真表示讀到結尾
//     （正常結束）；f.bad() 為真表示裝置層級的嚴重錯誤；兩者皆非而 f.fail() 為真，
//     通常是格式錯誤（資料髒了）。三種結束原因的處理方式完全不同，不該一律當成正常結束。
//     為什麼會錯：習慣「迴圈跑完就是做完了」，忽略 stream 是狀態機，結束有好幾種語意。
// ═══════════════════════════════════════════════════════════════════════════

#include <ios>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

static void dumpState(const std::ios& s, const std::string& label) {
    std::cout << "  [" << label << "]"
              << " good=" << s.good()
              << " eof=" << s.eof()
              << " fail=" << s.fail()
              << " bad=" << s.bad() << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：依序看 stream 進入各種狀態
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"42 hello"};
        std::cout << "[Demo1] start\n";
        dumpState(in, "init");

        int n;
        in >> n;
        dumpState(in, "after read 42");

        in >> n;                      // 讀 hello → fail
        dumpState(in, "after read hello (fail)");

        in.clear();                   // 清掉 fail
        std::string s;
        in >> s;                      // 讀 hello 成功
        dumpState(in, "after clear+read string");

        in >> n;                      // 沒東西 → eof + fail
        dumpState(in, "after read past end");
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：fail 復原
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"100 foo 200"};
        int v;
        in >> v;
        std::cout << "[Demo2] first = " << v << '\n';

        in >> v;
        if (in.fail()) {
            in.clear();
            std::string trash;
            in >> trash;             // 把 "foo" 吃掉
            std::cout << "[Demo2] skipped: " << trash << '\n';
        }
        in >> v;
        std::cout << "[Demo2] last = " << v << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：開啟 exceptions() — 失敗就 throw
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"abc"};
        in.exceptions(std::ios::failbit | std::ios::badbit);
        try {
            int v;
            in >> v;                  // 失敗 → throw
            std::cout << "[Demo3] got " << v << '\n';
        } catch (const std::ios_base::failure& e) {
            std::cout << "[Demo3] caught: " << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：輸入驗證 helper — 反覆要求直到拿到合法整數
    //   工作場景：互動式工具，要拿到「保證合法」的整數才能往下走。
    //   這裡用 istringstream 模擬若干輪輸入；實務 cin 寫法完全相同。
    // ─────────────────────────────────────────────────────────
    {
        auto readPositiveInt = [](std::istream& in) -> int {
            int v;
            while (true) {
                if (in >> v && v > 0) return v;
                if (in.eof()) return -1;     // 沒輸入了
                in.clear();                  // 清掉 fail bit
                in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "  retry: invalid input, please try again\n";
            }
        };
        std::istringstream in{"abc\n-5\n0\n42\n"};
        int got = readPositiveInt(in);
        std::cout << "[validate] finally got = " << got << '\n';   // 42
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：讀檔時偵測「真的 IO 錯誤」vs「正常讀到 EOF」
    //   重點：迴圈結束後檢查狀態 — 若 bad()，是真的 IO 出錯，要回報；
    //   若只是 eof()，那就是「資料讀完」算正常。
    // ─────────────────────────────────────────────────────────
    {
        std::istringstream in{"10 20 30"};   // 沒有錯誤的資料
        int x;
        long long sum = 0;
        while (in >> x) sum += x;

        if (in.bad()) {
            std::cout << "[diagnose] hard IO error\n";
        } else if (in.eof()) {
            std::cout << "[diagnose] read finished cleanly, sum=" << sum << '\n';
        } else if (in.fail()) {
            std::cout << "[diagnose] parse failed midway\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：cin.fail() 跟 !cin 一樣嗎？
    //    A：幾乎一樣 — `if (!cin)` 等價於 `if (cin.fail() || cin.bad())`，
    //       但 fail() 也包含 badbit，所以實務上多數人就直接寫 if (!cin)。
    //
    //  Q2：fail 不會自動「清」嗎？
    //    A：不會。fail bit 一旦被 set 就會維持到 clear()。這是設計上故意：
    //       讓使用者知道前面有問題，必須處理。
    //
    //  Q3：開 exceptions 之後不需要寫狀態檢查了？
    //    A：對，至少對「明確的 fail / bad」是 throw。但仍可能用 try/catch
    //       漏掉 eof（eof 預設不算 exception，要 in.exceptions(eofbit) 加進
    //       去）。多數人留 eof 在「用迴圈條件偵測」的那一邊。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 09_error_state.cpp -o 09_error_state

// === 預期輸出 ===
// [Demo1] start
//   [init] good=1 eof=0 fail=0 bad=0
//   [after read 42] good=1 eof=0 fail=0 bad=0
//   [after read hello (fail)] good=0 eof=0 fail=1 bad=0
//   [after clear+read string] good=0 eof=1 fail=0 bad=0
//   [after read past end] good=0 eof=1 fail=1 bad=0
// [Demo2] first = 100
// [Demo2] skipped: foo
// [Demo2] last = 200
// [Demo3] caught: basic_ios::clear: iostream error
//   retry: invalid input, please try again
//   retry: invalid input, please try again
//   retry: invalid input, please try again
// [validate] finally got = 42
// [diagnose] read finished cleanly, sum=60
