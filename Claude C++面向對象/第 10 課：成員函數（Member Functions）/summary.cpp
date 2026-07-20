/*
 * ================================================================
 * 【第 10 課：成員函數（Member Functions）】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 *
 * 本課重點：
 * 1. 類內定義（隱式 inline）vs 類外定義（使用 類別:: 作用域運算子）
 * 2. 成員函數的參數與返回值：各種組合
 * 3. 返回 *this 實現鏈式調用（method chaining）
 * 4. 函數重載（Overloading）：同名函數，不同參數
 * 5. 預設參數：從右往左設定，從右往左省略
 * 6. 成員函數互相調用：不需要任何前綴直接呼叫
 * 7. 隱藏的 this 指標：成員函數背後的機制
 * 8. 驗證 this 存在：印出 this 等於對象地址
 * 9. 成員函數 vs 普通函數的本質差異
 * ================================================================
 */

// =============================================================================
// 【主題資訊 Information】
// -----------------------------------------------------------------------------
//   主題：    成員函數（Member Functions）—— 類別的「行為」部分
//   語法：    class X {
//                 ret f(args);           // 宣告
//                 ret g(args) { ... }    // 類內定義 → 隱式 inline
//             };
//             ret X::f(args) { ... }     // 類外定義
//   標準：    本課全部語法在 C++98 即已存在。
//             其中 C++11 起可用預設成員初始化（double width = 0;），
//             本檔已採用該寫法，因此最低需 -std=c++11；示範以 C++17 編譯。
//   標頭檔：  <iostream>、<string>、<cmath>
//   關鍵型別：this 在非 const 成員函式中為 X* const；const 成員函式中為 const X* const
//
// =============================================================================
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. 成員函數的本質：多吃一個 this 參數的普通函數】
//   這是理解本課所有內容的唯一關鍵。編譯器把
//       class Dog { void bark() { cout << name; } };
//       d1.bark();
//   概念上翻譯成
//       void Dog_bark(Dog* this) { cout << this->name; }
//       Dog_bark(&d1);
//   由此可以立刻推出本課的所有結論：
//     - 為什麼所有物件共用一份程式碼，卻能各自印出自己的資料？→ this 不同。
//     - 為什麼成員函數不佔物件大小？→ 程式碼在 text 段，只有一份。
//     - 為什麼 static 成員函數不能存取非 static 成員？→ 它沒有 this。
//     - 為什麼 const 成員函數不能修改成員？→ this 變成 const X*。
//
// 【2. 類內定義 vs 類外定義：差別只在 inline 與名稱限定】
//   兩者語意完全相同。類內定義自動帶 inline 語意，
//   而 inline 的標準意義是**放寬 ODR**（允許多個 translation unit 有同一份
//   定義，連結器擇一），**不是**「保證展開」——
//   是否展開完全由最佳化器決定，與關鍵字幾乎無關。
//   類外定義必須寫 X:: 限定，否則會變成一個看不到成員的自由函數，
//   而錯誤訊息會是「width 未宣告」，離真正原因很遠。
//
// 【3. 回傳 *this 與鏈式調用】
//   回傳型別必須是**引用** X&。回傳值 X 會每步複製一份新物件，
//   後續操作改的是副本，原物件不變 —— 而且**這仍然編譯得過**，
//   只在執行結果才看得出錯，是最難察覺的一類 bug。
//
// 【4. 重載 vs 預設引數：看起來相似，本質完全不同】
//     重載      → 真的存在多個函式、多個符號，編譯期依參數型別決議。
//     預設引數  → 只有一個函式、一個符號，由**呼叫端**補上省略的引數。
//   兩個推論：
//     (a) 改預設引數的值需要**重新編譯所有呼叫端**才會生效
//         （這是共享函式庫的 ABI 陷阱）；
//     (b) 兩者混用極易 ambiguous：同時有 f(int) 與 f(int, int = 0) 時，
//         f(1) 兩者皆可行 → 編譯錯誤。
//
// 【5. 成員函數互相呼叫 = 邏輯分層】
//   本課的 TemperatureConverter 是很好的樣本：
//     底層純計算（toFahrenheit）→ 單一述詞（isBoiling）
//     → 組合決策（getState）→ 呈現（report）
//   每層只依賴下一層，因此「改沸點定義」只需動 isBoiling 一處。
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
// (A) this 在機器層級長什麼樣
//   this 的傳遞方式屬於 ABI 規範。在 x86-64 System V ABI（Linux）上，
//   this 是隱含的第一個參數，放在 rdi 暫存器，其餘參數依序往後推。
//   這是「成員函數就是多一個參數的普通函數」最直接的實證。
//   （MSVC 的 __thiscall 放 ecx，細節依平台而異，屬實作定義。）
//
// (B) 重載是用 name mangling 實作的
//   編譯器把參數型別編進符號名，所以 print(int) 與 print(double)
//   在連結器眼中是兩個完全不同的符號。這解釋了兩件事：
//   為什麼 C 沒有重載（符號名就是函式名），
//   以及為什麼 extern "C" 的函式不能重載（它要求符號名不被 mangle）。
//
// (C) 重載決議只比「轉換等級」，不看語意
//   等級由低到高：完全匹配 > 型別提升 > 標準轉換 > 使用者自訂轉換 > ...
//   最惡名昭彰的後果：若重載集合中有 f(bool)，
//   則 f("hello") 會選中 **bool** 版本 ——
//   因為 const char* → bool 是標準轉換，
//   而 const char* → std::string 是使用者自訂轉換，等級較低。
//
// (D) 物件大小與成員函數無關
//   本檔 Rectangle 只有兩個 double，本機 x86-64 實測 sizeof 為 16
//   （實作定義值，依 double 大小與對齊而異）。
//   四個成員函數不佔任何空間。要等到有 virtual 函式時，
//   物件才會被塞進一個 vptr（第 20 課以後）。
//
// (E) const 正確性有傳染性
//   本課多數唯讀函式（area、toFahrenheit、isBoiling…）其實都該是 const。
//   不加的後果是 const 物件與 const 引用參數完全無法使用它們。
//   而且必須**由底層往上**一次補齊：只要 report() 沒加 const，
//   它就無法被任何 const 成員函式呼叫。事後補會牽一髮動全身。
//
// =============================================================================
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. 鏈式調用的回傳型別必須是 X& 而非 X，否則語意錯誤但編譯照過。
//  2. 只差回傳型別不構成重載，是重複定義（編譯錯誤）；
//     只差 top-level const（f(int) vs f(const int)）同理。
//  3. **virtual 函式不要給預設引數**：預設值依靜態型別決定，
//     函式本體依動態型別決定，兩者會對不起來。
//  4. 預設引數只能從右往左連續給，因為 C++ 沒有具名引數。
//     需要跳著指定時，改用重載或 options struct。
//  5. 浮點門檻一律用 >= / <=，不要用 ==。
//     危險之處在於 == 有時湊巧成立、有時不成立，無法預測。
//  6. 物件位址（this、&obj）每次執行都不同（stack 位置 + ASLR），
//     不可寫入測試斷言。
//  7. lambda 捕獲 [this] 只抓裸指標，**不延長物件生命週期**；
//     非同步回呼比物件活得久就是 use-after-free。C++17 可改用 [*this]。
//  8. 判斷該寫成員還是自由函數：**它需要存取 private 嗎？**
//     不需要就寫自由函數 —— 能碰 private 的程式碼越少，封裝性越好。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函數（Member Functions）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 所有物件共用同一份成員函數程式碼，它怎麼知道該操作哪個物件？
//     答：靠編譯器隱含加上的 this 參數。d1.bark() 實際上把 &d1 當第一個
//         引數傳進去，函式內未限定的成員名都被補成 this->名稱。
//         成員函數本質上就是「多吃一個 this 參數的普通函數」。
//     追問：那 static 成員函式呢？
//         → 沒有 this，不綁定任何物件，因此無法存取非 static 成員；
//           它比較像是「放在類別命名空間下的自由函數」。
//
// 🔥 Q2. inline 到底做什麼？把函式寫在類內會比較快嗎？
//     答：inline 的標準語意是**放寬 ODR**，讓同一份定義可出現在多個
//         translation unit 而不會 multiple definition。
//         是否展開是最佳化器的獨立決策，與關鍵字幾乎無關
//         （-O0 寫了也不展開，-O2 沒寫也可能展開）。
//         所以「寫類內比較快」是錯誤認知。
//     追問：那為什麼類內定義必須是隱式 inline？
//         → 因為類定義通常放在 header，會被多個 .cpp include；
//           若不是 inline 就會連結失敗。
//
// 🔥 Q3. 函數重載與預設引數有什麼本質差別？
//     答：重載是多個獨立的函式、多個符號，編譯期依型別決議；
//         預設引數只有一個函式、一個符號，由呼叫端補上省略的引數。
//         因此改預設值需重編所有呼叫端才生效，改重載則不必。
//     追問：兩者可以並用嗎？
//         → 技術上可以但很危險：f(int) 與 f(int, int = 0) 並存時，
//           f(1) 無法決議，直接編譯錯誤。
//
// ⚠️ 陷阱 1. 重載集合裡有 f(bool) 與 f(const std::string&)，f("hello") 選哪個？
//     答：選 **f(bool)**。因為 const char* → bool 是標準轉換，
//         const char* → std::string 是使用者自訂轉換，前者等級較高。
//     為什麼會錯：大家用語意推理「字串當然配字串」，
//         但重載決議只比轉換等級，且字面值的型別是 const char* 而非 string。
//         解法是補一個 f(const char*) 重載精準接住。
//
// ⚠️ 陷阱 2. 基底 virtual f(int x = 10)，衍生覆寫成 f(int x = 20)，
//            透過 Base* 呼叫 p->f() 用哪個預設值？
//     答：用 **Base 的 10**，但執行 **Derived 的函式本體**。
//         預設引數在編譯期依靜態型別填入，虛擬呼叫在執行期依動態型別分派 ——
//         分屬不同階段，所以會出現「本體是新的、預設值是舊的」。
//     為什麼會錯：以為既然叫到 Derived 的版本，整組都該用 Derived 的；
//         但預設引數只是呼叫端的語法糖，根本沒進 vtable。
//         準則：virtual 函式不要給預設引數。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <sstream>
using namespace std;


