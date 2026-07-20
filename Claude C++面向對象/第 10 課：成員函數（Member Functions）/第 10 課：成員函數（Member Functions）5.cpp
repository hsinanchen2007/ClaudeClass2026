// =============================================================================
//  第 10 課：成員函數 5  —  成員函數互相呼叫，組合出高階行為
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  類內任一成員函式可直接呼叫其他成員函式，不需限定名稱；
//           f() 等價於 this->f()。
//   標準：  C++98 起即有。
//   標頭檔：<iostream>、<string>
//   複雜度：本檔所有轉換與判斷皆為 O(1) 浮點運算。
//
// 【詳細解釋 Explanation】
//
// 【1. 成員函數互相呼叫 = 把邏輯分層】
//   本檔是個很好的分層樣本，由下而上共三層：
//     第 1 層（原始轉換）：toFahrenheit()、toKelvin()   ← 純計算
//     第 2 層（單一判斷）：isBoiling()、isFreezing()     ← 純述詞（predicate）
//     第 3 層（組合決策）：getState()                    ← 呼叫第 2 層
//     第 4 層（呈現）：    report()                      ← 呼叫 1~3 層
//   每一層只做一件事，且只依賴下一層。這樣的好處是
//   **修改沸點定義時只要改 isBoiling() 一處**，getState() 與 report() 自動跟著對。
//   若把判斷條件直接內嵌在 report() 裡，同樣的 100.0 就會散落好幾份。
//
// 【2. this-> 為什麼可以省略】
//   getState() 裡寫 isFreezing() 時，編譯器做的是：
//   先在類的 scope 找到這個名字 → 發現它是非 static 成員函式
//   → 自動補成 this->isFreezing()。所以整條呼叫鏈上的所有函式
//   操作的都是**同一個物件**的 celsius。
//   ★ 例外：在 template 的衍生類別中呼叫依賴基底類別的成員時，
//     必須寫 this->f() 或 Base<T>::f()，因為 two-phase name lookup
//     不會去 dependent base 裡找 —— 這是 template 章節的經典錯誤。
//
// 【3. 為什麼 getState() 的判斷順序重要】
//   getState() 先問 isFreezing() 再問 isBoiling()。在本檔的定義下
//   （<= 0 為固態、>= 100 為氣態）兩者不可能同時成立，順序不影響結果。
//   但若有人把門檻改成重疊區間，順序就會決定結果 ——
//   這類「條件互斥性」在重構時最容易被打破，是實務上的隱形地雷。
//
// 【4. 這一組函式全都該是 const】
//   toFahrenheit()、toKelvin()、isBoiling()、isFreezing()、getState()
//   都只讀取 celsius，理應宣告成 const 成員函式：
//       double toFahrenheit() const { ... }
//   本檔為配合課程進度未加。後果是：
//       const TemperatureConverter t;
//       t.report();      // ❌ 編譯錯誤：const 物件不能呼叫非 const 成員函式
//   ★ 而且 const 有傳染性：只要 report() 沒加 const，
//     它就無法被任何 const 成員函式呼叫。所以 const 正確性必須**由底層往上**
//     一次補齊，事後補會牽一髮動全身。
//
// 【概念補充 Concept Deep Dive】
//   浮點比較的陷阱：isBoiling() 用 celsius >= 100.0 是安全的（門檻比較），
//   但**絕不能**寫成 celsius == 100.0。因為 0.1 + 0.2 != 0.3 這類
//   二進位浮點表示誤差，等值比較幾乎永遠是錯的。
//   本檔 report() 印出 25 而非 25.00，是因為 std::cout 對 double 預設
//   採用 6 位有效數字並移除尾隨 0（defaultfloat）—— 這是格式化行為，
//   不代表精度遺失。要固定小數位需用 <iomanip> 的 std::fixed/setprecision。
//
//   另外 toKelvin() 回傳 celsius + 273.15：當 celsius = -10 時
//   結果是 263.15，能被精確印出；但若換成某些十進位小數，
//   可能出現 263.14999999999998 這類尾數 —— 這是 IEEE 754 double
//   無法精確表示某些十進位小數的固有結果，不是程式寫錯。
//
// 【注意事項 Pay Attention】
//   1. 成員函式互相呼叫**沒有額外成本疑慮**，但要小心**無窮遞迴**：
//      若 getState() 呼叫 report()、report() 又呼叫 getState()，
//      就會堆疊溢位。分層設計（只往下呼叫）可自然避免。
//   2. 建構函式中呼叫 virtual 成員函式**不會**分派到衍生類別，
//      因為此時衍生部分尚未建構完成（第 20 課以後的重點）。
//   3. 浮點門檻請用 >=／<=，不要用 ==。
//   4. 本檔所有唯讀函式都缺 const —— 這是刻意保留的教學缺口，
//      閱讀時請自行想像加上 const 之後的樣子。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函數的組合與 const 正確性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. getState() 裡直接寫 isFreezing()，這個呼叫是怎麼找到物件的？
//     答：非 static 成員函式有個隱含的 this 參數。
//         isFreezing() 會被編譯器補成 this->isFreezing()，
//         把當前物件位址傳下去，所以整條呼叫鏈操作的都是同一個物件的 celsius。
//     追問：什麼情況下 this-> 不能省略？
//         → 在 class template 的衍生類別中呼叫**依賴基底類別**的成員時，
//           two-phase lookup 不會搜尋 dependent base，
//           必須寫 this->f() 或 Base<T>::f()。
//
// 🔥 Q2. 這些函式為什麼都該加 const？不加會怎樣？
//     答：它們只讀不寫，加上 const 才能被 const 物件、
//         const 引用（很常見的函式參數形式）呼叫。
//         不加的話 void print(const TemperatureConverter& t) 裡
//         連 t.toFahrenheit() 都叫不動。
//     追問：const 成員函式裡到底什麼不能做？
//         → 不能修改非 mutable 的成員、不能呼叫非 const 成員函式。
//           this 的型別從 T* 變成 const T*。
//           若某成員是快取（如 memoization），可標 mutable 讓 const 函式也能改。
//
// ⚠️ 陷阱. 把 isBoiling() 從 celsius >= 100.0 改成 celsius == 100.0，
//         那些「數學上剛好等於 100」的累加結果還會被判定為沸騰嗎？
//     答：**不保證**，而且真正危險的是它「有時對、有時錯」。
//         本機實測：0.1 + 0.2 == 0.3 為 false（差約 4.4e-17），
//         但 0.1 + 0.2 + 99.7 == 100.0 卻湊巧為 true。
//         同樣的開頭、不同的捨入路徑，結論就相反 ——
//         所以浮點門檻永遠用 >= / <=，或比較差值是否小於容忍值 epsilon。
//     為什麼會錯：腦中用的是十進位算術的直覺，以為 0.1+0.2 就是 0.3；
//         但 double 存的是二進位近似值，「數學上相等」不等於「位元樣式相等」。
//         更糟的是測試時可能剛好通過（像上面的 (B)），
//         等換了資料或平台才在正式環境爆掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

