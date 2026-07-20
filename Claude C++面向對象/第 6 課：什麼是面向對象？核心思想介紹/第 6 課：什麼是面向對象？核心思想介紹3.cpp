// =============================================================================
//  第 6 課：什麼是面向對象？核心思想介紹3.cpp  —  抽象（Abstraction）與純虛擬介面
// =============================================================================
//
//  ⚠️ 檔案結構說明：本檔【前 364 行是一個 /* */ 註解區塊】（本課講義），
//     裡面含有多個示範用的 class 與 int main()，那些程式碼【不會被編譯】。
//     真正會編譯執行的程式從第 372 行的 class Database 開始。
//     搜尋 Database / doWork / main 時請以【最後一次出現】為準。
//
// 【主題資訊 Information】
//   virtual 回傳型別 函式名(參數) = 0;      // 純虛擬函式（pure virtual）
//   含有純虛擬函式的類別稱為【抽象類別】（abstract class），無法建立實體
//   標準版本：C++98 起（override 為 C++11）
//   執行期成本：與一般虛擬函式相同（一次 vtable 查表）
//   標頭檔：語言核心特性
//
// 【詳細解釋 Explanation】
//
// 【1. = 0 的意義：宣告「有這個能力，但這裡沒有實作」】
//   Database 宣告了 connect / query / disconnect 三個純虛擬函式，
//   等於定義了一份【契約】：任何想當 Database 的類別，
//   都必須提供這三個操作。
//   純虛擬帶來兩個編譯期保證：
//     ① Database 無法被實體化 —— Database db; 直接編譯失敗。
//        這是對的，「一個通用的資料庫」本來就不是可以存在的東西。
//     ② 派生類若漏掉任何一個函式沒實作，它自己也還是抽象類別，
//        一旦試圖建立實體就編譯失敗。
//   對照同課 2 號檔的 Shape::area() { return 0; }——
//   那個版本可以被實體化，而且忘記覆寫時會安靜地回傳 0，
//   錯誤要到執行期才會顯現。純虛擬把這類錯誤提前到編譯期。
//
// 【2. 這個檔案示範的是「依賴反轉」】
//   doWork(Database*) 只認識抽象介面，
//   完全不知道 MySQL 或 SQLite 的存在。
//   於是依賴方向被反轉了：
//       傳統寫法：業務邏輯 → 直接依賴 MySQL（換資料庫要改業務邏輯）
//       本檔寫法：業務邏輯 → 依賴 Database 介面 ← MySQL 實作它
//   要換成 PostgreSQL 只需新增一個類別，doWork 一行都不用改。
//   這也是單元測試的基礎 —— 測試時傳入一個 FakeDatabase，
//   就能在完全不碰真實資料庫的情況下測試業務邏輯。
//
// 【3. 抽象類別仍然可以有資料成員與已實作的函式】
//   常見誤解是「抽象類別＝純介面，不能有任何實作」。
//   實際上 C++ 的抽象類別只要求【至少一個】純虛擬函式，
//   其他成員完全自由：可以有資料成員、一般成員函式、
//   甚至可以為純虛擬函式【提供】一份實作
//   （派生類仍必須覆寫，但可以用 Database::connect() 明確呼叫基類版本，
//     這是共用預設行為的技巧）。
//   這讓 C++ 的抽象類別比 Java 的 interface 更靈活，
//   同時也涵蓋了 abstract class 的角色。
//
// 【4. 為什麼 query 的參數是 const char* 而不是 std::string】
//   這是原始教材的簡化。實務上應該用 std::string_view（C++17）
//   或 const std::string&：
//     • const char*      無法承載含 '\0' 的資料、無法直接得知長度
//     • const string&    若呼叫端傳字面值會產生一次臨時物件與配置
//     • string_view      零複製、能表示子字串、可接受兩者
//   介面一旦公開就很難改，參數型別的選擇在抽象基類上格外重要。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 抽象類別的物件大小
//   本機實測：sizeof(Database) = 8（只有 vptr），
//   sizeof(MySQL) = sizeof(SQLite) = 8 —— 兩者都沒有新增資料成員。
//   純虛擬函式在 vtable 中通常填入一個特殊項目
//   （libstdc++ 會指向 __cxa_pure_virtual）。
//   這些數值屬【實作定義】。
//
// (B) 為什麼還是需要 virtual ~Database()
//   本檔的 doWork 只接收指標、沒有 delete，所以不會踩到；
//   但只要有人寫 Database* db = new MySQL(); delete db;
//   而基類解構子非虛擬，就是【未定義行為】。
//   純虛擬介面幾乎必然會透過基類指標管理生命週期，
//   所以 virtual 解構子在這裡不是選配而是必需。
//   （另一個選項是宣告為 protected 非虛擬解構子，禁止多型刪除。）
//
// (C) 純虛擬解構子是合法的，而且必須有定義
//   virtual ~Database() = 0; 是允許的寫法
//   （用來讓類別成為抽象，即使沒有其他純虛擬函式），
//   但你【仍然必須】在類別外提供定義：
//       Database::~Database() {}
//   因為派生類的解構子一定會呼叫它。
//   這是 C++ 中「純虛擬卻必須有實作」的唯一情況。
//
// 【注意事項 Pay Attention】
//   1. 本檔前 364 行是註解區塊，其中的 class 與 main 都不會被編譯。
//   2. 抽象類別無法建立實體，但【可以】有指標與參考 ——
//      事實上多型正是靠這一點運作的。
//   3. 抽象類別仍可擁有資料成員與已實作的函式，不等於「純介面」。
//   4. 純虛擬介面幾乎一定會透過基類指標管理生命週期，
//      所以 virtual 解構子是必需品。
//   5. 派生類若漏實作任一純虛擬函式，它自己也是抽象類別 ——
//      錯誤會在「試圖建立實體」的那一行才出現，而不是在類別定義處。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】抽象類別與純虛擬函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 純虛擬函式（= 0）和「有實作的虛擬函式」差在哪？為什麼要用純虛擬？
//     答：純虛擬讓類別成為【抽象類別】，無法建立實體，
//         而且派生類若沒有實作它，派生類自己也還是抽象的 ——
//         錯誤在【編譯期】就被擋下來。
//         有實作的虛擬函式（如 Shape::area() { return 0; }）
//         則允許派生類忘記覆寫，程式照樣編譯通過、
//         執行期安靜地回傳一個無意義的預設值。
//         用純虛擬等於把「你必須實作這個」寫進型別系統。
//     追問：純虛擬函式可以有實作嗎？
//           → 可以。= 0 只表示「派生類必須覆寫」，
//             不表示「不能有定義」。基類仍可提供一份預設實作，
//             派生類用 Base::func() 明確呼叫它來共用邏輯。
//             而純虛擬【解構子】更是【必須】提供定義，
//             因為派生類的解構子一定會呼叫它。
//
// 🔥 Q2. 這個 Database 介面的設計，對單元測試有什麼幫助？
//     答：doWork 只依賴抽象的 Database，不知道任何具體實作。
//         測試時可以傳入一個 FakeDatabase（記錄呼叫、回傳假資料），
//         完全不需要真的連上資料庫就能驗證業務邏輯，
//         測試因此變得快速、穩定、可重複。
//         這就是依賴反轉原則帶來的可測試性。
//     追問：那為什麼不直接用 template 做靜態多型？
//           → 可以，而且沒有執行期成本（CRTP 或 concept）。
//             取捨是：template 的型別必須在編譯期已知，
//             無法把不同型別放進同一個容器、也無法在執行期依設定檔切換實作；
//             而且會增加編譯時間與程式碼膨脹。
//             需要執行期彈性就用虛擬函式，需要極致效能就用 template。
//
// ⚠️ 陷阱. 「抽象類別不能被實體化，所以不能有建構子」—— 對嗎？
//     答：不對。抽象類別【可以而且經常需要】有建構子。
//         它不是用來建立 Database 物件的，
//         而是在派生類建構時，由派生類的建構子自動呼叫，
//         用來初始化基類子物件裡的資料成員。
//         這也是為什麼抽象基類的建構子通常宣告為 protected ——
//         明確表示「只給派生類用」。
//     為什麼會錯：把「不能建立獨立實體」誤解成「這個類別不會被建構」。
//         實際上每次建立 MySQL 物件時，
//         Database 的建構子【一定】會被執行（它就是物件的一部分）。
//         同樣的誤解也常出現在解構子上 ——
//         正因為 Database 的解構子必然會被呼叫，
//         純虛擬解構子才【必須】提供定義。
// ═══════════════════════════════════════════════════════════════════════════

