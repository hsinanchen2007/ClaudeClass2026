// =============================================================================
//  第 16 課：初始化列表 8  —  在初始化列表中使用運算式
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  Circle(double r) : radius(r), area(PI * r * r) { }
//           每一項的括號內可以是任意「單一運算式」，不限於直接傳參數。
//   標準版本：C++98 起即有
//   複雜度：取決於運算式本身；本檔的算術與字串串接都是 O(1) 與 O(n)
//   標頭檔：<string>、<cmath>、<numbers>（C++20 才有 std::numbers::pi）
//
// 【詳細解釋 Explanation】
//
// 【1. 初始化列表的括號內可以放什麼】
//   任何能產生該成員型別的**單一運算式**都可以：
//     ● 直接用參數：radius(r)
//     ● 算術運算：  area(PI * r * r)
//     ● 呼叫函數：  hash_(computeHash(key))
//     ● 字串串接：  fullName(last + " " + first)
//     ● 三元運算：  level_(score >= 60 ? "及格" : "不及格")
//   不能放的是**敘述**：沒有 if、沒有 for、沒有多行流程。
//   需要流程控制時，把它包成一個函數，再在初始化列表呼叫該函數。
//
// 【2. 為什麼要在初始化列表算，而不是在函數體算】
//   對 const 成員與參考成員來說這是**唯一**選擇（前面幾個檔案已示範）。
//   對一般成員，好處是「一次到位」：
//       // 函數體版本：先預設建構成空字串，再串接後賦值（兩步）
//       FullName(...) { fullName = last + " " + first; }
//       // 初始化列表版本：直接用串接結果複製建構（一步）
//       FullName(...) : fullName(last + " " + first) { }
//   對 std::string 這種會配置記憶體的型別，差別是實際存在的。
//
// 【3. 用參數，不要用其他成員】
//   注意本檔 Circle 的寫法：
//       : radius(r), area(PI * r * r), circumference(2.0 * PI * r)
//   area 與 circumference 都用**參數 r**，而不是用成員 radius。
//   雖然此處 radius 宣告在前、確實會先初始化，寫成 PI * radius * radius
//   也能正確運作，但用參數是更穩健的習慣：
//     ● 完全不受成員宣告順序影響（見 7.cpp 的教訓）
//     ● 日後有人調整成員宣告順序時，不會默默改變行為
//   這是一條很便宜的防禦性規則，值得養成。
//
// 【4. 運算式的求值時機】
//   每一項的初值運算式，是在「**該成員被初始化的那一刻**」求值，
//   而不是在進入建構函數時全部先算好。
//   所以若初值運算式讀取了另一個成員，是否安全完全取決於宣告順序——
//   這正是 7.cpp 那個 bug 的成因。
//
// 【5. 例外安全的小提醒】
//   若某個成員的初值運算式拋出例外，**已經初始化完成的成員會被正常解構**，
//   但這個物件本身視為從未建構成功，它的解構函數**不會**被呼叫。
//   這對「在建構函數裡配置多個資源」的類別很重要——這也是為什麼
//   應該讓每個資源各自由一個 RAII 成員（如 std::vector、std::unique_ptr）
//   持有，而不是在一個建構函數裡 new 好幾次。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 關於 M_PI：它**不是**標準 C++ 的一部分
//     M_PI 來自 POSIX／傳統 C 函式庫，不在 C++ 標準中。
//     本機 glibc 的 /usr/include/math.h 第 1289 行確實定義了它，
//     但外面包著 `#if defined __USE_MISC || defined __USE_XOPEN` 這類條件；
//     在 MSVC 上則必須先 #define _USE_MATH_DEFINES 才會有。
//     所以「直接用 M_PI」是**不可攜**的寫法。
//
//   ● 原始碼中 #define M_PI ... 為什麼沒有觸發重定義警告
//     因為 glibc 的定義與該行的 token 序列**完全相同**，
//     這在 C/C++ 中稱為 benign redefinition（良性重定義），標準允許且不需診斷。
//     本機實測：內容相同 → 無警告；改成 #define M_PI 3.14（內容不同）
//     → 立刻出現 warning: 'M_PI' redefined。
//     這種「碰巧沒事」的寫法很脆弱，本檔改用自己的具名常數。
//
//   ● 可攜的做法（本檔採用）
//     (a) C++11 起：自己定義 constexpr 常數
//             constexpr double kPi = 3.14159265358979323846;
//     (b) C++20 起：用標準提供的
//             #include <numbers>
//             std::numbers::pi
//     本檔以 C++17 為基準，因此採用 (a)，並在註解說明 (b)。
//
//   ● 為什麼用 constexpr 而不是 #define
//     constexpr 有型別、有作用域、能被除錯器看到、不會意外污染其他識別字；
//     巨集則是純文字替換，沒有任何這些好處。現代 C++ 應避免用巨集定義常數。
//
// 【注意事項 Pay Attention】
//   1. 初始化列表只能放單一運算式，不能放 if／for 等敘述。
//   2. 初值優先使用**參數**而非其他成員，以免受宣告順序影響。
//   3. M_PI 不是標準 C++；請用自訂 constexpr 或 C++20 的 std::numbers::pi。
//   4. 初值運算式若拋出例外，該物件的解構函數不會被呼叫（已建好的成員仍會解構）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】初始化列表中的運算式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 初始化列表的括號內可以放什麼？可以放 if 判斷嗎？
//     答：可以放任何能產生該成員型別的**單一運算式**——算術、函數呼叫、
//         字串串接、三元運算子都行。但不能放敘述，所以沒有 if／for。
//         需要流程控制時，把邏輯包成一個（通常是 static 或 private 的）
//         函數，再在初始化列表呼叫它。
//     追問：為什麼不乾脆全部搬到函數體算就好？
//         → const 成員、參考成員、無預設建構函數的成員在函數體根本無法初始化；
//           而且函數體是賦值，對有建構成本的型別會多做一次白工。
//
// 🔥 Q2. 初值運算式是什麼時候求值的？如果它用到另一個成員安全嗎？
//     答：在「該成員被初始化的那一刻」才求值，不是進建構函數時全部先算好。
//         所以用到另一個成員是否安全，完全取決於那個成員是否**宣告在前**。
//         最穩健的做法是初值只依賴建構函數的**參數**，徹底避開順序問題。
//     追問：那如果初值運算式拋出例外會怎樣？
//         → 已初始化完成的成員會被正常解構，但物件本身視為建構失敗，
//           它的解構函數不會被呼叫。所以每個資源應各自由 RAII 成員持有。
//
// ⚠️ 陷阱. 程式裡直接用 M_PI 算圓面積，編譯通過就代表這樣寫沒問題吧？
//     答：不代表。M_PI 不是 C++ 標準的一部分，它來自 POSIX／傳統 C 函式庫。
//         glibc 在常見設定下會提供，但 MSVC 需要先 #define _USE_MATH_DEFINES，
//         所以這是不可攜的寫法。可攜做法是自己定義 constexpr 常數，
//         或在 C++20 使用 <numbers> 的 std::numbers::pi。
//     為什麼會錯：把「我的編譯器能編」當成「這是標準」。標準函式庫與
//         平台擴充在同一個標頭檔裡混雜出現，很容易誤判；
//         判斷是否標準要查標準文件，不能只看有沒有編過。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// 可攜的圓周率常數（C++11 起可用 constexpr）
// 注意：不要用 M_PI —— 它來自 POSIX，不是 C++ 標準的一部分。
// C++20 起可改用 <numbers> 的 std::numbers::pi。
constexpr double kPi = 3.14159265358979323846;

