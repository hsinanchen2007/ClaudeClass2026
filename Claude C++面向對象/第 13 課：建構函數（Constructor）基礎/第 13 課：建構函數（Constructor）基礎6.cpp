// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 6  —  多個多載並存時的解析與陷阱
// =============================================================================
//
// 【主題資訊 Information】
//   主題      : 四個 constructor 多載並存；隱式轉換與 ambiguity
//   標準版本  : C++98 起；本檔用到的 explicit（單參數）自 C++98 即有
//   標頭檔    : <iostream>、<string>
//   關鍵風險  : 單參數 constructor 會成為隱式轉換的來源
//
// 【詳細解釋 Explanation】
//
// 【1. 這個 Rectangle 有四個 constructor，它們如何被挑中】
//       Rectangle()                              → r1;              參數 0 個
//       Rectangle(double side)                   → r2(5.0);         參數 1 個
//       Rectangle(double w, double h)            → r3(4.0, 6.0);    參數 2 個
//       Rectangle(double w, double h, string c)  → r4(3, 7, "紅色"); 參數 3 個
//   因為參數個數各不相同，overload resolution 幾乎不會有懸念。
//   真正的問題出在「參數個數相同、型別可互轉」的情況——見下一段。
//
// 【2. 單參數 constructor 是一個隱式轉換運算子（本檔最重要的觀念）】
//   Rectangle(double side) 沒有加 explicit，所以下面這些全都合法：
//       Rectangle r = 5.0;              // double 悄悄變成一個正方形
//       void draw(Rectangle);  draw(5.0);   // 也會自動轉
//       rects.push_back(7.0);           // 連容器操作都會轉
//   語意上，「一個數字」和「一個矩形」是完全不同的東西，這種自動轉換
//   幾乎一定不是你要的。它會讓型別錯誤變成「編得過但行為詭異」。
//   正解是加上 explicit：
//       explicit Rectangle(double side);
//   加了之後 Rectangle r = 5.0; 直接編譯失敗，Rectangle r(5.0); 仍然可用。
//   準則：**單參數 constructor 預設就該加 explicit**，除非你刻意要那個轉換
//   （例如 std::string 從 const char* 轉，那是設計意圖）。
//
// 【3. 什麼情況會 ambiguous】
//   假設再加一個 Rectangle(int side)，讓 (int) 與 (double) 兩個多載並存：
//       Rectangle r(5);      // int    完全相符 → 選 int 版，OK
//       Rectangle r(5.0);    // double 完全相符 → 選 double 版，OK
//       Rectangle r(5.0f);   // float  → 不 ambiguous！選 double 版
//       long n = 5;
//       Rectangle r(n);      // long   → ambiguous，編譯失敗
//   為什麼 float 沒事、long 卻不行？關鍵在標準轉換序列的「等級」：
//       float → double : floating-point promotion  → 等級 Promotion
//       float → int    : floating-integral conv.   → 等級 Conversion
//   Promotion 的等級高於 Conversion，所以 double 版本明確勝出，毫無懸念。
//   但是：
//       long  → int    : integral conversion       → 等級 Conversion
//       long  → double : floating-integral conv.   → 等級 Conversion
//   兩者同為 Conversion、分不出高下 → ambiguous。unsigned、long long 同理。
//   （以上皆已在本機 GCC 15.2 用 -std=c++17 實測驗證。）
//   結論：不要憑「感覺哪個比較接近」推論，多載解析比的是**轉換等級**，
//   而不是數值上的相似程度。
//
// 【4. 為什麼「正方形也用 Rectangle」在語意上要小心】
//   Rectangle(double side) 把邊長相同的矩形當成正方形，這在資料表示上沒問題。
//   但如果日後有人寫 class Square : public Rectangle，就會踩到著名的
//   Liskov 替換原則反例（設定 Square 的寬會意外改變高）。
//   這裡先記住結論：**用 constructor 表達「這是一種特例」是安全的，
//   用繼承表達則常常不是。**
//
// 【概念補充 Concept Deep Dive】
//   ▍隱式轉換最多只會發生一次
//     使用者定義的轉換在一個轉換序列中只能出現一次。所以若有
//     Rectangle(const string&)，傳 const char* 進去**不會**自動轉
//     （需要 const char* → string → Rectangle 兩次使用者定義轉換）。
//     這是很多人以為「應該可以」卻編不過的原因。
//
//   ▍為什麼本檔的 4 個多載沒有 ambiguity
//     因為每個多載的參數個數都不同，第一步「參數個數篩選」就分開了。
//     只要參數個數不同，就永遠不會互相競爭。
//
//   ▍{} 初始化與多載
//     Rectangle r{4.0, 6.0}; 與 r(4.0, 6.0) 選到同一個 constructor，
//     但 {} 額外禁止窄化轉換。若類別有 initializer_list constructor，
//     {} 會**強烈優先**選它——這是 std::vector<int> v{3, 1} 得到 {3,1}
//     而不是「3 個 1」的原因。第 15 課會再談。
//
// 【注意事項 Pay Attention】
//   1. 單參數 constructor 不加 explicit → 成為隱式轉換來源，容易造成意外。
//   2. 多個參數個數相同、型別可互轉的多載，很容易 ambiguous（編譯錯誤而非警告）；
//      判準是「標準轉換序列的等級」是否相同，不是數值上像不像。
//   3. 值傳遞 std::string 參數會多一次複製，正式碼請用 const string&。
//   4. 「正方形是一種矩形」在資料建模上成立，在繼承上常常不成立。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多載、隱式轉換與 explicit
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 converting constructor？為什麼建議單參數 constructor 加 explicit？
//     答：可以用單一引數呼叫的非 explicit constructor，會成為「使用者定義轉換」
//         的來源，讓編譯器在需要時自動把該型別的值轉成你的類別。
//         這使得型別錯誤不再是編譯錯誤，而是靜悄悄地轉換掉。
//         加 explicit 後仍可直接初始化 T x(v);，只是不再自動轉換。
//     追問：那什麼時候「不」該加 explicit？→ 當那個轉換就是設計意圖時，
//         例如 std::string 接受 const char*、std::function 接受 lambda。
//
// 🔥 Q2. 有 Rectangle(int) 和 Rectangle(double) 兩個多載，傳 float 和傳 long 各會怎樣？
//     答：傳 float **不會** ambiguous——float→double 是 floating-point promotion，
//         等級高於 float→int 的 floating-integral conversion，double 版明確勝出。
//         傳 long 才會 ambiguous——long→int 與 long→double 同為 Conversion 等級，
//         分不出高下，編譯失敗。unsigned、long long 同理。
//     追問：那要怎麼修？→ 明確轉型 Rectangle r(static_cast<double>(n));，
//         或把多載改成單一個 template／移除其中一個，別留下等級相同的競爭者。
//
// ⚠️ 陷阱. 類別有 Rectangle(const std::string&)，為什麼 Rectangle r = "紅色"; 編不過？
//     答：因為那需要兩次使用者定義轉換：const char* → std::string → Rectangle。
//         標準規定一個隱式轉換序列中最多只能有一次使用者定義轉換。
//     為什麼會錯：以為「反正最後轉得到就好」。實際上編譯器只肯替你走一步，
//         多一步就必須由你明確寫出來（例如 Rectangle r{std::string("紅色")};）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設的矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
        cout << "[建構1] 預設矩形" << endl;
    }

    // 建構函數 2：正方形（只給一個邊長）
    // ⚠️ 沒有 explicit → 這是一個 converting constructor，double 可自動變 Rectangle
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
        cout << "[建構2] 正方形, 邊長 = " << side << endl;
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
        cout << "[建構3] 矩形 " << w << " x " << h << endl;
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
        cout << "[建構4] " << c << "矩形 " << w << " x " << h << endl;
    }

    void print() const {
        cout << "  " << color << " 矩形: "
             << width << " x " << height
             << ", 面積 = " << width * height << endl;
    }
};

