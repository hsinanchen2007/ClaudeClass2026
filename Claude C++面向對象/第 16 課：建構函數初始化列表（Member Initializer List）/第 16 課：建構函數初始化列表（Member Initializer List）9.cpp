// =============================================================================
//  第 16 課：初始化列表 9  —  類別內預設成員初始化（NSDMI）與初始化列表的互動
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { int w = 1280; string title = "My Game"; };
//   正式名稱：NSDMI（Non-Static Data Member Initializer，非靜態資料成員初始化器）
//   標準版本：**C++11** 引入；C++14 起放寬到聚合型別也能用
//   複雜度：O(1)；等同於把該初值寫進每個建構函數的初始化列表
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. NSDMI 解決什麼問題】
//   在 C++11 之前，若一個類別有五個建構函數，而某個成員的預設值都一樣，
//   你必須在**五個**初始化列表裡各寫一次。漏寫一個就是一個潛在 bug，
//   而且改預設值要改五個地方。
//   NSDMI 讓你在宣告成員的地方直接寫預設值，一次寫好，所有建構函數共用。
//
// 【2. 優先順序：初始化列表**覆蓋** NSDMI】
//   這是本檔最重要的規則：
//     ● 成員有寫在某個建構函數的初始化列表 → 用初始化列表的值，**忽略** NSDMI
//     ● 成員沒寫在初始化列表 → 用 NSDMI 的值
//     ● 兩者都沒有 → 類別型別呼叫預設建構函數；內建型別**不初始化**（值不定）
//   本檔的 GameConfig(int w, int h) 只覆蓋 screenWidth 與 screenHeight，
//   其餘三個成員自動沿用 NSDMI 的預設值——這正是 NSDMI 最實用的地方。
//
// 【3. 重要：NSDMI 不是「先設預設值再覆蓋」】
//   很多人以為流程是「先用 1280 初始化，再改成 1920」，那是錯的。
//   編譯器的實際做法是：**在建構函數的初始化列表中，該成員的位置若沒有
//   你指定的初值，就填入 NSDMI 的運算式**。所以只會初始化一次，
//   不會有「先建一個再蓋掉」的浪費。
//   換句話說，NSDMI 是編譯期的「填空預設值」，不是執行期的兩段式賦值。
//
// 【4. NSDMI 對內建型別特別有價值】
//   類別型別成員（string、vector）沒寫初值時會呼叫預設建構函數，
//   至少是個有效的空物件。但**內建型別成員（int、double、指標）
//   沒寫初值就是不定值**，讀取它是未定義行為。
//   替所有內建型別成員寫上 NSDMI，等於一勞永逸地消除這整類 bug：
//       int   count_ = 0;
//       bool  ready_ = false;
//       Node* next_  = nullptr;
//   這是現代 C++ 相當推薦的預設習慣。
//
// 【5. NSDMI 的兩個限制】
//   ● 只能用 = 或 {} 的形式，不能用小括號：
//         int a = 5;      // OK
//         int b{5};       // OK
//         int c(5);       // 錯誤：這會被當成函數宣告
//   ● 在 C++11 中，有 NSDMI 的類別就不是聚合型別（aggregate），
//     不能用 X x = {1, 2}; 這種聚合初始化。
//     **C++14 放寬了這個限制**，有 NSDMI 也仍可為聚合型別。
//
// 【概念補充 Concept Deep Dive】
//
//   ● NSDMI 與「宣告了建構函數就沒有預設建構函數」的互動
//     本檔的 GameConfig 寫了三個建構函數，所以編譯器不再自動生成
//     預設建構函數——因此它必須自己寫一個 GameConfig()。
//     注意這個手寫的預設建構函數初始化列表是空的，五個成員全部
//     由 NSDMI 提供初值，函數體只印一行訊息。
//
//   ● NSDMI 的求值時機
//     NSDMI 的運算式是在**物件建構時**求值，不是在類別定義時。
//     所以每個物件都會各自求值一次，可以呼叫函數、可以用其他已初始化的成員
//     （仍受宣告順序約束）。
//
//   ● NSDMI 與 const 成員
//     const 成員也可以有 NSDMI：
//         const int kVersion = 3;
//     若某個建構函數的初始化列表另有指定，就用該指定值；否則用 3。
//     這讓 const 成員不必在每個建構函數都重寫一次。
//
//   ● 與「預設參數」的取捨
//     另一種給預設值的方式是建構函數預設參數：
//         GameConfig(int w = 1280, int h = 720);
//     差別在於：預設參數是「呼叫端沒傳就補上」，屬於介面的一部分，
//     會影響重載決議；NSDMI 則是「成員自己的預設狀態」，與介面無關。
//     成員的固有預設值用 NSDMI，介面的便利性用預設參數，兩者常搭配使用。
//
// 【注意事項 Pay Attention】
//   1. 初始化列表**覆蓋** NSDMI；不是先設再改，只會初始化一次。
//   2. NSDMI 不能用小括號形式（會被解析成函數宣告）。
//   3. 建議替所有內建型別成員寫上 NSDMI，消除不定值風險。
//   4. C++11 中有 NSDMI 就不是聚合型別；**C++14 起放寬**。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別內預設成員初始化（NSDMI）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員同時有類別內預設值和初始化列表指定值，最後用哪個？
//     答：用初始化列表的值，NSDMI 被忽略。而且不是「先設 NSDMI 再覆蓋」，
//         編譯器只是在該成員沒有指定初值時，才把 NSDMI 的運算式填進去，
//         所以只會初始化一次，沒有多餘成本。
//     追問：兩個都沒寫會怎樣？
//         → 類別型別呼叫預設建構函數；內建型別**不初始化**，值不定，
//           讀取它是未定義行為——這正是該替內建型別寫 NSDMI 的理由。
//
// 🔥 Q2. NSDMI 是哪個標準引入的？有什麼限制？
//     答：C++11 引入。限制是只能用 = 或 {} 形式，不能用小括號
//         （int c(5); 會被解析成函數宣告）。另外在 C++11 中，
//         有 NSDMI 的類別不算聚合型別，不能用聚合初始化；
//         這個限制在 **C++14 已放寬**。
//     追問：NSDMI 和建構函數預設參數該怎麼選？
//         → NSDMI 表達「成員自己的預設狀態」，與介面無關；
//           預設參數是介面的一部分，會參與重載決議。兩者常搭配使用。
//
// ⚠️ 陷阱. class Config { int retries = 3; }; 寫了預設值，
//          所以不管怎麼建立物件，retries 一定是 3 或我指定的值，對嗎？
//     答：對於這個成員是的。但要小心的是**沒有寫 NSDMI 的內建型別成員**——
//         它們在多數建構情境下不會被初始化，值是不定的。
//         同一個類別裡「有些成員寫了、有些沒寫」是最容易出事的狀態，
//         因為你會誤以為整個類別都被保護了。
//     為什麼會錯：把 NSDMI 想成類別層級的保護機制。它其實是**逐成員**的，
//         漏掉哪個成員，那個成員就沒有保護。建議一律替所有內建型別成員補上。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
using namespace std;

