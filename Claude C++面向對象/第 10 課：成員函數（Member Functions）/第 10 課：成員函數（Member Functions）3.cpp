// =============================================================================
//  第 10 課：成員函數 3  —  函數重載（Function Overloading）與多載決議
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  同一個 scope 內，函式名相同、**參數列不同**（型別／個數／順序）。
//   標準：  C++98 起即有；C++11 起加入 rvalue reference 重載等新維度。
//   標頭檔：<iostream>、<string>
//   關鍵規則：回傳型別**不參與**重載決議；只有參數列算數。
//
// 【詳細解釋 Explanation】
//
// 【1. 重載解決的是「命名爆炸」問題】
//   沒有重載的語言（如 C）只能寫 print_int / print_double / print_string，
//   呼叫端得自己記住型別對應哪個名字。重載讓「同一個概念用同一個名字」，
//   由編譯器依實際引數型別自動挑選 —— 這是**編譯期**決定的（static dispatch），
//   與 virtual function 的執行期多型（dynamic dispatch）是完全不同的機制。
//   ★ 常見混淆：重載叫 "overloading"，覆寫叫 "overriding"，兩者毫無關係。
//
// 【2. 重載決議（overload resolution）的三步驟】
//   編譯器看到 p.print(x) 時做三件事：
//     (a) 名稱查找：收集所有叫 print 的候選函式；
//     (b) 篩選可行候選：引數能否（隱式）轉成參數型別；
//     (c) 挑最佳：比較各引數所需轉換的「代價等級」。
//   轉換代價由低到高大致是：
//       完全匹配 > 型別提升(promotion，如 char→int)
//       > 標準轉換(conversion，如 int→double、任意指標→bool)
//       > 使用者自訂轉換(user-defined，如 const char*→std::string)
//       > 省略號 ...
//   **同一等級內若有兩個候選一樣好，就是 ambiguous，編譯錯誤。**
//
// 【3. 回傳型別為什麼不能用來重載】
//   因為呼叫端可以完全忽略回傳值：寫 p.print(42); 時，
//   編譯器無從得知你要哪一個版本。所以標準直接規定回傳型別不參與決議 ——
//   只差在回傳型別的兩個函式是**重複定義**，不是重載。
//
// 【4. 本檔的四個 print 分別怎麼被選中】
//     p.print(42)      → int 完全匹配 print(int)
//     p.print(3.14)    → double 完全匹配 print(double)
//     p.print("Hello") → const char[6] 衰變成 const char*，
//                        唯一可行的是 print(const string&)（使用者自訂轉換）
//     p.print(42, 10)  → 兩個引數，只有 print(int,int) 參數個數對得上
//   注意最後一個：**參數個數不同也算重載**，這是最容易被選中的維度。
//
// 【概念補充 Concept Deep Dive】
//   重載是在**編譯期**用 name mangling 實現的。編譯器把參數型別編進符號名，
//   所以連結器看到的是四個完全不同的符號。可用 nm -C 觀察，例如
//   print(int) 與 print(double) 會 mangle 成不同名稱 —— 這也解釋了
//   為什麼 C 沒有重載（C 的符號名就是函式名本身），以及為什麼
//   extern "C" 的函式**不能重載**（它要求符號名不被 mangle）。
//
//   另一個重要細節：重載只在**同一個 scope** 內比較。若衍生類別宣告了
//   同名函式，會把基底類別的所有同名重載整組**遮蔽（name hiding）**，
//   而不是加入候選集 —— 需要 using Base::print; 才能把它們拉回同一 scope。
//   這是繼承章節（第 20 課以後）最常見的坑之一。
//
// 【注意事項 Pay Attention】
//   1. 只差在回傳型別 → 不是重載，是重複定義，編譯錯誤。
//   2. 只差在 top-level const（void f(int) 與 void f(const int)）→ 也不是重載，
//      因為傳值時 const 對呼叫端沒有意義。但 f(int&) 與 f(const int&) **是**重載。
//   3. 重載 + 預設引數同時用，極易造成 ambiguous：
//      若同時有 print(int) 與 print(int, int width = 0)，
//      則 print(42) 兩者皆可行 → 編譯錯誤。本檔刻意避開了這個組合。
//   4. **最惡名昭彰的陷阱**：若重載集合中有 bool 版本，
//      傳字串字面值會選中 bool（見下方【面試題】陷阱與可執行示範）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】函數重載與多載決議
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼構成合法的重載？回傳型別算嗎？
//     答：參數列不同才算 —— 型別、個數、順序。回傳型別**不參與**重載決議，
//         因為呼叫端可以丟棄回傳值，編譯器無從判斷要哪個版本；
//         只差回傳型別會被當成重複定義而編譯失敗。
//     追問：那 const 呢？
//         → 參數的 top-level const 不算（f(int) vs f(const int) 是同一個）；
//           但引用／指標所指之物的 const 算（f(int&) vs f(const int&) 是重載），
//           而成員函式的**尾綴 const**（f() vs f() const）也算，
//           這正是 const 與非 const 版 operator[] 能並存的原因。
//
// 🔥 Q2. 重載是編譯期還是執行期決定？跟 virtual 有什麼不同？
//     答：重載是**編譯期**依靜態型別決定（static dispatch），
//         函式在編譯完就綁定好了。virtual 是**執行期**依物件實際型別
//         透過 vtable 決定（dynamic dispatch）。兩者正交，可以並用。
//     追問：衍生類別定義同名函式會怎樣？
//         → 會遮蔽（name hiding）基底類別的整組同名重載，而非加入候選；
//           要用 using Base::f; 才拉得回來。
//
// ⚠️ 陷阱. 重載集合裡同時有 f(bool) 與 f(const std::string&)，
//         呼叫 f("Hello") 會選中哪一個？
//     答：選中 **f(bool)**（本機 g++ 15.2 實測，見下方 TrapPrinter 的實際輸出）。
//         因為 const char* → bool 是**標準轉換**（指標轉 bool），
//         而 const char* → std::string 是**使用者自訂轉換**，等級較低。
//         標準轉換永遠優先於使用者自訂轉換。
//     為什麼會錯：大家腦中想的是「字串當然配字串」，用語意在推理；
//         但重載決議完全不看語意，只比轉換等級，而字面值的型別是
//         const char* 而不是 std::string。
//         解法：加一個 f(const char*) 重載把它精準接住。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <iomanip>
using namespace std;

