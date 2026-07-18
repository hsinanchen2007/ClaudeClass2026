/*
 * ============================================================
 * 【第 7 課：類別（class）的定義與語法】總複習 summary.cpp
 * ============================================================
 * 本課程重點：
 * 1. 什麼是類別（class）：藍圖的概念
 * 2. class 的基本語法結構
 * 3. 存取修飾符（public / private / protected）
 * 4. 成員變數（Member Variables）的宣告與類內初始化
 * 5. 成員函數（Member Functions）的兩種定義方式
 *    - 類內定義（inline）
 *    - 類外定義（使用範圍解析運算子 ::）
 * 6. 成員函數直接存取成員變數（this 指標的預告）
 * 7. class 定義的常見錯誤
 * 8. class 命名慣例（PascalCase）
 * 9. class 與 C 語言 struct + 函數的對比
 * ============================================================
 */

#include <iostream>
#include <string>
using namespace std;


// ===== 重點一：class 是什麼？藍圖的概念 =====
//
// class（類別）是 C++ 面向對象程式設計（OOP）的核心語法工具。
// 你可以把 class 想成一張「藍圖（blueprint）」：
//   - 它「定義」了一個東西有什麼屬性（資料）和能做什麼（函數）
//   - 但它本身不是實際的東西——根據藍圖建造出來的才叫「對象（object）」
//
// 比喻：
//   class Car      = 汽車的設計圖紙
//   Car myCar      = 根據圖紙造出來的一輛實際的車
//
// OOP 的核心：把「資料」和「操作資料的函數」綁在一起，
// class 就是實現這個想法的語法工具。


// ===== 重點二：class 的基本語法 =====
//
// class 類別名稱 {
// 存取修飾符:
//     成員變數（屬性）;
//     成員函數（行為）;
// };  // ← 注意這個分號！非常容易忘記！
//
// 下面是最基礎的完整範例：

class Dog {
public:
    // 成員變數（Member Variables）—— 描述「狗有什麼」
    // 這些是對象的屬性，每個 Dog 對象都有自己獨立的一份
    string name;
    int age;
    string breed;  // 品種

    // 成員函數（Member Functions）—— 描述「狗能做什麼」
    // 成員函數直接定義在 class 裡面，稱為「類內定義」（隱式 inline）
    void bark() {
        // 直接使用 name，不需要傳參數！
        // 這是因為背後有隱藏的 this 指標（第 26 課會詳解）
        cout << name << " 說：汪汪！" << endl;
    }

    void sit() {
        cout << name << " 乖乖坐下了" << endl;
    }

    void showInfo() {
        cout << "名字: " << name << endl;
        cout << "年齡: " << age << " 歲" << endl;
        cout << "品種: " << breed << endl;
    }
};  // ← 別忘了分號！class 定義結束後必須加分號


// ===== 重點三：命名慣例（PascalCase）=====
//
// C++ 慣例：類別名用「大駝峰」（PascalCase），每個單字首字母大寫
//   class Dog { };          ✅ 正確
//   class BankAccount { };  ✅ 多個單字也用大駝峰
//   class dog { };          ⚠️ 能編譯，但不符合慣例
//   class DOG { };          ⚠️ 全大寫通常留給巨集（macro）
//
// 和變數名的 camelCase（myDog）或 snake_case（my_dog）區分開。


// ===== 重點四：存取修飾符（Access Specifiers）=====
//
// class 有三種存取等級：
//   public:    任何人都能存取
//   protected: 只有自己和子類能存取（繼承時使用）
//   private:   只有自己的成員函數能存取
//
// 關鍵規則：在 class 中，不寫任何存取修飾符時，「預設是 private」！
// 這和 struct 不同——struct 預設是 public（第 12 課會詳細比較）。
//
// 範例：
class AccessDemo {
    int secretValue;  // 這是 private！class 預設
public:
    int publicValue;  // 這是 public
    void setSecret(int v) { secretValue = v; }  // public 函數可以存取 private 成員
    int getSecret() { return secretValue; }
};


// ===== 重點五：成員函數的兩種定義方式 =====
//
// 方式一：類內定義（inline，直接寫在 class 裡面）
//   適合：函數體很短（1~3 行）的情況
//
// 方式二：類外定義（宣告和實現分離）
//   適合：函數體較長的情況，讓類別定義保持簡潔
//   語法：用「類別名::函數名」在 class 外面實現
//   實際專案：通常把宣告放在 .h 標頭檔，實現放在 .cpp 原始碼檔

