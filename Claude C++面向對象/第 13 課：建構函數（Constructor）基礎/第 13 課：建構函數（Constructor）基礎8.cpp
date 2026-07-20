// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 8  —  綜合實戰：Car 類別
// =============================================================================
//
// 【主題資訊 Information】
//   主題：本課完整講義 + 一個把「多載、驗證、預設值」全部用上的 Car 類別
//   標準版本：C++98 起即有建構函式；委派建構函式需 C++11
//   標頭檔：<string>、<vector>
//   本檔結構：① 下方大段課程講義（markdown）② 可執行的 Car 展示廳實作
//
// 【詳細解釋 Explanation】
//
// 【1. Car 的三個建構函式各對應一種真實情境】
//   * Car()                       → 展示廳的空位，稍後再填資料
//   * Car(brand, model)           → 新車，年份取當年、里程必然為 0
//   * Car(brand, model, y, mi)    → 二手車，四個屬性都要指定且都要驗證
//   注意第二個版本隱含了一條業務規則：「新車的里程一定是 0」。
//   把這條規則寫進建構函式，就沒有人能建立出「里程 5 萬公里的新車」——
//   這正是建構函式作為「業務規則守門員」的價值。
//
// 【2. 這個類別最值得改進的地方：重複的初始化與驗證】
//   原始版本三個建構函式各自寫了一遍 year = 2024、mileage = 0.0，
//   而驗證邏輯只出現在第三個。這帶來兩個實際問題：
//     (a) 預設年份要改成 2026 時，得改三個地方，漏一個就不一致
//     (b) 前兩個建構函式完全沒有驗證 —— 若日後有人讓它們接受參數，
//         很容易忘記補上檢查
//   正解是用委派建構函式（C++11）讓三者全部匯流到「完整版」，
//   初始化與驗證都只有一份。本檔的 CarV2 示範了這個重構。
//
// 【3. 驗證策略的選擇：靜默修正 vs 丟例外】
//   原始版本採「靜默修正 + 印警告」：年份不合法就改成 2024。
//   這在展示廳這種「資料由店員輸入、錯了就用預設值也無妨」的場景勉強可接受，
//   但要清楚它的代價：呼叫端不知道自己的輸入被改掉了。
//   若這是車輛履歷系統（年份牽涉估價與稅務），靜默修正就是嚴重錯誤 ——
//   應該丟例外或回傳 optional，讓呼叫端無法忽略（見本課第 7 檔的三種策略）。
//
// 【4. 硬編年份 2025 是個真實的維護地雷】
//   原始碼寫 `if (y < 1886 || y > 2025)`。這行程式碼在 2026 年就會開始
//   誤判所有當年的新車為非法。這類「寫死當下年份」的檢查在真實專案中
//   非常常見，而且往往要等到跨年才爆發。
//   正解是從系統時間動態取得當前年份（本檔的 CarV2 用 <ctime> 實作），
//   或至少把它抽成具名常數並加註「需定期更新」。
//
// 【概念補充 Concept Deep Dive】
//   * 三個建構函式的參數個數不同（0、2、4），因此不會有多載歧義。
//     若新增一個 Car(string) 版本，則 Car("Toyota") 仍然明確；
//     但若新增 Car(string, int)，那 Car("Toyota", "Camry") 會因為
//     const char* 轉 string 與轉 int 的優先序而變得微妙 —— 設計時要留意。
//   * 本檔的建構函式在本體內賦值而非用初始化列表，因此 brand / model
//     這兩個 std::string 各被「預設建構一次 + 賦值一次」。
//     CarV2 改用初始化列表，省掉多餘的一次操作（見第 3 檔的實測）。
//   * 1886 這個下限是有典故的：Karl Benz 的 Patent-Motorwagen 於 1886 年
//     取得專利，通常被視為汽車的誕生年。這種「有業務意義的魔術數字」
//     應該寫成具名常數並附註來源，而不是裸露在條件式裡。
//
// 【注意事項 Pay Attention】
//   1. 別在驗證條件裡硬編當下年份，跨年就會誤判。
//   2. 多個建構函式的重複邏輯要委派，否則修改時必定產生不一致。
//   3. 靜默修正會隱藏呼叫端的錯誤；只在「錯了也無妨」的場景使用。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構函數綜合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Car 有三個建構函式，編譯器怎麼決定要呼叫哪一個？
//     答：走一般的多載解析（overload resolution），依引數的個數與型別比對。
//         這三個版本的參數個數分別是 0、2、4，彼此不可能歧義。
//         若參數個數相同而型別相近（例如同時有 Car(string,int) 與
//         Car(string,double)），就可能出現 ambiguous 的編譯錯誤。
//     追問：那 Car c2("Toyota", "Camry") 傳的是 const char*，怎麼會匹配 string？
//         → 透過 std::string 的轉換建構函式做一次隱式轉換，這是允許的標準轉換。
//
// 🔥 Q2. 這三個建構函式有什麼設計上的問題？該怎麼改？
//     答：初始化與驗證邏輯重複 —— year=2024、mileage=0.0 各寫了三遍，
//         而驗證只寫在第三個版本。改法是用委派建構函式（C++11）讓前兩個
//         委派給完整版，初始化與驗證都集中在一處，日後修改不會漏改。
//     追問：委派會有效能成本嗎？→ 不會。它就是一次普通的建構函式呼叫，
//           而且通常會被內聯掉。
//
// ⚠️ 陷阱. `if (y < 1886 || y > 2025)` 這行檢查有什麼問題？
//     答：硬編了「當下的年份」。這段程式碼在 2026 年會開始把所有當年的新車
//         判定為非法年份並靜默改成 2024 —— 而且問題會在跨年的那一刻才出現，
//         測試時通常完全發現不了。
//     為什麼會錯：寫程式當下覺得「2025 就是未來的上限」，
//         忽略了程式的壽命通常比這個假設長。凡是與「現在」有關的邊界，
//         都應該從系統時間動態取得，而不是寫死。
// ═══════════════════════════════════════════════════════════════════════════

