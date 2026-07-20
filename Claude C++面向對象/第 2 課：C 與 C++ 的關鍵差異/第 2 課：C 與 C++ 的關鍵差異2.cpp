// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 2  —  變數宣告位置：從「區塊開頭」到「使用點」
// =============================================================================
//
// 【主題資訊 Information】
//   語法       ：宣告可出現在任何 statement 允許的位置
//   for 內宣告 ：for (int i = 0; i < n; ++i)   // i 的 scope 只到 for 結束
//   標準版本   ：C++ 自 C++98 起一律允許；C 要到 C99 才允許
//   標頭檔     ：無（語言層級特性）
//   複雜度     ：對 POD 型別無執行期成本；純粹是編譯期的 scope 規則
//
// 【詳細解釋 Explanation】
//
// 【1. C89 為什麼要求「宣告集中在區塊開頭」】
//   這不是設計美學，而是 1970 年代編譯器的現實限制。早期 C 編譯器是
//   single-pass 的：讀到函式開頭就要一次算出「這個 stack frame 要多大」，
//   然後產生一條調整 stack pointer 的指令。若變數可以散落在函式各處，
//   編譯器必須先掃完整個函式才知道 frame 大小 —— 對記憶體只有幾十 KB 的
//   機器來說，多一遍掃描是實質負擔。所以 C89 規定：
//       int main(void) {
//           int a, b, c;      /* 所有宣告必須先寫在這裡 */
//           a = 1;            /* 然後才能寫 statement   */
//       }
//   C99 取消了這個限制（編譯器早已是多遍的了），C++ 從一開始就沒有。
//
// 【2. C++ 為什麼「非取消不可」】
//   C++ 有建構子（constructor）。一個物件被宣告的那一刻就被建構了，
//   所以「宣告的位置」等於「建構的時機」。如果 C++ 沿用 C89 規則，
//   就會被迫寫出這種東西：
//       std::string name;          // 先預設建構一個空字串
//       int fd = open(...);        // ...但這行還沒執行，name 已經活著了
//       name = compute();          // 再指派 → 建構一次 + 指派一次
//   而允許就地宣告後：
//       std::string name = compute();   // 只建構一次，直接就是最終值
//   對有建構子/解構子的型別而言，「延後宣告到有值可給的那一刻」不只是風格，
//   而是實打實少一次建構 + 一次指派。這也是「宣告即初始化」原則的由來。
//
// 【3. for 迴圈變數的 scope：C++ 的一個歷史修正】
//   for (int i = 0; ...) 的 i 屬於 for 自己的 scope，迴圈結束就消失：
//       for (int i = 0; i < 10; ++i) { }
//       // 這裡再用 i 會編譯錯誤：'i' was not declared in this scope
//   但早期（C++ 標準化過程中的草案與舊版 MSVC）i 是洩漏到外層 scope 的，
//   所以「兩個 for 都宣告 int i」會撞名。這是舊教材與新標準最常見的落差之一，
//   舊 MSVC 甚至提供 /Zc:forScope 開關來切換行為。現代編譯器一律是新行為。
//
// 【4. 「縮到最小 scope」為什麼是好習慣】
//   * 變數活著的期間越短，能被誤用的機會越少（未初始化、被前面的邏輯汙染）。
//   * 編譯器的暫存器配置（register allocation）更好做：live range 短，
//     更容易把變數整個放進暫存器而不落到 stack。
//   * 讀者不需要往上捲 30 行去確認這個變數當初是什麼。
//
// 【概念補充 Concept Deep Dive】
//   宣告位置「不等於」記憶體配置位置。編譯器通常在函式進入時就一次把整個
//   frame 開好（一條 sub rsp, N），把所有 local 的 slot 都算進去；
//   把宣告寫在迴圈裡並不會讓程式在每次迭代都去「配置」stack 記憶體。
//   對 POD 型別而言，就地宣告是純粹的編譯期概念，執行期成本為零。
//   真正在迴圈內反覆發生的是「建構與解構」——那只對有 ctor/dtor 的型別成立：
//       for (...) { std::string s = f(); }   // 每次迭代真的建構+解構一次
//       for (...) { int x = f(); }           // 沒有任何額外執行期動作
//
// 【注意事項 Pay Attention】
//   1. 迴圈內宣告「非 POD」型別要留意：每次迭代都會建構 + 解構。若該物件
//      很貴（例如有 heap 配置的 std::string / std::vector），把它移到迴圈外
//      並在迴圈內 clear() 重用，可以省掉反覆的配置。這是取捨，
//      不是一律哪邊比較好——先寫清楚的版本，量測後再改。
//   2. switch 的 case 內直接宣告並初始化變數會編譯錯誤
//      （crosses initialization of ...），因為 case 標籤可以跳過該初始化。
//      解法是用大括號替該 case 開一個新 scope：case 1: { int x = 0; ... }
//   3. 內層 scope 宣告同名變數會遮蔽（shadow）外層，編譯器不會預設報錯；
//      要靠 -Wshadow 才抓得到。這是實務上很常見的 bug 來源。
//   4. C99 之後 C 也允許就地宣告，所以「就地宣告」本身已經不是 C 與 C++ 的
//      差別；真正的差別是 C++ 用它來配合建構子語意。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】變數宣告位置與 scope
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把變數宣告寫在迴圈裡面，會不會比寫在迴圈外面慢？
//     答：分型別看。對 int/double 這種 POD，實務上沒有差別——編譯器在函式
//         進入時就把整個 stack frame 一次開好，迴圈內的宣告只是編譯期的
//         scope 標記，不產生額外的配置指令。但對有建構子/解構子的型別
//         （std::string、std::vector），每次迭代真的會建構一次、解構一次，
//         如果內部有 heap 配置，就是每次迭代一次配置 + 一次釋放。
//     追問：那 std::string 一定要移到迴圈外嗎？→ 不一定。移到外面並在迴圈內
//         clear() 可以保留已配置的 buffer（clear() 不保證歸還 capacity），
//         省掉反覆配置；但也讓變數的 scope 變大、可讀性變差。先確認這個迴圈
//         真的是熱點再改。而且短字串會走 SSO 根本不配置 heap，改了也沒差。
//
// 🔥 Q2. for (int i = 0; ...) 的 i，迴圈結束後還能用嗎？
//     答：不能。i 的 scope 屬於 for 本身，迴圈結束即離開 scope，
//         再使用是編譯錯誤（not declared in this scope）。
//         如果需要「迴圈結束時 i 的值」，必須把 i 宣告在 for 外面。
//     追問：那為什麼有些舊書說可以？→ C++ 標準化早期的草案與舊版 MSVC
//         確實讓 i 洩漏到外層 scope，MSVC 還留了 /Zc:forScope 開關切換。
//         現代編譯器一律是「不洩漏」的標準行為。
//
// ⚠️ 陷阱. 為什麼這段 switch 編譯不過？
//         switch (n) {
//             case 1:
//                 int x = 10;      // error: crosses initialization of 'int x'
//                 break;
//             case 2:
//                 break;
//         }
//     答：case 標籤本質上只是 goto 的目標。當 n == 2 時，控制流會「跳過」
//         x 的初始化直接落到 case 2，但 x 的 scope 卻涵蓋整個 switch 區塊——
//         於是出現「x 在 scope 內但從未被初始化」的狀態，標準禁止這種跳躍。
//         解法是用大括號把該 case 圍成獨立 scope：
//             case 1: { int x = 10; break; }
//     為什麼會錯：多數人腦中把 case 想成「一個獨立的區塊」，
//         但它其實只是同一個 switch 大括號裡的一個標籤，
//         所有 case 共用同一個 scope。
//
// 註：本主題是語言 scope 規則，沒有能誠實對應的 LeetCode 題目
//     （LeetCode 考的是演算法，不會考宣告位置），故不硬湊，僅附日常實務範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 "key=value" 設定行：把每個變數都宣告在有值可給的那一刻
//
// 情境：讀 .ini / .env / .properties 這類設定檔，逐行拆成 key 與 value。
// 為何用到本主題：這個函式刻意示範「宣告即初始化」——每個變數在宣告的當下
//   就已經是最終值，沒有任何「先宣告成空的、之後再指派」的中間狀態。
//   若照 C89 風格把 pos / key / value 全部宣告在函式開頭，
//   std::string 就會先被預設建構成空字串，之後再被指派一次。
// -----------------------------------------------------------------------------
bool parseConfigLine(const std::string& line, std::string& outKey, std::string& outValue) {
    // 註解行與空行直接略過——這個 early return 也讓後面的變數根本不必宣告
    if (line.empty() || line[0] == '#') return false;

    const std::size_t pos = line.find('=');           // 宣告即初始化
    if (pos == std::string::npos) return false;       // 沒有 '=' 就不是設定行

    const std::string key   = line.substr(0, pos);    // 到這裡才需要 key
    const std::string value = line.substr(pos + 1);   // 到這裡才需要 value
    if (key.empty()) return false;

    outKey   = key;
    outValue = value;
    return true;
}