class Printer {
public:
    // 同名函數，不同參數 —— 這就是重載
    // 函數重載（Function Overloading）是 C++ 中的一種特性，允許在同一個作用域內定義多個同名但參數列表不同的函數。
    // 編譯器會根據函數調用時提供的參數類型和數量來決定調用哪一個函數。
    // 這使得我們可以使用相同的函數名稱來處理不同類型或數量的輸入，從而提高代碼的可讀性和靈活性。
    void print(int value) {
        cout << "整數: " << value << endl;
    }

    void print(double value) {
        cout << "浮點數: " << value << endl;
    }

    void print(const string& value) {
        cout << "字串: " << value << endl;
    }

    void print(int value, int width) {
        cout << "整數(寬度 " << width << "): ";
        // 簡單的右對齊
        string s = to_string(value);
        for (int i = 0; i < width - (int)s.length(); i++) {
            cout << " ";
        }
        cout << s << endl;
    }
};

// -----------------------------------------------------------------------------
// 【可執行示範】重載決議的經典陷阱：const char* 為什麼選中 bool
//   這不是假設，是真的會發生。下面兩個類只差一個 const char* 重載，
//   同樣寫 f("Hello")，結果完全不同。
// -----------------------------------------------------------------------------
class TrapPrinter {          // ❌ 沒有 const char* 重載
public:
    void f(bool v)                 { cout << "  選中 f(bool) -> " << boolalpha << v << endl; }
    void f(const string& v)        { cout << "  選中 f(const string&) -> " << v << endl; }
};

