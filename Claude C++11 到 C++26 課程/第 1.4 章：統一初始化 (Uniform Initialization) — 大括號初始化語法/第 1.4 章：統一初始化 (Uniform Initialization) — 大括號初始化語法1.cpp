// =============================================================================
//  第 1.4 章 範例 1  —  統一初始化 (Uniform Initialization)：{} 的完整用法
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  T obj{a, b, ...};      // 直接列表初始化 direct-list-initialization
//           T obj = {a, b, ...};   // 複製列表初始化 copy-list-initialization
//           T obj{};               // 值初始化 value-initialization
//   標準：  C++11 引入（本檔全部語法皆為 C++11 即可用）
//   標頭檔：語言核心語法，不需標頭檔；容器範例需 <vector>/<map>/<array> 等
//   複雜度：初始化本身無額外成本；容器的 {} 走 initializer_list 建構子，
//           成本為「複製 N 個元素」O(N)
//   本檔驗證標準：g++ -std=c++11 -pedantic-errors -Wall -Wextra（本機 g++ 15.2.0）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼要「統一」？——C++98 的初始化語法是碎片化的】
//   C++98 時代，「怎麼初始化」取決於「初始化什麼」，規則彼此不通：
//       int    x = 5;              // copy-initialization
//       int    y(5);               // direct-initialization
//       int    a[3] = {1, 2, 3};   // aggregate initialization（只有陣列能用）
//       Point  p(1, 2);            // 呼叫建構子
//       std::vector<int> v;        // 完全沒辦法在宣告時給初值！
//   最後那一條特別痛：C++98 要把 5 個值放進 vector，只能宣告完再 push_back 五次，
//   或先做一個 C 陣列再用 iterator 區間建構。C++11 的 {} 讓上面五種情境共用
//   同一套語法，這就是「統一初始化」名稱的由來。
//
// 【2. 三個實質好處（不只是語法糖）】
//   (a) 禁止窄化轉換（narrowing）——把「可能悄悄丟失資料」的轉換從「執行期
//       靜默截斷」提升為「編譯期錯誤」。詳見第 2 節。
//   (b) 消除 Most Vexing Parse——`Widget w{};` 不可能被讀成函式宣告。
//       詳見本章範例 2。
//   (c) 保證值初始化——`T obj{};` 對內建型別、聚合、有無建構子的類別
//       都保證歸零／呼叫預設建構子，不會留下不確定值。
//
// 【3. 窄化轉換的判定規則：不只看型別，也看「是不是編譯期常數」】
//   標準 [dcl.init.list]/7 列出的窄化情形，關鍵在於有沒有「常數例外」：
//       * 浮點 → 整數           ：**永遠**是窄化，沒有常數例外
//                                 （int n{3.0}; 即使 3.0 剛好是整數也被拒）
//       * 整數 → 較窄的整數     ：有常數例外。來源是編譯期常數且值放得下就放行
//                                 （char c{65} 合法；char c{2000} 被拒）
//       * 整數 → 浮點           ：有常數例外（值需能精確表示）
//       * long double→double→float：有常數例外
//   本機以 -pedantic-errors 實測全部符合上述規則（見【注意事項】4）。
//
// 【4. 為什麼 {} 有時會「選錯」建構子？——initializer_list 的絕對優先權】
//   只要類別有任何 initializer_list 建構子，`{}` 就會**強烈優先**選它，
//   強到即使另一個建構子是完全精準的匹配也照樣輸。這是 {} 唯一真正的坑：
//       std::vector<int> a{3, 5};   // 2 個元素 [3, 5]
//       std::vector<int> b(3, 5);   // 3 個元素 [5, 5, 5]
//   規則的邊界是「可行性」而非「匹配度」：只有當所有 initializer_list 建構子
//   都**完全不可行**（元素型別根本轉不過去）時，才會退回考慮一般建構子。
//   本機實測佐證這條邊界：
//       std::vector<std::string> s1{3, "hi"};  // size = 3！
//   因為 3 無法轉成 std::string，initializer_list<string> 不可行，於是退回
//   (count, value) 建構子 —— 和 vector<int>{3,5} 的結果剛好相反。
//
// 【概念補充 Concept Deep Dive】
//   * {} 背後其實是三條不同的路，編譯器依序判斷：
//       (1) 目標是聚合（aggregate）→ 走 aggregate initialization，
//           按成員宣告順序逐一初始化，不足的成員值初始化。
//       (2) 目標有 initializer_list 建構子 → 幾乎必定走它（見上）。
//       (3) 其餘 → 當成一般多載解析，把 {} 內的元素當引數。
//     所以「{} 到底做了什麼」要先問「這個型別是哪一類」，不能一概而論。
//   * 空的 {} 是特例：`T obj{}` **一律**解讀為值初始化／預設建構，
//     不會退化成「長度 0 的 initializer_list」。真要傳空 list 必須寫 `T obj({});`。
//     本機實測：類別同時有 T() 與 T(initializer_list<int>) 時，
//       T a{};    → 呼叫 T()          （預設建構）
//       T b({});  → 呼叫 T(il)，size=0
//       T c{{}};  → 呼叫 T(il)，size=1（內層 {} 變成一個值初始化的元素）
//   * 聚合初始化中，「沒寫到的成員」是值初始化（歸零），不是不確定值。
//     但整個物件若寫成 `Data d;`（連 {} 都沒有），對聚合而言是預設初始化，
//     內建型別成員的值是**不確定的**，讀取它是未定義行為。
//
// 【注意事項 Pay Attention】
//   1. 有 initializer_list 建構子的類別（所有標準容器都是），要呼叫
//      「指定大小／指定重複次數」語意的建構子，一律用 ()。
//   2. `T obj{}` 是值初始化、`T obj()` 是函式宣告、`T obj;` 是預設初始化，
//      三者不同，別混用。
//   3. 讀取預設初始化後未賦值的內建型別成員是未定義行為 —— 結果不保證，
//      可能是任何值、也可能被最佳化成別的樣子，不要依賴任何觀察到的數字。
//   4. 版本差異（皆以 g++ 15.2.0 加 -pedantic-errors 實測）：
//        * C++11：{} 與 narrowing 檢查、initializer_list 全部到位。
//        * C++14：聚合開始允許含 NSDMI（成員預設值）。
//                 `struct A { int x; int y = 5; }; A a{1};`
//                 在 -std=c++11 被拒、-std=c++14 起通過。
//        * C++17：聚合可以有基底類別（`struct D : B { int d; }; D d{{1},2};`
//                 在 c++11/14 被拒、c++17 起通過）；同時引入 CTAD。
//        * C++20：新增指定初始化 `Cfg c{.port=8080}`（本檔不使用，見 summary.cpp）；
//                 允許用小括號初始化聚合（P0960）；
//                 有 user-declared 建構子（含 `= default`）者不再是聚合（P1008）。
//      ※ 指定初始化是 C++20 才進標準，但 g++ 在舊模式下會當**擴充**放行；
//        必須加 -pedantic-errors 才會看到真正的標準行為（本機實測：
//        -std=c++11/14/17 加上 -pedantic-errors 皆明確報錯並指出需要 c++20）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】統一初始化 / 大括號初始化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. `std::vector<int> v{10};` 和 `std::vector<int> v(10);` 分別是什麼？
//     答：`v{10}` 走 initializer_list 建構子 → **1 個元素、值為 10**；
//         `v(10)` 走 (count) 建構子 → **10 個元素、值皆為 0**。
//         本機實測：前者 size=1 front=10，後者 size=10 front=0。
//     追問：那 `vector<int> v{3, 5}` 與 `v(3, 5)` 呢？（{} → 2 個元素 [3,5]；
//         () → 3 個元素 [5,5,5]。這是本主題最常被考的一題）
//
// 🔥 Q2. {} 憑什麼能防止窄化？只看型別還是也看值？
//     答：兩者都看。整數→較窄整數時，若來源是**編譯期常數**且值放得下就放行
//         （`char c{65}` 合法、`char c{2000}` 被拒、變數 `char c{n}` 也被拒）。
//         但**浮點→整數永遠是窄化，沒有常數例外**：`int n{3.0}` 照樣編譯失敗。
//     追問：為什麼 double→int 不給常數例外？（因為那是「表示方式」的改變而非
//         單純範圍問題，標準選擇一律禁止，逼你寫出 static_cast 表明意圖）
//
// ⚠️ 陷阱 1. 「幫類別加一個 initializer_list 建構子，是純粹的新增功能，
//             不會影響既有程式碼。」
//     答：會，而且可能是**編譯期錯誤**或**靜默改變語意**。只要有 initializer_list
//         建構子，所有用 {} 的呼叫都會優先撞上它。經典案例：
//             struct W { W(int, double); W(std::initializer_list<bool>); };
//             W w{10, 5.0};   // 不會選 W(int,double)！
//         本機實測結果是**編譯錯誤**：它選了 il<bool>，然後 10→bool 屬於窄化。
//         注意它並不會因為「這條走不通」就回頭選 W(int,double)。
//     為什麼會錯：一般人用「哪個比較匹配就選哪個」的多載解析直覺去想。
//         但 initializer_list 的優先權不是「匹配度更高」，而是「先被挑中」；
//         只有在它**完全不可行**時才輪到別人。實測邊界：
//         `vector<string> s{3, "hi"}` 因 3 轉不成 string 而退回 (count,value)，
//         得到 size=3 —— 和 `vector<int>{3,5}` 得到 size=2 恰恰相反。
//
// ⚠️ 陷阱 2. 「`Data d;` 和 `Data d{};` 只是寫法不同，結果一樣。」
//     答：不一樣。對聚合／內建型別，`Data d;` 是預設初始化，成員值**不確定**，
//         讀取它是未定義行為（結果不保證，不要依賴任何觀察值）；
//         `Data d{};` 是值初始化，保證所有成員歸零。
//     為什麼會錯：debug 版常常「剛好」是 0（新配置的堆疊頁多半是零），
//         讓人以為有保證；換成 release 或換個呼叫路徑就變成垃圾值。
//         「測起來是 0」從來不是「標準保證是 0」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>
#include <complex>