class TemperatureConverter {
public:
    double celsius = 0.0;

    // 基礎轉換函數
    double toFahrenheit() {
        return celsius * 9.0 / 5.0 + 32.0;
    }

    double toKelvin() {
        return celsius + 273.15;
    }

    // 判斷函數 —— 調用其他成員函數
    bool isBoiling() {
        return celsius >= 100.0;
    }

    bool isFreezing() {
        return celsius <= 0.0;
    }

    string getState() {
        if (isFreezing()) return "固態（冰）";    // 調用自己的成員函數
        if (isBoiling()) return "氣態（水蒸氣）";
        return "液態（水）";
    }

    // 綜合報告 —— 調用多個成員函數
    void report() {
        cout << "=========================" << endl;
        cout << "攝氏:   " << celsius << " °C" << endl;
        cout << "華氏:   " << toFahrenheit() << " °F" << endl;
        cout << "克式:   " << toKelvin() << " K" << endl;
        cout << "水的狀態: " << getState() << endl;
        cout << "=========================" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【可執行示範】為什麼浮點門檻不能用 ==
//   重點不是「== 一定會錯」，而是「它有時對、有時錯，你無法預測」——
//   這正是它比穩定的錯誤更危險的原因。
//   下面兩組式子在數學上都成立，但用 double 算出來的結果不同調：
//     (A) 0.1 + 0.2 == 0.3          → 本機實測 false（差約 4.4e-17）
//     (B) 0.1 + 0.2 + 99.7 == 100.0 → 本機實測 true（湊巧捨入到剛好）
//   同樣是 0.1 + 0.2 開頭，只因後續運算的捨入方向不同，結論就相反。
//   ★ 這些結果是特定平台／編譯選項下的實測值，換一台機器或改最佳化等級都可能不同。
// -----------------------------------------------------------------------------
void demoFloatEquality() {
    double a = 0.1 + 0.2;
    cout << "  (A) 0.1 + 0.2 的實際值: " << setprecision(17) << a
         << defaultfloat << setprecision(6) << endl;
    cout << "      == 0.3 ? " << (a == 0.3 ? "true" : "false") << "   <- 數學上該成立，卻是 false" << endl;

    double b = 0.1 + 0.2 + 99.7;
    cout << "  (B) 0.1 + 0.2 + 99.7 的實際值: " << setprecision(17) << b
         << defaultfloat << setprecision(6) << endl;
    cout << "      == 100.0 ? " << (b == 100.0 ? "true" : "false") << "   <- 這次湊巧成立" << endl;
    cout << "      >= 100.0 ? " << (b >= 100.0 ? "true" : "false") << "   <- 門檻比較才是穩健寫法" << endl;
    cout << "  ★ 同樣的開頭、相反的結論 —— 所以永遠不要對浮點用 ==" << endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】機房／GPU 溫度告警分級（帶遲滯 hysteresis）
//   情境：監控系統每分鐘讀一次溫度，要決定是否發告警。
//         天真做法是「> 80 就告警」，但溫度在門檻附近抖動時
//         （80.1 → 79.9 → 80.2）會**瘋狂發送與解除告警**，把值班的人洗版。
//         實務解法是**遲滯**：升級用高門檻、降級用較低的門檻，
//         中間那段維持原狀態。
//   為什麼用到本主題：這與本檔 getState() 完全同構 ——
//         底層是單一門檻述詞，上層 evaluate() 組合它們做決策，
//         最上層 describe() 只負責呈現。修改門檻只需動一處。
// -----------------------------------------------------------------------------
class ThermalMonitor {
public:
    // 門檻集中定義在一處，是這個設計最重要的一點
    static constexpr double kCriticalOn  = 87.0;   // 升到 CRITICAL 的門檻
    static constexpr double kCriticalOff = 82.0;   // 降回 WARNING 的門檻（遲滯）
    static constexpr double kWarningOn   = 75.0;
    static constexpr double kWarningOff  = 70.0;

    enum class Level { Normal, Warning, Critical };

    double temp  = 0.0;
    Level  state = Level::Normal;    // 目前狀態，遲滯判斷需要它

    // 第 1 層：單一門檻述詞
    bool aboveCriticalOn()  const { return temp >= kCriticalOn;  }
    bool belowCriticalOff() const { return temp <  kCriticalOff; }
    bool aboveWarningOn()   const { return temp >= kWarningOn;   }
    bool belowWarningOff()  const { return temp <  kWarningOff;  }

    // 第 2 層：組合述詞做狀態轉移（呼叫第 1 層）
    void evaluate() {
        switch (state) {
            case Level::Normal:
                if (aboveCriticalOn())      state = Level::Critical;
                else if (aboveWarningOn())  state = Level::Warning;
                break;
            case Level::Warning:
                if (aboveCriticalOn())      state = Level::Critical;
                else if (belowWarningOff()) state = Level::Normal;
                break;
            case Level::Critical:
                // 只有掉到 kCriticalOff 以下才降級，避免在 87 附近來回跳
                if (belowCriticalOff())     state = Level::Warning;
                break;
        }
    }

    // 第 3 層：呈現（呼叫第 2 層的結果）
    string levelName() const {
        switch (state) {
            case Level::Critical: return "🔴 CRITICAL";
            case Level::Warning:  return "⚠️  WARNING";
            default:              return "✅ NORMAL";
        }
    }

    void feed(double t) {
        temp = t;
        evaluate();
        cout << "  溫度 " << temp << "°C -> " << levelName() << endl;
    }
};

int main() {
    cout << "=== 基本：成員函數分層組合 ===" << endl;
    TemperatureConverter t;

    t.celsius = -10;
    t.report();

    cout << endl;
    t.celsius = 25;
    t.report();

    cout << endl;
    t.celsius = 100;
    t.report();

    cout << "\n=== 陷阱：浮點門檻不可用 == ===" << endl;
    demoFloatEquality();

    cout << "\n=== 日常實務：溫度告警分級（帶遲滯）===" << endl;
    ThermalMonitor m;
    m.feed(65.0);    // NORMAL
    m.feed(76.0);    // 升到 WARNING
    m.feed(72.0);    // 仍在 70~75 之間 → 維持 WARNING（遲滯生效，不會抖回 NORMAL）
    m.feed(88.0);    // 升到 CRITICAL
    m.feed(84.0);    // 仍高於 82 → 維持 CRITICAL（不會一掉到 87 以下就解除）
    m.feed(80.0);    // 低於 82 → 降回 WARNING
    m.feed(68.0);    // 低於 70 → 降回 NORMAL

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）5.cpp" -o member5

// ★ 註：(A)(B) 的浮點結果為本機 g++ 15.2 / x86-64 實測值，

// === 預期輸出 ===
// === 基本：成員函數分層組合 ===
// =========================
// 攝氏:   -10 °C
// 華氏:   14 °F
// 克式:   263.15 K
// 水的狀態: 固態（冰）
// =========================
//
// =========================
// 攝氏:   25 °C
// 華氏:   77 °F
// 克式:   298.15 K
// 水的狀態: 液態（水）
// =========================
//
// =========================
// 攝氏:   100 °C
// 華氏:   212 °F
// 克式:   373.15 K
// 水的狀態: 氣態（水蒸氣）
// =========================
//
// === 陷阱：浮點門檻不可用 == ===
//   (A) 0.1 + 0.2 的實際值: 0.30000000000000004
//       == 0.3 ? false   <- 數學上該成立，卻是 false
//   (B) 0.1 + 0.2 + 99.7 的實際值: 100
//       == 100.0 ? true   <- 這次湊巧成立
//       >= 100.0 ? true   <- 門檻比較才是穩健寫法
//   ★ 同樣的開頭、相反的結論 —— 所以永遠不要對浮點用 ==
//
// === 日常實務：溫度告警分級（帶遲滯）===
//   溫度 65°C -> ✅ NORMAL
//   溫度 76°C -> ⚠️  WARNING
//   溫度 72°C -> ⚠️  WARNING
//   溫度 88°C -> 🔴 CRITICAL
//   溫度 84°C -> 🔴 CRITICAL
//   溫度 80°C -> ⚠️  WARNING
//   溫度 68°C -> ✅ NORMAL
//
//        屬實作定義行為，換平台或改最佳化等級可能不同。
