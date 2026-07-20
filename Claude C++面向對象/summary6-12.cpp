/*
 * ================================================================
 *  【第 6～12 課總複習】summary6-12.cpp
 * ================================================================
 *  涵蓋課程：
 *    第  6 課：什麼是面向對象？核心思想介紹（封裝/繼承/多型/抽象）
 *    第  7 課：類別（class）的定義與語法
 *    第  8 課：對象（object）的創建與使用
 *    第  9 課：成員變數（Member Variables）
 *    第 10 課：成員函數（Member Functions）
 *    第 11 課：存取修飾符——public、private、protected
 *    第 12 課：struct 與 class 的差異
 *
 *  編譯：g++ -std=c++17 -Wall -Wextra -o summary6-12 summary6-12.cpp
 * ================================================================
 */

// =============================================================================
// 【主題資訊 Information】
// -----------------------------------------------------------------------------
//   範圍：    OOP 的核心思想（第 6 課）→ class 語法（第 7 課）
//             → 物件的建立與生命週期（第 8 課）→ 成員變數（第 9 課）
//             → 成員函數（第 10 課）→ 存取控制（第 11 課）
//             → struct vs class（第 12 課）
//   標準：    本檔以 C++17 編譯。其中「類內成員初始化」（int hp = 100;）
//             需 C++11 起；override 關鍵字亦為 C++11。
//   標頭檔：  <iostream>、<string>、<vector>、<cmath>、<cstdint>
//   一句話：  這七課合起來回答一個問題 ——
//             **「怎麼把『資料』與『操作那份資料的程式碼』綁在一起，並保證它始終正確？」**
//
// =============================================================================
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. OOP 四大支柱不是四個並列的特性，而是一條依賴鏈（第 6 課）】
//   多數教材把封裝／繼承／多型／抽象並列，但它們其實有先後：
//     封裝（Encapsulation）→ 先讓「狀態」有人看守，型別才有可信的不變量；
//     繼承（Inheritance）  → 在可信的型別之上建立「is-a」關係，共用實作；
//     多型（Polymorphism） → 有了共同基底，才能用同一介面操作不同實作；
//     抽象（Abstraction）  → 把介面抽到純虛擬，讓呼叫端只依賴「能做什麼」。
//   ★ 關鍵：**多型必須透過指標或引用**。
//     用值傳遞會發生 object slicing（物件切片）——
//     衍生部分被硬生生切掉，只留下基底子物件，virtual 也就失效了。
//     這是初學者最常踩、而且不會有編譯錯誤的坑。
//
// 【2. class 是型別、object 是實體（第 7、8 課）】
//   class 只是「設計圖」，不佔執行期空間；object 才是依圖產生的實體。
//   由此推出兩個常被混淆的事實：
//     (a) 成員**函數**不佔物件大小 —— 所有物件共用同一份機器碼，
//         靠隱含的 this 參數區分「現在操作的是哪一個物件」。
//     (b) 物件大小只由**非靜態資料成員 + 對齊填充 +（若有 virtual）vptr** 決定。
//   本檔實測（x86-64 Linux，**實作定義值**）：
//     sizeof(Empty_L8)    = 1   ← 空類別也必須佔 1 byte
//     sizeof(WithData_L8) = 12  ← char + int + char，因對齊而填充
//
// 【3. 為什麼空類別的 sizeof 是 1 而不是 0】
//   因為 C++ 要求**同時存活的相異物件必須有相異位址**（pointer identity）。
//   若可以是 0，陣列裡相鄰兩個元素就會落在同一個位址，
//   指標比較與 &arr[i] 全部失去意義。所以標準規定完整物件至少 1 byte。
//   ★ 例外：空的**基底子物件**可與衍生物件共用位址
//     （EBO，empty base optimization）—— 但那不是獨立的完整物件。
//
// 【4. 記憶體對齊：為什麼 char+int+char 是 12 而不是 6（第 8、9 課）】
//   CPU 讀取未對齊的資料需要多次匯流排存取（某些架構甚至直接錯誤），
//   所以編譯器會插入 padding 讓每個成員落在其對齊要求的邊界上：
//       char a;    // offset 0
//       // 3 bytes padding
//       int  b;    // offset 4（int 需 4-byte 對齊）
//       char c;    // offset 8
//       // 3 bytes padding（讓整體大小是最大對齊值 4 的倍數，陣列才排得整齊）
//   ★ 實務推論：**把大成員排在前面、小成員集中在後**可減少填充。
//     把上面改成 int, char, char 就只要 8 bytes。
//     這在有數百萬個物件時是真實的記憶體節省。
//
// 【5. 成員變數的初始化（第 9 課）—— 最容易產生 UB 的地方】
//   基本型別（int/double/bool/char）若沒有初始器，
//   在預設初始化下其值是**不確定的**（indeterminate value），讀取它是未定義行為 ——
//   不保證是 0、不保證每次執行相同、也不保證會崩潰。
//   class 型別（string/vector）則會呼叫其預設建構函數，天然安全。
//   ★ 解法只有一個：**一律使用類內成員初始化**（C++11）：
//       int hp = 100;   string fuel = "汽油";
//   這比在每個建構函數裡逐一賦值更不容易漏，也讓預設值集中在一處。
//
// 【6. 初始化順序永遠依「宣告順序」（第 9 課的隱形陷阱）】
//   成員的初始化順序由**宣告順序**決定，與初始化列的書寫順序**無關**。
//   所以這段程式碼是錯的：
//       class Buf {
//           char*  m_data;      // 先宣告 → 先初始化
//           size_t m_len;       // 後宣告 → 後初始化
//           Buf(const char* s)
//             : m_len(strlen(s)), m_data(new char[m_len + 1]) {}  // ❌
//       };
//   m_data 會先被初始化，此時 m_len **尚未賦值**，
//   於是用一個不確定的長度去配置記憶體。
//   正確做法是讓**宣告順序**與依賴順序一致（長度在前、指標在後）。
//   GCC 的 -Wreorder 會提醒初始化列順序與宣告順序不符，務必開啟。
//
// 【7. 存取控制保護的是「不變量」，不是「機密」（第 11 課）】
//   private 的價值不在安全（它是純編譯期檢查，沒有執行期成本，
//   用指標算術照樣讀得到），而在兩件事：
//     (a) 讓「修改狀態的所有途徑」都經過你寫的檢查 → 不變量得以強制；
//     (b) 讓內部表示可以自由演進 —— private 成員改名／改型別／刪除，
//         外界零感知；public 成員一旦公開就成了契約，改動即 breaking change。
//   ★ 注意：**空殼 setter 等於 public**。
//     判準是「這個 setter 有沒有可能拒絕呼叫端」——
//     本檔 Student 的 setGpa 會拒絕 -1，那才是真的在保護不變量。
//
// 【8. struct 與 class 只差兩個預設值（第 12 課）】
//     (a) 成員預設存取權：struct = public，class = private；
//     (b) 繼承預設方式：  struct D : B 是 public 繼承，
//                        class  D : B 是 **private** 繼承。
//   其餘完全相同 —— struct 一樣能有建構函式、virtual、繼承、private 成員。
//   ★ (b) 比 (a) 危險得多：成員寫錯會立刻編譯錯誤，
//     但繼承方式寫錯只會讓多型悄悄失效（D* 無法轉成 B*），
//     錯誤訊息還離現場很遠。**永遠顯式寫出繼承方式。**
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
// (A) virtual 的執行期機制與成本
//   有 virtual 函式的類別，每個物件會多一個隱藏的 **vptr**（指向該類別的 vtable）。
//   vtable 是一張函式指標表，執行期依 vptr 找到實際該呼叫的版本。
//   成本是：每物件多一個指標的空間 + 每次呼叫多一次間接跳轉，
//   且通常無法被 inline。這符合 C++ 的零開銷原則 ——
//   **不用 virtual 就完全不付這筆錢**（沒有 virtual 的類別沒有 vptr）。
//
// (B) object slicing（物件切片）為什麼無聲無息
//       void print(Shape s);          // ❌ 傳值
//       Circle c; print(c);           // Circle 的部分被切掉，只剩 Shape
//   這會編譯成功、不會警告、也不會崩潰，只是行為悄悄變成基底版本。
//   正解是傳引用或指標：void print(const Shape& s);
//   這也是為什麼多型的介面幾乎一律用 const T& 或智慧指標。
//
// (C) 組合（composition）優先於繼承
//   本檔第 9 課的 Car_L9 內含 Engine_L9，是 **has-a** 關係。
//   繼承表達的是 **is-a**。實務上「組合優先於繼承」是主流建議，
//   因為繼承會把基底的實作細節暴露給衍生類別、耦合極強，
//   而且一旦繼承鏈變深就難以理解與修改。
//   判準：問「X 真的是一種 Y 嗎？」——
//   汽車不是一種引擎，所以該用組合而非繼承。
//
// (D) 存取控制是「以類別為單位」，不是「以物件為單位」
//   同一個類別的成員函式，可以存取**另一個同類別物件**的 private 成員：
//       bool Account::richerThan(const Account& o) const { return m_bal > o.m_bal; }
//   完全合法。因為存取檢查在編譯期以類別為單位進行。
//   若非如此，operator==、拷貝建構函數這類必須讀取另一個物件內部的操作
//   根本寫不出來。本檔第 11 課的「陳信安 vs 小明」比較正是這個機制。
//
// =============================================================================
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. **多型必須用指標或引用**；傳值會 object slicing，且無編譯錯誤或警告。
//  2. 基本型別成員未初始化就讀取是 UB，值不確定 —— 一律用類內初始化。
//  3. 成員初始化順序依**宣告順序**，與初始化列書寫順序無關（開 -Wreorder）。
//  4. sizeof 與對齊填充是**實作定義**的；本檔數值為 x86-64 Linux 實測。
//  5. 空類別 sizeof 為 1（保證物件位址相異），不是 0。
//  6. 沒有驗證邏輯的 setter 等同 public，別為了「看起來封裝」而寫。
//  7. private 不是安全機制，只是編譯期的誤用防護。
//  8. 繼承時**永遠顯式寫** public/private —— class D : B 是 private 繼承。
//  9. 有 virtual 函式的基底類別，其解構函數**必須是 virtual**，
//     否則透過基底指標 delete 衍生物件是 UB（資源可能不被釋放）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】第 6～12 課（OOP 核心、類別與物件、封裝）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼多型一定要透過指標或引用？傳值會怎樣？
//     答：因為傳值會發生 **object slicing** —— 衍生類別多出來的部分
//         被硬生生切掉，只留下基底子物件，vptr 也被換成基底的，
//         於是 virtual 分派失效，永遠呼叫到基底版本。
//         最危險的是它**不會有編譯錯誤，也不會有警告**，
//         程式照跑、行為卻悄悄錯了。
//     追問：那該怎麼寫？
//         → 介面一律用 const Base& 或 Base*（或智慧指標）。
//           想禁止切片可把基底的拷貝建構函數設為 protected 或 deleted。
//
// 🔥 Q2. sizeof 一個空類別是多少？為什麼不是 0？
//     答：是 **1**（本機實測）。因為 C++ 要求同時存活的相異物件
//         必須有相異位址；若允許 0，陣列中相鄰元素就會同址，
//         指標比較與 &arr[i] 全部失效。
//     追問：那有沒有例外？
//         → 有，empty base optimization：空的**基底子物件**
//           可與衍生物件共用位址，因為它不是獨立的完整物件。
//           這正是 std::allocator 這類空類別當基底時不增加大小的原因。
//
// 🔥 Q3. 成員的初始化順序由什麼決定？
//     答：由**宣告順序**決定，與建構函數初始化列的書寫順序**完全無關**。
//         所以若寫 : m_len(strlen(s)), m_data(new char[m_len + 1])
//         而 m_data 宣告在 m_len 之前，m_data 會先初始化，
//         此時 m_len 尚未賦值 → 用不確定的值配置記憶體。
//     追問：怎麼避免？
//         → 讓宣告順序與依賴順序一致，並開啟 -Wreorder，
//           它會在初始化列順序與宣告順序不符時警告。
//
// ⚠️ 陷阱 1. class Derived : Base { }; 和 struct Derived : Base { }; 差在哪？
//     答：前者是 **private 繼承**，後者是 **public 繼承**。
//         private 繼承表達「用 Base 實作 Derived」而非「Derived is-a Base」，
//         因此 Derived* 無法隱式轉成 Base*，多型完全失效。
//     為什麼會錯：大家只記得「struct 預設 public、class 預設 private」
//         是講**成員**，忘了同一條規則也套用在**繼承**上。
//         而這個錯誤的症狀（無法轉型）出現的位置往往離繼承宣告很遠。
//
// ⚠️ 陷阱 2. 把所有成員設 private、每個都配一組 getter/setter，就是好的封裝嗎？
//     答：**不是**。若 setter 只是 { m_x = x; } 這種空殼，
//         它與 public 完全等價，不變量一樣沒被保護，
//         還讓程式碼多了一倍。真正的判準是
//         「這個 setter 有沒有可能**拒絕**呼叫端」。
//         更好的設計是不暴露欄位而暴露**操作**：
//         用 deposit()／withdraw() 取代 setBalance()。
//     為什麼會錯：把「封裝」誤解成一種寫法上的儀式，
//         而非「保護不變量」這個目的。
//         純資料聚合（如本檔的 RGB、Config）根本不需要 getter/setter，
//         直接用 struct 公開成員反而更清楚。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
using namespace std;

