/*=============================================================================
 * 檔名：12_Friend.cpp
 * 主題：friend 朋友函式 與 friend 朋友類別
 * 適合：寫過運算子重載 (例如 operator<<) 但不知道為什麼要用 friend 的人
 *
 * 【課題介紹】
 *   封裝原則告訴我們：private 成員不該被外面碰。但有時候我們還是想讓
 *   「特定的外部函式或類別」可以存取 private 成員，例如：
 *
 *     - 想用 std::cout << myObj 印出物件內容，但 operator<< 必須是「外部函式」。
 *     - 兩個密切合作的類別，其中一個想直接讀對方的 private 成員以提高效率。
 *
 *   解法：在類別內宣告對方為「朋友 (friend)」。
 *
 *       「friend 是一個類別主動授權，告訴編譯器：
 *        『這個函式 / 類別是我的朋友，他可以看我的 private 成員。』」
 *
 * 【friend 的兩種型式】
 *   A. friend 函式 (friend function)：
 *        class Foo {
 *            friend void peek(const Foo&);   // 不是 Foo 的成員，但能看 private
 *        };
 *
 *   B. friend 類別 (friend class)：
 *        class Foo {
 *            friend class Bar;               // Bar 內任何函式都能看 Foo 的 private
 *        };
 *
 * 【friend 不打破封裝嗎？】
 *   重點是「friend 是被類別自己 explicitly 授權」，不是隨便外人就能闖進來。
 *   這跟 「在家裡發鑰匙給家人」 是一樣的道理：仍是受控的。
 *
 *   慣例：friend 用得越少越好。如果非用不可，多半是：
 *     - 重載 operator<<, operator>>
 *     - 重載對稱運算子 (例如 operator+ 想吃 (int + Foo)) 可寫成 friend
 *     - 兩個關係極為緊密、像「同一個邏輯模組」的類別
 *
 * 【為什麼 operator<< 必須是 friend (或外部函式)？】
 *   呼叫方式：    std::cout << obj;
 *   它本質上是：  operator<<(std::cout, obj)
 *   左運算元是 std::cout，不是我們的物件 → 無法寫成成員函式 (因為成員函式
 *   左邊一定是 *this，得是「我們的物件」)。
 *   所以只能寫成「外部函式」，但又想存取 private → 用 friend。
 *
 * 【日常實用範例】
 *   一個 Complex (複數) 類別，重載 operator<< 印出 (a + bi) 形式。
 *   再示範 friend class 場景：Engine 與 Diagnostic。
 *
 * 【對應 Leetcode】1929. Concatenation of Array
 *   題目：給一個陣列 nums，回傳 ans = [nums..., nums...] (把自己接自己一份)。
 *   為什麼選這題：剛好可以示範用 friend 重載 operator+ 來做「向量串接」，
 *   讓 nums + nums 自然得到答案。比直接 push_back 兩遍直觀許多。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/friend
 *   https://cplusplus.com/doc/tutorial/inheritance/
 *=============================================================================*/

