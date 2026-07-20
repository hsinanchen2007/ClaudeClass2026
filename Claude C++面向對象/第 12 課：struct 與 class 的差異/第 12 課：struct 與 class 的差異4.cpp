// =============================================================================
//  第 12 課：struct 與 class 的差異 4  —  什麼時候該用 struct（純資料聚合）
// =============================================================================
//
// 【主題資訊 Information】
//   主題：struct 的正確適用場景 —— 沒有不變條件（invariant）要維護的純資料
//   標準版本：NSDMI（成員預設值）需 C++11；配合 aggregate initialization 需 C++14
//   標頭檔：無（語言核心特性）
//   判斷準則：Core Guidelines C.2 ——「若型別有不變條件要維護，用 class；
//             若各資料成員可以獨立自由變動，用 struct」
//
// 【詳細解釋 Explanation】
//
// 【1. 判斷準則只有一句話：有沒有不變條件？】
//   不變條件（invariant）是指「這個物件在任何時刻都必須成立的規則」，例如：
//     * 銀行帳戶：balance 永遠不可為負
//     * 動態陣列：size 永遠 <= capacity
//     * 日期：month 必須落在 1..12
//   只要存在這種跨欄位或有範圍限制的規則，就必須用 class 把資料藏起來，
//   讓所有修改都經過你的檢查。反之，若每個欄位都能獨立、自由地被賦任何值，
//   那封裝只是徒增樣板程式碼 —— 這時 struct 才是對的選擇。
//
// 【2. 為什麼 Point 用 struct 是對的？】
//   Point{x, y} 的 x 與 y 彼此獨立，任何 double 值都是合法座標（含負數、0、
//   甚至極大值）。硬替它寫 getX()/setX() 不會擋掉任何錯誤，只是把
//   `p.x = 3.5` 變成 `p.setX(3.5)`，多打字卻沒有多任何保護。
//   注意本檔的 Color 是個微妙的反例：r/g/b 理論上該落在 0..255。若你真的要
//   保證這個範圍，Color 就該改用 class；本檔維持 struct，是因為教學上把它
//   當成「呼叫端自行負責」的輕量資料袋 —— 這個取捨要自覺，不要含糊帶過。
//
// 【3. aggregate 的價值：不用寫建構函式就能初始化】
//   本檔四個 struct 都是 aggregate（沒有 private 成員、沒有使用者提供的建構函式、
//   沒有虛擬函式、沒有 base class），因此可以直接用大括號逐欄位初始化：
//       Config cfg{1920, 1080, true, 144};
//   而且 C++11 起的 NSDMI 讓你能同時指定預設值，兩者可以混用 —— 沒給的欄位
//   自動吃 NSDMI 的預設值。這讓 struct 幾乎不需要寫任何建構函式。
//
// 【4. designated initializer（C++20）讓設定物件更好讀】
//   C++20 起可以指名欄位初始化：
//       Config cfg{.width = 1920, .height = 1080, .fps = 144};
//   對「欄位很多的設定結構」可讀性提升極大，且未指名的欄位會用 NSDMI 預設值。
//   注意 C++20 的規則比 C 嚴格：順序必須與宣告順序一致，不可跳著亂寫。
//   本檔以 C++17 為主，因此只在註解中示範，未實際使用。
//
// 【概念補充 Concept Deep Dive】
//   * aggregate 的完整條件（C++17）：無 private/protected 非靜態資料成員、
//     無使用者提供或 explicit 的建構函式、無虛擬函式、無 virtual/private/protected
//     的 base class。只要違反任何一條就不再是 aggregate，大括號初始化的語意也隨之改變。
//   * struct 的成員在記憶體中依宣告順序排列（同一 access section 內為標準保證），
//     編譯器會插入 padding 來對齊。把大欄位排在前面、小欄位排在後面通常能減少
//     padding —— 本檔在輸出中實測了 Config 的大小，可自行驗證。
//   * 純資料 struct 通常同時是 trivially copyable 與 standard-layout，因此可以
//     memcpy、可以與 C 互通、可以直接放進共享記憶體。加上 std::string 之後這些
//     性質就全部消失（見本課第 3 檔）。
//
// 【注意事項 Pay Attention】
//   1. 「用 struct 就代表不能有函式」是錯的。加上 distanceTo() 這種不改狀態的
//      便利函式，並不會讓它失去純資料的本質（見本課第 6 檔）。
//   2. 一旦你開始需要「檢查某個欄位的值是否合法」，那就是型別在告訴你
//      「我該升級成 class 了」。不要在 struct 外面到處寫檢查程式碼。
//   3. NSDMI 只在「沒有被明確初始化」時生效。寫 `Config cfg;` 會套用 NSDMI，
//      但若是 `Config cfg{800, 600, false, 30};` 則四個欄位都被顯式覆蓋。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】何時用 struct、何時用 class
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼情況下該用 struct，什麼情況下該用 class？
//     答：判準是「有沒有不變條件要維護」。各成員可獨立自由賦值（座標、設定、
//         RGB 色彩、回傳多值的小包裝）用 struct；有跨欄位規則或範圍限制需要
//         保護（餘額不可為負、size <= capacity）用 class。
//     追問：只是慣例還是編譯器會管？→ 純粹是團隊慣例，編譯器完全不管；
//           它的價值在於讓讀者一眼判斷「這個型別需不需要小心維護狀態」。
//
// 🔥 Q2. 什麼是 aggregate？為什麼它重要？
//     答：沒有 private 非靜態資料成員、沒有使用者提供的建構函式、沒有虛擬函式與
//         虛擬／非 public base class 的 class type。重要在於它能用大括號逐欄位
//         初始化，不必寫任何建構函式，且支援 C++20 的 designated initializer。
//     追問：加一個建構函式會怎樣？→ 立刻不再是 aggregate，Config{1,2,3,4} 這種
//           寫法會失效（改為去比對建構函式多載），現有呼叫端可能整批編譯失敗。
//
// ⚠️ 陷阱. struct 加了成員函式之後，還能不能用 `Point p{1.0, 2.0};` 初始化？
//     答：可以。成員函式（含 static 函式）不影響 aggregate 資格，
//         只有「使用者提供的建構函式」才會取消它。
//     為什麼會錯：把「有函式」和「有建構函式」混為一談。判斷 aggregate 時
//         真正的關鍵字是 constructor，不是 member function。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ✅ 適合用 struct —— 純資料集合
// x 與 y 彼此獨立，任何 double 都是合法座標，沒有不變條件可守
struct Point {
    double x = 0;
    double y = 0;
};

