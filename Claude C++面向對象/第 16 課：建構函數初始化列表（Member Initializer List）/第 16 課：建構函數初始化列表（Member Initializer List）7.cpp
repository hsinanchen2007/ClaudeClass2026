// =============================================================================
//  第 16 課：初始化列表 7  —  順序錯誤造成的真實 bug（危險示範）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：當成員之間有依賴時，初始化順序寫錯會讀到尚未初始化的成員
//   標準版本：C++98 起即有此規則
//   相關診斷：-Wreorder（已包含在 -Wall 中）
//   本檔性質：**危險示範**。DangerousOrder 這個類別故意保留錯誤寫法，
//             但**刻意不建立它的物件**——因為建立它就會讀取未初始化的
//             成員，那是未定義行為。要看到症狀請自行取消註解（後果自負）。
//   標頭檔：<iostream>、<vector>
//
//   ※ 本檔會**故意**產生 -Wreorder 警告，那正是本檔的教學重點。
//
// 【詳細解釋 Explanation】
//
// 【1. 上一個檔案的規則，在這裡變成真的 bug】
//   6.cpp 示範了「順序由宣告決定」，但那個例子的成員互不依賴，
//   所以順序錯了也看不出差別。本檔把依賴關係加進來，問題就浮現了：
//       class DangerousOrder {
//           int length;    // 第 1 個宣告 → 先初始化
//           int* data;     // 第 2 個宣告 → 後初始化
//           DangerousOrder(int len)
//               : data(new int[length]),   // 寫在前面，但實際上「後」執行
//                 length(len)              // 寫在後面，但實際上「先」執行
//       };
//   乍看之下「data 用 length 來配置，而 length 就寫在下一行」很合理。
//   但實際執行順序是 length 先、data 後嗎？**是的**——問題在於
//   `new int[length]` 這個運算式，它是 data 的初值，會在 **data 初始化時**
//   才求值，而那已經是 length 之後了……
//
//   等一下，這樣不是剛好沒問題嗎？**不對**。請仔細看清楚：
//   宣告順序是 length、data，所以執行順序是 length 先、data 後。
//   而 `new int[length]` 是在初始化 data 時求值的，此時 length **已經**
//   初始化完成了。所以這個特定的例子其實是安全的。
//
//   真正的 bug 在「宣告順序反過來」的時候——這才是本檔的核心。
//
// 【2. 真正會出事的宣告順序（本檔的 ReallyDangerous）】
//       class ReallyDangerous {
//           int* data;     // 第 1 個宣告 → **先**初始化
//           int length;    // 第 2 個宣告 → **後**初始化
//           ReallyDangerous(int len)
//               : length(len),               // 寫在前面，但實際「後」執行
//                 data(new int[length])      // 寫在後面，但實際「先」執行
//                                            // ← 此時 length 還沒初始化！
//       };
//   這裡 data 先初始化，而它的初值運算式 `new int[length]` 讀取了
//   **尚未初始化的 length**——這是未定義行為。
//   常見的災難後果包括：配置出巨大到失敗的大小、配置出 0 或極小的緩衝區
//   而後續寫入時越界。但請注意：**不能說它「一定會」怎樣**，
//   因為讀取不定值之後整個程式的行為都失去保證。
//
// 【3. 這個 bug 的真實案例特徵】
//   這正是實務上最惡名昭彰的初始化順序 bug 樣板：
//       char* m_data;      // 宣告在前
//       size_t m_len;      // 宣告在後
//       String(const char* s) : m_len(strlen(s)), m_data(new char[m_len + 1])
//   看起來完全合理——先算長度、再依長度配置。但因為 m_data 宣告在前，
//   它會**先**初始化，此時 m_len 還是不定值，於是 `new char[m_len + 1]`
//   用一個不定值去配置記憶體。
//   症狀通常是「後面 strcpy 時 heap buffer overflow」，
//   而在偵錯工具（如 AddressSanitizer）下會直接指出溢位點；
//   有時則表現為配置失敗拋出 std::bad_alloc——但那只是**症狀**，
//   真正的根因是這裡的初始化順序。
//
// 【4. 三個層次的修法】
//   (a) **治本**：讓被依賴的成員宣告在前面。
//       int length; int* data;   → length 先初始化，data 才用得到它。
//   (b) **輔助**：初始化列表的書寫順序 = 宣告順序，讓程式碼不會誤導人。
//   (c) **防呆**：不要依賴另一個成員，直接用建構函數的**參數**。
//       : data(new int[len]), length(len)
//       參數 len 在建構函數被呼叫時就已經有值，完全不受成員順序影響。
//       這是最穩健的做法，本檔的 SafestOrder 採用它。
//   最根本的建議：能用標準容器就別自己管記憶體。std::vector<int> 一行解決，
//   而且順序、例外安全、解構全都不用操心。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 為什麼 -Wreorder 抓得到、卻不能完全依賴它
//     -Wreorder 只比對「書寫順序 vs 宣告順序」，它不分析依賴關係。
//     所以：
//       - 順序不一致但無依賴 → 有警告，其實沒事（6.cpp 的情況）
//       - 順序一致但有依賴且宣告順序本身就錯 → **沒有警告，卻是真 bug**
//     後者正是最危險的組合：程式碼看起來整整齊齊，編譯器安安靜靜。
//     所以真正的防線是「被依賴者宣告在前」與「改用參數」，警告只是輔助。
//
//   ● 用參數而非成員，為什麼更安全
//     建構函數的參數在函數被呼叫的那一刻就全部準備好了，
//     它們的生命週期與初始化順序完全無關。
//     只要初值運算式只依賴參數（不依賴其他成員），順序問題就從根本消失。
//
//   ● 偵測工具，以及它們各自抓不到什麼
//     這類 bug 靠肉眼很難抓，建議在測試階段加上：
//         g++ -fsanitize=address,undefined -g
//     但要清楚它們的分工：
//       - AddressSanitizer 抓的是**下游症狀**（越界讀寫、use-after-free），
//         會在越界的當下報錯並指出位置。它**不會**偵測「讀取未初始化的值」。
//       - 真正針對未初始化讀取的是 MemorySanitizer（Clang 的 -fsanitize=memory），
//         但它要求整個程式（含標準函式庫）都以該模式建置，門檻較高。
//     實測佐證：把本檔的 ReallyDangerous 單獨取出以 ASan+UBSan 執行，
//     它可能**完全不報錯、看起來一切正常**——因為配置剛好成功了。
//     這正好說明 UB 的危險：工具沒響不代表沒問題，
//     真正的防線仍然是「被依賴者宣告在前」與「初值只用參數」。
//
// 【注意事項 Pay Attention】
//   1. 讀取未初始化的成員是未定義行為，不可描述成固定結果。
//   2. 本檔的 ReallyDangerous 與 DangerousOrder 都**不建立物件**；
//      要觀察症狀請自行取消註解，並務必搭配 AddressSanitizer。
//   3. -Wreorder 抓不到「宣告順序本身就錯」的情況，別把它當唯一防線。
//   4. 需要動態陣列時優先用 std::vector，不要自己 new[]／delete[]。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】初始化順序造成的 bug
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 下面這段程式碼有什麼問題？
//        class String {
//            char* m_data;
//            size_t m_len;
//            String(const char* s) : m_len(strlen(s)), m_data(new char[m_len+1]) {}
//        };
//     答：m_data 宣告在 m_len 之前，所以 m_data 會**先**初始化，
//         而它的初值 new char[m_len+1] 讀取了尚未初始化的 m_len——
//         未定義行為。配置出來的大小不可預期，後續複製字串時很可能越界。
//     追問：怎麼修？
//         → 把 m_len 宣告在 m_data 前面（治本），或初值直接用參數
//           new char[strlen(s)+1]（防呆），最好是改用 std::string／std::vector。
//
// 🔥 Q2. 開了 -Wall（含 -Wreorder）就能擋住這類 bug 嗎？
//     答：不能。-Wreorder 只比對書寫順序與宣告順序是否一致，
//         不分析成員之間的依賴。若宣告順序本身就錯，而初始化列表又
//         「乖乖照著宣告順序寫」，就完全不會有警告，但 bug 依然存在。
//     追問：那還有什麼防線？
//         → 讓被依賴者宣告在前、初值盡量只用建構函數參數，
//           並在測試時開 -fsanitize=address,undefined。
//
// ⚠️ 陷阱. 程式跑起來拋出 std::bad_alloc，所以是記憶體不足造成的，對嗎？
//     答：多半不是。在這類順序 bug 中，bad_alloc 常常只是**症狀**：
//         因為用一個不定值當作配置大小，剛好算出一個天文數字而配置失敗。
//         真正的根因在初始化順序，不是機器記憶體不夠。
//     為什麼會錯：看到例外就對著例外的字面意思找原因。
//         正確的做法是往回追「這個大小是怎麼算出來的」，
//         此時 AddressSanitizer 或除錯器印出該值，往往一眼就看出是垃圾值。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
using namespace std;

