/*
 * ================================================================
 * 【第 11 課：存取修飾符——public、private、protected】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 為什麼需要存取控制：防止外界隨意修改對象內部資料
 * 2. public：任何人都可以存取（對外介面）
 * 3. private：只有類別自己的成員函數可以存取（內部實現）
 * 4. protected：自己和子類可以存取（繼承時使用，第八階段詳述）
 * 5. class 預設 private，struct 預設 public（唯一語法差異）
 * 6. 存取權限表：類別自身 / 子類 / 外部的可見性
 * 7. 同一類別的不同對象可以互相存取彼此的 private 成員
 * 8. 多個存取修飾符區塊（語法合法，但不建議分散）
 * 9. 最小權限原則：成員變數幾乎總是 private
 * 10. 設計決策流程：何時用 public / protected / private
 * ================================================================
 */

// =============================================================================
// 【主題資訊 Information】
// -----------------------------------------------------------------------------
//   主題：    存取修飾符（Access Specifiers）—— 封裝的語言機制
//   語法：    class X {
//             public:    // 任何人可存取
//             protected: // 自己 + 衍生類別
//             private:   // 只有自己（與 friend）
//             };
//   標準：    C++98 起即有三種 access specifier。
//             本檔用到預設成員初始化（int x = 0;），需 C++11 起；示範以 C++17 編譯。
//   標頭檔：  <iostream>、<string>
//   預設值：  class 預設 private；struct 預設 public（兩者唯一的語法差異）
//   執行期成本：**零**。access specifier 是純編譯期檢查。
//
// =============================================================================
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. 封裝的目的不是「藏起來」，而是「保護不變量」】
//   class invariant（不變量）＝ 物件在任何外部可觀察的時間點都必須成立的條件。
//   以銀行帳戶為例：balance >= 0、owner 不為空、每次變動都要留下軌跡。
//   這些是**業務規則**，編譯器不認識。
//   唯一能強制執行它們的方法，是讓「修改狀態的所有途徑」都經過你寫的函式。
//   public 資料成員正好把這條途徑拆掉：
//       acc.balance = -999999;   // 沒有任何一行你寫的程式碼被執行
//   於是規則從「型別保證」退化成「大家自律」。
//
// 【2. 三種存取權的真正分界】
//     public    → 這是**契約**。一旦公開，就不能隨意更動，
//                 因為外界（可能是別的團隊）已經依賴它。
//     private   → 這是**實作細節**。可以隨時改名、改型別、刪除，外界零感知。
//     protected → 這是**給衍生類別的擴充點**，是一種「半公開契約」。
//   ★ 關鍵認知：protected 的封裝性其實**接近 public**。
//     任何人只要繼承你的類別，就取得了存取權 ——
//     而你無法控制誰來繼承。所以 protected 資料成員是設計上的警訊，
//     實務建議是「protected 只放函式，資料成員仍維持 private」。
//
// 【3. getter/setter 不等於封裝】
//   最常見的誤解是把封裝理解成「成員設 private，每個都配一組 get/set」。
//   空殼 setter 與 public 完全等價：
//       void setBalance(double b) { balance = b; }   // 毫無保護作用
//   判準是：**這個 setter 有沒有可能拒絕呼叫端？**
//   真正的封裝是「不暴露欄位，而暴露操作」：
//       deposit(amt)      // 內含 amt > 0 檢查
//       withdraw(amt)     // 內含餘額足夠檢查，失敗回 false
//   介面該描述「這個物件能做什麼」，而不是「裡面有什麼」。
//
// 【4. 存取控制是「以類別為單位」，不是「以物件為單位」】
//   這是本課最反直覺的一點（重點七）：
//   同一個類別的成員函式，可以存取**另一個同類別物件**的 private 成員。
//       bool Box::isBiggerThan(const Box& other) {
//           return area() > other.width * other.height;   // 合法！
//       }
//   原因是存取檢查發生在**編譯期、以類別為單位**：
//   編譯器問的是「這段程式碼寫在 Box 的成員函式裡嗎」，
//   而不是「這個物件是不是 this」。
//   若非如此，operator==、copy constructor 這類需要讀取另一個物件內部的
//   操作全都寫不出來。
//
// 【5. 最小權限原則的實際收益】
//   把成員設為 private 的收益不是「安全」，而是**可演進性**。
//   假設要把 double balance 改成 long long balance_cents 以避免浮點誤差：
//     - private：只改建構函式與內部運算，外界零修改。
//     - public ：所有讀寫它的程式碼（可能散落數十個檔案、甚至別團隊的專案）
//                全部要改。你的「實作細節」因為 public 變成了「公開契約」。
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
// (A) access specifier 沒有執行期成本，也不是安全機制
//   private 與 public 成員在記憶體中毫無差別，編譯後的機器碼裡
//   也沒有任何「檢查權限」的指令。用 reinterpret_cast 或指標算術
//   照樣讀得到 private 的位元組（雖然那是 UB）。
//   它擋的是**誤用**，不是**惡意存取** —— 別拿 private 保護密碼或金鑰。
//
// (B) 資料成員拆散在多個 access 區塊，會讓型別失去 standard-layout
//   標準規定 standard-layout 的必要條件之一是
//   「所有非靜態資料成員宣告在同一個 access specifier 區塊」。
//   一旦拆開（如本檔重點八的 Demo），該型別就不再是 standard-layout，
//   後果是 offsetof 失去保證、無法安全對應 C struct、
//   「首成員位址 == 物件位址」的保證也失效。
//   這是「別把成員拆散」除了可讀性之外，**可測量的**實質理由。
//
// (C) private 繼承與 protected 繼承
//   本課談的是**成員**的存取權；繼承時的 : public / : protected / : private
//   則是另一層：它決定基底類別的成員在衍生類別中「降級」到什麼程度。
//   public 繼承表達「is-a」，private 繼承表達「用它實作」（has-a 的替代）。
//   這是第八階段的主題，但兩者常被混淆，這裡先劃清界線。
//
// (D) friend 是封裝的正式逃生門
//   friend 讓指定的函式／類別能存取 private。它不打破封裝原則，
//   而是「明確地擴大信任範圍」—— 因為 friend 必須寫在類別定義裡，
//   由類別自己決定信任誰。典型用途是 operator<<
//   （它必須是自由函式，卻常需要讀內部狀態）。
//   但 friend 應克制使用：每加一個，可能碰內部的程式碼就多一份。
//
// =============================================================================
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. 沒有驗證邏輯的 setter 等同 public，別為了「看起來封裝」而寫。
//  2. protected 資料成員的封裝性接近 public（任何人都能繼承取得存取權）；
//     建議 protected 只放函式，資料成員維持 private。
//  3. 存取檢查以**類別**為單位，同類別的不同物件可互訪 private。
//  4. 未初始化的內建型別成員讀取是 UB，值不確定 —— 一律給預設成員初始化。
//  5. 唯讀 getter 應宣告為 const，否則 const 物件與 const 引用無法使用。
//  6. 資料成員應全部放在同一個 access 區塊，以免失去 standard-layout。
//  7. private 不是安全邊界。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】存取修飾符與封裝
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. public / protected / private 的差別是什麼？實務上怎麼選？
//     答：public 任何人可存取、protected 自己與衍生類別、private 只有自己（與 friend）。
//         選擇準則是最小權限：**資料成員幾乎永遠 private**，
//         對外介面 public，內部輔助函式 private，
//         只為衍生類別準備的擴充點才 protected。
//     追問：protected 資料成員有什麼問題？
//         → 它的封裝性接近 public —— 任何人只要繼承就取得存取權，
//           而你無法控制誰來繼承。改動它等同破壞所有衍生類別，
//           所以建議 protected 只放函式。
//
// 🔥 Q2. 成員設 private 再加一組 getter/setter，就算封裝了嗎？
//     答：不一定。若 setter 是 { m_x = x; } 這種空殼，
//         它與 public 完全等價，不變量一樣沒被保護。
//         真正的判準是「這個 setter 有沒有可能拒絕呼叫端」。
//         更好的做法是不暴露欄位而暴露操作 ——
//         用 deposit()／withdraw() 取代 setBalance()。
//     追問：那封裝真正的收益是什麼？
//         → 可演進性。private 成員可以隨時改名、改型別、刪除而不影響外界；
//           public 成員一旦公開就成了契約，改動即 breaking change。
//
// 🔥 Q3. private 能防止資料被讀取嗎？它是安全機制嗎？
//     答：不是。access specifier 是純編譯期檢查，
//         機器碼中沒有任何權限檢查指令，private 與 public 成員
//         在記憶體中也毫無區別。用指標算術照樣讀得到（雖然是 UB）。
//         它擋的是誤用，不是惡意存取。
//     追問：那怎麼真正保護機密資料？
//         → 那是加密、作業系統權限、記憶體保護的領域，
//           不是語言存取控制能解決的問題。
//
// ⚠️ 陷阱 1. 同一個類別的兩個不同物件，a 能不能存取 b 的 private 成員？
//     答：**可以**。存取檢查是**以類別為單位、在編譯期**進行的，
//         編譯器問的是「這段程式碼是否寫在該類別的成員函式內」，
//         而不是「這個物件是不是 this」。
//         所以 Box::isBiggerThan(const Box& other) 裡直接讀 other.width 完全合法。
//     為什麼會錯：直覺把 private 想成「每個物件各自上鎖」，
//         但 C++ 的存取控制是型別層級的。
//         若非如此，operator==、copy constructor 這些需要讀取
//         另一個同型物件內部的操作根本寫不出來。
//
// ⚠️ 陷阱 2. class 與 struct 除了預設存取權，還有別的差別嗎？
//     答：**沒有**。這是兩者唯一的差異（class 預設 private、
//         struct 預設 public；繼承時的預設也依此類推）。
//         struct 一樣可以有成員函式、建構函式、繼承、virtual、private 成員。
//     為什麼會錯：從 C 帶來的印象是「struct 只能裝資料」，
//         於是以為 C++ 的 struct 也有功能限制。
//         實際上兩者是同一個東西，差別純粹是預設值與慣例
//         （慣例上 struct 用於純資料聚合，class 用於有不變量的型別）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <optional>
using namespace std;