class FixedPrinter {         // ✅ 補上 const char* 重載，精準接住字面值
public:
    void f(bool v)                 { cout << "  選中 f(bool) -> " << boolalpha << v << endl; }
    void f(const string& v)        { cout << "  選中 f(const string&) -> " << v << endl; }
    void f(const char* v)          { cout << "  選中 f(const char*) -> " << v << endl; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔輸出器：用重載讓不同型別的值各自正確序列化
//   情境：把程式的執行時設定 dump 成 key=value 格式寫進 log 或 .ini。
//         不同型別要有不同的呈現規則 ——
//           bool   要印 true/false，不能印成 1/0；
//           double 要固定小數位，不能印成科學記號；
//           string 若含空白要加引號，否則解析回來會斷掉。
//   為什麼用到本主題：呼叫端一律寫 w.write("key", value) 不必關心型別，
//         由重載在編譯期挑出正確的格式化規則 —— 這正是重載最有價值的地方：
//         **對外統一介面，對內分型別處理**。
//   ★ 注意 write(const string&, const char*) 這個重載不是多餘的：
//     沒有它，w.write("host", "localhost") 會落到 bool 版本印出 true。
// -----------------------------------------------------------------------------
class ConfigWriter {
public:
    void write(const string& key, bool value) {
        cout << key << " = " << (value ? "true" : "false") << endl;
    }
    void write(const string& key, int value) {
        cout << key << " = " << value << endl;
    }
    void write(const string& key, double value) {
        // 固定 2 位小數，避免 0.0001 被印成 1e-04 而讓設定檔解析器讀不懂
        cout << key << " = " << fixed << setprecision(2) << value << defaultfloat << endl;
    }
    void write(const string& key, const char* value) {
        write(key, string(value));      // 轉交給 string 版本，避免落到 bool
    }
    void write(const string& key, const string& value) {
        // 含空白就加引號，否則 key=value 解析時會被空白截斷
        bool needQuote = value.find(' ') != string::npos || value.empty();
        cout << key << " = " << (needQuote ? "\"" + value + "\"" : value) << endl;
    }
};

int main() {
    cout << "=== 基本：四個 print 重載 ===" << endl;
    Printer p;

    p.print(42);              // 呼叫 print(int)
    p.print(3.14);            // 呼叫 print(double)
    p.print("Hello");         // 呼叫 print(const string&)
    p.print(42, 10);          // 呼叫 print(int, int)

    cout << "\n=== 陷阱示範：f(\"Hello\") 選中誰？ ===" << endl;
    cout << "沒有 const char* 重載時:" << endl;
    TrapPrinter tp;
    tp.f("Hello");            // ⚠️ 選中 f(bool)：指標轉 bool 是標準轉換，優先於使用者自訂轉換
    cout << "補上 const char* 重載後:" << endl;
    FixedPrinter fp;
    fp.f("Hello");            // ✅ 完全匹配，選中 f(const char*)

    cout << "\n=== 日常實務：設定檔輸出器 ===" << endl;
    ConfigWriter w;
    w.write("debug_mode", true);
    w.write("max_retry", 3);
    w.write("timeout_sec", 2.5);
    w.write("host", "localhost");          // 若無 const char* 重載，這行會印成 true
    w.write("app_name", "My Great App");   // 含空白 → 自動加引號
    w.write("note", string(""));           // 空字串 → 也加引號，才不會被誤讀成無值

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 10 課：成員函數（Member Functions）3.cpp" -o member3

// === 預期輸出 ===
// === 基本：四個 print 重載 ===
// 整數: 42
// 浮點數: 3.14
// 字串: Hello
// 整數(寬度 10):         42
//
// === 陷阱示範：f("Hello") 選中誰？ ===
// 沒有 const char* 重載時:
//   選中 f(bool) -> true
// 補上 const char* 重載後:
//   選中 f(const char*) -> Hello
//
// === 日常實務：設定檔輸出器 ===
// debug_mode = true
// max_retry = 3
// timeout_sec = 2.50
// host = localhost
// app_name = "My Great App"
// note = ""
