// ============================================================================
// 第 1.6 章：列表初始化的陷阱 — Narrowing Conversion 與優先順序問題（總複習）
// ============================================================================
//
// 【主題資訊 Information】
//   主題    ：{} 列表初始化的十大陷阱（窄化轉換、重載優先權、生命週期）
//   標準版本：列表初始化 / initializer_list      C++11
//             auto x{42} 推導為 int              C++17（N3922）
//             CTAD（vector v{1,2,3}）             C++17
//             保證的複製省略（guaranteed copy elision）C++17
//   標頭檔  ：<initializer_list>、<vector>、<string>
//   複雜度  ：語法特性；重載決議發生於編譯期，執行期無額外成本
//   本檔宣告的標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「更安全的 {}」反而有一整章陷阱】
//   {} 被設計來解決舊語法的問題（防窄化、避開 Most Vexing Parse），
//   但它同時引入了一條新的重載決議路徑。
//   結果是：{} 在「安全性」上是進步的，在「可預測性」上卻多了一層規則。
//   本章的十大陷阱幾乎都源自同一個根：
//       「{} 不是 () 的同義詞，它走不同的決議規則。」
//
// 【2. 窄化轉換：{} 唯一「多做事」的地方】
//   標準明確列舉哪些轉換屬於窄化（narrowing）：
//     * 浮點 → 整數（一律算窄化，即使值是 3.0）
//     * 整數 → 浮點（若無法精確表示）
//     * 較寬整數 → 較窄整數（若值不可表示）
//   關鍵豁免：若來源是「編譯期常數」且值可被目標型別精確表示，就不算窄化。
//       int  a{3.0};   // ❌ 錯誤 — double → int 一律窄化
//       char b{65};    // ✅ 可以 — 常數 65 在 char 範圍內
//       char c{300};   // ❌ 錯誤 — 300 超出 char 範圍
//   這條豁免解釋了為什麼「有時 {} 擋得住、有時擋不住」：
//   關鍵在於編譯器能不能在編譯期知道那個值。
//
// 【3. initializer_list 的絕對優先權（最容易出事的一條）】
//   重載決議對 {} 有特例：只要有任何 initializer_list 建構子可行，
//   它就勝過所有其他建構子 —— 即使其他建構子是更精確的匹配。
//       std::vector<int> v1(5, 10);   // 5 個 10
//       std::vector<int> v2{5, 10};   // 2 個元素 [5, 10]
//   更隱蔽的版本是「隱式轉換也算可行」：
//       std::vector<double> v{1, 2};  // int 可轉 double → 仍走 initializer_list
//
// 【4. 生命週期：initializer_list 不擁有資料】
//   {1,2,3} 背後是編譯器產生的「後備陣列」，而 initializer_list 只是它的 view。
//   陣列壽命 = list 物件壽命，因此回傳它或存成成員都會懸空。
//   本檔保留了一個 dangerousReturn() 作為教材，但刻意「只定義、不呼叫」——
//   因為呼叫它就會執行未定義行為，而 UB 沒有可展示的固定結果。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼窄化檢查只在 {} 生效
//     = 與 () 沿用 C 的隱式轉換規則，那是為了向後相容而不能改的歷史包袱。
//     C++11 需要一個「新的、可以嚴格一點」的語法入口，{} 正好是全新的，
//     沒有既有程式碼依賴它的行為，因此可以安全地加上嚴格檢查。
//     這是 C++ 演進的典型手法：不改舊語法，而是給新語法更好的預設。
//
// (B) 「編譯期常數」豁免的實務意義
//     char c{65}; 能過，但下面這段不能：
//         int n = readFromConfig();   // 執行期才知道
//         char c{n};                  // ❌ 錯誤：編譯器無法保證 n 在範圍內
//     這代表 {} 的保護只涵蓋「編譯期已知」的值。
//     從設定檔、網路、使用者輸入來的資料，仍然必須自己做範圍檢查 ——
//     {} 不是執行期的安全網。
//
// (C) auto + {} 的版本差異（本檔為 C++17）
//       auto a = {1, 2, 3};   // initializer_list<int>（所有版本）
//       auto b{42};           // C++11/14：initializer_list<int>
//                             // C++17 起：int   ← 本檔採用此規則
//       auto c = {42};        // initializer_list<int>（所有版本）
//     C++17 的 N3922 規則：直接列表初始化（無 =）且單一元素 → 推導為元素型別；
//     複製列表初始化（有 =）→ 仍為 initializer_list。
//
// (D) 聚合部分初始化的保證
//     Aggregate a{1}; 未列出的成員會被「值初始化」（歸零），不是垃圾值。
//     這是標準的明文保證，可以放心依賴。
//     ※ 本檔此處會產生 -Wmissing-field-initializers 警告，
//       那是 GCC 在提醒「你有成員沒寫」，而我們正是要示範這條規則，
//       故刻意保留，屬教材的一部分而非缺陷。
//
// 【注意事項 Pay Attention】
//   1. {} 的窄化檢查只對「編譯期已知」的值有效；執行期資料仍須自行驗證。
//   2. vector<int> v(5,10) 與 v{5,10} 語意完全不同，改寫時務必留意。
//   3. initializer_list 建構子的優先權是絕對的，隱式轉換也算「可行」。
//   4. 絕不要回傳或儲存 initializer_list —— 會懸空，後續使用屬未定義行為，
//      其表現不固定（可能看似正常、可能讀到垃圾、也可能崩潰），不可依賴。
//   5. auto x{42} 在本檔（C++17）是 int；C++11/14 則是 initializer_list<int>。
//   6. 本檔刻意保留 3 個 -Wmissing-field-initializers 與 1 個
//      -Winit-list-lifetime 警告作為教材；dangerousReturn() 只定義不呼叫，
//      因此程式執行過程中不會真的觸發未定義行為。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】列表初始化的陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 int x{3.0}; 編譯不過，但 char c{65}; 可以？
//     答：浮點 → 整數一律被標準列為窄化轉換，即使 3.0 沒有小數部分也一樣。
//         而 char c{65}; 有豁免：來源是編譯期常數且 65 在 char 範圍內，
//         編譯器能證明不會失真，所以不算窄化。
//     追問：char c{300}; 呢？→ 編譯錯誤，300 超出 char 表示範圍，豁免不成立。
//         再追問：int n = 讀設定(); char c{n}; → 也是錯誤，因為 n 執行期才知道，
//         編譯器無法證明它安全。
//
// 🔥 Q2. std::vector<double> v{1, 2}; 會呼叫哪個建構子？
//     答：initializer_list<double> 版本。雖然 1 和 2 是 int，
//         但 int → double 是可行的隱式轉換，所以 initializer_list 建構子「可行」，
//         而只要它可行就擁有絕對優先權。結果是 2 個元素 [1.0, 2.0]。
//     追問：那要怎麼建立「2 個元素、每個都是 1.0」的 vector？→ 只能用 ()：
//         std::vector<double> v(2, 1.0);
//
// 🔥 Q3. 為什麼不能從函式回傳 std::initializer_list？
//     答：它只是編譯器生成之後備陣列的 view，不擁有資料。
//         函式返回時後備陣列即銷毀，回傳的 view 隨即懸空，
//         使用它屬未定義行為（GCC 會給 -Winit-list-lifetime 警告）。
//     追問：正確作法？→ 回傳擁有資料的容器，例如 std::vector<int>，
//         寫 return {1,2,3}; 讓 vector 複製一份自己的資料。
//
// ⚠️ 陷阱 1. 「{} 比較安全，所以應該全面改用 {}」對嗎？
//     答：不完全對。{} 在防窄化與避開 Most Vexing Parse 上確實更安全，
//         但它會改變重載決議 —— 對有 initializer_list 建構子的型別
//         （vector/map 等），把 () 換成 {} 是語意改變，不是風格調整。
//     為什麼會錯：把 {} 當成「比較新的括號」。
//         正確心智模型是：{} 是一條獨立的初始化路徑，不是 () 的同義詞。
//
// ⚠️ 陷阱 2. Aggregate a{1}; 其餘成員是垃圾值嗎？
//     答：不是。標準保證未列出的成員會被「值初始化」——
//         算術型別歸零、指標為 nullptr、類別型別呼叫其預設建構子。
//     為什麼會錯：把它類比成 C 的區域變數「未初始化 = 垃圾值」。
//         但聚合初始化只要出現了 {}，就啟動了值初始化的保證，
//         這是可以安心依賴的行為。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++17 -Wall -Wextra -o summary summary.cpp
//   註：本檔會出現 3 個 -Wmissing-field-initializers 與 1 個 -Winit-list-lifetime
//       警告，皆為刻意保留的教材內容（示範聚合部分初始化與懸空 view），
//       非程式缺陷；dangerousReturn() 只定義不呼叫，執行時不會觸發 UB。
//
// 本檔案是第 1.6 章的完整總複習，涵蓋列表初始化（brace initialization）的
// 十大常見陷阱，包含完整可編譯的程式碼範例與詳盡的中文註解。
// 閱讀本檔案即可掌握本章所有核心概念。
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <map>
#include <cstdint>
#include <algorithm>