// ===== 用於展示的類別 =====

class Point
{
public:
    int x;
    int y;
    
    // 預設建構子
    Point() : x{0}, y{0}
    {
        std::cout << "  Point() 預設建構\n";
    }
    
    // 雙參數建構子
    Point(int x_, int y_) : x{x_}, y{y_}
    {
        std::cout << "  Point(" << x << ", " << y << ") 建構\n";
    }
    
    void print() const
    {
        std::cout << "  Point(" << x << ", " << y << ")\n";
    }
};

// 有 initializer_list 建構子的類別
class NumberList
{
    std::vector<int> data_;
    
public:
    // 一般建構子：指定大小和初始值
    NumberList(std::size_t size, int value)
        : data_(size, value)
    {
        std::cout << "  NumberList(size=" << size << ", value=" << value << ")\n";
    }
    
    // initializer_list 建構子
    NumberList(std::initializer_list<int> init)
        : data_(init)
    {
        std::cout << "  NumberList(initializer_list, size=" << init.size() << ")\n";
    }
    
    void print() const
    {
        std::cout << "  內容: ";
        for (int n : data_)
        {
            std::cout << n << " ";
        }
        std::cout << "\n";
    }
};

// 聚合類別（Aggregate）：無使用者定義建構子、無私有成員等
struct Aggregate
{
    int a;
    double b;
    std::string c;
};

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本型別的初始化 =====
    std::cout << "===== 1. 基本型別的初始化 =====\n";
    
    int i1{42};          // 直接列表初始化
    int i2 = {42};       // 複製列表初始化
    int i3{};            // 值初始化，i3 = 0
    double d{3.14};
    char c{'A'};
    bool b{true};
    
    std::cout << "i1 = " << i1 << "\n";
    std::cout << "i2 = " << i2 << "\n";
    std::cout << "i3 = " << i3 << " (值初始化為 0)\n";
    std::cout << "d = " << d << "\n";
    std::cout << "c = " << c << "\n";
    std::cout << "b = " << b << "\n\n";
    
    // ===== 2. 防止窄化轉換 (Narrowing Conversion) =====
    std::cout << "===== 2. 防止窄化轉換 =====\n";
    
    int largeValue = 1000;
    
    // 使用 () 或 = 不會檢查窄化
    char c1 = largeValue;     // 允許，但可能資料遺失
    char c2(largeValue);      // 允許，但可能資料遺失
    
    // 使用 {} 會在編譯期檢查窄化
    // char c3{largeValue};   // 編譯錯誤！窄化轉換
    // char c4 = {largeValue}; // 編譯錯誤！窄化轉換
    
    // 常量表達式在範圍內則允許
    char c5{65};              // 合法，65 在 char 範圍內
    
    // double 到 int 也是窄化
    double pi = 3.14159;
    // int truncated{pi};     // 編譯錯誤！窄化轉換
    int truncated(pi);        // 允許，但會截斷
    
    std::cout << "c1 = " << static_cast<int>(c1) << " (可能不正確)\n";
    std::cout << "c2 = " << static_cast<int>(c2) << " (() 同樣不擋窄化)\n";
    std::cout << "c5 = " << c5 << " (65 = 'A')\n";
    std::cout << "truncated = " << truncated << " (從 3.14159 截斷)\n";
    std::cout << "大括號初始化會在編譯期防止窄化轉換！\n\n";
    
    // ===== 3. 陣列初始化 =====
    std::cout << "===== 3. 陣列初始化 =====\n";
    
    // C 風格陣列
    int arr1[5]{1, 2, 3, 4, 5};
    int arr2[5]{1, 2};         // 其餘元素為 0
    int arr3[5]{};             // 全部為 0
    
    std::cout << "arr1: ";
    for (int n : arr1) std::cout << n << " ";
    std::cout << "\n";
    
    std::cout << "arr2: ";
    for (int n : arr2) std::cout << n << " ";
    std::cout << "(其餘為 0)\n";
    
    std::cout << "arr3: ";
    for (int n : arr3) std::cout << n << " ";
    std::cout << "(全部為 0)\n";
    
    // std::array
    std::array<int, 4> stdArr{10, 20, 30, 40};
    std::cout << "std::array: ";
    for (int n : stdArr) std::cout << n << " ";
    std::cout << "\n\n";
    
    // ===== 4. STL 容器初始化 =====
    std::cout << "===== 4. STL 容器初始化 =====\n";
    
    // vector
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << "\n";
    
    // map
    std::map<std::string, int> scores{
        {"Alice", 95},
        {"Bob", 87},
        {"Charlie", 92}
    };
    std::cout << "map:\n";
    for (const auto& pair : scores)
    {
        std::cout << "  " << pair.first << ": " << pair.second << "\n";
    }
    
    // pair
    std::pair<int, std::string> p{42, "Answer"};
    std::cout << "pair: (" << p.first << ", " << p.second << ")\n";
    
    // 巢狀容器
    std::vector<std::vector<int>> matrix{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    std::cout << "matrix:\n";
    for (const auto& row : matrix)
    {
        std::cout << "  ";
        for (int n : row) std::cout << n << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
    
    // ===== 5. 類別初始化 =====
    std::cout << "===== 5. 類別初始化 =====\n";
    
    std::cout << "Point p1{}:\n";
    Point p1{};        // 呼叫預設建構子
    p1.print();
    
    std::cout << "Point p2{10, 20}:\n";
    Point p2{10, 20};  // 呼叫雙參數建構子
    p2.print();
    
    std::cout << "\n";
    
    // ===== 6. 聚合初始化 =====
    std::cout << "===== 6. 聚合初始化 =====\n";
    
    Aggregate agg1{42, 3.14, "Hello"};
    Aggregate agg2{100};                    // b = 0.0, c = ""
    Aggregate agg3{};                       // 全部預設初始化
    
    std::cout << "agg1: a=" << agg1.a << ", b=" << agg1.b 
              << ", c=\"" << agg1.c << "\"\n";
    std::cout << "agg2: a=" << agg2.a << ", b=" << agg2.b 
              << ", c=\"" << agg2.c << "\"\n";
    std::cout << "agg3: a=" << agg3.a << ", b=" << agg3.b 
              << ", c=\"" << agg3.c << "\"\n\n";
    
    // ===== 7. initializer_list 建構子的優先順序 =====
    std::cout << "===== 7. initializer_list 建構子的優先順序 =====\n";
    
    std::cout << "NumberList n1{5, 10}:\n";
    NumberList n1{5, 10};    // 呼叫 initializer_list 版本！
    n1.print();
    
    std::cout << "NumberList n2(5, 10):\n";
    NumberList n2(5, 10);    // 呼叫一般建構子 (size, value)
    n2.print();
    
    std::cout << "\n注意：大括號優先匹配 initializer_list 建構子！\n\n";
    
    // ===== 8. 動態記憶體配置 =====
    std::cout << "===== 8. 動態記憶體配置 =====\n";
    
    // new 搭配大括號
    int* pInt = new int{42};
    std::cout << "*pInt = " << *pInt << "\n";
    delete pInt;
    
    // 陣列
    int* pArr = new int[5]{1, 2, 3, 4, 5};
    std::cout << "pArr: ";
    for (int i = 0; i < 5; ++i) std::cout << pArr[i] << " ";
    std::cout << "\n";
    delete[] pArr;
    
    // 智慧指標
    auto sp = std::make_shared<Point>(30, 40);
    sp->print();
    std::cout << "\n";
    
    // ===== 9. 函式參數與回傳值 =====
    std::cout << "===== 9. 函式參數與回傳值 =====\n";
    
    // Lambda 接收 vector
    auto printVec = [](const std::vector<int>& v) {
        std::cout << "  ";
        for (int n : v) std::cout << n << " ";
        std::cout << "\n";
    };
    
    // 直接傳遞大括號初始化列表
    printVec({10, 20, 30});
    
    // Lambda 回傳大括號初始化
    auto createPoint = []() -> Point {
        return {100, 200};  // 直接回傳，無需寫 Point
    };
    
    std::cout << "createPoint() 回傳:\n";
    Point p3 = createPoint();
    p3.print();
    std::cout << "\n";
    
    // ===== 10. 成員初始化列表 =====
    std::cout << "===== 10. 成員初始化列表中的大括號 =====\n";
    
    struct Container
    {
        std::vector<int> data;
        Point point;
        int value;
        
        Container()
            : data{1, 2, 3}      // 直接初始化 vector
            , point{5, 6}        // 直接初始化 Point
            , value{42}          // 直接初始化 int
        {
            std::cout << "Container 建構完成\n";
        }
    };
    
    Container cont;
    std::cout << "data: ";
    for (int n : cont.data) std::cout << n << " ";
    std::cout << "\n";
    cont.point.print();
    std::cout << "value: " << cont.value << "\n\n";
    
    // ===== 11. 零初始化 vs 預設初始化 =====
    std::cout << "===== 11. 零初始化 vs 預設初始化 =====\n";
    
    struct Data
    {
        int x;
        double y;
    };
    
    Data d1;      // 預設初始化：成員值不確定！
    Data d2{};    // 值初始化：x = 0, y = 0.0
    Data d3 = {}; // 同上
    
    // d1.x 和 d1.y 是未初始化的，讀取是未定義行為
    std::cout << "Data d2{}: x = " << d2.x << ", y = " << d2.y << "\n";
    std::cout << "使用 {} 確保成員被零初始化\n";
    
    return 0;
}

// 編譯: g++ -std=c++11 -pedantic-errors -Wall -Wextra "第 1.4 章：統一初始化 (Uniform Initialization) — 大括號初始化語法1.cpp" -o uniform_init1
//   本檔全部語法自 C++11 起即可用，已用 -std=c++11 -pedantic-errors 實測零 error 通過。

// 註 1:編譯時會出現 4 個警告，全部是【刻意】的教材內容，非程式缺陷：
//        • 2 個 -Wmissing-field-initializers（Aggregate::b、Aggregate::c）
//          → 第 6 節就是要示範「聚合只給部分初始值，其餘被值初始化」。
//        • 2 個 -Wunused-variable（d1、d3）
//          → 第 11 節的 d1 是預設初始化的物件，成員值不確定。
//            它【刻意宣告但從不讀取】—— 讀取未初始化的值才是 UB，
//            本檔只印出 d2（{} 值初始化）的 0/0 作為對照，
//            所以這支程式執行時【不會】觸發任何 UB。

// 註 2:本檔輸出是【完全確定的】，連跑 5 次位元組完全相同。

// 註 3:第 2 節的 c1 = -24 / c2 = -24 是【實作定義】的結果，不是 UB，
//      也不可視為跨平台保證：
//        char c1 = 1000; 把超出 char 範圍的值轉進 char，
//        本機（x86-64 Linux，char 為 signed、8 bit）得到 1000 mod 256 = 232，
//        以 signed 解讀即 232 − 256 = −24。
//      在 char 預設為 unsigned 的平台（如部分 ARM）會印出 232。
//      C++20 起整數轉換已明定為二補數取模，C++11 時期則屬實作定義。
//      這一節真正的重點是「= 和 () 都不擋窄化，只有 {} 會在編譯期擋下來」
//      —— 第 248 行被註解掉的 int truncated{pi}; 才是會編譯失敗的那一行。

// === 預期輸出 ===
// ===== 1. 基本型別的初始化 =====
// i1 = 42
// i2 = 42
// i3 = 0 (值初始化為 0)
// d = 3.14
// c = A
// b = true
//
// ===== 2. 防止窄化轉換 =====
// c1 = -24 (可能不正確)
// c2 = -24 (() 同樣不擋窄化)
// c5 = A (65 = 'A')
// truncated = 3 (從 3.14159 截斷)
// 大括號初始化會在編譯期防止窄化轉換！
//
// ===== 3. 陣列初始化 =====
// arr1: 1 2 3 4 5
// arr2: 1 2 0 0 0 (其餘為 0)
// arr3: 0 0 0 0 0 (全部為 0)
// std::array: 10 20 30 40
//
// ===== 4. STL 容器初始化 =====
// vector: 1 2 3 4 5
// map:
//   Alice: 95
//   Bob: 87
//   Charlie: 92
// pair: (42, Answer)
// matrix:
//   1 2 3
//   4 5 6
//   7 8 9
//
// ===== 5. 類別初始化 =====
// Point p1{}:
//   Point() 預設建構
//   Point(0, 0)
// Point p2{10, 20}:
//   Point(10, 20) 建構
//   Point(10, 20)
//
// ===== 6. 聚合初始化 =====
// agg1: a=42, b=3.14, c="Hello"
// agg2: a=100, b=0, c=""
// agg3: a=0, b=0, c=""
//
// ===== 7. initializer_list 建構子的優先順序 =====
// NumberList n1{5, 10}:
//   NumberList(initializer_list, size=2)
//   內容: 5 10
// NumberList n2(5, 10):
//   NumberList(size=5, value=10)
//   內容: 10 10 10 10 10
//
// 注意：大括號優先匹配 initializer_list 建構子！
//
// ===== 8. 動態記憶體配置 =====
// *pInt = 42
// pArr: 1 2 3 4 5
//   Point(30, 40) 建構
//   Point(30, 40)
//
// ===== 9. 函式參數與回傳值 =====
//   10 20 30
// createPoint() 回傳:
//   Point(100, 200) 建構
//   Point(100, 200)
//
// ===== 10. 成員初始化列表中的大括號 =====
//   Point(5, 6) 建構
// Container 建構完成
// data: 1 2 3
//   Point(5, 6)
// value: 42
//
// ===== 11. 零初始化 vs 預設初始化 =====
// Data d2{}: x = 0, y = 0
// 使用 {} 確保成員被零初始化