// ===== 重點一：類內定義 vs 類外定義 =====
// 說明：
// 類內定義（inline）：函數體直接寫在 class {} 裡，編譯器視為「隱式 inline」。
//   適合短函數（1~3 行），編譯器可能在呼叫處展開，減少函數調用開銷。
// 類外定義：在 class 外用「類別名::函數名」定義，適合長函數，保持 class 定義整潔。
// 實務建議：短函數寫類內，長函數寫類外，不需要刻意追求 inline。

class Rectangle {
public:
    double width = 0;
    double height = 0;

    // 類內定義（隱式 inline）—— 短函數放這裡
    double area() {
        return width * height;
    }

    // 類內宣告，類外定義 —— 長函數的聲明放這裡
    double perimeter();
    void scale(double factor);
    void print();
};

// 類外定義：必須寫 Rectangle:: 前綴
double Rectangle::perimeter() {
    return 2 * (width + height);
}

void Rectangle::scale(double factor) {
    width *= factor;
    height *= factor;
}

void Rectangle::print() {
    cout << "Rectangle(" << width << " x " << height << ")" << endl;
    cout << "  面積: " << area() << endl;       // 成員函數可以直接呼叫另一個成員函數
    cout << "  周長: " << perimeter() << endl;
}


// ===== 重點二：成員函數的參數與返回值 =====
// 說明：成員函數和普通函數一樣，可以有各種參數和返回值。
// 特別介紹：返回 *this 引用，可實現「鏈式調用（method chaining）」。
// *this 是指向當前對象的指標解引用，即「當前對象本身」。
// 鏈式調用：sh.appendChain("A").appendChain("B") 因為每次返回的都是 sh 本身。