// ============================================================================
// 【陷阱 1】initializer_list 建構子的優先順序
// ============================================================================
//
// 核心觀念：
//   當類別同時擁有「一般建構子」和「std::initializer_list 建構子」時，
//   使用 {} 初始化會「優先」匹配 initializer_list 建構子。
//   這是 C++11 列表初始化中最常被忽略的規則。
//
// 規則整理：
//   - 使用 () 初始化 → 遵循傳統的多載解析規則
//   - 使用 {} 初始化 → 編譯器會先嘗試匹配 initializer_list 建構子
//     只有在完全無法匹配時，才會退回到一般建構子
//
// 實務影響：
//   Container c(5);    → 呼叫 Container(size_t) — 建立大小為 5 的容器
//   Container c{5};    → 呼叫 Container(initializer_list<int>) — 建立含有元素 5 的容器
//   兩者語意完全不同！
// ============================================================================

class Container
{
public:
    // 建構子 1：指定容器大小
    explicit Container(std::size_t size)
    {
        std::cout << "  Container(size_t size=" << size << ")\n";
    }

    // 建構子 2：指定大小與初始值
    Container(std::size_t size, int value)
    {
        std::cout << "  Container(size_t size=" << size
                  << ", int value=" << value << ")\n";
    }

    // 建構子 3：接受 initializer_list
    // 一旦存在此建構子，{} 初始化就會優先匹配它
    Container(std::initializer_list<int> init)
    {
        std::cout << "  Container(initializer_list<int>, size="
                  << init.size() << ", values={";
        bool first = true;
        for (int v : init)
        {
            if (!first) std::cout << ", ";
            std::cout << v;
            first = false;
        }
        std::cout << "})\n";
    }
};

void demo_trap1_initializer_list_priority()
{
    std::cout << "===== 【陷阱 1】initializer_list 建構子的優先順序 =====\n\n";

    // 使用 () → 傳統多載解析，匹配 Container(size_t)
    std::cout << "Container c1(5);        → ";
    Container c1(5);

    // 使用 {} → 優先匹配 initializer_list<int>，將 5 當作列表的一個元素
    std::cout << "Container c2{5};        → ";
    Container c2{5};

    // 使用 () → 匹配 Container(size_t, int)
    std::cout << "Container c3(5, 10);    → ";
    Container c3(5, 10);

    // 使用 {} → 優先匹配 initializer_list<int>，列表包含 5 和 10
    std::cout << "Container c4{5, 10};    → ";
    Container c4{5, 10};

    // 明確傳入 initializer_list 物件
    std::cout << "Container c5({5, 10});  → ";
    Container c5({5, 10});

    std::cout << "\n";
}

// ============================================================================
// 【陷阱 2】窄化轉換（Narrowing Conversion）
// ============================================================================
//
// 核心觀念：
//   C++11 的列表初始化（{} 初始化）禁止「窄化轉換」。
//   窄化轉換是指可能導致資料損失的隱式型別轉換。
//   傳統的 () 和 = 初始化則不會禁止窄化轉換。
//
// 窄化轉換的四大類型：
//   (a) 浮點數 → 整數          例: double → int
//   (b) 大整數 → 小整數        例: long long → int
//   (c) 有號 ↔ 無號 整數       例: int → unsigned（負數會出問題）
//   (d) 指標 → bool            例: int* → bool
//
// 常量表達式的例外：
//   如果 {} 內是編譯期常量，且值可以精確表示為目標型別，則允許。
//   例: int x{3};     → 合法（3 可以精確表示為 int）
//       int x{3.14};  → 錯誤（3.14 有小數部分，無法精確表示為 int）
//       int x{100LL}; → 合法（100 在 int 範圍內）
//
// 設計目的：
//   防止程式設計師在不知不覺中進行可能丟失資料的轉換。
//   這是 {} 初始化相較於 () 初始化的一大安全優勢。
// ============================================================================

