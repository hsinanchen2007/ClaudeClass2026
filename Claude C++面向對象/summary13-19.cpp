/*
 * ================================================================
 * 【第 13～19 課總複習】summary13-19.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary13-19 summary13-19.cpp
 *
 * 涵蓋課程：
 *   第 13 課：建構函數（Constructor）基礎
 *   第 14 課：預設建構函數（Default Constructor）
 *   第 15 課：帶參數的建構函數（Parameterized Constructor）
 *   第 16 課：建構函數初始化列表（Member Initializer List）
 *   第 17 課：解構函數（Destructor）
 *   第 18 課：對象的生命週期（Object Lifetime）
 *   第 19 課：動態對象的創建與銷毀（new / delete）
 *
 * 每課重點速查：
 *   13: 建構函數語法、四種調用時機、重載、資料驗證、Most Vexing Parse
 *   14: 預設建構函數、= default vs 手動空建構、= delete、陣列需求、歧義陷阱
 *   15: const 引用傳遞、同名解決(this/m_/suffix_/init list)、五種調用語法、
 *       窄化防護、explicit、預設參數
 *   16: 初始化列表 vs 函數體賦值(一步 vs 兩步)、四種必須使用情況
 *       (const/引用/無預設建構成員/基類)、順序陷阱、效能對比
 *   17: ~ClassName 語法、LIFO 解構順序、RAII(動態記憶體/檔案/計時器)、
 *       記憶體洩漏、編譯器自動生成的解構函數、不要拋異常
 *   18: 四種存儲期(自動/靜態全域/靜態局部/動態)、for 每次迭代建構解構、
 *       延遲初始化(static local)、Static Initialization Order Fiasco、
 *       臨時對象生命週期(const& 延長)、懸空引用/野指標陷阱
 *   19: new/delete 語法、new vs malloc、new[]/delete[] 配對、
 *       bad_alloc/nothrow、四種洩漏場景、RAII、unique_ptr/make_unique
 * ================================================================
 */

