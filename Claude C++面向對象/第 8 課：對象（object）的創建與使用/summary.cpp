/*
 * ================================================================
 * 【第 8 課：對象（object）的創建與使用】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 對象是類別（class）的實例（instance），在記憶體中有實體
 * 2. 棧上創建（Stack Allocation）：最常用，自動管理生命週期
 * 3. 堆上創建（Heap Allocation）：用 new/delete，手動管理
 * 4. 棧對象用 . 存取成員，堆指標用 -> 存取成員
 * 5. 同一類別的不同對象各自擁有獨立的成員變數副本
 * 6. sizeof：只計算成員變數大小（含記憶體對齊填充），不計函數
 * 7. 空類別大小為 1 byte（C++ 保證每個對象有唯一地址）
 * 8. 對象可透過引用互相交互
 * 9. 對象陣列：同一類別的多個對象組成陣列
 * 10. 對象作為函數參數：傳值、傳引用、傳 const 引用
 * ================================================================
 */

#include <iostream>
#include <string>
using namespace std;


// ===== 重點一：棧上創建（Stack Allocation）=====
// 說明：最常用、最安全的建立對象方式。
// 對象儲存在棧上，離開作用域時自動銷毀，不需要手動管理記憶體。
// 語法：類別名 變數名;  然後用 . 存取成員。

class Dog {
public:
    string name;
    int age = 0;

    void bark() {
        cout << name << " 汪汪！" << endl;
    }
};


// ===== 重點二：堆上創建（Heap Allocation）=====
// 說明：使用 new 在堆（heap）上配置記憶體，回傳指標。
// 優點：生命週期不受作用域限制，可跨函數使用。
// 缺點：必須手動用 delete 釋放，否則造成記憶體洩漏（Memory Leak）。
// 釋放後應將指標設為 nullptr，避免野指標（Dangling Pointer）。
// 語法：類別名* 指標名 = new 類別名(); 用 -> 存取成員。


// ===== 重點三：棧 vs 堆 比較 =====
// 說明：兩種創建方式的存取語法不同。
// 棧對象使用 .（點運算子）存取成員
// 堆指標使用 ->（箭頭運算子）存取成員
// (*指標).成員 等價於 指標->成員，但 -> 更常用也更清晰

class Cat {
public:
    string name;

    void meow() {
        cout << name << " 喵～" << endl;
    }
};


// ===== 重點四：每個對象是獨立的 =====
// 說明：同一類別建立的不同對象，各自擁有獨立的成員變數副本。
// 修改 a.count 完全不影響 b.count，它們在記憶體中是不同的位址。
// 這是面向對象設計的核心特性之一：「封裝」與「獨立性」。

class Counter {
public:
    int count = 0;

    void increment() { count++; }
    void show() { cout << "count = " << count << endl; }
};


// ===== 重點五：對象的大小 sizeof =====
// 說明：對象在記憶體中佔用的大小 = 所有成員變數大小的總和（含對齊填充）。
// 成員函數不佔對象空間（函數共享一份，透過 this 指標區分）。
// 空類別大小為 1 byte：C++ 規定每個對象必須有唯一地址，所以不能是 0。
// 記憶體對齊（Memory Alignment）：和 C 語言 struct 規則完全一樣。

class Empty {};                     // 大小 = 1 byte（特殊規則）

class OnlyFunction {
public:
    void doSomething() { cout << "doing" << endl; }
    // 函數不佔空間，所以大小仍是 1 byte
};

class WithData {
public:
    int x;      // 4 bytes
    int y;      // 4 bytes
    double z;   // 8 bytes
    // 總計 16 bytes，剛好對齊，無填充
    void show() { cout << x << ", " << y << ", " << z << endl; }
};

class Mixed {
public:
    char a;     // 1 byte
    int b;      // 4 bytes（前面需要 3 bytes 填充以對齊）
    char c;     // 1 byte（後面需要 3 bytes 填充以整體對齊）
    // 典型輸出：1 + 3(pad) + 4 + 1 + 3(pad) = 12 bytes
};


// ===== 重點六：多個對象的交互 =====
// 說明：對象之間可以透過引用互相傳遞、操作。
// 函數參數寫 Player& target 表示「引用」，直接操作原對象，不是複製品。
// 這樣 target.hp -= attack 才會真正修改傳入對象的 HP。

class Player {
public:
    string name;
    int hp = 100;
    int attack = 10;

    // 以引用方式接收 target，直接操作原對象
    void attackTarget(Player& target) {
        cout << name << " 攻擊了 " << target.name << "！" << endl;
        target.hp -= attack;
        cout << target.name << " 剩餘 HP: " << target.hp << endl;
    }

    void showStatus() {
        cout << "[" << name << "] HP:" << hp << " 攻擊:" << attack << endl;
    }

    bool isAlive() { return hp > 0; }
};


// ===== 重點七：對象陣列 =====
// 說明：可以建立同一類別的多個對象組成的陣列。
// 用索引 students[i] 存取，再用 . 存取成員。
// 陣列中的每個對象都是獨立的，可以用迴圈遍歷。

class Student {
public:
    string name = "未命名";
    int score = 0;

    void show() { cout << name << ": " << score << " 分" << endl; }
};