/*
好的，信安！讓我們繼續 OOP 課程，從第 6 課開始。

---

# 第 6 課：什麼是面向對象？核心思想介紹

## 一、為什麼需要面向對象？

在你學過的 C 語言中，程式是以**函數**為中心組織的——資料和操作資料的函數是分開的。這種方式叫做**程序式程式設計（Procedural Programming）**。

```c
// C 語言的程序式風格
struct Student {
    char name[50];
    int age;
    float gpa;
};

void print_student(struct Student* s) {
    printf("Name: %s, Age: %d, GPA: %.1f\n", s->name, s->age, s->gpa);
}

void set_gpa(struct Student* s, float new_gpa) {
    s->gpa = new_gpa;  // 任何人都可以隨便改資料！
}
```

這有幾個問題：

1. **資料暴露**：`struct` 的所有成員對外完全公開，任何人都能直接修改 `s->gpa = -999.0`，沒有任何保護。
2. **資料和函數分離**：`print_student` 和 `set_gpa` 與 `Student` 之間沒有語言層面的綁定關係，只是「碰巧」操作同一個 struct。
3. **難以擴展**：如果你要區分「大學生」和「研究生」，C 語言沒有原生的繼承機制，你必須手動管理。

**面向對象程式設計（Object-Oriented Programming, OOP）** 就是為了解決這些問題而誕生的。

---

## 二、OOP 的四大核心思想

### 1. 封裝（Encapsulation）

**核心概念**：把資料和操作資料的函數綁在一起，並控制外界的存取權限。

```cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
private:    // 外界不能直接存取
    string name;
    int age;
    float gpa;

public:     // 外界可以使用的介面
    void setGpa(float new_gpa) {
        if (new_gpa >= 0.0f && new_gpa <= 4.0f) {  // 加入驗證！
            gpa = new_gpa;
        } else {
            cout << "錯誤：GPA 必須在 0.0 到 4.0 之間" << endl;
        }
    }

    float getGpa() const {
        return gpa;
    }
};
```

**對比 C 語言**：在 C 中，`s->gpa = -999.0` 完全合法且不會有任何警告。但在 C++ 的封裝下，所有修改必須經過 `setGpa()`，非法值會被攔截。

封裝的本質是：**隱藏實現細節，只暴露必要的介面。**

---

### 2. 繼承（Inheritance）

**核心概念**：新的類別可以從已有的類別「繼承」屬性和行為，實現程式碼重用。

```cpp
#include <iostream>
#include <string>
using namespace std;

// 基類（父類）
class Animal {
public:
    string name;

    void eat() {
        cout << name << " 正在吃東西" << endl;
    }
};

// 派生類（子類）—— 繼承了 Animal 的一切
class Dog : public Animal {
public:
    void bark() {
        cout << name << " 汪汪叫！" << endl;
    }
};

class Cat : public Animal {
public:
    void meow() {
        cout << name << " 喵喵叫！" << endl;
    }
};

int main() {
    Dog d;
    d.name = "旺財";
    d.eat();   // 繼承自 Animal
    d.bark();  // Dog 自己的行為

    Cat c;
    c.name = "咪咪";
    c.eat();   // 繼承自 Animal
    c.meow();  // Cat 自己的行為

    return 0;
}
```

**預期輸出：**
```
旺財 正在吃東西
旺財 汪汪叫！
咪咪 正在吃東西
咪咪 喵喵叫！
```

**在 C 語言中怎麼做？** 你必須手動在 `Dog` 結構體中包含一個 `Animal` 成員，然後手動轉發函數調用——繁瑣且容易出錯。C++ 的繼承讓這一切變得自然。

---

### 3. 多型（Polymorphism）

**核心概念**：同一個介面，不同的實現。透過基類指標或引用，可以調用不同派生類的實現。

```cpp
#include <iostream>
#include <string>
using namespace std;

class Shape {
public:
    virtual double area() {  // virtual = 虛函數，允許派生類覆寫
        return 0;
    }
    virtual ~Shape() {}  // 虛解構函數（後續課程會詳解）
};

class Circle : public Shape {
private:
    double radius;
public:
    Circle(double r) : radius(r) {}

    double area() override {  // 覆寫基類的 area()
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
public:
    Rectangle(double w, double h) : width(w), height(h) {}

    double area() override {
        return width * height;
    }
};

// 這個函數接受任何 Shape！
void printArea(Shape* s) {
    cout << "面積 = " << s->area() << endl;
}

int main() {
    Circle c(5.0);
    Rectangle r(4.0, 6.0);

    printArea(&c);  // 呼叫 Circle::area()
    printArea(&r);  // 呼叫 Rectangle::area()

    return 0;
}
```

**預期輸出：**
```
面積 = 78.5398
面積 = 24
```

**多型的威力**：`printArea` 函數根本不知道傳進來的是圓形還是矩形，但它能「自動」調用正確的 `area()` 實現。這就是多型。

---

### 4. 抽象（Abstraction）

**核心概念**：只展示「是什麼」，隱藏「怎麼做」。讓使用者關注功能而非實現。

```cpp
#include <iostream>
using namespace std;

// 抽象概念：資料庫
class Database {
public:
    virtual void connect() = 0;     // = 0 表示純虛函數，必須由子類實現
    virtual void query(const char* sql) = 0;
    virtual void disconnect() = 0;
    virtual ~Database() {}
};

// 具體實現：MySQL
class MySQL : public Database {
public:
    void connect() override {
        cout << "連接到 MySQL 資料庫..." << endl;
    }
    void query(const char* sql) override {
        cout << "MySQL 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "斷開 MySQL 連接" << endl;
    }
};

// 具體實現：SQLite
class SQLite : public Database {
public:
    void connect() override {
        cout << "打開 SQLite 檔案..." << endl;
    }
    void query(const char* sql) override {
        cout << "SQLite 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "關閉 SQLite 檔案" << endl;
    }
};

// 使用者只需要知道 Database 介面
void doWork(Database* db) {
    db->connect();
    db->query("SELECT * FROM users");
    db->disconnect();
}

int main() {
    MySQL mysql;
    SQLite sqlite;

    cout << "=== 使用 MySQL ===" << endl;
    doWork(&mysql);

    cout << "\n=== 使用 SQLite ===" << endl;
    doWork(&sqlite);

    return 0;
}
```

**預期輸出：**
```
=== 使用 MySQL ===
連接到 MySQL 資料庫...
MySQL 執行: SELECT * FROM users
斷開 MySQL 連接

=== 使用 SQLite ===
打開 SQLite 檔案...
SQLite 執行: SELECT * FROM users
關閉 SQLite 檔案
```

**抽象的意義**：`doWork()` 函數完全不關心底層是 MySQL 還是 SQLite。如果未來要支援 PostgreSQL，只需新增一個繼承 `Database` 的類別，`doWork()` 完全不用修改。

---

## 三、四大思想的關係圖

```
         面向對象程式設計 (OOP)
         ┌──────────────────────┐
         │                      │
    ┌────┴────┐           ┌─────┴─────┐
    │  封裝    │           │   繼承     │
    │Encapsulation│       │Inheritance │
    └────┬────┘           └─────┬─────┘
         │                      │
         │    ┌─────────────────┤
         │    │                 │
    ┌────┴────┴──┐        ┌────┴─────┐
    │   抽象      │        │   多型    │
    │ Abstraction │        │Polymorphism│
    └────────────┘        └──────────┘

封裝 → 保護資料，控制存取
繼承 → 重用程式碼，建立層次
多型 → 同一介面，不同行為（需要繼承作為基礎）
抽象 → 隱藏細節，只暴露介面（結合封裝和繼承）
```

---

## 四、OOP 思維 vs 程序式思維對比

| 面向 | 程序式（C 風格） | 面向對象（C++ 風格） |
|------|------------------|---------------------|
| 組織方式 | 以**函數**為中心 | 以**對象**為中心 |
| 資料保護 | 無，struct 全部公開 | 有，private/protected |
| 程式碼重用 | 函數調用 | 繼承 + 組合 |
| 擴展性 | 修改原有程式碼 | 新增派生類，不動原有程式碼 |
| 現實映射 | 較弱 | 較強（對象模擬真實事物） |

---

## 五、一句話記住四大思想

> **封裝**是把東西裝進盒子，只留一個操作窗口。
> **繼承**是兒子自動擁有父親的能力。
> **多型**是同一個命令，不同的人執行出不同的結果。
> **抽象**是只告訴你「按這個按鈕就行」，不告訴你裡面怎麼運作。

---

## 六、編譯與執行

以上每個範例都可以獨立編譯執行：

```bash
g++ -std=c++17 -o example example.cpp
./example
```

Windows (VS Code) 環境中：
```bash
g++ -std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 -o example.exe example.cpp
./example.exe
```

---

## 本課重點回顧

1. **OOP 誕生原因**：解決程序式程式設計中資料暴露、程式碼重用困難、擴展性差的問題
2. **四大核心**：封裝、繼承、多型、抽象
3. **封裝**：資料 + 方法綁定，存取控制
4. **繼承**：程式碼重用，建立類別層次
5. **多型**：相同介面、不同實現，運行時決定行為
6. **抽象**：隱藏實現細節，只暴露介面

---

這就是 OOP 的全貌鳥瞰。從下一課開始，我們會一步步深入每一個概念。

準備好進入 **第 7 課：類別（class）的定義與語法** 了嗎？
*/



