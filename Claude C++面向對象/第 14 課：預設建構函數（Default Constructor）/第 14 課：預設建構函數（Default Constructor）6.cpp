// =============================================================================
//  第 14 課：預設建構函數 6  —  全部參數都有預設值，它也是 default constructor
// =============================================================================
//
// 【主題資訊 Information】
//   規則      : 「default constructor」的定義是「可以不帶任何引數呼叫的 constructor」，
//               而不是「參數列為空的 constructor」
//   標準版本  : C++98 起（預設參數本身自 C++98 即有）
//   標頭檔    : <iostream>、<type_traits>
//   最大陷阱  : 它會和一個真正的無參 constructor 互相衝突（ambiguous）
//
// 【詳細解釋 Explanation】
//
// 【1. 定義要看「能不能不帶引數呼叫」】
//   下面兩者**都是** default constructor：
//       Color();                                 // 形式 1：參數列為空
//       Color(int r = 0, int g = 0, int b = 0);   // 形式 2：所有參數都有預設值
//   判準是「Color c; 這行寫得出來嗎」。形式 2 寫得出來，所以它就是。
//   這個定義的實際意義在於：形式 2 的類別**可以**建立陣列、可以放進
//   std::vector<Color> v(10)、可以當 map 的 mapped_type——
//   所有需要 default constructor 的地方它都滿足。
//
// 【2. 致命陷阱：兩者不能並存】
//   若你同時寫了：
//       Color() { ... }
//       Color(int r = 0, int g = 0, int b = 0) { ... }
//   那麼 Color c; 會編譯失敗，錯誤訊息是：
//       error: call of overloaded 'Color()' is ambiguous
//   因為「不帶引數」時兩個候選都可行，而且**一樣好**，編譯器無法選擇。
//   （本機 GCC 15.2 實測確認，錯誤訊息如上。）
//   注意這個錯誤發生在**呼叫的地方**，不是宣告的地方——
//   類別本身編得過，直到有人寫 Color c; 才爆。這使它更難發現。
//
// 【3. 預設參數的規則：只能從右往左省略】
//   Color(int r = 0, int g = 0, int b = 0);
//       Color c1;              // r=0, g=0, b=0
//       Color c2(255);         // r=255, g=0, b=0
//       Color c3(0, 128);      // r=0, g=128, b=0
//   你**不能**跳過中間的參數（沒有 Color(r=0, b=255) 這種寫法，
//   C++ 沒有具名引數）。一旦某個參數有了預設值，它右邊的所有參數也必須有。
//   想要「只指定某幾個欄位」的效果，慣用作法是 named parameter idiom
//   （回傳 *this 的鏈式 setter）或傳一個設定用的 struct。
//
// 【4. 預設參數的隱藏成本：它是呼叫端的一部分】
//   預設值不是存在函式裡，而是在**每個呼叫端**被編譯進去。
//   這代表：
//     * 改動預設值後，所有呼叫端都必須重新編譯才會生效（函式庫的 ABI 議題）
//     * virtual function 的預設參數是**靜態繫結**的——用 base 指標呼叫時
//       取的是 base 宣告的預設值，即使實際執行的是 derived 的 override。
//       這是相當惡名昭彰的坑，所以準則是「virtual function 不要用預設參數」。
//
// 【概念補充 Concept Deep Dive】
//   ▍為什麼「一樣好」就是錯誤而不是「隨便選一個」
//     C++ 的多載解析要求結果必須唯一且可預測。若允許編譯器任選，
//     同一份程式碼換個編譯器就可能有不同行為，這對語言是不可接受的。
//
//   ▍全預設參數的 constructor 仍會讓隱式 default constructor 消失
//     它本身就是「使用者宣告的 constructor」，所以隱式版本一樣不會生成。
//     只是因為它**自己**就能當 default constructor 用，通常不會有人發現差異。
//
//   ▍單參數可呼叫 → 也是 converting constructor
//     Color(int r = 0, int g = 0, int b = 0) 可以用單一個 int 呼叫，
//     因此它同時是 converting constructor：Color c = 255; 會編譯通過。
//     若不想要這個隱式轉換，要加 explicit。本檔實測示範。
//
// 【注意事項 Pay Attention】
//   1. 全預設參數的 constructor 不能和無參 constructor 並存——會 ambiguous。
//   2. 預設參數只能從右往左省略，C++ 沒有具名引數。
//   3. 預設值編進呼叫端；改動需要重新編譯所有使用者（函式庫 ABI 注意）。
//   4. virtual function 的預設參數是靜態繫結——不要在 virtual function 上用。
//   5. 可用單一引數呼叫時它同時是 converting constructor，視情況加 explicit。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】全預設參數的 constructor
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Color(int r=0, int g=0, int b=0) 算不算 default constructor？
//     答：算。default constructor 的定義是「可以不帶任何引數呼叫」，
//         而不是「參數列為空」。所以它同樣能支援 Color c;、Color arr[10];、
//         std::vector<Color> v(10) 這些需要 default constructor 的場景。
//     追問：那它和 Color() 可以同時存在嗎？→ 不行。兩者對「不帶引數」的呼叫
//         同樣可行且一樣好，寫 Color c; 就會得到 ambiguous 編譯錯誤。
//
// 🔥 Q2. 預設參數和函式多載，實務上怎麼選？
//     答：語意相同、只是省略尾端參數 → 預設參數，較精簡；
//         語意不同、初始化邏輯不同 → 多載，各寫各的。
//         另外預設值會編進呼叫端，改動需要所有使用者重新編譯，
//         寫函式庫時這是實質的 ABI 考量。
//     追問：virtual function 用預設參數有什麼問題？→ 預設參數是靜態繫結的，
//         用 Base* 呼叫時會取 Base 宣告的預設值，卻執行 Derived 的實作，
//         得到「一半 base、一半 derived」的詭異結果。準則是別這樣寫。
//
// ⚠️ 陷阱. 想「只指定 blue」而寫 Color c(0, 0, 255) 覺得很囉唆，於是想寫 Color c(b=255)
//     答：C++ 沒有具名引數（named arguments），這是語法錯誤。
//         預設參數只能從右往左省略。想達到類似效果，慣用作法是
//         named parameter idiom（Color().blue(255)，每個 setter 回傳 *this）
//         或傳一個設定 struct。
//     為什麼會錯：把 Python／C# 的具名引數習慣帶進 C++。
//         C++ 到目前為止（含 C++23）都沒有這個語法。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;

