/*=============================================================================
 * 檔名：16_VirtualFunction.cpp
 * 主題：虛擬函式 (virtual function) 與 多型 (Polymorphism)
 * 適合：學完繼承，準備學 OOP 三大支柱最後一個的人
 *
 * 【課題介紹】
 *   先看一個很自然的需求：我有一隻 Dog 跟一隻 Cat，都繼承自 Animal。
 *   我想用一個函式 makeNoise(Animal& a) 同時處理兩種動物，
 *   呼叫 a.speak() 時希望 Dog 叫「汪」、Cat 叫「喵」 — 也就是
 *   「實際呼叫到子類別自己的版本」。
 *
 *   問題是：如果 speak() 是普通成員函式，編譯器在 makeNoise 看到 a 的型別是
 *   Animal&，就只會呼叫 Animal::speak() (它看到的「靜態型別」就是 Animal)。
 *   這叫「靜態 (static) dispatch」。
 *
 *   解法：把 speak() 宣告為 virtual，告訴編譯器：
 *     「呼叫這個函式時，請依據『物件實際的型別』 (動態型別) 決定走哪個版本。」
 *   這就是「動態 (dynamic) dispatch」 / 「執行期多型 (runtime polymorphism)」。
 *
 *       「多型 (Polymorphism) = 同一個介面，不同的實作，
 *        執行時自動挑對的版本。」
 *
 * 【關鍵字小結】
 *   - virtual    放在父類別函式宣告前，啟用動態 dispatch。
 *   - override   (C++11) 放在子類別覆寫函式後，告訴編譯器「我是要覆寫」，
 *                簽名打錯時會編譯錯誤，避免不小心做出新函式而非覆寫。
 *   - final      (C++11) 放在函式或 class 後面，禁止後續再被覆寫 / 繼承。
 *
 * 【底層大致原理 (vtable)】
 *   含 virtual 的類別，每個物件多藏一個指標 (vptr)，指向該類別的虛擬函式表 (vtable)。
 *   呼叫 virtual 函式時，先從 vptr 找 vtable，再走表格找到實際版本。
 *   你不必背細節，只需要知道：「virtual 有一點點額外成本，但提供了多型」。
 *
 * 【虛擬函式 + 解構子】
 *   重要規則：「打算被繼承的父類別，解構子幾乎一定要 virtual」。
 *   不然透過父類別指標 delete 子類別物件，會只跑父類別解構子，子類別資源洩漏。
 *   這個規則第 18 篇單獨開一篇詳述。
 *
 * 【對應 Leetcode】232. Implement Queue using Stacks
 *   題目簡述：用兩個堆疊實作一個 FIFO 佇列 (Queue)。
 *   為什麼放在這裡：
 *     雖然這題本身不需要 virtual，但我們可以設計一個抽象的 IQueue「介面」
 *     (純虛擬函式，下一篇 17 會深入)，再用兩種不同實作來示範多型 — 同樣呼叫
 *     push/pop，但底層可以是「兩個堆疊」或「直接用 deque」之類的不同做法。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/virtual
 *   https://en.cppreference.com/w/cpp/language/override
 *=============================================================================*/