void demo_trap2_narrowing_conversion()
{
    std::cout << "===== 【陷阱 2】窄化轉換 =====\n";

    // --- 2.1 浮點數 → 整數 ---
    std::cout << "\n--- 2.1 浮點數 → 整數 ---\n";

    double d = 3.14;

    int a(d);       // 合法：使用 () 不會禁止窄化，a = 3（截斷小數）
    [[maybe_unused]] int b = d;      // 合法：使用 = 不會禁止窄化，b = 3
    // int c{d};    // 編譯錯誤！d 是 double 變數，{} 禁止窄化轉換
    // int e = {d}; // 編譯錯誤！同上

    // 常量表達式例外：值在範圍內且無小數部分時允許
    int f{3};       // 合法：3 是整數常量，可精確表示為 int
    // int g{3.14}; // 編譯錯誤！3.14 有小數部分

    std::cout << "int a(3.14) = " << a << "  ← () 允許，但截斷小數\n";
    std::cout << "int f{3}    = " << f << "  ← {} 允許，整數常量無損失\n";
    std::cout << "int c{d};        ← 編譯錯誤！{} 禁止 double→int 窄化\n";

    // --- 2.2 大整數 → 小整數 ---
    std::cout << "\n--- 2.2 大整數 → 小整數 ---\n";

    long long big = 1000;

    int i1(big);     // 合法：使用 () 不會報錯
    [[maybe_unused]] int i2 = big;    // 合法：使用 = 不會報錯
    // int i3{big};  // 編譯錯誤！即使 big 的值 1000 放得進 int，
                     // 但 big 是變數（非常量表達式），編譯器無法保證安全

    int i4{100LL};   // 合法：100LL 是常量表達式，100 在 int 範圍內
    // int i5{10'000'000'000LL}; // 編譯錯誤！超出 int 範圍

    std::cout << "int i1(big)  = " << i1 << "  ← () 允許\n";
    std::cout << "int i4{100LL} = " << i4 << " ← {} 允許，常量在 int 範圍內\n";
    std::cout << "int i3{big};       ← 編譯錯誤！big 是變數，即使值放得下\n";

    // --- 2.3 有號 ↔ 無號 轉換 ---
    std::cout << "\n--- 2.3 有號 ↔ 無號 轉換 ---\n";

    int negative = -1;

    unsigned u1(negative);    // 合法但危險：-1 變成一個非常大的無號數
    [[maybe_unused]] unsigned u2 = negative;   // 合法但危險：同上
    // unsigned u3{negative}; // 編譯錯誤！{} 禁止有號→無號的窄化

    [[maybe_unused]] unsigned u4{0};           // 合法：0 是常量且在無號範圍內
    // unsigned u5{-1};       // 編譯錯誤！-1 是負數常量，不在 unsigned 範圍內

    std::cout << "unsigned u1(-1)   = " << u1 << "  ← () 允許，但值是垃圾\n";
    std::cout << "unsigned u3{negative}; ← 編譯錯誤！{} 禁止有號→無號窄化\n";

    // --- 2.4 指標 → bool ---
    std::cout << "\n--- 2.4 指標 → bool ---\n";

    int* ptr = nullptr;

    bool b1(ptr);    // 合法：指標隱式轉換為 bool
    [[maybe_unused]] bool b2 = ptr;   // 合法：同上
    // bool b3{ptr}; // 編譯錯誤！C++11 禁止指標→bool 的窄化轉換

    std::cout << "bool b1(nullptr) = " << std::boolalpha << b1 << "  ← () 允許\n";
    std::cout << "bool b3{ptr};          ← 編譯錯誤！{} 禁止指標→bool\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 3】auto 與大括號的推導規則
// ============================================================================
//
// 核心觀念：
//   auto 搭配 {} 的型別推導在 C++11/14 和 C++17 之間有重大變化。
//
// 規則對照表：
//   語法               C++11/14                    C++17
//   -----------------------------------------------------------------
//   auto x = {1,2,3}   initializer_list<int>       initializer_list<int>
//   auto x = {42}      initializer_list<int>       initializer_list<int>
//   auto x{42}         initializer_list<int>       int（重大改變！）
//   auto x{1,2,3}      initializer_list<int>       編譯錯誤
//
// 重點變化（C++17）：
//   auto x{42}; 不再推導為 initializer_list<int>，而是推導為 int。
//   auto x{1,2,3}; 變成編譯錯誤（直接列表初始化只允許一個元素）。
//
// 實務建議：
//   - 如果要明確建立 initializer_list，使用 auto x = {1, 2, 3}; 語法
//   - 如果只要單一值，使用 auto x{42}; 在 C++17 會推導為 int
// ============================================================================

void demo_trap3_auto_brace()
{
    std::cout << "===== 【陷阱 3】auto 與大括號的推導規則 =====\n";

    // --- auto = {...} 語法（C++11/14/17 行為一致） ---
    std::cout << "\n--- auto = {...} ---\n";

    auto a = {1, 2, 3};  // 型別: std::initializer_list<int>，所有版本皆同
    auto b = {42};        // 型別: std::initializer_list<int>（即使只有一個元素！）

    std::cout << "auto a = {1, 2, 3}; → initializer_list<int>, size=" << a.size() << "\n";
    std::cout << "auto b = {42};      → initializer_list<int>, size=" << b.size() << "\n";

    // --- auto x{value} 語法（C++17 行為改變） ---
    std::cout << "\n--- auto x{value} (C++17 改變) ---\n";

    auto c{42};           // C++17: 推導為 int（不是 initializer_list）
    // auto d{1, 2};      // C++17: 編譯錯誤！直接列表初始化不允許多元素

    std::cout << "auto c{42}; → type is "
              << (std::is_same<decltype(c), int>::value ? "int" : "other")
              << ", value=" << c << "\n";

    // --- 完整對比表 ---
    std::cout << "\n--- 對比表 ---\n";
    std::cout << "語法              C++11/14               C++17\n";
    std::cout << "auto x = {1,2,3}  initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x = {42}     initializer_list<int>  initializer_list<int>\n";
    std::cout << "auto x{42}        initializer_list<int>  int\n";
    std::cout << "auto x{1,2,3}     initializer_list<int>  編譯錯誤\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 4】空大括號 {} 的歧義
// ============================================================================
//
// 核心觀念：
//   當類別同時有「預設建構子」和「initializer_list 建構子」時，
//   空大括號 {} 的行為是：呼叫「預設建構子」，而不是 initializer_list。
//
// 規則：
//   Widget w{};    → 呼叫預設建構子（特殊規則：空 {} 優先匹配預設建構子）
//   Widget w({});  → 呼叫 initializer_list 建構子，傳入空列表
//   Widget w{{}}; → 呼叫 initializer_list 建構子，列表含一個預設初始化的元素
//
// 陷阱點：
//   std::vector<int> v{{}};  → 包含 1 個元素（值為 0），不是空 vector！
//   因為 {} 被解析為 initializer_list 中的一個「預設初始化的 int（= 0）」
// ============================================================================

class Widget
{
public:
    Widget()
    {
        std::cout << "  Widget() 預設建構子\n";
    }

    Widget(std::initializer_list<int> init)
    {
        std::cout << "  Widget(initializer_list<int>, size="
                  << init.size() << ")\n";
    }
};

class Gadget
{
public:
    Gadget()
    {
        std::cout << "  Gadget() 預設建構子\n";
    }
    // 沒有 initializer_list 建構子
};

void demo_trap4_empty_braces()
{
    std::cout << "===== 【陷阱 4】空大括號 {} 的歧義 =====\n";

    // --- 有 initializer_list 建構子的類別 ---
    std::cout << "\n--- Widget（有 initializer_list 建構子）---\n";

    std::cout << "Widget w1;      → ";
    Widget w1;          // 預設建構子

    std::cout << "Widget w2{};    → ";
    Widget w2{};        // 預設建構子（空 {} 優先匹配預設建構子）

    std::cout << "Widget w3({});  → ";
    Widget w3({});      // initializer_list 建構子，傳入空列表

    std::cout << "Widget w4{{}}; → ";
    Widget w4{{}};      // initializer_list 建構子，列表含一個 int{} = 0

    // --- 無 initializer_list 建構子的類別 ---
    std::cout << "\n--- Gadget（無 initializer_list 建構子）---\n";

    std::cout << "Gadget g1;      → ";
    Gadget g1;          // 預設建構子

    std::cout << "Gadget g2{};    → ";
    Gadget g2{};        // 預設建構子

    // --- std::vector 的案例（最容易出錯的地方）---
    std::cout << "\n--- std::vector 的空大括號陷阱 ---\n";

    std::vector<int> v1;      // 空 vector
    std::vector<int> v2{};    // 空 vector（空 {} → 預設建構子）
    std::vector<int> v3({});  // 空 vector（透過空 initializer_list 建構）
    std::vector<int> v4{{}};  // 注意：包含 1 個元素（值為 0）！

    std::cout << "vector<int> v1;     size = " << v1.size() << "\n";
    std::cout << "vector<int> v2{};   size = " << v2.size() << "\n";
    std::cout << "vector<int> v3({}); size = " << v3.size() << "\n";
    std::cout << "vector<int> v4{{}}; size = " << v4.size()
              << " (元素值: " << v4[0] << ") ← 不是空的！\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 5】std::vector 的著名陷阱 — () vs {}
// ============================================================================
//
// 核心觀念：
//   std::vector<int> 有多個建構子，其中：
//     vector(size_t count)           → 建立 count 個元素（值為 0）
//     vector(size_t count, int val)  → 建立 count 個元素（值為 val）
//     vector(initializer_list<int>)  → 用列表中的值初始化元素
//
//   使用 () 和 {} 會呼叫完全不同的建構子：
//     vector<int> v(5);      → 5 個元素，值都是 0
//     vector<int> v{5};      → 1 個元素，值是 5
//     vector<int> v(5, 10);  → 5 個元素，值都是 10
//     vector<int> v{5, 10};  → 2 個元素，值是 5 和 10
//
// 實務建議：
//   - 想指定「大小」時使用 ()
//   - 想指定「元素內容」時使用 {}
// ============================================================================

void demo_trap5_vector_pitfall()
{
    std::cout << "===== 【陷阱 5】std::vector 的著名陷阱 =====\n";

    // --- int 版本 ---
    std::cout << "\n--- vector<int> ---\n";

    std::vector<int> v1(5);       // 5 個元素，全部為 0
    std::vector<int> v2{5};       // 1 個元素，值為 5
    std::vector<int> v3(5, 10);   // 5 個元素，全部為 10
    std::vector<int> v4{5, 10};   // 2 個元素：5 和 10

    // 輔助 lambda：印出 vector 內容
    auto printVec = [](const std::string& name, const std::vector<int>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << v[i];
        }
        std::cout << "}\n";
    };

    printVec("v1(5)    ", v1);    // {0, 0, 0, 0, 0}
    printVec("v2{5}    ", v2);    // {5}
    printVec("v3(5, 10)", v3);    // {10, 10, 10, 10, 10}
    printVec("v4{5, 10}", v4);    // {5, 10}

    // --- string 版本（展示 {} 不匹配時的退回行為）---
    std::cout << "\n--- vector<string> ---\n";

    std::vector<std::string> s1(3);            // 3 個空字串
    // std::vector<std::string> s2{3};         // 編譯錯誤！int 無法轉為 string
    std::vector<std::string> s3(3, "hello");   // 3 個 "hello"
    std::vector<std::string> s4{"a", "b"};     // 2 個元素："a" 和 "b"

    auto printStrVec = [](const std::string& name,
                          const std::vector<std::string>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << "\"" << v[i] << "\"";
        }
        std::cout << "}\n";
    };

    printStrVec("s1(3)         ", s1);
    printStrVec("s3(3, \"hello\")", s3);
    printStrVec("s4{\"a\", \"b\"} ", s4);
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 6】建構子選擇的微妙差異 — 即使轉換不完美也優先匹配
// ============================================================================
//
// 核心觀念：
//   當 {} 內的值「可以轉換」為 initializer_list 的元素型別時，
//   即使存在更精確匹配的一般建構子，編譯器仍然會優先選擇 initializer_list 版本。
//
// 範例：
//   class Converter 有三個建構子：
//     Converter(int)
//     Converter(double)
//     Converter(initializer_list<double>)
//
//   Converter c(42);   → 呼叫 Converter(int)      （精確匹配）
//   Converter c{42};   → 呼叫 Converter(initializer_list<double>)
//                        因為 42（int）可以隱式轉換為 double
//
// 結論：
//   只要 {} 內的值可以轉換為 initializer_list 的元素型別，
//   initializer_list 建構子就會「劫持」所有 {} 初始化。
// ============================================================================

class Converter
{
public:
    Converter(int value)
    {
        std::cout << "  Converter(int " << value << ")\n";
    }

    Converter(double value)
    {
        std::cout << "  Converter(double " << value << ")\n";
    }

    Converter(std::initializer_list<double> init)
    {
        std::cout << "  Converter(initializer_list<double>, size="
                  << init.size() << ")\n";
    }
};

void demo_trap6_constructor_selection()
{
    std::cout << "===== 【陷阱 6】建構子選擇的微妙差異 =====\n";

    std::cout << "\nConverter c1(42);   → ";
    Converter c1(42);       // 呼叫 Converter(int)

    std::cout << "Converter c2{42};   → ";
    Converter c2{42};       // 呼叫 initializer_list<double>！
                            // 因為 42 (int) 可以隱式轉換為 double

    std::cout << "Converter c3(3.14); → ";
    Converter c3(3.14);     // 呼叫 Converter(double)

    std::cout << "Converter c4{3.14}; → ";
    Converter c4{3.14};     // 呼叫 initializer_list<double>

    std::cout << "\n重點：即使存在更精確的 Converter(int) 建構子，\n";
    std::cout << "      {} 初始化仍然優先匹配 initializer_list<double>，\n";
    std::cout << "      因為 int 可以隱式轉換為 double。\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 7】如何強制呼叫非 initializer_list 建構子
// ============================================================================
//
// 核心觀念：
//   當類別有 initializer_list 建構子時，使用 {} 初始化會被「劫持」。
//   要繞過這個行為，必須使用 () 初始化。
//
// 解決方案：
//   方案 1：使用小括號 () 初始化（最簡單、最直接）
//     ForceConstruct f(5, 10);   → 呼叫 (int, int) 建構子
//     ForceConstruct f{5, 10};   → 呼叫 initializer_list 建構子
//
//   方案 2：讓元素型別無法轉換為 initializer_list 的元素型別
//     （通常不實用，僅理論上可行）
//
// 實務建議：
//   記住一個原則 —
//     {} → 優先 initializer_list
//     () → 匹配最佳的非 initializer_list 建構子
// ============================================================================

class ForceConstruct
{
public:
    ForceConstruct(int a, int b)
    {
        std::cout << "  ForceConstruct(int " << a << ", int " << b << ")\n";
    }

    ForceConstruct(std::initializer_list<int> init)
    {
        std::cout << "  ForceConstruct(initializer_list, size="
                  << init.size() << ")\n";
    }
};

void demo_trap7_force_non_initlist()
{
    std::cout << "===== 【陷阱 7】如何強制呼叫非 initializer_list 建構子 =====\n";

    // 問題：想呼叫 (int, int) 版本
    std::cout << "\n--- 問題：{} 被 initializer_list 劫持 ---\n";
    std::cout << "ForceConstruct f1{5, 10};  → ";
    ForceConstruct f1{5, 10};   // 匹配 initializer_list 版本

    // 解決方案：使用 () 初始化
    std::cout << "\n--- 解決：使用 () 呼叫 ---\n";
    std::cout << "ForceConstruct f2(5, 10);  → ";
    ForceConstruct f2(5, 10);   // 匹配 (int, int) 版本

    std::cout << "\n結論：\n";
    std::cout << "  使用 {} → 優先匹配 initializer_list 建構子\n";
    std::cout << "  使用 () → 匹配最佳的非 initializer_list 建構子\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 8】聚合類別（Aggregate）的特殊初始化規則
// ============================================================================
//
// 核心觀念：
//   聚合類別可以使用 {} 進行「聚合初始化」，不需要建構子。
//   未提供的成員會被值初始化（對基本型別來說就是 0）。
//
// 聚合類別的條件（C++11/14）：
//   1. 沒有使用者宣告的建構子
//   2. 沒有私有（private）或保護（protected）的非靜態成員
//   3. 沒有虛擬函式（virtual function）
//   4. 沒有虛擬、私有、保護的基礎類別
//
// 聚合初始化 vs 列表初始化：
//   聚合初始化使用 {} 時，按照成員宣告順序依次賦值。
//   非聚合類別無法進行聚合初始化，必須透過建構子。
//
// C++17 擴展：
//   C++17 允許聚合類別有基礎類別（public、非虛擬），
//   並且可以在 {} 中包含基礎類別的初始值。
// ============================================================================

// 聚合類別：沒有使用者定義的建構子
struct Aggregate
{
    int x;
    int y;
    int z;
};

// 非聚合類別：有使用者定義的建構子
struct NonAggregate
{
    int x;
    int y;
    int z;

    NonAggregate() : x{0}, y{0}, z{0} {}  // 使用者定義的建構子 → 不再是聚合類別
};

void demo_trap8_aggregate()
{
    std::cout << "===== 【陷阱 8】聚合類別的特殊初始化規則 =====\n";

    // --- 聚合類別：可以直接用 {} 按成員順序初始化 ---
    std::cout << "\n--- 聚合類別 Aggregate ---\n";

    Aggregate a1{1, 2, 3};   // x=1, y=2, z=3
    Aggregate a2{1, 2};      // x=1, y=2, z=0（未提供的成員值初始化為 0）
    Aggregate a3{1};          // x=1, y=0, z=0
    Aggregate a4{};           // x=0, y=0, z=0（全部值初始化為 0）

    std::cout << "Aggregate a1{1, 2, 3}: x=" << a1.x
              << ", y=" << a1.y << ", z=" << a1.z << "\n";
    std::cout << "Aggregate a2{1, 2}:    x=" << a2.x
              << ", y=" << a2.y << ", z=" << a2.z << "\n";
    std::cout << "Aggregate a3{1}:       x=" << a3.x
              << ", y=" << a3.y << ", z=" << a3.z << "\n";
    std::cout << "Aggregate a4{}:        x=" << a4.x
              << ", y=" << a4.y << ", z=" << a4.z << "\n";

    // --- 非聚合類別：必須透過建構子 ---
    std::cout << "\n--- 非聚合類別 NonAggregate ---\n";

    // NonAggregate n1{1, 2, 3};  // 編譯錯誤！不是聚合類別，無法聚合初始化
    NonAggregate n2{};            // 呼叫預設建構子

    std::cout << "NonAggregate n2{}: 呼叫預設建構子\n";
    std::cout << "NonAggregate n1{1,2,3}; → 編譯錯誤！不是聚合類別\n";

    std::cout << "\n--- 聚合類別的條件 (C++11/14) ---\n";
    std::cout << "1. 沒有使用者宣告的建構子\n";
    std::cout << "2. 沒有私有或保護的非靜態成員\n";
    std::cout << "3. 沒有虛擬函式\n";
    std::cout << "4. 沒有虛擬、私有、保護的基礎類別\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 9】initializer_list 的生命週期問題
// ============================================================================
//
// 核心觀念：
//   std::initializer_list 只是一個輕量級的「參考」物件，
//   它不擁有底層的陣列資料。底層陣列是一個臨時物件。
//
// 安全的使用方式：
//   (a) 立即用於初始化容器：
//       std::vector<int> v{1, 2, 3};  → 安全，元素被複製到 vector
//   (b) 在 range-for 中使用：
//       for (int x : {1, 2, 3}) { ... }  → 安全，底層陣列在迴圈期間存活
//
// 危險的使用方式：
//   (a) 從函式回傳 initializer_list：
//       std::initializer_list<int> f() { return {1,2,3}; }
//       → 危險！底層陣列在函式返回後已銷毀，使用回傳值是未定義行為
//   (b) 將 initializer_list 作為類別成員儲存：
//       class Holder { std::initializer_list<int> data_; };
//       → 危險！建構子結束後底層陣列可能已銷毀
//
// 正確做法：
//   將 initializer_list 的元素複製到 std::vector 等容器中儲存。
// ============================================================================

// 危險範例：從函式回傳 initializer_list
std::initializer_list<int> dangerousReturn()
{
    return {1, 2, 3};  // 危險！底層陣列是函式內的臨時物件
}

void demo_trap9_lifetime()
{
    std::cout << "===== 【陷阱 9】initializer_list 的生命週期問題 =====\n";

    // --- 安全用法 1：立即用於初始化容器 ---
    std::cout << "\n--- 安全用法 ---\n";

    std::vector<int> v{1, 2, 3};  // 安全：vector 的建構子會複製所有元素
    std::cout << "std::vector<int> v{1, 2, 3}; → 安全，元素被複製到 vector\n";

    // --- 安全用法 2：在 range-for 中使用 ---
    for (int x : {1, 2, 3})
    {
        // 底層陣列在整個 for 迴圈期間存活
        (void)x;  // 避免未使用變數的警告
    }
    std::cout << "for (int x : {1, 2, 3}) { ... } → 安全，陣列在迴圈期間存活\n";

    // --- 危險用法 1：從函式回傳 initializer_list ---
    std::cout << "\n--- 危險用法 ---\n";

    std::cout << "std::initializer_list<int> dangerousReturn() {\n";
    std::cout << "    return {1, 2, 3};  // 危險！底層陣列在返回後已銷毀\n";
    std::cout << "}\n";
    std::cout << "→ 使用回傳的 initializer_list 是未定義行為！\n";

    // auto list = dangerousReturn();  // 未定義行為！請勿使用

    // --- 危險用法 2：作為類別成員儲存 ---
    std::cout << "\n--- 危險：作為成員儲存 ---\n";
    std::cout << "class Holder {\n";
    std::cout << "    std::initializer_list<int> data_;  // 危險！\n";
    std::cout << "};\n";
    std::cout << "→ 建構子結束後，底層陣列可能已銷毀\n";

    // --- 正確做法：複製到容器 ---
    std::cout << "\n--- 正確做法 ---\n";
    std::cout << "class SafeHolder {\n";
    std::cout << "    std::vector<int> data_;  // 安全！\n";
    std::cout << "    SafeHolder(std::initializer_list<int> init)\n";
    std::cout << "        : data_(init) {}  // 元素被複製到 vector\n";
    std::cout << "};\n";
    std::cout << "\n";
}

// ============================================================================
// 【陷阱 10】嵌套大括號的解析
// ============================================================================
//
// 核心觀念：
//   C++11 的列表初始化允許嵌套使用 {}，
//   這在初始化 pair、map、vector<vector<int>> 等複合型別時非常方便。
//
// 常見用法：
//   std::pair<int, int> p = {1, 2};           → 直覺的 pair 初始化
//   std::map<string, int> m = {{"a", 1}};     → 嵌套 {} 初始化 map
//   std::vector<vector<int>> mat = {{1,2}, {3,4}};  → 二維矩陣
//
// 注意事項：
//   嵌套層級越多，可讀性越低。
//   適度使用可以讓程式碼更簡潔，過度使用則會造成混淆。
// ============================================================================

void demo_trap10_nested_braces()
{
    std::cout << "===== 【陷阱 10】嵌套大括號的解析 =====\n";

    // --- std::pair 的初始化 ---
    std::cout << "\n--- std::pair 的初始化 ---\n";

    std::pair<int, int> p1(1, 2);      // 傳統的 () 方式
    std::pair<int, int> p2{1, 2};      // C++11 的 {} 方式
    std::pair<int, int> p3 = {1, 2};   // C++11 的 = {} 方式

    std::cout << "pair p1(1, 2):   " << p1.first << ", " << p1.second << "\n";
    std::cout << "pair p2{1, 2}:   " << p2.first << ", " << p2.second << "\n";
    std::cout << "pair p3 = {1,2}: " << p3.first << ", " << p3.second << "\n";

    // --- std::map 的嵌套初始化 ---
    std::cout << "\n--- std::map 的嵌套初始化 ---\n";

    std::map<std::string, int> m1{
        {"one", 1},      // 每個 {} 是一個 pair<string, int>
        {"two", 2},
        {"three", 3}
    };

    std::cout << "map 使用嵌套 {} 初始化:\n";
    for (const auto& [key, value] : m1)  // C++17 結構化繫結
    {
        std::cout << "  " << key << " -> " << value << "\n";
    }

    // --- vector<vector<int>> 二維矩陣 ---
    std::cout << "\n--- vector<vector<int>> 二維矩陣 ---\n";

    std::vector<std::vector<int>> matrix{
        {1, 2, 3},      // 第 0 列
        {4, 5, 6},      // 第 1 列
        {7, 8, 9}       // 第 2 列
    };

    std::cout << "matrix:\n";
    for (const auto& row : matrix)
    {
        std::cout << "  ";
        for (int n : row)
        {
            std::cout << n << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

// ============================================================================
// 主程式：依序展示所有十大陷阱
// ============================================================================

// ============================================================================
// 【LeetCode 實戰範例】LeetCode 973. K Closest Points to Origin
//   題目：給定平面上的點，回傳距離原點最近的 k 個點。
//   為什麼用到本主題：
//     這題的輸入與輸出都是 vector<vector<int>>，全程與列表初始化正面遭遇：
//       (a) 回傳 {p[0], p[1]} 就地建構結果元素 —— {} 最自然的用途；
//       (b) 距離用 long long 累加，避免 int 溢位；若寫成
//           int d{p[0]*p[0] + p[1]*p[1]}; 在座標大時會溢位，
//           而且 {} 也擋不住（那是溢位不是窄化，屬兩回事）——
//           這正好示範「{} 的保護有邊界」。
//   複雜度：時間 O(n log n)（排序），空間 O(n)
// ============================================================================
std::vector<std::vector<int>> kClosest(std::vector<std::vector<int>> points, int k)
{
    if (k <= 0 || points.empty()) return {};                 // {} = 空結果

    // 用 long long 算平方距離：座標達 1e4 時 x*x+y*y 會超出 int 範圍
    std::sort(points.begin(), points.end(),
              [](const std::vector<int>& a, const std::vector<int>& b) {
                  long long da = 1LL * a[0] * a[0] + 1LL * a[1] * a[1];
                  long long db = 1LL * b[0] * b[0] + 1LL * b[1] * b[1];
                  return da < db;
              });

    std::vector<std::vector<int>> out;
    int take = (k < static_cast<int>(points.size()))
                   ? k : static_cast<int>(points.size());
    for (int i = 0; i < take; ++i) {
        out.push_back({points[i][0], points[i][1]});         // {} 就地建構
    }
    return out;
}

// ============================================================================
// 【日常實務範例 1】感測器封包解析 — {} 擋得住什麼、擋不住什麼
//   情境：從裝置收到的原始讀數是 double（例如 23.7 度），
//         但下游的儲存格式是 int16_t（節省頻寬）。
//   為什麼用到本主題：
//     這是實務上最容易「靜默失真」的地方。本例對照三種寫法，
//     並示範 {} 的保護邊界：編譯期常數擋得住，執行期資料擋不住。
// ============================================================================
struct SensorReading {
    std::int16_t temperatureCentiC;   // 溫度，單位 0.01°C
    std::int16_t humidityPercent;
    std::uint8_t batteryPercent;
};

// 安全轉換：執行期資料必須自己檢查範圍（{} 在這裡幫不了忙）
bool toCentiCelsius(double celsius, std::int16_t& out)
{
    double centi = celsius * 100.0;
    if (centi < -32768.0 || centi > 32767.0) return false;   // 自行做範圍把關
    out = static_cast<std::int16_t>(centi);                  // 明確轉換
    return true;
}

// ============================================================================
// 【日常實務範例 2】容器初始化的經典災難 — 誤把 {} 當 ()
//   情境：要預先配置一個「有 N 個預設值」的緩衝區。
//         這是 {} vs () 差異在真實專案裡最常造成 bug 的形式。
//   為什麼用到本主題：
//     buffer{1024} 只會得到「1 個元素、值為 1024」，
//     而非預期的「1024 個元素」。程式編得過、跑得動，
//     直到寫入第 2 個位置才越界 —— 而那可能是上線之後的事。
// ============================================================================
void demonstrateBufferBug()
{
    const std::size_t kBufferSize = 1024;

    std::vector<int> correct(kBufferSize);      // ✅ () → 1024 個元素，全為 0
    std::vector<int> wrong{kBufferSize};        // ❌ {} → 1 個元素，值為 1024

    std::cout << "  預期配置 " << kBufferSize << " 個元素的緩衝區：\n";
    std::cout << "    std::vector<int> buf(1024);  → size = "
              << correct.size() << "  ✅ 正確\n";
    std::cout << "    std::vector<int> buf{1024};  → size = "
              << wrong.size() << "     ❌ 只有 1 個元素，值是 1024\n";
    std::cout << "    後者寫入 buf[1] 就已越界，屬未定義行為，\n";
    std::cout << "    但不保證當場崩潰 —— 常常要到上線後才以奇怪的方式浮現。\n";
}

// ============================================================================
int main()
{
    std::cout << std::boolalpha;

    std::cout << "================================================================\n";
    std::cout << " 第 1.6 章 總複習：列表初始化的陷阱 — 十大陷阱完整展示\n";
    std::cout << "================================================================\n\n";

    demo_trap1_initializer_list_priority();     // 陷阱 1：initializer_list 優先順序
    demo_trap2_narrowing_conversion();          // 陷阱 2：窄化轉換
    demo_trap3_auto_brace();                    // 陷阱 3：auto 與大括號
    demo_trap4_empty_braces();                  // 陷阱 4：空大括號的歧義
    demo_trap5_vector_pitfall();                // 陷阱 5：vector 的 () vs {}
    demo_trap6_constructor_selection();          // 陷阱 6：建構子選擇差異
    demo_trap7_force_non_initlist();            // 陷阱 7：強制呼叫非 initializer_list
    demo_trap8_aggregate();                     // 陷阱 8：聚合類別特殊規則
    demo_trap9_lifetime();                      // 陷阱 9：initializer_list 生命週期
    demo_trap10_nested_braces();                // 陷阱 10：嵌套大括號解析

    std::cout << "================================================================\n";
    std::cout << " 【總結】實務建議\n";
    std::cout << "================================================================\n";
    std::cout << "\n";
    std::cout << "1. 當類別有 initializer_list 建構子時，{} 會優先匹配它\n";
    std::cout << "2. {} 初始化禁止窄化轉換，這是安全優勢\n";
    std::cout << "3. auto 與 {} 的推導規則在 C++17 有重大變化\n";
    std::cout << "4. 空 {} 優先匹配預設建構子，而非 initializer_list\n";
    std::cout << "5. vector<int> v(5) 和 v{5} 語意完全不同\n";
    std::cout << "6. 即使轉換不完美，{} 仍優先匹配 initializer_list\n";
    std::cout << "7. 想繞過 initializer_list 優先權，使用 () 初始化\n";
    std::cout << "8. 聚合類別可以直接用 {} 按成員順序初始化\n";
    std::cout << "9. 不要回傳或儲存 initializer_list，改用 vector\n";
    std::cout << "10. 嵌套 {} 適合初始化複合型別，但不要過度嵌套\n";
    std::cout << "\n";

    // ========================================================================
    // 【LeetCode 實戰】973. K Closest Points to Origin
    // ========================================================================
    std::cout << "================================================================\n";
    std::cout << " 【LeetCode 973】K Closest Points to Origin\n";
    std::cout << "================================================================\n\n";
    {
        // 巢狀 {} 把測資寫成「點的清單」，形狀一目了然
        std::vector<std::vector<int>> pts{{1, 3}, {-2, 2}, {5, 8}, {0, 1}};
        std::vector<std::vector<int>> ans = kClosest(pts, 2);

        std::cout << "輸入點: (1,3) (-2,2) (5,8) (0,1)，取最近 2 個\n";
        std::cout << "結果  : ";
        for (const auto& p : ans) std::cout << "(" << p[0] << "," << p[1] << ") ";
        std::cout << "\n";
        std::cout << "k 超過點數時的保護: k=99 → 回傳 "
                  << kClosest(pts, 99).size() << " 個點（全部）\n";
        std::cout << "k=0 → 回傳 " << kClosest(pts, 0).size()
                  << " 個點（return {}; 表示空結果）\n\n";
        std::cout << "重點：距離用 long long 累加。若寫成 int 並以 {} 初始化，\n";
        std::cout << "      座標大時會「溢位」而非「窄化」，{} 擋不住 ——\n";
        std::cout << "      這說明 {} 的保護只涵蓋型別轉換，不涵蓋算術溢位。\n\n";
    }

    // ========================================================================
    // 【日常實務 1】感測器封包：{} 的保護邊界
    // ========================================================================
    std::cout << "================================================================\n";
    std::cout << " 【日常實務 1】感測器封包 — {} 的保護邊界\n";
    std::cout << "================================================================\n\n";
    {
        // 編譯期常數：{} 全程把關，任何失真都會編譯失敗
        SensorReading r{2370, 55, 87};
        std::cout << "編譯期常數初始化（{} 有保護）：\n";
        std::cout << "  溫度 " << r.temperatureCentiC / 100.0 << "°C, 濕度 "
                  << r.humidityPercent << "%, 電量 "
                  << static_cast<int>(r.batteryPercent) << "%\n";
        std::cout << "  若寫成 SensorReading{2370.5, 55, 87} → 編譯錯誤（窄化）\n";
        std::cout << "  若寫成 SensorReading{99999, 55, 87} → 編譯錯誤（超出 int16_t）\n\n";

        // 執行期資料：{} 幫不上忙，必須自己檢查
        std::cout << "執行期資料（{} 沒有保護，須自行驗證）：\n";
        const double samples[] = {23.7, -12.34, 400.0};
        for (std::size_t i = 0; i < 3; ++i) {
            std::int16_t centi{0};
            bool ok = toCentiCelsius(samples[i], centi);
            std::cout << "  " << samples[i] << "°C → "
                      << (ok ? "接受" : "拒絕（超出 int16_t 範圍）");
            if (ok) std::cout << "，存為 " << centi;
            std::cout << "\n";
        }
        std::cout << "\n  結論：{} 只在「編譯期已知值」時擋得住失真。\n";
        std::cout << "        來自設定檔／網路／使用者的資料仍須自行做範圍檢查。\n\n";
    }

    // ========================================================================
    // 【日常實務 2】緩衝區配置的經典災難
    // ========================================================================
    std::cout << "================================================================\n";
    std::cout << " 【日常實務 2】vector 緩衝區：{} 與 () 的致命差異\n";
    std::cout << "================================================================\n\n";
    demonstrateBufferBug();
    std::cout << "\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "summary.cpp" -o narrowing_summary

// === 預期輸出 ===
// ================================================================
//  第 1.6 章 總複習：列表初始化的陷阱 — 十大陷阱完整展示
// ================================================================
//
// ===== 【陷阱 1】initializer_list 建構子的優先順序 =====
//
// Container c1(5);        →   Container(size_t size=5)
// Container c2{5};        →   Container(initializer_list<int>, size=1, values={5})
// Container c3(5, 10);    →   Container(size_t size=5, int value=10)
// Container c4{5, 10};    →   Container(initializer_list<int>, size=2, values={5, 10})
// Container c5({5, 10});  →   Container(initializer_list<int>, size=2, values={5, 10})
//
// ===== 【陷阱 2】窄化轉換 =====
//
// --- 2.1 浮點數 → 整數 ---
// int a(3.14) = 3  ← () 允許，但截斷小數
// int f{3}    = 3  ← {} 允許，整數常量無損失
// int c{d};        ← 編譯錯誤！{} 禁止 double→int 窄化
//
// --- 2.2 大整數 → 小整數 ---
// int i1(big)  = 1000  ← () 允許
// int i4{100LL} = 100 ← {} 允許，常量在 int 範圍內
// int i3{big};       ← 編譯錯誤！big 是變數，即使值放得下
//
// --- 2.3 有號 ↔ 無號 轉換 ---
// unsigned u1(-1)   = 4294967295  ← () 允許，但值是垃圾
// unsigned u3{negative}; ← 編譯錯誤！{} 禁止有號→無號窄化
//
// --- 2.4 指標 → bool ---
// bool b1(nullptr) = false  ← () 允許
// bool b3{ptr};          ← 編譯錯誤！{} 禁止指標→bool
//
// ===== 【陷阱 3】auto 與大括號的推導規則 =====
//
// --- auto = {...} ---
// auto a = {1, 2, 3}; → initializer_list<int>, size=3
// auto b = {42};      → initializer_list<int>, size=1
//
// --- auto x{value} (C++17 改變) ---
// auto c{42}; → type is int, value=42
//
// --- 對比表 ---
// 語法              C++11/14               C++17
// auto x = {1,2,3}  initializer_list<int>  initializer_list<int>
// auto x = {42}     initializer_list<int>  initializer_list<int>
// auto x{42}        initializer_list<int>  int
// auto x{1,2,3}     initializer_list<int>  編譯錯誤
//
// ===== 【陷阱 4】空大括號 {} 的歧義 =====
//
// --- Widget（有 initializer_list 建構子）---
// Widget w1;      →   Widget() 預設建構子
// Widget w2{};    →   Widget() 預設建構子
// Widget w3({});  →   Widget(initializer_list<int>, size=0)
// Widget w4{{}}; →   Widget(initializer_list<int>, size=1)
//
// --- Gadget（無 initializer_list 建構子）---
// Gadget g1;      →   Gadget() 預設建構子
// Gadget g2{};    →   Gadget() 預設建構子
//
// --- std::vector 的空大括號陷阱 ---
// vector<int> v1;     size = 0
// vector<int> v2{};   size = 0
// vector<int> v3({}); size = 0
// vector<int> v4{{}}; size = 1 (元素值: 0) ← 不是空的！
//
// ===== 【陷阱 5】std::vector 的著名陷阱 =====
//
// --- vector<int> ---
// v1(5)     (size=5): {0, 0, 0, 0, 0}
// v2{5}     (size=1): {5}
// v3(5, 10) (size=5): {10, 10, 10, 10, 10}
// v4{5, 10} (size=2): {5, 10}
//
// --- vector<string> ---
// s1(3)          (size=3): {"", "", ""}
// s3(3, "hello") (size=3): {"hello", "hello", "hello"}
// s4{"a", "b"}  (size=2): {"a", "b"}
//
// ===== 【陷阱 6】建構子選擇的微妙差異 =====
//
// Converter c1(42);   →   Converter(int 42)
// Converter c2{42};   →   Converter(initializer_list<double>, size=1)
// Converter c3(3.14); →   Converter(double 3.14)
// Converter c4{3.14}; →   Converter(initializer_list<double>, size=1)
//
// 重點：即使存在更精確的 Converter(int) 建構子，
//       {} 初始化仍然優先匹配 initializer_list<double>，
//       因為 int 可以隱式轉換為 double。
//
// ===== 【陷阱 7】如何強制呼叫非 initializer_list 建構子 =====
//
// --- 問題：{} 被 initializer_list 劫持 ---
// ForceConstruct f1{5, 10};  →   ForceConstruct(initializer_list, size=2)
//
// --- 解決：使用 () 呼叫 ---
// ForceConstruct f2(5, 10);  →   ForceConstruct(int 5, int 10)
//
// 結論：
//   使用 {} → 優先匹配 initializer_list 建構子
//   使用 () → 匹配最佳的非 initializer_list 建構子
//
// ===== 【陷阱 8】聚合類別的特殊初始化規則 =====
//
// --- 聚合類別 Aggregate ---
// Aggregate a1{1, 2, 3}: x=1, y=2, z=3
// Aggregate a2{1, 2}:    x=1, y=2, z=0
// Aggregate a3{1}:       x=1, y=0, z=0
// Aggregate a4{}:        x=0, y=0, z=0
//
// --- 非聚合類別 NonAggregate ---
// NonAggregate n2{}: 呼叫預設建構子
// NonAggregate n1{1,2,3}; → 編譯錯誤！不是聚合類別
//
// --- 聚合類別的條件 (C++11/14) ---
// 1. 沒有使用者宣告的建構子
// 2. 沒有私有或保護的非靜態成員
// 3. 沒有虛擬函式
// 4. 沒有虛擬、私有、保護的基礎類別
//
// ===== 【陷阱 9】initializer_list 的生命週期問題 =====
//
// --- 安全用法 ---
// std::vector<int> v{1, 2, 3}; → 安全，元素被複製到 vector
// for (int x : {1, 2, 3}) { ... } → 安全，陣列在迴圈期間存活
//
// --- 危險用法 ---
// std::initializer_list<int> dangerousReturn() {
//     return {1, 2, 3};  // 危險！底層陣列在返回後已銷毀
// }
// → 使用回傳的 initializer_list 是未定義行為！
//
// --- 危險：作為成員儲存 ---
// class Holder {
//     std::initializer_list<int> data_;  // 危險！
// };
// → 建構子結束後，底層陣列可能已銷毀
//
// --- 正確做法 ---
// class SafeHolder {
//     std::vector<int> data_;  // 安全！
//     SafeHolder(std::initializer_list<int> init)
//         : data_(init) {}  // 元素被複製到 vector
// };
//
// ===== 【陷阱 10】嵌套大括號的解析 =====
//
// --- std::pair 的初始化 ---
// pair p1(1, 2):   1, 2
// pair p2{1, 2}:   1, 2
// pair p3 = {1,2}: 1, 2
//
// --- std::map 的嵌套初始化 ---
// map 使用嵌套 {} 初始化:
//   one -> 1
//   three -> 3
//   two -> 2
//
// --- vector<vector<int>> 二維矩陣 ---
// matrix:
//   1 2 3 
//   4 5 6 
//   7 8 9 
//
// ================================================================
//  【總結】實務建議
// ================================================================
//
// 1. 當類別有 initializer_list 建構子時，{} 會優先匹配它
// 2. {} 初始化禁止窄化轉換，這是安全優勢
// 3. auto 與 {} 的推導規則在 C++17 有重大變化
// 4. 空 {} 優先匹配預設建構子，而非 initializer_list
// 5. vector<int> v(5) 和 v{5} 語意完全不同
// 6. 即使轉換不完美，{} 仍優先匹配 initializer_list
// 7. 想繞過 initializer_list 優先權，使用 () 初始化
// 8. 聚合類別可以直接用 {} 按成員順序初始化
// 9. 不要回傳或儲存 initializer_list，改用 vector
// 10. 嵌套 {} 適合初始化複合型別，但不要過度嵌套
//
// ================================================================
//  【LeetCode 973】K Closest Points to Origin
// ================================================================
//
// 輸入點: (1,3) (-2,2) (5,8) (0,1)，取最近 2 個
// 結果  : (0,1) (-2,2) 
// k 超過點數時的保護: k=99 → 回傳 4 個點（全部）
// k=0 → 回傳 0 個點（return {}; 表示空結果）
//
// 重點：距離用 long long 累加。若寫成 int 並以 {} 初始化，
//       座標大時會「溢位」而非「窄化」，{} 擋不住 ——
//       這說明 {} 的保護只涵蓋型別轉換，不涵蓋算術溢位。
//
// ================================================================
//  【日常實務 1】感測器封包 — {} 的保護邊界
// ================================================================
//
// 編譯期常數初始化（{} 有保護）：
//   溫度 23.7°C, 濕度 55%, 電量 87%
//   若寫成 SensorReading{2370.5, 55, 87} → 編譯錯誤（窄化）
//   若寫成 SensorReading{99999, 55, 87} → 編譯錯誤（超出 int16_t）
//
// 執行期資料（{} 沒有保護，須自行驗證）：
//   23.7°C → 接受，存為 2370
//   -12.34°C → 接受，存為 -1234
//   400°C → 拒絕（超出 int16_t 範圍）
//
//   結論：{} 只在「編譯期已知值」時擋得住失真。
//         來自設定檔／網路／使用者的資料仍須自行做範圍檢查。
//
// ================================================================
//  【日常實務 2】vector 緩衝區：{} 與 () 的致命差異
// ================================================================
//
//   預期配置 1024 個元素的緩衝區：
//     std::vector<int> buf(1024);  → size = 1024  ✅ 正確
//     std::vector<int> buf{1024};  → size = 1     ❌ 只有 1 個元素，值是 1024
//     後者寫入 buf[1] 就已越界，屬未定義行為，
//     但不保證當場崩潰 —— 常常要到上線後才以奇怪的方式浮現。