// -----------------------------------------------------------------------------
// 情況 A：宣告順序 length → data（安全，但書寫順序誤導人）
//   實際順序：length 先、data 後。data 的初值 new int[length] 求值時，
//   length 已經初始化完成，所以這個類別**其實是安全的**。
//   問題只在於「書寫順序讓人以為 data 先」，因此仍會有 -Wreorder 警告。
//   ※ 本類別安全，但為了不混淆教學重點，同樣不在 main 中建立物件。
// -----------------------------------------------------------------------------
class DangerousOrder {
private:
    int length;    // 第 1 個宣告 → 先初始化
    int* data;     // 第 2 個宣告 → 後初始化

public:
    DangerousOrder(int len)
        : data(new int[length]),  // 寫在第 1 個，但實際第 2 個執行；
                                  // 此時 length 已初始化，故此例安全
          length(len)             // 寫在第 2 個，但實際第 1 個執行
    {
    }

    ~DangerousOrder() { delete[] data; }
};

// -----------------------------------------------------------------------------
// 情況 B：宣告順序 data → length（**真正的 bug**）
//   實際順序：data 先、length 後。data 的初值 new int[length] 求值時，
//   length **尚未初始化**，讀取它是未定義行為。
//   ※ 刻意不建立物件。要觀察症狀請自行取消 main 中的註解，
//     並務必搭配 -fsanitize=address,undefined。
// -----------------------------------------------------------------------------
class ReallyDangerous {
private:
    int* data;     // 第 1 個宣告 → **先**初始化
    int length;    // 第 2 個宣告 → **後**初始化

public:
    ReallyDangerous(int len)
        : length(len),             // 寫在前，但實際「後」執行
          data(new int[length])    // 寫在後，但實際「先」執行；
                                   // ← 讀取尚未初始化的 length：未定義行為
    {
    }

