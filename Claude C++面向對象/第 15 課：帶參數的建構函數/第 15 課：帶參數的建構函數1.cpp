// =============================================================================
//  第 15 課：帶參數的建構函數 1  —  值傳遞 vs const 引用傳遞
// =============================================================================
//
// 【主題資訊 Information】
//   對比：  Student(string n, int a)          // 值傳遞：複製一份參數
//           Student(const string& n, int a)   // const 引用：不複製
//   標準版本：C++98 起即可；本檔的移動語意（std::move）需 C++11
//   標頭檔：<string>
//   準則：便宜可複製的型別（int/double/指標）用值傳遞；
//         有動態配置成本的型別（string/vector/map）用 const&
//
// 【詳細解釋 Explanation】
//
// 【1. 值傳遞到底複製了幾次？】
//   `Student(string n, int a)` 搭配 `name = n;` 這種寫法，實際發生：
//     (1) 呼叫端把實參複製進參數 n            ← 第一次複製
//     (2) 函式本體把 n 賦值給 name             ← 第二次複製
//   而 `StudentBetter(const string& n, int a)` 搭配初始化列表 `name(n)`：
//     (1) 參數 n 只是既有物件的別名           ← 零複製
//     (2) name 直接以 n 為來源建構            ← 一次複製（無可避免，要存起來）
//   所以差別是「兩次」對「一次」——本檔用計數器實測了這個差異。
//
// 【2. 為什麼是 const 引用而不是普通引用？】
//   普通引用 `string& n` 有兩個致命問題：
//     (a) 無法綁定臨時物件與字面值：`Student s("張三", 20)` 會直接編譯失敗，
//         因為 "張三" 產生的是臨時 string，不能綁到非 const 引用
//     (b) 語意錯誤：非 const 引用暗示「我可能會修改你的參數」，
//         但建構函式只是要讀取它
//   const 引用兩個問題都沒有：可以綁定臨時物件、字面值與 const 物件，
//   同時向讀者承諾「我不會改動它」。
//
// 【3. 什麼時候「不該」用 const&？】
//   對「複製成本本來就極低」的型別，const& 反而可能更慢：
//     * int / double / char / bool / 指標 / 小 enum
//     這些型別直接放進暫存器就好，改用引用等於多一次間接存取。
//   準則：sizeof 小於等於兩個指標大小、且可 trivially copy 的型別用值傳遞。
//   所以本檔的 `int a` 維持值傳遞是正確的，不需要改成 `const int&`。
//
// 【4. C++11 之後的第三個選項：值傳遞 + std::move】
//   若參數「一定會被存起來」，現代寫法是：
//       Student(string n, int a) : name(std::move(n)), age(a) {}
//   這樣：
//     * 呼叫端傳左值 → 一次複製 + 一次移動（移動很便宜）
//     * 呼叫端傳右值（臨時物件）→ 一次移動 + 一次移動，完全不複製
//   對比 const& 版本：傳右值時仍會複製一次（因為要從 const& 複製出來）。
//   所以「一定會存起來」的場景，值傳遞 + move 對右值更有利。
//   本檔的 StudentMove 完整示範，並用計數器對照三者差異。
//
// 【概念補充 Concept Deep Dive】
//   * const& 綁定臨時物件時，會延長該臨時物件的生命週期到引用的作用域結束 ——
//     但這只適用於「直接綁定到區域 const 引用」的情況。
//     把 const& 存成成員變數「不會」延長生命週期，那是懸空引用的經典來源。
//   * 上述複製次數在開了最佳化（-O2）後可能減少，因為編譯器有 copy elision
//     與 RVO 等機制。本檔的計數器是在 -O0（預設）下的實際觀察，
//     用來說明語意差異；實際效能請以 profiler 為準。
//   * 「值傳遞 + move」的寫法有個代價：它讓函式簽章看起來像會複製，
//     而且對「只讀不存」的參數是錯的選擇。用它之前先確認參數真的會被存起來。
//
// 【注意事項 Pay Attention】
//   1. 不要對 int/double 這類便宜型別用 const& —— 多一層間接反而慢。
//   2. 不要用非 const 引用當唯讀參數 —— 字面值與臨時物件會綁不上。
//   3. 絕不要把 const& 參數直接存成成員引用，除非你能保證來源活得比物件久。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】參數傳遞方式
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 建構函式的 std::string 參數，該用值傳遞還是 const 引用？
//     答：預設用 `const std::string&` —— 避免呼叫端到參數的那一次複製。
//         若這個參數「一定會被存進成員」，C++11 之後可考慮
//         「值傳遞 + std::move」：對右值引數能完全避免複製。
//         而 int/double 這類便宜型別維持值傳遞即可。
//     追問：為什麼不能全部都用 const&？→ 對小型 trivially copyable 的型別，
//           引用是多一層間接存取，反而比直接放暫存器慢。
//
// 🔥 Q2. 為什麼是 const 引用而不是普通引用？
//     答：兩個原因。① 普通引用無法綁定臨時物件與字面值，
//         `Student s("張三", 20)` 會直接編譯失敗；
//         ② 語意上非 const 引用代表「我可能會改你的參數」，與唯讀意圖不符。
//     追問：const& 綁到臨時物件安全嗎？→ 在函式參數的情境下安全 ——
//           臨時物件的生命週期涵蓋整個函式呼叫。但絕不能把它存成成員引用。
//
// ⚠️ 陷阱. 下面這個類別有什麼嚴重問題？
//         class Bad {
//             const std::string& m_name;      // 存成 const 引用成員
//         public:
//             Bad(const std::string& n) : m_name(n) {}
//         };
//         Bad b("臨時字串");
//     答：懸空引用。"臨時字串" 產生的臨時 string 在建構完成後就被銷毀，
//         m_name 從此指向已失效的記憶體，之後任何存取都是 UB。
//     為什麼會錯：知道「const& 會延長臨時物件的生命週期」，卻沒注意到
//         那條規則只適用於「直接綁定到區域 const 引用變數」。
//         透過建構函式參數再存進成員的情形完全不適用 ——
//         正解是讓成員是 `std::string` 而非 `const std::string&`。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <utility>      // std::move
#include <vector>
using namespace std;