// ===== 重點一：沒有存取控制的危險 =====
// 說明：如果所有成員都是 public，外界可以隨意修改，導致資料不一致。
// 例：帳戶餘額可以被設成負數，帳戶主人名稱可以被清空——沒有任何防護！
// 這在小程式中可能不明顯，但在大型專案中是重大安全隱患。

class DangerousAccount {
public:
    string owner;
    double balance = 0.0;
    // 問題：任何地方都能做 acc.balance = -999999.0; 毫無防護
};


// ===== 重點二：public —— 完全公開（對外介面）=====
// 說明：public 成員可以被任何程式碼存取。
// 用途：對外公開的函數介面、常數、需要公開的資料。
// 比喻：商店的服務台，任何顧客都可以來使用。

class Light {
public:
    bool isOn = false;   // public 變數：可以被直接讀取和修改

    void toggle() { isOn = !isOn; }

    void show() {
        cout << "燈: " << (isOn ? "開" : "關") << endl;
    }
};


// ===== 重點三：private —— 完全私有（只有自己）=====
// 說明：private 成員只能被該類別自己的成員函數存取。
// 外部程式碼（包括 main）無法直接存取 private 成員。
// 比喻：保險箱裡的東西，只有保險箱本身（類別內部函數）能碰。
// 用途：需要保護的資料、內部輔助函數。