/*
補充筆記：Friend
  - Friend 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - friend 讓指定函式或類別能存取 private/protected 成員；它不是成員函式，也不會有 this。
  - friend 應用在「邏輯上需要貼近內部表示」的操作，例如 operator<< 輸出物件、兩個類別高度合作的實作細節。
  - friend 宣告放在 class 內，但它授權的是外部函式；函式定義通常仍寫在 namespace scope。
  - friend 會降低封裝隔離，因為外部函式知道內部欄位；若只是想讀資料，優先考慮 public 查詢函式。
  - operator<< 常寫成 friend，是因為左運算元是 std::ostream，不能把它做成你的類別成員函式。
  - friend 關係不會繼承，base class 的 friend 不會自動成為 derived class 的 friend。
  - friend 也不是雙向的；A 把 B 宣告為 friend，不代表 A 能存取 B 的 private。
  - 使用 friend 前先問：這個函式是否真的屬於類別的抽象介面附近，還是只是偷懶想直接碰 private 欄位。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】friend
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. friend 是什麼？它破壞封裝嗎？
//     答：friend 授予特定函式或類別存取自身 private/protected 成員的權限。
//     它是由類別「自己明確授權」的 — 外部無法自行取得這個權限，所以比較像是
//     擴展介面，而不是破壞封裝。
//     追問：什麼時候該用？（`operator<<` 這種必須是非成員的運算子、
//     以及兩個緊密耦合、實質上屬於同一個邏輯模組的型別）
//
// 🔥 Q2. 為什麼 `operator<<` 通常要寫成 friend？
//     答：`std::cout << obj` 展開後是 `operator<<(std::cout, obj)`，左運算元是
//     std::ostream 而不是你的物件；成員函式的左運算元一定是 `*this`，所以它只能寫成
//     非成員函式。既然是非成員又需要讀 private，就用 friend 授權。
//     追問：回傳型別為什麼是 `std::ostream&`？（才能串接 `cout << a << b`）
//
// ⚠️ 陷阱. friend 關係會繼承、會遞移、會對稱嗎？
//     答：三個都不會。A 把 B 宣告為 friend：B 的 derived class 不會因此成為 A 的
//     friend、A 的 derived class 也不會繼承這份授權、更不代表 A 能存取 B 的 private。
//     為什麼會錯：把 friend 想成「兩個類別之間建立了一段關係」；
//     它其實是單向、逐一指名、且不會傳遞的授權。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 範例 1：Complex 複數 + friend operator<<
// -----------------------------------------------------------------------------
class Complex {
private:
    double re_;
    double im_;

public:
    Complex(double r = 0, double i = 0) : re_(r), im_(i) {}

    // 成員運算子：對稱運算 (Complex + Complex)
    Complex operator+(const Complex& other) const {
        return Complex(re_ + other.re_, im_ + other.im_);
    }

    // 把 operator<< 宣告成 friend，讓它能讀 re_, im_
    friend std::ostream& operator<<(std::ostream& os, const Complex& c);
    // 注意：這個 friend 宣告可以放在 public/private 任何區，效果一樣，
    //       因為 friend 不算「class 的成員」，access modifier 對它無意義。
};

// 在 class 外定義 friend 函式 - 它「不是」Complex 的成員，所以前面不寫 Complex::
std::ostream& operator<<(std::ostream& os, const Complex& c) {
    os << "(" << c.re_;
    if (c.im_ >= 0) os << " + " << c.im_ << "i)";
    else            os << " - " << (-c.im_) << "i)";
    return os;     // 回傳 os 才能讓 cout << a << b 連續呼叫
}

// -----------------------------------------------------------------------------
// 範例 2：friend class - Engine 公開私房菜給 Diagnostic
// -----------------------------------------------------------------------------
class Diagnostic;     // 前向宣告 (forward declaration)：因為下面 Engine 用到 Diagnostic 名稱

class Engine {
private:
    int rpm_;          // 引擎轉速，外人不該亂碰
    int oilTemp_;      // 機油溫度

public:
    Engine() : rpm_(800), oilTemp_(75) {}

    void run() { rpm_ = 3000; oilTemp_ = 92; }

    friend class Diagnostic;    // 讓 Diagnostic 看到 Engine 的所有 private 成員
};

class Diagnostic {
public:
    // 因為 Engine 把我們宣告成 friend，所以可以直接讀 e.rpm_ / e.oilTemp_
    void report(const Engine& e) {
        std::cout << "[診斷] rpm = " << e.rpm_
                  << ", 機油溫度 = " << e.oilTemp_ << "C" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：Leetcode 1929 - 用 friend operator+ 做向量串接
// -----------------------------------------------------------------------------
// IntArray 是一個薄薄的 vector 包裝。我們用 friend operator+ 讓它支援
//     IntArray a + IntArray b → 串接成新陣列
// 這樣 LC 1929 的解法可以寫成：return nums + nums;
//
// (註：必須定義在檔案範圍而非函式內 — 區域類別不允許 friend 函式)
class IntArray {
public:
    std::vector<int> v;
    IntArray() = default;
    IntArray(std::initializer_list<int> il) : v(il) {}

    // friend 函式：把兩個 IntArray 串接成新的 IntArray
    // 寫成 friend 是為了讓「兩個運算元都享有完整存取權」，
    // 並讓兩邊運算元都能參與 implicit 轉換 (對稱性)。
    friend IntArray operator+(const IntArray& a, const IntArray& b) {
        IntArray r;
        r.v.reserve(a.v.size() + b.v.size());
        r.v.insert(r.v.end(), a.v.begin(), a.v.end());
        r.v.insert(r.v.end(), b.v.begin(), b.v.end());
        return r;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：對應 Leetcode 1313 - Decompress Run-Length Encoded List
// -----------------------------------------------------------------------------
// 題目簡述：給 nums = [a1,b1,a2,b2,...]，每對 (ai,bi) 代表「把 bi 重複 ai 次」，
//           回傳解壓後的陣列。例 [1,2,3,4] → [2,4,4,4]。
// 重點：示範 friend operator<< 直接印 RleData 物件，內部 vector 不必暴露。
class RleData {
private:
    std::vector<int> codes_;      // 編碼資料：成對 (count, value)
public:
    explicit RleData(std::vector<int> v) : codes_(std::move(v)) {}

    // 解壓縮：回傳解壓後的整數陣列
    std::vector<int> decompress() const {
        std::vector<int> out;
        for (size_t i = 0; i + 1 < codes_.size(); i += 2) {
            int cnt = codes_[i];
            int val = codes_[i + 1];
            for (int k = 0; k < cnt; ++k) out.push_back(val);
        }
        return out;
    }

    // friend：可以印出私有 codes_ 內容
    friend std::ostream& operator<<(std::ostream& os, const RleData& r);
};

std::ostream& operator<<(std::ostream& os, const RleData& r) {
    os << "RLE[ ";
    for (int x : r.codes_) os << x << " ";       // 直接讀 private codes_
    os << "]";
    return os;
}

// -----------------------------------------------------------------------------
// 範例 5：日常實用 - Container + UnitTest friend 類別
// -----------------------------------------------------------------------------
// 寫單元測試時常想直接讀「正常情況下外人不該碰」的內部狀態，
// 以方便驗證。把測試類別宣告為 friend，可以在不破壞 public 介面的情況下測試。
class Container {
private:
    int data_[3];
    int size_;
public:
    Container() : data_{0, 0, 0}, size_(0) {}
    void push(int x) {
        if (size_ < 3) data_[size_++] = x;
    }
    int size() const { return size_; }

    friend class ContainerTest;          // 測試類別是朋友
};

class ContainerTest {
public:
    static void runCapacityTest() {
        Container c;
        c.push(1); c.push(2); c.push(3); c.push(4);   // 第 4 個應被擋下
        // 直接讀 private 來驗證
        std::cout << "[Test] size_ = " << c.size_
                  << ", data_={" << c.data_[0] << "," << c.data_[1]
                  << "," << c.data_[2] << "}" << std::endl;
    }
};

int main() {
    std::cout << "----- 範例 1：Complex + operator<< -----" << std::endl;
    Complex a(3, 4);
    Complex b(1, -2);
    Complex c = a + b;
    std::cout << "a = " << a << std::endl;     // (3 + 4i)
    std::cout << "b = " << b << std::endl;     // (1 - 2i)
    std::cout << "c = " << c << std::endl;     // (4 + 2i)

    std::cout << "----- 範例 2：friend class -----" << std::endl;
    Engine engine;
    Diagnostic d;
    d.report(engine);     // 引擎還沒啟動
    engine.run();
    d.report(engine);     // 啟動後

    std::cout << "----- 範例 3：Leetcode 1929 - 用 friend operator+ 做向量串接 -----" << std::endl;
    IntArray nums{1, 2, 3};
    IntArray ans = nums + nums;        // LC 1929 解答：[1,2,3,1,2,3]
    std::cout << "ans = ";
    for (int x : ans.v) std::cout << x << " ";
    std::cout << "\n";

    std::cout << "----- 範例 4：Leetcode 1313 RLE 解壓 + friend operator<< -----" << std::endl;
    RleData r({1, 2, 3, 4});               // 編碼：1 個 2、3 個 4
    std::cout << r << std::endl;            // friend 讓 << 能讀 private codes_
    std::vector<int> dec = r.decompress();
    std::cout << "解壓: ";
    for (int x : dec) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "----- 範例 5：friend class 做單元測試 -----" << std::endl;
    ContainerTest::runCapacityTest();
    return 0;
}

/* 預期輸出：
 * ----- 範例 1：Complex + operator<< -----
 * a = (3 + 4i)
 * b = (1 - 2i)
 * c = (4 + 2i)
 * ----- 範例 2：friend class -----
 * [診斷] rpm = 800, 機油溫度 = 75C
 * [診斷] rpm = 3000, 機油溫度 = 92C
 * ----- 範例 3：Leetcode 1929 - 用 friend operator+ 做向量串接 -----
 * ans = 1 2 3 1 2 3
 * ----- 範例 4：Leetcode 1313 RLE 解壓 + friend operator<< -----
 * RLE[ 1 2 3 4 ]
 * 解壓: 2 4 4 4
 * ----- 範例 5：friend class 做單元測試 -----
 * [Test] size_ = 3, data_={1,2,3}
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. friend 由類別自己授權，外部函式 / 類別藉此能讀 private 成員。
 *   2. friend 函式不是類別的成員，因此沒有 this，呼叫時也不需要物件實例。
 *   3. operator<< / operator>> 因為左運算元是 stream，必須寫成外部函式 → 常用 friend。
 *   4. friend class 一旦宣告，「對方的所有函式」都能看你的 private。
 *      因此 friend 越克制越好，避免破壞封裝。
 *
 * 【下一篇預告】
 *   13_OperatorOverloading.cpp
 *   運算子重載 (operator + - == << ...) — 讓自訂類別像內建型別一樣自然使用。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 12_Friend.cpp -o 12_Friend

// === 預期輸出 ===
// ----- 範例 1：Complex + operator<< -----
// a = (3 + 4i)
// b = (1 - 2i)
// c = (4 + 2i)
// ----- 範例 2：friend class -----
// [診斷] rpm = 800, 機油溫度 = 75C
// [診斷] rpm = 3000, 機油溫度 = 92C
// ----- 範例 3：Leetcode 1929 - 用 friend operator+ 做向量串接 -----
// ans = 1 2 3 1 2 3
// ----- 範例 4：Leetcode 1313 RLE 解壓 + friend operator<< -----
// RLE[ 1 2 3 4 ]
// 解壓: 2 4 4 4
// ----- 範例 5：friend class 做單元測試 -----
// [Test] size_ = 3, data_={1,2,3}
