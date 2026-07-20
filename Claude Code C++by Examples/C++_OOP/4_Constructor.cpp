/*=============================================================================
 * 檔名：4_Constructor.cpp
 * 主題：建構子 (Constructor) - 物件出生時自動執行的特殊函式
 * 適合：已會 class、private/public，但物件成員還在用 set 來初始化的初學者
 *
 * 【課題介紹】
 *   前幾篇我們建立物件後，都要再呼叫 setter 才能把成員變數設好：
 *
 *       BankAccount acc;
 *       acc.setOwner("Alice");
 *       acc.setInitialBalance(0);
 *
 *   這樣的問題：
 *     1. 容易忘記初始化 → 導致未定義行為。
 *     2. 一個物件分兩三步驟才完工，邏輯散落各處。
 *
 *   解決辦法：用「建構子 (Constructor)」。
 *
 *       「建構子是一個特殊的成員函式，物件被建立時 C++ 會自動呼叫它。」
 *
 *   只要在建構子內把成員變數設好，就能保證每個物件「一誕生就是合法狀態」。
 *
 * 【建構子的規則】
 *   1. 函式名「必須」與類別同名。
 *   2. 不寫回傳型別 (連 void 都不寫)。
 *   3. 可以多載 (overload) 多個版本，吃不同參數。
 *   4. 不傳參數的版本叫「預設建構子 (default constructor)」。
 *   5. 你不寫的話，編譯器在某些條件下會幫你產一個預設建構子；
 *      一旦你自己寫了任何建構子，編譯器就「不會再」幫你補預設建構子，要自己加。
 *
 * 【三種常見建構方式】
 *   建構子可以這樣呼叫：
 *       MyClass a;             // 預設建構 (default-init)
 *       MyClass b(1, 2);       // 直接初始化 (direct-init)
 *       MyClass c{1, 2};       // C++11 統一初始化 (uniform / brace-init)，推薦
 *       MyClass d = {1, 2};    // 拷貝列表初始化 (copy-list-init)
 *   現代寫法推薦用 {} 大括號 (brace-init)，能避免一個叫做
 *   「最棘手解析 (most vexing parse)」的奇怪坑。
 *
 *   坑的範例（不要這樣寫）：
 *       MyClass e();   ← 看起來像建構子？其實會被當成「函式宣告」，不是物件！
 *
 * 【對應 Leetcode】1656. Design an Ordered Stream
 *   題目簡述：
 *     設計一個類別 OrderedStream(n)，它有 n 個格子（編號 1..n）。
 *     呼叫 insert(idKey, value) 把 value 放到第 idKey 格，
 *     並從目前指標開始，回傳所有「連續已填滿」的格子內容。
 *   為什麼選這題：建構時就需要決定大小 n，並把儲存空間配好，
 *   是「建構子帶參數初始化」的經典場景。
 *
 * 【參考】
 *   https://en.cppreference.com/w/cpp/language/constructor
 *   https://cplusplus.com/doc/tutorial/classes/
 *=============================================================================*/

