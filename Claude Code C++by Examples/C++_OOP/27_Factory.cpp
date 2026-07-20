/*=============================================================================
 * 檔名：27_Factory.cpp
 * 主題：工廠模式 (Factory Pattern) - 把「建立物件」的邏輯集中
 * 適合：學完抽象類別與 unique_ptr，想學第二個常見設計模式的人
 *
 * 【課題介紹】
 *   想像第 17 篇我們有 Shape 抽象類別，與 Circle / Rectangle / Triangle 三種子類別。
 *   有些情境下我們在執行期才知道「要建立哪一種」 — 例如：
 *
 *     - 從設定檔讀到 "circle" 字串 → 建一個 Circle
 *     - 使用者選擇下拉選單 → 建對應形狀
 *
 *   如果寫在每個用到的地方都來一段：
 *
 *       Shape* s = nullptr;
 *       if      (type == "circle")    s = new Circle(...);
 *       else if (type == "rectangle") s = new Rectangle(...);
 *       ...
 *
 *   程式碼會散落到處都是 → 新增一種形狀就要改 N 個地方。
 *   解法：「工廠 (Factory)」設計模式：把建立物件的邏輯集中到一個地方。
 *
 *       「工廠 = 一個函式 / 類別，根據輸入參數回傳對應的子類別物件，
 *        通常以父類別 (抽象介面) 的指標 / 智慧指標形式回傳。」
 *
 *   呼叫端只看到工廠回傳的「Shape」介面，不關心是哪個具體子類別 — 與多型完美搭配。
 *
 * 【常見的兩種寫法】
 *   1. 「工廠函式 (Factory Function / Function-style Factory)」
 *      簡單情境，一個 free function 即可。
 *
 *   2. 「工廠類別 (Factory Class)」
 *      有更多狀態時 (例如註冊表)，把工廠包成類別。
 *
 *   進階變體還有 Abstract Factory、Builder 等，本篇先學最入門的 (1)。
 *
 * 【為什麼回傳 std::unique_ptr<Shape>？】
 *   - 自動釋放，不會洩漏
 *   - 表達「呼叫者擁有這個物件」的意圖
 *   - 即便子類別內部有資源，由 Shape 的 virtual destructor 統一釋放 (參考第 18 篇)
 *
 * 【日常實用】
 *   你可以把 ShapeType 換成從 JSON / 設定檔讀來的字串，立刻可以做成
 *   「資料驅動 (data-driven)」的繪圖系統。
 *
 * 【參考】
 *   設計模式經典書 GoF (Design Patterns: Elements of Reusable OO Software)
 *   https://en.cppreference.com/w/cpp/memory/unique_ptr
 *=============================================================================*/

