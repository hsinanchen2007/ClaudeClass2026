// =============================================================================
//  第 16 課：初始化列表 2  —  必用場景之一：const 成員
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  class X { const int id; public: X(int i) : id(i) {} };
//   標準版本：C++98 起即有；C++11 起也可用類別內預設初始化 const int id = 0;
//   複雜度：O(1)
//   標頭檔：<string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 const 成員不能在函數體內賦值】
//   const 的意思是「這個物件誕生之後，值不可以再改變」。
//   而函數體內的 studentId = id; 是**賦值**——賦值的定義就是「改變一個
//   已存在物件的值」。對 const 物件做這件事，語意上直接矛盾，
//   所以編譯器一定拒絕：
//       error: assignment of read-only member 'Student::studentId'
//   唯一能給 const 成員值的時機，是它「誕生的那一刻」，
//   也就是初始化——而類別成員的初始化只發生在初始化列表。
//
// 【2. 這件事的另一面：const 成員一定要被初始化】
//   因為之後不能再改，所以編譯器要求你在建構時就給定值。
//   如果一個 const 成員既沒寫在初始化列表、也沒有類別內預設初始值，
//   而且它的型別沒有預設建構函數（int 就是這種），編譯器會直接報錯：
//       error: uninitialized const member in 'class X'
//   這其實是好事：它讓「忘了初始化」變成編譯期錯誤，而不是執行期的不定值。
//
// 【3. const 成員帶來的副作用：複製指派會被停用】
//   這是很多人沒想到的地方。編譯器自動生成的 operator= 會逐成員賦值，
//   但 const 成員不能被賦值，所以**隱式的複製指派運算子會被定義為 deleted**：
//       Student a(1, "甲"), b(2, "乙");
//       a = b;      // 編譯錯誤：use of deleted function 'operator='
//   複製**建構**仍然可以（那是初始化，不是賦值），但指派不行。
//   影響很實際：這種類別放進 std::vector 後，任何需要「賦值」的操作
//   （例如 sort、erase 造成的元素搬移、resize）都會編譯失敗。
//   本檔在輸出中實際示範「複製建構可以、複製指派不行」的對比。
//
// 【4. 那什麼時候該用 const 成員】
//   ● 適合：真正的不變量，例如物件的身分（學號、UUID）、
//     建立後就固定的容量或設定。
//   ● 不適合：只是「我希望別人不要亂改」的欄位——這種用 private + 只提供
//     getter 就好，既能防止外部修改，又保留類別內部的彈性與可指派性。
//   實務上，如果類別需要放進容器並被排序／搬移，通常會避免 const 成員，
//   改用 private 加 getter。
//
// 【概念補充 Concept Deep Dive】
//
//   ● const 成員與「邏輯上的常數」不同
//     const 成員是物件層級的：每個物件都有自己的一份，只是各自不可改。
//     若你要的是「全類別共用、且是常數」，那應該用
//         static const int kMax = 100;      // 或 static constexpr
//     兩者概念完全不同：前者每個物件一份，後者整個類別一份。
//
//   ● const 成員與移動語意
//     移動建構／移動指派對 const 成員也無能為力——const 成員無法被「搬走」，
//     只能複製。所以 const 成員也會讓移動退化成複製。
//
//   ● C++11 之後的替代寫法
//         const int studentId = 0;   // 類別內預設初始化
//     這讓 const 成員即使沒寫在初始化列表也有值；但初始化列表若有指定，
//     會覆蓋這個預設值。實務上兩者常搭配使用。
//
//   ● 為什麼「初始化」不受 const 限制
//     因為初始化不是修改，而是「賦予物件誕生時的值」。C++ 的物件模型裡，
//     一個 const 物件在生命週期開始的那一刻寫入初值，之後才進入不可變狀態。
//
// 【注意事項 Pay Attention】
//   1. const 成員會使隱式的複製指派運算子被刪除，影響容器操作。
//   2. 「不想被外部改」通常用 private + getter 就夠，不必動用 const 成員。
//   3. 全類別共用的常數請用 static constexpr，不要用 const 成員。
//   4. const 成員未初始化且型別無預設建構函數時，是編譯期錯誤而非執行期問題。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】const 成員
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 const 成員只能用初始化列表，不能在建構函數本體內設定？
//     答：函數體內的 = 是賦值，也就是修改一個已存在的物件，這與 const
//         「誕生後不可改變」直接矛盾。const 成員唯一能取得值的時機是初始化，
//         而類別成員的初始化只發生在初始化列表（或類別內預設初始化）。
//     追問：那 const 成員可以完全不初始化嗎？
//         → 不行。若型別沒有預設建構函數（例如 int），
//           編譯器會報 uninitialized const member。
//
// 🔥 Q2. 類別裡加了一個 const 成員，會對這個類別產生什麼連帶影響？
//     答：隱式的複製指派運算子（operator=）會被定義為 deleted，因為它需要
//         逐成員賦值，而 const 成員不能被賦值。複製**建構**仍然可用。
//         連帶地，移動指派也不可用，且該類別放進 vector 後無法 sort、
//         無法透過賦值搬移元素。
//     追問：那要怎麼辦？
//         → 若類別需要可指派，就別用 const 成員，改成 private + getter；
//           真正需要不變量時才付出這個代價。
//
// ⚠️ 陷阱. const 成員的類別，放進 std::vector 完全沒問題吧？反正只是存進去？
//     答：push_back 通常沒問題（那是複製／移動建構），但只要碰到需要
//         **賦值**的操作就會編譯失敗——sort、remove、erase 造成的元素前移、
//         resize 等等都會用到 operator=。
//     為什麼會錯：把「放進容器」想成只有一次複製。實際上標準容器的許多演算法
//         是靠賦值來搬移元素的，而 const 成員正好把賦值這條路封死了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Student {
private:
    const int studentId;  // const 成員：一旦初始化就不能修改
    string name;