#include <iostream>
using namespace std;

// 抽象概念：資料庫
class Database {
public:
    virtual void connect() = 0;     // = 0 表示純虛函數，必須由子類實現
    virtual void query(const char* sql) = 0;
    virtual void disconnect() = 0;
    virtual ~Database() {}
};

// 具體實現：MySQL
class MySQL : public Database {
public:
    void connect() override {
        cout << "連接到 MySQL 資料庫..." << endl;
    }
    void query(const char* sql) override {
        cout << "MySQL 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "斷開 MySQL 連接" << endl;
    }
};

// 具體實現：SQLite
class SQLite : public Database {
public:
    void connect() override {
        cout << "打開 SQLite 檔案..." << endl;
    }
    void query(const char* sql) override {
        cout << "SQLite 執行: " << sql << endl;
    }
    void disconnect() override {
        cout << "關閉 SQLite 檔案" << endl;
    }
};

// 使用者只需要知道 Database 介面
void doWork(Database* db) {
    db->connect();
    db->query("SELECT * FROM users");
    db->disconnect();
}

int main() {
    MySQL mysql;
    SQLite sqlite;

    cout << "=== 使用 MySQL ===" << endl;
    doWork(&mysql);

    cout << "\n=== 使用 SQLite ===" << endl;
    doWork(&sqlite);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 6 課：什麼是面向對象？核心思想介紹3.cpp" -o oo3

// 註 1:本檔前 364 行是 /* */ 註解區塊（本課講義），
//      其中的 class 與 int main() 都【不會被編譯】。
//      真正的程式從第 372 行的 class Database 開始。
//
// 註 2:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址。
//
// 註 3:同一個 doWork(Database*) 函式產生兩組完全不同的輸出，
//      而它的程式碼裡沒有任何 if 或型別判斷 ——
//      這就是抽象與多型的價值：新增 PostgreSQL 時 doWork 一行都不用改。
//
// 註 4:本機實測物件大小（實作定義，x86-64 / g++ 15.2.0）：
//      sizeof(Database) = sizeof(MySQL) = sizeof(SQLite) = 8，
//      三者都只含一個 vptr、沒有資料成員。
//
// 【LeetCode 實戰範例】從缺 —— 理由：
//      本檔主題是「純虛擬介面與依賴反轉」。可選清單中的設計題
//      （146 LRU Cache、155 Min Stack、705 Design HashSet、
//        707 Design Linked List、1603 Design Parking System、
//        1656 Design an Ordered Stream）都只要求實作【單一具體類別】，
//      評測系統也不會去驗證是否使用了抽象介面。
//      硬加一題會讓「介面與實作分離」這個重點被淹沒。

// === 預期輸出 ===
// === 使用 MySQL ===
// 連接到 MySQL 資料庫...
// MySQL 執行: SELECT * FROM users
// 斷開 MySQL 連接
//
// === 使用 SQLite ===
// 打開 SQLite 檔案...
// SQLite 執行: SELECT * FROM users
// 關閉 SQLite 檔案