class Circle {
private:
    double radius;
    double area;
    double circumference;

public:
    // 在初始化列表中使用運算式計算
    // 注意：area 與 circumference 都用「參數 r」，而不是成員 radius，
    //       這樣完全不受成員宣告順序影響（見 7.cpp 的教訓）
    Circle(double r)
        : radius(r),
          area(kPi * r * r),               // 用公式計算
          circumference(2.0 * kPi * r)     // 用公式計算
    { }

    void print() const {
        cout << "  半徑: " << radius << endl;
        cout << "  面積: " << area << endl;
        cout << "  周長: " << circumference << endl;
    }
};

class FullName {
private:
    string firstName;
    string lastName;
    string fullName;

public:
    // 初始化列表中做字串串接：一步建好，不必先預設建構再賦值
    // 同樣用參數 first／last，而不是成員 firstName／lastName
    FullName(const string& first, const string& last)
        : firstName(first),
          lastName(last),
          fullName(last + " " + first)    // 串接
    { }

    void print() const {
        cout << "  姓: " << lastName << endl;
        cout << "  名: " << firstName << endl;
        cout << "  全名: " << fullName << endl;
    }
};

// -----------------------------------------------------------------------------
// 需要流程控制時：把邏輯包成函數，再從初始化列表呼叫
//   初始化列表放不下 if／for，但放得下「函數呼叫」——這就是解法。
// -----------------------------------------------------------------------------
class Grade {
private:
    int score_;
    string level_;      // 由分數換算，需要多重判斷

public:
    explicit Grade(int score)
        : score_(clampScore(score))          // 呼叫函數做夾值
        , level_(toLevel(clampScore(score))) // 呼叫函數做分級
    { }