// ===== 重點八：對象作為函數參數（三種方式）=====
// 說明：傳值（by value）、傳引用（by reference）、傳 const 引用。
// 傳值：複製一份，函數內修改不影響原對象。有複製成本，少用。
// 傳引用：直接操作原對象，函數內的修改會反映到外部。
// 傳 const 引用：最推薦！不複製（高效），也不能修改（安全）。

class Box {
public:
    double width = 0;
    double height = 0;

    double area() { return width * height; }
};

// 傳值：函數內拿到的是複製品，修改不影響原對象
void printByValue(Box b) {
    cout << "面積 = " << b.area() << endl;
    b.width = 999;  // 只改了複製品，原對象不受影響
}

// 傳引用：函數內直接操作原對象
void doubleSize(Box& b) {
    b.width *= 2;
    b.height *= 2;
}

// 傳 const 引用：不複製（效率高），且無法修改（安全）—— 最推薦
void printByConstRef(const Box& b) {
    cout << "寬: " << b.width << ", 高: " << b.height << endl;
    // b.width = 10;  // 編譯錯誤！const 不允許修改
}


// ===== main：展示所有重點 =====
int main() {
    cout << "===== 重點一：棧上創建 =====" << endl;
    // 棧上創建：語法最簡單，離開作用域自動銷毀
    Dog d1;
    d1.name = "旺財";
    d1.age = 3;
    d1.bark();   // 用 . 存取

    Dog d2;
    d2.name = "小黑";
    d2.age = 5;
    d2.bark();


    cout << "\n===== 重點二：堆上創建 =====" << endl;
    // new 在堆上配置記憶體，回傳指標
    Dog* ptr = new Dog();
    ptr->name = "阿花";   // 用 -> 存取
    ptr->age = 2;
    ptr->bark();
    delete ptr;           // 必須手動釋放！
    ptr = nullptr;        // 好習慣：防止野指標


    cout << "\n===== 重點三：棧 vs 堆 存取語法 =====" << endl;
    Cat c1;                // 棧：自動管理
    c1.name = "咪咪";
    c1.meow();             // . 存取

    Cat* c2 = new Cat();   // 堆：手動管理
    c2->name = "花花";
    c2->meow();            // -> 存取
    (*c2).meow();          // 等價寫法：先解引用再用 .
    delete c2;
    c2 = nullptr;


    cout << "\n===== 重點四：每個對象獨立 =====" << endl;
    Counter a, b;
    a.increment(); a.increment(); a.increment();  // a.count = 3
    b.increment();                                 // b.count = 1（與 a 無關）
    cout << "a: "; a.show();
    cout << "b: "; b.show();


    cout << "\n===== 重點五：sizeof 對象 =====" << endl;
    // 成員函數不佔對象空間，空類別最小為 1 byte
    cout << "sizeof(Empty)        = " << sizeof(Empty) << " bytes" << endl;
    cout << "sizeof(OnlyFunction) = " << sizeof(OnlyFunction) << " bytes" << endl;
    cout << "sizeof(WithData)     = " << sizeof(WithData) << " bytes" << endl;
    cout << "sizeof(Mixed)        = " << sizeof(Mixed) << " bytes (含對齊填充)" << endl;


    cout << "\n===== 重點六：對象交互（引用參數）=====" << endl;
    Player warrior, mage;
    warrior.name = "戰士"; warrior.hp = 120; warrior.attack = 15;
    mage.name = "法師";    mage.hp = 80;    mage.attack = 25;

    warrior.showStatus();
    mage.showStatus();
    warrior.attackTarget(mage);  // 引用：直接修改 mage 的 hp
    mage.attackTarget(warrior);
    cout << warrior.name << (warrior.isAlive() ? " 存活" : " 陣亡") << endl;
    cout << mage.name   << (mage.isAlive()   ? " 存活" : " 陣亡") << endl;


    cout << "\n===== 重點七：對象陣列 =====" << endl;
    Student students[3];
    students[0].name = "小明"; students[0].score = 85;
    students[1].name = "小華"; students[1].score = 92;
    students[2].name = "小美"; students[2].score = 78;

    // 用迴圈遍歷
    for (int i = 0; i < 3; i++) {
        students[i].show();
    }

    // 找最高分
    int maxIdx = 0;
    for (int i = 1; i < 3; i++) {
        if (students[i].score > students[maxIdx].score)
            maxIdx = i;
    }
    cout << "最高分: " << students[maxIdx].name
         << " (" << students[maxIdx].score << " 分)" << endl;


    cout << "\n===== 重點八：對象作為函數參數 =====" << endl;
    Box box;
    box.width = 5.0;
    box.height = 3.0;

    cout << "--- 傳值（複製品，原對象不變）---" << endl;
    printByValue(box);
    cout << "原始 width = " << box.width << "（未被改變）" << endl;

    cout << "--- 傳引用（直接操作原對象）---" << endl;
    doubleSize(box);
    cout << "放大後 width = " << box.width << endl;

    cout << "--- 傳 const 引用（最推薦，高效且安全）---" << endl;
    printByConstRef(box);


    cout << "\n===== 重點回顧：選擇參數傳遞方式 =====" << endl;
    cout << "傳值       void f(Box b)         : 少用，有複製成本" << endl;
    cout << "傳引用     void f(Box& b)         : 需要修改原對象時" << endl;
    cout << "傳const引用 void f(const Box& b)  : 最常用！只讀取不修改" << endl;

    return 0;
}