// 示範「縮到最小 scope」：暫存變數只活在它需要的那幾行內
std::size_t countValidLines(const std::vector<std::string>& lines) {
    std::size_t valid = 0;
    for (const std::string& line : lines) {
        // key / value 的 scope 只有這個迴圈本體，離開就消失
        std::string key, value;
        if (parseConfigLine(line, key, value)) ++valid;
    }
    // 這裡再寫 key 會編譯錯誤——正是我們要的：不讓暫存變數逃出去
    return valid;
}

int main() {
    std::cout << "=== 原始示範：宣告可出現在任何位置 ===\n";
    // C++ 可以在任何位置宣告變數
    int sum = 0;

    // 甚至可以在 for 迴圈內宣告
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    // 這裡 i 已經不存在了（scope 只到 for 結束）

    // 需要時才宣告，更清晰
    int result = sum * 2;

    std::cout << "Sum = " << sum << std::endl;
    std::cout << "Result = " << result << std::endl;

    std::cout << "\n=== for 變數的 scope 是獨立的 ===\n";
    // 兩個 for 各自宣告 i，互不干擾——舊行為下這會是重複定義錯誤
    for (int i = 0; i < 3; ++i) std::cout << "第一個迴圈 i=" << i << "\n";
    for (int i = 0; i < 3; ++i) std::cout << "第二個迴圈 i=" << i << "\n";

    std::cout << "\n=== switch 內宣告要自己開 scope ===\n";
    for (int n = 1; n <= 2; ++n) {
        switch (n) {
            case 1: {                       // 大括號讓 x 有自己的 scope
                int x = 10;
                std::cout << "case 1, x=" << x << "\n";
                break;
            }
            case 2: {
                int x = 20;                 // 與上面的 x 完全無關
                std::cout << "case 2, x=" << x << "\n";
                break;
            }
            default:
                break;
        }
    }

    std::cout << "\n=== 日常實務：解析設定行 ===\n";
    const std::vector<std::string> configLines = {
        "# 這是註解",
        "host=192.168.1.10",
        "port=8080",
        "",
        "這行沒有等號",
        "timeout=30"
    };

    for (const std::string& line : configLines) {
        std::string key, value;
        if (parseConfigLine(line, key, value)) {
            std::cout << "  key=[" << key << "] value=[" << value << "]\n";
        } else {
            std::cout << "  略過：[" << line << "]\n";
        }
    }
    std::cout << "有效設定行數 = " << countValidLines(configLines) << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異2.cpp" -o demo2

// === 預期輸出 ===
// === 原始示範：宣告可出現在任何位置 ===
// Sum = 45
// Result = 90
//
// === for 變數的 scope 是獨立的 ===
// 第一個迴圈 i=0
// 第一個迴圈 i=1
// 第一個迴圈 i=2
// 第二個迴圈 i=0
// 第二個迴圈 i=1
// 第二個迴圈 i=2
//
// === switch 內宣告要自己開 scope ===
// case 1, x=10
// case 2, x=20
//
// === 日常實務：解析設定行 ===
//   略過：[# 這是註解]
//   key=[host] value=[192.168.1.10]
//   key=[port] value=[8080]
//   略過：[]
//   略過：[這行沒有等號]
//   key=[timeout] value=[30]
// 有效設定行數 = 3