class Safe {
private:
    string password = "secret123";   // 外界無法存取
    double money = 50000.0;          // 外界無法存取

public:
    bool unlock(const string& input) {
        if (input == password) {     // 類別內部可以存取 private
            cout << "保險箱已開啟，裡面有 $" << money << endl;
            return true;
        } else {
            cout << "密碼錯誤！" << endl;
            return false;
        }
    }
    // 注意：mySafe.password 或 mySafe.money = 0 在外部是編譯錯誤！
};


// ===== 重點四：protected —— 對子類開放（繼承用）=====
// 說明：protected 介於 public 和 private 之間。
// 外界（main 等）不能存取，但子類的成員函數可以存取。
// 目前先理解概念，第八階段（繼承）時會大量使用。
// 比喻：家族的內部資訊，外人不知道，但家族成員（子類）可以知道。

class Animal {
protected:
    string species = "未知";   // 外界不可存取，但子類可以
    int legs = 0;

public:
    void showInfo() {
        cout << species << ", " << legs << " 條腿" << endl;
    }
};

class DogAnimal : public Animal {
public:
    void setup() {
        species = "犬科";   // 子類可以存取父類的 protected 成員
        legs = 4;
    }
};

class Spider : public Animal {
public:
    void setup() {
        species = "蛛形綱";
        legs = 8;
    }
};


