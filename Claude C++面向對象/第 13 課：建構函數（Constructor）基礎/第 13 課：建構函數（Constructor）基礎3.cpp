// =============================================================================
//  第 13 課：建構函數（Constructor）基礎 3  —  第一個 constructor：它何時被呼叫
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : class T { public: T() { /* 初始化 */ } };
//   規則      : 函式名 = 類別名；沒有回傳型別（連 void 都不能寫）；不能手動呼叫
//   標準版本  : C++98 起
//   標頭檔    : <iostream>、<string>
//   複雜度    : 由 constructor 內容決定；本例是 O(1)
//
// 【詳細解釋 Explanation】
//
// 【1. constructor 為什麼「沒有回傳型別」？】
//   這不是語法潔癖，而是語意上的必然。一般函式的回傳值是「呼叫者拿到的結果」，
//   但 constructor 的結果就是那個物件本身——它被寫進呼叫端指定的那塊記憶體裡。
//   呼叫端不會「接收」什麼，所以沒有東西可以回傳。
//   反過來說，一旦你寫了 void Student() { }，它就變成一個「剛好和類別同名的
//   普通成員函式」，編譯器不會抱怨，但它再也不會被自動呼叫——
//   而且此時類別又回到「沒有使用者宣告的 constructor」的狀態。
//   這是初學者最難自己發現的錯誤之一，因為它「編得過」。
//
// 【2. 「自動呼叫」到底是什麼意思？】
//   Student s; 這一行，編譯器實際上做了兩件事：
//     (a) 在 stack frame 上保留 sizeof(Student) 的空間（純粹是位址計算，不花時間）
//     (b) 產生一段呼叫 Student::Student(&s) 的程式碼
//   注意 (b) 是**編譯期就決定好的**，不是執行期查表。
//   所以「自動」不代表有額外開銷，它只是編譯器替你寫了那行呼叫。
//
// 【3. constructor 內「賦值」與「初始化」的差別（本檔埋的伏筆）】
//   本檔寫的是：
//       Student() { name = "未命名"; age = 0; }
//   實際發生的順序是：
//       1. 先對 name 執行 default-initialization → 呼叫 string 的預設建構，得到空字串
//       2. 進入 constructor 本體
//       3. 執行 name = "未命名" → 呼叫 string 的 operator=，把內容換掉
//   也就是說 name 被「建構一次、再賦值一次」。對 std::string 這種便宜的型別
//   影響不大，但對重量級成員就是實實在在的浪費。
//   正解是成員初始化列表（第 15 課）：
//       Student() : name("未命名"), age(0) {}   // 直接建構成想要的值，只做一次
//   更關鍵的是：const 成員與 reference 成員**只能**用初始化列表，
//   因為它們沒有「先預設建構、再賦值」這個選項。
//
// 【4. 為什麼 print() 要加 const？】
//   void print() const 承諾「我不會修改任何成員」。加了它之後：
//     * const Student cs; cs.print();  才能編譯
//     * 傳 const Student& 進函式後才能呼叫
//   規則很簡單：不修改狀態的成員函式一律加 const，這是零成本的正確性保證。
//
// 【概念補充 Concept Deep Dive】
//   ▍constructor 不能被明確呼叫
//     s.Student(); 是語法錯誤。想在既有記憶體上重新建構物件要用 placement new，
//     那是完全不同的機制，且必須自己負責先呼叫 destructor。
//
//   ▍constructor 執行順序（完整版）
//     1. base class 的 constructor（依繼承宣告順序）
//     2. 非靜態成員的初始化（**依成員在類別中的宣告順序**，不是初始化列表的書寫順序）
//     3. constructor 本體 { } 內的程式碼
//     destructor 則完全相反。第 2 點是面試常考點，寫錯順序 GCC 會給 -Wreorder 警告。
//
//   ▍constructor 是「物件生命週期的起點」
//     標準規定：constructor 成功結束後，物件的生命週期才開始。
//     在此之前把 this 指標傳出去給別的執行緒使用，是典型的錯誤。
//
// 【注意事項 Pay Attention】
//   1. void Student() { } 不是 constructor，是普通成員函式——編得過，但不會自動執行。
//   2. constructor 中的 cout 只是為了教學觀察；正式程式碼不該在 constructor 裡做 I/O。
//   3. constructor 內用「賦值」而非「初始化列表」，對 const/reference 成員直接編譯失敗。
//   4. 一旦宣告了任何 constructor，編譯器就不再自動生成預設 constructor（第 14 課主題）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】constructor 的基本規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. constructor 為什麼不能有回傳型別？可以是 virtual 嗎？
//     答：沒有回傳型別，是因為它的產物就是物件本身，呼叫端沒有東西可接收。
//         也不能是 virtual：virtual 分派要靠物件的 vptr，而 vptr 正是在
//         constructor 執行過程中才被設定好的——「還沒建好就要靠建構結果分派」
//         邏輯上不成立。要依型別動態建立物件請用 factory 或 clone 慣用法。
//     追問：那 destructor 為什麼可以是 virtual？→ 解構時物件已完整建構，
//         vptr 是有效的，所以分派得起來。
//
// 🔥 Q2. constructor 本體裡寫 name = "x"，和寫在初始化列表 : name("x")，差在哪？
//     答：本體內是「先預設建構、再賦值」兩個步驟；初始化列表是「直接建構成目標值」
//         一個步驟。前者對重量級成員有額外成本，而且 const 成員與 reference 成員
//         根本不能用賦值——它們只能在初始化列表中初始化。
//     追問：初始化列表的順序重要嗎？→ 重要但不是你寫的順序決定的；
//         實際執行順序永遠是成員的宣告順序，寫反會拿到 -Wreorder 警告。
//
// ⚠️ 陷阱. 有人寫 void Student() { ... }，然後抱怨「建構函數沒有被呼叫」。
//     答：因為那根本不是 constructor。加上回傳型別之後，它變成一個名字剛好
//         叫 Student 的普通成員函式。編譯器完全不會報錯，物件建立時也不會執行它。
//         而且此時類別沒有使用者宣告的 constructor，編譯器會另外生成一個預設的。
//     為什麼會錯：以為「函式名和類別同名」就足以構成 constructor。
//         真正的判準是「同名 **且** 沒有回傳型別」，兩個條件缺一不可。
// ═══════════════════════════════════════════════════════════════════════════

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
    // 沒有返回值（連 void 都不寫）—— 寫了就變成普通成員函式
    Student() {
        cout << ">>> 建構函數被調用了！ <<<" << endl;
        name = "未命名";      // 注意：這是「賦值」，name 在此之前已被預設建構過一次
        age = 0;
        gpa = 0.0f;
    }

    void print() const {      // 不修改狀態 → 加 const
        cout << "姓名: " << name
             << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// 對照組：用成員初始化列表，只建構一次（第 15 課的正式主題）
class StudentInitList {
private:
    string name;
    int age;
    float gpa;

public:
    StudentInitList() : name("未命名"), age(0), gpa(0.0f) {
        cout << ">>> 初始化列表版建構函數 <<<" << endl;
    }

    void print() const {
        cout << "姓名: " << name << ", 年齡: " << age
             << ", GPA: " << gpa << endl;
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】從缺
//   理由：本檔的主題是「constructor 的語法規則與自動呼叫時機」，是語言機制，
//         不是可以拿題目來練的資料結構問題。LeetCode 的設計題（146、155、232…）
//         都預設你已經會寫 constructor，考的是資料結構本身。
//         本課的設計題實作放在 summary.cpp，那裡才是合適的位置。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】應用程式的日誌器（Logger）
//   情境：幾乎每個服務啟動時都要先決定「日誌等級」與「輸出前綴」。
//         若採用「先建立物件、再呼叫 setLevel()」的兩段式流程，
//         任何在 setLevel() 之前發生的錯誤都會用到未設定的等級。
//   作法：把這些決定放進 constructor，物件一存在就處於可用狀態。
//         這正是 constructor 最典型的日常用途——建立不變式（invariant）。
// -----------------------------------------------------------------------------
class Logger {
public:
    enum Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

private:
    string prefix_;
    Level  minLevel_;
    int    emitted_ = 0;     // NSDMI：計數器一定從 0 開始

public:
    // 一建立就決定好「我是誰」「我要記錄到多細」，之後不會有半成品狀態
    Logger(const string& prefix, Level minLevel)
        : prefix_(prefix), minLevel_(minLevel) {
        cout << "  [Logger 建立] prefix=" << prefix_
             << ", minLevel=" << levelName(minLevel_) << endl;
    }

    static const char* levelName(Level lv) {
        switch (lv) {
            case DEBUG: return "DEBUG";
            case INFO:  return "INFO";
            case WARN:  return "WARN";
            case ERROR: return "ERROR";
        }
        return "?";
    }

    void log(Level lv, const string& msg) {
        if (lv < minLevel_) return;          // 低於門檻就丟棄
        ++emitted_;
        cout << "  [" << prefix_ << "][" << levelName(lv) << "] " << msg << endl;
    }

    int emitted() const { return emitted_; }
};

int main() {
    cout << "=== 觀察 constructor 的呼叫時機 ===" << endl;
    cout << "（下一行才會建立物件）" << endl;

    Student s;   // 建立物件的瞬間，constructor 自動被呼叫

    cout << "=== 物件已建立 ===" << endl;
    s.print();   // 成員已被初始化，不再是不確定值

    cout << "\n=== 對照：初始化列表版本 ===" << endl;
    StudentInitList s2;
    s2.print();
    cout << "（兩者輸出相同，但初始化列表版少了一次多餘的賦值）" << endl;

    cout << "\n=== 日常實務：Logger 一建立就可用 ===" << endl;
    Logger appLog("app", Logger::INFO);
    appLog.log(Logger::DEBUG, "這行低於門檻，會被丟棄");
    appLog.log(Logger::INFO,  "服務啟動完成");
    appLog.log(Logger::ERROR, "資料庫連線逾時");
    cout << "  實際輸出筆數 = " << appLog.emitted() << " / 送進來 3 筆" << endl;

    cout << "\n=== 同一個類別，不同門檻 ===" << endl;
    Logger dbLog("db", Logger::ERROR);
    dbLog.log(Logger::WARN,  "查詢偏慢（低於 ERROR 門檻，丟棄）");
    dbLog.log(Logger::ERROR, "連線池耗盡");
    cout << "  實際輸出筆數 = " << dbLog.emitted() << " / 送進來 2 筆" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 13 課：建構函數（Constructor）基礎3.cpp" -o ctor3

// === 預期輸出 ===
// === 觀察 constructor 的呼叫時機 ===
// （下一行才會建立物件）
// >>> 建構函數被調用了！ <<<
// === 物件已建立 ===
// 姓名: 未命名, 年齡: 0, GPA: 0
//
// === 對照：初始化列表版本 ===
// >>> 初始化列表版建構函數 <<<
// 姓名: 未命名, 年齡: 0, GPA: 0
// （兩者輸出相同，但初始化列表版少了一次多餘的賦值）
//
// === 日常實務：Logger 一建立就可用 ===
//   [Logger 建立] prefix=app, minLevel=INFO
//   [app][INFO] 服務啟動完成
//   [app][ERROR] 資料庫連線逾時
//   實際輸出筆數 = 2 / 送進來 3 筆
//
// === 同一個類別，不同門檻 ===
//   [Logger 建立] prefix=db, minLevel=ERROR
//   [db][ERROR] 連線池耗盡
//   實際輸出筆數 = 1 / 送進來 2 筆