class Calculator {
public:
    // 方式一：類內定義（短函數適用）
    int add(int a, int b) {
        return a + b;
    }

    // 方式二：類內只宣告，類外定義
    int subtract(int a, int b);       // 只有宣告
    void showResult(int result);      // 只有宣告
};

// 類外定義：用「類別名::函數名」的語法
// 「::」是範圍解析運算子（scope resolution operator）
// 告訴編譯器：這個 subtract 函數屬於 Calculator 類別
int Calculator::subtract(int a, int b) {
    return a - b;
}

void Calculator::showResult(int result) {
    cout << "計算結果 = " << result << endl;
}

// 注意常見錯誤：類外定義忘記加類別名前綴
// void showResult(int result) { }      // ❌ 這是全域函數，不是 Calculator 的成員！
// void Calculator::showResult(...) { } // ✅ 正確寫法


// ===== 重點六：成員函數直接存取成員變數 =====
//
// 這是 class 最核心的特性之一：
// 成員函數天生就能存取同一個類別的成員變數，不需要額外傳參數。
// 背後原理：編譯器偷偷傳入了 this 指標（第 26 課詳解）

class Person {
public:
    string name;
    int age;

    void introduce() {
        // 直接使用 name 和 age，不需要寫成 Person::name 或傳入參數！
        // 這是「this->name」和「this->age」的簡寫
        cout << "你好，我叫 " << name << "，今年 " << age << " 歲。" << endl;
    }
};

// 對比 C 語言寫法：
// 在 C 中，必須把 struct 指標手動傳入函數：
//   void introduce(struct Person* p) { printf("我叫 %s", p->name); }
// C++ 的成員函數自動知道自己屬於哪個對象，語法更自然。


// ===== 重點七：類內初始化（C++11 起）=====
//
// C++11 起，可以在宣告成員變數時直接給預設值。
// 這是避免「垃圾值」的好習慣，讓對象創建後就有合理的初始狀態。

class Student {
public:
    string name = "未命名";   // C++11 起支援類內初始化
    int age = 0;              // 預設值 0
    float gpa = 0.0f;         // 預設值 0.0
    bool isEnrolled = false;  // 預設值 false

    void show() {
        cout << name << ", " << age << " 歲, GPA=" << gpa << endl;
    }
};


// ===== 重點八：class vs C 語言 struct + 函數 =====
//
// C 語言風格（資料和函數是分離的）：
//
//   struct Dog_C {
//       char name[50];
//       int age;
//   };
//   void dog_bark(struct Dog_C* d) {      // 必須手動傳入指標
//       printf("%s 汪汪!\n", d->name);
//   }
//   // 使用：
//   struct Dog_C d;
//   strcpy(d.name, "旺財");
//   dog_bark(&d);                         // 必須手動傳入
//
// C++ class 風格（資料和函數綁在一起）：
//   Dog_CPP d;
//   d.name = "旺財";
//   d.bark();                             // 自然的「對象.行為」語法
//
// C++ 的 class 讓「資料」和「行為」的關係從語法層面就綁在一起，
// 而不是靠程式設計師的自律，這是 OOP 的本質優勢。


// ===== 重點九：class 定義的常見錯誤 =====
//
// 錯誤 1：忘記結尾分號（最常見！）
//   class Dog {
//   public:
//       string name;
//   }   // ❌ 缺少分號，編譯器會給出莫名其妙的錯誤訊息
//
// 錯誤 2：在類別定義內直接寫執行語句
//   class Dog {
//   public:
//       string name;
//       cout << name << endl;  // ❌ 不能在這裡寫執行語句！
//   };
//   類別定義裡只能放「宣告」，不能放獨立的執行語句。
//
// 錯誤 3：類外定義忘記加類別名前綴
//   void bark() { }         // ❌ 這是全域函數！
//   void Dog::bark() { }    // ✅ 這才是 Dog 的成員函數


// ===== 綜合實戰：銀行帳戶（完整展示所有重點）=====
//
// 這個範例展示：
// 1. 類內定義（display）和類外定義（deposit、withdraw）混用
// 2. 類內初始化（balance = 0.0）
// 3. 成員函數直接存取成員變數
// 4. 多個對象各自獨立（acc1 和 acc2 的 balance 互不影響）

