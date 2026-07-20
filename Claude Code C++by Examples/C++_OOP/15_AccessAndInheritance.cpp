/*=============================================================================
 * 檔名：15_AccessAndInheritance.cpp
 * 主題：三種繼承方式 (public / protected / private) 與 protected 成員
 * 適合：學會基本繼承後，想知道「: public Base」中那個 public 換成別的會怎樣的人
 *
 * 【課題介紹】
 *   上一篇我們都寫 class Dog : public Animal，那個「public」是什麼意思？
 *   它是「繼承存取等級」(inheritance access)，控制「父類別成員在子類別中的可見度」。
 *
 *   完整三種：
 *     class B : public  A { ... };   // 最常見：is-a 關係
 *     class B : protected A { ... };  // 較少見
 *     class B : private  A { ... };   // 較少見：implemented-in-terms-of (組合代替方案)
 *
 * 【三種繼承的影響表 (重要！背一下)】
 *
 *   父類別成員  | public 繼承  | protected 繼承 | private 繼承
 *   -------------------------------------------------------------------
 *   public      | public       | protected      | private
 *   protected   | protected    | protected      | private
 *   private     | (不可見)     | (不可見)       | (不可見)
 *
 *   可以看到：
 *     - public 繼承不會「降級」存取等級。
 *     - protected / private 繼承會把父類別的 public 成員「降級」為 protected/private。
 *     - private 成員「永遠」對子類別不可見。
 *
 * 【實務觀察】
 *   - 99% 的繼承用 public：表示 is-a 關係 (子類別「就是」父類別的一種)。
 *   - private 繼承多半用來「以實作為目的的繼承」(implemented-in-terms-of)，
 *     有點像把父類別「藏起來」當成自己的內部工具，但這種需求多半用「組合 (has-a)」
 *     就好了，因此 private 繼承很少寫。
 *   - protected 繼承幾乎不出現，遇到再說。
 *
 * 【protected 成員】
 *   protected 對「自己 + 子類別」可見，對「外面」不可見。它讓父類別把實作細節
 *   分享給子類別、又不會把細節公開給整個世界，是繼承關係下很自然的等級。
 *
 *   一個常見爭議：把成員設成 protected 雖然方便子類別擴充，但也代表「所有子類別
 *   都依賴這份內部表示」，未來重構成本變高。慣例：「能 private 就 private」。
 *
 * 【日常實用範例】
 *   Employee (員工) 是父類別，Manager (經理) 是子類別。經理是一種員工，所以用
 *   public 繼承。再示範「不該對外公開的內部紀錄欄位」用 protected 共享給子類。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/access     (member access)
 *   https://en.cppreference.com/w/cpp/language/derived_class
 *=============================================================================*/

