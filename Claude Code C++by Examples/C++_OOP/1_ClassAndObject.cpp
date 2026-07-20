/*=============================================================================
 * 檔名：1_ClassAndObject.cpp
 * 主題：Class（類別）與 Object（物件） - OOP 的起點
 * 適合：完全沒接觸過 OOP 的初學者
 *
 * 【課題介紹】
 *   在學 OOP 之前，我們寫的 C++ 大多是「程序式 (Procedural)」寫法：
 *   把資料宣告成變數，把行為寫成函式，函式吃變數作為參數。
 *
 *   例如要表示一個學生：
 *       std::string name  = "Alice";
 *       int         score = 90;
 *       void printStudent(const std::string& n, int s) { ... }
 *
 *   這種寫法資料一變多就會亂，也很難保證「資料」與「處理該資料的函式」
 *   永遠一起搬移。OOP（Object-Oriented Programming，物件導向程式設計）
 *   的核心思想就是：
 *
 *       「把『資料』與『操作那份資料的行為』綁在一起，當作一個整體看待。」
 *
 *   這個整體就叫做「物件 (Object)」，描述物件長什麼樣子的藍圖叫做
 *   「類別 (Class)」。
 *
 * 【生活比喻】
 *   - 類別 (Class)  ≒ 設計藍圖、餅乾模具、餐廳的菜單描述
 *   - 物件 (Object) ≒ 照藍圖蓋出來的房子、用模具做出來的餅乾、實際做出來的菜
 *   一張藍圖可以蓋出許多棟房子，每棟房子都有自己的地址、住戶；
 *   一個類別可以建立許多個物件，每個物件都有自己獨立的資料值。
 *
 * 【關鍵字】
 *   class    定義一個類別
 *   public:  接下來宣告的成員，外界都能存取（誰都能用）
 *   private: 接下來宣告的成員，外界不能存取（只有自己人能用，第 3 篇會深入）
 *
 * 【class 與 struct 的差別】
 *   兩者語法幾乎一樣，唯一差異是：
 *     - class  預設成員為 private
 *     - struct 預設成員為 public
 *   慣例上：純資料用 struct，有行為（成員函式）的物件用 class。
 *
 * 【術語對照表（中英文一起記）】
 *   class             類別
 *   object / instance 物件 / 實例
 *   member variable   成員變數（又叫 field, attribute, data member）
 *   member function   成員函式（又叫 method）
 *   instantiate       實例化（從類別建立一個物件）
 *
 * 【對應 Leetcode】1480. Running Sum of 1d Array
 *   題目簡述：給一個陣列 nums，回傳 runningSum，其中
 *             runningSum[i] = nums[0] + nums[1] + ... + nums[i]
 *   為什麼選這題：這題本質上是「累計和」這個狀態 + 「加新數字」這個行為，
 *   很適合包裝成一個類別，方便看出 class 跟程序式寫法的對比。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/classes
 *   https://cplusplus.com/doc/tutorial/classes/
 *=============================================================================*/