    void print() const {
        cout << "  分數 " << score_ << " → " << level_ << endl;
    }

private:
    // static 私有輔助函數：不依賴任何成員，因此在初始化列表中呼叫絕對安全
    static int clampScore(int s) {
        if (s < 0)   return 0;
        if (s > 100) return 100;
        return s;
    }

    static string toLevel(int s) {
        if (s >= 90) return "優等";
        if (s >= 80) return "甲等";
        if (s >= 70) return "乙等";
        if (s >= 60) return "丙等";
        return "不及格";
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】監控告警門檻：由基準值推算出一整組衍生欄位
//   情境：監控系統設定某個指標的基準值（例如磁碟使用率 70%），
//         系統要自動推算出警告門檻、嚴重門檻、以及顯示用的說明字串。
//   重點：這些衍生欄位全部在初始化列表一次算好，物件一旦建立就是
//         完全一致的狀態，不會有「建好了但門檻還沒算」的中間狀態。
//         所有初值都只依賴參數，順序完全無關。
// -----------------------------------------------------------------------------
class AlertThreshold {
private:
    string metric_;
    double base_;        // 基準值（百分比）
    double warnAt_;      // 警告門檻
    double critAt_;      // 嚴重門檻
    string summary_;     // 顯示用摘要

public:
    AlertThreshold(const string& metric, double basePercent)
        : metric_(metric)
        , base_(clampPercent(basePercent))
        , warnAt_(clampPercent(basePercent) * 1.15)   // 基準 +15%
        , critAt_(clampPercent(basePercent) * 1.30)   // 基準 +30%
        , summary_(metric + " 監控（基準 "
                   + trimZero(clampPercent(basePercent)) + "%）")
    { }

    void print() const {
        cout << "  " << summary_ << endl;
        cout << "    警告門檻: " << warnAt_ << "%  嚴重門檻: " << critAt_ << "%" << endl;
    }

private:
    static double clampPercent(double v) {
        if (v < 0.0)   return 0.0;
        if (v > 100.0) return 100.0;
        return v;
    }

    // 把 70.000000 這種輸出修短，純顯示用
    static string trimZero(double v) {
        string s = to_string(v);
        size_t dot = s.find('.');
        if (dot == string::npos) return s;
        size_t last = s.find_last_not_of('0');
        if (last == dot) last = dot - 1;       // 整數，連小數點一起去掉
        return s.substr(0, last + 1);
    }
};

int main() {
    cout << "=== Circle：在初始化列表用公式計算 ===" << endl;
    Circle c(5.0);
    c.print();

    cout << "\n=== FullName：在初始化列表做字串串接 ===" << endl;
    FullName fn("信安", "陳");
    fn.print();

    cout << "\n=== 需要 if 判斷時：包成函數再呼叫 ===" << endl;
    Grade g1(95);
    Grade g2(73);
    Grade g3(120);   // 會被夾到 100
    Grade g4(-5);    // 會被夾到 0
    g1.print();
    g2.print();
    g3.print();
    g4.print();

    cout << "\n=== 日常實務：監控告警門檻 ===" << endl;
    AlertThreshold disk("disk_usage", 70.0);
    AlertThreshold mem("memory_usage", 60.0);
    disk.print();
    mem.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）8.cpp" -o demo8
//
// ※ 註：本檔原本使用 M_PI 並自行 #define 一份。因為 M_PI 不是 C++ 標準
//   （來自 POSIX／傳統 C 函式庫，MSVC 需 _USE_MATH_DEFINES），
//   已改為自訂的 constexpr kPi。C++20 之後可改用 <numbers> 的 std::numbers::pi。

// === 預期輸出 ===
// === Circle：在初始化列表用公式計算 ===
//   半徑: 5
//   面積: 78.5398
//   周長: 31.4159
//
// === FullName：在初始化列表做字串串接 ===
//   姓: 陳
//   名: 信安
//   全名: 陳 信安
//
// === 需要 if 判斷時：包成函數再呼叫 ===
//   分數 95 → 優等
//   分數 73 → 乙等
//   分數 100 → 優等
//   分數 0 → 不及格
//
// === 日常實務：監控告警門檻 ===
//   disk_usage 監控（基準 70%）
//     警告門檻: 80.5%  嚴重門檻: 91%
//   memory_usage 監控（基準 60%）
//     警告門檻: 69%  嚴重門檻: 78%