// ===== 重點五：class vs struct 預設存取 =====
// 說明：這是 class 和 struct 在 C++ 中唯一的語法差異。
// class 裡未指定修飾符的成員預設是 private。
// struct 裡未指定修飾符的成員預設是 public。

class MyClass {
    int x = 10;   // 預設 private！外部不能直接存取
public:
    int getX() { return x; }
};

struct MyStruct {
    int x = 10;   // 預設 public！外部可以直接存取
};


// ===== 重點六：存取權限總結 =====
// 成員的可見性規則：
//
//  存取者                   | public | protected | private
//  類別自己的成員函數        |   V    |     V     |    V
//  子類的成員函數            |   V    |     V     |    X
//  外部程式碼（main 等）     |   V    |     X     |    X
//  friend 函數/類別          |   V    |     V     |    V
//
// V = 可存取，X = 不可存取


// ===== 重點七：同一類別的不同對象可以互相存取 private =====
// 說明：private 是「類別層級」的存取控制，不是「對象層級」的。
// 也就是說：同一個類別的成員函數，可以存取「任何同類型對象」的 private 成員。
// 理由：同一個類別的作者寫了所有成員函數，他知道如何正確使用自己的私有資料。
// 這個特性在實作「比較」、「複製」等功能時非常有用。

class Box {
private:
    double width = 0;
    double height = 0;

public:
    void setSize(double w, double h) { width = w; height = h; }
    double area() { return width * height; }

    // isLargerThan 是 Box 的成員函數
    // 所以它可以存取「任何 Box 對象」的 private 成員，包括 other.width
    bool isLargerThan(const Box& other) {
        return (width * height) > (other.width * other.height);
    }

    void show() {
        cout << width << " x " << height << " = " << area() << endl;
    }
};


// ===== 重點八：多個存取修飾符區塊（語法合法，不建議分散）=====
// 說明：一個類別中可以出現多個 public / private 區塊，語法上合法。
// 但不建議這樣做！應把所有 public 集中在一起，private 集中在一起，
// 這樣閱讀程式碼時一目瞭然。

class Demo {
public:
    void func1() { cout << "func1" << endl; }
private:
    int data1 = 0;
public:
    // 第二個 public 區塊——合法但不推薦
    void func2() { cout << "func2, data1=" << data1 << endl; }
private:
    int data2 = 0;
public:
    void func3() {
        cout << "func3, data1=" << data1 << ", data2=" << data2 << endl;
    }
};


// ===== 重點九：最小權限原則（設計原則）=====
// 說明：成員變數幾乎總是應該設為 private，只透過 public 函數存取。
// 這叫做「最小權限原則（Principle of Least Privilege）」。
// 好處：
//   1. 資料驗證：修改前可以檢查值是否合法
//   2. 可維護性：未來改內部儲存方式，外部介面不用改
//   3. 除錯方便：異常值只可能從 setter 進入，容易追蹤

class BadStudent {
public:
    string name;
    int age;       // 問題：可以設成 -5 或 999，沒有任何防護
    double gpa;    // 問題：可以設成 100.0，沒有任何防護
};

class GoodStudent {
public:
    // 受控的存取介面
    void setName(const string& n) {
        if (!n.empty()) name = n;
    }

    void setAge(int a) {
        if (a > 0 && a < 150) age = a;
        else cout << "無效年齡: " << a << endl;
    }

    void setGpa(double g) {
        if (g >= 0.0 && g <= 4.0) gpa = g;
        else cout << "無效 GPA: " << g << endl;
    }

    string getName() { return name; }
    int    getAge()  { return age; }
    double getGpa()  { return gpa; }

    void show() {
        cout << name << ", " << age << " 歲, GPA: " << gpa << endl;
    }

private:
    // 資料藏在 private，外界無法直接修改
    string name = "未命名";
    int age = 0;
    double gpa = 0.0;
};


// ===== 重點十：銀行帳戶完整設計（封裝示範）=====
// 說明：正確的封裝設計——資料 private，操作 public，輔助函數 private。
// 存款/提款必須經過驗證，外界無法直接修改 balance，保證資料完整性。

class BankAccount {
public:
    void init(const string& ownerName, const string& accId, double initialBalance) {
        owner = ownerName;
        accountId = accId;
        balance = (initialBalance >= 0) ? initialBalance : 0;
    }

