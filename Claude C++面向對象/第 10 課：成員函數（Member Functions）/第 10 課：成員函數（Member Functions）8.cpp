// =============================================================================
//  第 10 課：成員函數 8  —  成員函數 vs 普通（自由）函數：該選哪一個
// =============================================================================
//
// 【主題資訊 Information】
//   成員函數：  c.area()          — 資料隱含在 this 裡
//   自由函數：  circleArea(r)     — 資料靠參數明確傳入
//   標準：  C++98 起即有；C++20 起有 std::numbers::pi 可取代硬編的 3.14159。
//   標頭檔：<iostream>（本檔）；π 建議用 <numbers>(C++20) 或 <cmath> 的 M_PI（POSIX 擴充）
//
// 【詳細解釋 Explanation】
//
// 【1. 兩者的差別只有「資料從哪來」】
//   成員函數 c.area()：資料綁在物件上，透過隱含的 this 取得；
//   自由函數 circleArea(r)：資料由呼叫端明確傳入。
//   兩者的計算完全一樣，差別在**耦合方向**：
//     - 成員函數與該類別的內部表示綁定，可存取 private 成員；
//     - 自由函數只能用公開介面，因此天然地與實作解耦。
//
// 【2. 什麼時候該用成員函數】
//   (a) 需要存取 private 成員；
//   (b) 該操作是類別「本質行為」的一部分（改變不變量的操作幾乎都該是成員）；
//   (c) 需要是 virtual（自由函數不能 virtual）。
//
// 【3. 什麼時候該用自由函數 —— 這才是多數人不知道的】
//   Scott Meyers 有一條著名建議：**能寫成自由函數就不要寫成成員函數**，
//   因為這樣能提高封裝性。理由是：
//   「能存取 private 成員的函式越少，改動內部表示時要檢查的程式碼就越少。」
//   若某個操作只用得到公開介面，把它放進類別裡並不會讓它更快或更安全，
//   卻擴大了「有權碰內部」的範圍。
//   實務判準：**它需要看到 private 嗎？不需要 → 自由函數。**
//
//   另外有些運算子**必須**是自由函數：
//     - operator<<／operator>>：左運算元是 ostream，不是你的類別；
//     - 需要左運算元隱式轉換的對稱運算（如 2 * vec 與 vec * 2 都要成立）。
//
// 【4. 泛型演算法偏好自由函數】
//   std::sort、std::accumulate 這類演算法接受的是自由函數／函式物件，
//   因為它們要能作用在**任何**型別上，不能要求對方是某個類別的成員。
//   這是 STL 選擇自由函數風格的根本原因。
//
// 【概念補充 Concept Deep Dive】
//   本檔硬編的 3.14159 是個真實的精度問題：它只有 6 位有效數字，
//   而 double 可表示約 15~17 位。半徑 5 時 area 誤差約 6.6e-5，
//   放大到天文或工程尺度就不可接受。正確做法：
//     C++20：  #include <numbers>   std::numbers::pi          （最佳）
//     C++17：  constexpr double kPi = 3.14159265358979323846;
//     POSIX ：  <cmath> 的 M_PI（非 ISO C++ 標準，靠平台擴充，需 _USE_MATH_DEFINES 於 MSVC）
//
//   還有一個常被忽略的點：自由函數 circleArea(double) 可以用在
//   **任何** double 上，包括根本不是圓的資料；而成員函數 c.area()
//   只能作用在 Circle 上。型別安全性上成員函數更強，
//   這也是「用型別表達意圖」的一個小例子。
//
// 【注意事項 Pay Attention】
//   1. 別把 3.14159 這類魔術數字散落各處 —— 集中成一個 constexpr 常數。
//   2. 自由函數若需要存取 private，唯一途徑是宣告為 friend，
//      但 friend 會打破封裝，應克制使用。
//   3. 自由函數無法是 virtual；需要執行期多型時只能用成員函數。
//   4. 同名的成員函數與自由函數不會互相重載（不同 scope），
//      呼叫方式也不同（c.area() vs circleArea(r)），不會混淆。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函數 vs 自由函數
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個操作該做成成員函數還是自由函數？判準是什麼？
//     答：核心判準是「它需不需要存取 private 成員」。
//         需要 → 成員函數；只用得到公開介面 → 自由函數。
//         Scott Meyers 的建議是「能寫成自由函數就寫成自由函數」，
//         因為能碰 private 的程式碼越少，封裝性越好、日後改內部表示越安全。
//     追問：那哪些一定要是成員？
//         → 需要 virtual 的、operator=／operator[]／operator()／operator->
//           這幾個標準規定必須是成員的運算子，以及會維護類別不變量的操作。
//
// 🔥 Q2. 為什麼 operator<< 一定要寫成自由函數？
//     答：因為二元運算子的左運算元決定它能不能是成員。
//         cout << obj 的左運算元是 std::ostream，
//         若寫成成員就得改 ostream 的定義 —— 我們無權那麼做。
//         所以只能寫成自由函數 ostream& operator<<(ostream&, const T&)，
//         需要存取 private 時再宣告為 friend。
//     追問：那 operator+ 呢？
//         → 兩者皆可，但慣例是寫成自由函數，
//           這樣 2 + obj 與 obj + 2 才能對稱地都成立
//           （成員版的左運算元不會做隱式轉換）。
//
// ⚠️ 陷阱. 把所有函式都塞進類別裡當成員函數，是不是比較「物件導向」、比較安全？
//     答：恰恰相反，這會**降低**封裝性。每多一個成員函數，
//         就多一份能直接碰 private 的程式碼；日後想改內部表示時，
//         要檢查與修改的範圍就更大。
//     為什麼會錯：把「封裝」誤解成「東西都放進類別裡」，
//         但封裝真正的度量是「有多少程式碼依賴內部表示」。
//         只用公開介面的操作放在類別外，反而讓類別更容易演進。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <sstream>
using namespace std;