/*
補充筆記：VirtualFunction
  - virtual function 透過 base pointer/reference 呼叫時會依真實物件型別動態派發。
  - override 讓編譯器檢查你真的覆寫了 base class 的虛擬函式。
  - 建構子與解構子期間的 virtual call 不會派發到尚未建構或已解構的 derived 部分。
  - virtual function 解決的是「靜態型別是 Base，但真實物件可能是 Derived」的呼叫問題；沒有 virtual 時，編譯器依變數型別決定呼叫版本。
  - 動態派發通常透過 vptr/vtable 實作；每個 polymorphic object 多一點記憶體與間接呼叫成本，但換來執行期可替換的行為。
  - override 不是必要語法，但應視為必寫；它能抓出 const 少寫、參數型別不同、函式名稱拼錯造成的「其實沒有覆寫」。
  - 只要類別有 virtual 函式，通常就應讓 destructor 也是 virtual；否則 base pointer delete derived object 時 derived destructor 可能不會跑。
  - 建構子和解構子中呼叫 virtual 函式不會派發到尚未建構或已解構的 derived 部分；此時物件的動態型別概念會被限制在當前層級。
  - virtual 只對 pointer/reference 的多型呼叫有意義；若把 Derived 以值存成 Base，物件切片後 derived 部分已經不在。
  - final 可用在 class 或 virtual function，明確禁止後續繼承或覆寫；這能表達設計邊界，也可能幫助最佳化。
  - 抽象介面應保持小而穩定；每增加一個 virtual function，所有 derived 實作都可能需要跟著變，這是 runtime polymorphism 的維護成本。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】virtual function 與執行期多型
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vtable 與 vptr 是什麼？各自屬於 class 還是 object？
//     答：只要 class 有 virtual 函式，編譯器就為「該 class」產生一張 vtable
//         （函式指標表，同一 class 的所有物件共用）；並在「每個 object」裡塞一個
//         隱藏的 vptr 指向它。呼叫時：由物件取 vptr → 查 vtable slot → 間接呼叫。
//         也就是 vtable 屬 class、vptr 屬 object。
//     追問：物件會因此變大嗎？（只要有 virtual 函式，物件就多一個 vptr，64-bit 上
//           sizeof 通常變 8；再加更多 virtual 函式也不會再變大。但這是常見實作
//           （Itanium ABI）的結果，vptr 的位置與大小標準並未規定）
//
// 🔥 Q2. 執行期多型的兩個必要條件是什麼？
//     答：① base 的 virtual 函式被 derived override；② 透過 base 的 pointer 或
//         reference 呼叫。兩者缺一不可 —— 本檔 makeNoise(const Animal&) 正是條件②。
//         若用物件本身（by value）呼叫，是靜態綁定，不構成多型。
//     追問：把 Derived 以值存進 Base 變數會怎樣？（object slicing —— derived 部分
//           被切掉，vptr 也變回 Base 的，之後不再多型）
//
// 🔥 Q3. `override` 與 `final` 有什麼用？
//     答：`override` 讓編譯器檢查「你確實覆寫了 base 的某個 virtual 函式」，簽名打錯
//         （漏 const、參數型別不同）會編譯錯誤 —— 這是最常見的靜默 bug 來源。
//         `final` 用在函式上禁止再被覆寫、用在 class 上禁止再被繼承，也給編譯器
//         devirtualization 的優化機會。兩者都是 C++11 的 contextual keyword。
//     追問：`override` 會改變執行期行為嗎？（不會，純編譯期檢查）
//
// Q4. virtual function 可以是 private 嗎？
//     答：可以。存取控制（編譯期）與 virtual 分派（執行期）是正交的兩件事。經由
//         base 的 public 非虛介面呼叫時，仍會正確分派到 derived 的 private override。
//         這正是 NVI（Non-Virtual Interface）模式：public non-virtual 介面
//         ＋ private virtual 實作點，讓 base 能在前後插入共用邏輯。
//     追問：derived 覆寫時可以改變存取等級嗎？（可以；存取檢查依呼叫點的靜態型別）
//
// ⚠️ 陷阱. virtual function 的 default argument 是動態綁定還是靜態綁定？
//     答：函式「本體」是動態綁定，但 default argument 是「靜態綁定」，依指標/參考的
//         靜態型別決定。所以 `Base* p = new Derived; p->f();` 會執行 Derived 的函式
//         本體，卻套用 Base 宣告的預設參數值。結論：不要在 virtual 函式上用預設參數。
//     為什麼會錯：多數人腦中「virtual 一切都跟著實際型別走」；但預設參數是編譯期
//         就填進呼叫端的，編譯器只看得到靜態型別，根本不經過 vtable。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：Animal speak() 從靜態到動態 dispatch
// -----------------------------------------------------------------------------
class Animal {
public:
    // 重點：speak 標 virtual → 任何子類別覆寫的版本都會在執行期被正確呼叫
    virtual void speak() const {
        std::cout << "(動物的某種聲音)" << std::endl;
    }

    // 父類別解構子 → 設成 virtual (預告第 18 篇)
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    // override：告訴編譯器「我要覆寫父類別的虛擬函式」
    // 如果簽名不一致 (例如多打字、漏 const)，編譯器會報錯 → 防呆
    void speak() const override {
        std::cout << "汪汪!" << std::endl;
    }
};

class Cat : public Animal {
public:
    void speak() const override {
        std::cout << "喵~" << std::endl;
    }
};

// 同一個函式同時處理 Dog / Cat，靠的是多型
void makeNoise(const Animal& a) {
    a.speak();        // 這行因為 speak() 是 virtual，會走子類別的版本
}

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 232 - Implement Queue using Stacks
// -----------------------------------------------------------------------------
// 順便用「介面 + 兩個實作」展示多型：先定義一個 IIntQueue 抽象介面 (用純虛擬)。
// 純虛擬 = void f() = 0; (細節下一篇 17 講，這裡先用)

class IIntQueue {
public:
    virtual void push(int x) = 0;
    virtual int  pop()       = 0;
    virtual int  peek()      = 0;
    virtual bool empty() const = 0;
    virtual ~IIntQueue() = default;
};

// 實作 1：經典「兩個 stack 模擬一個 queue」(就是 Leetcode 232)
class TwoStackQueue : public IIntQueue {
private:
    std::stack<int> in_;     // 進站
    std::stack<int> out_;    // 出站

    // 把 in_ 倒進 out_，讓最早進來的元素到 out_ 頂端
    void shiftIfNeeded() {
        if (out_.empty()) {
            while (!in_.empty()) {
                out_.push(in_.top());
                in_.pop();
            }
        }
    }

public:
    void push(int x) override { in_.push(x); }

    int pop() override {
        shiftIfNeeded();
        int v = out_.top();
        out_.pop();
        return v;
    }

    int peek() override {
        shiftIfNeeded();
        return out_.top();
    }

    bool empty() const override {
        return in_.empty() && out_.empty();
    }
};

// 實作 2：簡單實作，內部直接用 std::deque (示意：相同介面，不同實作)
#include <deque>
class DequeQueue : public IIntQueue {
private:
    std::deque<int> dq_;
public:
    void push(int x)   override { dq_.push_back(x); }
    int  pop()         override { int v = dq_.front(); dq_.pop_front(); return v; }
    int  peek()        override { return dq_.front(); }
    bool empty() const override { return dq_.empty(); }
};

// 用「父類別參考」操作不同實作：使用者根本不需要知道底層是哪一種
void demoQueue(IIntQueue& q, const std::string& tag) {
    std::cout << "[" << tag << "] ";
    q.push(1); q.push(2); q.push(3);
    std::cout << q.peek() << " ";    // 1
    std::cout << q.pop()  << " ";    // 1
    std::cout << q.pop()  << " ";    // 2
    std::cout << q.empty()<< std::endl;   // 0 (還有一個 3)
}

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 225 - Implement Stack using Queues
// -----------------------------------------------------------------------------
// 題目簡述：用 queue 實作 stack (LIFO)。
// 與範例 2 對偶，並再次示範多型：和 TwoStackQueue 共用 IIntStack 介面。
class IIntStack {
public:
    virtual void push(int x)  = 0;
    virtual int  pop()        = 0;
    virtual int  top()        = 0;
    virtual bool empty() const = 0;
    virtual ~IIntStack() = default;
};

// 用一個 deque 模擬：push 時直接放後面，但要把整串旋轉，
// 讓「最新元素」永遠在 front。這樣 top/pop 都 O(1)。
class QueueBackedStack : public IIntStack {
private:
    std::deque<int> q_;
public:
    void push(int x) override {
        q_.push_back(x);
        // 旋轉：把 x 之前的 (size-1) 個元素搬到後面 → 最新元素變第一個
        for (size_t i = 0; i + 1 < q_.size(); ++i) {
            q_.push_back(q_.front());
            q_.pop_front();
        }
    }
    int  pop()  override { int v = q_.front(); q_.pop_front(); return v; }
    int  top()  override { return q_.front(); }
    bool empty() const override { return q_.empty(); }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - Shape 多型
// -----------------------------------------------------------------------------
// 工作上很常見：以「形狀」為例，用同一個介面處理不同子類別。
class Shape {
public:
    virtual double area() const = 0;     // 純虛擬，下一篇詳述
    virtual std::string name() const { return "Shape"; }
    virtual ~Shape() = default;
};

class Rectangle : public Shape {
    double w_, h_;
public:
    Rectangle(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    std::string name() const override { return "Rectangle"; }
};

class Circle : public Shape {
    double r_;
public:
    explicit Circle(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
    std::string name() const override { return "Circle"; }
};

int main() {
    std::cout << "===== 範例 1：Animal speak() 多型 =====" << std::endl;
    Dog d;
    Cat c;
    makeNoise(d);    // 汪汪!
    makeNoise(c);    // 喵~

    Animal a;
    makeNoise(a);    // (動物的某種聲音) — 父類別本身的版本

    std::cout << "===== 範例 2：Leetcode 232 兩種實作 =====" << std::endl;
    TwoStackQueue q1;
    DequeQueue    q2;
    demoQueue(q1, "TwoStack");
    demoQueue(q2, "Deque   ");

    std::cout << "===== 範例 3：Leetcode 225 用 queue 做 stack =====" << std::endl;
    QueueBackedStack s;
    s.push(1); s.push(2); s.push(3);
    std::cout << "top = " << s.top() << " (預期 3)" << std::endl;
    std::cout << "pop = " << s.pop() << " (預期 3)" << std::endl;
    std::cout << "pop = " << s.pop() << " (預期 2)" << std::endl;
    std::cout << "empty? " << s.empty() << " (預期 0)" << std::endl;

    std::cout << "===== 範例 4：Shape 多型 =====" << std::endl;
    Shape* shapes[] = { new Rectangle(3, 4), new Circle(5) };
    for (Shape* sh : shapes) {
        // 父類別指標 → 自動派發到子類別的版本
        std::cout << sh->name() << " area = " << sh->area() << std::endl;
        delete sh;     // 父類別解構子是 virtual，能正確解構子類別
    }
    return 0;
}

/* 預期輸出：
 * ===== 範例 1：Animal speak() 多型 =====
 * 汪汪!
 * 喵~
 * (動物的某種聲音)
 * ===== 範例 2：Leetcode 232 兩種實作 =====
 * [TwoStack] 1 1 2 0
 * [Deque   ] 1 1 2 0
 * ===== 範例 3：Leetcode 225 用 queue 做 stack =====
 * top = 3 (預期 3)
 * pop = 3 (預期 3)
 * pop = 2 (預期 2)
 * empty? 0 (預期 0)
 * ===== 範例 4：Shape 多型 =====
 * Rectangle area = 12
 * Circle area = 78.5397
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. virtual 函式啟用「動態 dispatch」：依「物件實際型別」決定呼叫哪個版本。
 *   2. 子類別覆寫時加 override 是好習慣，可避免簽名錯誤導致沒覆寫成功。
 *   3. 多型讓「相同介面，不同實作」成為可能，也是後續設計模式的基礎。
 *   4. 父類別若要被繼承使用，幾乎都該把解構子設為 virtual (見第 18 篇)。
 *   5. 純虛擬函式 (= 0) 讓我們定義「介面類別」，下一篇 17 會深入。
 *
 * 【下一篇預告】
 *   17_AbstractClass.cpp
 *   純虛擬函式與抽象類別 — 為什麼有時候我們會寫「沒有實作的函式」？
 *=============================================================================*/