#include <iostream>
#include <stdexcept>   // std::runtime_error（RAII 例外示範）
#include <string>
#include <cmath>
#include <memory>    // unique_ptr, make_unique
#include <chrono>
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
//
// 【主題資訊 Information】
//   範圍：  物件如何誕生（13、14、15、16）→ 如何消滅（17）
//           → 活多久（18）→ 手動控制生死（19）
//   標準：  本檔以 C++17 為基準。涉及版本的關鍵點：
//             * = default / = delete、統一初始化 {}、nullptr、
//               區域 static 執行緒安全初始化 —— 皆為 C++11
//             * std::make_unique —— C++14（C++11 只有 make_shared）
//             * 保證的 copy elision（回傳純右值不再需要可存取的拷貝建構）—— C++17
//   標頭檔：<iostream> <string> <vector> <memory>
//   關鍵詞：constructor、member initializer list、RAII、storage duration、
//           Most Vexing Parse、SIOF、copy elision、smart pointer
//
// 【詳細解釋 Explanation】
//
// 【1. 這七課的主線：物件的一生】
//     13-16 課  誕生：怎麼把物件從「一塊記憶體」變成「滿足不變量的物件」
//     17 課     消滅：怎麼保證它離開時把借來的資源還回去
//     18 課     期間：它活在哪個儲存區、活多久、何時被建構與解構
//     19 課     手動：new/delete 讓你自己決定生死，代價是自己負責
// 把 13～19 看成一條時間軸，要記的東西會少很多。
// 而貫穿全部的核心觀念只有一個：**RAII**
//   —— 資源的取得寫在建構函式、釋放寫在解構函式，
//      然後讓「物件的生命週期」去驅動「資源的生命週期」。
//   這是 C++ 相對於 GC 語言最根本的差異：不是靠執行期回收器，
//   而是靠編譯期就決定好的作用域規則，把釋放時機釘死。
//
// 【2. 初始化列表 vs 函數體賦值：不是風格差異，是語意差異】
// 這是第 16 課最重要、也最常被當成「風格偏好」而輕忽的一點。
//     Vehicle(int p) : engine_(p) { }           // 初始化：直接用 p 建構 engine_
//     Vehicle(int p) { engine_ = Engine(p); }   // 賦值：先預設建構，再賦值覆蓋
// 第二種做了兩件事（預設建構 + 拷貝／移動賦值），第一種只做一件。
// 對 int 這種平凡型別看不出差別，對含資源的類別就是實實在在的多餘工作。
// 更關鍵的是有四種情況**只能**用初始化列表，函數體賦值根本編譯不過：
//     (a) const 成員        —— 賦值就是修改 const，不合法
//     (b) 引用成員          —— 引用必須在誕生時綁定，且不可重新綁定
//     (c) 沒有預設建構函數的成員 —— 進不了函數體就已經失敗
//     (d) 基底類別的建構     —— 基底必須在衍生類別的函數體之前完成
// 所以正確的心智模型是：**初始化列表才是預設做法**，函數體賦值是例外。
//
// 【3. 成員初始化順序：宣告順序決定，不是列表順序】
// 這是本專題最經典的陷阱（第 16 課「順序陷阱」）：
//     class X { int b_; int a_;
//               X() : a_(1), b_(a_) {} };   // b_ 先初始化，此時 a_ 還沒值
// 標準規定初始化順序**只依成員的宣告順序**，初始化列表只是提供初始值。
// 上例中 b_ 先宣告所以先初始化，而它用了尚未初始化的 a_ → UB。
// 編譯器的 -Wreorder 就是專門警告「列表順序與宣告順序不一致」，
// 看到它請把兩者對齊，不要忽略。
// （姊妹檔 summary20-26.cpp 的 Buffer26 保留了這個警告作為示範。）
//
// 【4. 解構順序是 LIFO，且與建構順序嚴格相反】
// 第 17 課的重點。在同一個作用域裡：
//     A a; B b; C c;        // 建構順序 a → b → c
//                           // 解構順序 c → b → a
// 這不是實作巧合，是標準保證。它之所以重要，是因為物件之間常有依賴：
// 若 c 依賴 b（例如 c 持有 b 的引用），那麼「c 先死」正是唯一安全的順序。
// 同樣的規則也適用於：
//     * 類別的成員：建構依宣告順序，解構依相反順序
//     * 繼承體系：基底先建構、最後解構；衍生後建構、最先解構
//     * 陣列元素：索引 0..n-1 建構，n-1..0 解構
//
// 【5. 四種儲存期（第 18 課），決定了「誰在什麼時候呼叫解構函式」】
//   * 自動（automatic）：區域變數。離開作用域即解構。最常用，也最安全。
//   * 靜態（static）：全域變數、static 成員、函式內的 static 區域變數。
//     程式結束時才解構，順序是「建構順序的相反」。
//   * 動態（dynamic）：new 出來的。**只有你呼叫 delete 才會解構** ——
//     忘記就是記憶體洩漏，這正是第 19 課與智慧指標存在的理由。
//   * 執行緒（thread_local，C++11）：每個執行緒各一份。
//   ⚠️ SIOF（Static Initialization Order Fiasco）：
//     不同翻譯單元（.cpp）的全域物件，其建構順序是**未指定**的。
//     所以一個 .cpp 的全域物件不可以在建構時依賴另一個 .cpp 的全域物件。
//     標準解法就是本檔 L18_Singleton 用的「函式內的 static 區域變數」
//     —— 它保證在第一次呼叫該函式時才初始化，順序因此變得確定。
//
// 【概念補充 Concept Deep Dive】
// (A) Most Vexing Parse：最惱人的解析
//     `Widget w();` 不是「建立一個 Widget」，而是「宣告一個回傳 Widget、
//     不收參數的函式」。C++ 的文法規定：只要一段程式碼**可以**被解析成宣告，
//     它就會被解析成宣告。
//     解法：改用大括號 `Widget w{};`（C++11 起）——
//     大括號不可能是函式宣告的參數列，歧義自然消失。
//     這也是「優先使用 {} 初始化」的理由之一。
//
// (B) = default 與「手寫空建構函數」不等價
//     class A { int v; public: A() = default; };   // 平凡（trivial）
//     class B { int v; public: B() {} };           // 使用者定義，非平凡
//     A a{};  →  值初始化會做零初始化 → v 保證為 0
//     B b{};  →  只呼叫那個空建構函數 → v 未被初始化，讀取即 UB
//     差別在於 = default 保留了型別的「平凡性」，
//     使 {} 得以走零初始化那條路徑。本檔 demo_lesson14 示範此點
//     （但刻意不去讀取未初始化的值，因為那是 UB）。
//
// (C) explicit 阻止的是「隱式轉換」，不是「轉換本身」
//     explicit Dist(double);  之後
//         Dist d1 = 100.0;     // ❌ 隱式轉換被禁止
//         Dist d2(100.0);      // ✅ 直接初始化仍然可以
//         Dist d3{100.0};      // ✅ 同上
//     準則：單一參數（或除第一個外都有預設值）的建構函數，預設就該加 explicit，
//     除非你「刻意」想要那個隱式轉換（例如 std::string 從 const char* 轉換）。
//
// (D) new/delete 與 malloc/free 不可混用
//     new  = 配置記憶體（operator new）+ 呼叫建構函數
//     delete = 呼叫解構函數 + 釋放記憶體（operator delete）
//     malloc/free 完全不碰建構／解構。因此：
//       * malloc 出來的物件其建構函式從未執行 → 當成物件使用就是 UB
//       * new 出來的用 free 釋放 → 解構函式沒跑 → 資源洩漏且 UB
//     同理 new[] 必須配 delete[]：delete[] 才知道要對幾個元素呼叫解構函式
//     （多數實作在配置區塊前額外存了元素數量）。用錯配對是 UB。
//
// (E) 為什麼解構函式不該拋出例外
//     若在例外傳播（stack unwinding）過程中，某個解構函式又拋出第二個例外，
//     程式會直接呼叫 std::terminate。這正是 C++11 起解構函式
//     **預設為 noexcept** 的原因。
//     實務準則：解構函式裡的可能失敗操作（如關閉檔案、commit）要自己吞掉錯誤
//     或記錄下來，絕不讓它逸出。需要回報失敗的話，另外提供一個 close() 函式。
//
// 【注意事項 Pay Attention】
// 1. 成員初始化順序只依宣告順序；讓初始化列表與宣告順序一致，並重視 -Wreorder。
// 2. const 成員、引用成員、無預設建構的成員、基底類別 —— 這四種只能用初始化列表。
// 3. `Widget w();` 是函式宣告（Most Vexing Parse），要建立物件請寫 `Widget w{};`。
// 4. = default 與手寫空建構不等價：前者保留平凡性，{} 才會零初始化。
// 5. 讀取未初始化的變數是 UB —— 不要說「一定是垃圾值」，標準不保證任何結果。
// 6. new/delete、new[]/delete[]、malloc/free 三組不可交叉混用。
// 7. 解構函式預設 noexcept；讓例外逸出解構函式會導致 std::terminate。
// 8. 跨翻譯單元的全域物件建構順序未指定（SIOF）；用函式內 static 區域變數迴避。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】建構、解構、生命週期與動態記憶體
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 初始化列表和在建構函數本體裡賦值，差在哪？哪些情況「只能」用初始化列表？
//     答：初始化列表是「直接用給定的值建構成員」；函數體賦值是
//         「先預設建構成員，再賦值覆蓋」—— 多做一次工。
//         四種情況只能用初始化列表：const 成員、引用成員、
//         沒有預設建構函數的成員、以及基底類別的建構。
//     追問：那成員的初始化順序由誰決定？
//         → 只由**宣告順序**決定，與初始化列表的書寫順序無關。
//           寫錯順序又互相依賴就會讀到未初始化的值（UB），
//           編譯器的 -Wreorder 會警告。
//
// 🔥 Q2. 什麼是 RAII？為什麼說它是 C++ 資源管理的核心？
//     答：Resource Acquisition Is Initialization —— 在建構函式取得資源、
//         在解構函式釋放，讓資源的生命週期綁定物件的生命週期。
//         因為 C++ 保證物件離開作用域時一定呼叫解構函式（即使是例外傳播路徑），
//         資源釋放因此變成編譯期就確定的事，不需要 GC，也不需要手動 finally。
//         智慧指標、std::lock_guard、fstream 全都是 RAII。
//     追問：例外拋出時，已經建構完成的區域物件會被解構嗎？
//         → 會。這叫 stack unwinding，正是 RAII 能保證例外安全的關鍵。
//           但要注意：解構函式本身不可以再拋例外，否則會 std::terminate。
//
// 🔥 Q3. new 和 malloc 有什麼差別？可以混用嗎？
//     答：new = operator new 配置記憶體 + 呼叫建構函數，回傳正確型別，
//         失敗時預設拋 std::bad_alloc；
//         malloc 只配置原始位元組，不呼叫建構函數，回傳 void*，失敗回 nullptr。
//         絕對不可混用：malloc 的記憶體上物件沒被建構、
//         new 的物件用 free 釋放則解構函式不會執行，兩者都是 UB。
//     追問：new[] 為什麼一定要配 delete[]？
//         → delete[] 才會對每一個元素呼叫解構函式；多數實作會在配置區塊前
//           多存一個元素數量。用 delete 釋放 new[] 的記憶體是 UB。
//
// ⚠️ 陷阱 1. `Widget w();` 為什麼沒有建立物件？
//     答：它被解析成「宣告一個名為 w、不收參數、回傳 Widget 的函式」。
//         這就是 Most Vexing Parse：C++ 文法規定，一段程式碼只要**可能**
//         被解析成宣告，就會被解析成宣告。
//         解法是用大括號：`Widget w{};`。
//     為什麼會錯：大家從 `Widget w(1,2);` 這種有參數的寫法類推，
//         以為去掉參數只是「呼叫無參數版本」。但沒有參數時，
//         這串符號與函式宣告在文法上完全一樣，而標準選擇了宣告那一邊。
//         這也是現代 C++ 建議優先用 {} 初始化的原因之一。
//
// ⚠️ 陷阱 2. 為什麼這個 class 有問題，即使我在解構函式裡寫了 delete[]？
//         class A { int* p_; public: A() : p_(new int[100]) {}
//                   ~A() { delete[] p_; } };
//         A a1; A a2 = a1;      // 之後行為異常
//     答：問題不在解構函式，而在**拷貝**。編譯器產生的預設拷貝建構函式做的是
//         淺拷貝：a2.p_ 與 a1.p_ 指向同一塊記憶體。
//         兩者解構時各自 delete[] 同一個指標 → double free，這是 UB，
//         標準不保證任何結果（可能崩潰、可能靜默毀壞堆積、也可能看似正常）。
//     為什麼會錯：把「我有寫 delete」當成「資源管理做完了」。
//         實際上只要類別自己管理資源，就必須同時考慮
//         解構、拷貝建構、拷貝賦值（Rule of Three；加上移動則是 Rule of Five）。
//         現代解法是 Rule of Zero：改用 std::vector 或智慧指標，
//         讓那些已經寫好的類別替你處理，自己一個特殊函式都不用寫。
//         （這正是第 27～35 課的主題。）
//
// 【LeetCode 實戰範例】—— 從缺（刻意不加）
//     本專題是「物件生命週期與資源管理」，它影響的是你**怎麼寫**任何一題，
//     而不是某一題的解法本身。LeetCode 的判題只看回傳值，
//     不會檢查你有沒有洩漏記憶體，也沒有題目在考建構／解構順序。
//     真正對應 OOP 設計的題目（146 LRU Cache、155 Min Stack、
//     707 Design Linked List）已分別放在更貼題的檔案示範
//     —— 例如 155 見 summary20-26.cpp（封裝不變量），
//     146 見第 28 課 LRUCache。此處硬塞一題只會模糊本檔重點。
// ═══════════════════════════════════════════════════════════════════════════