    bool deposit(double amount) {
        if (amount <= 0) { cout << "錯誤：金額需大於0" << endl; return false; }
        balance += amount;
        addTransaction("存款", amount);
        return true;
    }

    bool withdraw(double amount) {
        if (amount <= 0) { cout << "錯誤：金額需大於0" << endl; return false; }
        if (amount > balance) {
            cout << "錯誤：餘額不足（目前: $" << balance << "）" << endl;
            return false;
        }
        balance -= amount;
        addTransaction("提款", amount);
        return true;
    }

    double getBalance()       { return balance; }
    string getOwner()         { return owner; }
    string getAccountId()     { return accountId; }
    int    getTransactionCount() { return transactionCount; }

    void display() {
        cout << "================================" << endl;
        cout << "持有人: " << owner << " | 帳號: " << accountId << endl;
        cout << "餘額: $" << balance << " | 交易次數: " << transactionCount << endl;
        cout << "================================" << endl;
    }

private:
    // 資料完全保護——外界只能透過 public 函數存取
    string owner = "";
    string accountId = "";
    double balance = 0.0;
    int transactionCount = 0;

    // 私有輔助函數——內部邏輯，外界不需要知道
    void addTransaction(const string& type, double amount) {
        transactionCount++;
        cout << "[交易 #" << transactionCount << "] "
             << type << " $" << amount
             << " → 餘額: $" << balance << endl;
    }
};



// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack
//   題目：設計一個堆疊，支援 push / pop / top，
//         並且 getMin() 必須在 **O(1)** 時間內取得堆疊中的最小值。
//   為什麼用到本主題：這題是「封裝讓實作可自由演進」的教科書案例。
//         對外介面只有 push/pop/top/getMin 四個動作 ——
//         使用者完全不需要知道「為了達成 O(1) 的 getMin，
//         內部其實維護了**兩個** vector」。
//         若把內部容器開成 public，這個技巧就變成公開契約，
//         日後想改成單一 vector + 差值編碼就成了 breaking change。
//         不變量：m_min 的長度永遠等於 m_data 的長度，
//         且 m_min[i] == m_data[0..i] 的最小值 —— 由 private 保證無人能破壞。
//   複雜度：四個操作皆 O(1)；空間 O(n)。
// -----------------------------------------------------------------------------
class MinStack {
    vector<int> m_data;   // 實際資料
    vector<int> m_min;    // m_min[i] = 到第 i 層為止的最小值（實作細節，不外露）

public:
    void push(int val) {
        m_data.push_back(val);
        // 維護不變量：新的最小值 = min(舊最小值, 新值)
        if (m_min.empty()) m_min.push_back(val);
        else               m_min.push_back(val < m_min.back() ? val : m_min.back());
    }

    void pop() {
        if (m_data.empty()) return;      // 空堆疊 pop 直接忽略，維持物件有效
        m_data.pop_back();
        m_min.pop_back();                // 兩者必須同步，否則不變量被破壞
    }

    int top() const { return m_data.empty() ? 0 : m_data.back(); }
    int getMin() const { return m_min.empty() ? 0 : m_min.back(); }