/*
補充筆記：Constructor
  - Constructor 這類 OOP 範例要追蹤物件狀態：建構後是否有效、操作後是否仍符合類別承諾。
  - 如果類別擁有資源，就要檢查 destructor、copy、move 是否表達同一套所有權規則。
  - 繼承、friend、static、operator overload 都應服務於清楚的物件語意，而不是只展示語法。
  - constructor 是物件生命週期的起點，任務是讓物件一出生就進入可用狀態；不要依賴使用者建立物件後再記得呼叫 init()。
  - 預設建構子沒有參數；如果你宣告了其他建構子，編譯器不一定再自動產生無參數建構子，這會影響 Student s; 這類寫法能不能編譯。
  - 建構子沒有回傳型別，連 void 都不能寫；寫了回傳型別會變成語法錯誤，因為建構子不是一般函式。
  - 成員初始化應優先使用 member initializer list，而不是在建構子本體內賦值；前者是直接初始化，後者是先預設建構再指派。
  - const 成員、reference 成員、沒有預設建構子的成員，必須在初始化列表初始化，不能等到建構子本體再賦值。
  - 建構順序依照成員在 class 內宣告的順序，不是初始化列表書寫順序；若成員彼此相依，宣告順序要刻意安排。
  - 單參數建構子可能造成隱式轉換，例如 Money m = 10;；若這不是你要的語意，應加 explicit 避免呼叫端不小心轉型。
  - 建構子若拋例外，物件不會完成建立；已成功建構的成員會自動解構，所以資源成員應交給 RAII 型別管理。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構子（Constructor）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 建構與解構的順序是什麼？
//     答：建構：base class → 非靜態成員（依「在 class 中的宣告順序」，不是初始化列表
//     的書寫順序）→ 建構子本體。解構完全相反。
//     追問：初始化列表順序寫反會怎樣？（不會照你寫的順序執行，可能用到尚未初始化的
//     成員；GCC/Clang 會給 -Wreorder 警告）
//
// 🔥 Q2. constructor 可以是 virtual 嗎？為什麼？
//     答：不行。virtual 分派要靠物件的 vptr，而 vptr 是在建構過程中才被設定 —
//     「還沒建好就要靠建構結果去分派」邏輯上不成立。想「依型別動態建立物件」，
//     用 factory pattern 或 clone 慣用法（virtual constructor idiom）。
//     追問：destructor 為什麼就可以是 virtual？（解構時物件已完整建構，vptr 有效）
//
// 🔥 Q3. 在 constructor 裡呼叫 virtual function 會發生什麼事？
//     答：不是「不能」，而是「不會如你預期」。執行 Base 的 constructor 時，物件的動態
//     型別就是 Base，呼叫到的是 Base 的版本，derived 的 override 不會生效
//     （CERT OOP50-CPP 明文禁止此寫法）。若呼叫的是 pure virtual 函式則是 UB。
//     追問：那要做「建構後才能執行的初始化」怎麼辦？（two-phase init：建構完再由外部
//     呼叫 init()，並用 factory function 把這兩步包起來，別讓使用者自己記得）
//
// Q4. explicit 的作用是什麼？
//     答：單參數（或其餘參數都有預設值）的建構子會成為隱式轉換的來源，
//     `Money m = 10;` 可能不是你要的語意。加上 explicit 後就只允許直接初始化
//     `Money m(10);` 或 `Money m{10};`，避免呼叫端不小心轉型。
//
// ⚠️ 陷阱. `MyClass e();` 建立了一個物件嗎？
//     答：沒有。它被解析成「一個不吃參數、回傳 MyClass 的函式宣告」— 這就是
//     most vexing parse。要預設建構請寫 `MyClass e;` 或 `MyClass e{};`。
//     為什麼會錯：以為括號跟 `MyClass e(1, 2)` 一樣是在呼叫建構子；C++ 文法規定
//     「只要能解析成宣告，就當作宣告」。
//
// ⚠️ 陷阱. `using Base::Base;`（C++11 繼承建構子）會初始化 derived 自己的成員嗎？
//     答：不會。繼承來的建構子只初始化 base 子物件，derived 自己新增的成員完全不碰，
//     處於未初始化狀態，讀取即 UB，而且編譯器通常不會警告。本機實測
//     struct D : Base { using Base::Base; int extra; }; D d(7); → d.b == 7，
//     但 d.extra 是不定值（可能剛好讀到 0，這正是它危險的地方）。給 derived 成員加上
//     NSDMI（int extra = 42;）即可修正。
//     為什麼會錯：把它跟 `using Base::f;` 混為一談——後者是把被 derived 同名成員隱藏
//     掉的 base 多載拉回 overload set（解 name hiding），與初始化無關。另注意 copy／
//     move constructor 不在繼承範圍內，derived 仍依一般規則自行生成；base 建構子若是
//     private／protected，存取權會一併繼承過來，不會被放寬。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 範例 1：點 (Point) - 認識多個建構子
// -----------------------------------------------------------------------------
class Point {
private:
    double x_;
    double y_;

public:
    // (1) 預設建構子 (default constructor)：不吃參數，預設座標 (0,0)
    Point() {
        x_ = 0;
        y_ = 0;
        std::cout << "Point() 預設建構子被呼叫" << std::endl;
    }

    // (2) 帶參數建構子：可以指定初始座標
    Point(double x, double y) {
        x_ = x;
        y_ = y;
        std::cout << "Point(x, y) 被呼叫，x=" << x_ << " y=" << y_ << std::endl;
    }

    // 一個簡單的顯示函式
    void show() const {
        std::cout << "(" << x_ << ", " << y_ << ")" << std::endl;
    }
};

// -----------------------------------------------------------------------------
// 範例 2：對應 Leetcode 1656 - Design an Ordered Stream
// -----------------------------------------------------------------------------
// 設計概念：
//   - 用一個 vector<string> 當儲存格 (空字串代表還沒填)。
//   - 用一個 ptr_ 紀錄「下次要從哪一格開始輸出」。
//   - 每次 insert，把值放到對應位置，然後從 ptr_ 開始把連續已填的全部撈出來。
class OrderedStream {
private:
    std::vector<std::string> stream_;   // 1-based 容易理解，所以多開一格
    int                       ptr_;     // 下次要回傳的起點 (1-based)

public:
    // 建構子：給定 n，初始化內部資料
    // 注意這裡 stream_ 用大小 n+1 (因為題目用 1..n 編號，第 0 格不用)
    OrderedStream(int n) {
        stream_.assign(n + 1, "");      // n+1 格，全部初始化為空字串
        ptr_ = 1;                       // 從 1 開始
    }

    // insert 行為：放進 idKey 格，並回傳「目前可以連續輸出的字串」
    std::vector<std::string> insert(int idKey, std::string value) {
        stream_[idKey] = std::move(value);   // move 比 copy 高效，第 22 篇詳述

        std::vector<std::string> out;
        // 只要目前 ptr_ 那格已被填了 (非空字串)，就一直往後吐
        while (ptr_ < static_cast<int>(stream_.size()) && !stream_[ptr_].empty()) {
            out.push_back(stream_[ptr_]);
            ++ptr_;
        }
        return out;
    }
};

// 工具：印出 vector<string>
static void printVec(const std::vector<std::string>& v) {
    std::cout << "[ ";
    for (const auto& s : v) std::cout << "\"" << s << "\" ";
    std::cout << "]" << std::endl;
}

// -----------------------------------------------------------------------------
// 範例 3：對應 Leetcode 232 - Implement Queue using Stacks
// -----------------------------------------------------------------------------
// 題目簡述：用兩個 stack 實作 queue (FIFO)，支援 push / pop / peek / empty。
// 為什麼選這題：建構子裡不用做太多事，但「成員物件 (兩個 stack) 怎麼自動建構」
//               正好示範了「成員的預設建構子自動呼叫」這個重要觀念。
class MyQueue {
private:
    std::vector<int> inStk_;        // 用 vector 模擬 stack: push 寫入這裡
    std::vector<int> outStk_;       // pop 從這裡拿，空了再從 inStk_ 倒過來

    void shift() {
        // 把 inStk_ 全部倒進 outStk_，順序自動翻轉就成了 queue 順序
        while (!inStk_.empty()) {
            outStk_.push_back(inStk_.back());
            inStk_.pop_back();
        }
    }

public:
    // 不寫建構子也行，編譯器會自動產生預設建構子，並把 vector 都預設建構為空。
    // 為了示範「明確寫出建構子也可以」這裡寫一個。
    MyQueue() {
        std::cout << "MyQueue() 建構子被呼叫" << std::endl;
    }

    void push(int x) { inStk_.push_back(x); }

    int pop() {
        if (outStk_.empty()) shift();
        int v = outStk_.back();
        outStk_.pop_back();
        return v;
    }

    int peek() {
        if (outStk_.empty()) shift();
        return outStk_.back();
    }

    bool empty() const { return inStk_.empty() && outStk_.empty(); }
};

// -----------------------------------------------------------------------------
// 範例 4：日常實用 - Event 事件物件
// -----------------------------------------------------------------------------
// 工作上很常用：把一個事件 (時間戳記 + 類型 + 訊息) 包成一個物件。
// 用「建構子初始化列表」一行設好所有欄位，是建構子的標準寫法。
class Event {
private:
    long        timestamp_;
    std::string type_;
    std::string message_;

public:
    // 用「初始化列表」一次設好所有成員 (第 7 篇會深入講)
    Event(long ts, const std::string& type, const std::string& msg)
        : timestamp_(ts), type_(type), message_(msg) {}

    void print() const {
        std::cout << "[" << timestamp_ << "] " << type_ << ": " << message_ << std::endl;
    }
};

int main() {
    std::cout << "----- 範例 1：Point 多個建構子 -----" << std::endl;

    Point p1;                  // 呼叫預設建構子
    p1.show();                 // (0, 0)

    Point p2(3.5, 4.2);        // 呼叫帶參數的建構子，傳統寫法
    p2.show();                 // (3.5, 4.2)

    Point p3{1.0, 2.0};        // 推薦寫法：大括號統一初始化
    p3.show();                 // (1, 2)

    // Point p4();             // ← 不要這樣寫！這會被當成函式宣告，叫做「最棘手解析」

    std::cout << "----- 範例 2：Leetcode 1656 OrderedStream -----" << std::endl;

    // 模擬題目範例：n=5
    OrderedStream os(5);

    // 順序刻意打亂 (3, 1, 2, 5, 4)，看看每次 insert 回傳什麼
    printVec(os.insert(3, "ccccc"));   // ptr 還在 1 → []
    printVec(os.insert(1, "aaaaa"));   // ptr 從 1 開始能吐 1 → [aaaaa]
    printVec(os.insert(2, "bbbbb"));   // 接著 ptr=2,3 都填了 → [bbbbb, ccccc]
    printVec(os.insert(5, "eeeee"));   // 4 還沒到 → []
    printVec(os.insert(4, "ddddd"));   // 4,5 都齊了 → [ddddd, eeeee]

    std::cout << "----- 範例 3：Leetcode 232 用兩個 stack 做 queue -----" << std::endl;
    MyQueue q;
    q.push(1);
    q.push(2);
    q.push(3);
    std::cout << "peek = " << q.peek() << std::endl;     // 1 (先進)
    std::cout << "pop  = " << q.pop()  << std::endl;     // 1
    std::cout << "pop  = " << q.pop()  << std::endl;     // 2
    std::cout << "empty? " << q.empty() << std::endl;    // 0 (還有 3)

    std::cout << "----- 範例 4：Event 事件物件 -----" << std::endl;
    Event e1(1700000000, "INFO",  "服務已啟動");
    Event e2(1700000123, "ERROR", "資料庫連線逾時");
    e1.print();
    e2.print();

    return 0;
}

/* 預期輸出：
 * ----- 範例 1：Point 多個建構子 -----
 * Point() 預設建構子被呼叫
 * (0, 0)
 * Point(x, y) 被呼叫，x=3.5 y=4.2
 * (3.5, 4.2)
 * Point(x, y) 被呼叫，x=1 y=2
 * (1, 2)
 * ----- 範例 2：Leetcode 1656 OrderedStream -----
 * [ ]
 * [ "aaaaa" ]
 * [ "bbbbb" "ccccc" ]
 * [ ]
 * [ "ddddd" "eeeee" ]
 * ----- 範例 3：Leetcode 232 用兩個 stack 做 queue -----
 * MyQueue() 建構子被呼叫
 * peek = 1
 * pop  = 1
 * pop  = 2
 * empty? 0
 * ----- 範例 4：Event 事件物件 -----
 * [1700000000] INFO: 服務已啟動
 * [1700000123] ERROR: 資料庫連線逾時
 */

/*=============================================================================
 * 【本篇重點回顧】
 *   1. 建構子函式名 = 類別名，沒有回傳型別。
 *   2. 物件被建立時自動呼叫，可確保「物件一出生就有合法狀態」。
 *   3. 建構子可以多載多個版本，分別吃不同參數。
 *   4. 自己寫了帶參數的建構子之後，預設建構子不會再被自動產生，要自己加。
 *   5. 推薦用 ClassName obj{args...}; 大括號統一初始化，避免「最棘手解析」陷阱。
 *
 * 【下一篇預告】
 *   5_Destructor.cpp
 *   解構子 (Destructor) — 物件死亡時自動執行的清理函式，
 *   並用日常範例（自動關閉檔案）來看為什麼它很重要。
 *=============================================================================*/