// ================================================================
// 第 13 課：建構函數（Constructor）基礎
// ================================================================
// ┌─────────────────┬──────────────────────────────────┐
// │ 特徵             │ 說明                              │
// ├─────────────────┼──────────────────────────────────┤
// │ 函數名           │ 必須與類別名完全相同              │
// │ 返回值           │ 沒有返回值，連 void 都不寫        │
// │ 調用時機         │ 對象創建時自動調用                │
// │ 可以重載         │ 一個類別可以有多個建構函數        │
// │ 存取權限         │ 通常是 public                     │
// └─────────────────┴──────────────────────────────────┘
//
// 四種調用時機：
// ┌──────────────┬───────────────────────────────┐
// │ 對象類型      │ 建構時機                       │
// ├──────────────┼───────────────────────────────┤
// │ 全域對象      │ main() 之前                    │
// │ 局部對象      │ 執行到宣告語句時                │
// │ 區塊內對象    │ 進入區塊，執行到宣告時          │
// │ 動態對象      │ new 運算子執行時                │
// └──────────────┴───────────────────────────────┘
//
// 三大陷阱：
//   1. void ClassName() { } → 不是建構函數，是普通函數！
//   2. ClassName obj(); → 函數宣告（Most Vexing Parse），不是對象創建！
//      正確寫法：ClassName obj; 或 ClassName obj{};
//   3. 定義了任何建構函數後，編譯器不再自動生成預設建構函數

class L13_Car {
private:
    string brand;
    int year;
    double mileage;

public:
    // 建構函數 1：預設建構
    L13_Car() : brand("未知"), year(2024), mileage(0) {}

    // 建構函數 2：重載——部分指定
    L13_Car(const string& b) : brand(b), year(2024), mileage(0) {}

    // 建構函數 3：重載——完整指定，帶驗證
    L13_Car(const string& b, int y, double mi) : brand(b) {
        year = (y >= 1886 && y <= 2026) ? y : 2024;      // 年份驗證
        mileage = (mi >= 0) ? mi : 0;                     // 里程驗證
        if (y != year || mi != mileage)
            cout << "    (警告：數據被修正)" << endl;
    }

    void print() const {
        cout << "  " << year << " " << brand << ", " << mileage << " km" << endl;
    }
};

void demo_lesson13() {
    cout << "\n===== 第 13 課：建構函數基礎 =====" << endl;

    // 重載示範
    L13_Car c1;                             // 預設建構
    L13_Car c2("Toyota");                   // 部分指定
    L13_Car c3("BMW", 2020, 35000);         // 完整指定
    L13_Car c4("時光機", 1800, -100);       // 驗證修正
    c1.print(); c2.print(); c3.print(); c4.print();

    // 區塊內建構/解構時機
    {
        L13_Car block("區塊內");
        block.print();
    } // block 在此離開作用域

    // 動態對象
    L13_Car* heap = new L13_Car("動態");
    heap->print();
    delete heap;

    cout << "  陷阱提醒：" << endl;
    cout << "    Student s(); ← 是函數宣告（Most Vexing Parse）！" << endl;
    cout << "    正確：Student s; 或 Student s{};" << endl;
}


// ================================================================
// 第 14 課：預設建構函數（Default Constructor）
// ================================================================
// 定義：不需要傳入任何參數就能調用的建構函數
//   形式 1：MyClass() { }              → 無參數
//   形式 2：MyClass(int x = 0) { }    → 所有參數都有預設值
//
// 編譯器自動生成的預設建構函數：
// ┌────────────────────────────────────┬────────────────────────┐
// │ 成員類型                           │ 自動生成的行為          │
// ├────────────────────────────────────┼────────────────────────┤
// │ 基本型別（int, double 等）         │ 不初始化（垃圾值）      │
// │ 類別型別（string, vector 等）      │ 調用該類別的預設建構函數│
// └────────────────────────────────────┴────────────────────────┘
//
// = default vs 手動空建構 { } 的差異：
// ┌────────────────────┬───────────────────┬────────────────────┐
// │ 場景                │ = default          │ 手動寫 { }          │
// ├────────────────────┼───────────────────┼────────────────────┤
// │ T obj;              │ 基本型別不初始化   │ 基本型別不初始化     │
// │ T obj{};            │ 觸發值初始化，歸零 │ 只調用空建構，不歸零 │
// │ 語義                │ 平凡(trivial)建構  │ 使用者定義的建構     │
// └────────────────────┴───────────────────┴────────────────────┘
//
// = delete：明確禁止預設建構（表達設計意圖）
// 全預設參數的建構函數也算預設建構函數（注意歧義！）
// 對象陣列 T arr[N]; 需要預設建構函數

class L14_A { public: int value; L14_A() = default; };   // 平凡建構
class L14_B { public: int value; L14_B() { } };          // 使用者定義

// 全預設參數 → 也是預設建構函數
class L14_Color {
    int r, g, b;
public:
    L14_Color(int red = 0, int green = 0, int blue = 0)
        : r(red), g(green), b(blue) {}
    void print() const { cout << "  RGB(" << r << "," << g << "," << b << ")" << endl; }
};

// = delete：沒有連接資訊就不能創建
class L14_DbConn {
    string host; int port;
public:
    L14_DbConn() = delete;
    L14_DbConn(const string& h, int p) : host(h), port(p) {}
    void print() const { cout << "  " << host << ":" << port << endl; }
};

