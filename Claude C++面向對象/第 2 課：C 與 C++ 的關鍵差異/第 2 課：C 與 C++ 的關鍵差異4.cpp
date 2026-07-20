// =============================================================================
//  第 2 課：C 與 C++ 的關鍵差異 4  —  C++ iostream：把型別安全找回來
// =============================================================================
//
// 【主題資訊 Information】
//   語法     ：std::cout << value;          // operator<< 重載
//              std::cin  >> variable;       // operator>> 重載
//   回傳值   ：兩者都回傳 stream 本身的 reference（所以能串接），
//              而 stream 可以隱式轉成 bool（C++11 起是 explicit operator bool）
//   標準版本 ：C++98 起；stream 的 explicit operator bool 是 C++11
//              （C++98 是 operator void*，用途相同但更容易誤用）
//   標頭檔   ：<iostream>（cin/cout/cerr）、<string>、<sstream>（字串串流）
//   複雜度   ：每次 << 是一次函式呼叫；格式狀態查詢是 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. << 為什麼能「自動知道型別」——它根本不是同一個函式】
//   printf 只有一個函式，靠格式字串在執行期猜型別。
//   std::cout << x 則是「多載解析（overload resolution）」：
//       ostream& operator<<(ostream&, int);
//       ostream& operator<<(ostream&, double);
//       ostream& operator<<(ostream&, const char*);
//       template<class T> ostream& operator<<(ostream&, const T&);  // 自訂型別
//   編譯器在「編譯期」根據 x 的靜態型別挑出對應的那一個。
//   所以型別不匹配這件事根本不可能發生——沒有對應的多載就是編譯錯誤，
//   而不是像 printf 那樣編過了、跑起來才爆。
//   這是 C 與 C++ 在 I/O 上最本質的差異：把執行期的猜測換成編譯期的解析。
//
// 【2. 為什麼要回傳 ostream&：串接（chaining）的機制】
//   std::cout << "a" << 1 << "\n"; 的求值方式是由左而右結合：
//       ((std::cout << "a") << 1) << "\n";
//   每個 operator<< 回傳同一個 stream 的 reference，讓下一個 << 接手。
//   注意回傳的是 reference 不是複製——stream 是不可複製的（copy ctor 被刪除），
//   因為它擁有作業系統資源（file descriptor / buffer）。
//
// 【3. 可擴充性：printf 永遠做不到的事】
//   要讓自訂型別能被印出，只要寫一個多載：
//       std::ostream& operator<<(std::ostream& os, const Point& p) {
//           return os << "(" << p.x << ", " << p.y << ")";
//       }
//   之後 std::cout << p; 就能用。printf 沒有 %Point，永遠只能先手動拆成
//   基本型別。這對「物件導向」的 C++ 來說是必需品，而不是加分項。
//
// 【4. cin >> 失敗時到底發生什麼事（C++11 改過，很多書還是舊的）】
//   當 std::cin >> age 讀到 "abc"：
//     * stream 被設上 failbit，之後 (bool)std::cin 為 false；
//     * 壞掉的輸入「留在緩衝區沒有被消耗」，跟 scanf 一樣；
//     * 關鍵差異：C++11 起，失敗時會把目標變數「寫成 0」。
//       C++98 的行為是「不動目標變數」（於是未初始化就變成不定值）。
//       本機 g++ 15.2 / libstdc++ 實測（見本檔輸出）：
//           讀 "abc"                  → failbit，值被寫成 0
//           讀 "99999999999999999999" → failbit，值被夾成 INT_MAX  2147483647
//           讀 "-99999999999999999999"→ failbit，值被夾成 INT_MIN -2147483648
//       這個「out of range 夾到極值並設 failbit」也是 C++11 的規定。
//     * 要繼續讀，必須先 clear() 清狀態，再 ignore() 丟掉壞輸入：
//           std::cin.clear();
//           std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//
// 【5. >> 與 getline 的分工】
//   * >> 會「跳過前導空白，讀到下一個空白為止」——所以讀不到含空格的姓名。
//   * std::getline(is, s) 讀整行（含空格），但不含換行符本身。
//   * 混用兩者的經典坑：先 cin >> age 再 getline，getline 會立刻讀到 age
//     後面那個殘留的換行符而回傳空字串。解法是在中間加一次 ignore()。
//
// 【概念補充 Concept Deep Dive】
//   * std::cin >> name 讀進 std::string 時，不會有 scanf("%s") 那種緩衝區
//     溢位問題——因為 std::string 會自己成長。安全性是型別本身帶來的，
//     不是靠使用者記得寫欄寬。這就是 RAII 與「容器自己管理容量」的價值。
//     （但仍要注意：惡意的超長輸入會讓記憶體用量無上限成長，
//       對外服務仍應自行限制長度，這是 DoS 面向而非記憶體安全面向。）
//   * 效能：iostream 預設與 C stdio 同步（sync_with_stdio(true)），
//     每次操作都要經過額外一層，競賽或大量 I/O 時常見
//         std::ios_base::sync_with_stdio(false);
//         std::cin.tie(nullptr);
//     來加速。代價是之後不能再安全地混用 printf 與 cout。
//   * std::endl 不只是換行，它還會 flush。在迴圈裡大量使用會嚴重拖慢
//     （每次都強制寫出）。多數情況應該用 '\n'，需要時才明確 flush。
//
// 【注意事項 Pay Attention】
//   1. 讀取後一定要檢查 stream 狀態：if (std::cin >> age) { ... }
//      直接把 stream 當條件用，是最慣用也最安全的寫法。
//   2. 失敗後要 clear() + ignore()，只 clear() 不 ignore() 會無窮迴圈。
//   3. >> 讀不到含空白的整行；要整行用 std::getline。
//   4. 迴圈裡用 std::endl 會反覆 flush，效能差；預設請用 '\n'。
//   5. C++11 之後失敗會寫入 0，但「不要依賴這件事來偵測錯誤」——
//      使用者本來就可能真的輸入 0。判斷成功與否請看 stream 狀態。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】iostream 與型別安全
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::cout << x 怎麼知道 x 是什麼型別？跟 printf 差在哪？
//     答：它不需要「知道」——operator<< 有一整組多載，編譯器在編譯期依 x 的
//         靜態型別做多載解析，挑出對應的函式。printf 只有一個函式，
//         靠執行期解讀格式字串猜型別，猜錯就是未定義行為。
//         所以 C++ 把「型別錯誤」從執行期災難變成編譯錯誤。
//     追問：那自訂的 class 要怎麼支援？→ 自己寫一個
//         std::ostream& operator<<(std::ostream&, const T&) 多載即可，
//         這是 printf 永遠做不到的（沒有 %MyClass）。
//
// 🔥 Q2. std::cin >> age 讀到非數字時，age 會變成什麼？
//     答：C++11 起，stream 設上 failbit，並且把 age「寫成 0」。
//         若數值超出範圍（例如 int 讀到 99999999999999999999），
//         則夾到 INT_MAX / INT_MIN 並同樣設 failbit。
//         這跟 scanf 不同——scanf 失敗時「完全不碰」目標變數。
//     追問：那 C++98 呢？→ C++98 的規定是失敗時不修改目標變數，
//         所以未初始化的變數會維持不定值。很多舊教材停在這個版本，
//         回答時最好把版本講清楚。
//
// ⚠️ 陷阱. 這段為什麼會無窮迴圈？
//         int n;
//         while (!(std::cin >> n)) {
//             std::cout << "請重新輸入\n";
//             std::cin.clear();          // 只清狀態
//         }
//     答：clear() 只把 failbit 清掉，讓 stream 「可以再讀」，
//         但那個造成失敗的壞輸入（例如 "abc"）還原封不動留在緩衝區裡。
//         下一圈再讀同一份壞資料，再次失敗，於是永遠轉不出去。
//         必須把壞輸入也丟掉：
//             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//     為什麼會錯：多數人把 clear() 理解成「清空輸入緩衝區」，
//         但它清的是 stream 的「錯誤狀態旗標」，跟緩衝區內容毫無關係。
//         真正清緩衝區的是 ignore()。
//
// 註：本檔主題是 I/O 機制與型別安全，LeetCode 的判題模式是直接把參數餵給
//     函式、不經過 cin/cout，任何題號都套不上，故不附 LeetCode 範例。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 【自訂型別的 operator<<】printf 永遠做不到的事
// -----------------------------------------------------------------------------
struct SensorReading {
    std::string device;
    double      celsius;
    int         humidity;
};

