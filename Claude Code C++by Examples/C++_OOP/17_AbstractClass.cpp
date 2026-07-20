/*=============================================================================
 * 檔名：17_AbstractClass.cpp
 * 主題：純虛擬函式 (Pure Virtual Function) 與 抽象類別 (Abstract Class)
 * 適合：學完 virtual function，想理解「介面 (interface) 在 C++ 怎麼寫」的人
 *
 * 【課題介紹】
 *   上一篇我們在 Animal 寫了一個 virtual speak()，實作是「(動物的某種聲音)」。
 *   但這實在很怪：動物本身應該是抽象概念，沒有具體叫聲。我們希望：
 *
 *     1. Animal 不能被「直接建立成物件」 — 「動物」沒有實體，只有「具體的某種動物」。
 *     2. 任何子類別「必須」自己實作 speak()，否則編譯不過。
 *
 *   解法：把 speak() 寫成「純虛擬函式 (pure virtual)」：
 *
 *       class Animal {
 *       public:
 *           virtual void speak() const = 0;     // 純虛擬：宣告但無實作
 *           virtual ~Animal() = default;
 *       };
 *
 *   只要一個類別「至少有一個純虛擬函式」，就會自動變成「抽象類別 (abstract class)」，
 *   有兩個結果：
 *     A. 它不能被實例化 (Animal a; ← 編譯錯誤)。
 *     B. 子類別必須覆寫所有純虛擬函式，否則它也是抽象類別 (一樣不能實例化)。
 *
 * 【抽象類別的用途：定義「介面 / 合約」】
 *   抽象類別的角色像是「告訴別人這個世界上有種類別叫 Shape，它必須能算面積」，
 *   至於實際面積怎麼算？讓 Circle 自己決定，讓 Rectangle 自己決定。
 *   這在大型系統中非常有用：
 *
 *       std::vector<std::unique_ptr<Shape>> shapes;
 *       shapes.push_back(std::make_unique<Circle>(5));
 *       shapes.push_back(std::make_unique<Rectangle>(3, 4));
 *       for (auto& s : shapes) std::cout << s->area();
 *
 *   呼叫方根本不在乎具體型別，只認介面 (Shape) → 加新形狀時不用改既有程式碼。
 *
 * 【可不可以給「純虛擬函式」實作？】
 *   可以！「純虛擬」+「有實作」聽起來矛盾，但用途是「給子類別當作預設可以呼叫」：
 *
 *       virtual void f() const = 0;          // 宣告
 *       void Base::f() const { ... }         // 還是可以提供預設實作
 *       // 子類別必須覆寫，但實作裡可以呼叫 Base::f() 來重用
 *
 *   常見應用：純虛擬解構子 (有時想讓 class 抽象，但又沒別的純虛擬函式可放，
 *   就把解構子寫成純虛擬，但仍要給實作 — 物件被銷毀時還是要跑那段)。
 *
 * 【日常實用範例】
 *   Shape 抽象類別 + Circle / Rectangle / Triangle 三個具體子類別。
 *   再示範用 std::vector<Shape*> 統一管理。
 *
 * 【對應 Leetcode】1672. Richest Customer Wealth
 *   題目：給一個 m x n 的 accounts 二維陣列，每列代表一個顧客在不同銀行的存款。
 *         回傳「最有錢顧客」的總財產 (max over rows of sum of row)。
 *   為什麼選這題：很適合示範「用抽象類別當策略 (Strategy) 介面」的設計手法 —
 *   定義一個 IWealthCalc 介面，再給一個具體實作 SumByRow，主流程只認介面，
 *   未來想換成「加權平均」或「中位數」只要再寫一個子類別、不動既有程式。
 *   這就是抽象類別在實務上最常見的價值。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/abstract_class
 *   https://cplusplus.com/doc/tutorial/polymorphism/
 *=============================================================================*/