    ~ReallyDangerous() { delete[] data; }
};

// -----------------------------------------------------------------------------
// 修法 (a) + (b)：被依賴者宣告在前，且書寫順序 = 宣告順序
// -----------------------------------------------------------------------------
class SafeOrder {
private:
    int length;    // 被依賴者，宣告在前
    int* data;

public:
    SafeOrder(int len)
        : length(len),            // 先初始化 length
          data(new int[length])   // 再用它配置（此時 length 已有值）
    {
        for (int i = 0; i < length; ++i) data[i] = i * i;
        cout << "  SafeOrder 安全配置了 " << length << " 個元素" << endl;
    }

    ~SafeOrder() { delete[] data; }

    void print() const {
        cout << "    內容: ";
        for (int i = 0; i < length; ++i) {
            if (i) cout << " ";
            cout << data[i];
        }
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 修法 (c)：初值只依賴「參數」，完全不受成員順序影響（最穩健）
//   刻意把 data 宣告在 length 前面，證明即使順序「不利」也不會出事
// -----------------------------------------------------------------------------
class SafestOrder {
private:
    int* data;     // 就算宣告在前也沒關係
    int length;

public:
    SafestOrder(int len)
        : data(new int[len]),   // 用參數 len，不用成員 length
          length(len)
    {
        for (int i = 0; i < length; ++i) data[i] = i + 1;
        cout << "  SafestOrder 用參數配置了 " << length << " 個元素（順序無關）" << endl;
    }

    ~SafestOrder() { delete[] data; }

    void print() const {
        cout << "    內容: ";
        for (int i = 0; i < length; ++i) {
            if (i) cout << " ";
            cout << data[i];
        }
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】最佳解：改用 std::vector，讓整個問題消失
//   情境：影像處理要配置一塊 width × height 的像素緩衝區。
//         用裸指標就得處理配置大小、解構、複製、例外安全；
//         用 std::vector 全部自動搞定，而且初值只依賴參數。
//   重點：真正的資深寫法不是「小心翼翼地排好順序」，
//         而是「選一個根本不會有這個問題的工具」。
// -----------------------------------------------------------------------------
class PixelBuffer {
private:
    size_t width_;
    size_t height_;
    vector<unsigned char> pixels_;   // 自動管理記憶體，不必寫解構函數

public:
    PixelBuffer(size_t width, size_t height)
        : width_(width)
        , height_(height)
        , pixels_(width * height, 0)   // 只依賴「參數」，順序完全無關
    { }

    void setPixel(size_t x, size_t y, unsigned char v) {
        if (x < width_ && y < height_) pixels_[y * width_ + x] = v;
    }

    void print() const {
        cout << "  " << width_ << "x" << height_
             << " 緩衝區，共 " << pixels_.size() << " 位元組" << endl;
        for (size_t y = 0; y < height_; ++y) {
            cout << "    ";
            for (size_t x = 0; x < width_; ++x) {
                cout << (pixels_[y * width_ + x] ? '#' : '.');
            }
            cout << endl;
        }
    }
};

int main() {
    cout << "=== 危險示範：兩個類別都刻意不建立物件 ===" << endl;
    // DangerousOrder d(10);    // 此例其實安全，但書寫順序誤導人
    // ReallyDangerous r(10);   // ← 真正的 UB：讀取未初始化的 length
    cout << "  DangerousOrder：宣告 length→data，實際安全但書寫順序誤導" << endl;
    cout << "  ReallyDangerous：宣告 data→length，讀取未初始化成員（UB）" << endl;
    cout << "  兩者皆未建立物件；要觀察症狀請取消註解並開 AddressSanitizer" << endl;

    cout << "\n=== 修法 (a)(b)：被依賴者宣告在前 ===" << endl;
    SafeOrder s(6);
    s.print();

    cout << "\n=== 修法 (c)：初值只用參數，順序無關 ===" << endl;
    SafestOrder s2(6);
    s2.print();

    cout << "\n=== 最佳解：改用 std::vector ===" << endl;
    PixelBuffer buf(8, 4);
    buf.setPixel(0, 0, 255);
    buf.setPixel(7, 3, 255);
    buf.setPixel(3, 1, 255);
    buf.setPixel(4, 2, 255);
    buf.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）7.cpp" -o demo7
// 觀察危險示範的症狀（取消 main 中的註解後）:
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g <檔名> -o demo7
//
// ※ 重要說明（放在預期輸出標記之前）：
//   1. 本檔**刻意**保留 -Wreorder 警告（DangerousOrder 與 ReallyDangerous），
//      因為「書寫順序不等於實際順序」正是本檔要教的內容。
//      編譯時看到這些警告是預期行為。
//   2. 兩個危險類別都沒有建立物件，所以本檔實際執行時**不會**觸發
//      未定義行為，下方輸出是完全確定的。

// === 預期輸出 ===
// === 危險示範：兩個類別都刻意不建立物件 ===
//   DangerousOrder：宣告 length→data，實際安全但書寫順序誤導
//   ReallyDangerous：宣告 data→length，讀取未初始化成員（UB）
//   兩者皆未建立物件；要觀察症狀請取消註解並開 AddressSanitizer
//
// === 修法 (a)(b)：被依賴者宣告在前 ===
//   SafeOrder 安全配置了 6 個元素
//     內容: 0 1 4 9 16 25
//
// === 修法 (c)：初值只用參數，順序無關 ===
//   SafestOrder 用參數配置了 6 個元素（順序無關）
//     內容: 1 2 3 4 5 6
//
// === 最佳解：改用 std::vector ===
//   8x4 緩衝區，共 32 位元組
//     #.......
//     ...#....
//     ....#...
//     .......#