class StringHelper {
public:
    string text = "";

    void clear() { text = ""; }                             // 無參數、無返回值

    void append(const string& suffix) { text += suffix; }  // 有參數、無返回值

    int length() { return (int)text.length(); }             // 無參數、有返回值

    char charAt(int index) {                                 // 有參數、有返回值
        if (index < 0 || index >= (int)text.length()) {
            cout << "錯誤：索引超出範圍" << endl;
            return '\0';
        }
        return text[index];
    }

    string substring(int start, int len) {
        if (start < 0 || start >= (int)text.length()) return "";
        return text.substr(start, len);
    }

    bool contains(const string& target) {                   // 返回 bool
        return text.find(target) != string::npos;
    }

    // 返回自身引用（鏈式調用的基礎）
    // *this 解引用 this 指標得到當前對象本身，返回引用使呼叫方可以繼續鏈接
    StringHelper& appendChain(const string& suffix) {
        text += suffix;
        return *this;   // 返回自己
    }
};


// ===== 重點三：函數重載（Function Overloading）=====
// 說明：同一個作用域內可以定義多個同名但參數列表不同的函數。
// 編譯器根據呼叫時傳入的參數型別和數量決定呼叫哪個版本。
// 這稱為「靜態多型（compile-time polymorphism）」。
// 注意：只有返回值不同不算重載！必須是參數型別或數量不同。