class Color {
private:
    int r, g, b;

public:
    // 所有參數都有預設值 → 這也是預設建構函數！
    // ⚠️ 若再加一個 Color() { }，Color c1; 就會 ambiguous（編譯失敗）
    Color(int red = 0, int green = 0, int blue = 0) {
        r = (red   >= 0 && red   <= 255) ? red   : 0;
        g = (green >= 0 && green <= 255) ? green : 0;
        b = (blue  >= 0 && blue  <= 255) ? blue  : 0;
    }

    void print() const {
        cout << "  RGB(" << r << ", " << g << ", " << b << ")" << endl;
    }
};

// 編譯期事實查核：全預設參數的 constructor 確實讓它可以預設建構
static_assert(std::is_default_constructible<Color>::value,
              "全參數都有預設值 → 可以不帶引數呼叫 → 就是 default constructor");

// -----------------------------------------------------------------------------
// named parameter idiom：C++ 沒有具名引數時的慣用替代方案
// -----------------------------------------------------------------------------
class ColorBuilder {
private:
    int r_ = 0, g_ = 0, b_ = 0;

public:
    ColorBuilder& red(int v)   { r_ = v; return *this; }
    ColorBuilder& green(int v) { g_ = v; return *this; }
    ColorBuilder& blue(int v)  { b_ = v; return *this; }

