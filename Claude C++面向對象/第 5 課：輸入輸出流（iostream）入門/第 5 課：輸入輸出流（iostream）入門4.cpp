// =============================================================================
//  第 5 課：輸入輸出流（iostream）入門4.cpp  —  endl / '\n' / flush 的差別
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<iostream>（manipulator 本身宣告於 <ostream>，由 <iostream> 間接引入）
//   簽名：
//     template<class CharT, class Traits>
//     std::basic_ostream<CharT, Traits>& endl  (std::basic_ostream<CharT, Traits>& os);
//     template<class CharT, class Traits>
//     std::basic_ostream<CharT, Traits>& flush (std::basic_ostream<CharT, Traits>& os);
//   等價定義：
//     endl(os)  { os.put(os.widen('\n')); os.flush(); return os; }
//     flush(os) { os.flush(); return os; }
//   標準版本：C++98 起。
//   複雜度：endl / flush 的成本主要來自底層 streambuf 的 sync()，通常對應一次寫出動作。
//
// 【詳細解釋 Explanation】
//
// 【1. 三者到底差在哪——一句話版本】
//     '\n'        ：只把一個換行字元放進緩衝區。
//     std::flush  ：不寫任何字元，只要求把緩衝區內容交出去。
//     std::endl   ：上面兩件事一起做（先 put('\n')，再 flush()）。
//   所以 endl 並不是「比較正式的換行」，它是「換行 + 強制寫出」的組合技。
//
// 【2. 為什麼要有緩衝區？】
//   把資料交給作業系統是有固定成本的（一次呼叫的開銷跟資料量幾乎無關）。
//   若每印一個字元就交出去一次，成本會被放大成千上萬倍。
//   所以 std::cout 底層的 streambuf 會先把輸出累積在記憶體中，
//   等到「緩衝區滿了」「被要求 flush」「程式正常結束」時才真正寫出。
//   endl 等於每一行都主動放棄這個最佳化。
//
// 【3. manipulator 是什麼？為什麼 endl 可以直接放進 << 鏈裡？】
//   endl 不是特殊語法，它就是一個「函式」。ostream 有一個 overload：
//       ostream& operator<<(ostream& (*pf)(ostream&)) { return pf(*this); }
//   也就是「插入一個函式指標時，就呼叫它並把自己傳進去」。
//   這是一個很優雅的擴充點——你也可以自己寫 manipulator：
//       std::ostream& tab(std::ostream& os) { return os << '\t'; }
//       std::cout << "a" << tab << "b";
//   注意 std::endl 是「函式名」不是物件，所以千萬別寫成 std::endl()。
//
// 【4. 什麼時候「必須」flush？】
//   * 程式可能非正常結束（abort、被 signal 殺掉、segfault）——緩衝內容會遺失。
//   * 輸出正被另一支程式即時消費（pipe 給 grep、tail 之類）。
//   * 互動式提示：印出 "請輸入年齡: " 後要等使用者輸入，
//     這時提示必須先出現在螢幕上。
//     不過這個情境通常不需要你手動處理——見下方【概念補充 (B) tie】。
//
// 【概念補充 Concept Deep Dive】
//   (A) 本檔用「計數」而不是「計時」來證明差異
//       計時結果每次執行都不同，不能當作穩定的預期輸出，證據力也弱。
//       本檔改用自訂的 streambuf，直接數 sync()（即 flush）被呼叫了幾次：
//       1000 次 endl → 1000 次 sync；1000 次 '\n' → 0 次 sync。
//       這是決定性的、可重現的，也更能說明「endl 貴在哪裡」。
//
//   (B) std::cin 與 std::cout 是「綁」在一起的（tie）
//       std::cin.tie() 預設指向 &std::cout。
//       意思是：每次要從 cin 讀取之前，會自動先 flush cout。
//       所以「印提示 → 讀輸入」這個常見流程，其實不需要你手動加 endl 或 flush；
//       寫 '\n' 甚至不換行都能正常顯示提示。
//       高效能競賽程式常寫 std::cin.tie(nullptr) 解除這個綁定。
//
//   (C) std::cerr 預設就是 unitbuf
//       std::cerr 的 unitbuf flag 預設為開，代表「每次插入後自動 flush」，
//       這樣錯誤訊息才不會因為程式崩潰而遺失。
//       std::clog 則同樣寫到 stderr，但沒有 unitbuf（有緩衝，適合大量 log）。
//
//   (D) 標準輸出接終端機 vs 接檔案，行為不同（作業系統層）
//       C 的 stdio 在「接終端機」時通常是行緩衝（遇到換行就寫出），
//       接檔案或 pipe 時是全緩衝。這是 libc 的行為，不是 C++ 標準的規定。
//       這解釋了為什麼「同一支程式，直接跑看得到輸出，但 > 導到檔案就好像卡住」。
//       不要依賴這個平台行為，需要即時性就明確 flush。
//
// 【注意事項 Pay Attention】
//   1. 迴圈裡用 std::endl 是最常見的效能反模式。預設用 '\n'，需要時才 flush。
//   2. endl 是函式名，不要加括號寫成 std::endl()。
//   3. 「程式正常結束時輸出會遺失」是錯的——std::cout 在正常結束時會被 flush。
//      會遺失的是 abort() / 未處理 signal / _exit() 這類非正常結束的情況。
//   4. flush 不保證資料已經寫進實體磁碟，只保證交給了作業系統。
//      要保證落盤是 fsync 層級的事，C++ 標準 I/O 不提供這個保證。
//   5. std::endl 只是 flush 一個 stream。cout 與 cerr 是不同的 stream，
//      各自有各自的緩衝，兩者交錯輸出時順序可能不如預期。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】endl / '\n' / flush
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::endl 和 '\n' 的差別是什麼？為什麼說迴圈裡用 endl 是效能問題？
//     答：endl = put('\n') + flush()，'\n' 只是把換行字元放進緩衝區。
//         flush 會要求底層 streambuf 把累積的內容交給作業系統，
//         迴圈中每行都 flush 等於完全放棄緩衝這個最佳化。
//         本檔實測：1000 行用 endl 觸發 1000 次 sync，用 '\n' 觸發 0 次。
//     追問：那什麼時候「應該」用 endl？
//         → 程式可能非正常結束、輸出要被即時消費、或寫入 log 後隨即可能崩潰時。
//
// 🔥 Q2. 為什麼「印提示 → std::cin >> x」不用手動 flush，提示就會出現？
//     答：因為 std::cin.tie() 預設指向 &std::cout。
//         標準規定從 cin 讀取前會先 flush 被 tie 的那個 ostream，
//         所以提示會自動被推出去。若寫了 std::cin.tie(nullptr) 解除綁定，
//         就必須自己 flush，否則提示可能還留在緩衝區裡。
//
// ⚠️ 陷阱. 「不用 endl 的話，程式結束時緩衝區的東西會不會遺失？」
//     答：正常結束不會。std::cout 在 main 正常返回或呼叫 exit() 時會被 flush。
//         真正會遺失的是非正常結束：abort()、未處理的 signal、_exit()。
//     為什麼會錯：多數人把「緩衝」想成「不可靠」，於是到處灑 endl 求心安。
//         實際上該擔心的不是正常結束，而是崩潰路徑；
//         而崩潰路徑該用的是 std::cerr（預設 unitbuf）而不是到處用 endl。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <ostream>
#include <streambuf>
#include <string>   // writeLog 用到 std::string；不要依賴 <iostream> 的間接引入