class Printer {
public:
    void print(int value) {
        cout << "整數: " << value << endl;
    }

    void print(double value) {
        cout << "浮點數: " << value << endl;
    }

    void print(const string& value) {
        cout << "字串: " << value << endl;
    }

    void print(int value, int width) {       // 相同名，不同參數數量
        cout << "整數(寬度 " << width << "): ";
        string s = to_string(value);
        for (int i = 0; i < width - (int)s.length(); i++) cout << " ";
        cout << s << endl;
    }

    // int print(int value);   // 錯誤！只有返回值不同，不算重載，編譯器無法區分
};


// ===== 重點四：帶預設參數的成員函數 =====
// 說明：函數參數可以指定預設值，呼叫時省略的參數使用預設值。
// 規則：預設值必須從「右邊」開始設定，不能跳過。
//       呼叫時，省略的參數也必須從「右邊」開始省略。
// 例：void foo(int a, int b = 10, int c = 20)
//   foo(1)       → a=1, b=10, c=20
//   foo(1, 2)    → a=1, b=2,  c=20
//   foo(1, 2, 3) → a=1, b=2,  c=3

class Logger {
public:
    string name = "APP";

    // level 有預設值 "INFO"，不提供時自動使用
    void log(const string& message, const string& level = "INFO") {
        cout << "[" << level << "] " << name << ": " << message << endl;
    }

    // ch 和 repeat 都有預設值
    void divider(char ch = '-', int repeat = 40) {
        for (int i = 0; i < repeat; i++) cout << ch;
        cout << endl;
    }
};


// ===== 重點五：成員函數調用成員函數 =====
// 說明：成員函數之間可以自由互相呼叫，直接使用函數名即可，不需要任何前綴。
// 呼叫機制：編譯器自動轉換為 this->函數名()。
// 好處：可以構建層次化的邏輯——高層函數調用低層函數，職責分離。

class TemperatureConverter {
public:
    double celsius = 0.0;

    double toFahrenheit() { return celsius * 9.0 / 5.0 + 32.0; }
    double toKelvin()     { return celsius + 273.15; }

    bool isBoiling()  { return celsius >= 100.0; }
    bool isFreezing() { return celsius <= 0.0; }