/*
好的，信安！我們進入 **第三階段：建構與解構**，從第 13 課開始。

---

# 第 13 課：建構函數（Constructor）基礎

---

## 13.1 什麼是建構函數？

在前面的課程中，我們學會了定義類別和創建對象。但有個問題——當我們創建一個對象時，它的成員變數處於什麼狀態？

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name;
    int age;
    float gpa;

    void print() {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    Student s;
    s.print();  // name 是空字串, age 和 gpa 是未定義的垃圾值！
    return 0;
}
```

**問題**：`age` 和 `gpa` 沒有被初始化，它們的值是不確定的垃圾值（和你在 C 語言中遇到的一樣）。`string` 類型因為自身有預設建構函數，所以會被初始化為空字串，但基本型別（int、float 等）不會。

**建構函數（Constructor）** 就是用來解決這個問題的——它是一個**在對象創建時自動被調用的特殊成員函數**，負責初始化對象的狀態。

---

## 13.2 建構函數的語法規則

建構函數有幾個獨特的特徵，和普通成員函數不同：

| 特徵 | 說明 |
|------|------|
| 函數名 | **必須與類別名完全相同** |
| 返回值 | **沒有返回值，連 `void` 都不寫** |
| 調用時機 | **對象創建時自動調用**，不需要手動調用 |
| 可以重載 | 一個類別可以有多個建構函數（參數不同） |
| 存取權限 | 通常是 `public`（否則外界無法創建對象） |

```cpp
class 類別名 {
public:
    // 建構函數：函數名 = 類別名，無返回值
    類別名() {
        // 初始化程式碼
    }
    
    類別名(參數列表) {
        // 帶參數的初始化程式碼
    }
};
```

---

## 13.3 對比 C 語言的初始化方式

在 C 語言中，你需要手動調用初始化函數：

```c
// C 語言風格
typedef struct {
    char name[50];
    int age;
    float gpa;
} Student;

// 必須手動調用！而且有可能忘記調用！
void student_init(Student* s, const char* name, int age, float gpa) {
    strcpy(s->name, name);
    s->age = age;
    s->gpa = gpa;
}

int main() {
    Student s;
    // 如果忘記調用 student_init()，s 的內容就是垃圾值
    // 編譯器不會警告你！
    student_init(&s, "張三", 20, 3.8);
    return 0;
}
```

C 語言的問題是：**初始化和對象創建是分離的**，程式設計師可能忘記調用初始化函數，而編譯器不會提醒你。

C++ 的建構函數把這兩步合為一步，**保證對象一旦創建就已經被正確初始化**。

---

## 13.4 第一個建構函數範例

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // ====== 建構函數 ======
    // 函數名 = 類別名 "Student"
    // 沒有返回值（連 void 都不寫）
    Student() {
        cout << ">>> 建構函數被調用了！ <<<" << endl;
        name = "未命名";
        age = 0;
        gpa = 0.0f;
    }

    void print() const {
        cout << "姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 準備創建對象 ===" << endl;
    
    Student s;  // 創建對象的瞬間，建構函數自動被調用
    
    cout << "=== 對象已創建 ===" << endl;
    s.print();  // 成員變數已被初始化，不再是垃圾值
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson13 lesson13.cpp
./lesson13
```

### 預期輸出

```
=== 準備創建對象 ===
>>> 建構函數被調用了！ <
=== 對象已創建 ===
姓名: 未命名, 年齡: 0, GPA: 0
```

**關鍵觀察**：我們從來沒有手動調用 `Student()`，它在 `Student s;` 這一行**自動**被調用了。

---

## 13.5 建構函數的調用時機

建構函數在不同場景下的調用時機：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Box {
private:
    string label;
    
public:
    Box() {
        label = "空盒子";
        cout << "建構函數: 創建了 [" << label << "]" << endl;
    }
    
    string getLabel() const { return label; }
};

// ====== 全域對象 ======
Box globalBox;  // 程式啟動時，在 main() 之前就建構

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    // ====== 局部對象 ======
    Box localBox;  // 執行到這一行時建構
    
    cout << "\n=== 進入區塊 ===" << endl;
    {
        // ====== 區塊內的對象 ======
        Box blockBox;  // 進入這個區塊時建構
        cout << "區塊內..." << endl;
    }  // blockBox 在這裡離開作用域
    
    cout << "\n=== 動態對象 ===" << endl;
    // ====== 動態分配的對象 ======
    Box* heapBox = new Box();  // new 的時候建構
    delete heapBox;  // 記得釋放
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
```

### 預期輸出

```
建構函數: 創建了 [空盒子]

=== main() 開始 ===
建構函數: 創建了 [空盒子]

=== 進入區塊 ===
建構函數: 創建了 [空盒子]
區塊內...

=== 動態對象 ===
建構函數: 創建了 [空盒子]

=== main() 結束 ===
```

**四種場景總結**：

| 對象類型 | 建構時機 | 範例 |
|----------|----------|------|
| 全域對象 | `main()` 之前 | `Box globalBox;` |
| 局部對象 | 執行到宣告語句時 | `Box localBox;` |
| 區塊內對象 | 進入區塊執行到宣告時 | `{ Box blockBox; }` |
| 動態對象 | `new` 運算子執行時 | `new Box()` |

---

## 13.6 帶參數的建構函數

只能初始化為固定值很不實用。實際上，建構函數可以接受參數：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:
    string name;
    int age;
    float gpa;

public:
    // 無參數建構函數
    Student() {
        name = "未命名";
        age = 0;
        gpa = 0.0f;
        cout << "[預設建構] 創建學生: " << name << endl;
    }

    // 帶參數的建構函數
    Student(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        cout << "[帶參建構] 創建學生: " << name << endl;
    }

    void print() const {
        cout << "  姓名: " << name 
             << ", 年齡: " << age 
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== 使用預設建構函數 ===" << endl;
    Student s1;           // 調用無參數建構函數
    s1.print();
    
    cout << "\n=== 使用帶參數建構函數 ===" << endl;
    Student s2("張三", 20, 3.8f);   // 調用帶參數建構函數
    s2.print();
    
    Student s3("李四", 22, 3.5f);   // 另一個帶參數建構
    s3.print();
    
    return 0;
}
```

### 預期輸出

```
=== 使用預設建構函數 ===
[預設建構] 創建學生: 未命名
  姓名: 未命名, 年齡: 0, GPA: 0

=== 使用帶參數建構函數 ===
[帶參建構] 創建學生: 張三
  姓名: 張三, 年齡: 20, GPA: 3.8

[帶參建構] 創建學生: 李四
  姓名: 李四, 年齡: 22, GPA: 3.5
```

---

## 13.7 建構函數重載

和普通函數一樣，建構函數支援重載——可以有多個建構函數，只要參數列表不同：

```cpp
#include <iostream>
#include <string>
using namespace std;

class Rectangle {
private:
    double width;
    double height;
    string color;

public:
    // 建構函數 1：無參數 → 預設的矩形
    Rectangle() {
        width = 1.0;
        height = 1.0;
        color = "白色";
        cout << "[建構1] 預設矩形" << endl;
    }

    // 建構函數 2：正方形（只給一個邊長）
    Rectangle(double side) {
        width = side;
        height = side;
        color = "白色";
        cout << "[建構2] 正方形, 邊長 = " << side << endl;
    }

    // 建構函數 3：指定寬高
    Rectangle(double w, double h) {
        width = w;
        height = h;
        color = "白色";
        cout << "[建構3] 矩形 " << w << " x " << h << endl;
    }

    // 建構函數 4：指定寬高和顏色
    Rectangle(double w, double h, string c) {
        width = w;
        height = h;
        color = c;
        cout << "[建構4] " << c << "矩形 " << w << " x " << h << endl;
    }

    void print() const {
        cout << "  " << color << " 矩形: " 
             << width << " x " << height 
             << ", 面積 = " << width * height << endl;
    }
};

int main() {
    Rectangle r1;                         // 調用建構函數 1
    r1.print();
    
    Rectangle r2(5.0);                    // 調用建構函數 2
    r2.print();
    
    Rectangle r3(4.0, 6.0);              // 調用建構函數 3
    r3.print();
    
    Rectangle r4(3.0, 7.0, "紅色");       // 調用建構函數 4
    r4.print();
    
    return 0;
}
```

### 預期輸出

```
[建構1] 預設矩形
  白色 矩形: 1 x 1, 面積 = 1
[建構2] 正方形, 邊長 = 5
  白色 矩形: 5 x 5, 面積 = 25
[建構3] 矩形 4 x 6
  白色 矩形: 4 x 6, 面積 = 24
[建構4] 紅色矩形 3 x 7
  紅色 矩形: 3 x 7, 面積 = 21
```

**編譯器如何選擇？** 根據你傳入的參數數量和型別，編譯器自動匹配最合適的建構函數，這就是**重載解析（Overload Resolution）**。

---

## 13.8 建構函數中的資料驗證

建構函數不僅用來賦值，還可以加入**驗證邏輯**，確保對象從一開始就處於合法狀態：

```cpp
#include <iostream>
#include <string>
using namespace std;

class BankAccount {
private:
    string owner;
    double balance;
    string accountId;

public:
    BankAccount(string ownerName, double initialBalance, string id) {
        // 驗證帳戶名
        if (ownerName.empty()) {
            cout << "警告：帳戶名不能為空，使用預設值" << endl;
            owner = "未知";
        } else {
            owner = ownerName;
        }
        
        // 驗證初始餘額
        if (initialBalance < 0) {
            cout << "警告：初始餘額不能為負數，設為 0" << endl;
            balance = 0.0;
        } else {
            balance = initialBalance;
        }
        
        // 驗證帳戶 ID
        if (id.length() != 10) {
            cout << "警告：帳戶 ID 必須為 10 位，使用預設 ID" << endl;
            accountId = "0000000000";
        } else {
            accountId = id;
        }
        
        cout << "帳戶創建成功: " << owner << endl;
    }

    void print() const {
        cout << "  帳戶: " << accountId 
             << ", 戶主: " << owner 
             << ", 餘額: $" << balance << endl;
    }
};

int main() {
    cout << "=== 正常創建 ===" << endl;
    BankAccount a1("王五", 10000.0, "1234567890");
    a1.print();
    
    cout << "\n=== 非法數據 ===" << endl;
    BankAccount a2("", -500.0, "123");  // 全部都是非法值
    a2.print();
    
    return 0;
}
```

### 預期輸出

```
=== 正常創建 ===
帳戶創建成功: 王五
  帳戶: 1234567890, 戶主: 王五, 餘額: $10000

=== 非法數據 ===
警告：帳戶名不能為空，使用預設值
警告：初始餘額不能為負數，設為 0
警告：帳戶 ID 必須為 10 位，使用預設 ID
帳戶創建成功: 未知
  帳戶: 0000000000, 戶主: 未知, 餘額: $0
```

**重要觀念**：建構函數是你防止非法對象存在的**第一道防線**。一個好的建構函數應該保證——如果對象成功創建，它的狀態一定是合法的。

---

## 13.9 常見錯誤與陷阱

### 陷阱 1：不要給建構函數寫返回值

```cpp
class Bad {
public:
    void Bad() {  // 錯誤！建構函數不能有返回值！
        // ...     // 這會變成一個普通的成員函數，名字碰巧叫 Bad
    }
};
```

### 陷阱 2：宣告對象時不要加空括號

```cpp
Student s();   // 錯誤！這是函數宣告，不是對象創建！
               // 編譯器把它理解為：宣告一個函數 s，返回 Student

Student s;     // 正確：調用預設建構函數
Student s{};   // 正確（C++11）：統一初始化語法
```

這個陷阱被稱為 **最令人困擾的解析（Most Vexing Parse）**，是 C++ 中非常著名的坑。

### 陷阱 3：如果定義了帶參建構函數，預設建構函數就消失了

```cpp
class Point {
public:
    int x, y;
    
    // 只定義了帶參數的建構函數
    Point(int px, int py) {
        x = px;
        y = py;
    }
};

int main() {
    Point p1(3, 4);   // OK
    Point p2;          // 編譯錯誤！沒有預設建構函數！
    return 0;
}
```

**原因**：如果你沒有定義任何建構函數，編譯器會自動生成一個預設建構函數。但只要你定義了**任何一個**建構函數，編譯器就**不再自動生成**預設建構函數。這個問題我們在第 14 課會詳細講解。

---

## 13.10 完整綜合範例

```cpp
#include <iostream>
#include <string>
using namespace std;

class Car {
private:
    string brand;
    string model;
    int year;
    double mileage;

public:
    // 建構函數 1：完全預設
    Car() {
        brand = "未知";
        model = "未知";
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建預設汽車" << endl;
    }
    
    // 建構函數 2：只指定品牌和型號
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建新車: " << brand << " " << model << endl;
    }
    
    // 建構函數 3：完整指定所有屬性
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;
        
        // 年份驗證
        if (y < 1886 || y > 2025) {  // 1886 年是汽車發明的年份
            cout << "  警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }
        
        // 里程驗證
        if (mi < 0) {
            cout << "  警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }
        
        cout << "[建構] 創建二手車: " << brand << " " << model 
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model 
             << ", 里程: " << mileage << " km" << endl;
    }
};

int main() {
    cout << "===== 汽車展示廳 =====" << endl << endl;
    
    Car c1;
    c1.print();
    cout << endl;
    
    Car c2("Toyota", "Camry");
    c2.print();
    cout << endl;
    
    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();
    cout << endl;
    
    // 測試非法數據
    Car c4("時光機", "DeLorean", 1800, -100.0);
    c4.print();
    
    return 0;
}
```

### 編譯與執行

```bash
g++ -std=c++17 -o lesson13_full lesson13_full.cpp
./lesson13_full
```

### 預期輸出

```
===== 汽車展示廳 =====

[建構] 創建預設汽車
  2024 未知 未知, 里程: 0 km

[建構] 創建新車: Toyota Camry
  2024 Toyota Camry, 里程: 0 km

[建構] 創建二手車: BMW M3 (2020)
  2020 BMW M3, 里程: 35000.5 km

  警告：年份不合法，使用 2024
  警告：里程不能為負，設為 0
[建構] 創建二手車: 時光機 DeLorean (2024)
  2024 時光機 DeLorean, 里程: 0 km
```

---

## 13.11 本課重點回顧

| 概念 | 說明 |
|------|------|
| 建構函數 | 對象創建時自動調用的特殊成員函數 |
| 命名規則 | 必須與類別名相同，沒有返回值 |
| 調用時機 | 對象宣告時 / `new` 時自動調用 |
| 重載 | 一個類別可以有多個建構函數 |
| 資料驗證 | 建構函數是確保對象合法性的第一道防線 |
| Most Vexing Parse | `Student s();` 是函數宣告，不是對象創建 |
| 編譯器行為 | 定義了任何建構函數後，預設建構函數不再自動生成 |

---

## 13.12 下一課預告

下一課我們將深入探討**預設建構函數（Default Constructor）**——什麼時候編譯器會幫你生成？什麼時候不會？以及 C++11 的 `= default` 語法。

準備好進入 **第 14 課：預設建構函數** 了嗎？
*/



