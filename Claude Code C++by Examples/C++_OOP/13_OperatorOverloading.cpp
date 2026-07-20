/*=============================================================================
 * 檔名：13_OperatorOverloading.cpp
 * 主題：運算子重載 (Operator Overloading)
 * 適合：寫過 friend / operator<< 後，想完整學會「讓物件像內建型別一樣用」的人
 *
 * 【課題介紹】
 *   你已經能寫 a + b 對 int / double 沒問題；但對自訂類別呢？
 *   C++ 允許你「重載」(overload) 各種運算子，讓自訂物件支援 +、-、==、<<...
 *
 *       Fraction a(1, 2), b(1, 3);
 *       Fraction c = a + b;        // 1/2 + 1/3 = 5/6
 *       std::cout << c;            // 印出 5/6
 *
 *   背後其實就是函式呼叫：
 *       a + b      → operator+(a, b)   或   a.operator+(b)
 *       std::cout << c
 *                  → operator<<(std::cout, c)
 *
 * 【寫成「成員函式」還是「外部函式 / friend」？】
 *   - 成員函式：左運算元必為 *this，因此左邊一定是「我們的物件」。
 *               適合 +=, -=, *=, /=, [], (), =, ++, --, ->
 *   - 外部 / friend 函式：兩邊運算元都能是「任何型別」，比較對稱。
 *               適合 +, -, *, /, ==, !=, <, <<, >>
 *   原則：可以讓「左邊也能是其他型別」(例如 int + Fraction) 的，寫成外部函式。
 *
 *   實務技巧：先把 += 寫成成員函式，再用它實作非成員的 operator+，避免重複邏輯：
 *       Fraction operator+(Fraction lhs, const Fraction& rhs) {
 *           lhs += rhs;
 *           return lhs;
 *       }
 *
 * 【哪些運算子不能重載？】
 *   ::, .*, ., ?:, sizeof, typeid - 這幾個 C++ 規定不能改。
 *
 * 【++ -- 前置與後置的差別】
 *   - 前置 ++a  → operator++();          回傳 ClassName& (改後的自己)
 *   - 後置 a++  → operator++(int);       回傳 ClassName  (改前的副本)
 *     那個 int 參數只是個「擺設」，用來區分兩個版本，慣例上沒名字也不用。
 *
 * 【日常實用範例】
 *   Fraction (分數) 類別，重載 +、-、*、/、==、!=、<<、++、--、+= 等。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/operators
 *=============================================================================*/