    string getState() {
        if (isFreezing()) return "固態（冰）";       // 呼叫自己的成員函數
        if (isBoiling())  return "氣態（水蒸氣）";
        return "液態（水）";
    }

    void report() {
        cout << "攝氏:" << celsius << "°C | 華氏:" << toFahrenheit()
             << "°F | 克式:" << toKelvin() << "K | " << getState() << endl;
    }
};


// ===== 重點六：隱藏的 this 指標 =====
// 說明：每個成員函數都有一個隱藏的 this 參數，指向呼叫函數的對象。
// 當你寫 d1.bark() 時，編譯器實際執行 Dog::bark(&d1)，把 &d1 傳給 this。
// 因此在 bark() 內部，name 等同於 this->name。
// 這就是為什麼 d1.bark() 和 d2.bark() 雖然執行同一份程式碼，卻能正確識別各自的成員。

class Dog {
public:
    string name;

    void bark() {
        // 你寫：name
        // 編譯器看到：this->name
        cout << name << " 汪汪！" << endl;
    }
};

// ===== 重點七：驗證 this 的存在 =====
// 說明：可以直接印出 this 來驗證它就是指向當前對象的指標。
// 對象的地址（&對象）與函數內部的 this 值完全一致。

class Widget {
public:
    int id = 0;

    void showAddress() {
        cout << "對象 id=" << id << " 的 this 地址: " << this << endl;
    }
};


// ===== 重點八：成員函數 vs 普通函數 =====
// 說明：兩者的本質差異——成員函數天生能透過 this 存取同對象的所有成員。
// 普通函數必須透過參數明確傳入所需的資料。
// 何時用哪種？當函數操作的是對象本身的資料，就應該寫成成員函數。

class Circle {
public:
    double radius = 0;

    // 成員函數：自動知道 radius，不需要傳入
    double area() {
        return 3.14159 * radius * radius;
    }
};

// 普通函數：必須手動傳入 radius，否則不知道要算哪個 circle 的面積
double circleArea(double radius) {
    return 3.14159 * radius * radius;
}


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System
//   題目：設計一個停車系統，建構時給大／中／小三種車位的數量；
//         addCar(carType) 嘗試停入一台車（1=大 2=中 3=小），
//         成功回 true 並佔用一格，車位不足回 false。
//   為什麼用到本主題：這是一道**純粹考成員函數設計**的題目 ——
//         建構函式初始化狀態、成員函數改變狀態並回答問題，
//         而狀態透過 this 隱含地在各成員函數間共享。
//         正是本課「物件 = 資料 + 操作該資料的成員函數」的最小完整示範。
//   複雜度：建構 O(1)，addCar 每次 O(1)。
// -----------------------------------------------------------------------------
class ParkingSystem {
    int m_slots[4] = {0, 0, 0, 0};   // 索引 1/2/3 對應大/中/小，索引 0 不用

public:
    ParkingSystem(int big, int medium, int small) {
        m_slots[1] = big;
        m_slots[2] = medium;
        m_slots[3] = small;
    }

    bool addCar(int carType) {
        if (carType < 1 || carType > 3) return false;   // 防呆：題目保證 1~3，仍擋一下
        if (m_slots[carType] == 0) return false;        // 該類車位已滿
        --m_slots[carType];                             // 佔用一格（修改 this 的狀態）
        return true;
    }