// -----------------------------------------------------------------------------
// 計數用的 streambuf：不真的輸出任何東西，只統計
//   * sync()  被呼叫幾次  → 等同「flush 了幾次」
//   * 總共寫入幾個字元
// 用「計數」而不是「計時」，結果才是決定性、可重現的。
// -----------------------------------------------------------------------------
class CountingBuf : public std::streambuf {
public:
    int         syncCount = 0;
    std::size_t charCount = 0;

    void reset() {
        syncCount = 0;
        charCount = 0;
    }

protected:
    // 沒有設定內部緩衝區，所以每個字元都會走到 overflow
    int overflow(int ch) override {
        if (ch != traits_type::eof()) {
            ++charCount;
        }
        return ch;
    }

    // 一次寫入多個字元時走這裡（例如插入一個字串）
    std::streamsize xsputn(const char* /*s*/, std::streamsize n) override {
        charCount += static_cast<std::size_t>(n);
        return n;
    }

    // flush 最終會呼叫到這裡
    int sync() override {
        ++syncCount;
        return 0;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】依「嚴重程度」決定要不要 flush 的 log 函式
// 情境：真實服務的 log 量很大，每行都 flush 會拖垮吞吐量；
//       但 ERROR 等級的訊息又必須保證「就算下一秒程式崩潰也留得下來」。
//       實務上的折衷就是：一般訊息用 '\n'，錯誤訊息才 flush。
// 為何用本主題：這正是 endl 與 '\n' 該如何取捨的真實答案——
//       不是二選一，而是依訊息重要性決定。
// -----------------------------------------------------------------------------
enum class LogLevel { Debug, Info, Error };

void writeLog(std::ostream& os, LogLevel level, const std::string& msg) {
    const char* tag = "";
    switch (level) {
        case LogLevel::Debug: tag = "[DEBUG]"; break;
        case LogLevel::Info:  tag = "[INFO ]"; break;
        case LogLevel::Error: tag = "[ERROR]"; break;
    }

    os << tag << ' ' << msg << '\n';       // 一律先用 '\n'，不強制 flush

    if (level == LogLevel::Error) {
        os << std::flush;                  // 只有 ERROR 才付出 flush 的成本
    }
}

// 【LeetCode 實戰範例】—— 本檔從缺，並說明理由
//   緩衝與 flush 屬於 I-O 效能與作業系統互動的議題，
//   不對應任何 LeetCode 演算法題。LeetCode 的評測甚至不看 stdout。
//   硬湊一題不相關的題目只會誤導，故此處留白。

int main() {
    // ── 原始教學範例：三種寫法的外觀 ──────────────────────────
    std::cout << "=== 三種換行/清空寫法（原始範例）===" << std::endl;

    // 方式一：std::endl（換行 + 清空緩衝區）
    std::cout << "Line 1" << std::endl;

    // 方式二：'\n'（只換行，不清空緩衝區）
    std::cout << "Line 2\n";

    // 方式三：std::flush（只清空緩衝區，不換行）
    std::cout << "Line 3" << std::flush;
    std::cout << " continued" << std::endl;

    // 注意：以上四行「看起來」完全一樣，因為外觀不是差別所在。
    //       真正的差別在「flush 了幾次」，用肉眼看不出來——所以要用計數。

    // ── 用計數證明差異（決定性、可重現）────────────────────────
    std::cout << "\n=== 用 sync() 次數證明 endl 的成本 ===" << std::endl;

    CountingBuf buf;
    std::ostream probe(&buf);
    const int N = 1000;

    buf.reset();
    for (int i = 0; i < N; ++i) {
        probe << 'x' << std::endl;          // 每行都 flush
    }
    const int syncWithEndl = buf.syncCount;
    const std::size_t charsWithEndl = buf.charCount;

    buf.reset();
    for (int i = 0; i < N; ++i) {
        probe << 'x' << '\n';               // 只放換行字元
    }
    const int syncWithNewline = buf.syncCount;
    const std::size_t charsWithNewline = buf.charCount;

    std::cout << "輸出 " << N << " 行：\n";
    std::cout << "  用 std::endl -> sync() 被呼叫 " << syncWithEndl
              << " 次，寫入 " << charsWithEndl << " 個字元\n";
    std::cout << "  用 '\\n'      -> sync() 被呼叫 " << syncWithNewline
              << " 次，寫入 " << charsWithNewline << " 個字元\n";
    std::cout << "  兩者寫出的字元數完全相同: " << std::boolalpha
              << (charsWithEndl == charsWithNewline) << std::noboolalpha << '\n';
    std::cout << "  差別純粹在 flush 次數，這就是 endl 的成本來源。\n";

    // ── std::flush 只 flush、不換行 ───────────────────────────
    std::cout << "\n=== std::flush 不會產生換行字元 ===" << std::endl;
    buf.reset();
    probe << std::flush;
    std::cout << "只插入 std::flush -> sync 次數 = " << buf.syncCount
              << "，字元數 = " << buf.charCount << "（沒有寫入任何字元）\n";

    // ── cin 與 cout 的 tie 關係 ───────────────────────────────
    std::cout << "\n=== std::cin 預設 tie 到 std::cout ===" << std::endl;
    std::cout << "cin.tie() == &cout: " << std::boolalpha
              << (std::cin.tie() == &std::cout) << std::noboolalpha << '\n';
    std::cout << "所以「印提示 -> 讀輸入」不需要手動 flush。\n";

    // ── cerr 預設 unitbuf ─────────────────────────────────────
    std::cout << "\n=== std::cerr 預設開啟 unitbuf ===" << std::endl;
    std::cout << "cerr 有 unitbuf: " << std::boolalpha
              << ((std::cerr.flags() & std::ios_base::unitbuf) != 0) << '\n';
    std::cout << "cout 有 unitbuf: "
              << ((std::cout.flags() & std::ios_base::unitbuf) != 0)
              << std::noboolalpha << '\n';

    // ── 日常實務：依等級決定是否 flush ────────────────────────
    std::cout << "\n=== 日常實務: 依 log 等級決定要不要 flush ===" << std::endl;
    writeLog(std::cout, LogLevel::Debug, "cache hit for key=user:1001");
    writeLog(std::cout, LogLevel::Info,  "request completed in 12ms");
    writeLog(std::cout, LogLevel::Error, "database connection lost");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 5 課：輸入輸出流（iostream）入門4.cpp" -o io4

// 說明（證據形式與環境相關性）：
//   * 本檔刻意用「sync() 被呼叫的次數」而不是耗時來證明 endl 的成本。
//     計數是決定性的、每次執行都相同；耗時則每次不同且受機器負載影響。
//   * sync 次數 1000 / 0 是 C++ 標準對 endl 與 '\n' 語意的直接推論
//     （endl 定義就含 flush），並非實作定義的巧合。
//   * std::cerr 預設帶 unitbuf、std::cin 預設 tie 到 std::cout，
//     兩者皆為標準規定（[iostream.objects]），故輸出中的 true/false 穩定。
//   * 原始範例那四行 "Line 1..3 continued" 的外觀在三種寫法下相同，
//     差別看不出來——這正是本檔要用計數補強的原因。
//   以下輸出為實跑擷取，已多次執行確認完全一致。

// === 預期輸出 ===
// === 三種換行/清空寫法（原始範例）===
// Line 1
// Line 2
// Line 3 continued
//
// === 用 sync() 次數證明 endl 的成本 ===
// 輸出 1000 行：
//   用 std::endl -> sync() 被呼叫 1000 次，寫入 2000 個字元
//   用 '\n'      -> sync() 被呼叫 0 次，寫入 2000 個字元
//   兩者寫出的字元數完全相同: true
//   差別純粹在 flush 次數，這就是 endl 的成本來源。
//
// === std::flush 不會產生換行字元 ===
// 只插入 std::flush -> sync 次數 = 1，字元數 = 0（沒有寫入任何字元）
//
// === std::cin 預設 tie 到 std::cout ===
// cin.tie() == &cout: true
// 所以「印提示 -> 讀輸入」不需要手動 flush。
//
// === std::cerr 預設開啟 unitbuf ===
// cerr 有 unitbuf: true
// cout 有 unitbuf: false
//
// === 日常實務: 依 log 等級決定要不要 flush ===
// [DEBUG] cache hit for key=user:1001
// [INFO ] request completed in 12ms
// [ERROR] database connection lost