/*
補充筆記：Factory
  - Factory 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - Factory 把「根據條件建立哪個 derived class」的決策集中起來，呼叫者只依賴 base interface。
  - Factory 常回傳 std::unique_ptr<Base>，表示呼叫者取得獨占所有權，也避免裸指標 delete 責任不清。
  - 建立多型物件時 base class 需要 virtual destructor，否則 unique_ptr<Base> 銷毀 derived 物件會有風險。
  - 簡單 Factory 可用 switch/if 選型別；型別很多或需要插件化時，可改成註冊表 map<string, creator function>。
  - Factory 能降低呼叫端和具體類別耦合，但不應把大量業務規則都塞進建立函式，否則 Factory 會變成巨大條件集合。
  - 若建構可能失敗，可考慮回傳 nullptr、std::optional、std::expected 或拋例外；選擇要和專案錯誤處理風格一致。
  - Factory 函式名稱應表達建立語意，例如 createParser、makeShape；不要只命名 get，因為 get 聽起來像借用既有物件。
  - 這個模式常和 abstract class 搭配：介面穩定，具體類別可替換，呼叫者測試時也能換成 fake 實作。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Factory 工廠模式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. constructor 可以是 virtual 嗎？那要「依型別動態建立物件」怎麼辦？
//     答：不行。virtual 分派需要 vptr，而 vptr 是在 constructor 執行過程中才被設定的
//         —— 「還沒建構就要靠建構結果去分派」邏輯上不成立。要在執行期決定建立哪個
//         型別，就用 **factory pattern**（本檔 makeShape / ShapeFactory::create），
//         或 clone 慣用法 `virtual Base* clone() const`（即所謂 virtual constructor idiom）。
//     追問：destructor 為什麼就可以是 virtual？（解構時物件已完整建構，vptr 有效）
//
// 🔥 Q2. 為什麼 factory 應該回傳 `std::unique_ptr<Base>` 而不是裸指標？
//     答：① 明確表達「所有權轉移給呼叫端」，呼叫端不會忘記 delete 或搞不清誰負責；
//         ② 例外安全，中途 return 也不會洩漏；③ 呼叫端只依賴抽象介面 Base，
//         具體型別完全被封在工廠裡。前提是 Base 必須有 virtual destructor（第 18 篇）。
//     追問：建構可能失敗時怎麼表達？（回傳 nullptr（本檔的做法）、std::optional、
//           C++23 的 std::expected 或拋例外 —— 選哪個要和專案錯誤處理風格一致）
//
// Q3. 什麼是 covariant return type（協變回傳型別）？
//     答：覆寫 virtual 函式時，回傳型別可以從 `Base*` / `Base&` 收窄成
//         `Derived*` / `Derived&`（必須是指標或參考，且該繼承是可存取、非歧義的）。
//         典型用途正是 clone：`virtual Derived* clone() const override;`。
//     追問：那回傳 `std::unique_ptr<Derived>` 去覆寫 `unique_ptr<Base>` 可以嗎？
//           （不行 —— covariance 只適用於裸指標與參考，smart pointer 是兩個
//           不同的 class template，沒有這層語言規則）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>

constexpr double PI = 3.14159265358979323846;

// -----------------------------------------------------------------------------
// 沿用第 17 篇的 Shape / Circle / Rectangle / Triangle
// -----------------------------------------------------------------------------
class Shape {
public:
    virtual double area() const = 0;
    virtual std::string name() const = 0;
    virtual ~Shape() = default;
};

class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return PI * r_ * r_; }
    std::string name() const override { return "Circle"; }
};

class Rectangle : public Shape {
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    std::string name() const override { return "Rectangle"; }
};

class Triangle : public Shape {
    double a_, b_;
public:
    Triangle(double a, double b) : a_(a), b_(b) {}
    double area() const override { return 0.5 * a_ * b_; }
    std::string name() const override { return "Triangle"; }
};

// -----------------------------------------------------------------------------
// 寫法 1：工廠函式 (Free Function Factory)
// -----------------------------------------------------------------------------
// 在這個函式裡集中所有「字串 → 子類別」的對應，外面就不用關心細節了。
std::unique_ptr<Shape> makeShape(const std::string& type) {
    if (type == "circle")    return std::make_unique<Circle>(3.0);
    if (type == "rectangle") return std::make_unique<Rectangle>(4.0, 5.0);
    if (type == "triangle")  return std::make_unique<Triangle>(3.0, 4.0);
    return nullptr;          // 未知型別 → 回傳空指標讓呼叫者處理
}

// -----------------------------------------------------------------------------
// 寫法 2：工廠類別 (Factory Class)，可以拿來「擴充註冊」
// -----------------------------------------------------------------------------
// 進階：以後可以呼叫 ShapeFactory::registerType("hexagon", ...) 動態加入新類型，
// 但本篇先用最簡單寫法。
class ShapeFactory {
public:
    // 根據枚舉建立 (比字串更安全 — 編譯期可檢查打字錯誤)
    enum class Type { Circle, Rectangle, Triangle };

    static std::unique_ptr<Shape> create(Type t) {
        switch (t) {
            case Type::Circle:    return std::make_unique<Circle>(2.0);
            case Type::Rectangle: return std::make_unique<Rectangle>(3.0, 4.0);
            case Type::Triangle:  return std::make_unique<Triangle>(5.0, 12.0);
        }
        return nullptr;
    }
};

// -----------------------------------------------------------------------------
// 範例：對應 Leetcode 1603 - Design Parking System (工廠建立不同尺寸的停車場)
// -----------------------------------------------------------------------------
// 介面：IParking 統一 addCar 行為
// 不同預設規格 (Small / Standard / Mega) 由工廠決定建構參數，呼叫者只認介面。
class IParking {
public:
    virtual bool addCar(int carType) = 0;
    virtual ~IParking() = default;
};

class SimpleParking : public IParking {
private:
    int slots_[4] = {0, 0, 0, 0};
public:
    SimpleParking(int b, int m, int s) {
        slots_[1] = b; slots_[2] = m; slots_[3] = s;
    }
    bool addCar(int carType) override {
        if (carType < 1 || carType > 3 || slots_[carType] <= 0) return false;
        --slots_[carType];
        return true;
    }
};

// 工廠函式：根據停車場規格回傳預設配置好的物件
std::unique_ptr<IParking> makeParking(const std::string& size) {
    if (size == "small")    return std::make_unique<SimpleParking>(1, 1, 1);
    if (size == "standard") return std::make_unique<SimpleParking>(5, 10, 20);
    if (size == "mega")     return std::make_unique<SimpleParking>(50, 100, 200);
    return nullptr;
}

// -----------------------------------------------------------------------------
// 範例：日常實用 - Notification Factory
// -----------------------------------------------------------------------------
// 工作上常見：根據用戶設定建立不同通知通道 (Email / SMS / Push)。
class INotifier {
public:
    virtual void send(const std::string& msg) const = 0;
    virtual ~INotifier() = default;
};

class EmailNotifier : public INotifier {
public:
    void send(const std::string& msg) const override {
        std::cout << "  [Email] " << msg << std::endl;
    }
};

class SmsNotifier : public INotifier {
public:
    void send(const std::string& msg) const override {
        std::cout << "  [SMS] " << msg << std::endl;
    }
};

std::unique_ptr<INotifier> makeNotifier(const std::string& kind) {
    if (kind == "email") return std::make_unique<EmailNotifier>();
    if (kind == "sms")   return std::make_unique<SmsNotifier>();
    return nullptr;
}

int main() {
    std::cout << "===== 寫法 1：工廠函式 + 字串 =====" << std::endl;
    std::vector<std::string> requests = {"circle", "rectangle", "triangle", "hexagon"};
    for (const auto& req : requests) {
        auto s = makeShape(req);
        if (s) {
            std::cout << "  做了一個 " << s->name()
                      << "，面積 = " << s->area() << "\n";
        } else {
            std::cout << "  未知形狀: " << req << "\n";
        }
    }

    std::cout << "===== 寫法 2：工廠類別 + enum =====" << std::endl;
    auto shapes = std::vector<std::unique_ptr<Shape>>{};
    shapes.push_back(ShapeFactory::create(ShapeFactory::Type::Circle));
    shapes.push_back(ShapeFactory::create(ShapeFactory::Type::Rectangle));
    shapes.push_back(ShapeFactory::create(ShapeFactory::Type::Triangle));

    double total = 0;
    for (auto& s : shapes) {
        std::cout << "  " << s->name() << " area = " << s->area() << "\n";
        total += s->area();
    }
    std::cout << "  全部面積總和 = " << total << "\n";

    std::cout << "===== Leetcode 1603 工廠建立停車場 =====" << std::endl;
    auto lot = makeParking("small");
    if (lot) {
        std::cout << "addCar(1) = " << lot->addCar(1) << " (預期 1)\n";
        std::cout << "addCar(1) = " << lot->addCar(1) << " (預期 0)\n";
        std::cout << "addCar(2) = " << lot->addCar(2) << " (預期 1)\n";
    }

    std::cout << "===== Notification Factory =====" << std::endl;
    std::vector<std::unique_ptr<INotifier>> notifiers;
    notifiers.push_back(makeNotifier("email"));
    notifiers.push_back(makeNotifier("sms"));
    for (const auto& n : notifiers) {
        n->send("您的訂單已成立");
    }
    return 0;
}

/* 預期輸出：
 * ===== 寫法 1：工廠函式 + 字串 =====
 *   做了一個 Circle，面積 = 28.2743
 *   做了一個 Rectangle，面積 = 20
 *   做了一個 Triangle，面積 = 6
 *   未知形狀: hexagon
 * ===== 寫法 2：工廠類別 + enum =====
 *   Circle area = 12.5664
 *   Rectangle area = 12
 *   Triangle area = 30
 *   全部面積總和 = 54.5664
 * ===== Leetcode 1603 工廠建立停車場 =====
 * addCar(1) = 1 (預期 1)
 * addCar(1) = 0 (預期 0)
 * addCar(2) = 1 (預期 1)
 * ===== Notification Factory =====
 *   [Email] 您的訂單已成立
 *   [SMS] 您的訂單已成立
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 工廠模式把「決定建立哪種子類別」的邏輯集中，呼叫端只認介面 (父類別)。
 *   2. 簡單情境用 free function；需要狀態時用 factory class。
 *   3. 回傳型別常用 std::unique_ptr<Base>，自動管理生命週期 + 表達擁有權移轉。
 *   4. 配合多型，讓系統易於擴充：加新形狀只要動工廠 + 寫新子類別。
 *   5. 真實工程中還會看到 Abstract Factory / Builder 等更進階的版本。
 *
 * 【下一篇預告】
 *   28_LRUCache.cpp
 *   綜合應用 — 用學過的所有觀念實作 Leetcode 146. LRU Cache，
 *   把整個 OOP 學習做個收尾。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 27_Factory.cpp -o 27_Factory

// === 預期輸出 ===
// ===== 寫法 1：工廠函式 + 字串 =====
//   做了一個 Circle，面積 = 28.2743
//   做了一個 Rectangle，面積 = 20
//   做了一個 Triangle，面積 = 6
//   未知形狀: hexagon
// ===== 寫法 2：工廠類別 + enum =====
//   Circle area = 12.5664
//   Rectangle area = 12
//   Triangle area = 30
//   全部面積總和 = 54.5664
// ===== Leetcode 1603 工廠建立停車場 =====
// addCar(1) = 1 (預期 1)
// addCar(1) = 0 (預期 0)
// addCar(2) = 1 (預期 1)
// ===== Notification Factory =====
//   [Email] 您的訂單已成立
//   [SMS] 您的訂單已成立