/*
補充筆記：AccessAndInheritance
  - AccessAndInheritance 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - public/private/protected inheritance 會改變 base public/protected 成員在 derived 中的可見程度；最常見也最符合 is-a 語意的是 public inheritance。
  - private inheritance 表示以 base 實作 derived，而不是讓 derived 被當成 base；多數情況 composition 更清楚。
  - protected inheritance 很少需要；它讓 derived 的子類別仍可接觸 base 介面，容易製造難懂的層級。
  - 存取控制不改變物件實際包含哪些 base subobject，只改變哪些名稱可從哪些地方使用。
  - using Base::func 可以把被隱藏的 base overload 帶回 derived scope；否則 derived 宣告同名函式會隱藏 base 的整組 overload。
  - private 成員不能被 derived 直接碰，不代表 derived 物件沒有那份資料；這是封裝保護 base invariant 的方式。
  - 若外部程式需要把 Derived* 轉成 Base*，public inheritance 才能自然成立；private/protected inheritance 會限制這種轉換。
  - 設計繼承時先決定語意，再選存取權限；不要先為了通過編譯改 public/protected，否則類別關係會變混亂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三種繼承方式與存取控制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. public / protected / private 繼承的差別？
//     答：決定 base 成員在 derived 中的「最高存取等級」。public 繼承 — base 的
//         public 仍 public、protected 仍 protected（is-a 關係）；protected 繼承 —
//         base 的 public 降為 protected；private 繼承 — 全部降為 private。
//         只有 public 繼承允許外部把 Derived* 隱式轉成 Base*。
//     追問：private 繼承 vs 組合（composition）該選哪個？（優先組合；只有需要覆寫
//           base 的 virtual、或需要 empty base optimization 時才用 private 繼承）
//
// 🔥 Q2. protected 成員與 private 成員對子類別有什麼差別？慣例是什麼？
//     答：private 連子類別都碰不到（本檔的 Employee::ssn_）；protected 對自己與
//         子類別可見、對外不可見（name_ / salary_）。慣例是「能 private 就 private」
//         —— protected 等於把內部表示公開給所有子類別，日後重構成本很高。
//     追問：private 成員子類別看不到，代表 Derived 物件裡沒有那份資料嗎？
//           （不是。存取控制只管「名稱能從哪裡使用」，base subobject 仍完整存在）
//
// Q3. 什麼是 diamond problem？virtual inheritance 如何解決？
//     答：B、C 各自繼承 A，D 同時繼承 B、C → D 內含「兩份 A subobject」，存取 A 的
//         成員產生 ambiguity 且浪費空間。解法是 `class B : virtual public A`、
//         `class C : virtual public A`，讓 A 成為 virtual base，D 中只有一份共享的 A。
//     追問：virtual base 由誰負責初始化？（由最衍生類別 D 直接呼叫 A 的 constructor；
//           B / C 初始化列表中對 A 的初始化會被忽略）
//
// ⚠️ 陷阱. Derived 定義了同名函式後，Base 的其他重載還能用嗎？
//     答：不能。derived 只要定義了同名成員函式，base 中「所有」同名重載都被隱藏
//         （name hiding），即使參數型別完全不同。解法：在 derived 寫 `using Base::f;`
//         把 base 的整組重載拉回 derived scope。
//     為什麼會錯：多數人以為 overload resolution 會跨 base / derived 一起比對；
//         實際上「名稱查找」在 derived scope 找到就停止，根本輪不到 overload resolution。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 範例 1：Employee → Manager (public 繼承示範 + protected 成員)
// -----------------------------------------------------------------------------
class Employee {
private:
    // private 成員：連子類別都看不到，必須透過父類別 public/protected 介面存取
    std::string ssn_;     // 社會安全碼，極機密

protected:
    // protected 成員：父類別與子類別都能直接讀寫
    std::string name_;
    double      salary_;

public:
    Employee(const std::string& ssn, const std::string& name, double salary)
        : ssn_(ssn), name_(name), salary_(salary) {}

    void show() const {
        // 注意 ssn_ 即便是父類別自己也只能在自己 (Employee) 的程式碼內讀
        std::cout << "Employee[" << name_ << ", $" << salary_
                  << ", ssn=***-**-" << ssn_.substr(ssn_.size() - 4) << "]"
                  << std::endl;
    }

    void raise(double amount) { salary_ += amount; }
};

class Manager : public Employee {
private:
    std::vector<std::string> reports_;     // 直接下屬名單

public:
    Manager(const std::string& ssn, const std::string& name, double salary)
        : Employee(ssn, name, salary) {}

    void addReport(const std::string& reportName) {
        reports_.push_back(reportName);
    }

    // 子類別可以直接讀寫 protected 成員 name_ / salary_，
    // 但完全碰不到 ssn_ (它是 private)
    void giveBonus(double pct) {
        salary_ += salary_ * pct;          // 這行能寫，因為 salary_ 是 protected
        std::cout << name_ << " 得到 " << pct * 100
                  << "% 加薪，新薪水 $" << salary_ << std::endl;
    }

    void show() const {
        std::cout << "Manager[" << name_ << ", $" << salary_
                  << ", reports=" << reports_.size() << "]" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 2：對比 public vs private 繼承 (簡短示意，理解差別即可)
// -----------------------------------------------------------------------------
class Engine {
public:
    void start() { std::cout << "Engine started" << std::endl; }
};

// (A) public 繼承：對外仍然可以呼叫 start()
class Car : public Engine {};

// (B) private 繼承：對外把 start() 藏起來，只有 SportsCar 內部可用
class SportsCar : private Engine {
public:
    void launch() {
        start();    // OK：自己內部可以用
        std::cout << "SportsCar launched" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 706 - Design HashMap
// -----------------------------------------------------------------------------
// 題目簡述：設計 HashMap (key=value)，支援 put / get / remove。
// 重點：用 protected 把 buckets 開放給子類別擴充 (例：CountingHashMap 加統計)。
class MyHashMap {
protected:
    static constexpr int B = 769;
    std::vector<std::vector<std::pair<int, int>>> table_;     // protected 讓子類擴充

    int h(int key) const { return key % B; }

public:
    MyHashMap() : table_(B) {}

    void put(int key, int value) {
        auto& bucket = table_[h(key)];
        for (auto& kv : bucket) {
            if (kv.first == key) { kv.second = value; return; }
        }
        bucket.push_back({key, value});
    }

    int get(int key) const {
        const auto& bucket = table_[h(key)];
        for (const auto& kv : bucket) {
            if (kv.first == key) return kv.second;
        }
        return -1;
    }

    void remove(int key) {
        auto& bucket = table_[h(key)];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->first == key) { bucket.erase(it); return; }
        }
    }
};

// 子類別：把 protected 的 table_ 用來計算「目前共有幾筆資料」
class CountingHashMap : public MyHashMap {
public:
    int totalEntries() const {
        int sum = 0;
        for (const auto& bucket : table_) sum += bucket.size();   // 用到 protected
        return sum;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - Logger → TimestampedLogger
// -----------------------------------------------------------------------------
// 父類別有一個 protected 寫出方法，子類別擴充加上時間戳。
class BaseLogger {
protected:
    std::string moduleName_;
    // protected：子類別可以呼叫，外部不能
    void writeRaw(const std::string& msg) const {
        std::cout << "[" << moduleName_ << "] " << msg << std::endl;
    }
public:
    explicit BaseLogger(const std::string& m) : moduleName_(m) {}
};

class TimestampedLogger : public BaseLogger {
public:
    explicit TimestampedLogger(const std::string& m) : BaseLogger(m) {}
    void info(const std::string& msg) const {
        // 直接用父類別 protected 方法 (外部使用者無法直接呼叫 writeRaw)
        writeRaw("[INFO @ t=123] " + msg);
    }
};

int main() {
    std::cout << "===== 範例 1：Employee / Manager =====" << std::endl;
    Manager bob("123-45-6789", "Bob", 80000);
    bob.show();
    bob.addReport("Alice");
    bob.addReport("Charlie");
    bob.giveBonus(0.10);          // 透過 protected 成員加薪
    bob.show();
    bob.raise(5000);              // 也能用父類別的 public 介面
    bob.show();

    std::cout << "===== 範例 2：public vs private 繼承 =====" << std::endl;
    Car car;
    car.start();                  // OK：public 繼承沒有降級

    SportsCar sc;
    sc.launch();                  // OK：launch() 內部可以呼叫 start()
    // sc.start();                // ← 編譯錯誤！因為 private 繼承後 start 變 private

    std::cout << "===== 範例 3：Leetcode 706 + protected 擴充 =====" << std::endl;
    CountingHashMap hm;
    hm.put(1, 100);
    hm.put(2, 200);
    hm.put(1, 999);        // 更新 key=1
    std::cout << "get(1) = " << hm.get(1) << " (預期 999)" << std::endl;
    std::cout << "get(3) = " << hm.get(3) << " (預期 -1)" << std::endl;
    std::cout << "目前筆數 = " << hm.totalEntries() << " (預期 2)" << std::endl;

    std::cout << "===== 範例 4：BaseLogger → TimestampedLogger =====" << std::endl;
    TimestampedLogger lg("auth");
    lg.info("使用者登入成功");
    // lg.writeRaw("xxx");   // 編譯錯誤：writeRaw 是 protected
    return 0;
}

/* 預期輸出：
 * ===== 範例 1：Employee / Manager =====
 * Manager[Bob, $80000, reports=0]
 * Bob 得到 10% 加薪，新薪水 $88000
 * Manager[Bob, $88000, reports=2]
 * Manager[Bob, $93000, reports=2]
 * ===== 範例 2：public vs private 繼承 =====
 * Engine started
 * Engine started
 * SportsCar launched
 * ===== 範例 3：Leetcode 706 + protected 擴充 =====
 * get(1) = 999 (預期 999)
 * get(3) = -1 (預期 -1)
 * 目前筆數 = 2 (預期 2)
 * ===== 範例 4：BaseLogger → TimestampedLogger =====
 * [auth] [INFO @ t=123] 使用者登入成功
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 繼承存取等級會決定父類別成員「在子類別中對外的可見度」。
 *   2. 99% 用 public 繼承 → 表示 is-a 關係。
 *   3. private/protected 成員：private 連子類都看不到；protected 子類能用、外人不能。
 *   4. 「能 private 就 private」是封裝的好習慣，protected 只在子類確實需要時才開。
 *
 * 【下一篇預告】
 *   16_VirtualFunction.cpp
 *   多型 (Polymorphism) 與虛擬函式 — 父類別指標呼叫子類別行為的奇妙機制，
 *   並用 Leetcode 232. Implement Queue using Stacks 練習。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 15_AccessAndInheritance.cpp -o 15_AccessAndInheritance

// === 預期輸出 ===
// ===== 範例 1：Employee / Manager =====
// Manager[Bob, $80000, reports=0]
// Bob 得到 10% 加薪，新薪水 $88000
// Manager[Bob, $88000, reports=2]
// Manager[Bob, $93000, reports=2]
// ===== 範例 2：public vs private 繼承 =====
// Engine started
// Engine started
// SportsCar launched
// ===== 範例 3：Leetcode 706 + protected 擴充 =====
// get(1) = 999 (預期 999)
// get(3) = -1 (預期 -1)
// 目前筆數 = 2 (預期 2)
// ===== 範例 4：BaseLogger → TimestampedLogger =====
// [auth] [INFO @ t=123] 使用者登入成功