void demo_lesson14() {
    cout << "\n===== 第 14 課：預設建構函數 =====" << endl;

    // = default vs 手動空建構差異
    // ⚠️ a1/b1 刻意「不」印出它們的 value：預設初始化的 int 成員未被初始化，
    //    讀取它是 undefined behavior，標準不保證任何結果（不是「一定是垃圾值」，
    //    也不是「一定是 0」）。這裡只示範語法差異，不去觀察那個值。
    //    [[maybe_unused]] 是 C++17 屬性，用來明確表達「宣告了但故意不使用」。
    [[maybe_unused]] L14_A a1;   // 預設初始化：value 未初始化，讀取即 UB
    [[maybe_unused]] L14_B b1;   // 同上
    L14_A a2{};     // 值初始化 → value = 0（= default 保留平凡性，故會零初始化）
    L14_B b2{};     // L14_B 有使用者定義的建構函數 → 只呼叫它，value 仍未初始化
    cout << "  = default + {}: a2.value = " << a2.value << " (保證為 0)" << endl;
    cout << "  手動定義空建構 + {}: b2.value 未被初始化，讀取即 UB，故不印出" << endl;
    cout << "  → 關鍵差異：= default 保留平凡性，{} 會做零初始化；" << endl;
    cout << "     手動寫的空建構函數是「使用者定義」的，{} 只會呼叫它，不做零初始化。" << endl;

    // 全預設參數
    L14_Color black;                   // (0,0,0)
    L14_Color red(255);                // (255,0,0)
    L14_Color custom(100, 200, 50);    // (100,200,50)
    black.print(); red.print(); custom.print();

    // = delete
    // L14_DbConn db;  // 編譯錯誤！
    L14_DbConn db("localhost", 5432);
    db.print();

    // 陣列需要預設建構函數
    L14_Color colors[3];  // 3 次調用預設建構
    cout << "  陣列 colors[0]: "; colors[0].print();

    cout << "  重點：= default 保留平凡性(triviality)，手動 { } 不保留" << endl;
    cout << "  注意：不能同時有無參建構 + 全預設參數建構（歧義！）" << endl;
}


// ================================================================
// 第 15 課：帶參數的建構函數
// ================================================================
// 傳遞方式推薦：
//   基本型別（int, double）→ 值傳遞
//   類別型別（string, vector）→ const 引用（避免無謂複製）
//   const 引用可以綁定字面值和臨時對象（普通引用不行）
//
// 參數同名問題的四種解決方案：
//   1. this->name = name;         // this 指針
//   2. m_name = name;             // m_ 前綴
//   3. name_ = name;              // 底線後綴（Google 風格）
//   4. : name(name) { }           // 初始化列表（最推薦！）
//
// 五種調用語法：
//   T obj(args);                  // 直接初始化
//   T obj{args};                  // 統一初始化（禁止窄化轉換！）
//   T obj = T(args);              // 拷貝初始化
//   T obj = {args};               // 等號 + 大括號
//   T* p = new T(args);           // 動態分配
//
// explicit：禁止單參數建構的隱式轉換
//   Distance d = 50.0;            → 允許（沒有 explicit）
//   explicit 後必須 SafeDistance d(50.0); 明確調用

class L15_Player {
    string name_;     // Google 風格底線後綴
    int level_;
public:
    // explicit + const 引用 + 預設參數
    explicit L15_Player(const string& name, int level = 1)
        : name_(name), level_(level) {}  // 初始化列表中同名不衝突
    void print() const { cout << "  " << name_ << " Lv." << level_ << endl; }
};

// explicit 對比
class L15_Dist {
    double m;
public:
    L15_Dist(double meters) : m(meters) {}
    void print() const { cout << "  " << m << " m" << endl; }
};

class L15_SafeDist {
    double m;
public:
    explicit L15_SafeDist(double meters) : m(meters) {}
    void print() const { cout << "  " << m << " m" << endl; }
};

void showDist(L15_Dist d) { d.print(); }

void demo_lesson15() {
    cout << "\n===== 第 15 課：帶參數的建構函數 =====" << endl;

    // 五種調用語法
    L15_Player p1("勇者", 10);         // 直接初始化
    L15_Player p2{"法師", 5};          // 統一初始化
    L15_Player p3 = L15_Player("盜賊", 7); // 拷貝初始化
    // L15_Player p4 = {"弓手", 3};    // 錯誤！explicit 禁止
    p1.print(); p2.print(); p3.print();

    // 隱式轉換 vs explicit
    L15_Dist d1 = 100.0;              // OK：隱式轉換（double → L15_Dist）
    cout << "  隱式轉換 L15_Dist d1 = 100.0; →"; d1.print();
    showDist(200.0);                   // OK：隱式轉換
    L15_SafeDist sd1(100.0);           // OK：直接初始化
    // L15_SafeDist sd2 = 100.0;       // 錯誤！explicit 禁止

    // 窄化轉換防護
    // L15_Player bad{string("test"), 3.14};  // 錯誤！double→int 是窄化
    cout << "  {} 初始化禁止窄化轉換，比 () 更安全" << endl;
    cout << "  explicit 防止單參數隱式轉換，建議單參數建構都加 explicit" << endl;
}


// ================================================================
// 第 16 課：建構函數初始化列表（Member Initializer List）
// ================================================================
// 語法：ClassName(params) : member1(val1), member2(val2) { }
//
// 初始化(一步) vs 賦值(兩步)：
//   初始化列表 → 直接用參數建構成員（一步到位）
//   函數體賦值 → 先預設建構，再賦值（兩步，多一次操作）
//
// 四種必須使用初始化列表的情況：
// ┌──────────────────────────────────┬──────────────────────────────┐
// │ 情況                              │ 原因                          │
// ├──────────────────────────────────┼──────────────────────────────┤
// │ const 成員變數                    │ const 只能初始化，不能賦值    │
// │ 引用成員變數                      │ 引用必須在宣告時綁定          │
// │ 沒有預設建構函數的成員物件         │ 無法先預設建構再賦值          │
// │ 基類的建構函數（繼承時）           │ 基類必須在派生類之前建構      │
// └──────────────────────────────────┴──────────────────────────────┘
//
// 順序陷阱：初始化順序按成員「宣告順序」，不是列表「書寫順序」！
//   最佳實踐：書寫順序永遠和宣告順序一致，加 -Wall -Wextra 偵測
//
// C++11 類別內預設值 + 初始化列表覆蓋規則：
//   初始化列表有指定 → 用列表的值
//   初始化列表沒有   → 用類別內預設值
//   都沒有           → 基本型別垃圾值 / 類別型別調用預設建構

// 展示四種必須使用的情況
class L16_Engine {
    int hp; string fuel;
public:
    L16_Engine(int h, const string& f) : hp(h), fuel(f) {}
    void print() const { cout << hp << "HP(" << fuel << ")"; }
};

class L16_Vehicle {
    const int vin;              // ① const 成員
    ostream& log;               // ② 引用成員
    L16_Engine engine;          // ③ 無預設建構的成員
    string brand;

    // C++11 類別內預設值
    int speed = 0;
    bool running = false;

public:
    L16_Vehicle(int id, ostream& os, int hp, const string& fuel, const string& b)
        : vin(id),              // ① const → 必須用初始化列表
          log(os),              // ② 引用 → 必須用初始化列表
          engine(hp, fuel),     // ③ 無預設建構 → 必須用初始化列表
          brand(b)              // speed, running 使用類別內預設值
    {}

    void print() const {
        cout << "  [" << vin << "] " << brand << " ";
        engine.print();
        cout << " 速度:" << speed << " " << (running ? "運行中" : "停止") << endl;
    }
};