// r/g/b/a 各自獨立。若要強制 0..255 的範圍，就該改用 class 並把欄位藏起來；
// 這裡刻意維持 struct，代表「範圍由呼叫端負責」——這是自覺的取捨，不是疏忽
struct Color {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

struct Config {
    int  width = 800;
    int  height = 600;
    bool fullscreen = false;
    int  fps = 60;
};

struct DateInfo {
    int year = 2025;
    int month = 1;
    int day = 1;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】用 struct 當「函式的多重回傳值」
//   情境：解析視窗解析度設定字串（例如來自設定檔的 "1920x1080@144"）。
//   C 時代要回傳多個值，只能用 out-parameter（把指標傳進去給函式填），
//   呼叫端難讀又容易忘記檢查。改用一個小 struct 當回傳型別後，
//   語意清楚、可以一次帶回「成功與否 + 解析結果」，且完全不需要封裝。
//   這是現代 C++ 中 struct 最常見的用途之一。
// -----------------------------------------------------------------------------
struct ParseResult {
    bool   ok = false;       // 解析是否成功
    Config config{};         // 成功時的結果；失敗時維持預設值
    string error;            // 失敗原因，成功時為空
};

ParseResult parseResolution(const string& text) {
    ParseResult res;
    size_t xPos = text.find('x');
    if (xPos == string::npos) {
        res.error = "缺少 'x' 分隔符";
        return res;                       // 一次帶回失敗與原因
    }

    size_t atPos = text.find('@');        // '@' 之後是 fps，可省略
    try {
        res.config.width  = stoi(text.substr(0, xPos));
        res.config.height = (atPos == string::npos)
                          ? stoi(text.substr(xPos + 1))
                          : stoi(text.substr(xPos + 1, atPos - xPos - 1));
        if (atPos != string::npos) {
            res.config.fps = stoi(text.substr(atPos + 1));
        }
    } catch (const exception& e) {
        res.error = string("數字格式錯誤: ") + e.what();
        return res;
    }

    if (res.config.width <= 0 || res.config.height <= 0) {
        res.error = "寬高必須為正數";
        return res;
    }
    res.ok = true;
    return res;
}

int main() {
    cout << "=== Point：純座標 ===" << endl;
    Point p;
    p.x = 3.5;
    p.y = 7.2;
    cout << "Point(" << p.x << ", " << p.y << ")" << endl;

    // aggregate initialization：不必寫任何建構函式（C++14 起可與 NSDMI 併用）
    Point origin{};             // 兩個欄位都吃 NSDMI 預設值 → (0, 0)
    Point corner{1920, 1080};   // 逐欄位指定
    cout << "origin(" << origin.x << ", " << origin.y << ")"
         << "  corner(" << corner.x << ", " << corner.y << ")" << endl;

    cout << "\n=== Color：RGBA ===" << endl;
    Color red;
    red.r = 255;
    cout << "Color(" << red.r << ", " << red.g << ", " << red.b
         << ", a=" << red.a << ")" << endl;
    Color semiTransparentBlue{0, 0, 255, 128};   // 只寫需要的欄位順序即可
    cout << "半透明藍 alpha = " << semiTransparentBlue.a << endl;

    cout << "\n=== Config：設定物件 ===" << endl;
    Config cfg;                 // 全部套用 NSDMI 預設值
    cout << cfg.width << "x" << cfg.height
         << (cfg.fullscreen ? " 全螢幕" : " 視窗")
         << " @" << cfg.fps << "fps" << endl;
    // C++20 起可寫成 Config hi{.width = 2560, .height = 1440, .fps = 165};
    Config hi{2560, 1440, true, 165};
    cout << hi.width << "x" << hi.height
         << (hi.fullscreen ? " 全螢幕" : " 視窗")
         << " @" << hi.fps << "fps" << endl;

    cout << "\n=== DateInfo ===" << endl;
    DateInfo d{2026, 7, 20};
    cout << d.year << "-" << d.month << "-" << d.day << endl;
    cout << "註：若要強制 month 落在 1..12，DateInfo 就該升級成 class" << endl;

    cout << "\n=== 實務：struct 當多重回傳值 ===" << endl;
    vector<string> inputs = {"1920x1080@144", "800x600", "1920", "abcx100"};
    for (const string& in : inputs) {
        ParseResult r = parseResolution(in);
        cout << "  \"" << in << "\" -> ";
        if (r.ok) {
            cout << "OK " << r.config.width << "x" << r.config.height
                 << " @" << r.config.fps << "fps" << endl;
        } else {
            cout << "失敗: " << r.error << endl;
        }
    }

    cout << "\n=== 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===" << endl;
    cout << "  sizeof(Point)  = " << sizeof(Point)  << endl;
    cout << "  sizeof(Color)  = " << sizeof(Color)  << endl;
    cout << "  sizeof(Config) = " << sizeof(Config)
         << " （3 個 int + 1 個 bool，含對齊 padding）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 12 課：struct 與 class 的差異4.cpp" -o demo4

// === 預期輸出 ===
// === Point：純座標 ===
// Point(3.5, 7.2)
// origin(0, 0)  corner(1920, 1080)
//
// === Color：RGBA ===
// Color(255, 0, 0, a=255)
// 半透明藍 alpha = 128
//
// === Config：設定物件 ===
// 800x600 視窗 @60fps
// 2560x1440 全螢幕 @165fps
//
// === DateInfo ===
// 2026-7-20
// 註：若要強制 month 落在 1..12，DateInfo 就該升級成 class
//
// === 實務：struct 當多重回傳值 ===
//   "1920x1080@144" -> OK 1920x1080 @144fps
//   "800x600" -> OK 800x600 @60fps
//   "1920" -> 失敗: 缺少 'x' 分隔符
//   "abcx100" -> 失敗: 數字格式錯誤: stoi
//
// === 記憶體佈局（實作定義，本機 GCC 15.2 x86-64）===
//   sizeof(Point)  = 16
//   sizeof(Color)  = 16
//   sizeof(Config) = 16 （3 個 int + 1 個 bool，含對齊 padding）