    // 額外的唯讀查詢：示範 const 成員函數
    int remaining(int carType) const {
        if (carType < 1 || carType > 3) return 0;
        return m_slots[carType];
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例】服務指標蒐集器（Metrics Collector）
//   情境：線上服務要統計每個 API endpoint 的呼叫次數與累計耗時，
//         再輸出成監控系統看得懂的一行行文字。
//         這是每個後端服務都會有的東西（Prometheus / StatsD 的簡化版）。
//   為什麼用到本主題：它把本課幾乎所有概念都用上了 ——
//     - 重載        ：record() 可接受毫秒(int) 或秒(double)
//     - 預設引數    ：record(name, ms, count = 1) 批次記錄
//     - 回傳 *this  ：鏈式串接多次記錄
//     - 成員互呼叫  ：report() 呼叫 averageMs()，averageMs() 呼叫 totalMs()
//     - 類外定義    ：report() 較長，宣告在類內、定義在類外
//     - const 正確性：所有唯讀查詢都標 const，讓 const 物件也能用
// -----------------------------------------------------------------------------
class MetricsCollector {
    struct Entry {
        string name;
        long long calls   = 0;
        double    totalMs = 0.0;
    };
    vector<Entry> m_entries;

    // private 輔助函式：找不到就新建一筆
    Entry& findOrCreate(const string& name) {
        for (Entry& e : m_entries) {
            if (e.name == name) return e;
        }
        m_entries.push_back(Entry{name, 0, 0.0});
        return m_entries.back();
    }

public:
    // 重載 1：以毫秒記錄；count 有預設值 → 可批次記錄
    MetricsCollector& record(const string& name, int ms, long long count = 1) {
        Entry& e = findOrCreate(name);
        e.calls   += count;
        e.totalMs += static_cast<double>(ms) * static_cast<double>(count);
        return *this;                       // 回傳自身引用 → 可鏈式
    }

    // 重載 2：以秒記錄（轉成毫秒後交給重載 1，避免邏輯重複）
    MetricsCollector& record(const string& name, double seconds, long long count = 1) {
        return record(name, static_cast<int>(seconds * 1000.0), count);
    }

    // 唯讀查詢，全部標 const
    long long callsOf(const string& name) const {
        for (const Entry& e : m_entries) {
            if (e.name == name) return e.calls;
        }
        return 0;
    }

    double totalMs(const string& name) const {
        for (const Entry& e : m_entries) {
            if (e.name == name) return e.totalMs;
        }
        return 0.0;
    }

    // 成員函數呼叫另一個成員函數
    double averageMs(const string& name) const {
        long long n = callsOf(name);
        return (n == 0) ? 0.0 : totalMs(name) / static_cast<double>(n);
    }

    void report() const;                    // 類內宣告、類外定義
};

// 類外定義：必須寫 MetricsCollector:: 限定，且 const 要一起帶上
void MetricsCollector::report() const {
    for (const Entry& e : m_entries) {
        ostringstream oss;
        oss.setf(ios::fixed);
        oss.precision(1);
        oss << "  " << e.name
            << "  calls=" << e.calls
            << "  total=" << e.totalMs << "ms"
            << "  avg="   << averageMs(e.name) << "ms";   // 呼叫其他成員函數
        cout << oss.str() << endl;
    }
}


// ===== 綜合實戰：計算器類別（展示本課所有概念）=====
class Calculator {
public:
    double result = 0.0;
    int operationCount = 0;

    // 重載：接受 double 或 int 都能設初值
    void setValue(double val) {
        result = val;
        operationCount = 0;
        cout << "初始值設為: " << val << endl;
    }
    void setValue(int val) { setValue((double)val); }   // 呼叫 double 版本

    // 基本運算（每個函數都呼叫私有的 record 輔助函數）
    void add(double v)      { result += v; record("+", v); }
    void subtract(double v) { result -= v; record("-", v); }
    void multiply(double v) { result *= v; record("*", v); }

    bool divide(double v) {
        if (v == 0.0) { cout << "錯誤：不能除以零！" << endl; return false; }
        result /= v; record("/", v);
        return true;
    }

    // 進階運算：調用自己的 multiply（成員函數互調）
    void square()  { multiply(result); }
    void negate()  { multiply(-1); }

    void showResult() { cout << "結果: " << result << endl; }

    void reset() {
        result = 0.0;
        operationCount = 0;
        cout << "計算器已重置" << endl;
    }

private:
    // 私有輔助函數：外界不需要知道的內部邏輯
    void record(const string& op, double value) {
        operationCount++;
        cout << "  [#" << operationCount << "] " << op << " " << value
             << " → " << result << endl;
    }
};


int main() {
    cout << "===== 重點一：類內 vs 類外定義 =====" << endl;
    Rectangle r;
    r.width = 5.0; r.height = 3.0;
    r.print();
    r.scale(2.0);
    cout << "放大2倍後："; r.print();


    cout << "\n===== 重點二：參數/返回值 & 鏈式調用 =====" << endl;
    StringHelper sh;
    sh.append("Hello"); sh.append(", "); sh.append("World!");
    cout << "文字: " << sh.text << endl;
    cout << "長度: " << sh.length() << endl;
    cout << "charAt(0): " << sh.charAt(0) << endl;
    cout << "substring(7,5): " << sh.substring(7, 5) << endl;
    cout << "含 'World': " << (sh.contains("World") ? "是" : "否") << endl;

    cout << "--- 鏈式調用 ---" << endl;
    StringHelper sh2;
    sh2.appendChain("C++").appendChain(" is").appendChain(" awesome!");
    cout << sh2.text << endl;


    cout << "\n===== 重點三：函數重載 =====" << endl;
    Printer p;
    p.print(42);          // 呼叫 print(int)
    p.print(3.14);        // 呼叫 print(double)
    p.print("Hello");     // 呼叫 print(const string&)
    p.print(42, 10);      // 呼叫 print(int, int)


    cout << "\n===== 重點四：預設參數 =====" << endl;
    Logger logger;
    logger.name = "MyApp";
    logger.divider();                      // 全部使用預設值
    logger.log("程式啟動");               // level 使用預設值 "INFO"
    logger.log("連線失敗", "ERROR");      // 指定 level
    logger.divider('=', 20);


    cout << "\n===== 重點五：成員函數互調 =====" << endl;
    TemperatureConverter t;
    t.celsius = -10; t.report();
    t.celsius = 25;  t.report();
    t.celsius = 100; t.report();


    cout << "\n===== 重點六 & 七：this 指標 =====" << endl;
    Dog d1, d2;
    d1.name = "旺財"; d2.name = "小黑";
    d1.bark();   // this == &d1，所以 name 是 "旺財"
    d2.bark();   // this == &d2，所以 name 是 "小黑"

    Widget a, b;
    a.id = 1; b.id = 2;
    a.showAddress();
    b.showAddress();
    cout << "驗證 &a = " << &a << "（應與上方 id=1 的地址相同）" << endl;


    cout << "\n===== 重點八：成員函數 vs 普通函數 =====" << endl;
    Circle c;
    c.radius = 5.0;
    cout << "成員函數: " << c.area() << endl;           // 自動知道 radius
    cout << "普通函數: " << circleArea(c.radius) << endl; // 必須手動傳入


    cout << "\n===== 綜合實戰：計算器 =====" << endl;
    Calculator calc;
    calc.setValue(10);   // 重載：傳入 int
    calc.add(5);
    calc.multiply(3);
    calc.subtract(8);
    calc.divide(7);
    calc.divide(0);      // 錯誤測試
    calc.showResult();


    cout << "\n===== LeetCode 1603. Design Parking System =====" << endl;
    ParkingSystem ps(1, 1, 0);      // 大 1 格、中 1 格、小 0 格
    cout << "addCar(1) 大車 -> " << boolalpha << ps.addCar(1) << "  (唯一大車位被佔用)" << endl;
    cout << "addCar(2) 中車 -> " << ps.addCar(2) << "  (唯一中車位被佔用)" << endl;
    cout << "addCar(3) 小車 -> " << ps.addCar(3) << " (小車位本來就是 0)" << endl;
    cout << "addCar(1) 大車 -> " << ps.addCar(1) << " (大車位已滿)" << endl;
    cout << "剩餘 大/中/小 = " << ps.remaining(1) << "/" << ps.remaining(2)
         << "/" << ps.remaining(3) << endl;


    cout << "\n===== 日常實務：服務指標蒐集器 =====" << endl;
    MetricsCollector metrics;
    // 鏈式 + 重載（int 毫秒）+ 預設引數（count 省略）
    metrics.record("GET /api/users", 120)
           .record("GET /api/users", 95)
           .record("GET /api/users", 140);
    // 用預設引數的第三參數批次記錄：50 次、每次 8ms
    metrics.record("GET /health", 8, 50);
    // 重載（double 秒）：1.5 秒 = 1500 毫秒
    metrics.record("POST /api/report", 1.5);

    metrics.report();
    cout << "  ---" << endl;
    cout << "  GET /api/users 平均耗時 = " << metrics.averageMs("GET /api/users") << "ms" << endl;
    cout << "  不存在的 endpoint 平均 = " << metrics.averageMs("GET /nope")
         << "ms  (安全回 0，不會除以零)" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// ★ 注意：「重點六 & 七」的 0x... 位址每次執行都不同（stack 位置 + ASLR）。

// === 預期輸出 ===
//    要驗證的是「id=1 的 this」與「&a」印出相同的值這個對應關係。
//
// ===== 重點一：類內 vs 類外定義 =====
// Rectangle(5 x 3)
//   面積: 15
//   周長: 16
// 放大2倍後：Rectangle(10 x 6)
//   面積: 60
//   周長: 32
//
// ===== 重點二：參數/返回值 & 鏈式調用 =====
// 文字: Hello, World!
// 長度: 13
// charAt(0): H
// substring(7,5): World
// 含 'World': 是
// --- 鏈式調用 ---
// C++ is awesome!
//
// ===== 重點三：函數重載 =====
// 整數: 42
// 浮點數: 3.14
// 字串: Hello
// 整數(寬度 10):         42
//
// ===== 重點四：預設參數 =====
// ----------------------------------------
// [INFO] MyApp: 程式啟動
// [ERROR] MyApp: 連線失敗
// ====================
//
// ===== 重點五：成員函數互調 =====
// 攝氏:-10°C | 華氏:14°F | 克式:263.15K | 固態（冰）
// 攝氏:25°C | 華氏:77°F | 克式:298.15K | 液態（水）
// 攝氏:100°C | 華氏:212°F | 克式:373.15K | 氣態（水蒸氣）
//
// ===== 重點六 & 七：this 指標 =====
// 旺財 汪汪！
// 小黑 汪汪！
// 對象 id=1 的 this 地址: 0x7ffdc26c9b30
// 對象 id=2 的 this 地址: 0x7ffdc26c9b34
// 驗證 &a = 0x7ffdc26c9b30（應與上方 id=1 的地址相同）
//
// ===== 重點八：成員函數 vs 普通函數 =====
// 成員函數: 78.5397
// 普通函數: 78.5397
//
// ===== 綜合實戰：計算器 =====
// 初始值設為: 10
//   [#1] + 5 → 15
//   [#2] * 3 → 45
//   [#3] - 8 → 37
//   [#4] / 7 → 5.28571
// 錯誤：不能除以零！
// 結果: 5.28571
//
// ===== LeetCode 1603. Design Parking System =====
// addCar(1) 大車 -> true  (唯一大車位被佔用)
// addCar(2) 中車 -> true  (唯一中車位被佔用)
// addCar(3) 小車 -> false (小車位本來就是 0)
// addCar(1) 大車 -> false (大車位已滿)
// 剩餘 大/中/小 = 0/0/0
//
// ===== 日常實務：服務指標蒐集器 =====
//   GET /api/users  calls=3  total=355.0ms  avg=118.3ms
//   GET /health  calls=50  total=400.0ms  avg=8.0ms
//   POST /api/report  calls=1  total=1500.0ms  avg=1500.0ms
//   ---
//   GET /api/users 平均耗時 = 118.333ms
//   不存在的 endpoint 平均 = 0ms  (安全回 0，不會除以零)