// ================================================================
// 第 6 課：OOP 四大支柱
// ================================================================
//
// 【封裝 Encapsulation】資料+函數綁在 class，private/public 控制存取
// 【繼承 Inheritance】  子類繼承父類成員，class 子 : public 父 {}
// 【多型 Polymorphism】 virtual + override，同一介面不同行為（需指標/引用）
// 【抽象 Abstraction】  純虛函數 = 0，使類別不可實例化，子類必須實作
// ================================================================

// --- 封裝：private 資料 + public 介面 + 驗證 ---
class Student_L6 {
    string name_;
    float gpa_ = 0.0f;
public:
    Student_L6(const string& n, float g) : name_(n) { setGpa(g); }
    void setGpa(float g) {
        if (g >= 0.0f && g <= 4.0f) gpa_ = g;
        else cout << "  [拒絕] GPA 需 0~4，收到: " << g << "\n";
    }
    void print() const { cout << "  " << name_ << " GPA=" << gpa_ << "\n"; }
};

// --- 繼承：子類繼承父類，並新增自己的行為 ---
class Animal_L6 {
public:
    string name;
    Animal_L6(const string& n) : name(n) {}
    void eat() { cout << "  " << name << " 正在吃東西\n"; }
};
class Dog_L6 : public Animal_L6 {
public:
    Dog_L6(const string& n) : Animal_L6(n) {}  // 呼叫父類建構函數
    void bark() { cout << "  " << name << " 汪汪！\n"; }
};

