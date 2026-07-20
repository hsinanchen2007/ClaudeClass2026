/*=============================================================================
 * 檔名：2_MemberAndMethod.cpp
 * 主題：成員變數 (Member Variable) 與 成員函式 (Member Function)
 * 適合：剛學會「class 是什麼」之後，想再深入一點的初學者
 *
 * 【課題介紹】
 *   一個類別 (class) 內主要由兩種「成員 (member)」組成：
 *
 *     1. 成員變數 (member variable / data member / field)
 *        - 用來存放「狀態 (state)」，例如分數、名字、餘額...
 *        - 每個物件都有自己獨立的一份。
 *
 *     2. 成員函式 (member function / method)
 *        - 用來定義「行為 (behavior)」，例如顯示、計算、修改...
 *        - 通常會去讀寫該物件的成員變數。
 *
 *   本篇要更仔細看：
 *     A. 函式怎麼定義（在 class 內 vs class 外）
 *     B. 函式參數、回傳值
 *     C. inline（行內展開）、const 成員函式（先簡介，第 10 篇深入）
 *     D. 用 Leetcode 1929 練習
 *
 * 【在 class 內 vs class 外定義】
 *   寫小範例時，把實作直接寫在 class 大括號內最方便（也預設帶 inline 屬性）。
 *   寫大型專案時，慣例是：
 *     - .h 檔（標頭檔）只放「宣告」(declaration)
 *     - .cpp 檔放「定義」(definition)
 *   定義在 class 外時，要用 「類別名::函式名」表明這個函式屬於哪個類別。
 *   :: 叫做「scope resolution operator (範圍解析運算子)」。
 *
 * 【inline 是什麼？】
 *   inline 提示編譯器：呼叫這個函式時可以「直接把函式內容貼進來」，省下函式呼叫成本。
 *   ※ 寫在 class 內的成員函式，預設就有 inline 屬性，不必特別加。
 *   ※ inline 只是「建議」，編譯器有權忽略。
 *
 * 【對應 Leetcode】1929. Concatenation of Array
 *   題目簡述：給一個長度 n 的陣列 nums，回傳長度 2n 的新陣列 ans，
 *             使得 ans[i] = nums[i] 且 ans[i+n] = nums[i]。
 *   為什麼選這題：邏輯非常單純（複製兩次接起來），重點不在演算法，
 *   而在於「用一個類別把 vector 與相關行為包起來」這件事。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/member_functions
 *   https://cplusplus.com/doc/tutorial/classes/
 *=============================================================================*/