// 會自報行蹤的字串包裝，用來實測複製與移動的次數
class TrackedString {
public:
    TrackedString() = default;
    TrackedString(const char* s) : m_data(s) { ++ctor; }              // NOLINT: 刻意允許隱式轉換
    TrackedString(const TrackedString& o) : m_data(o.m_data) { ++copies; }
    TrackedString(TrackedString&& o) noexcept : m_data(std::move(o.m_data)) { ++moves; }
    TrackedString& operator=(const TrackedString& o) { m_data = o.m_data; ++copyAssign; return *this; }
    TrackedString& operator=(TrackedString&& o) noexcept { m_data = std::move(o.m_data); ++moveAssign; return *this; }

    const string& str() const { return m_data; }

    static int ctor, copies, moves, copyAssign, moveAssign;
    static void reset() { ctor = copies = moves = copyAssign = moveAssign = 0; }
    static void report(const char* label) {
        cout << "    " << label
             << " 複製建構 " << copies << " 次、移動建構 " << moves
             << " 次、複製賦值 " << copyAssign << " 次、移動賦值 " << moveAssign << " 次" << endl;
    }
private:
    string m_data;
};
int TrackedString::ctor = 0;
int TrackedString::copies = 0;
int TrackedString::moves = 0;
int TrackedString::copyAssign = 0;
int TrackedString::moveAssign = 0;

// ===== 方式 A：值傳遞 + 本體內賦值（最差：兩次複製）=====
class Student {
private:
    TrackedString name;
    int age;

public:
    // 每次調用都會複製一份，本體內又賦值一次
    Student(TrackedString n, int a) {
        name = n;    // 這裡又複製了一次（複製賦值）
        age = a;
    }
    void print() const {
        cout << "    " << name.str() << ", " << age << " 歲" << endl;
    }
};