// --- 多型：virtual + override，透過基類指標呼叫正確的子類函數 ---
class Shape_L6 {
public:
    virtual double area() const { return 0; }
    virtual void describe() const { cout << "  形狀，面積=" << area() << "\n"; }
    virtual ~Shape_L6() {}   // 虛解構函數：確保透過基類指標刪除時正確析構
};
class Circle_L6 : public Shape_L6 {
    double r_;
public:
    Circle_L6(double r) : r_(r) {}
    double area() const override { return 3.14159 * r_ * r_; }
    void describe() const override { cout << "  圓形 r=" << r_ << " 面積=" << area() << "\n"; }
};
class Rect_L6 : public Shape_L6 {
    double w_, h_;
public:
    Rect_L6(double w, double h) : w_(w), h_(h) {}
    double area() const override { return w_ * h_; }
    void describe() const override { cout << "  矩形 " << w_ << "x" << h_ << " 面積=" << area() << "\n"; }
};

// --- 抽象：純虛函數 = 0，不可實例化 ---
class Vehicle_L6 {
public:
    string brand;
    Vehicle_L6(const string& b) : brand(b) {}
    virtual void start() = 0;   // 純虛函數：子類必須實作
    virtual ~Vehicle_L6() {}
};
class Car_L6 : public Vehicle_L6 {
public:
    Car_L6(const string& b) : Vehicle_L6(b) {}
    void start() override { cout << "  " << brand << " 引擎發動！\n"; }
};