#include <iostream>
#include <string>
#include <vector>
#include <ctime>
using namespace std;

class Car {
private:
    string brand;
    string model;
    int year;
    double mileage;

public:
    // 建構函數 1：完全預設
    Car() {
        brand = "未知";
        model = "未知";
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建預設汽車" << endl;
    }

    // 建構函數 2：只指定品牌和型號（隱含業務規則：新車里程必為 0）
    Car(string b, string m) {
        brand = b;
        model = m;
        year = 2024;
        mileage = 0.0;
        cout << "[建構] 創建新車: " << brand << " " << model << endl;
    }

    // 建構函數 3：完整指定所有屬性
    Car(string b, string m, int y, double mi) {
        brand = b;
        model = m;

        // 年份驗證
        // ⚠️ 注意：上限硬編 2025 是個維護地雷，跨年就會誤判當年新車。
        //    正解見下方 CarV2：從系統時間動態取得當前年份。
        if (y < 1886 || y > 2025) {  // 1886 年是汽車發明的年份
            cout << "  警告：年份不合法，使用 2024" << endl;
            year = 2024;
        } else {
            year = y;
        }

        // 里程驗證
        if (mi < 0) {
            cout << "  警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        } else {
            mileage = mi;
        }

        cout << "[建構] 創建二手車: " << brand << " " << model
             << " (" << year << ")" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << " " << model
             << ", 里程: " << mileage << " km" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【重構版】CarV2：委派建構函式 + 動態年份上限 + 初始化列表
//   三處改進，全部針對上方 Car 的實際缺陷：
//     (1) 委派：初始化與驗證只寫一份，三種建立方式共用，日後修改不會漏改
//     (2) 動態年份：用 <ctime> 取得當前年份當上限，跨年不會誤判
//     (3) 初始化列表：brand/model 直接以目標值建構，省掉「預設建構+賦值」
//   常數 FIRST_CAR_YEAR 具名並附註來源，取代裸露的魔術數字 1886。
// -----------------------------------------------------------------------------
class CarV2 {
public:
    // 1886：Karl Benz 的 Patent-Motorwagen 取得專利，通常視為汽車誕生年
    static const int FIRST_CAR_YEAR = 1886;

    // 動態取得當前年份，取代硬編上限
    static int currentYear() {
        time_t now = time(nullptr);
        tm* lt = localtime(&now);
        return (lt != nullptr) ? (lt->tm_year + 1900) : 2024;   // 取不到就退回安全值
    }

    // 完整版：唯一做初始化與驗證的地方
    CarV2(const string& b, const string& m, int y, double mi)
        : brand(b), model(m), year(y), mileage(mi) {      // 宣告順序 = 初始化順序
        int maxYear = currentYear() + 1;                  // 允許明年式的新車
        if (year < FIRST_CAR_YEAR || year > maxYear) {
            cout << "  警告：年份 " << year << " 不合法（合法範圍 "
                 << FIRST_CAR_YEAR << "-" << maxYear << "），改用 " << currentYear() << endl;
            year = currentYear();
        }
        if (mileage < 0) {
            cout << "  警告：里程不能為負，設為 0" << endl;
            mileage = 0.0;
        }
    }

    // 其餘版本全部委派過去，不重複任何邏輯
    CarV2() : CarV2("未知", "未知", currentYear(), 0.0) {}
    CarV2(const string& b, const string& m) : CarV2(b, m, currentYear(), 0.0) {}

    void print() const {
        cout << "  " << year << " " << brand << " " << model
             << ", 里程: " << mileage << " km" << endl;
    }

private:
    string brand;      // 宣告順序決定初始化順序
    string model;
    int    year;
    double mileage;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】車輛庫存報表：建構期驗證讓後續統計可以無條件信任資料
//   情境：中古車行的庫存系統，要算出「平均里程」與「最舊的車」。
//   關鍵洞察：因為 CarV2 的建構函式保證了 mileage >= 0 且 year 在合理範圍，
//   這個統計函式完全不需要再做防禦性檢查 —— 不變條件已經在物件誕生時建立。
//   若沒有建構期驗證，這裡就得對每一筆資料重複檢查一次，
//   而且一旦漏檢，負里程會直接污染平均值且極難追查來源。
//   「驗證一次、之後全部信任」是封裝帶來的最大工程效益。
// -----------------------------------------------------------------------------
struct InventoryReport {
    size_t count = 0;
    double avgMileage = 0.0;
    int    oldestYear = 0;
    string oldestName;
};

class CarRecord {
public:
    CarRecord(const string& name, int year, double mileage)
        : m_name(name), m_year(year), m_mileage(mileage) {
        // 不變條件在此建立：里程非負、年份合理
        if (m_mileage < 0) m_mileage = 0.0;
        int maxYear = CarV2::currentYear() + 1;
        if (m_year < CarV2::FIRST_CAR_YEAR || m_year > maxYear) {
            m_year = CarV2::currentYear();
        }
    }
    const string& name()    const { return m_name; }
    int           year()    const { return m_year; }
    double        mileage() const { return m_mileage; }

private:
    string m_name;
    int    m_year;
    double m_mileage;
};

InventoryReport summarize(const vector<CarRecord>& stock) {
    InventoryReport rep;
    if (stock.empty()) return rep;          // 空庫存要先擋，否則除以 0

    double total = 0.0;
    rep.oldestYear = stock[0].year();
    rep.oldestName = stock[0].name();
    for (const CarRecord& c : stock) {
        // 不需要檢查 c.mileage() 是否為負 —— 建構函式已經保證了
        total += c.mileage();
        if (c.year() < rep.oldestYear) {
            rep.oldestYear = c.year();
            rep.oldestName = c.name();
        }
    }
    rep.count = stock.size();
    rep.avgMileage = total / static_cast<double>(stock.size());
    return rep;
}

int main() {
    cout << "===== 汽車展示廳 =====" << endl << endl;

    Car c1;
    c1.print();
    cout << endl;

    Car c2("Toyota", "Camry");
    c2.print();
    cout << endl;

    Car c3("BMW", "M3", 2020, 35000.5);
    c3.print();
    cout << endl;

    // 測試非法數據
    Car c4("時光機", "DeLorean", 1800, -100.0);
    c4.print();

    cout << "\n===== 重構版 CarV2（委派 + 動態年份）=====" << endl;
    cout << "（本機當前年份 = " << CarV2::currentYear() << "）" << endl;

    CarV2 v1;
    v1.print();

    CarV2 v2("Toyota", "Camry");     // 新車：年份自動取當年，里程必為 0
    v2.print();

    CarV2 v3("BMW", "M3", 2020, 35000.5);
    v3.print();

    cout << "  非法資料一律由完整版統一攔截:" << endl;
    CarV2 v4("時光機", "DeLorean", 1800, -100.0);
    v4.print();

    cout << "\n  ↑ 三種建立方式共用同一份驗證；年份上限隨系統時間變動，" << endl;
    cout << "    不會像硬編 2025 那樣在跨年後誤判當年新車。" << endl;

    cout << "\n===== 實務：庫存報表 =====" << endl;
    vector<CarRecord> stock = {
        CarRecord("Toyota Camry",   2020, 35000.5),
        CarRecord("Honda Civic",    2018, 82000.0),
        CarRecord("Mazda 3",        2022, 12500.0),
        CarRecord("Ford Focus",     2015, -500.0),    // 髒資料：負里程
        CarRecord("BMW M3",         1800,  60000.0),  // 髒資料：不合理年份
    };

    InventoryReport rep = summarize(stock);
    cout << "  庫存數量: " << rep.count << " 輛" << endl;
    cout << "  平均里程: " << rep.avgMileage << " km" << endl;
    cout << "  最舊車輛: " << rep.oldestName << "（" << rep.oldestYear << " 年）" << endl;
    cout << "  ↑ 兩筆髒資料在建構時就被修正，統計函式完全不需要重複防禦性檢查" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎8.cpp" -o demo8
// 註：委派建構函式需 C++11 以上。

// === 預期輸出 ===
// ⚠️ 注意：CarV2 使用系統時間動態取得年份，因此下方「2026」「1886-2027」
//    等數字會隨執行當下的年份改變 —— 這正是它取代硬編 2025 的目的。
//    上半部舊版 Car 的輸出（硬編 2024/2025）則不受時間影響。
//
// ===== 汽車展示廳 =====
//
// [建構] 創建預設汽車
//   2024 未知 未知, 里程: 0 km
//
// [建構] 創建新車: Toyota Camry
//   2024 Toyota Camry, 里程: 0 km
//
// [建構] 創建二手車: BMW M3 (2020)
//   2020 BMW M3, 里程: 35000.5 km
//
//   警告：年份不合法，使用 2024
//   警告：里程不能為負，設為 0
// [建構] 創建二手車: 時光機 DeLorean (2024)
//   2024 時光機 DeLorean, 里程: 0 km
//
// ===== 重構版 CarV2（委派 + 動態年份）=====
// （本機當前年份 = 2026）
//   2026 未知 未知, 里程: 0 km
//   2026 Toyota Camry, 里程: 0 km
//   2020 BMW M3, 里程: 35000.5 km
//   非法資料一律由完整版統一攔截:
//   警告：年份 1800 不合法（合法範圍 1886-2027），改用 2026
//   警告：里程不能為負，設為 0
//   2026 時光機 DeLorean, 里程: 0 km
//
//   ↑ 三種建立方式共用同一份驗證；年份上限隨系統時間變動，
//     不會像硬編 2025 那樣在跨年後誤判當年新車。
//
// ===== 實務：庫存報表 =====
//   庫存數量: 5 輛
//   平均里程: 37900.1 km
//   最舊車輛: Ford Focus（2015 年）
//   ↑ 兩筆髒資料在建構時就被修正，統計函式完全不需要重複防禦性檢查
