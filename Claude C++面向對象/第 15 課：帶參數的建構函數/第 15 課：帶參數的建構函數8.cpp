// =============================================================================
//  第 15 課：帶參數的建構函數 8  —  大括號初始化與窄化轉換的防護
// =============================================================================
//
// 【主題資訊 Information】
//   對照：  Holder h1(pi);    // 小括號：允許窄化，double 3.14 → int 3
//           Holder h2{pi};    // 大括號：標準規定為 ill-formed（窄化轉換）
//           Holder h3{3};     // 大括號：int → int 無窄化，合法
//   標準版本：大括號初始化與窄化規則都是 C++11 引入
//   複雜度：O(1)，窄化檢查完全在編譯期完成，無執行期成本
//   標頭檔：<iostream>
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼叫「窄化轉換」】
//   標準列舉了幾類會遺失資訊的隱式轉換，統稱 narrowing conversion：
//     ● 浮點 → 整數：double → int（小數部分直接被截掉）
//     ● 較寬浮點 → 較窄浮點：double → float（精度可能不足）
//     ● 整數 → 浮點：int → float（大整數可能無法精確表示）
//     ● 較寬整數 → 較窄整數：long → short、int → char（可能溢位）
//     ● 整數／未定範圍列舉 → 無法容納其所有值的型別
//   共同點是「來源型別的某些值，目標型別裝不下」。
//
// 【2. 為什麼 C++11 要在大括號裡禁止它】
//   C 語言傳統上放行這種轉換，導致大量安靜的資料遺失：
//       int len = someDouble;      // 小數不見了，編譯器一聲不吭
//   這類 bug 極難查，因為程式照跑、只是數字慢慢不對。C++11 引入大括號
//   初始化時，順便訂了一條規則：**在大括號內不准窄化**。
//   換句話說，大括號不只是新語法，它還是一道「請編譯器幫我檢查」的開關。
//
// 【3. 那為什麼小括號還是允許】
//   因為小括號初始化沿用的是函數呼叫的引數轉換規則，而那套規則從 C 繼承而來，
//   為了相容既有程式碼不能改。所以同一件事寫兩種括號會有不同結果——
//   這正是本檔要示範的重點。
//
// 【4. 重要：g++ 的實際行為與標準規定不完全一樣】
//   標準說 Holder h2{pi}; 是 ill-formed（不合法），但標準只要求編譯器
//   **發出診斷訊息**，而「警告」也算診斷。g++ 為了相容大量既有程式碼，
//   預設選擇只給警告就放行。本機 g++ 15.2 實測：
//       g++ -std=c++17 -Wall -Wextra      → warning: narrowing conversion
//       g++ -std=c++17 -pedantic-errors   → error:   narrowing conversion
//   所以精確的說法是「標準規定不合法，g++ 預設以警告放行，加
//   -pedantic-errors 才會變成錯誤」，而不是「一定編不過」。
//   這也是為什麼驗證標準版本行為時要用 -pedantic-errors 而不是 -fsyntax-only。
//
// 【5. 需要轉換時怎麼寫】
//   真的要截斷時，用明確轉換把意圖寫清楚：
//       Holder h{static_cast<int>(pi)};   // 我知道會截斷，這是我要的
//   這樣既通過窄化檢查，也讓下一個讀程式的人知道這不是意外。
//
// 【概念補充 Concept Deep Dive】
//
//   ● 常數運算式例外：適用於哪些方向？（這裡很容易記錯，以下皆本機實測）
//     標準對部分窄化情形開了例外：若來源是**編譯期常數運算式**且值合乎條件，
//     就不算窄化。但**這個例外不適用於「浮點 → 整數」**：
//         Holder h{3.0};   // double→int：即使 3.0 剛好是整數，仍是窄化 ✗
//         Holder h{3.5};   // double→int：窄化 ✗
//     例外真正適用的是這三個方向（以 -pedantic-errors 實測）：
//         char  c{65};        // 整數常數 → 較窄整數，值裝得下      合法 ✓
//         char  c{300};       // 裝不下                            窄化 ✗
//         float f{3};         // 整數常數 → 浮點，可精確表示        合法 ✓
//         float f{16777217};  // 超過 float 精確表示整數的範圍      窄化 ✗
//         float f{3.0};       // double 常數 → float，在範圍內      合法 ✓
//         float f{0.1};       // 精度雖有損，但**在 float 範圍內**  合法 ✓
//         float f{1e300};     // 超出 float 可表示範圍              窄化 ✗
//     注意 float f{0.1} 這一項：浮點縮小的例外條件是「在目標型別的可表示
//     範圍內」，並不要求精確，這跟整數方向的規則不同。
//     另外，例外只認**常數運算式**；只要來源是變數就一律以型別關係判定：
//         int c = 65; char h{c};   // 變數 → 窄化 ✗
//     （作者按：本檔初稿曾誤寫成「3.0 可精確表示為 int 故合法」，
//       是編譯器以 -pedantic-errors 擋下來才更正的，這正是要實測的理由。）
//
//   ● 截斷是「趨零」不是「四捨五入」
//     double → int 的轉換規則是捨去小數部分（趨向零），所以 3.9 → 3、
//     -3.9 → -3。要四捨五入請用 std::round。
//     另外，若原值超出 int 可表示的範圍，該轉換是未定義行為，
//     不是「取某個奇怪的數字」——不要依賴任何觀察到的結果。
//
//   ● 大括號同時解決了 most vexing parse
//         Holder h();     // 這是宣告函數，不是建立物件！
//         Holder h{};     // 這才是值初始化
//
// 【注意事項 Pay Attention】
//   1. 「大括號一定編不過」的說法不精確；請以 -pedantic-errors 為準。
//   2. 大括號防窄化很好用，但若類別有 initializer_list 建構函數，
//      大括號會**優先**選它，可能挑到你沒預期的重載。
//   3. 明確要截斷就寫 static_cast，別靠小括號默默完成。
//   4. 建議專案至少開 -Wall -Wextra；對數值敏感的專案可再加
//      -Werror=narrowing，把這個警告升級成錯誤。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】窄化轉換與大括號初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是窄化轉換？大括號初始化為什麼要禁止它？
//     答：窄化轉換是指來源型別的某些值在目標型別裝不下的隱式轉換，
//         例如 double→int、long→short、int→float。C 傳統上放行，造成安靜的
//         資料遺失；C++11 在大括號初始化中禁止它，把這類 bug 提前到編譯期。
//     追問：小括號為什麼不擋？
//         → 小括號沿用函數引數的轉換規則，那套規則從 C 繼承，為相容性不能改。
//
// 🔥 Q2. Holder h{3.0}; 能編譯嗎？（Holder 成員是 int，3.0 剛好是整數值）
//     答：不能。「浮點 → 整數」**永遠**是窄化，沒有常數運算式例外，
//         即使值剛好是 3.0 也一樣。常數例外只適用於「整數→較窄整數」、
//         「整數→浮點」、「浮點→較窄浮點」這三個方向。
//     追問：那 char c{65}; 為什麼可以？
//         → 那是整數常數轉較窄整數，65 在 char 範圍內，符合常數例外；
//           但 char c{300}; 就會被擋，而 int n = 65; char c{n}; 也會被擋
//           （來源是變數，不是常數運算式）。
//
// ⚠️ 陷阱. 大括號初始化禁止窄化，所以 Holder h{pi}; 一定編譯失敗，對嗎？
//     答：標準規定它 ill-formed，但 g++ 預設只發出 -Wnarrowing 警告就放行，
//         程式仍然編得出來。要加 -pedantic-errors 或 -Werror=narrowing
//         才會真的變成錯誤。本機 g++ 15.2 實測確認。
//     為什麼會錯：把「標準說不合法」等同於「編譯器一定拒絕」。標準只要求
//         發出診斷訊息，警告也算數；編譯器可以為了相容性選擇警告。
//         這也是為什麼驗證標準行為要用 -pedantic-errors，
//         光用 -fsyntax-only 會被 GCC 當成擴充放行，看不出真正的差別。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <cmath>
using namespace std;