// ================================================================
// 第 7 課：class 的定義與語法
// ================================================================
//
// 【語法】class Name { public: / private: / protected: ... };  ← 別忘分號！
// 【藍圖】class = 設計圖，object = 依圖建出的實體
// 【命名】PascalCase（如 BankAccount）
// 【預設】class 內未標存取修飾符 → private
// 【類內定義】短函數直接寫在 class {}（隱式 inline）
// 【類外定義】長函數只在 class 宣告，外面用 ClassName::funcName 實作
// 【類內初始化】C++11 起可在宣告時給預設值：int hp = 100;
// ================================================================

class Calc_L7 {
public:
    int add(int a, int b) { return a + b; }   // 類內定義
    int subtract(int a, int b);               // 類內只宣告
};
// 類外定義：必須加 Calc_L7:: 前綴
int Calc_L7::subtract(int a, int b) { return a - b; }

// ================================================================
// 第 8 課：對象的創建與使用
// ================================================================
//
// 【棧上創建】Dog d;         → 用 . 存取，離開作用域自動銷毀（最常用）
// 【堆上創建】Dog* p = new Dog(); → 用 -> 存取，必須 delete，否則記憶體洩漏
// 【每個對象獨立】a.count 和 b.count 互不影響
// 【sizeof】只計成員變數（含對齊填充），空類別 = 1 byte，函數不佔空間
// 【參數傳遞】
//   傳值       void f(Box b)        — 複製，少用
//   傳引用     void f(Box& b)       — 可修改原對象
//   傳const引用 void f(const Box& b) — 最推薦！不複製、不可改
// ================================================================

class Counter_L8 {
public:
    int count = 0;
    void increment() { count++; }
};

class Empty_L8 {};   // sizeof = 1

class WithData_L8 {
public:
    char a;    // 1 byte
    int  b;    // 4 bytes（前面 3 bytes 填充對齊）
    char c;    // 1 byte（後面 3 bytes 填充對齊）
    // 典型 sizeof = 12
};

// ================================================================
// 第 9 課：成員變數（Member Variables）
// ================================================================
//
// 【基本型別】int, double, bool, char → 不自動初始化，是垃圾值！必須手動給值
// 【class 型別】string, vector 等 → 有預設建構函數，自動初始化（安全）
// 【組合 Composition】類別包含另一個類別對象 → has-a 關係（如 Car 有 Engine）
// 【固定陣列】double data[2][2]; 可作成員，C++11 可類內初始化 = {}
// 【最佳實踐】一律使用類內初始化（C++11）給所有基本型別預設值
// ================================================================

class Engine_L9 {
public:
    int hp = 0;
    string fuel = "汽油";
    void start() { cout << "  " << hp << "hp " << fuel << " 引擎啟動\n"; }
};
class Car_L9 {
public:
    string brand;
    Engine_L9 engine;   // 組合：Car「擁有」Engine（has-a）
    void drive() { cout << "  " << brand << " → "; engine.start(); }
};

// ================================================================
// 第 10 課：成員函數（Member Functions）
// ================================================================
//
// 【類內 vs 類外】短函數放類內（inline），長函數類外定義（ClassName::func）
// 【鏈式調用】返回 *this 引用 → obj.a().b().c()
// 【函數重載】同名不同參數（型別/數量），只有返回值不同不算！
// 【預設參數】從右往左設定，呼叫時從右往左省略
// 【成員函數互調】直接呼叫，不需前綴（編譯器用 this-> 處理）
// 【隱藏 this】每個成員函數都有隱藏的 this 指標指向呼叫者
//   d1.bark() → 編譯器: Dog::bark(&d1)，this == &d1
// ================================================================

class Builder_L10 {
public:
    string text;
    // 鏈式調用：返回 *this
    Builder_L10& add(const string& s) { text += s; return *this; }
};

class Printer_L10 {
public:
    // 函數重載：同名不同參數
    void print(int v)           { cout << "  int: " << v << "\n"; }
    void print(double v)        { cout << "  double: " << v << "\n"; }
    void print(const string& v) { cout << "  string: " << v << "\n"; }
};

class Logger_L10 {
public:
    // 預設參數：從右側開始
    void log(const string& msg, const string& level = "INFO") {
        cout << "  [" << level << "] " << msg << "\n";
    }
};

// ================================================================
// 第 11 課：存取修飾符 public / private / protected
// ================================================================
//
// 【存取權限表】
//   存取者          public  protected  private
//   類別自己           V        V         V
//   子類               V        V         X
//   外部(main等)       V        X         X
//
// 【class 預設 private，struct 預設 public】（唯一語法差異）
// 【同類別不同對象】可互訪 private（private 是類別層級，非對象層級）
// 【最小權限原則】成員變數幾乎總是 private，透過 public getter/setter 存取
// ================================================================