    bool empty() const { return m_data.empty(); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】API 金鑰管理：封裝真正該保護的東西
//   情境：服務要保存第三方 API 的金鑰，並在 log／錯誤訊息中顯示時
//         只能露出遮罩後的形式（sk-live-****3f9a），絕不能印出完整金鑰。
//   為什麼用到本主題：這是封裝的**正確用途示範**，同時澄清一個重要邊界 ——
//         private **不是安全機制**（記憶體裡仍是明文，指標算術照樣讀得到）。
//         它保護的是「不會被**不小心**印出來」這件事：
//         因為類別根本沒提供回傳完整金鑰的公開介面，
//         cout << key 只能走 masked()，
//         於是「金鑰誤入 log」這個最常見的事故從源頭被消除。
//         真正的機密保護要靠加密、權限與記憶體管理，那是另一個層次。
// -----------------------------------------------------------------------------
class ApiKey {
    string m_secret;    // private：沒有任何公開介面能取回完整內容

    static string maskOf(const string& s) {
        if (s.size() <= 8) return string(s.size(), '*');
        // 保留前綴（辨識用途）與末 4 碼（比對用途），中間全部遮掉
        return s.substr(0, 8) + string(s.size() - 12, '*') + s.substr(s.size() - 4);
    }

public:
    // 工廠函式：空金鑰無法建立，從源頭排除非法狀態
    static optional<ApiKey> create(const string& secret) {
        if (secret.size() < 12) return nullopt;   // 太短一律視為無效
        return ApiKey(secret);
    }

    // 唯一的對外呈現方式 —— 永遠是遮罩後的
    string masked() const { return maskOf(m_secret); }

    // 提供「比對」而非「取出」：呼叫端能驗證，卻拿不到明文
    bool matches(const string& candidate) const { return m_secret == candidate; }

    size_t length() const { return m_secret.size(); }

private:
    explicit ApiKey(string secret) : m_secret(std::move(secret)) {}
};

// 因為沒有回傳明文的公開介面，operator<< 只可能印出遮罩版
ostream& operator<<(ostream& os, const ApiKey& k) { return os << k.masked(); }

int main() {
    cout << "===== 重點一：沒有存取控制的危險 =====" << endl;
    DangerousAccount da;
    da.owner = "陳信安";
    da.balance = 10000.0;
    da.balance = -999999.0;   // 任何人都能這樣做！
    da.owner = "";             // 帳戶名被清空？
    cout << "持有人: '" << da.owner << "', 餘額: " << da.balance << endl;


    cout << "\n===== 重點二：public（完全公開）=====" << endl;
    Light lamp;
    lamp.show();
    lamp.toggle();
    lamp.show();
    lamp.isOn = false;   // 可以直接存取 public 變數
    lamp.show();


    cout << "\n===== 重點三：private（完全私有）=====" << endl;
    Safe mySafe;
    mySafe.unlock("wrong");       // public 函數可以呼叫
    mySafe.unlock("secret123");
    // mySafe.password;           // 編譯錯誤！private 不能從外部存取
    // mySafe.money = 0;          // 編譯錯誤！


    cout << "\n===== 重點四：protected（子類可存取）=====" << endl;
    DogAnimal dog;
    dog.setup();
    dog.showInfo();

    Spider spider;
    spider.setup();
    spider.showInfo();
    // dog.species = "test";   // 編譯錯誤！外界不能存取 protected


    cout << "\n===== 重點五：class vs struct 預設存取 =====" << endl;
    MyClass mc;
    cout << "class: " << mc.getX() << endl;   // 必須透過 public 函數

    MyStruct ms;
    cout << "struct: " << ms.x << endl;        // 可以直接存取


    cout << "\n===== 重點七：同類別對象互訪 private =====" << endl;
    Box boxA, boxB;
    boxA.setSize(5, 4);   // 面積 20
    boxB.setSize(3, 8);   // 面積 24
    cout << "Box a: "; boxA.show();
    cout << "Box b: "; boxB.show();
    // boxA.isLargerThan(boxB) 內部可以存取 boxB.width（private）
    cout << (boxA.isLargerThan(boxB) ? "a 比 b 大" : "b 比 a 大（或一樣大）") << endl;


    cout << "\n===== 重點八：多個存取區塊（不推薦分散）=====" << endl;
    Demo demo;
    demo.func1();
    demo.func2();
    demo.func3();


    cout << "\n===== 重點九：最小權限原則（GoodStudent）=====" << endl;
    GoodStudent gs;
    gs.setName("陳信安");
    gs.setAge(25);
    gs.setGpa(3.8);
    gs.show();

    gs.setAge(-5);    // 被攔截
    gs.setGpa(5.0);   // 被攔截
    gs.show();         // 值沒變


    cout << "\n===== 重點十：銀行帳戶完整封裝 =====" << endl;
    BankAccount acc;
    acc.init("陳信安", "ACC-001", 5000.0);
    acc.display();
    acc.deposit(2000);
    acc.withdraw(1500);
    acc.withdraw(10000);   // 餘額不足
    acc.deposit(-500);     // 無效金額
    cout << endl;
    acc.display();
    // acc.balance = 999999; // 編譯錯誤！private


    cout << "\n===== 存取修飾符使用原則速查 =====" << endl;
    cout << "成員變數  -> 幾乎總是 private（用 getter/setter 保護）" << endl;
    cout << "對外函數  -> public（這是類別的使用介面）" << endl;
    cout << "輔助函數  -> private（內部邏輯，外界不需要知道）" << endl;
    cout << "給子類的  -> protected（繼承時使用，第八階段詳述）" << endl;


    cout << "\n===== LeetCode 155. Min Stack =====" << endl;
    MinStack minSt;
    minSt.push(-2); minSt.push(0); minSt.push(-3);
    cout << "push -2, 0, -3 後 getMin() = " << minSt.getMin() << endl;   // -3
    minSt.pop();
    cout << "pop 之後 top() = " << minSt.top()
         << ", getMin() = " << minSt.getMin() << endl;                    // 0, -2
    minSt.pop(); minSt.pop();
    cout << "全部彈出後 empty() = " << boolalpha << minSt.empty() << endl;
    minSt.pop();   // 空堆疊再 pop：被擋下，物件仍維持有效狀態
    cout << "空堆疊再 pop 不會壞掉，empty() = " << minSt.empty() << endl;

    cout << "\n===== 日常實務：API 金鑰的封裝 =====" << endl;
    auto key = ApiKey::create("sk-live-9c2f4a7b3f9a");
    if (key) {
        cout << "  直接輸出金鑰物件: " << *key << endl;      // 只可能是遮罩版
        cout << "  長度: " << key->length() << endl;
        cout << "  比對正確金鑰: " << key->matches("sk-live-9c2f4a7b3f9a") << endl;
        cout << "  比對錯誤金鑰: " << key->matches("sk-live-0000000000") << endl;
        cout << "  ★ 類別沒有提供取回明文的公開介面，所以不可能誤印進 log" << endl;
    }
    auto tooShort = ApiKey::create("abc");
    cout << "  太短的金鑰 -> " << (tooShort ? "建立成功(不該發生)" : "建立失敗，回傳 nullopt") << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ===== 重點一：沒有存取控制的危險 =====
// 持有人: '', 餘額: -999999
// 
// ===== 重點二：public（完全公開）=====
// 燈: 關
// 燈: 開
// 燈: 關
// 
// ===== 重點三：private（完全私有）=====
// 密碼錯誤！
// 保險箱已開啟，裡面有 $50000
// 
// ===== 重點四：protected（子類可存取）=====
// 犬科, 4 條腿
// 蛛形綱, 8 條腿
// 
// ===== 重點五：class vs struct 預設存取 =====
// class: 10
// struct: 10
// 
// ===== 重點七：同類別對象互訪 private =====
// Box a: 5 x 4 = 20
// Box b: 3 x 8 = 24
// b 比 a 大（或一樣大）
// 
// ===== 重點八：多個存取區塊（不推薦分散）=====
// func1
// func2, data1=0
// func3, data1=0, data2=0
// 
// ===== 重點九：最小權限原則（GoodStudent）=====
// 陳信安, 25 歲, GPA: 3.8
// 無效年齡: -5
// 無效 GPA: 5
// 陳信安, 25 歲, GPA: 3.8
// 
// ===== 重點十：銀行帳戶完整封裝 =====
// ================================
// 持有人: 陳信安 | 帳號: ACC-001
// 餘額: $5000 | 交易次數: 0
// ================================
// [交易 #1] 存款 $2000 → 餘額: $7000
// [交易 #2] 提款 $1500 → 餘額: $5500
// 錯誤：餘額不足（目前: $5500）
// 錯誤：金額需大於0
// 
// ================================
// 持有人: 陳信安 | 帳號: ACC-001
// 餘額: $5500 | 交易次數: 2
// ================================
// 
// ===== 存取修飾符使用原則速查 =====
// 成員變數  -> 幾乎總是 private（用 getter/setter 保護）
// 對外函數  -> public（這是類別的使用介面）
// 輔助函數  -> private（內部邏輯，外界不需要知道）
// 給子類的  -> protected（繼承時使用，第八階段詳述）
// 
// ===== LeetCode 155. Min Stack =====
// push -2, 0, -3 後 getMin() = -3
// pop 之後 top() = 0, getMin() = -2
// 全部彈出後 empty() = true
// 空堆疊再 pop 不會壞掉，empty() = true
// 
// ===== 日常實務：API 金鑰的封裝 =====
//   直接輸出金鑰物件: sk-live-********3f9a
//   長度: 20
//   比對正確金鑰: true
//   比對錯誤金鑰: false
//   ★ 類別沒有提供取回明文的公開介面，所以不可能誤印進 log
//   太短的金鑰 -> 建立失敗，回傳 nullopt