// 順序陷阱示範
class L16_OrderDemo {
    int length;   // 第 1 個宣告 → 先初始化
    int doubled;  // 第 2 個宣告 → 後初始化
public:
    // 正確：書寫順序和宣告順序一致
    L16_OrderDemo(int val) : length(val), doubled(length * 2) {}
    void print() const { cout << "  length=" << length << " doubled=" << doubled << endl; }
};

// ④ 基類建構（繼承時必須用初始化列表）
class L16_Animal {
    string species;
public:
    L16_Animal(const string& s) : species(s) {}
    string getSpecies() const { return species; }
};

class L16_Dog : public L16_Animal {
    string name;
public:
    L16_Dog(const string& n)
        : L16_Animal("犬科"),   // ④ 基類建構 → 必須用初始化列表
          name(n) {}
    void print() const { cout << "  " << name << " (" << getSpecies() << ")" << endl; }
};

void demo_lesson16() {
    cout << "\n===== 第 16 課：建構函數初始化列表 =====" << endl;

    // 四種必須使用的情況
    L16_Vehicle v(1001, cout, 300, "汽油", "BMW");
    v.print();

    // 基類建構
    L16_Dog dog("旺財");
    dog.print();

    // 順序陷阱
    L16_OrderDemo od(5);
    od.print();

    cout << "  效能：初始化列表(1步) > 函數體賦值(2步)" << endl;
    cout << "  規則：初始化順序按宣告順序，不是書寫順序！" << endl;
    cout << "  最佳實踐：永遠使用初始化列表，書寫順序=宣告順序" << endl;
}


// ================================================================
// 第 17 課：解構函數（Destructor）
// ================================================================
// 語法：~ClassName()，無返回值，無參數，不能重載，每個類別只能一個
//
// 建構 vs 解構對照：
// ┌──────────────┬─────────────────────┬─────────────────────┐
// │ 特徵          │ 建構函數             │ 解構函數             │
// ├──────────────┼─────────────────────┼─────────────────────┤
// │ 名稱          │ ClassName()          │ ~ClassName()         │
// │ 參數          │ 可以有（可重載）     │ 不能有（不可重載）   │
// │ 數量          │ 可以有多個           │ 只能有一個           │
// │ 調用時機      │ 對象創建時           │ 對象銷毀時           │
// │ 職責          │ 獲取資源/初始化      │ 釋放資源/清理        │
// └──────────────┴─────────────────────┴─────────────────────┘
//
// 解構順序與建構順序相反（LIFO，後建構先解構）
//
// RAII（Resource Acquisition Is Initialization）：
//   建構函數獲取資源 → 解構函數釋放資源 → 離開作用域自動清理
//   應用：動態記憶體、檔案、鎖、計時器
//
// 編譯器自動生成的解構函數：
//   - 基本型別(int) → 不做任何事
//   - 類別型別(string) → 調用成員的解構函數
//   - 裸指標(int*) → 不做任何事！不會 delete！→ 必須手動寫
//
// 解構函數中不要拋出異常！（try-catch 包住）

class L17_Tracker {
    string name;
    static int count;
public:
    L17_Tracker(const string& n) : name(n) {
        count++;
        cout << "  [+] " << name << " (存活:" << count << ")" << endl;
    }
    ~L17_Tracker() {
        count--;
        cout << "  [-] " << name << " (存活:" << count << ")" << endl;
    }
    static int getCount() { return count; }
};
int L17_Tracker::count = 0;

// RAII 示範：動態陣列
class L17_DynArray {
    int* data;
    int size;
public:
    L17_DynArray(int sz) : data(new int[sz]()), size(sz) {
        cout << "  [建構] 分配 " << size << " 個 int" << endl;
    }
    ~L17_DynArray() {
        delete[] data;
        cout << "  [解構] 釋放 " << size << " 個 int" << endl;
    }
    void set(int i, int v) { if (i >= 0 && i < size) data[i] = v; }
    int get(int i) const { return (i >= 0 && i < size) ? data[i] : -1; }
};

// RAII 示範：自動計時器
class L17_ScopedTimer {
    string task;
    chrono::high_resolution_clock::time_point start;
public:
    L17_ScopedTimer(const string& t) : task(t),
        start(chrono::high_resolution_clock::now()) {}
    ~L17_ScopedTimer() {
        auto ms = chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start).count();
        cout << "  [計時] " << task << ": " << ms << " ms" << endl;
    }
};

void demo_lesson17() {
    cout << "\n===== 第 17 課：解構函數 =====" << endl;

    // LIFO 解構順序 + static 追蹤
    {
        L17_Tracker a("Alpha");
        L17_Tracker b("Beta");
        cout << "  --- 即將離開區塊 ---" << endl;
    } // Beta 先解構，Alpha 後解構（LIFO）

    // RAII：動態陣列自動釋放
    {
        L17_DynArray arr(5);
        arr.set(0, 42);
        cout << "  arr[0] = " << arr.get(0) << endl;
    } // 自動 delete[]

    // RAII：自動計時器
    {
        L17_ScopedTimer timer("模擬計算");
        volatile int sum = 0;
        for (int i = 0; i < 1000000; i++) sum += i;
    } // 自動印出耗時

    cout << "  重點：裸指標成員不會被自動 delete → 必須自己寫解構函數" << endl;
    cout << "  解構函數中不要拋異常（用 try-catch 包住）" << endl;
}


// ================================================================
// 第 18 課：對象的生命週期（Object Lifetime）
// ================================================================
// 四種存儲期：
// ┌──────────┬────────────┬────────────────┬──────────────────┐
// │ 存儲期    │ 宣告位置    │ 誕生            │ 死亡              │
// ├──────────┼────────────┼────────────────┼──────────────────┤
// │ 自動(棧)  │ 函數/區塊內 │ 執行到宣告時    │ 離開作用域時      │
// │ 靜態(全域)│ 函數外      │ main() 之前     │ main() 之後       │
// │ 靜態(局部)│ 函數內static│ 第一次執行到時  │ main() 之後       │
// │ 動態(堆)  │ new 創建    │ new 時          │ delete 時         │
// └──────────┴────────────┴────────────────┴──────────────────┘
//
// for 迴圈：每次迭代都是完整的 建構 → 使用 → 解構 週期
//
// 靜態局部對象（Lazy Initialization）：
//   只在第一次執行到時初始化，之後不再重複
//   C++11 保證線程安全（Magic Statics）
//   → 單例模式基礎
//
// Static Initialization Order Fiasco：
//   不同 .cpp 的全域對象初始化順序未定義！
//   解決：用函數包裝靜態局部對象
//
// 臨時對象生命週期：
//   Temp("x").show();          → 語句結束時死亡
//   Temp t = Temp("x");        → 和 t 的生命週期相同
//   const Temp& r = Temp("x"); → 延長到 r 離開作用域
//
// 懸空引用/野指標陷阱：
//   不要返回局部對象的引用 → Dangling Reference
//   不要返回局部對象的地址 → Dangling Pointer
//   安全做法：返回值（副本）或用 new 分配

class L18_Probe {
    string name;
public:
    L18_Probe(const string& n) : name(n) { cout << "  [+] " << name << endl; }
    ~L18_Probe() { cout << "  [-] " << name << endl; }
    void work() const { cout << "  [=] " << name << endl; }
};