// 對照組：加了 explicit，杜絕意外的隱式轉換
class SafeRectangle {
private:
    double width_, height_;

public:
    explicit SafeRectangle(double side) : width_(side), height_(side) {}
    SafeRectangle(double w, double h)   : width_(w),   height_(h)   {}

    void print() const {
        cout << "  安全矩形: " << width_ << " x " << height_
             << ", 面積 = " << width_ * height_ << endl;
    }
};

// 用來示範「隱式轉換會悄悄發生」
static void drawRectangle(const Rectangle& r) {
    cout << "  drawRectangle 收到 → ";
    r.print();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔的核心是 overload resolution 與隱式轉換，是編譯期的型別系統議題。
//         LeetCode 題目的類別簽名都是固定單一 constructor，
//         既不會產生多載競爭，也觀察不到隱式轉換造成的意外，沒有合適題目。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】金額型別（Money）：explicit 如何擋下真實世界的 bug
//   情境：交易系統中把金額包成型別，是為了避免「把手續費當成本金」這類錯誤。
//         但只要 constructor 忘了加 explicit，Money 就會和 double 自由互換，
//         包裝型別的保護作用整個消失——傳錯單位、傳錯欄位都編得過。
//   對照：UnsafeMoney（可隱式轉換）vs Money（explicit）。
// -----------------------------------------------------------------------------
class UnsafeMoney {
private:
    double amount_;
public:
    UnsafeMoney(double a) : amount_(a) {}       // ⚠️ 沒有 explicit
    double amount() const { return amount_; }
};

class Money {
private:
    long long cents_;                            // 用整數分，避免浮點誤差
public:
    explicit Money(long long cents) : cents_(cents) {}
    long long cents() const { return cents_; }
    void print() const {
        cout << "  $" << (cents_ / 100) << "." ;
        long long f = cents_ % 100;
        if (f < 10) cout << "0";
        cout << f << endl;
    }
};

static void chargeUnsafe(UnsafeMoney m) {
    cout << "  [不安全] 扣款 " << m.amount() << endl;
}
static void charge(const Money& m) {
    cout << "  [安全] 扣款 "; m.print();
}

int main() {
    cout << "=== 四個多載，各自由參數個數決定 ===" << endl;
    Rectangle r1;                        // 建構函數 1
    r1.print();

    Rectangle r2(5.0);                   // 建構函數 2
    r2.print();

    Rectangle r3(4.0, 6.0);              // 建構函數 3
    r3.print();

    Rectangle r4(3.0, 7.0, "紅色");       // 建構函數 4
    r4.print();

    cout << "\n=== 陷阱：單參數 constructor 造成的隱式轉換 ===" << endl;
    cout << "下一行寫的是 Rectangle box = 9.0; 一個 double 就變成矩形：" << endl;
    Rectangle box = 9.0;                 // 合法！因為建構函數 2 沒有 explicit
    box.print();

    cout << "函式參數也會自動轉：drawRectangle(2.5);" << endl;
    drawRectangle(2.5);                  // double → Rectangle，悄悄發生

    cout << "連 vector 都會：v.push_back(4.0);" << endl;
    vector<Rectangle> v;
    v.reserve(2);
    v.push_back(4.0);                    // 同樣是隱式轉換
    v.back().print();

    cout << "\n=== 加上 explicit 之後 ===" << endl;
    SafeRectangle sr(9.0);               // 直接初始化：仍然可以
    sr.print();
    cout << "  SafeRectangle s = 9.0;  ← 這行會編譯失敗（explicit 擋下）" << endl;

    cout << "\n=== 日常實務：金額型別 ===" << endl;
    chargeUnsafe(1500);                  // ⚠️ 傳的是 int，卻自動變成 UnsafeMoney
    cout << "  ↑ 沒人擋得住：任何數字都能當金額傳進去" << endl;

    Money fee(1500);                     // 明確：1500 分 = $15.00
    charge(fee);
    cout << "  charge(1500);  ← 這行會編譯失敗（explicit 擋下），" << endl;
    cout << "  逼你寫出 charge(Money(1500)); 表明「這確實是金額」" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎6.cpp" -o ctor6


// === 預期輸出 ===
// === 四個多載，各自由參數個數決定 ===
// [建構1] 預設矩形
//   白色 矩形: 1 x 1, 面積 = 1
// [建構2] 正方形, 邊長 = 5
//   白色 矩形: 5 x 5, 面積 = 25
// [建構3] 矩形 4 x 6
//   白色 矩形: 4 x 6, 面積 = 24
// [建構4] 紅色矩形 3 x 7
//   紅色 矩形: 3 x 7, 面積 = 21
//
// === 陷阱：單參數 constructor 造成的隱式轉換 ===
// 下一行寫的是 Rectangle box = 9.0; 一個 double 就變成矩形：
// [建構2] 正方形, 邊長 = 9
//   白色 矩形: 9 x 9, 面積 = 81
// 函式參數也會自動轉：drawRectangle(2.5);
// [建構2] 正方形, 邊長 = 2.5
//   drawRectangle 收到 →   白色 矩形: 2.5 x 2.5, 面積 = 6.25
// 連 vector 都會：v.push_back(4.0);
// [建構2] 正方形, 邊長 = 4
//   白色 矩形: 4 x 4, 面積 = 16
//
// === 加上 explicit 之後 ===
//   安全矩形: 9 x 9, 面積 = 81
//   SafeRectangle s = 9.0;  ← 這行會編譯失敗（explicit 擋下）
//
// === 日常實務：金額型別 ===
//   [不安全] 扣款 1500
//   ↑ 沒人擋得住：任何數字都能當金額傳進去
//   [安全] 扣款   $15.00
//   charge(1500);  ← 這行會編譯失敗（explicit 擋下），
//   逼你寫出 charge(Money(1500)); 表明「這確實是金額」