/*
補充筆記：OperatorOverloading
  - operator overload 應維持使用者對運算子的直覺語意。
  - 比較運算尤其要支援 strict weak ordering，否則排序容器和 sort 會出問題。
  - 成員或非成員 overload 的選擇會影響隱式轉換是否對左右運算元都有效。
  - operator overloading 讓自訂型別使用 +、==、<、[]、() 等熟悉語法，但應保留運算子的直覺意義，不要讓讀者猜錯。
  - 成員運算子左運算元固定是 *this；非成員運算子可讓左右兩邊都參與隱式轉換，因此對稱運算如 a + b 常適合寫非成員。
  - operator=、operator[]、operator()、operator-> 必須或通常寫成成員，因為它們語意上依附在左側物件。
  - operator== 應比較物件的值語意；若類別有多個欄位，要確認哪些欄位屬於可觀察狀態，哪些只是 cache 或實作細節。
  - operator< 或比較器若要給排序容器使用，必須符合 strict weak ordering；不一致會讓 std::set/std::map 或 sort 出現錯誤行為。
  - 前置 ++ 通常回傳 Class&，後置 ++ 以 int 假參數區分並回傳舊值；後置版本常需要複製，成本較高。
  - 輸入輸出運算子 operator<< 和 operator>> 常回傳 stream reference，才能串接 std::cout << a << b。
  - 不要過度重載罕見或語意曖昧的運算子；如果命名函式更清楚，例如 normalize()、distanceTo()，就不必硬用符號。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】運算子重載（Operator Overloading）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 哪些運算子不能重載？成員 vs 非成員該怎麼選？
//     答：不能重載的有 `::`、`.`、`.*`、`?:`，以及 `sizeof`、`typeid`、`alignof`、
//     `noexcept`。選擇原則：`=`、`[]`、`()`、`->` 標準要求必須是成員；會修改左運算元的
//     （`+=` 這類）通常寫成員；對稱的二元運算子（`+`、`==`）建議寫成非成員，
//     這樣左運算元也能享有隱式轉換（例如讓 `2 + frac` 也能編譯）。
//     追問：實務上怎麼避免兩份重複邏輯？（先把 `+=` 寫成成員，再用它實作非成員的 `+`）
//
// 🔥 Q2. 前置與後置 ++ 怎麼區分？
//     答：前置是 `T& operator++();`，回傳改動後的自己；後置是 `T operator++(int);`，
//     那個 int 只是用來區分版本的擺設（沒有名字也不會用到），而且必須回傳「改動前的副本」。
//     追問：為什麼慣例上偏好前置？（後置一定要多做一次複製，對迭代器或大型物件成本明顯）
//
// Q3. 自訂型別要能放進 std::map / std::unordered_map，各需要什麼？
//     答：`std::map` 需要符合嚴格弱序（strict weak ordering）的比較 — 提供 `operator<`
//     或自訂 comparator；`std::unordered_map` 則需要 `std::hash` 特化（或自訂 hash
//     functor）再加上 `operator==`。
//     追問：`operator<` 誤寫成 `<=` 會怎樣？（違反嚴格弱序 → UB，map/set/sort 的行為
//     會錯亂甚至崩潰，而且不一定馬上出事）
//
// ⚠️ 陷阱. 定義了 C++20 的 `operator<=>`，六個比較運算子就全都有了嗎？
//     答：不完全。`<=>` 讓編譯器改寫 `<`、`>`、`<=`、`>=`，但 `==` 與 `!=` 走的是
//     `operator==`。只有「`= default` 的 `operator<=>`」才會一併隱式宣告 defaulted 的
//     `operator==`；你自己手寫的 `<=>` 並不會給你 `==`。
//     為什麼會錯：把 `<=>` 當成「一鍵搞定全部六個」的萬用鍵。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <numeric>     // std::gcd (C++17)
#include <vector>

class Fraction {
private:
    int num_;     // 分子 (numerator)
    int den_;     // 分母 (denominator)

    // 內部小工具：把分數化簡 (例如 4/6 → 2/3)
    void reduce() {
        if (den_ < 0) { num_ = -num_; den_ = -den_; }   // 規定分母為正
        int g = std::gcd(num_ < 0 ? -num_ : num_, den_);
        if (g > 0) { num_ /= g; den_ /= g; }
    }

public:
    Fraction(int n = 0, int d = 1) : num_(n), den_(d) {
        if (d == 0) { num_ = 0; den_ = 1; }      // 防分母為 0，簡化處理
        reduce();
    }

    // ---- 算術運算子 (對稱型，寫成 friend 比較自然) ----
    friend Fraction operator+(const Fraction& a, const Fraction& b);
    friend Fraction operator-(const Fraction& a, const Fraction& b);
    friend Fraction operator*(const Fraction& a, const Fraction& b);
    friend Fraction operator/(const Fraction& a, const Fraction& b);

    // ---- 比較運算子 ----
    friend bool operator==(const Fraction& a, const Fraction& b);
    friend bool operator!=(const Fraction& a, const Fraction& b);
    friend bool operator< (const Fraction& a, const Fraction& b);

    // ---- 串流輸出 ----
    friend std::ostream& operator<<(std::ostream& os, const Fraction& f);

    // ---- 複合賦值運算子 (修改自己 → 寫成成員函式) ----
    Fraction& operator+=(const Fraction& other) {
        num_ = num_ * other.den_ + other.num_ * den_;
        den_ = den_ * other.den_;
        reduce();
        return *this;
    }

    // ---- 前置 ++ (將分數加 1) ----
    Fraction& operator++() {     // 沒參數 → 前置版本
        num_ += den_;            // 1/2 + 1 = 3/2，所以分子加分母即可
        return *this;
    }

    // ---- 後置 ++ (注意 int 參數是區分用) ----
    Fraction operator++(int) {
        Fraction tmp = *this;    // 先存舊值
        ++(*this);               // 借用前置版本實作，避免邏輯重複
        return tmp;              // 回傳「改之前」的副本
    }
};

Fraction operator+(const Fraction& a, const Fraction& b) {
    return Fraction(a.num_ * b.den_ + b.num_ * a.den_, a.den_ * b.den_);
}
Fraction operator-(const Fraction& a, const Fraction& b) {
    return Fraction(a.num_ * b.den_ - b.num_ * a.den_, a.den_ * b.den_);
}
Fraction operator*(const Fraction& a, const Fraction& b) {
    return Fraction(a.num_ * b.num_, a.den_ * b.den_);
}
Fraction operator/(const Fraction& a, const Fraction& b) {
    return Fraction(a.num_ * b.den_, a.den_ * b.num_);
}

bool operator==(const Fraction& a, const Fraction& b) {
    // 因為 reduce() 會把分數化簡，分子分母都相等就代表值相等
    return a.num_ == b.num_ && a.den_ == b.den_;
}
bool operator!=(const Fraction& a, const Fraction& b) {
    return !(a == b);
}
bool operator<(const Fraction& a, const Fraction& b) {
    // a/b < c/d  ⇔  a*d < c*b  (這裡假設兩邊分母都為正，已由 reduce 保證)
    return a.num_ * b.den_ < b.num_ * a.den_;
}

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    if (f.den_ == 1) os << f.num_;            // 整數情況不印分母
    else             os << f.num_ << "/" << f.den_;
    return os;
}

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1351 - Count Negative Numbers in a Sorted Matrix
// -----------------------------------------------------------------------------
// 題目簡述：給一個 m x n 的「每行/每列遞減」矩陣，回傳負數的數量。
// 重點：示範重載 operator() 讓物件能像函式一樣被呼叫 (functor)。
class NegativeCounter {
private:
    std::vector<std::vector<int>> grid_;
public:
    explicit NegativeCounter(std::vector<std::vector<int>> g) : grid_(std::move(g)) {}

    // 重載 operator()：讓物件可以像函式一樣呼叫，例：counter()
    int operator()() const {
        int cnt = 0;
        for (const auto& row : grid_) {
            for (int x : row) if (x < 0) ++cnt;
        }
        return cnt;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：日常實用 - Vec2 二維向量
// -----------------------------------------------------------------------------
// 工作上 2D/3D 向量幾乎都會重載運算子，讓使用方式直觀。
class Vec2 {
public:
    double x, y;
    Vec2(double a = 0, double b = 0) : x(a), y(b) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(double k)      const { return Vec2(x * k, y * k); }   // 純量乘
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }

    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};

int main() {
    Fraction a(1, 2);
    Fraction b(1, 3);

    std::cout << "a = " << a << std::endl;      // 1/2
    std::cout << "b = " << b << std::endl;      // 1/3

    std::cout << "a + b = " << (a + b) << std::endl;     // 5/6
    std::cout << "a - b = " << (a - b) << std::endl;     // 1/6
    std::cout << "a * b = " << (a * b) << std::endl;     // 1/6
    std::cout << "a / b = " << (a / b) << std::endl;     // 3/2

    std::cout << "(a == b) = " << (a == b) << std::endl; // 0
    std::cout << "(a < b)  = " << (a < b)  << std::endl; // 0 (1/2 > 1/3)

    Fraction c(2, 4);                 // 化簡後是 1/2
    std::cout << "c = " << c << std::endl;
    std::cout << "(a == c) = " << (a == c) << std::endl; // 1

    a += b;
    std::cout << "a after += b → " << a << std::endl;    // 5/6

    Fraction x(1, 4);
    std::cout << "x++  印的是改前: " << x++ << std::endl; // 1/4
    std::cout << "現在 x = "       << x   << std::endl;   // 5/4
    std::cout << "++x  印的是改後: " << ++x << std::endl; // 9/4

    std::cout << "----- 範例 2：Leetcode 1351 NegativeCounter operator() -----" << std::endl;
    NegativeCounter counter({
        { 4,  3,  2, -1},
        { 3,  2,  1, -1},
        { 1,  1, -1, -2},
        {-1, -1, -2, -3}
    });
    // 像函式一樣呼叫物件
    std::cout << "負數個數 = " << counter() << std::endl;   // 8

    std::cout << "----- 範例 3：Vec2 二維向量運算 -----" << std::endl;
    Vec2 p(1, 2), q(3, 4);
    std::cout << "p + q = " << (p + q) << std::endl;   // (4, 6)
    std::cout << "q - p = " << (q - p) << std::endl;   // (2, 2)
    std::cout << "p * 3 = " << (p * 3) << std::endl;   // (3, 6)
    std::cout << "p == q? " << (p == q) << std::endl;  // 0

    return 0;
}

/* 預期輸出：
 * a = 1/2
 * b = 1/3
 * a + b = 5/6
 * a - b = 1/6
 * a * b = 1/6
 * a / b = 3/2
 * (a == b) = 0
 * (a < b)  = 0
 * c = 1/2
 * (a == c) = 1
 * a after += b → 5/6
 * x++  印的是改前: 1/4
 * 現在 x = 5/4
 * ++x  印的是改後: 9/4
 * ----- 範例 2：Leetcode 1351 NegativeCounter operator() -----
 * 負數個數 = 8
 * ----- 範例 3：Vec2 二維向量運算 -----
 * p + q = (4, 6)
 * q - p = (2, 2)
 * p * 3 = (3, 6)
 * p == q? 0
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. operator+ - * / 、 == 、 << 、 ++ 等都可以重載；但 :: . .* ?: sizeof 不行。
 *   2. 對稱運算子 (+, -, ==, <) 寫成外部函式 / friend 較自然；複合賦值 (+=) 寫成成員。
 *   3. operator<< 必須是外部函式 (左運算元為 ostream)；通常配合 friend 存取 private。
 *   4. 前置 ++ 沒參數，後置 ++ 多一個假 int 參數做區分；後置實作通常借用前置。
 *   5. 重載要保持「直覺一致」(例如 + 不該有副作用)，否則使用者反而會踩雷。
 *
 * 【下一篇預告】
 *   14_Inheritance.cpp
 *   繼承 (Inheritance) — OOP 三大支柱第二個，
 *   並用 Leetcode 155. Min Stack 練習「在現成類別之上擴充」。
 *=============================================================================*/