class GameConfig {
private:
    // C++11 NSDMI：直接在宣告時給預設值，所有建構函數共用
    int screenWidth = 1280;
    int screenHeight = 720;
    bool fullscreen = false;
    string title = "My Game";
    int fps = 60;

public:
    // 因為下面寫了帶參建構函數，編譯器不再自動生成預設建構函數，
    // 所以這個無參數版本必須自己寫。
    // 注意它的初始化列表是空的——五個成員全部由 NSDMI 提供初值。
    GameConfig() {
        cout << "  [預設建構] 使用所有預設值" << endl;
    }

    // 部分自定義：初始化列表覆蓋 NSDMI
    // 只有 screenWidth/screenHeight 被覆蓋，其餘三個沿用 NSDMI
    GameConfig(int w, int h)
        : screenWidth(w), screenHeight(h)
    {
        cout << "  [部分自定義] 自定義解析度，其餘沿用預設" << endl;
    }

    // 完全自定義：五個成員全部由初始化列表指定，NSDMI 全部被忽略
    GameConfig(int w, int h, bool fs, const string& t, int f)
        : screenWidth(w), screenHeight(h), fullscreen(fs),
          title(t), fps(f)
    {
        cout << "  [完全自定義] 所有參數指定" << endl;
    }

    void print() const {
        cout << "    遊戲: " << title << endl;
        cout << "    解析度: " << screenWidth << "x" << screenHeight << endl;
        cout << "    全螢幕: " << (fullscreen ? "是" : "否") << endl;
        cout << "    FPS: " << fps << endl;
    }
};

// -----------------------------------------------------------------------------
// 證明「NSDMI 不是先設再覆蓋」：用會出聲的型別觀察建構次數
//   若真的是「先設預設值再覆蓋」，Loud 應該會建構兩次；
//   實際輸出只有一次，證明編譯器只是把 NSDMI 當成填空的預設初值。
// -----------------------------------------------------------------------------
class Loud {
public:
    explicit Loud(const string& tag) : tag_(tag) {
        cout << "      Loud 建構: " << tag_ << endl;
    }
private:
    string tag_;
};