// 延遲初始化示範
class L18_Singleton {
    string data;
public:
    L18_Singleton(const string& d) : data(d) {
        cout << "  [Singleton建構] " << data << endl;
    }
    void use() const { cout << "  [Singleton使用] " << data << endl; }
};

L18_Singleton& getSingleton() {
    static L18_Singleton instance("唯一實例");  // 只初始化一次
    return instance;
}

// Static Init Order Fiasco 解決方案
class L18_Config {
    int maxConn;
public:
    L18_Config() : maxConn(100) {}
    int getMax() const { return maxConn; }
};

L18_Config& getConfig() {
    static L18_Config cfg;
    return cfg;
}

void demo_lesson18() {
    cout << "\n===== 第 18 課：對象的生命週期 =====" << endl;

    // 嵌套作用域 LIFO
    cout << "--- 嵌套作用域 LIFO ---" << endl;
    {
        L18_Probe a("外層");
        {
            L18_Probe b("內層");
        } // b 先死
    } // a 後死

    // for 迴圈：每次迭代完整建構→解構
    cout << "\n--- for 迴圈：每次迭代 建構→解構 ---" << endl;
    for (int i = 0; i < 2; i++) {
        L18_Probe p("迴圈#" + to_string(i));
        p.work();
    }

    // 延遲初始化（只初始化一次）
    cout << "\n--- 延遲初始化（static local）---" << endl;
    getSingleton().use();  // 第一次：建構 + 使用
    getSingleton().use();  // 第二次：只使用（不再建構）

    // 臨時對象生命週期
    cout << "\n--- 臨時對象生命週期 ---" << endl;
    L18_Probe("臨時物件").work();   // 語句結束即死亡
    cout << "  (臨時物件已死亡)" << endl;

    const L18_Probe& ref = L18_Probe("const引用續命");
    ref.work();
    cout << "  (ref 綁定的臨時物件還活著)" << endl;
    // 臨時物件在 ref 離開作用域時才死亡

    cout << "\n  陷阱：不要返回局部對象的引用(懸空引用)或地址(野指標)" << endl;
    cout << "  解決：返回值(副本) 或 new + delete" << endl;
    cout << "  全域初始化順序陷阱 → 用函數包裝 static 局部對象解決" << endl;
}


// ================================================================
// 第 19 課：動態對象的創建與銷毀（new / delete）
// ================================================================
// new  做兩件事：① 分配記憶體  ② 調用建構函數
// delete 做兩件事：① 調用解構函數  ② 釋放記憶體
//
// new vs malloc：
// ┌───────────┬──────────────────────────┬─────────────────────┐
// │           │ new / delete             │ malloc / free        │
// ├───────────┼──────────────────────────┼─────────────────────┤
// │ 調用建構   │ 是                       │ 否！                 │
// │ 調用解構   │ 是（delete 時）          │ 否！（free 時）      │
// │ 返回型別   │ 正確的指標型別            │ void*（需強轉）      │
// │ 失敗行為   │ 拋出 bad_alloc           │ 返回 NULL            │
// └───────────┴──────────────────────────┴─────────────────────┘
//
// new[] / delete[] 配對：
//   new T[n]  → delete[] p;    // 正確
//   new T     → delete p;      // 正確
//   混用 → 未定義行為！
//
// delete nullptr 安全，double delete 是未定義行為
// 好習慣：delete 後設為 nullptr
//
// 記憶體洩漏四場景：
//   1. 忘記 delete
//   2. 覆蓋指標（丟失原地址）
//   3. 提前 return（跳過 delete）
//   4. 異常中斷（跳過 delete）
//
// RAII 解決一切：建構獲取 → 解構釋放 → 不管怎麼離開都安全
// 現代 C++：unique_ptr / make_unique 取代裸 new/delete

class L19_Weapon {
    string name;
    int damage;
    int* durability;
public:
    L19_Weapon(const string& n, int dmg, int dur)
        : name(n), damage(dmg), durability(new int(dur)) {
        cout << "  [鍛造] " << name << " (攻擊:" << damage
             << " 耐久:" << *durability << ")" << endl;
    }
    ~L19_Weapon() {
        cout << "  [銷毀] " << name << endl;
        delete durability;
    }
    void use() {
        if (*durability > 0) {
            *durability -= 10;
            cout << "  使用 " << name << " (耐久:" << *durability << ")" << endl;
        } else {
            cout << "  " << name << " 已損壞！" << endl;
        }
    }
};

void demo_lesson19() {
    cout << "\n===== 第 19 課：new / delete =====" << endl;

    // 基本語法
    cout << "--- 基本型別 ---" << endl;
    int* p1 = new int(42);
    int* p2 = new int{100};
    cout << "  *p1 = " << *p1 << ", *p2 = " << *p2 << endl;
    delete p1; delete p2;

    // 動態陣列
    cout << "\n--- 動態陣列 ---" << endl;
    int* arr = new int[5]{10, 20, 30, 40, 50};
    cout << "  arr: ";
    for (int i = 0; i < 5; i++) cout << arr[i] << " ";
    cout << endl;
    delete[] arr;  // 必須用 delete[] 對應 new[]

    // 裸指標方式（手動管理）
    cout << "\n--- 裸指標（手動管理）---" << endl;
    {
        L19_Weapon* sword = new L19_Weapon("鐵劍", 25, 50);
        sword->use();
        sword->use();
        delete sword;  // 必須手動 delete
    }

    // unique_ptr 方式（自動管理）
    cout << "\n--- unique_ptr（自動管理）---" << endl;
    {
        auto bow = make_unique<L19_Weapon>("長弓", 20, 40);
        bow->use();
        // 不需要 delete！離開作用域自動清理
    }

    // 異常安全示範
    cout << "\n--- 異常安全（unique_ptr）---" << endl;
    try {
        auto staff = make_unique<L19_Weapon>("法杖", 35, 30);
        staff->use();
        throw runtime_error("戰鬥中斷！");
    } catch (const exception& e) {
        cout << "  異常: " << e.what() << " (法杖已自動清理)" << endl;
    }

    // delete nullptr 安全
    int* safe = nullptr;
    delete safe;  // 完全安全

    cout << "\n  記憶體洩漏四場景：忘記/覆蓋指標/提前return/異常" << endl;
    cout << "  RAII 解決一切：建構獲取→解構釋放→自動管理" << endl;
    cout << "  現代 C++：用 unique_ptr/make_unique 取代裸 new/delete" << endl;
}


// ================================================================
// 主程式
// ================================================================
// 【日常實務範例】RAII 連線池租借：把「一定要還」交給編譯器保證
// ================================================================
//   情境：服務要跟資料庫借連線，用完必須歸還。這是實務上最容易出事的地方：
//     * 中途 return  → 忘記歸還
//     * 中途拋例外   → 更難記得歸還
//     * 多個離開路徑 → 每條都要寫一次歸還，漏一條就是資源洩漏
//   手動寫法（錯誤示範）：
//       auto* c = pool.acquire();
//       if (bad) return;            // ← 洩漏！
//       doWork(c);
//       pool.release(c);
//   RAII 寫法：把「歸還」放進解構函式，於是**所有**離開路徑
//   （正常 return、提早 return、例外傳播）都保證會歸還 —— 這是語言層級的保證，
//   不是靠自律。
//
//   本範例用到的本專題概念：
//     第 13/15 課 建構函式取得資源、第 16 課 初始化列表（含引用成員，只能用它）
//     第 17 課 解構函式釋放資源、第 18 課 作用域決定生命週期
//     第 19 課 對照手動 new/delete 的風險
// ================================================================
class ConnectionPool {
    int m_available;
    int m_peakInUse = 0;
    int m_total;
public:
    explicit ConnectionPool(int n) : m_available(n), m_total(n) {}