class BankAccount {
public:
    // ===== 成員變數，有類內初始化預設值 =====
    string ownerName;
    string accountId;
    double balance = 0.0;  // 類內預設值，避免垃圾值

    // ===== 類內定義（函數體短，適合放在 class 裡）=====
    void display() {
        cout << "========================" << endl;
        cout << "帳戶持有人: " << ownerName << endl;
        cout << "帳號:       " << accountId << endl;
        cout << "餘額:       $" << balance << endl;
        cout << "========================" << endl;
    }

    // ===== 類內只宣告，類外定義（函數體較長）=====
    void deposit(double amount);
    void withdraw(double amount);
};

// 類外定義：注意 BankAccount:: 前綴
void BankAccount::deposit(double amount) {
    if (amount > 0) {
        balance += amount;  // 直接修改成員變數！
        cout << "存入 $" << amount << "，目前餘額: $" << balance << endl;
    } else {
        cout << "錯誤：存款金額必須大於 0" << endl;
    }
}

void BankAccount::withdraw(double amount) {
    if (amount <= 0) {
        cout << "錯誤：提款金額必須大於 0" << endl;
    } else if (amount > balance) {
        cout << "錯誤：餘額不足！目前餘額: $" << balance << endl;
    } else {
        balance -= amount;  // 直接修改成員變數！
        cout << "提取 $" << amount << "，目前餘額: $" << balance << endl;
    }
}


// ===== 主程式：示範所有重點 =====
int main() {
    cout << "===== 重點一到七示範 =====" << endl;

    // --- 基本 class 使用 ---
    Dog myDog;              // 根據 Dog 類別建造一個對象（在棧上）
    myDog.name = "旺財";    // 用「.」設定成員變數（對象.成員）
    myDog.age = 3;
    myDog.breed = "柴犬";

    myDog.showInfo();       // 用「.」調用成員函數
    myDog.bark();
    myDog.sit();

    cout << "\n--- Person 範例（成員函數存取成員變數）---" << endl;
    Person p1;
    p1.name = "小明";
    p1.age = 20;
    p1.introduce();  // 輸出：你好，我叫 小明，今年 20 歲。

    Person p2;
    p2.name = "小華";
    p2.age = 22;
    p2.introduce();  // p1 和 p2 各自獨立！

    cout << "\n--- Calculator 範例（類外定義）---" << endl;
    Calculator calc;
    int sum = calc.add(5, 3);       // 類內定義
    int diff = calc.subtract(10, 4); // 類外定義
    calc.showResult(sum);
    calc.showResult(diff);

    cout << "\n--- Student 範例（類內初始化）---" << endl;
    Student s1;            // 使用所有預設值
    s1.show();             // 輸出：未命名, 0 歲, GPA=0
    s1.name = "陳信安";
    s1.age = 20;
    s1.gpa = 3.8f;
    s1.show();

    cout << "\n===== 綜合範例：銀行帳戶 =====" << endl;
    BankAccount acc1;
    acc1.ownerName = "陳信安";
    acc1.accountId = "ACC-001";

    acc1.display();
    acc1.deposit(1000.0);
    acc1.deposit(500.0);
    acc1.withdraw(200.0);
    acc1.withdraw(2000.0);  // 故意超額
    acc1.deposit(-100.0);   // 故意輸入負數

    cout << endl;
    acc1.display();

    cout << "\n--- 第二個帳戶（獨立對象）---" << endl;
    // acc2 和 acc1 完全獨立，修改 acc2.balance 不影響 acc1.balance
    BankAccount acc2;
    acc2.ownerName = "小明";
    acc2.accountId = "ACC-002";
    acc2.balance = 3000.0;

    acc2.display();
    acc2.withdraw(500.0);
    acc2.display();

    cout << "\n--- 存取修飾符示範 ---" << endl;
    AccessDemo ad;
    ad.publicValue = 100;       // ✅ public，可以直接存取
    ad.setSecret(42);           // ✅ 透過 public 函數設定 private 值
    // ad.secretValue = 99;     // ❌ 如果取消註解，編譯錯誤！private 不能從外部存取
    cout << "public: " << ad.publicValue << endl;
    cout << "secret（透過 getter）: " << ad.getSecret() << endl;

    return 0;
}