class Holder {
public:
    int value;
    Holder(int v) : value(v) { }
};

// 下面兩個小類別用來示範「常數運算式例外」真正適用的方向
class CharBox {
public:
    char value;
    CharBox(char v) : value(v) { }
};

class FloatBox {
public:
    float value;
    FloatBox(float v) : value(v) { }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器讀數轉整數百分比：明確表達「我要怎麼取整」
//   情境：溫溼度感測器回傳 double（例如 67.83%），但上報協定的欄位是
//         0~100 的整數。直接截斷會系統性低估，通常應該四捨五入。
//   重點：不要靠小括號默默截斷；用 static_cast 或 std::lround 把意圖寫清楚，
//         並在建構時就把範圍夾住，避免超出目標型別造成未定義行為。
// -----------------------------------------------------------------------------
class HumidityReading {
private:
    int percent_;      // 0~100 的整數百分比

public:
    // 明確指定轉換方式：先四捨五入，再夾到合法範圍
    explicit HumidityReading(double raw)
        : percent_(clampToPercent(lround(raw))) { }

    int percent() const { return percent_; }

private:
    // 夾住範圍，順便避免超出 int 範圍的未定義行為
    static int clampToPercent(long v) {
        if (v < 0)   return 0;
        if (v > 100) return 100;
        return static_cast<int>(v);
    }
};

int main() {
    double pi = 3.14;

    cout << "=== 小括號：允許窄化（安靜截斷）===" << endl;
    Holder h1(pi);            // double → int，截斷為 3
    cout << "  Holder h1(pi)   → h1.value = " << h1.value
         << "  （3.14 的小數被截掉了，編譯器沒吭聲）" << endl;

    // 大括號：標準規定為 ill-formed（窄化轉換）
    // g++ 預設只給 -Wnarrowing 警告，加 -pedantic-errors 才會是錯誤
    // Holder h2{pi};
    cout << "  Holder h2{pi}   → 標準規定不合法；g++ 預設警告、"
            "-pedantic-errors 則為錯誤" << endl;

    cout << "\n=== 大括號：沒有窄化就完全合法 ===" << endl;
    Holder h3{3};             // int → int，無窄化
    cout << "  Holder h3{3}    → h3.value = " << h3.value << endl;

    // 注意：浮點 → 整數「永遠」是窄化，沒有常數運算式例外
    // Holder h4{3.0};        // 即使 3.0 剛好是整數值，仍是窄化
    cout << "  Holder h4{3.0}  → 仍是窄化：浮點→整數沒有常數例外" << endl;

    cout << "\n=== 常數運算式例外真正適用的方向 ===" << endl;
    // 整數常數 → 較窄整數：值裝得下就合法
    CharBox cb{65};
    cout << "  CharBox cb{65}  → cb.value = '" << cb.value
         << "'  （整數常數→char，65 裝得下）" << endl;
    // CharBox bad{300};      // 300 超出 char 範圍 → 窄化
    cout << "  CharBox bad{300}→ 300 超出 char 範圍，仍是窄化" << endl;

    // 整數常數 → 浮點：可精確表示就合法
    FloatBox fb{3};
    cout << "  FloatBox fb{3}  → fb.value = " << fb.value
         << "  （整數常數→float，可精確表示）" << endl;
    // FloatBox bad2{16777217};  // 超出 float 精確表示整數的範圍 → 窄化
    cout << "  FloatBox{16777217} → 超出 float 精確整數範圍，窄化" << endl;

    // double 常數 → float：只要求「在範圍內」，不要求精確
    FloatBox fb2{0.1};
    cout << "  FloatBox fb2{0.1} → fb2.value = " << fb2.value
         << "  （在 float 範圍內即可，不要求精確）" << endl;

    cout << "\n=== 需要截斷時：用 static_cast 表明意圖 ===" << endl;
    Holder h6{static_cast<int>(pi)};
    cout << "  Holder h6{static_cast<int>(pi)} → h6.value = " << h6.value << endl;

    cout << "\n=== 日常實務：感測器讀數轉整數百分比 ===" << endl;
    HumidityReading r1(67.83);    // 四捨五入 → 68（截斷會變 67）
    HumidityReading r2(12.4);     // → 12
    HumidityReading r3(103.7);    // 超出範圍 → 夾到 100
    cout << "  raw 67.83 → " << r1.percent() << " %" << endl;
    cout << "  raw 12.40 → " << r2.percent() << " %" << endl;
    cout << "  raw 103.7 → " << r3.percent() << " %（已夾到上限）" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數8.cpp" -o demo8
//
// ※ 想親自看到窄化診斷，把上面 Holder h2{pi}; 那行的註解拿掉，再比較：
//     g++ -std=c++17 -Wall -Wextra ...      → warning: narrowing conversion
//     g++ -std=c++17 -pedantic-errors ...   → error:   narrowing conversion

// === 預期輸出 ===
// === 小括號：允許窄化（安靜截斷）===
//   Holder h1(pi)   → h1.value = 3  （3.14 的小數被截掉了，編譯器沒吭聲）
//   Holder h2{pi}   → 標準規定不合法；g++ 預設警告、-pedantic-errors 則為錯誤
//
// === 大括號：沒有窄化就完全合法 ===
//   Holder h3{3}    → h3.value = 3
//   Holder h4{3.0}  → 仍是窄化：浮點→整數沒有常數例外
//
// === 常數運算式例外真正適用的方向 ===
//   CharBox cb{65}  → cb.value = 'A'  （整數常數→char，65 裝得下）
//   CharBox bad{300}→ 300 超出 char 範圍，仍是窄化
//   FloatBox fb{3}  → fb.value = 3  （整數常數→float，可精確表示）
//   FloatBox{16777217} → 超出 float 精確整數範圍，窄化
//   FloatBox fb2{0.1} → fb2.value = 0.1  （在 float 範圍內即可，不要求精確）
//
// === 需要截斷時：用 static_cast 表明意圖 ===
//   Holder h6{static_cast<int>(pi)} → h6.value = 3
//
// === 日常實務：感測器讀數轉整數百分比 ===
//   raw 67.83 → 68 %
//   raw 12.40 → 12 %
//   raw 103.7 → 100 %（已夾到上限）