public:
    // 錯誤寫法（函數體內賦值）：
    //   Student(int id, const string& n) {
    //       studentId = id;   // error: assignment of read-only member
    //       name = n;
    //   }

    // 正確寫法：const 成員只能在初始化列表初始化
    Student(int id, const string& n)
        : studentId(id), name(n)
    { }

    int getId() const { return studentId; }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

// -----------------------------------------------------------------------------
// 對照組：把 const 拿掉，改用 private + getter
//   同樣達到「外部不能改」的效果，但保留了可指派性
// -----------------------------------------------------------------------------
class StudentAssignable {
private:
    int studentId;        // 沒有 const，但也沒有 setter
    string name;

public:
    StudentAssignable(int id, const string& n)
        : studentId(id), name(n)
    { }

    int getId() const { return studentId; }

    void print() const {
        cout << "  學號: " << studentId << ", 姓名: " << name << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】稽核日誌條目：事件 ID 是真正的不變量
//   情境：稽核系統的每一筆紀錄，一旦寫入就必須保證 ID 與時間戳不被竄改，
//         這是合規需求（誰在什麼時候做了什麼，不可事後修改）。
//   重點：這是 const 成員少數真正適合的場景——不變性是需求本身，
//         而不是「我希望別人不要亂改」。代價是這個型別不可指派，
//         所以容器操作要挑不需要賦值的做法（例如只 push_back、不 sort）。
// -----------------------------------------------------------------------------
class AuditEntry {
private:
    const long eventId;      // 事件序號：合規要求不可變更
    const string timestamp;  // 事件時間：同上
    string detail;           // 描述可以事後補充，不設 const

public:
    AuditEntry(long id, const string& ts, const string& d)
        : eventId(id), timestamp(ts), detail(d)
    { }

    void appendDetail(const string& more) { detail += "; " + more; }

    void print() const {
        cout << "  #" << eventId << " [" << timestamp << "] " << detail << endl;
    }
};

int main() {
    cout << "=== const 成員：必須用初始化列表 ===" << endl;
    Student s(20250001, "張三");
    s.print();

    // s.studentId = 999;   // 編譯錯誤：const 不能修改（而且它是 private）

    cout << "\n=== const 成員的連帶影響：複製建構 OK ===" << endl;
    Student copy = s;        // 複製建構是「初始化」，合法
    copy.print();

    cout << "\n=== const 成員的連帶影響：複製指派被刪除 ===" << endl;
    Student other(20250002, "李四");
    // copy = other;         // 編譯錯誤：use of deleted function 'operator='
    cout << "  copy = other; 會編譯失敗（隱式 operator= 因 const 成員被刪除）" << endl;
    other.print();

    cout << "\n=== 對照：改用 private + getter 就可以指派 ===" << endl;
    StudentAssignable a1(30000001, "王五");
    StudentAssignable a2(30000002, "趙六");
    a1 = a2;                 // 合法：沒有 const 成員
    cout << "  a1 = a2; 成功，a1 現在是：";
    a1.print();

    cout << "\n=== 可指派的版本才能放進容器做排序等操作 ===" << endl;
    vector<StudentAssignable> roster;
    roster.push_back(StudentAssignable(30000005, "錢七"));
    roster.push_back(StudentAssignable(30000003, "孫八"));
    cout << "  容器內共 " << roster.size() << " 筆：" << endl;
    for (const auto& st : roster) st.print();

    cout << "\n=== 日常實務：稽核日誌（不變性是需求本身）===" << endl;
    AuditEntry e1(1001, "2026-07-20 09:15:02", "使用者 alice 登入");
    AuditEntry e2(1002, "2026-07-20 09:16:44", "設定變更 retention=30d");
    e2.appendDetail("核准者 bob");     // 描述可補充，ID 與時間不可改
    e1.print();
    e2.print();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 16 課：建構函數初始化列表（Member Initializer List）2.cpp" -o demo2

// === 預期輸出 ===
// === const 成員：必須用初始化列表 ===
//   學號: 20250001, 姓名: 張三
//
// === const 成員的連帶影響：複製建構 OK ===
//   學號: 20250001, 姓名: 張三
//
// === const 成員的連帶影響：複製指派被刪除 ===
//   copy = other; 會編譯失敗（隱式 operator= 因 const 成員被刪除）
//   學號: 20250002, 姓名: 李四
//
// === 對照：改用 private + getter 就可以指派 ===
//   a1 = a2; 成功，a1 現在是：  學號: 30000002, 姓名: 趙六
//
// === 可指派的版本才能放進容器做排序等操作 ===
//   容器內共 2 筆：
//   學號: 30000005, 姓名: 錢七
//   學號: 30000003, 姓名: 孫八
//
// === 日常實務：稽核日誌（不變性是需求本身）===
//   #1001 [2026-07-20 09:15:02] 使用者 alice 登入
//   #1002 [2026-07-20 09:16:44] 設定變更 retention=30d; 核准者 bob
