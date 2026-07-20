/*=============================================================================
 * 檔名：14_Inheritance.cpp
 * 主題：繼承 (Inheritance) - OOP 三大支柱第二個
 * 適合：學完封裝，想知道「兩個類別有共通部分時怎麼避免重複」的人
 *
 * 【課題介紹】
 *   想像我們要設計動物相關類別：
 *     - Dog (狗)：有名字、年齡、會吃飯、會睡覺、會吠
 *     - Cat (貓)：有名字、年齡、會吃飯、會睡覺、會喵喵
 *
 *   你會發現「名字、年齡、吃飯、睡覺」是共通的，只有「叫聲」不同。
 *   如果直接抄兩份程式碼，未來改個 eat() 就要改兩處 → 維護地獄。
 *   解法：繼承 (Inheritance)。
 *
 *       「子類別 (Derived class) 可以繼承父類別 (Base class) 的所有成員，
 *        然後加上自己的特殊行為。」
 *
 *   寫法：
 *       class Dog : public Animal { ... };
 *
 *   讀作：「Dog is-a Animal」 — 是一種 has-a (組合) vs is-a (繼承) 的判斷標準：
 *     - is-a：Dog 是一種 Animal → 用繼承
 *     - has-a：Car 有一個 Engine → 用組合 (把 Engine 當成員)
 *   兩個都能複用程式碼，但語意不同；不確定時優先用組合。
 *
 * 【繼承後子類別擁有什麼？】
 *   - 父類別的「public」成員 → 在子類別中還是 public
 *   - 父類別的「protected」成員 → 子類別內部能用 (對外仍隱藏)
 *   - 父類別的「private」成員 → 子類別內部「也碰不到」 (但物件中還是有那些資料)
 *
 *   可以把 protected 想成「家族內部使用」：父子類別成員看得到，外人看不到。
 *
 * 【建構子怎麼處理？】
 *   - 子類別建構子執行前，「父類別建構子先被自動呼叫」(預設建構子)。
 *   - 想呼叫帶參數的父類別建構子，要在初始化列表這樣寫：
 *
 *         Dog(const std::string& n, int a)
 *             : Animal(n, a)        // 顯式呼叫父類別建構子
 *         { ... }
 *
 * 【三種繼承存取方式 (public / protected / private inheritance)】
 *   - public 繼承：is-a 關係，最常見 (今天用這個就好)
 *   - protected / private 繼承：較進階，第 15 篇詳細介紹
 *
 * 【對應 Leetcode】155. Min Stack
 *   題目簡述：
 *     設計一個 Stack，要支援 push / pop / top，並再多一個 getMin()
 *     回傳目前堆疊內最小值，所有操作都要 O(1)。
 *   為什麼選這題：
 *     做法是「在原本的 stack 上多加一個跟著走的最小值堆疊」，
 *     非常適合用「繼承普通 IntStack 來擴充功能」的方式來示範。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/derived_class
 *=============================================================================*/