std::ostream& operator<<(std::ostream& os, const SensorReading& r) {
    return os << r.device << " [" << r.celsius << "°C, " << r.humidity << "%]";
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】用 istringstream 解析 CSV 行，並正確處理解析失敗
//
// 情境：從 IoT 裝置收到一行 "sensor-01,23.5,60"，要拆成三個欄位。
// 為何用到本主題：這正是 >> 的型別安全在實務上的樣子——
//   欄位型別由變數決定，而不是由格式字串決定；
//   而且「是否解析成功」用 stream 狀態就能判斷，不必去數回傳了幾個欄位。
// -----------------------------------------------------------------------------
bool parseSensorCsv(const std::string& line, SensorReading& out) {
    std::istringstream iss(line);
    std::string        device, tempStr, humStr;

    if (!std::getline(iss, device,  ',')) return false;
    if (!std::getline(iss, tempStr, ',')) return false;
    if (!std::getline(iss, humStr))       return false;
    if (device.empty()) return false;

    // 對每個欄位各開一個 stream，才能精確判斷「這個欄位」是否合法
    double c = 0.0;
    int    h = 0;
    std::istringstream tempStream(tempStr);
    std::istringstream humStream(humStr);

    // 「讀成功」而且「後面沒有多餘字元」才算合法
    if (!(tempStream >> c) || !(tempStream >> std::ws).eof()) return false;
    if (!(humStream  >> h) || !(humStream  >> std::ws).eof()) return false;

    out.device   = device;
    out.celsius  = c;
    out.humidity = h;
    return true;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】示範 C++11 起 >> 失敗時目標變數的實際值
//
// 為何用到本主題：這是「iostream 與 scanf 錯誤語意差在哪」的可執行證據。
//   全部用 istringstream 對字面字串操作，所以輸出完全可重現、與 stdin 無關。
// -----------------------------------------------------------------------------
void demoExtractionFailure(const std::string& input) {
    int value = 12345;                   // 刻意給一個明顯的初始值
    std::istringstream iss(input);
    const bool ok = static_cast<bool>(iss >> value);

    std::cout << "  輸入 [" << input << "] → 成功=" << std::boolalpha << ok
              << "，變數值=" << value << "\n";
}

int main() {
    // ── 原始示範：C++ 風格互動式輸入 ────────────────────────────────
    // 注意：age 先初始化，而且用 stream 狀態判斷是否成功，
    //       所以任何輸入都不會讓本程式讀到不定值。
    std::string name;
    int         age = 0;

    std::cout << "請輸入你的名字: ";
    const bool gotName = static_cast<bool>(std::cin >> name);

    std::cout << "請輸入你的年齡: ";
    const bool gotAge = static_cast<bool>(std::cin >> age);

    if (gotName && gotAge) {
        std::cout << "你好 " << name << "，你今年 " << age << " 歲" << std::endl;
    } else {
        std::cout << "輸入解析失敗：gotName=" << std::boolalpha << gotName
                  << " gotAge=" << gotAge << std::endl;
    }

    // ── 以下全部對字面字串操作，輸出與 stdin 無關、完全可重現 ────────
    std::cout << "\n=== C++11 起 >> 失敗時目標變數的值（本機 g++ 15.2 實測）===\n";
    demoExtractionFailure("42");                      // 正常
    demoExtractionFailure("abc");                     // 失敗 → 寫成 0
    demoExtractionFailure("99999999999999999999");    // 溢位 → 夾到 INT_MAX
    demoExtractionFailure("-99999999999999999999");   // 溢位 → 夾到 INT_MIN
    std::cout << "  對照：scanf 失敗時「完全不寫入」目標變數，兩者語意不同\n";

    std::cout << "\n=== 自訂型別也能直接 << ===\n";
    const SensorReading r{"sensor-01", 23.5, 60};
    std::cout << "  " << r << "\n";

    std::cout << "\n=== 日常實務：解析 CSV 感測器資料 ===\n";
    const std::vector<std::string> lines = {
        "sensor-01,23.5,60",
        "sensor-02,-8.25,15",
        "sensor-03,abc,60",       // 溫度欄不是數字
        "sensor-04,23.5,60xyz",   // 濕度欄有多餘字元
        ",23.5,60"                // 裝置名稱是空的
    };
    for (const std::string& line : lines) {
        SensorReading out{};
        if (parseSensorCsv(line, out)) {
            std::cout << "  OK   " << out << "\n";
        } else {
            std::cout << "  拒絕 [" << line << "]\n";
        }
    }

    std::cout << "\n=== >> 與 getline 的分工 ===\n";
    {
        std::istringstream iss("Ada Lovelace,1815");
        std::string        word;
        iss >> word;                       // 只讀到第一個空白為止
        std::cout << "  >> 讀到：[" << word << "]（停在空白）\n";

        std::istringstream iss2("Ada Lovelace,1815");
        std::string        whole;
        std::getline(iss2, whole, ',');    // 讀到逗號為止，含空格
        std::cout << "  getline 讀到：[" << whole << "]（含空格）\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2 課：C 與 C++ 的關鍵差異4.cpp" -o demo4
// 執行: printf 'Alice\n20\n' | ./demo4
//
// 說明：本程式開頭會從 stdin 讀取姓名與年齡。下面的預期輸出是以
//       printf 'Alice\n20\n' | ./demo4 取得的實際結果。
//       換不同輸入，開頭那行「你好 ...」會不同；其餘段落全部對字面字串操作，
//       與輸入無關，固定不變。
//       另外兩行提示字串結尾沒有換行，所以會與後續輸出接在同一行。
//       「失敗時夾到 INT_MAX / INT_MIN」是 C++11 起的標準規定，
//       此處數值 2147483647 / -2147483648 對應本機 32-bit int。

// === 預期輸出 ===
// 請輸入你的名字: 請輸入你的年齡: 你好 Alice，你今年 20 歲
//
// === C++11 起 >> 失敗時目標變數的值（本機 g++ 15.2 實測）===
//   輸入 [42] → 成功=true，變數值=42
//   輸入 [abc] → 成功=false，變數值=0
//   輸入 [99999999999999999999] → 成功=false，變數值=2147483647
//   輸入 [-99999999999999999999] → 成功=false，變數值=-2147483648
//   對照：scanf 失敗時「完全不寫入」目標變數，兩者語意不同
//
// === 自訂型別也能直接 << ===
//   sensor-01 [23.5°C, 60%]
//
// === 日常實務：解析 CSV 感測器資料 ===
//   OK   sensor-01 [23.5°C, 60%]
//   OK   sensor-02 [-8.25°C, 15%]
//   拒絕 [sensor-03,abc,60]
//   拒絕 [sensor-04,23.5,60xyz]
//   拒絕 [,23.5,60]
//
// === >> 與 getline 的分工 ===
//   >> 讀到：[Ada]（停在空白）
//   getline 讀到：[Ada Lovelace]（含空格）