/*
補充筆記：ClassAndObject
  - ClassAndObject 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - class 宣告的是一種「自訂型別」，object 才是記憶體中實際存在的資料；寫出 Student alice; 時，alice 會擁有自己那份 name 和 score，bob 也會擁有另一份，兩個物件不會共用一般成員變數。
  - 成員函式表面上沒有把物件當參數傳進去，但編譯器會替你傳入一個隱含的 this 指標；因此 print() 裡直接寫 name，其實等價於 this->name。
  - public 成員讓外部程式可以直接讀寫，適合教學展示；真實程式若資料有合法範圍，例如 score 必須 0 到 100，通常要把資料設為 private，再用成員函式集中檢查規則。
  - class 和 struct 的能力幾乎相同，主要差在預設存取權限；class 預設 private，struct 預設 public。不要把 struct 誤解成只能放資料，C++ 的 struct 也能有建構子、成員函式與繼承。
  - 類別定義最後一定要有分號，因為 class 宣告本身是一個宣告陳述；初學者漏掉分號時，編譯錯誤常常會出現在下一行，這會讓人誤判真正問題。
  - RunningSum 的 total 若沒有初始化就先 total += x，是讀取未初始化 int，屬於未定義行為；C++ 不保證它是 0，也不保證每次執行結果相同。
  - 物件導向不是把所有東西都包進 class，而是當資料和操作有固定關係時，把它們放在一起，讓「誰負責維持狀態正確」變清楚。
  - 看到一個 class 時，先問三件事：它代表什麼概念、它保存哪些狀態、它提供哪些操作會改變或查詢這些狀態。這比只背 class 語法更接近實際設計。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Class 與 Object
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. class 與 struct 在 C++ 有什麼差別？
//     答：只有「預設存取權」不同 — struct 預設 public、class 預設 private，
//     繼承時的預設存取權同理。其餘（成員函式、建構子、繼承、多型）完全相同。
//     追問：那何時用 struct？（慣例：純資料聚合體用 struct，有不變條件要維護的用 class）
//
// 🔥 Q2. 空 class 的 sizeof 是多少？為什麼不是 0？
//     答：保證至少 1 — 標準要求不同物件必須有不同位址，大小 0 會讓相鄰兩物件位址相同。
//     追問：加了 virtual 函式之後呢？（物件會多一個 vptr；64-bit 上 sizeof 通常變成 8，
//     但確切大小與位置是 ABI/實作定義、標準未規定。再加更多 virtual 函式不會再變大）
//
// Q3. 成員變數沒初始化就使用會怎樣？（本檔 RunningSum 的 total 就是例子）
//     答：int 這類型別在 default-initialization 下不會被給值，內容是不確定的，
//     讀取它是 undefined behavior。C++ 不保證它是 0，也不保證每次執行結果相同。
//
// ⚠️ 陷阱. 一個類別建立 1000 個物件，成員函式的程式碼會被複製 1000 份嗎？
//     答：不會。成員函式的機器碼只有一份、所有物件共用；差別只在編譯器隱式傳入的
//     this 指標不同。所以物件的 sizeof 不會因為成員函式變多而變大（非 virtual 時）。
//     為什麼會錯：把「物件把資料與行為綁在一起」的比喻，誤解成每個物件內部真的包著函式。
//
// 🔥 Q. trivially copyable、standard-layout、trivial 三者分別管什麼？
//     答：常被混為一談，但三者各管一件事，且可以任意組合：trivially copyable 決定
//     「能不能用 memcpy／std::bit_cast」（要求 copy/move ctor、copy/move assign、
//     dtor 都是 trivial）；standard-layout 決定「能不能跟 C struct 互通、offsetof 是否
//     合法」（要求非靜態成員存取權一致、只有一個 class 有非靜態成員、無 virtual）；
//     trivial 則是 trivially copyable 再加上「有 trivial 的預設建構子」。本機實測四種
//     組合都存在，例如 struct { int x; private: int y; }; 可 memcpy 但不是
//     standard-layout；struct C { C(){} int x; }; 是 standard-layout 但不是 trivial。
//     追問：memcpy 的合法性由哪一個決定？（trivially copyable——不是 POD、也不是
//     standard-layout，這是最常見的誤解。POD = trivial + standard-layout，C++20 已把
//     std::is_pod 標記為 deprecated。對非 standard-layout 型別用 offsetof 是
//     conditionally-supported，本機 GCC 會給 -Winvalid-offsetof 警告但仍編譯過）
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>   // std::cout, std::endl 標準輸出
#include <string>     // std::string  字串型別
#include <vector>     // std::vector  動態陣列

// -----------------------------------------------------------------------------
// 範例 1：最基本的類別 - 學生 (Student)
// -----------------------------------------------------------------------------
// 這裡示範「如何定義一個類別」、「如何建立物件」、「如何存取成員」。
class Student {
public:                                    // public 之後宣告的東西，誰都能用
    std::string name;                      // 成員變數：每個物件各自擁有一份
    int         score;                     // 同上，存放這個學生的分數

    // 成員函式 (member function)，定義「行為」
    // 重點：成員函式內可以「直接」讀寫同一個物件的成員變數，
    //       不需要透過參數傳進來。
    void print() {
        std::cout << "學生: " << name
                  << " 分數: " << score << std::endl;
    }
};
// 重要：class 定義結尾的分號 ; 不能漏！這是 C++ 文法要求，
//       否則編譯器會給出讓人摸不著頭緒的錯誤訊息。

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1480 - Running Sum (累計和)
// -----------------------------------------------------------------------------
// 如果用「程序式寫法」(沒有 OOP) 解這題，大概會長這樣：
//   std::vector<int> runningSum(std::vector<int>& nums) {
//       for (size_t i = 1; i < nums.size(); ++i) nums[i] += nums[i-1];
//       return nums;
//   }
//
// 但若我們希望「資料一個一個進來，邊收邊累加」，就很適合用一個有狀態的物件，
// 因為這個物件能把『目前累計到多少』的狀態保留在自己身上：
class RunningSum {
public:
    int total;          // 物件的「狀態」：目前累計到多少

    // 行為：加入一個新的數字，並回傳目前累計和
    int add(int x) {
        total += x;     // 這裡的 total 就是上面那個成員變數
        return total;   // 回傳目前累計和
    }
};
// 注意：這個類別目前 total 沒有預設值，使用前必須自己設成 0，
//       否則會讀到不確定值（undefined behavior，未定義行為）。
//       下一篇 Constructor 會學到「物件出生時自動初始化」的方法。

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 1672 - Richest Customer Wealth
// -----------------------------------------------------------------------------
// 題目簡述：給一個 m x n 矩陣 accounts，accounts[i][j] 代表第 i 位客戶
//           在第 j 家銀行的存款。回傳「最富有客戶」的總財富。
// 為什麼選這題：把一位客戶 (一列) 包成一個 class，是 OOP 的好練習。
class Customer {
public:
    std::vector<int> banks;   // 此客戶在各家銀行的存款

    // 行為：計算自己的總財富
    int wealth() const {
        int sum = 0;
        for (int x : banks) sum += x;     // 累加所有銀行存款
        return sum;
    }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - SensorReading 感測器讀數
// -----------------------------------------------------------------------------
// 工作上常見：把感測器一次讀出來的資料包成一個物件。
class SensorReading {
public:
    std::string sensorId;     // 感測器編號
    double      temperature;  // 溫度 (攝氏)
    double      humidity;     // 濕度 (百分比)

    // 行為：判斷是否處於警戒範圍
    bool isAlarm() const {
        return temperature > 40.0 || humidity > 90.0;
    }

    void print() const {
        std::cout << "[" << sensorId << "] 溫度 " << temperature
                  << "°C, 濕度 " << humidity << "%"
                  << (isAlarm() ? " (警戒!)" : "") << std::endl;
    }
};

int main() {
    // -------------------------------------------------------------------------
    // [1] 建立物件 (instantiate an object)
    // -------------------------------------------------------------------------
    // 語法：    類別名稱  物件名稱;
    // 這一行的意思：「按照 Student 這張藍圖，蓋出一個叫 alice 的物件」。
    Student alice;

    // 用 . (dot operator，成員存取運算子) 來存取物件的成員
    alice.name  = "Alice";
    alice.score = 90;
    alice.print();        // 呼叫成員函式：印出 alice 自己的名字與分數

    // 我們可以從同一個類別建立多個物件，每個物件的資料完全獨立
    Student bob;
    bob.name  = "Bob";
    bob.score = 75;
    bob.print();

    // 確認：alice 跟 bob 是兩個不同的物件，改 bob 不會影響 alice
    std::cout << "alice.score 還是 " << alice.score << std::endl;

    std::cout << "----- Running Sum 範例 (Leetcode 1480) -----" << std::endl;

    // -------------------------------------------------------------------------
    // [2] Leetcode 1480 - 用我們的 RunningSum 類別示範
    // -------------------------------------------------------------------------
    std::vector<int> nums = {1, 2, 3, 4};
    RunningSum rs;
    rs.total = 0;        // 重要：必須先初始化，不然 total 是不確定值

    std::cout << "原陣列: ";
    for (int x : nums) std::cout << x << " ";
    std::cout << "\n累計和: ";
    for (int x : nums) {
        std::cout << rs.add(x) << " ";   // 預期輸出 1, 3, 6, 10
    }
    std::cout << std::endl;

    std::cout << "----- Leetcode 1672 Richest Customer Wealth -----" << std::endl;
    // 三位客戶，每人各在 3 家銀行的存款
    std::vector<Customer> customers(3);
    customers[0].banks = {1, 2, 3};       // 總財富 6
    customers[1].banks = {3, 2, 1};       // 總財富 6
    customers[2].banks = {1, 5, 7};       // 總財富 13 (最富有)

    int maxWealth = 0;
    for (const Customer& c : customers) {
        int w = c.wealth();
        if (w > maxWealth) maxWealth = w;
    }
    std::cout << "最富有財富: " << maxWealth << std::endl;

    std::cout << "----- 感測器讀數 (日常實用) -----" << std::endl;
    SensorReading s1;
    s1.sensorId = "S-01"; s1.temperature = 25.3; s1.humidity = 60.0;
    s1.print();

    SensorReading s2;
    s2.sensorId = "S-02"; s2.temperature = 42.5; s2.humidity = 88.0;
    s2.print();    // 超過 40 度 → 警戒

    return 0;
}

/* 預期輸出：
 * 學生: Alice 分數: 90
 * 學生: Bob 分數: 75
 * alice.score 還是 90
 * ----- Running Sum 範例 (Leetcode 1480) -----
 * 原陣列: 1 2 3 4
 * 累計和: 1 3 6 10
 * ----- Leetcode 1672 Richest Customer Wealth -----
 * 最富有財富: 13
 * ----- 感測器讀數 (日常實用) -----
 * [S-01] 溫度 25.3°C, 濕度 60%
 * [S-02] 溫度 42.5°C, 濕度 88% (警戒!)
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 類別 (class) 是「藍圖」；物件 (object) 是按藍圖蓋出來的「實體」。
 *   2. 定義 class 之後，結尾一定要 ; 分號。
 *   3. public: 之後的成員，外面可以用「物件名.成員名」來存取。
 *   4. 同一個類別可以建立多個物件，每個物件的成員變數值彼此獨立。
 *   5. 成員函式不用把物件當參數傳進去，因為它「本身」就是某個物件的行為。
 *   6. 沒有給初值的成員變數，使用前要自己設好，否則是「未定義行為」。
 *
 * 【下一篇預告】
 *   2_MemberAndMethod.cpp
 *   更深入認識成員變數與成員函式（如何在 class 外定義函式、行內函式 inline 等），
 *   並用 Leetcode 1929. Concatenation of Array 來練習。
 *=============================================================================*/