/*
補充筆記：Inheritance
  - Inheritance 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - public inheritance 表達 is-a 關係：Derived 可以被當成 Base 使用；若只是重用程式碼，composition 通常比 inheritance 更穩。
  - 建構 derived 物件時會先建構 base，再建構 derived 成員，最後進入 derived 建構子本體；解構順序相反。
  - base class 的 private 成員存在於 derived 物件內，但 derived 類別不能直接存取；要透過 base 提供的 protected/public 介面。
  - 函式覆寫需要 base 函式是 virtual，且 derived 函式簽名一致；加 override 可以讓編譯器幫你抓錯。
  - 物件切片會在把 derived 物件以值複製成 base 物件時發生，derived 部分被丟掉；多型應使用 base reference 或 pointer。
  - protected 讓 derived 可存取，但也讓 base 的內部設計暴露給所有子類別；過度使用 protected 會讓繼承層級難以重構。
  - 繼承層級越深，理解成本越高；實務上應保持 base class 介面小而穩定，避免把所有可重用函式都塞進父類別。
  - 若 class 打算作為 polymorphic base，通常需要 virtual destructor，否則透過 base pointer delete derived object 會出問題。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】繼承（Inheritance）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 object slicing（物件切片）？如何避免？
//     答：把 derived 物件「以值」指派或傳遞給 base 型別時，只有 base 的部分被複製，
//     derived 的成員與多型行為被切掉，得到的是一個純粹的 base 物件。
//     避免方式：base 一律用 pointer 或 reference 傳遞；把 base 的 copy constructor /
//     copy assignment 設為 protected 或 deleted；或讓 base 成為 abstract class。
//     追問：`std::vector<Base>` 塞 Derived 會怎樣？（每個元素都被 slice；
//     正解是 `std::vector<std::unique_ptr<Base>>`）
//
// 🔥 Q2. 什麼是 diamond problem？virtual inheritance 如何解決？
//     答：B、C 各自繼承 A，D 又同時繼承 B 與 C → D 內含「兩份」A subobject，
//     存取 A 的成員會 ambiguity，也浪費空間。解法是 `class B : virtual public A`、
//     `class C : virtual public A`，讓 A 成為 virtual base，D 中只有一份共享的 A。
//     追問：virtual base 由誰負責初始化？（由最衍生的類別 D 直接呼叫 A 的建構子，
//     B/C 初始化列表中對 A 的初始化會被忽略）
//     成本呢？（存取 virtual base 的成員需要多一層間接定址、物件也會變大；
//     實際的佈局方式屬 ABI/實作定義，標準未規定）
//
// 🔥 Q3. 執行期多型的兩個必要條件是什麼？
//     答：① base 的函式宣告為 virtual 且被 derived override；
//     ② 透過 base 的 pointer 或 reference 呼叫。
//     用物件本身（by value）呼叫是靜態綁定，不構成多型 — 這也正是 slicing 之所以
//     致命的原因：切完之後連 vptr 都是 Base 的了。
//
// Q4. 建構 derived 物件時的順序？
//     答：virtual base → 一般 base（依宣告順序）→ derived 的非靜態成員（依宣告順序）
//     → derived 建構子本體。解構完全相反。
//     這也解釋了為什麼在 base 的建構子裡呼叫 virtual function，看不到 derived 的 override。
//
// ⚠️ 陷阱. derived 定義了同名函式，base 的其他重載還叫得到嗎？
//     答：叫不到 — 這是 name hiding。名稱查找一旦在 derived scope 找到就停止，
//     base 中所有同名重載（即使參數型別完全不同）全部被隱藏、不參與 overload
//     resolution。解法：在 derived 中寫 `using Base::f;` 把 base 的重載拉回來。
//     為什麼會錯：以為 base 與 derived 的同名函式會合併成同一個 overload set；
//     也常與 override 混淆 — override 要求簽名完全相同且 base 為 virtual，
//     name hiding 則發生在名稱查找階段，跟 virtual 一點關係都沒有。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>     // std::runtime_error
#include <algorithm>     // std::min

// -----------------------------------------------------------------------------
// 範例 1：Animal → Dog / Cat (最經典的入門示範)
// -----------------------------------------------------------------------------
class Animal {
protected:                       // protected：自己 + 子類別能用，外人不能
    std::string name_;
    int         age_;

public:
    Animal(const std::string& name, int age) : name_(name), age_(age) {
        std::cout << "[Animal] " << name_ << " 出生" << std::endl;
    }
    ~Animal() {
        std::cout << "[Animal] " << name_ << " 死亡" << std::endl;
    }
    void eat()  const { std::cout << name_ << " 在吃飯" << std::endl; }
    void sleep()const { std::cout << name_ << " 在睡覺" << std::endl; }
};

// Dog 用 public 繼承 Animal
class Dog : public Animal {
public:
    // 透過初始化列表呼叫父類別建構子，把 name/age 傳進去
    Dog(const std::string& name, int age) : Animal(name, age) {
        std::cout << "[Dog] " << name_ << " 加入" << std::endl;
    }
    ~Dog() {
        // 注意：子類別解構子先執行，然後才執行父類別解構子 (與建構順序相反)
        std::cout << "[Dog] " << name_ << " 離開" << std::endl;
    }
    void bark() const {
        std::cout << name_ << " 在汪汪叫" << std::endl;
    }
};

class Cat : public Animal {
public:
    Cat(const std::string& name, int age) : Animal(name, age) {}
    void meow() const { std::cout << name_ << " 在喵喵叫" << std::endl; }
};

// -----------------------------------------------------------------------------
// 範例 2：Leetcode 155 Min Stack — 用繼承擴充一個基本的 IntStack
// -----------------------------------------------------------------------------
class IntStack {
protected:
    std::vector<int> data_;     // 用 vector 當底層儲存

public:
    void push(int x)   { data_.push_back(x); }
    void pop() {
        if (data_.empty()) throw std::runtime_error("空堆疊不能 pop");
        data_.pop_back();
    }
    int top() const {
        if (data_.empty()) throw std::runtime_error("空堆疊沒有 top");
        return data_.back();
    }
    bool empty() const { return data_.empty(); }
    size_t size() const { return data_.size(); }
};

// 繼承 IntStack，多一個「跟著走」的最小值堆疊
class MinStack : public IntStack {
private:
    std::vector<int> mins_;     // 每次 push 都記錄當下最小值

public:
    // 重點概念：我們要重寫 push / pop 來同步維護 mins_，但 top()/empty() 直接用父類別
    // C++ 中要「覆寫」非 virtual 函式叫做「shadow」，原本的 push 會被遮蔽。
    void push(int x) {
        IntStack::push(x);                // 呼叫父類別的 push
        // 新最小值 = min(舊最小值, x)
        int newMin = mins_.empty() ? x : std::min(mins_.back(), x);
        mins_.push_back(newMin);
    }

    void pop() {
        IntStack::pop();
        mins_.pop_back();
    }

    int getMin() const {
        if (mins_.empty()) throw std::runtime_error("空堆疊沒有最小值");
        return mins_.back();
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 1670 - Design Front Middle Back Queue  (難度: medium)
// -----------------------------------------------------------------------------
// 題目簡述：設計一個可以從前、中、後三個位置 push/pop 的 queue。
// 為什麼選這題：把基本 vector 操作放在 BaseQueue (父類別)，
//               擴充的 middle 操作放在 derived 類別，是繼承擴充的典型範例。
class BaseQueue {
protected:
    std::vector<int> data_;
public:
    void pushFront(int x) { data_.insert(data_.begin(), x); }
    void pushBack (int x) { data_.push_back(x); }
    int popFront() {
        if (data_.empty()) return -1;
        int v = data_.front();
        data_.erase(data_.begin());
        return v;
    }
    int popBack() {
        if (data_.empty()) return -1;
        int v = data_.back();
        data_.pop_back();
        return v;
    }
    size_t size() const { return data_.size(); }
};

// 子類別 FrontMiddleBackQueue：在 BaseQueue 上新增「中間插入/取出」的能力
class FrontMiddleBackQueue : public BaseQueue {
public:
    void pushMiddle(int x) {
        // 中間插入：題目規定若大小為偶數，插在「右邊中間」
        size_t mid = data_.size() / 2;
        data_.insert(data_.begin() + mid, x);
    }

    int popMiddle() {
        if (data_.empty()) return -1;
        // 取出中間：偶數時取「左邊中間」
        size_t mid = (data_.size() - 1) / 2;
        int v = data_[mid];
        data_.erase(data_.begin() + mid);
        return v;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - User → AdminUser
// -----------------------------------------------------------------------------
// 工作上常見：先有一個 User 基底，再有具備額外權限的 AdminUser。
class User {
protected:
    std::string username_;
public:
    explicit User(const std::string& u) : username_(u) {}
    void login()  const { std::cout << username_ << " 已登入" << std::endl; }
    void logout() const { std::cout << username_ << " 已登出" << std::endl; }
};

class AdminUser : public User {
public:
    explicit AdminUser(const std::string& u) : User(u) {}
    // 多一個「管理員專屬」的能力
    void banUser(const std::string& target) const {
        std::cout << "[Admin " << username_ << "] 已封鎖 " << target << std::endl;
    }
};

int main() {
    std::cout << "===== 範例 1：Animal / Dog / Cat =====" << std::endl;
    {
        Dog d("旺財", 3);
        d.eat();          // 繼承自 Animal
        d.sleep();        // 繼承自 Animal
        d.bark();         // Dog 自己的方法

        Cat c("咪咪", 2);
        c.meow();
    }   // 離開區塊：解構順序 → Cat 子→父，Dog 子→父

    std::cout << "===== 範例 2：Leetcode 155 Min Stack =====" << std::endl;
    MinStack ms;
    ms.push(-2);
    ms.push(0);
    ms.push(-3);
    std::cout << "getMin = " << ms.getMin() << std::endl;   // -3
    ms.pop();
    std::cout << "top    = " << ms.top()    << std::endl;   // 0
    std::cout << "getMin = " << ms.getMin() << std::endl;   // -2

    std::cout << "===== 範例 3：Leetcode 1670 FrontMiddleBackQueue =====" << std::endl;
    FrontMiddleBackQueue q;
    q.pushFront(1);                      // [1]
    q.pushBack(2);                       // [1, 2]
    q.pushMiddle(3);                     // [1, 3, 2]
    q.pushMiddle(4);                     // [1, 4, 3, 2]
    std::cout << "popFront  = " << q.popFront()  << std::endl;  // 1
    std::cout << "popMiddle = " << q.popMiddle() << std::endl;  // 3
    std::cout << "popMiddle = " << q.popMiddle() << std::endl;  // 4
    std::cout << "popBack   = " << q.popBack()   << std::endl;  // 2
    std::cout << "popFront  = " << q.popFront()  << std::endl;  // -1 (空)

    std::cout << "===== 範例 4：User → AdminUser =====" << std::endl;
    AdminUser admin("root");
    admin.login();                       // 繼承自 User
    admin.banUser("evil_user");          // 子類別專屬
    admin.logout();                      // 繼承自 User

    return 0;
}

/* 預期輸出：
 * ===== 範例 1：Animal / Dog / Cat =====
 * [Animal] 旺財 出生
 * [Dog] 旺財 加入
 * 旺財 在吃飯
 * 旺財 在睡覺
 * 旺財 在汪汪叫
 * [Animal] 咪咪 出生
 * 咪咪 在喵喵叫
 * [Animal] 咪咪 死亡
 * [Dog] 旺財 離開
 * [Animal] 旺財 死亡
 * ===== 範例 2：Leetcode 155 Min Stack =====
 * getMin = -3
 * top    = 0
 * getMin = -2
 * ===== 範例 3：Leetcode 1670 FrontMiddleBackQueue =====
 * popFront  = 1
 * popMiddle = 3
 * popMiddle = 4
 * popBack   = 2
 * popFront  = -1
 * ===== 範例 4：User → AdminUser =====
 * root 已登入
 * [Admin root] 已封鎖 evil_user
 * root 已登出
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 繼承讓子類別自動擁有父類別的成員 (private 也在物件中，但子類碰不到)。
 *   2. 寫法：class Derived : public Base { ... }
 *   3. 子類別建構子要在初始化列表呼叫父類別建構子 (Base(args))。
 *   4. 建構順序：Base → Derived；解構順序：Derived → Base (相反)。
 *   5. protected 成員專為「家族內部 (父子類別)」共用而存在，外人不能碰。
 *   6. is-a 用繼承，has-a 用組合；不確定時優先選組合。
 *
 * 【下一篇預告】
 *   15_AccessAndInheritance.cpp
 *   public / protected / private 三種繼承的差別，
 *   並用「員工 / 經理」場景示範什麼時候會踩到坑。
 *=============================================================================*/

// 編譯: g++ -std=c++20 -Wall -Wextra 14_Inheritance.cpp -o 14_Inheritance

// === 預期輸出 ===
// ===== 範例 1：Animal / Dog / Cat =====
// [Animal] 旺財 出生
// [Dog] 旺財 加入
// 旺財 在吃飯
// 旺財 在睡覺
// 旺財 在汪汪叫
// [Animal] 咪咪 出生
// 咪咪 在喵喵叫
// [Animal] 咪咪 死亡
// [Dog] 旺財 離開
// [Animal] 旺財 死亡
// ===== 範例 2：Leetcode 155 Min Stack =====
// getMin = -3
// top    = 0
// getMin = -2
// ===== 範例 3：Leetcode 1670 FrontMiddleBackQueue =====
// popFront  = 1
// popMiddle = 3
// popMiddle = 4
// popBack   = 2
// popFront  = -1
// ===== 範例 4：User → AdminUser =====
// root 已登入
// [Admin root] 已封鎖 evil_user
// root 已登出