class NsdmiProof {
private:
    Loud item = Loud("來自 NSDMI");   // NSDMI 提供的預設值

public:
    NsdmiProof() { }                              // 沿用 NSDMI
    explicit NsdmiProof(const string& t)
        : item(Loud(t)) { }                       // 覆蓋 NSDMI
};

// -----------------------------------------------------------------------------
// 【日常實務範例】重試策略設定：多個建構函數共用同一組預設值
//   情境：呼叫外部 API 時需要一組重試參數（次數、退避間隔、逾時、是否抖動）。
//         不同呼叫端只想調整其中一兩項，其餘沿用團隊共識的預設值。
//   重點一：NSDMI 讓預設值只寫一次，三個建構函數共用，改預設只改一行。
//   重點二：所有內建型別成員都寫了 NSDMI，徹底消除不定值的風險。
// -----------------------------------------------------------------------------
class RetryPolicy {
private:
    // 全部內建型別成員都給 NSDMI —— 建議養成的習慣
    int    maxAttempts_   = 3;
    int    baseDelayMs_   = 200;
    int    timeoutMs_     = 5000;
    bool   useJitter_     = true;
    string name_          = "default";

public:
    RetryPolicy() = default;      // 全部沿用 NSDMI

    // 只調整重試次數
    explicit RetryPolicy(int maxAttempts)
        : maxAttempts_(maxAttempts), name_("custom-attempts")
    { }

    // 調整次數與逾時
    RetryPolicy(int maxAttempts, int timeoutMs)
        : maxAttempts_(maxAttempts), timeoutMs_(timeoutMs), name_("custom-full")
    { }

    void print() const {
        cout << "  [" << name_ << "] 重試 " << maxAttempts_ << " 次"
             << ", 退避 " << baseDelayMs_ << "ms"
             << ", 逾時 " << timeoutMs_ << "ms"
             << ", 抖動 " << (useJitter_ ? "開" : "關") << endl;
    }
};

int main() {
    cout << "=== 配置 1：全部預設（NSDMI 提供）===" << endl;
    GameConfig cfg1;
    cfg1.print();

    cout << "\n=== 配置 2：只改解析度，其餘沿用 NSDMI ===" << endl;
    GameConfig cfg2(1920, 1080);
    cfg2.print();

    cout << "\n=== 配置 3：全部自定義，NSDMI 全被覆蓋 ===" << endl;
    GameConfig cfg3(2560, 1440, true, "RPG World", 144);
    cfg3.print();

    cout << "\n=== 證明 NSDMI 不是「先設再覆蓋」（只建構一次）===" << endl;
    cout << "    沿用 NSDMI:" << endl;
    { NsdmiProof a; (void)a; }
    cout << "    覆蓋 NSDMI（若是先設再覆蓋，這裡會出現兩行）:" << endl;
    { NsdmiProof b("來自初始化列表"); (void)b; }

    cout << "\n=== 日常實務：重試策略設定 ===" << endl;
    RetryPolicy p1;             // 全預設
    RetryPolicy p2(5);          // 只改次數
    RetryPolicy p3(5, 30000);   // 改次數與逾時
    p1.print();
    p2.print();
    p3.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）9.cpp" -o demo9
//   （NSDMI 為 C++11 特性，本檔以 -std=c++11 以上皆可編譯）

// === 預期輸出 ===
// === 配置 1：全部預設（NSDMI 提供）===
//   [預設建構] 使用所有預設值
//     遊戲: My Game
//     解析度: 1280x720
//     全螢幕: 否
//     FPS: 60
//
// === 配置 2：只改解析度，其餘沿用 NSDMI ===
//   [部分自定義] 自定義解析度，其餘沿用預設
//     遊戲: My Game
//     解析度: 1920x1080
//     全螢幕: 否
//     FPS: 60
//
// === 配置 3：全部自定義，NSDMI 全被覆蓋 ===
//   [完全自定義] 所有參數指定
//     遊戲: RPG World
//     解析度: 2560x1440
//     全螢幕: 是
//     FPS: 144
//
// === 證明 NSDMI 不是「先設再覆蓋」（只建構一次）===
//     沿用 NSDMI:
//       Loud 建構: 來自 NSDMI
//     覆蓋 NSDMI（若是先設再覆蓋，這裡會出現兩行）:
//       Loud 建構: 來自初始化列表
//
// === 日常實務：重試策略設定 ===
//   [default] 重試 3 次, 退避 200ms, 逾時 5000ms, 抖動 開
//   [custom-attempts] 重試 5 次, 退避 200ms, 逾時 5000ms, 抖動 開
//   [custom-full] 重試 5 次, 退避 200ms, 逾時 30000ms, 抖動 開