    bool acquire() {
        if (m_available <= 0) return false;
        --m_available;
        int inUse = m_total - m_available;
        if (inUse > m_peakInUse) m_peakInUse = inUse;
        return true;
    }
    void release() { if (m_available < m_total) ++m_available; }

    int available() const { return m_available; }
    int peakInUse() const { return m_peakInUse; }
};

// RAII 租借守衛：建構=借，解構=還
class PooledConnection {
    ConnectionPool& m_pool;     // 引用成員 → 只能用初始化列表初始化（第 16 課）
    bool m_ok;
    std::string m_name;
public:
    PooledConnection(ConnectionPool& pool, const std::string& name)
        : m_pool(pool), m_ok(pool.acquire()), m_name(name) {
        cout << "    [借出] " << m_name
             << (m_ok ? " 成功" : " 失敗（池已空）")
             << "，剩餘 " << m_pool.available() << endl;
    }

    ~PooledConnection() {
        if (m_ok) {
            m_pool.release();
            cout << "    [歸還] " << m_name
                 << "，剩餘 " << m_pool.available() << endl;
        }
    }

    // 租借憑證不可複製（否則會重複歸還）——Rule of Three 的實際考量
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    bool valid() const { return m_ok; }
};

// 提早 return：RAII 仍然保證歸還
static void queryWithEarlyReturn(ConnectionPool& pool, bool bad) {
    PooledConnection conn(pool, "查詢A");
    if (!conn.valid()) return;
    if (bad) {
        cout << "    參數有誤，提早 return —— 但解構函式仍會執行" << endl;
        return;                       // 不需要手動歸還
    }
    cout << "    查詢完成" << endl;
}

// 拋出例外：stack unwinding 一樣會呼叫解構函式
static void queryWithThrow(ConnectionPool& pool) {
    PooledConnection conn(pool, "查詢B");
    cout << "    即將拋出例外..." << endl;
    throw std::runtime_error("資料庫逾時");
}

void demo_practical_raii() {
    cout << "\n===== 日常實務：RAII 連線池（保證歸還）=====" << endl;

    ConnectionPool pool(2);
    cout << "  連線池容量 2，目前可用 " << pool.available() << endl;

    cout << "\n  (1) 正常流程：" << endl;
    queryWithEarlyReturn(pool, false);

    cout << "\n  (2) 提早 return：" << endl;
    queryWithEarlyReturn(pool, true);

    cout << "\n  (3) 拋出例外（stack unwinding 仍會解構）：" << endl;
    try {
        queryWithThrow(pool);
    } catch (const std::exception& e) {
        cout << "    捕獲例外：" << e.what() << endl;
    }

    cout << "\n  (4) 巢狀作用域：解構順序是 LIFO（後建構的先解構）" << endl;
    {
        PooledConnection c1(pool, "外層");
        {
            PooledConnection c2(pool, "內層");
            cout << "    內層作用域中，可用 " << pool.available() << endl;
        }   // c2 在此解構
        cout << "    離開內層後，可用 " << pool.available() << endl;
    }       // c1 在此解構

    cout << "\n  最終可用連線 " << pool.available()
         << "（與初始相同 → 三條離開路徑都沒有洩漏）" << endl;
    cout << "  歷史最高同時使用 " << pool.peakInUse() << " 條" << endl;
    cout << "  → 正常 return、提早 return、例外傳播，全部由解構函式保證歸還。" << endl;
}