/*
補充筆記：AbstractClass
  - AbstractClass 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - 純虛擬函式語法是 virtual void f() = 0;，它讓類別變成抽象類別，不能直接建立物件。
  - 抽象類別常用來描述介面：呼叫者只知道能做什麼，不需要知道底層如何做。
  - 即使函式是純虛擬，也可以在 class 外提供定義；derived 可選擇呼叫 Base::f() 來重用部分共通邏輯。
  - 抽象類別通常應有 virtual destructor，因為使用者很可能透過介面指標或 reference 操作 derived 物件。
  - 介面函式越多，實作者負擔越重；好的 interface 應聚焦最小必要行為，而不是把所有可能需求都塞進去。
  - 若介面只需要單一行為，也可以用 template、lambda 或 std::function 替代 virtual；選擇取決於是否需要執行期替換。
  - 抽象 base 不應在建構子中呼叫純虛擬函式；那時 derived 尚未建立完成，呼叫純虛擬通常代表設計錯誤。
  - 以抽象類別設計 Leetcode 或工作題時，重點是分離「使用者依賴的介面」和「底層資料結構的實作」。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】pure virtual function 與 abstract class
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. virtual function 與 pure virtual function 的差別？
//     答：`virtual void f();` 提供預設實作，derived 可覆寫也可不覆寫；
//         `virtual void f() = 0;` 是 pure virtual，使該 class 成為 abstract class
//         → 不能被實例化，derived 必須覆寫所有 pure virtual，否則自己也是 abstract。
//         本檔的 Shape::area()/perimeter() 是前者的對照組 describe() 的最好例子。
//     追問：抽象類別可以有 constructor 與成員資料嗎？（可以；它只是不能單獨實例化，
//           derived 建構時仍會呼叫它的 constructor）
//
// 🔥 Q2. pure virtual function 可以有實作嗎？pure virtual destructor 呢？
//     答：可以。`= 0` 只表示「derived 必須覆寫」，不代表不能提供定義；derived 可用
//         `Base::f()` 顯式呼叫來重用共通邏輯（本檔 Circle::describe() 就呼叫
//         Shape::describe()）。pure virtual destructor 也合法，而且「必須」提供定義
//         —— derived 解構結束後一定會呼叫到它。
//     追問：什麼時候會想用 pure virtual destructor？（想讓 class 抽象，但又沒有別的
//           函式適合設成 pure virtual 時）
//
// Q3. 需要「可替換的行為」時，一定要用 abstract class 嗎？
//     答：不一定。abstract class + virtual 是「執行期」可替換（付出 vptr 與間接呼叫
//         成本，且介面一改所有實作都要跟著改）；若型別在編譯期就決定，template 或
//         `std::function` / lambda 往往更輕。判準是：需不需要在執行期換掉實作。
//     追問：介面該設計多大？（越小越好；每加一個 virtual 函式，所有 derived 都被迫跟進）
//
// ⚠️ 陷阱. 可以在 constructor 裡呼叫 virtual / pure virtual 函式嗎？
//     答：不是「不能」，而是「不會如你預期」。Base 的 constructor 執行時，物件的動態
//         型別就是 Base，derived 的 override 不會生效。若呼叫的是 pure virtual，
//         則是 undefined behavior（實務上常見 abort 並印 "pure virtual method called"）。
//     為什麼會錯：多數人以為物件「一開始就是 Derived」；實際建構順序是 base → derived，
//         base 建構期間 derived 部分根本還不存在。要做建構後初始化就用 factory 包兩步。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <memory>     // std::unique_ptr (第 20 篇深入)
#include <cmath>      // M_PI 在某些編譯器有提供，這裡自己定義避免相依
#include <string>

constexpr double PI = 3.14159265358979323846;

// -----------------------------------------------------------------------------
// Shape 抽象類別 (介面)
// -----------------------------------------------------------------------------
class Shape {
public:
    // 純虛擬：每個子類別都必須自己實作
    virtual double area()      const = 0;
    virtual double perimeter() const = 0;

    // 一個普通虛擬函式：有預設實作，子類別可以選擇覆寫
    virtual void describe() const {
        std::cout << "面積 = " << area()
                  << ", 周長 = " << perimeter() << std::endl;
    }

    // 「打算被繼承」的父類別，解構子要設成 virtual
    virtual ~Shape() = default;
};

// -----------------------------------------------------------------------------
// 具體子類別：Circle 圓形
// -----------------------------------------------------------------------------
class Circle : public Shape {
private:
    double r_;
public:
    explicit Circle(double r) : r_(r) {}

    // override 是好習慣，編譯器會幫你檢查簽名
    double area()      const override { return PI * r_ * r_; }
    double perimeter() const override { return 2 * PI * r_; }

    // 我們覆寫 describe，讓圓形多印一行半徑資訊
    void describe() const override {
        std::cout << "[Circle r=" << r_ << "] ";
        Shape::describe();          // 借用父類別的版本，不用重寫一次
    }
};

// -----------------------------------------------------------------------------
// 具體子類別：Rectangle 長方形
// -----------------------------------------------------------------------------
class Rectangle : public Shape {
private:
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area()      const override { return w_ * h_; }
    double perimeter() const override { return 2 * (w_ + h_); }

    void describe() const override {
        std::cout << "[Rectangle " << w_ << "x" << h_ << "] ";
        Shape::describe();
    }
};

// -----------------------------------------------------------------------------
// 具體子類別：Triangle 直角三角形 (簡單版，斜邊長手算)
// -----------------------------------------------------------------------------
class Triangle : public Shape {
private:
    double a_, b_;     // 兩股
public:
    Triangle(double a, double b) : a_(a), b_(b) {}
    double area()      const override { return 0.5 * a_ * b_; }
    double perimeter() const override {
        return a_ + b_ + std::sqrt(a_ * a_ + b_ * b_);
    }
    // 沒覆寫 describe，會用 Shape 的預設版本
};

// -----------------------------------------------------------------------------
// 抽象類別當「策略介面」：Leetcode 1672 Richest Customer Wealth
// -----------------------------------------------------------------------------
// IWealthCalc 是一個只有純虛擬函式的「介面」。任何具體計算策略都必須實作 wealth()。
// 主流程只認 IWealthCalc，要換策略時不動既有程式 → 開放-封閉原則 (OCP)。
class IWealthCalc {
public:
    virtual double wealth(const std::vector<int>& row) const = 0;
    virtual ~IWealthCalc() = default;
};

// 具體策略 1：把整列加總當財富 (LC 1672 的官方解法)
class SumByRow : public IWealthCalc {
public:
    double wealth(const std::vector<int>& row) const override {
        double s = 0;
        for (int x : row) s += x;
        return s;
    }
};

// 主流程：對每一列計算財富、回傳最大值。它「不知道」strategy 內部怎麼算，
// 只透過 IWealthCalc 介面叫 wealth()，這就是抽象類別在實務最常見的用法。
double maximumWealth(const std::vector<std::vector<int>>& accounts,
                     const IWealthCalc& strategy) {
    double best = 0;
    for (const auto& row : accounts) {
        double w = strategy.wealth(row);
        if (w > best) best = w;
    }
    return best;
}

// -----------------------------------------------------------------------------
// 範例：對應 Leetcode 1603 - Design Parking System (用抽象類別當分配策略)
// -----------------------------------------------------------------------------
// 介面：IParkingStrategy 定義「給一輛車的類型，回傳是否停得進去」。
// 不同策略 (嚴格 vs 靈活) 可以實作不同的政策；主程式只認介面。
class IParkingStrategy {
public:
    virtual bool tryPark(int carType) = 0;
    virtual ~IParkingStrategy() = default;
};

// 具體策略：固定容量的嚴格停車場 (LC 1603 標準解法)
class StrictParking : public IParkingStrategy {
private:
    int slots_[4] = {0, 0, 0, 0};      // [1]=big, [2]=medium, [3]=small
public:
    StrictParking(int b, int m, int s) {
        slots_[1] = b; slots_[2] = m; slots_[3] = s;
    }
    bool tryPark(int carType) override {
        if (carType < 1 || carType > 3) return false;
        if (slots_[carType] > 0) { --slots_[carType]; return true; }
        return false;
    }
};

// -----------------------------------------------------------------------------
// 範例：日常實用 - PaymentMethod 抽象類別 (Strategy Pattern)
// -----------------------------------------------------------------------------
// 結帳系統常常會有多種付款方式，抽象類別讓未來加新方法不用改主流程。
class PaymentMethod {
public:
    virtual void pay(double amount) const = 0;
    virtual std::string name() const = 0;
    virtual ~PaymentMethod() = default;
};

class CreditCardPayment : public PaymentMethod {
public:
    void pay(double amount) const override {
        std::cout << "信用卡扣款 $" << amount << std::endl;
    }
    std::string name() const override { return "CreditCard"; }
};

class CashPayment : public PaymentMethod {
public:
    void pay(double amount) const override {
        std::cout << "現金支付 $" << amount << std::endl;
    }
    std::string name() const override { return "Cash"; }
};

int main() {
    // Shape s;                    // ← 編譯錯誤！抽象類別不能實例化

    // 把不同形狀放進同一個容器，統一處理 → 多型的核心優勢
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(5));
    shapes.push_back(std::make_unique<Rectangle>(3, 4));
    shapes.push_back(std::make_unique<Triangle>(3, 4));

    for (const auto& sp : shapes) {
        sp->describe();             // 各自呼叫到自己版本的 describe / area / perimeter
    }

    // 計算所有形狀面積總和 — 完全不用知道是哪一種
    double totalArea = 0;
    for (const auto& sp : shapes) totalArea += sp->area();
    std::cout << "全部面積加總 = " << totalArea << std::endl;

    std::cout << "----- Leetcode 1672 用抽象類別當 Strategy -----" << std::endl;
    std::vector<std::vector<int>> accounts = {
        {1, 2, 3},        // 顧客 0 共 6
        {3, 2, 1},        // 顧客 1 共 6
        {1, 5, 5}         // 顧客 2 共 11
    };
    SumByRow strategy;
    std::cout << "最有錢顧客的財產 = " << maximumWealth(accounts, strategy)
              << "  (預期 11)" << std::endl;

    std::cout << "----- Leetcode 1603 抽象停車策略 -----" << std::endl;
    std::unique_ptr<IParkingStrategy> ps = std::make_unique<StrictParking>(1, 1, 0);
    std::cout << ps->tryPark(1) << std::endl;   // 1
    std::cout << ps->tryPark(2) << std::endl;   // 1
    std::cout << ps->tryPark(3) << std::endl;   // 0
    std::cout << ps->tryPark(1) << std::endl;   // 0

    std::cout << "----- PaymentMethod 多型 -----" << std::endl;
    std::vector<std::unique_ptr<PaymentMethod>> methods;
    methods.push_back(std::make_unique<CreditCardPayment>());
    methods.push_back(std::make_unique<CashPayment>());
    for (const auto& m : methods) {
        std::cout << "方式: " << m->name() << " → ";
        m->pay(100.0);          // 各自走自己版本
    }
    return 0;
}

/* 預期輸出：
 * [Circle r=5] 面積 = 78.5398, 周長 = 31.4159
 * [Rectangle 3x4] 面積 = 12, 周長 = 14
 * 面積 = 6, 周長 = 12
 * 全部面積加總 = 96.5398
 * ----- Leetcode 1672 用抽象類別當 Strategy -----
 * 最有錢顧客的財產 = 11  (預期 11)
 * ----- Leetcode 1603 抽象停車策略 -----
 * 1
 * 1
 * 0
 * 0
 * ----- PaymentMethod 多型 -----
 * 方式: CreditCard → 信用卡扣款 $100
 * 方式: Cash → 現金支付 $100
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 在 virtual 函式宣告後加 = 0 → 純虛擬函式。
 *   2. 含純虛擬函式的類別 = 抽象類別，不能直接 new / 宣告為物件。
 *   3. 子類別必須實作所有純虛擬函式，否則自身也是抽象類別。
 *   4. 抽象類別常用來「定義介面」，搭配多型實作擴充性高的設計。
 *   5. 純虛擬函式可以同時提供實作，作為子類別的預設行為。
 *
 * 【下一篇預告】
 *   18_VirtualDestructor.cpp
 *   虛擬解構子 — 為什麼父類別解構子幾乎都要 virtual？
 *   不寫會發生什麼災難？
 *=============================================================================*/
