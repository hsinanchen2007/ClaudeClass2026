// =============================================================================
//  第 7 課：類別（class）的定義與語法 — summary.cpp
//  類別語法總整理：定義 / 存取控制 / 類內外定義 / 類內初始值 / 綜合實作
// =============================================================================
//
// 【主題資訊 Information】
//   class 名稱 { 存取修飾詞: 成員變數; 成員函式(); };   // 結尾分號不可省
//   存取修飾詞：public / protected / private（class 預設 private）
//   類外定義  ：回傳型別 類別名::函式名(參數) { ... }
//   類內初始值：int x = 0;                              // NSDMI，C++11 起
//   標準版本  ：類別語法 C++98；類內初始值 C++11
//   標頭檔    ：語言核心特性；本檔另用 <iostream> <string> <vector> <utility>
//
// 【詳細解釋 Explanation】
//
// 【1. 一個類別定義裡到底能放什麼】
//   本檔涵蓋了實務上最常見的全部組成：
//       資料成員         string name;              每個物件一份
//       類內初始值       double balance = 0.0;     C++11，所有建構子共用的預設值
//       類內定義的函式   void display() { ... }    隱式 inline
//       類內宣告         void deposit(double);     實作寫在類別外
//       存取控制         public / private          決定誰能碰
//   讀類別定義時，把它當成一份【契約書】來看：
//   public 區塊是對外承諾的介面，private 區塊是自己保留的實作細節。
//
// 【2. 存取控制的三個等級，以及該怎麼選】
//       public    ：任何人都能存取 —— 這是你對外的承諾，改動會破壞使用者程式碼
//       private   ：只有自己（與 friend）能存取 —— 預設就該選這個
//       protected ：自己與派生類能存取
//   實務原則是【預設 private，需要才開放】。
//   特別提醒 protected 資料成員通常是設計氣味：
//   它等於把封裝的破口開放給【整個繼承體系】，
//   之後想改內部表示就會牽連所有派生類。
//   要給派生類用的話，提供 protected 的【函式】通常比開放資料成員好。
//
// 【3. 本檔 AccessDemo 示範的正是封裝的最小形式】
//       ad.publicValue = 100;    // 直接改，沒有任何檢查
//       ad.setSecret(42);        // 透過唯一入口，可以驗證、記錄、觸發副作用
//       // ad.secretValue = 99;  // 編譯錯誤 —— 這正是重點
//   注意第三行的價值在於它【編譯不過】。
//   存取控制是【編譯期】的機制，不產生任何執行期指令、
//   也不影響 sizeof 與物件佈局 ——
//   它保護的是「程式碼的正確性」，不是「記憶體的安全性」。
//
// 【4. BankAccount 這個綜合範例的優點與缺陷】
//   優點：完整示範了類內初始值、類內定義、類外定義三種語法，
//         而且 deposit/withdraw 都有輸入驗證。
//   缺陷：balance 是 public，所有驗證都能被繞過
//         （acc.balance = -99999; 完全合法）。
//   這是本課刻意保留的教學順序 —— 先學語法，第 20 課才學封裝。
//   另外用 double 存金額在真實系統中是嚴重錯誤（浮點誤差無法對帳），
//   實務應以整數的最小單位（分）儲存。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 存取控制不影響物件佈局
//   把成員從 public 改成 private【不會】改變 sizeof、
//   不會改變成員的偏移量、也不會產生任何執行期檢查。
//   編譯器只是在編譯時拒絕不合法的存取。
//   （嚴格說來，若 private 與 public 成員之間有存取修飾詞分隔，
//     標準允許實作重新排序不同區段的成員，
//     但主流編譯器都維持宣告順序。）
//
// (B) 成員初始化的順序由【宣告順序】決定
//   成員一律依照【在類別中宣告的順序】初始化，
//   與建構子初始化列表中的書寫順序無關。
//       class C {
//           int b;
//           int a;
//       public:
//           C() : a(1), b(a) {}   // 危險！b 先初始化，此時 a 還沒有值
//       };
//   打開 -Wreorder（-Wall 已包含）編譯器會警告初始化列表順序與宣告順序不符。
//   養成「初始化列表順序＝宣告順序」的習慣可以完全避開這個問題。
//
// (C) 為什麼 explicit 對單參數建構子很重要
//   本檔加入的 OrderedStream(int n) 寫了 explicit。
//   若不寫，編譯器會允許隱式轉換：
//       OrderedStream os = 5;              // 沒有 explicit 時合法，但很怪
//       someFunc(5);                       // 若參數是 OrderedStream，也會默默轉換
//   規則：【單參數建構子預設就該加 explicit】，
//   除非你真的想要隱式轉換（例如 std::string 從 const char* 轉換）。
//
// 【注意事項 Pay Attention】
//   1. 類別定義結尾的分號不可省略。
//   2. 類外定義必須寫 ClassName::，否則會變成獨立的自由函式，
//      並在連結階段報 undefined reference。
//   3. 存取控制是編譯期機制，不影響 sizeof、佈局或執行期效能。
//   4. 成員初始化順序由【宣告順序】決定，與初始化列表的書寫順序無關。
//   5. 單參數建構子預設就該加 explicit。
//   6. 本檔的 BankAccount 把 balance 設為 public，驗證可被繞過 ——
//      這是教學順序上的簡化（封裝見第 20 課），不是設計範本。
//   7. 【不要】用 double 存真實金額。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別定義與存取控制
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把成員從 public 改成 private，物件的大小或效能會改變嗎？
//     答：都不會。存取控制純粹是【編譯期】的檢查機制 ——
//         它不產生任何執行期指令，不改變 sizeof，
//         成員的偏移量也不變。編譯器只是在你寫出非法存取時拒絕編譯。
//         換句話說，private 保護的是「程式碼的正確性」，
//         不是「記憶體的安全性」——
//         用指標硬算偏移量一樣能改到 private 成員（那是 UB，但編譯器擋不住）。
//     追問：那 private 的意義是什麼？
//           → 把「可能修改狀態的程式碼」收斂到少數幾個可檢查的入口。
//             它防的是【意外】與【維護困難】，不是防惡意存取。
//
// 🔥 Q2. 成員初始化的順序是由什麼決定的？
//     答：由成員在類別中的【宣告順序】決定，
//         與建構子初始化列表的書寫順序【完全無關】。
//         所以寫成 C() : a(1), b(a) {} 時，
//         如果 b 宣告在 a 前面，b 會先被初始化，
//         而此時 a 還沒有值 —— 讀取它是未定義行為。
//         -Wall 包含的 -Wreorder 會對「列表順序與宣告順序不符」發出警告。
//     追問：為什麼標準要這樣規定？
//           → 因為解構必須以【嚴格相反】的順序進行。
//             若初始化順序可以隨建構子而異，
//             編譯器就無法在解構時決定唯一的正確順序。
//
// ⚠️ 陷阱. 「deposit() 裡已經檢查了金額必須大於 0，
//          所以這個 BankAccount 的餘額不可能出錯」—— 錯在哪？
//     答：錯在 balance 是 public。
//         任何人都能寫 acc.balance = -99999; 直接改，
//         那兩個函式裡的檢查形同虛設。
//         驗證邏輯只有在「所有修改都必須經過它」時才有意義，
//         而 public 成員永遠存在繞過的路徑。
//     為什麼會錯：把「有寫檢查」當成「不變量成立」。
//         判斷不變量能不能成立，要問的不是「我檢查了嗎」，
//         而是【有沒有其他路徑可以修改這個狀態】。
//         這也是為什麼封裝的第一步永遠是「把資料設為 private」——
//         不是因為藏起來比較安全，
//         而是因為只有收斂了所有入口，檢查才真的有效力。
//         本檔的 AccessDemo 示範了正確做法：
//         secretValue 是 private，只能透過 setSecret 修改。
// ═══════════════════════════════════════════════════════════════════════════

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
#include <vector>     // LeetCode 1656 與設定檔範例使用
#include <utility>    // std::pair
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



// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1656. Design an Ordered Stream
//   題目：資料以 (id, value) 的形式亂序到達，id 為 1..n 且每個恰好出現一次。
//         每次 insert 之後，要回傳「從目前指標開始、連續已到達」的那一段值；
//         若指標處尚未到達，則回傳空清單。
//   為什麼用到本主題：這題是「類別＝資料＋行為」最乾淨的示範——
//         stream_（已收到的資料）與 ptr_（下一個要輸出的位置）
//         是必須【一起維護】的兩個狀態；
//         把它們綁進同一個類別、只透過 insert 修改，
//         正是本課「成員變數 + 成員函式」的核心用途。
//   複雜度：單次 insert 為 O(回傳長度)；n 次操作總計 O(n)（指標只前進不後退）。
// -----------------------------------------------------------------------------
class OrderedStream {
private:
    vector<string> stream_;     // 索引 1..n 存放對應 id 的值
    int ptr_;                   // 下一個待輸出的 id

public:
    explicit OrderedStream(int n) : stream_(static_cast<size_t>(n) + 1), ptr_(1) {}

    vector<string> insert(int idKey, const string& value) {
        stream_[static_cast<size_t>(idKey)] = value;
        vector<string> chunk;
        // 從 ptr_ 開始，把所有已經到達的連續資料一次輸出
        while (ptr_ < static_cast<int>(stream_.size())
               && !stream_[static_cast<size_t>(ptr_)].empty()) {
            chunk.push_back(stream_[static_cast<size_t>(ptr_)]);
            ++ptr_;
        }
        return chunk;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔讀取器（示範類內宣告 + 類外定義的分工）
//   情境：從 "key=value" 形式的設定內容中查值，找不到時回傳預設值。
//   為什麼用到本主題：這是「介面與實作分離」最典型的小型案例——
//         類別內只留下簡潔的函式簽名（像一份目錄），
//         實作寫在類別外，日後改解析邏輯不必動到類別宣告。
// -----------------------------------------------------------------------------
class ConfigReader {
private:
    vector<pair<string, string>> entries_;

public:
    void loadLine(const string& line);                    // 類內宣告
    string get(const string& key, const string& def) const;
    size_t size() const { return entries_.size(); }       // 短小者留在類內
};

// 類外定義（注意一定要寫 ConfigReader:: ）
void ConfigReader::loadLine(const string& line) {
    if (line.empty() || line[0] == '#') return;           // 略過空行與註解
    size_t eq = line.find('=');
    if (eq == string::npos) return;                       // 沒有 '=' 視為格式錯誤
    entries_.emplace_back(line.substr(0, eq), line.substr(eq + 1));
}

string ConfigReader::get(const string& key, const string& def) const {
    for (const auto& kv : entries_) {
        if (kv.first == key) return kv.second;
    }
    return def;
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


    // --- LeetCode 1656：類別綁定「資料 + 行為」 ---
    cout << "\n--- LeetCode 1656. Design an Ordered Stream ---" << endl;
    OrderedStream os(5);
    const int    ids[]  = { 3, 1, 2, 5, 4 };
    const char*  vals[] = { "ccccc", "aaaaa", "bbbbb", "eeeee", "ddddd" };
    for (int i = 0; i < 5; ++i) {
        vector<string> chunk = os.insert(ids[i], vals[i]);
        cout << "  insert(" << ids[i] << ", " << vals[i] << ") -> [";
        for (size_t j = 0; j < chunk.size(); ++j) {
            cout << (j ? ", " : "") << chunk[j];
        }
        cout << "]" << endl;
    }

    // --- 日常實務：設定檔讀取器（類外定義的分工） ---
    cout << "\n--- 日常實務：設定檔讀取器 ---" << endl;
    ConfigReader cfg;
    cfg.loadLine("# 這是註解，會被略過");
    cfg.loadLine("host=192.168.1.10");
    cfg.loadLine("port=8080");
    cfg.loadLine("這行沒有等號，會被忽略");
    cout << "  共載入 " << cfg.size() << " 筆設定" << endl;
    cout << "  host    = " << cfg.get("host", "(無)") << endl;
    cout << "  port    = " << cfg.get("port", "(無)") << endl;
    cout << "  timeout = " << cfg.get("timeout", "30（預設值）") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o class_summary
//   類內初始值（double balance = 0.0;）是 C++11 起的功能，其餘語法 C++98 即有。

// 註 1:本檔輸出是【完全確定的】—— 沒有輸入、亂數、執行緒或位址，
//      連跑 5 次位元組完全相同。

// 註 2:LeetCode 1656 的輸出與官方範例一致：
//        insert(3,"ccccc") -> []            指標停在 1，尚未到達
//        insert(1,"aaaaa") -> [aaaaa]       1 到齊，輸出並前進
//        insert(2,"bbbbb") -> [bbbbb,ccccc] 2 到齊，連同先前的 3 一起輸出
//        insert(5,"eeeee") -> []            指標在 4，尚未到達
//        insert(4,"ddddd") -> [ddddd,eeeee]
//      ptr_ 只前進不後退，所以 n 次 insert 的總成本是 O(n)。

// 註 3:設定檔範例中「共載入 2 筆設定」證明了註解行（# 開頭）
//      與沒有等號的行都被正確略過；timeout 不存在，因此回傳預設值。

// 註 4:餘額印成 $1300 而非 $1300.00，是 iostream 對 double 的預設格式
//      （6 位有效數字、去掉尾隨零）所致，並非計算結果有誤。
//      另請注意：真實金融系統【不應】用 double 存金額。

// 註 5:存取修飾符示範中被註解掉的 ad.secretValue = 99;
//      其價值在於它【編譯不過】。存取控制是編譯期機制，
//      不會在執行期產生任何檢查指令。

// === 預期輸出 ===
// ===== 重點一到七示範 =====
// 名字: 旺財
// 年齡: 3 歲
// 品種: 柴犬
// 旺財 說：汪汪！
// 旺財 乖乖坐下了
//
// --- Person 範例（成員函數存取成員變數）---
// 你好，我叫 小明，今年 20 歲。
// 你好，我叫 小華，今年 22 歲。
//
// --- Calculator 範例（類外定義）---
// 計算結果 = 8
// 計算結果 = 6
//
// --- Student 範例（類內初始化）---
// 未命名, 0 歲, GPA=0
// 陳信安, 20 歲, GPA=3.8
//
// ===== 綜合範例：銀行帳戶 =====
// ========================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $0
// ========================
// 存入 $1000，目前餘額: $1000
// 存入 $500，目前餘額: $1500
// 提取 $200，目前餘額: $1300
// 錯誤：餘額不足！目前餘額: $1300
// 錯誤：存款金額必須大於 0
//
// ========================
// 帳戶持有人: 陳信安
// 帳號:       ACC-001
// 餘額:       $1300
// ========================
//
// --- 第二個帳戶（獨立對象）---
// ========================
// 帳戶持有人: 小明
// 帳號:       ACC-002
// 餘額:       $3000
// ========================
// 提取 $500，目前餘額: $2500
// ========================
// 帳戶持有人: 小明
// 帳號:       ACC-002
// 餘額:       $2500
// ========================
//
// --- 存取修飾符示範 ---
// public: 100
// secret（透過 getter）: 42
//
// --- LeetCode 1656. Design an Ordered Stream ---
//   insert(3, ccccc) -> []
//   insert(1, aaaaa) -> [aaaaa]
//   insert(2, bbbbb) -> [bbbbb, ccccc]
//   insert(5, eeeee) -> []
//   insert(4, ddddd) -> [ddddd, eeeee]
//
// --- 日常實務：設定檔讀取器 ---
//   共載入 2 筆設定
//   host    = 192.168.1.10
//   port    = 8080
//   timeout = 30（預設值）