// ================================================================
int main() {
    cout << "============================================================" << endl;
    cout << "  第 13～19 課總複習：建構函數 → 解構函數 → 生命週期 → new/delete" << endl;
    cout << "============================================================" << endl;

    demo_lesson13();   // 建構函數基礎
    demo_lesson14();   // 預設建構函數
    demo_lesson15();   // 帶參數的建構函數
    demo_lesson16();   // 初始化列表
    demo_lesson17();   // 解構函數
    demo_lesson18();   // 對象的生命週期
    demo_lesson19();   // new / delete
    demo_practical_raii();   // 日常實務：RAII 連線池

    // ================================================================
    // 七課速查表
    // ================================================================
    cout << "\n============================================================" << endl;
    cout << "七課速查表" << endl;
    cout << "============================================================" << endl;
    cout << "第13課 建構函數基礎：" << endl;
    cout << "  - 函數名=類別名，無返回值，自動調用" << endl;
    cout << "  - 可重載，帶驗證是第一道防線" << endl;
    cout << "  - 陷阱：Most Vexing Parse, 定義建構後預設建構消失" << endl;
    cout << "第14課 預設建構函數：" << endl;
    cout << "  - = default 保留平凡性，T obj{} 會歸零" << endl;
    cout << "  - = delete 明確禁止預設建構" << endl;
    cout << "  - 陣列 T arr[N] 需要預設建構函數" << endl;
    cout << "第15課 帶參建構函數：" << endl;
    cout << "  - 類別型別參數推薦 const&" << endl;
    cout << "  - 同名解決最推薦初始化列表 : name(name)" << endl;
    cout << "  - explicit 禁止隱式轉換，{} 禁止窄化" << endl;
    cout << "第16課 初始化列表：" << endl;
    cout << "  - 一步到位 vs 函數體賦值兩步" << endl;
    cout << "  - 四必須：const/引用/無預設建構成員/基類" << endl;
    cout << "  - 順序按宣告順序，書寫順序要一致" << endl;
    cout << "第17課 解構函數：" << endl;
    cout << "  - ~ClassName()，無參數，不能重載" << endl;
    cout << "  - LIFO 解構順序，RAII 自動管理資源" << endl;
    cout << "  - 裸指標必須手動 delete，不要拋異常" << endl;
    cout << "第18課 對象生命週期：" << endl;
    cout << "  - 四種存儲期：自動/靜態全域/靜態局部/動態" << endl;
    cout << "  - static local 延遲初始化，C++11 線程安全" << endl;
    cout << "  - 不要返回局部對象的引用或地址" << endl;
    cout << "第19課 new/delete：" << endl;
    cout << "  - new=分配+建構，delete=解構+釋放" << endl;
    cout << "  - new[]/delete[] 必須配對，不能混用" << endl;
    cout << "  - unique_ptr/make_unique 取代裸 new/delete" << endl;
    cout << "============================================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary13-19.cpp -o summary13-19

// === 預期輸出 ===
// ============================================================
//   第 13～19 課總複習：建構函數 → 解構函數 → 生命週期 → new/delete
// ============================================================
//
// ===== 第 13 課：建構函數基礎 =====
//     (警告：數據被修正)
//   2024 未知, 0 km
//   2024 Toyota, 0 km
//   2020 BMW, 35000 km
//   2024 時光機, 0 km
//   2024 區塊內, 0 km
//   2024 動態, 0 km
//   陷阱提醒：
//     Student s(); ← 是函數宣告（Most Vexing Parse）！
//     正確：Student s; 或 Student s{};
//
// ===== 第 14 課：預設建構函數 =====
//   = default + {}: a2.value = 0 (保證為 0)
//   手動定義空建構 + {}: b2.value 未被初始化，讀取即 UB，故不印出
//   → 關鍵差異：= default 保留平凡性，{} 會做零初始化；
//      手動寫的空建構函數是「使用者定義」的，{} 只會呼叫它，不做零初始化。
//   RGB(0,0,0)
//   RGB(255,0,0)
//   RGB(100,200,50)
//   localhost:5432
//   陣列 colors[0]:   RGB(0,0,0)
//   重點：= default 保留平凡性(triviality)，手動 { } 不保留
//   注意：不能同時有無參建構 + 全預設參數建構（歧義！）
//
// ===== 第 15 課：帶參數的建構函數 =====
//   勇者 Lv.10
//   法師 Lv.5
//   盜賊 Lv.7
//   隱式轉換 L15_Dist d1 = 100.0; →  100 m
//   200 m
//   {} 初始化禁止窄化轉換，比 () 更安全
//   explicit 防止單參數隱式轉換，建議單參數建構都加 explicit
//
// ===== 第 16 課：建構函數初始化列表 =====
//   [1001] BMW 300HP(汽油) 速度:0 停止
//   旺財 (犬科)
//   length=5 doubled=10
//   效能：初始化列表(1步) > 函數體賦值(2步)
//   規則：初始化順序按宣告順序，不是書寫順序！
//   最佳實踐：永遠使用初始化列表，書寫順序=宣告順序
//
// ===== 第 17 課：解構函數 =====
//   [+] Alpha (存活:1)
//   [+] Beta (存活:2)
//   --- 即將離開區塊 ---
//   [-] Beta (存活:1)
//   [-] Alpha (存活:0)
//   [建構] 分配 5 個 int
//   arr[0] = 42
//   [解構] 釋放 5 個 int
//   [計時] 模擬計算: 1 ms
//   重點：裸指標成員不會被自動 delete → 必須自己寫解構函數
//   解構函數中不要拋異常（用 try-catch 包住）
//
// ===== 第 18 課：對象的生命週期 =====
// --- 嵌套作用域 LIFO ---
//   [+] 外層
//   [+] 內層
//   [-] 內層
//   [-] 外層
//
// --- for 迴圈：每次迭代 建構→解構 ---
//   [+] 迴圈#0
//   [=] 迴圈#0
//   [-] 迴圈#0
//   [+] 迴圈#1
//   [=] 迴圈#1
//   [-] 迴圈#1
//
// --- 延遲初始化（static local）---
//   [Singleton建構] 唯一實例
//   [Singleton使用] 唯一實例
//   [Singleton使用] 唯一實例
//
// --- 臨時對象生命週期 ---
//   [+] 臨時物件
//   [=] 臨時物件
//   [-] 臨時物件
//   (臨時物件已死亡)
//   [+] const引用續命
//   [=] const引用續命
//   (ref 綁定的臨時物件還活著)
//
//   陷阱：不要返回局部對象的引用(懸空引用)或地址(野指標)
//   解決：返回值(副本) 或 new + delete
//   全域初始化順序陷阱 → 用函數包裝 static 局部對象解決
//   [-] const引用續命
//
// ===== 第 19 課：new / delete =====
// --- 基本型別 ---
//   *p1 = 42, *p2 = 100
//
// --- 動態陣列 ---
//   arr: 10 20 30 40 50 
//
// --- 裸指標（手動管理）---
//   [鍛造] 鐵劍 (攻擊:25 耐久:50)
//   使用 鐵劍 (耐久:40)
//   使用 鐵劍 (耐久:30)
//   [銷毀] 鐵劍
//
// --- unique_ptr（自動管理）---
//   [鍛造] 長弓 (攻擊:20 耐久:40)
//   使用 長弓 (耐久:30)
//   [銷毀] 長弓
//
// --- 異常安全（unique_ptr）---
//   [鍛造] 法杖 (攻擊:35 耐久:30)
//   使用 法杖 (耐久:20)
//   [銷毀] 法杖
//   異常: 戰鬥中斷！ (法杖已自動清理)
//
//   記憶體洩漏四場景：忘記/覆蓋指標/提前return/異常
//   RAII 解決一切：建構獲取→解構釋放→自動管理
//   現代 C++：用 unique_ptr/make_unique 取代裸 new/delete
//
// ===== 日常實務：RAII 連線池（保證歸還）=====
//   連線池容量 2，目前可用 2
//
//   (1) 正常流程：
//     [借出] 查詢A 成功，剩餘 1
//     查詢完成
//     [歸還] 查詢A，剩餘 2
//
//   (2) 提早 return：
//     [借出] 查詢A 成功，剩餘 1
//     參數有誤，提早 return —— 但解構函式仍會執行
//     [歸還] 查詢A，剩餘 2
//
//   (3) 拋出例外（stack unwinding 仍會解構）：
//     [借出] 查詢B 成功，剩餘 1
//     即將拋出例外...
//     [歸還] 查詢B，剩餘 2
//     捕獲例外：資料庫逾時
//
//   (4) 巢狀作用域：解構順序是 LIFO（後建構的先解構）
//     [借出] 外層 成功，剩餘 1
//     [借出] 內層 成功，剩餘 0
//     內層作用域中，可用 0
//     [歸還] 內層，剩餘 1
//     離開內層後，可用 1
//     [歸還] 外層，剩餘 2
//
//   最終可用連線 2（與初始相同 → 三條離開路徑都沒有洩漏）
//   歷史最高同時使用 2 條
//   → 正常 return、提早 return、例外傳播，全部由解構函式保證歸還。
//
// ============================================================
// 七課速查表
// ============================================================
// 第13課 建構函數基礎：
//   - 函數名=類別名，無返回值，自動調用
//   - 可重載，帶驗證是第一道防線
//   - 陷阱：Most Vexing Parse, 定義建構後預設建構消失
// 第14課 預設建構函數：
//   - = default 保留平凡性，T obj{} 會歸零
//   - = delete 明確禁止預設建構
//   - 陣列 T arr[N] 需要預設建構函數
// 第15課 帶參建構函數：
//   - 類別型別參數推薦 const&
//   - 同名解決最推薦初始化列表 : name(name)
//   - explicit 禁止隱式轉換，{} 禁止窄化
// 第16課 初始化列表：
//   - 一步到位 vs 函數體賦值兩步
//   - 四必須：const/引用/無預設建構成員/基類
//   - 順序按宣告順序，書寫順序要一致
// 第17課 解構函數：
//   - ~ClassName()，無參數，不能重載
//   - LIFO 解構順序，RAII 自動管理資源
//   - 裸指標必須手動 delete，不要拋異常
// 第18課 對象生命週期：
//   - 四種存儲期：自動/靜態全域/靜態局部/動態
//   - static local 延遲初始化，C++11 線程安全
//   - 不要返回局部對象的引用或地址
// 第19課 new/delete：
//   - new=分配+建構，delete=解構+釋放
//   - new[]/delete[] 必須配對，不能混用
//   - unique_ptr/make_unique 取代裸 new/delete
// ============================================================