class Circle {
public:
    double radius = 0;

    // 成員函數：自動知道 radius
    double area() {
        return 3.14159 * radius * radius;
    }
};

// 普通函數：必須手動傳入 radius
double circleArea(double radius) {
    return 3.14159 * radius * radius;
}

// -----------------------------------------------------------------------------
// 【可執行示範】硬編 3.14159 的精度代價
//   3.14159 只有 6 位有效數字，double 卻能表示約 15~17 位。
//   半徑 5 時面積誤差約 6.6e-5 —— 對繪圖無感，對工程計算就不行了。
// -----------------------------------------------------------------------------
constexpr double kPi = 3.14159265358979323846;   // C++20 可直接用 std::numbers::pi

void demoPiPrecision() {
    double rough  = 3.14159 * 5.0 * 5.0;
    double better = kPi     * 5.0 * 5.0;
    cout.precision(10);
    cout << fixed;
    cout << "  用 3.14159 : " << rough  << endl;
    cout << "  用完整 pi  : " << better << endl;
    cout << scientific << "  誤差       : " << (better - rough) << endl;
    cout.unsetf(ios::floatfield);
    cout.precision(6);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】訂單金額 Money：示範「哪些該是成員、哪些該是自由函數」
//   情境：電商訂單金額。以「分」為單位存成整數，避免浮點誤差 ——
//         這是所有金流系統的鐵則（0.1 + 0.2 != 0.3 在錢上是事故）。
//   為什麼用到本主題：這個類別剛好把三種情況都示範了一遍：
//     (a) add()／cents()  → **成員函數**：需要碰 private 的 m_cents
//     (b) operator<<      → **必須是自由函數**：左運算元是 ostream
//     (c) formatInvoice() → **選擇做成自由函數**：只用得到公開介面，
//                            放在類別外可提高封裝性（Meyers 的建議）
// -----------------------------------------------------------------------------
class Money {
    long long m_cents = 0;          // private：以分為單位，避免浮點誤差
public:
    Money() = default;
    explicit Money(long long cents) : m_cents(cents) {}

    // (a) 成員函數：需要存取並修改 private 狀態
    Money& add(const Money& other) {
        m_cents += other.m_cents;   // 直接碰 private，只有成員做得到
        return *this;
    }

    // 公開的唯讀介面
    long long cents() const { return m_cents; }
};

// (b) 必須是自由函數：左運算元是 ostream，我們無法把它寫進 ostream 裡。
//     這裡只用公開的 cents()，所以連 friend 都不需要 —— 封裝完好。
ostream& operator<<(ostream& os, const Money& m) {
    long long c = m.cents();
    const char* sign = (c < 0) ? "-" : "";
    long long abs_c = (c < 0) ? -c : c;
    os << sign << "NT$" << (abs_c / 100) << '.'
       << (abs_c % 100 < 10 ? "0" : "") << (abs_c % 100);
    return os;
}

// (c) 刻意做成自由函數：它只需要公開介面，
//     放在類別外可以讓能碰 private 的程式碼保持最少。
string formatInvoice(const string& item, const Money& price, int qty) {
    Money total(price.cents() * qty);
    ostringstream oss;
    oss << item << " x" << qty << "  單價 " << price << "  小計 " << total;
    return oss.str();
}

int main() {
    cout << "=== 基本：成員函數 vs 自由函數，同樣的計算 ===" << endl;
    Circle c;
    c.radius = 5.0;

    // 成員函數風格
    cout << "成員函數: " << c.area() << endl;

    // 普通函數風格
    cout << "普通函數: " << circleArea(c.radius) << endl;

    cout << "\n=== 硬編 3.14159 的精度代價 ===" << endl;
    demoPiPrecision();

    cout << "\n=== 日常實務：Money 的三種函式歸屬 ===" << endl;
    Money price(12999);              // NT$129.99
    cout << "  單價: " << price << endl;          // 走自由函數 operator<<

    Money cart(0);
    cart.add(price).add(Money(4500)).add(Money(199));   // 成員函數，鏈式
    cout << "  購物車總額: " << cart << endl;

    cout << "  " << formatInvoice("USB-C 傳輸線", price, 3) << endl;

    Money refund(-2500);
    cout << "  退款: " << refund << endl;          // 負數格式也正確

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）8.cpp" -o member8

// === 預期輸出 ===
// === 基本：成員函數 vs 自由函數，同樣的計算 ===
// 成員函數: 78.5397
// 普通函數: 78.5397
//
// === 硬編 3.14159 的精度代價 ===
//   用 3.14159 : 78.5397500000
//   用完整 pi  : 78.5398163397
//   誤差       : 6.6339744833e-05
//
// === 日常實務：Money 的三種函式歸屬 ===
//   單價: NT$129.99
//   購物車總額: NT$176.98
//   USB-C 傳輸線 x3  單價 NT$129.99  小計 NT$389.97
//   退款: -NT$25.00