/*
補充筆記：MemberAndMethod
  - MemberAndMethod 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - 成員變數描述物件的狀態，成員函式描述能對狀態做的操作；好的類別不只是把變數和函式塞在一起，而是讓操作名稱表達明確意圖。
  - 在 class 裡直接定義的短函式通常隱含 inline；這不代表一定會被編譯器展開，而是允許該函式定義出現在多個 translation unit 中不違反 ODR。
  - 把成員函式宣告放在 class 內、定義放在 class 外時，要使用 類別名::函式名。少寫作用域解析運算子會變成普通函式，不能直接存取成員。
  - 成員函式可以讀寫同一個物件的成員，但它不應任意暴露內部資料；如果每個呼叫者都能直接改狀態，類別就很難保證自己的不變條件。
  - 命名上常把私有成員用底線結尾或 m_ 開頭，例如 total_，用來和區域變數區分；這不是語法要求，而是降低 this-> 是否必要的閱讀成本。
  - const 成員函式代表這個操作不應改變物件可觀察狀態；只查詢資料的 method 應該盡量標 const，這能讓 const 物件也能呼叫它。
  - method 的參數若是大型物件，通常用 const reference 傳入；若函式需要保存一份資料，才考慮複製或 move 進成員變數。
  - 學成員函式時要注意「物件是誰」：a.print() 和 b.print() 呼叫同一份函式程式碼，但 this 指向不同物件，所以讀到的 name/score 不同。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員變數與成員函式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 成員函式定義在 class 內與 class 外有什麼差別？
//     答：定義在 class 內的成員函式隱含 inline，因此同一份定義可以出現在多個
//     translation unit 而不違反 ODR；定義在 class 外要用 `ClassName::` 指明歸屬，
//     慣例放 .cpp（若放 header 就得自己加 inline）。
//     追問：inline 保證編譯器一定展開嗎？（不保證 — 現代 inline 的主要意義是 ODR 豁免，
//     展開與否由編譯器自行決定）
//
// 🔥 Q2. static 成員函式與一般成員函式差在哪？
//     答：static 成員函式沒有 this，屬於類別而非某個物件，因此不能直接存取非 static
//     成員；呼叫時不需要物件（本檔的 `ArrayConcat::print(v)`）。
//     追問：static 成員函式可以是 virtual 嗎？（不行 — virtual 分派要從物件取 vptr，
//     沒有 this 就無從分派，兩者語意互斥）
//
// ⚠️ 陷阱. 宣告寫 `void show() const;`，在 class 外定義時漏掉 const 會怎樣？
//     答：編譯錯誤。const 是函式簽名的一部分，`show()` 與 `show() const` 是兩個不同的
//     函式，class 外的定義會找不到對應的宣告。
//     為什麼會錯：把 const 當成可有可無的「提示」；事實上在 class 內這兩者甚至能構成
//     合法重載（依呼叫物件的 constness 選擇版本）。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// 範例 1：銀行帳戶 (BankAccount) - 看「在 class 內」與「在 class 外」定義
// -----------------------------------------------------------------------------
class BankAccount {
public:
    std::string owner;     // 戶名 (狀態)
    double      balance;   // 餘額 (狀態)

    // (A) 直接在 class 內定義 - 簡短的 setter/getter 通常這樣寫，最直觀
    void setOwner(const std::string& name) {
        owner = name;
    }

    // (B) 「在 class 內宣告，class 外定義」 - 大型專案常見寫法
    //     宣告：告訴編譯器有這個函式
    void deposit(double amount);              // 存款
    void withdraw(double amount);             // 提款
    void show() const;                        // 顯示帳戶資訊（const 成員函式，待第 10 篇詳述）
};

// 在 class 外定義 - 要用「類別名::」表明歸屬
// 注意 const 也要寫一致，不然會被當成另一個函式
void BankAccount::deposit(double amount) {
    balance += amount;
}

void BankAccount::withdraw(double amount) {
    // 簡單檢查：避免餘額不足
    if (amount > balance) {
        std::cout << "餘額不足！" << std::endl;
        return;
    }
    balance -= amount;
}

void BankAccount::show() const {     // const 表示這個函式不會修改任何成員變數
    std::cout << "[" << owner << "] 餘額: " << balance << std::endl;
}

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1929 - Concatenation of Array
// -----------------------------------------------------------------------------
// 我們把「持有一個陣列 + 提供把陣列重複串接成 2n 長度的方法」包成一個類別。
class ArrayConcat {
public:
    std::vector<int> nums;        // 成員變數：存放原始陣列

    // 行為：產生 ans，其中 ans[i] = nums[i] 且 ans[i+n] = nums[i]
    // 為什麼回傳值用 std::vector<int>（傳值而非參考）？
    //   - 簡單明瞭，呼叫端拿到一個全新的陣列，不會誤改原本的 nums。
    //   - 現代 C++ 因為 RVO/Move semantics，這樣寫不會慢（之後 22 篇會講 move）。
    std::vector<int> getConcatenation() const {
        std::vector<int> ans;
        ans.reserve(nums.size() * 2);     // 先預留空間，避免反覆重新配置記憶體
        for (int x : nums) ans.push_back(x);
        for (int x : nums) ans.push_back(x);
        return ans;
    }

    // 工具方法：印出陣列（純粹方便看結果）
    static void print(const std::vector<int>& v) {
        // static：屬於整個類別的方法，不需要物件就能呼叫，第 11 篇會深入。
        for (int x : v) std::cout << x << " ";
        std::cout << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 1572 - Matrix Diagonal Sum
// -----------------------------------------------------------------------------
// 題目簡述：給一個 n x n 的方陣 mat，回傳「主對角線 + 副對角線」上元素之和；
//           若 n 為奇數，正中央那一格只算一次。
// 我們把這個操作包成一個類別，示範「在 class 內宣告 + 在 class 外定義」的另一個練習。
class DiagonalSum {
public:
    std::vector<std::vector<int>> mat;     // 成員變數：方陣

    int compute() const;                   // 宣告
};

// 在 class 外定義 (必須加上 DiagonalSum::)
int DiagonalSum::compute() const {
    int n = static_cast<int>(mat.size());
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += mat[i][i];                  // 主對角線
        sum += mat[i][n - 1 - i];          // 副對角線
    }
    // n 為奇數時，正中央被加了兩次，扣回一次
    if (n % 2 == 1) sum -= mat[n / 2][n / 2];
    return sum;
}

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - Timer 計時器類別
// -----------------------------------------------------------------------------
// 工作上很常需要量「累計了多少時間」，這裡用最簡單的計次累加模擬。
class Timer {
private:
    long total_;        // 累計時間 (毫秒)
    int  laps_;         // 經過的圈數

public:
    Timer() : total_(0), laps_(0) {}        // 預設建構，初值都歸零

    // 加入一段耗時 (毫秒)
    void lap(long ms) {
        total_ += ms;
        ++laps_;
    }

    // 只讀方法，加上 const
    long total() const { return total_; }
    int  laps()  const { return laps_; }
    double average() const {
        return laps_ == 0 ? 0.0 : static_cast<double>(total_) / laps_;
    }
};

int main() {
    std::cout << "----- 範例 1：BankAccount -----" << std::endl;

    BankAccount acc;
    acc.setOwner("Alice");
    acc.balance = 0.0;          // 直接設成員變數（之後第 3 篇會學到改用建構子＋封裝）

    acc.deposit(1000);          // 存 1000
    acc.deposit(500);           // 再存 500
    acc.withdraw(300);          // 提 300
    acc.show();                 // 預期 1200

    acc.withdraw(99999);        // 提太多，預期會跳出「餘額不足」

    std::cout << "----- 範例 2：Leetcode 1929 -----" << std::endl;

    ArrayConcat ac;
    ac.nums = {1, 2, 1};        // 直接賦值給成員變數
    std::vector<int> ans = ac.getConcatenation();
    std::cout << "原陣列: "; ArrayConcat::print(ac.nums);
    std::cout << "結果:   "; ArrayConcat::print(ans);   // 預期 1 2 1 1 2 1

    // 再試一個
    ac.nums = {1, 3, 2, 1};
    std::cout << "原陣列: "; ArrayConcat::print(ac.nums);
    std::cout << "結果:   "; ArrayConcat::print(ac.getConcatenation()); // 1 3 2 1 1 3 2 1

    std::cout << "----- 範例 3：Leetcode 1572 對角線和 -----" << std::endl;
    DiagonalSum ds;
    ds.mat = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    // 主對角線: 1 + 5 + 9 = 15
    // 副對角線: 3 + 5 + 7 = 15
    // 5 被算兩次 → 扣 5 → 答案 25
    std::cout << "對角線和 = " << ds.compute() << std::endl;

    std::cout << "----- 範例 4：Timer 計時器 -----" << std::endl;
    Timer t;
    t.lap(120);
    t.lap(80);
    t.lap(100);
    std::cout << "總計 " << t.total() << " ms / "
              << t.laps() << " 圈, 平均 " << t.average() << " ms" << std::endl;

    return 0;
}

/* 預期輸出：
 * ----- 範例 1：BankAccount -----
 * [Alice] 餘額: 1200
 * 餘額不足！
 * ----- 範例 2：Leetcode 1929 -----
 * 原陣列: 1 2 1
 * 結果:   1 2 1 1 2 1
 * 原陣列: 1 3 2 1
 * 結果:   1 3 2 1 1 3 2 1
 * ----- 範例 3：Leetcode 1572 對角線和 -----
 * 對角線和 = 25
 * ----- 範例 4：Timer 計時器 -----
 * 總計 300 ms / 3 圈, 平均 100 ms
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 成員 = 成員變數 (狀態) + 成員函式 (行為)。
 *   2. 函式可以「在 class 內定義」（簡單時最常用）或「在 class 外定義」(用 ClassName::)。
 *   3. 在 class 內定義的成員函式預設有 inline 屬性，編譯器可選擇展開。
 *   4. 成員函式可加上 const，表示「不會改動成員變數」，常見於只讀取狀態的函式。
 *   5. static 成員函式不屬於某個物件，呼叫時不需建立物件 (例：ArrayConcat::print)。
 *
 * 【下一篇預告】
 *   3_Encapsulation.cpp
 *   學「封裝」(public / private)，把資料保護起來，
 *   並用 Leetcode 1603. Design Parking System 練習。
 *=============================================================================*/