class BankAcct_L11 {
    string owner_;
    double balance_ = 0;
    int txCount_ = 0;
    void record(const string& type, double amt) {   // private 輔助函數
        txCount_++;
        cout << "  [#" << txCount_ << "] " << type << " $" << amt << " → $" << balance_ << "\n";
    }
public:
    BankAcct_L11(const string& o, double init) : owner_(o), balance_(init > 0 ? init : 0) {}
    bool deposit(double amt) {
        if (amt <= 0) { cout << "  錯誤：金額需>0\n"; return false; }
        balance_ += amt; record("存款", amt); return true;
    }
    bool withdraw(double amt) {
        if (amt <= 0 || amt > balance_) { cout << "  錯誤：金額無效或餘額不足\n"; return false; }
        balance_ -= amt; record("提款", amt); return true;
    }
    double getBalance() const { return balance_; }
    void display() const {
        cout << "  " << owner_ << " 餘額=$" << balance_ << " 交易" << txCount_ << "筆\n";
    }
    // 同類別對象互訪 private（private 是類別層級）
    bool hasMore(const BankAcct_L11& other) const { return balance_ > other.balance_; }
};

// ================================================================
// 第 12 課：struct 與 class 的差異
// ================================================================
//
// 【唯一語法差異】預設存取：struct=public，class=private
// 【繼承預設】     struct 繼承預設 public，class 繼承預設 private
// 【功能完全相同】 struct 也能有 private、virtual、建構/解構函數
// 【慣例】
//   struct → 純資料（POD）、所有成員 public、無或少行為
//   class  → 有封裝（private資料+public介面）、完整 OOP
//
// 【對照表】
//   特性             struct      class
//   預設存取         public      private
//   預設繼承         public      private
//   可有 private     V           V
//   可有 virtual     V           V
//   慣例用途         POD 資料    OOP 設計
// ================================================================

struct RGB_L12 { uint8_t r, g, b; };             // struct 典型：純資料
struct Config_L12 { string host; int port; };     // struct 典型：設定資料

// struct 也能做完整 OOP（但慣例上用 class）
struct Point_L12 {
private:
    double x_, y_;
public:
    Point_L12(double x = 0, double y = 0) : x_(x), y_(y) {}
    double x() const { return x_; }
    double y() const { return y_; }
    void print() const { cout << "  Point(" << x_ << "," << y_ << ")\n"; }
};

// ================================================================
// 主程式
// ================================================================

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 707. Design Linked List
//   題目：自行設計一個單向鏈結串列，支援 get / addAtHead / addAtTail /
//         addAtIndex / deleteAtIndex。
//   為什麼用到本主題：這題把第 6～12 課幾乎全部用上了 ——
//     - 第 7、8 課：class 定義、物件在堆積上的建立與釋放（new/delete）
//     - 第  9 課：成員變數的類內初始化（Node* next = nullptr）—— 少了它就是 UB
//     - 第 10 課：成員函數彼此呼叫（addAtHead/addAtTail 都轉呼 addAtIndex）
//     - 第 11 課：m_head / m_size 為 private，外界無法把 size 改成與實際不符
//     - 第 12 課：Node 是純資料聚合 → 用 struct；List 有不變量 → 用 class
//   不變量：m_size 永遠等於實際節點數；索引超出範圍的操作一律安全地被忽略。
//   複雜度：get / addAtIndex / deleteAtIndex 皆 O(n)；addAtHead O(1)。
// -----------------------------------------------------------------------------
class MyLinkedList {
    // Node 是純資料聚合（沒有不變量要維護）→ 用 struct
    // ★ next 一定要類內初始化，否則是不確定值，走訪時就是 UB
    struct Node {
        int   val  = 0;
        Node* next = nullptr;
    };

    Node* m_head = nullptr;   // private：外界無法直接接管指標
    int   m_size = 0;         // 不變量：永遠等於實際節點數

public:
    MyLinkedList() = default;

    // 有動態資源 → 必須自訂解構函數把它還回去（第 30 課「三法則」的前奏）
    ~MyLinkedList() {
        while (m_head) {
            Node* nxt = m_head->next;
            delete m_head;
            m_head = nxt;
        }
    }

    // 本例僅供示範，禁止複製以免淺拷貝造成重複釋放
    MyLinkedList(const MyLinkedList&)            = delete;
    MyLinkedList& operator=(const MyLinkedList&) = delete;

    int get(int index) const {
        if (index < 0 || index >= m_size) return -1;   // 越界安全回 -1
        const Node* cur = m_head;
        for (int i = 0; i < index; ++i) cur = cur->next;
        return cur->val;
    }

    void addAtIndex(int index, int val) {
        if (index > m_size) return;      // 超過長度：題目規定不插入
        if (index < 0) index = 0;        // 負索引：視為插在頭部

        Node* node = new Node{val, nullptr};
        if (index == 0) {
            node->next = m_head;
            m_head = node;
        } else {
            Node* prev = m_head;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            node->next = prev->next;
            prev->next = node;
        }
        ++m_size;                        // 與實際節點數同步 → 維持不變量
    }

    // 成員函數呼叫成員函數，避免重複實作插入邏輯
    void addAtHead(int val) { addAtIndex(0, val); }
    void addAtTail(int val) { addAtIndex(m_size, val); }

    void deleteAtIndex(int index) {
        if (index < 0 || index >= m_size) return;
        Node* victim = nullptr;
        if (index == 0) {
            victim = m_head;
            m_head = m_head->next;
        } else {
            Node* prev = m_head;
            for (int i = 0; i < index - 1; ++i) prev = prev->next;
            victim = prev->next;
            prev->next = victim->next;
        }
        delete victim;
        --m_size;                        // 同步遞減，不變量不被破壞
    }