    void print() const {
        cout << "  RGB(" << r_ << ", " << g_ << ", " << b_ << ")" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔的重點是「全預設參數也算 default constructor」以及它與無參
//         constructor 的 ambiguity，屬於多載解析的編譯期議題。
//         LeetCode 題目的 constructor 簽名由題目固定，不會出現這種衝突。
//         本課的設計題實作放在 7.cpp（705 Design HashSet）與 summary.cpp。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】重試策略（RetryPolicy）：預設參數讓 API 好用又不失彈性
//   情境：一個 HTTP 客戶端的重試設定。90% 的呼叫端只想要「合理的預設」，
//         少數需要細調。全預設參數讓兩種需求用同一個 constructor 滿足，
//         呼叫端從最常用的參數開始寫，其餘自動採用預設。
//   設計要點：參數順序要按「最常被調整」排到「最少被調整」，
//         因為只能從右往左省略——把最常調的放最左邊，API 才好用。
// -----------------------------------------------------------------------------
class RetryPolicy {
private:
    int    maxAttempts_;
    int    baseDelayMs_;
    double backoffFactor_;
    bool   jitter_;

public:
    // 依「最常調整 → 最少調整」排序參數
    RetryPolicy(int maxAttempts       = 3,
                int baseDelayMs       = 200,
                double backoffFactor  = 2.0,
                bool jitter           = true)
        : maxAttempts_(maxAttempts), baseDelayMs_(baseDelayMs),
          backoffFactor_(backoffFactor), jitter_(jitter) {}

    // 算出第 n 次重試該等多久（不含 jitter 的隨機成分，保持輸出可重現）
    int delayForAttempt(int n) const {
        double d = baseDelayMs_;
        for (int i = 1; i < n; ++i) d *= backoffFactor_;
        return static_cast<int>(d);
    }

    void print() const {
        cout << "  maxAttempts=" << maxAttempts_
             << ", baseDelay=" << baseDelayMs_ << "ms"
             << ", backoff=" << backoffFactor_
             << ", jitter=" << (jitter_ ? "on" : "off") << endl;
        cout << "    退避序列: ";
        for (int i = 1; i <= maxAttempts_; ++i)
            cout << delayForAttempt(i) << "ms" << (i < maxAttempts_ ? " → " : "");
        cout << endl;
    }
};

int main() {
    cout << "=== 全預設參數：一個 constructor 涵蓋四種用法 ===" << endl;
    Color c1;                // 全部使用預設值 → (0, 0, 0)
    Color c2(255);           // green 和 blue 用預設值 → (255, 0, 0)
    Color c3(0, 128);        // blue 用預設值 → (0, 128, 0)
    Color c4(100, 200, 50);  // 全部指定

    cout << "c1 (無引數): "; c1.print();
    cout << "c2 (1 個)  : "; c2.print();
    cout << "c3 (2 個)  : "; c3.print();
    cout << "c4 (3 個)  : "; c4.print();

    cout << "\n=== 它確實算 default constructor ===" << endl;
    cout << boolalpha;
    cout << "  is_default_constructible<Color> = "
         << std::is_default_constructible<Color>::value << endl;
    Color arr[3];                      // 陣列：需要 default constructor
    cout << "  Color arr[3];      → OK，arr[0] = "; arr[0].print();
    vector<Color> v(2);                // vector 的 count 版本：同樣需要
    cout << "  vector<Color> v(2); → OK，size = " << v.size() << endl;

    cout << "\n=== 陷阱：不能再加一個 Color() ===" << endl;
    cout << "  若同時宣告 Color() 與 Color(int=0,int=0,int=0)，" << endl;
    cout << "  寫 Color c; 會得到編譯錯誤（本機 GCC 15.2 實測）：" << endl;
    cout << "    error: call of overloaded 'Color()' is ambiguous" << endl;
    cout << "  注意錯誤發生在「使用的地方」，不是宣告處——更難發現。" << endl;

    cout << "\n=== 陷阱：可用單一引數呼叫 → 也是 converting constructor ===" << endl;
    Color implicit = 200;              // int 隱式轉成 Color，因為沒有 explicit
    cout << "  Color implicit = 200; → "; implicit.print();
    cout << "  若不想要這個轉換，請在 constructor 前加 explicit。" << endl;

    cout << "\n=== C++ 沒有具名引數：用 named parameter idiom 代替 ===" << endl;
    cout << "  想「只指定 blue」不能寫 Color c(blue=255)，只能寫 Color c(0,0,255)；" << endl;
    cout << "  或改用鏈式 builder：" << endl;
    ColorBuilder b;
    b.blue(255);
    cout << "  ColorBuilder().blue(255) → "; b.print();

    cout << "\n=== 日常實務：重試策略 ===" << endl;
    cout << "大多數呼叫端（全用預設）：" << endl;
    RetryPolicy defaults;
    defaults.print();

    cout << "只想加大重試次數（從左邊開始寫）：" << endl;
    RetryPolicy aggressive(5);
    aggressive.print();

    cout << "內網服務，快速失敗：" << endl;
    RetryPolicy fast(2, 50, 1.5, false);
    fast.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 14 課：預設建構函數（Default Constructor）6.cpp" -o def6

// === 預期輸出 ===
// === 全預設參數：一個 constructor 涵蓋四種用法 ===
// c1 (無引數):   RGB(0, 0, 0)
// c2 (1 個)  :   RGB(255, 0, 0)
// c3 (2 個)  :   RGB(0, 128, 0)
// c4 (3 個)  :   RGB(100, 200, 50)
//
// === 它確實算 default constructor ===
//   is_default_constructible<Color> = true
//   Color arr[3];      → OK，arr[0] =   RGB(0, 0, 0)
//   vector<Color> v(2); → OK，size = 2
//
// === 陷阱：不能再加一個 Color() ===
//   若同時宣告 Color() 與 Color(int=0,int=0,int=0)，
//   寫 Color c; 會得到編譯錯誤（本機 GCC 15.2 實測）：
//     error: call of overloaded 'Color()' is ambiguous
//   注意錯誤發生在「使用的地方」，不是宣告處——更難發現。
//
// === 陷阱：可用單一引數呼叫 → 也是 converting constructor ===
//   Color implicit = 200; →   RGB(200, 0, 0)
//   若不想要這個轉換，請在 constructor 前加 explicit。
//
// === C++ 沒有具名引數：用 named parameter idiom 代替 ===
//   想「只指定 blue」不能寫 Color c(blue=255)，只能寫 Color c(0,0,255)；
//   或改用鏈式 builder：
//   ColorBuilder().blue(255) →   RGB(0, 0, 255)
//
// === 日常實務：重試策略 ===
// 大多數呼叫端（全用預設）：
//   maxAttempts=3, baseDelay=200ms, backoff=2, jitter=on
//     退避序列: 200ms → 400ms → 800ms
// 只想加大重試次數（從左邊開始寫）：
//   maxAttempts=5, baseDelay=200ms, backoff=2, jitter=on
//     退避序列: 200ms → 400ms → 800ms → 1600ms → 3200ms
// 內網服務，快速失敗：
//   maxAttempts=2, baseDelay=50ms, backoff=1.5, jitter=off
//     退避序列: 50ms → 75ms