// ===== 方式 B：const 引用 + 初始化列表（推薦通用做法）=====
class StudentBetter {
private:
    TrackedString name;
    int age;

public:
    // 不會複製參數，只在初始化列表複製一次進成員
    StudentBetter(const TrackedString& n, int a) : name(n), age(a) {}

    void print() const {
        cout << "    " << name.str() << ", " << age << " 歲" << endl;
    }
};

// ===== 方式 C：值傳遞 + std::move（C++11；參數一定會被存起來時最佳）=====
class StudentMove {
private:
    TrackedString name;
    int age;

public:
    // 傳左值：一次複製（進參數）+ 一次移動（進成員）
    // 傳右值：一次移動（進參數）+ 一次移動（進成員）→ 完全不複製
    StudentMove(TrackedString n, int a) : name(std::move(n)), age(a) {}

    void print() const {
        cout << "    " << name.str() << ", " << age << " 歲" << endl;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】日誌事件：高頻建立的物件，參數傳遞成本會被放大
//   情境：一個高流量服務每秒產生數萬筆日誌事件。每筆事件都要記錄
//   請求 ID、訊息與來源模組 —— 三個 std::string。
//   在這種每秒數萬次的熱路徑上，「每筆多複製三次字串」不是理論問題，
//   而是實際會出現在 profiler 上的 CPU 時間與記憶體配置壓力。
//   本例採用「值傳遞 + std::move」：
//     * 呼叫端多半傳的是剛組好的臨時字串（右值）→ 全程零複製
//     * 少數傳既有變數（左值）的情況，成本與 const& 版本相同
//   這是現代 C++ 對「參數一定會被存起來」的標準做法。
// -----------------------------------------------------------------------------
class LogEvent {
public:
    // 三個字串都會被存進成員 → 值傳遞 + move 是合適的選擇
    LogEvent(string requestId, string module, string message, int severity)
        : m_requestId(std::move(requestId)),
          m_module(std::move(module)),
          m_message(std::move(message)),
          m_severity(severity) {}

    string format() const {
        static const char* levels[] = {"DEBUG", "INFO", "WARN", "ERROR"};
        int idx = (m_severity < 0) ? 0 : (m_severity > 3 ? 3 : m_severity);
        return string("[") + levels[idx] + "][" + m_module + "] "
             + "req=" + m_requestId + " " + m_message;
    }

private:
    string m_requestId;    // 宣告順序 = 初始化順序
    string m_module;
    string m_message;
    int    m_severity;
};

int main() {
    TrackedString myName = "張三";

    cout << "=== 方式 A：值傳遞 + 本體內賦值 ===" << endl;
    TrackedString::reset();
    Student s1(myName, 20);
    s1.print();
    TrackedString::report("值傳遞");
    cout << "    ↑ 複製兩次：一次進參數、一次賦值給成員" << endl;

    cout << "\n=== 方式 B：const 引用 + 初始化列表 ===" << endl;
    TrackedString::reset();
    StudentBetter s2(myName, 20);
    s2.print();
    TrackedString::report("const 引用");
    cout << "    ↑ 只複製一次：參數不複製，直接以它建構成員" << endl;

    cout << "\n=== 方式 C：值傳遞 + std::move（傳左值）===" << endl;
    TrackedString::reset();
    StudentMove s3(myName, 20);
    s3.print();
    TrackedString::report("值+move（左值）");
    cout << "    ↑ 一次複製 + 一次移動，與 const& 版本成本相當" << endl;

    cout << "\n=== 方式 C：值傳遞 + std::move（傳右值）===" << endl;
    TrackedString::reset();
    StudentMove s4(TrackedString("李四"), 22);   // 臨時物件 = 右值
    s4.print();
    TrackedString::report("值+move（右值）");
    cout << "    ↑ 零複製！這是 const& 版本做不到的" << endl;

    cout << "\n=== 對照：const& 版本傳右值 ===" << endl;
    TrackedString::reset();
    StudentBetter s5(TrackedString("王五"), 23);
    s5.print();
    TrackedString::report("const&（右值）");
    cout << "    ↑ 仍然複製一次 —— 因為只能從 const& 複製出來" << endl;

    cout << "\n=== ⚠️ 為什麼不能用非 const 引用 ===" << endl;
    cout << "  若寫成 Student(TrackedString& n, int a)：" << endl;
    cout << "    Student s(TrackedString(\"臨時\"), 20);  // ❌ 編譯失敗" << endl;
    cout << "    臨時物件無法綁定到非 const 引用，且語意上暗示會修改參數" << endl;

    cout << "\n=== 實務：高頻日誌事件 ===" << endl;
    vector<LogEvent> events;
    events.reserve(4);
    // 這些引數都是剛組好的臨時字串（右值）→ 全程零複製
    events.emplace_back("req-1a2b", "auth", "使用者登入成功", 1);
    events.emplace_back("req-3c4d", "payment", "信用卡授權失敗，重試中", 2);
    events.emplace_back("req-5e6f", "database", "連線池即將耗盡 (9/10)", 2);
    events.emplace_back("req-7g8h", "api", "上游服務逾時 3000ms", 3);

    for (const LogEvent& e : events) {
        cout << "  " << e.format() << endl;
    }
    cout << "  ↑ 每秒數萬筆的熱路徑上，「每筆少複製三次字串」是實際可測量的差異" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 15 課：帶參數的建構函數1.cpp" -o demo1
// 註：std::move 與移動語意需 C++11 以上。
//     下方計數為 -O0（未最佳化）下的實測值；開 -O2 後編譯器的
//     copy elision 可能進一步減少次數，但語意上的差異不變。

// === 預期輸出 ===
// === 方式 A：值傳遞 + 本體內賦值 ===
//     張三, 20 歲
//     值傳遞 複製建構 1 次、移動建構 0 次、複製賦值 1 次、移動賦值 0 次
//     ↑ 複製兩次：一次進參數、一次賦值給成員
//
// === 方式 B：const 引用 + 初始化列表 ===
//     張三, 20 歲
//     const 引用 複製建構 1 次、移動建構 0 次、複製賦值 0 次、移動賦值 0 次
//     ↑ 只複製一次：參數不複製，直接以它建構成員
//
// === 方式 C：值傳遞 + std::move（傳左值）===
//     張三, 20 歲
//     值+move（左值） 複製建構 1 次、移動建構 1 次、複製賦值 0 次、移動賦值 0 次
//     ↑ 一次複製 + 一次移動，與 const& 版本成本相當
//
// === 方式 C：值傳遞 + std::move（傳右值）===
//     李四, 22 歲
//     值+move（右值） 複製建構 0 次、移動建構 1 次、複製賦值 0 次、移動賦值 0 次
//     ↑ 零複製！這是 const& 版本做不到的
//
// === 對照：const& 版本傳右值 ===
//     王五, 23 歲
//     const&（右值） 複製建構 1 次、移動建構 0 次、複製賦值 0 次、移動賦值 0 次
//     ↑ 仍然複製一次 —— 因為只能從 const& 複製出來
//
// === ⚠️ 為什麼不能用非 const 引用 ===
//   若寫成 Student(TrackedString& n, int a)：
//     Student s(TrackedString("臨時"), 20);  // ❌ 編譯失敗
//     臨時物件無法綁定到非 const 引用，且語意上暗示會修改參數
//
// === 實務：高頻日誌事件 ===
//   [INFO][auth] req=req-1a2b 使用者登入成功
//   [WARN][payment] req=req-3c4d 信用卡授權失敗，重試中
//   [WARN][database] req=req-5e6f 連線池即將耗盡 (9/10)
//   [ERROR][api] req=req-7g8h 上游服務逾時 3000ms
//   ↑ 每秒數萬筆的熱路徑上，「每筆少複製三次字串」是實際可測量的差異