    int size() const { return m_size; }

    string toString() const {
        string out;
        for (const Node* cur = m_head; cur; cur = cur->next) {
            if (!out.empty()) out += " -> ";
            out += to_string(cur->val);
        }
        return out.empty() ? "(空)" : out;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】任務排程器：七課概念在一個真實元件裡的分工
//   情境：背景服務要維護一組待執行的任務，依優先度取出下一個來跑，
//         並統計完成率。這是 worker / job queue 最典型的骨架。
//   為什麼用到本主題：這個元件剛好把七課的判斷都用上一次：
//     - Task 是**純資料聚合**（欄位彼此獨立、任意組合都合法）→ **struct**（第 12 課）
//     - Scheduler 有**不變量**（已完成數不可超過總數）→ **class**（第 11、12 課）
//     - 資料成員全部 private 且**類內初始化**（第 9 課），杜絕未初始化 UB
//     - 對外只暴露**操作**（submit / runNext / stats），不暴露欄位（第 11 課）
//     - runNext() 呼叫私有的 pickHighest()（第 10 課：成員函數互相呼叫）
//     - 唯讀查詢一律標 const，讓 const 引用參數也能使用（第 10 課）
// -----------------------------------------------------------------------------
struct Task {                    // 純資料聚合 → struct
    string name;
    int    priority = 0;         // 數字越大越優先
    bool   done     = false;
};

class Scheduler {                // 有不變量 → class
public:
    void submit(const string& name, int priority) {
        if (name.empty()) return;                  // 拒絕無效任務
        m_tasks.push_back(Task{name, priority, false});
    }

    // 取出優先度最高的未完成任務並標記完成；沒有可執行任務就回 false
    bool runNext(string& outName) {
        int idx = pickHighest();                   // 呼叫私有輔助函數
        if (idx < 0) return false;
        m_tasks[static_cast<size_t>(idx)].done = true;
        ++m_completed;                             // 與 done 標記同步
        outName = m_tasks[static_cast<size_t>(idx)].name;
        return true;
    }

    // 唯讀查詢，全部 const
    int  total()     const { return static_cast<int>(m_tasks.size()); }
    int  completed() const { return m_completed; }
    int  pending()   const { return total() - m_completed; }

    double completionRate() const {
        return total() == 0 ? 0.0 : 100.0 * m_completed / total();
    }

private:
    // 私有輔助函數：外界不需要、也不該知道挑選策略怎麼實作
    int pickHighest() const {
        int best = -1;
        for (size_t i = 0; i < m_tasks.size(); ++i) {
            if (m_tasks[i].done) continue;
            if (best < 0 || m_tasks[i].priority > m_tasks[static_cast<size_t>(best)].priority) {
                best = static_cast<int>(i);
            }
        }
        return best;
    }

    vector<Task> m_tasks;         // 全部類內初始化，無 UB 風險
    int          m_completed = 0; // 不變量：<= m_tasks.size()
};

int main() {
    cout << "================================================================\n";
    cout << "  C++ 面向對象 第 6～12 課 總複習\n";
    cout << "================================================================\n";

    // ============================================================
    // 第 6 課：OOP 四大支柱
    // ============================================================
    cout << "\n=== 第 6 課：OOP 四大支柱 ===\n";

    // 封裝
    cout << "\n[封裝] private 資料 + setter 驗證\n";
    Student_L6 s("張三", 3.8f);
    s.print();
    s.setGpa(-1.0f);   // 被攔截
    s.setGpa(3.9f);
    s.print();

    // 繼承
    cout << "\n[繼承] 子類繼承父類成員\n";
    Dog_L6 dog("旺財");
    dog.eat();    // 繼承自 Animal
    dog.bark();   // Dog 自己的

    // 多型
    cout << "\n[多型] virtual + override，透過基類指標動態分派\n";
    vector<Shape_L6*> shapes = { new Circle_L6(5), new Rect_L6(4, 6) };
    for (auto* sp : shapes) sp->describe();    // 動態呼叫正確版本
    for (auto* sp : shapes) delete sp;         // 虛解構函數確保正確釋放

    // 抽象
    cout << "\n[抽象] 純虛函數 = 0，子類必須實作\n";
    // Vehicle_L6 v("X");  // 編譯錯誤！抽象類別不可實例化
    Car_L6 car("Toyota");
    car.start();

    // ============================================================
    // 第 7 課：class 定義與語法
    // ============================================================
    cout << "\n=== 第 7 課：class 定義與語法 ===\n";

    // 類內 vs 類外定義
    Calc_L7 calc;
    cout << "  add(5,3)=" << calc.add(5, 3)             // 類內定義
         << " subtract(10,4)=" << calc.subtract(10, 4)  // 類外定義
         << "\n";

    // class 預設 private
    // class Foo { int x; };  → x 是 private，外部不能直接存取

    // 類內初始化（C++11）
    // class Player { int hp = 100; };  → 建立即有預設值，避免垃圾值

    // ============================================================
    // 第 8 課：對象的創建與使用
    // ============================================================
    cout << "\n=== 第 8 課：對象創建與使用 ===\n";

    // 棧 vs 堆
    Counter_L8 ca, cb;                 // 棧上：自動管理
    ca.increment(); ca.increment();
    cb.increment();
    cout << "  [棧] ca.count=" << ca.count << " cb.count=" << cb.count << " (獨立)\n";

    Counter_L8* cp = new Counter_L8(); // 堆上：手動管理
    cp->increment();                    // -> 存取
    cout << "  [堆] cp->count=" << cp->count << "\n";
    delete cp; cp = nullptr;            // 必須 delete！

    // sizeof
    cout << "  sizeof(Empty)=" << sizeof(Empty_L8) << " (空類=1byte)\n";
    cout << "  sizeof(WithData char+int+char)=" << sizeof(WithData_L8) << " (含對齊填充)\n";

    // 參數傳遞（概念說明）
    cout << "  [參數] 傳值=複製，傳引用=可改，const引用=最推薦\n";

    // ============================================================
    // 第 9 課：成員變數
    // ============================================================
    cout << "\n=== 第 9 課：成員變數 ===\n";

    // 基本型別不自動初始化（危險！）
    cout << "  int/double/bool/char → 不自動初始化 → 垃圾值！\n";
    cout << "  string/vector       → 自動初始化   → 安全\n";

    // 組合（Composition）
    Car_L9 myCar;
    myCar.brand = "Honda";
    myCar.engine.hp = 120;
    myCar.engine.fuel = "汽油";
    myCar.drive();   // car.engine.start() → 鏈式 . 存取內層對象

    // 類內初始化
    cout << "  [最佳] C++11 類內初始化：int hp = 100; 直接給預設值\n";

    // ============================================================
    // 第 10 課：成員函數
    // ============================================================
    cout << "\n=== 第 10 課：成員函數 ===\n";

    // 鏈式調用（return *this）
    Builder_L10 b;
    b.add("C++").add(" is").add(" awesome!");
    cout << "  [鏈式] " << b.text << "\n";

    // 函數重載
    cout << "  [重載] 同名不同參數：\n";
    Printer_L10 pr;
    pr.print(42);       // int 版
    pr.print(3.14);     // double 版
    pr.print("Hello");  // string 版

    // 預設參數
    cout << "  [預設參數]\n";
    Logger_L10 lg;
    lg.log("程式啟動");              // level = "INFO"（預設）
    lg.log("連線失敗", "ERROR");    // 指定 level

    // this 指標
    cout << "  [this] 成員函數有隱藏 this 指標 → d.bark() 等同 Dog::bark(&d)\n";

    // ============================================================
    // 第 11 課：存取修飾符
    // ============================================================
    cout << "\n=== 第 11 課：存取修飾符 ===\n";

    // 完整封裝的銀行帳戶
    BankAcct_L11 acc("陳信安", 5000);
    acc.display();
    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(99999);   // 餘額不足 → 被攔截
    // acc.balance_ = 0;   // 編譯錯誤！private
    acc.display();

    // 同類別對象互訪 private
    BankAcct_L11 acc2("小明", 8000);
    cout << "  陳信安 vs 小明: " << (acc.hasMore(acc2) ? "陳信安多" : "小明多") << "\n";

    // 權限表速查
    cout << "\n  [權限表]\n";
    cout << "  public:    類別/子類/外部 都可存取\n";
    cout << "  protected: 類別/子類 可存取，外部不行\n";
    cout << "  private:   只有類別自己可存取\n";
    cout << "  → 成員變數幾乎總是 private（最小權限原則）\n";

    // ============================================================
    // 第 12 課：struct vs class
    // ============================================================
    cout << "\n=== 第 12 課：struct vs class ===\n";

    // struct 預設 public
    RGB_L12 red = {255, 0, 0};
    cout << "  [struct] RGB=(" << (int)red.r << "," << (int)red.g << "," << (int)red.b << ")\n";

    Config_L12 cfg = {"localhost", 8080};
    cout << "  [struct] Config: " << cfg.host << ":" << cfg.port << "\n";

    // struct 也能有 private + 建構函數
    Point_L12 pt(3.5, 7.2);
    pt.print();
    // pt.x_ = 0;   // 編譯錯誤！struct 也能有 private

    cout << "\n  [差異速查]\n";
    cout << "  struct 預設 public  繼承預設 public  → 慣用於純資料(POD)\n";
    cout << "  class  預設 private 繼承預設 private → 慣用於完整OOP\n";
    cout << "  功能完全相同，差異僅在預設存取！\n";

    // ============================================================


    cout << "\n=== LeetCode 707. Design Linked List ===" << endl;
    MyLinkedList ll;
    ll.addAtHead(1);
    ll.addAtTail(3);
    ll.addAtIndex(1, 2);            // 1 -> 2 -> 3
    cout << "  addAtHead(1), addAtTail(3), addAtIndex(1,2): " << ll.toString() << endl;
    cout << "  get(1) = " << ll.get(1) << endl;
    ll.deleteAtIndex(1);            // 1 -> 3
    cout << "  deleteAtIndex(1) 後: " << ll.toString() << endl;
    cout << "  get(1) = " << ll.get(1) << endl;
    cout << "  get(99) = " << ll.get(99) << "  (越界安全回 -1)" << endl;
    ll.addAtIndex(99, 7);           // 超出長度 → 不插入
    cout << "  addAtIndex(99,7) 後長度仍為 " << ll.size() << "  (不變量未被破壞)" << endl;

    cout << "\n=== 日常實務：任務排程器 ===" << endl;
    Scheduler sched;
    sched.submit("備份資料庫", 5);
    sched.submit("寄送每日報表", 3);
    sched.submit("清理暫存檔", 1);
    sched.submit("處理付款失敗重試", 9);
    sched.submit("", 100);          // 無效任務 → 被拒絕，不進佇列
    cout << "  共提交 " << sched.total() << " 個有效任務（空名稱已被拒絕）" << endl;

    string name;
    while (sched.runNext(name)) {
        cout << "    執行: " << name
             << "   (剩餘 " << sched.pending() << ")" << endl;
    }
    cout << "  完成率: " << sched.completionRate() << "%" << endl;
    cout << "  再呼叫 runNext -> " << boolalpha << sched.runNext(name)
         << "  (沒有待辦任務)" << endl;

    cout << "\n================================================================\n";
    cout << "  第 6～12 課總複習完成！\n";
    cout << "================================================================\n";

    return 0;
}

/*
 * ================================================================
 * 七課重點速查表
 * ================================================================
 *
 * 課次    主題                核心關鍵字
 * ─────  ──────────────────  ──────────────────────────────────────
 * 第 6課  OOP 核心思想        封裝/繼承/多型(virtual+override)/抽象(=0)
 * 第 7課  class 定義語法      class{}; 類內/類外定義 :: 類內初始化
 * 第 8課  對象創建使用        棧(.) 堆(new/->delete) sizeof 對齊 const引用
 * 第 9課  成員變數            基本型別不初始化=垃圾 string/vector安全 組合
 * 第10課  成員函數            重載 預設參數 鏈式調用(*this) 隱藏this
 * 第11課  存取修飾符          public/private/protected 最小權限原則
 * 第12課  struct vs class     預設存取不同 功能相同 struct=POD class=OOP
 *
 * ================================================================
 */

// 編譯: g++ -std=c++17 -Wall -Wextra summary6-12.cpp -o summary6-12

// === 預期輸出 ===
// ================================================================
//   C++ 面向對象 第 6～12 課 總複習
// ================================================================
// 
// === 第 6 課：OOP 四大支柱 ===
// 
// [封裝] private 資料 + setter 驗證
//   張三 GPA=3.8
//   [拒絕] GPA 需 0~4，收到: -1
//   張三 GPA=3.9
// 
// [繼承] 子類繼承父類成員
//   旺財 正在吃東西
//   旺財 汪汪！
// 
// [多型] virtual + override，透過基類指標動態分派
//   圓形 r=5 面積=78.5397
//   矩形 4x6 面積=24
// 
// [抽象] 純虛函數 = 0，子類必須實作
//   Toyota 引擎發動！
// 
// === 第 7 課：class 定義與語法 ===
//   add(5,3)=8 subtract(10,4)=6
// 
// === 第 8 課：對象創建與使用 ===
//   [棧] ca.count=2 cb.count=1 (獨立)
//   [堆] cp->count=1
//   sizeof(Empty)=1 (空類=1byte)
//   sizeof(WithData char+int+char)=12 (含對齊填充)
//   [參數] 傳值=複製，傳引用=可改，const引用=最推薦
// 
// === 第 9 課：成員變數 ===
//   int/double/bool/char → 不自動初始化 → 垃圾值！
//   string/vector       → 自動初始化   → 安全
//   Honda →   120hp 汽油 引擎啟動
//   [最佳] C++11 類內初始化：int hp = 100; 直接給預設值
// 
// === 第 10 課：成員函數 ===
//   [鏈式] C++ is awesome!
//   [重載] 同名不同參數：
//   int: 42
//   double: 3.14
//   string: Hello
//   [預設參數]
//   [INFO] 程式啟動
//   [ERROR] 連線失敗
//   [this] 成員函數有隱藏 this 指標 → d.bark() 等同 Dog::bark(&d)
// 
// === 第 11 課：存取修飾符 ===
//   陳信安 餘額=$5000 交易0筆
//   [#1] 存款 $2000 → $7000
//   [#2] 提款 $1500 → $5500
//   錯誤：金額無效或餘額不足
//   陳信安 餘額=$5500 交易2筆
//   陳信安 vs 小明: 小明多
// 
//   [權限表]
//   public:    類別/子類/外部 都可存取
//   protected: 類別/子類 可存取，外部不行
//   private:   只有類別自己可存取
//   → 成員變數幾乎總是 private（最小權限原則）
// 
// === 第 12 課：struct vs class ===
//   [struct] RGB=(255,0,0)
//   [struct] Config: localhost:8080
//   Point(3.5,7.2)
// 
//   [差異速查]
//   struct 預設 public  繼承預設 public  → 慣用於純資料(POD)
//   class  預設 private 繼承預設 private → 慣用於完整OOP
//   功能完全相同，差異僅在預設存取！
// 
// === LeetCode 707. Design Linked List ===
//   addAtHead(1), addAtTail(3), addAtIndex(1,2): 1 -> 2 -> 3
//   get(1) = 2
//   deleteAtIndex(1) 後: 1 -> 3
//   get(1) = 3
//   get(99) = -1  (越界安全回 -1)
//   addAtIndex(99,7) 後長度仍為 2  (不變量未被破壞)
// 
// === 日常實務：任務排程器 ===
//   共提交 4 個有效任務（空名稱已被拒絕）
//     執行: 處理付款失敗重試   (剩餘 3)
//     執行: 備份資料庫   (剩餘 2)
//     執行: 寄送每日報表   (剩餘 1)
//     執行: 清理暫存檔   (剩餘 0)
//   完成率: 100%
//   再呼叫 runNext -> false  (沒有待辦任務)
// 
// ================================================================
//   第 6～12 課總複習完成！
// ================================================================
