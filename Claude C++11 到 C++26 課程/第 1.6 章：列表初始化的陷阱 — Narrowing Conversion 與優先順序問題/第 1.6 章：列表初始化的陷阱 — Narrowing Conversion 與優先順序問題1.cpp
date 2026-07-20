// =============================================================================
//  第 1.6 章：列表初始化的陷阱 — Narrowing Conversion 與優先順序問題
//  一句話主題：{} 不是「更好看的 ()」，它走的是一套完全不同的多載解析流程
// =============================================================================
//
// 【主題資訊 Information】
//   語法形式：
//     T obj{a, b};        // direct-list-initialization      （直接列表初始化）
//     T obj = {a, b};     // copy-list-initialization        （複製列表初始化）
//     T obj(a, b);        // direct-initialization           （傳統小括號）
//     f({a, b});          // 以 braced-init-list 當引數
//     return {a, b};      // 以 braced-init-list 當回傳值
//
//   標準版本：
//     C++11  引入 {} 統一初始化、std::initializer_list、禁止 narrowing conversion
//     C++14  聚合類別允許有 default member initializer（NSDMI）
//     C++17  聚合類別允許有 public 非虛擬基礎類別；auto x{1} 的推導規則由
//            N3922 修訂為 int（重要：見下方【5.】的誠實說明）
//     C++20  designated initializers（.x = 1）；聚合定義改為「無 user-declared
//            建構子」（P1008，比 C++17 的「無 user-provided」更嚴格）
//
//   標頭檔：<initializer_list>（用到 std::initializer_list 型別時需要；
//           但「寫出 {1,2,3} 這個語法」本身不需要 include 任何東西）
//
//   複雜度：initializer_list 的走訪是 O(N)；它內部只存 (const T* begin, size_t
//           size) 兩個欄位，複製 initializer_list 物件本身是 O(1)（淺複製，
//           不複製底層陣列）。
//
// -----------------------------------------------------------------------------
// 【詳細解釋 Explanation】
// -----------------------------------------------------------------------------
//
// 【1. C++11 為什麼要發明 {}：三個舊世界的痛點】
//   在 C++98，初始化語法是碎裂的：
//     int    a[3] = {1, 2, 3};   // 陣列用 {}
//     Point  p    = {1, 2};      // POD struct 用 {}
//     std::string s("hi");       // 類別用 ()
//     std::vector<int> v;        // 容器沒辦法用字面值直接填內容
//   三個具體痛點：
//     (a) 不一致：同樣是「給初值」，陣列/struct 用 {}，類別用 ()。
//     (b) 容器無法字面初始化：想要一個內含 1,2,3 的 vector，只能先建立
//         再 push_back 三次，或先做一個 C 陣列再用 iterator 區間建構。
//     (c) Most Vexing Parse：Widget w(); 看起來像建立物件，其實是宣告函式。
//   C++11 的答案是「Uniform Initialization」——任何地方都能用 {}，
//   再加上 std::initializer_list 讓「使用者自訂類別」也能接受字面列表。
//
// 【2. 為什麼 {} 要禁止 narrowing conversion】
//   C 的隱式轉換規則允許大量無聲的資料流失：
//     char c = 300;          // 無聲截斷
//     int  n = 3.99;         // 無聲丟掉小數
//     unsigned u = -1;       // 無聲變成天文數字
//   這些在 C 是「特性」，在現代 C++ 是 bug 溫床。C++11 藉著引入新語法的機會，
//   在新語法上「補上更嚴格的規則」而不破壞舊程式碼——這是標準委員會常用的
//   手法：舊語法 () 維持原樣（相容性），新語法 {} 採用更安全的規則。
//   所以本章的第一條實務準則是：
//     「{} 幫你把『可能失真的轉換』從 runtime 的錯誤答案，提前變成 compile time
//       的錯誤訊息。」
//
// 【3. 但 {} 同時帶來一個更大的坑：initializer_list 劫持】
//   為了讓 std::vector<int> v{1,2,3} 能運作，標準必須讓編譯器「優先」
//   把 {} 內的東西看成一個列表。這個「優先」不是普通的多載解析加權，
//   而是一個 兩階段的硬性過濾（見下方【概念補充】(A)）。
//   後果是 C++ 中最惡名昭彰的一組對比：
//     std::vector<int> a(10);   // 10 個元素，值都是 0
//     std::vector<int> b{10};   // 1  個元素，值是 10
//   兩行看起來只差一個括號形狀，語意天差地遠。這不是設計失誤，而是
//   「讓 {1,2,3} 可用」與「讓 {10} 表示大小」兩個需求本質上互斥，
//   標準必須擇一，它選了前者。
//
// 【4. 空的 {} 為什麼是預設建構、而不是空列表】
//   如果 {} 一律優先走 initializer_list，那 Widget w{}; 就會呼叫
//   Widget(initializer_list<int>) 並傳入空列表——但這顯然不是大家要的。
//   所以標準加了一條「例外的例外」：當 braced-init-list 為空且類別有
//   預設建構子時，直接呼叫預設建構子。想要真的傳空列表，必須寫成
//   Widget w({}); ——外層 () 是函式呼叫，內層 {} 才是那個空列表。
//   由此衍生一個高頻面試題：vector<int> v{{}} 的 size 是多少？答案是 1，
//   因為外層 {} 是列表、內層 {} 是列表裡的「一個值初始化的 int」= 0。
//
// 【5. auto x{1} 的推導：一段必須誠實交代的歷史（N3922）】
//   C++11 原始條文：auto 遇到 braced-init-list 一律推導成
//   std::initializer_list<T>，因此 auto x{1}; 的型別是
//   std::initializer_list<int>——這相當違反直覺。
//   N3922（":the auto bug"）修正為：
//     auto x{1};      → int                       （direct-list-init，恰好一個元素）
//     auto x{1, 2};   → ill-formed                （direct-list-init 只准一個元素）
//     auto x = {1};   → std::initializer_list<int>（copy-list-init，維持原樣）
//     auto x = {1,2}; → std::initializer_list<int>
//   這項修訂被視為 Defect Report（缺陷報告），意思是「原本的條文是錯的」，
//   所以編譯器廠商普遍把它「回溯套用」到舊的 -std= 模式，而不是只在
//   -std=c++17 才啟用。
//
//   ★ 本機實測（GCC 15.2.0，-pedantic-errors）：
//     -std=c++11 / c++14 / c++17 / c++20 四種模式下，auto c{42} 的
//     decltype 全部都是 int；auto d{1,2} 全部都被拒絕，錯誤訊息是
//     "direct-list-initialization of 'auto' requires exactly one element"。
//   ★ 因此本章的立場是：這是「條文的版本差異」，不是「你在這台機器上
//     觀察得到的行為差異」。要看到 C++11 的原始行為，需要一個未套用
//     N3922 的老編譯器（例如很舊的 GCC/Clang）。教材中若把它畫成
//     「C++11 欄 = initializer_list、C++17 欄 = int」的對照表，
//     請務必附註「指的是條文，不是本機行為」，否則學生照著編會對不上。
//
// 【6. Most Vexing Parse：{} 真正無可取代的價值】
//   C++ 有一條古老的消歧義規則：「任何『可以』被解析成宣告的東西，
//   就『必須』被解析成宣告。」於是：
//     Widget w();            // 不是物件！是宣告一個回傳 Widget 的函式 w
//     Wrap   x(Timer());     // 不是物件！是宣告函式 x，參數是「函式指標」
//   因為 {} 不能用來寫函式參數列，所以 {} 完全沒有這個歧義：
//     Widget w{};            // 一定是物件
//     Wrap   x{Timer{}};     // 一定是物件
//   這是「能用 {} 就用 {}」這條準則最無爭議的理由。
//
// -----------------------------------------------------------------------------
// 【概念補充 Concept Deep Dive】
// -----------------------------------------------------------------------------
//
// (A) 多載解析到底怎麼把 initializer_list 建構子排到所有人前面？
//     關鍵在於：它 根本不是靠「排名比較高」贏的，而是靠「別人沒被放進候選名單」。
//     標準 [over.match.list] 規定，用 {} 初始化類別型別時分兩個階段：
//
//       階段 1：只把「initializer-list constructors」放進候選集合，
//               並把整個 {…} 當成 單一個引數 傳進去。
//       階段 2：只有在階段 1 找不到任何 viable 候選時，才改用全部建構子，
//               並把 {…} 的元素 拆開當成多個引數。
//
//     這解釋了兩個現象：
//       * 為什麼 Container c{5} 選中 initializer_list<int> 而不是 explicit
//         Container(size_t)：階段 1 就找到可行解了，階段 2 從未執行。
//       * 為什麼「更精確的匹配」救不了你：Converter 同時有 Converter(int) 與
//         Converter(initializer_list<double>)，Converter c{42} 仍走 list 版本，
//         因為 Converter(int) 連候選名單都沒進去，談不上「比較精確」。
//
//     ★ 最惡毒的後果：階段 1 一旦選中，就 不會 因為後續轉換失敗而回頭。
//       本檔 demonstrateHijackHardError() 實測示範：
//         int r = 42;  Converter b{r};
//       階段 1 選中 initializer_list<double>，接著 int → double 因為 r 不是
//       常量表達式而構成 narrowing → 直接 硬性編譯錯誤，
//       而不是「退回去用 Converter(int)」。本機錯誤訊息：
//         error: narrowing conversion of 'r' from 'int' to 'double' [-Wnarrowing]
//       很多人第一次看到會非常困惑：「明明有 Converter(int) 可以用啊？」
//       ——因為它從來沒被考慮過。
//
// (B) 為什麼 narrowing 的定義要繞「常量表達式」這一大圈？
//     直覺上你會想這樣定義：「會失真的轉換就是 narrowing」。
//     但編譯器在編譯期 通常不知道變數的值，無從判斷會不會失真。
//     所以標準 [dcl.init.list] 採用了一個「可判定」的定義，形式是：
//
//       「X → Y 屬於 narrowing，除非 來源是常量表達式 且 其值轉換後
//         仍落在 Y 可表示的範圍（且轉回來還是原值）。」
//
//     拆開看就是兩個條件都要成立才放行：
//       條件 1：編譯期可知（是 constant expression）
//       條件 2：這個具體的值放得下（value is representable）
//
//     於是產生本章最經典的「三胞胎」對比（本機 GCC 15.2 實測）：
//       char a{65};            // ✅ 常量，且 65 在 char 範圍內
//       const int ci = 65;
//       char b{ci};            // ✅ ci 是常量表達式，值 65 放得下
//       int  run = 65;
//       char c{run};           // ❌ error: narrowing conversion of 'run'
//                              //    from 'int' to 'char'
//       char d{300};           // ❌ 是常量，但 300 放不下（本機 CHAR_MAX=127）
//
//     注意 c 這一行：值明明也是 65，也「明明放得下」，卻被拒絕。
//     因為 run 是 runtime 變數，編譯器 不被允許 去猜它的值。
//     這正是「規則要可判定」所付出的代價，也是面試最愛考的點。
//
// (C) std::initializer_list 的記憶體佈局
//     它是個「胖指標」，典型實作只有兩個欄位：
//         const T* __begin_;
//         size_t   __size_;      // 或 const T* __end_
//     本機 sizeof(std::initializer_list<int>) = 16（實作定義值，64-bit 平台）。
//     寫下 {1, 2, 3} 時，編譯器會先產生一個 const int[3] 的 臨時陣列（backing
//     array），再讓 initializer_list 指向它。所以：
//       * 複製 initializer_list 是淺複製，兩份指向同一個底層陣列。
//       * 元素是 const 的 → 無法 move，只能 copy。這就是為什麼
//         std::vector<std::unique_ptr<int>> v{ make_unique<int>(1) }; 編不過。
//       * 底層陣列的生命週期跟著「那個完整運算式」走 → 回傳它 = 懸空。
//
// (D) 那到底該用 {} 還是 ()？本章給的可執行準則
//     1. 預設用 {}（免費得到 narrowing 檢查 + 不會踩 Most Vexing Parse）。
//     2. 「我要指定的是 大小 / 容量 / 重複次數」→ 一律改用 ()。
//        vector, string, deque 這幾個 (count, value) 建構子是主要地雷區。
//     3. 撰寫 template 時，對未知型別 T 優先用 () 或明確的建構語法——
//        因為你不知道 T 有沒有 initializer_list 建構子，{} 的行為會跟著 T 變。
//        （這也是 std::make_shared / emplace_back 內部一律用 () 的原因。）
//
// -----------------------------------------------------------------------------
// 【注意事項 Pay Attention】
// -----------------------------------------------------------------------------
//  1. vector<int> v(10) 與 v{10} 語意完全不同：前者 10 個 0，後者 1 個 10。
//  2. narrowing 的判斷看「是不是常量表達式」，不是看「值實際上放不放得下」。
//     int big = 1000; int x{big}; 會被拒絕，即使 1000 遠在 int 範圍內。
//  3. initializer_list 建構子一旦存在，就會劫持 所有 {} 初始化；
//     而且劫持後不會因為 narrowing 失敗而回退，是硬性錯誤。
//  4. 空的 {} 走預設建構子；要傳空列表得寫 X x({});
//     而 X x{{}} 是「含一個值初始化元素的列表」，不是空列表。
//  5. 不要回傳、也不要用成員變數長期保存 std::initializer_list。
//     底層臨時陣列在完整運算式結束後即失效，之後再讀取是 未定義行為
//     ——結果不保證，可能看似正常、可能讀到垃圾、可能崩潰，不可依賴。
//  6. auto x{1} 在本機（GCC 15.2）所有 -std= 模式都推導成 int（N3922 以
//     缺陷報告方式回溯套用）。教材談「C++11 曾經推導成 initializer_list」時，
//     請標明那是條文沿革，不是本機可觀察到的行為。
//  7. char 是否帶正負號是 實作定義。本機實測 CHAR_MIN=-128、CHAR_MAX=127
//     （即 signed char），因此 char c{200} 在本機是 narrowing 錯誤，
//     但在 char 預設無號的平台（如部分 ARM）則可能合法。
//  8. -Wextra 會對「聚合初始化沒寫滿所有成員」發出
//     -Wmissing-field-initializers 警告。這是刻意的提醒，不是誤報；
//     本檔以 #pragma 局部關閉，並在該處說明原因。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】列表初始化的陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> v{10}; 和 std::vector<int> v(10); 差在哪裡？
//     答：v{10} 呼叫 vector(initializer_list<int>)，得到 1 個元素、值是 10；
//         v(10) 呼叫 vector(size_type count)，得到 10 個元素、值都是 0。
//         原因是 {} 會優先（其實是「只」）考慮 initializer_list 建構子。
//     追問：那 std::vector<std::string> s{3}; 呢？
//         → 這題才是真陷阱：initializer_list<string> 完全不可行（int 轉不出
//           string），階段 1 找不到候選，於是退回階段 2，選中
//           vector(size_type)，得到 3 個空字串。本機實測 s.size() == 3。
//           也就是說「{} 一定走 list 版本」是錯的，正確說法是
//           「list 版本可行時才走它，完全不可行時會退回一般建構子」。
//
// 🔥 Q2. 為什麼有 initializer_list 建構子時，其他建構子好像「永遠選不到」？
//     答：不是選不到，是根本沒被列入候選。[over.match.list] 規定用 {} 初始化
//         時分兩階段：階段 1 只放 initializer-list 建構子；只有階段 1 完全
//         沒有 viable 候選，才進入階段 2 考慮全部建構子。
//     追問：那我一定要呼叫 (int, int) 那個版本怎麼辦？
//         → 改用小括號 ForceConstruct f(5, 10);。這是唯一乾淨的做法。
//
// ⚠️ 陷阱 Q3. 下面兩行都是把 65 塞進 char，為什麼一行過、一行不過？
//         char a{65};                 // OK
//         int r = 65;  char b{r};     // 編譯錯誤
//     答：narrowing 的判定條件是「來源是常量表達式 且 值放得下」，兩個條件
//         都要成立。65 是常量表達式且放得下 → 放行；r 是 runtime 變數，
//         編譯器不被允許推測它的值 → 一律視為 narrowing 而拒絕。
//     為什麼會錯：多數人腦中的模型是「值放不放得下」，
//         所以覺得 r 的值就是 65、當然放得下、當然該過。
//         但標準要的是一條 編譯期可判定 的規則——編譯器看的是「型別 +
//         是否為常量表達式」，不是「這個變數此刻的值」。
//         順帶一提 char d{300}; 也不過：它是常量，但本機 CHAR_MAX=127 放不下。
//
// ⚠️ 陷阱 Q4. 類別有 Converter(int) 也有 Converter(initializer_list<double>)。
//         int r = 42;  Converter c{r};  會發生什麼？
//     答：編譯錯誤（本機：narrowing conversion of 'r' from 'int' to 'double'）。
//         階段 1 選中了 initializer_list<double>，接著要把 r 轉成 double，
//         而 r 不是常量表達式 → 構成 narrowing → 硬性錯誤。
//     為什麼會錯：直覺會以為「list 版本用不了，那就自動退回 Converter(int)」。
//         但兩階段規則的「退回」只在 階段 1 找不到 viable 候選 時發生；
//         這裡階段 1 明明找到了，只是後續轉換違規，於是直接報錯、不回頭。
//         （若寫成 Converter c{42}，42 是常量且能精確轉成 double，反而會過，
//           呼叫的是 list 版本——同樣不是你以為的 Converter(int)。）
//
// ⚠️ 陷阱 Q5. Widget w(); 為什麼沒有印出建構子訊息？{} 為什麼能解決？
//     答：Widget w(); 被解析成「宣告一個不吃參數、回傳 Widget 的函式 w」，
//         根本沒有建立物件，這就是 Most Vexing Parse。原因是 C++ 的消歧義
//         規則：能解析成宣告的就必須解析成宣告。改寫成 Widget w{}; 即可，
//         因為 {} 不能出現在函式參數列，語法上沒有第二種解讀。
//     追問：Wrap x(Timer()); 呢？
//         → 一樣是函式宣告：x 是函式，參數型別是「回傳 Timer 的函式指標」。
//           本機 GCC 會發出 -Wvexing-parse 警告，且 static_assert(
//           std::is_function<decltype(x)>::value) 會成立——可以在編譯期
//           證明它真的是函式。修正寫法是 Wrap x{Timer{}};。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <initializer_list>
#include <type_traits>
#include <map>
#include <array>
#include <climits>
#include <cstdint>

// ===== 陷阱 1：initializer_list 優先順序 =====

class Container
{
public:
    // 建構子 1：指定大小
    explicit Container(std::size_t size)
    {
        std::cout << "  Container(size_t size=" << size << ")\n";
    }
    
    // 建構子 2：指定大小和初始值
    Container(std::size_t size, int value)
    {
        std::cout << "  Container(size_t size=" << size 
                  << ", int value=" << value << ")\n";
    }
    
    // 建構子 3：initializer_list
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

// ===== 陷阱 2：窄化轉換的各種情況 =====

void demonstrateNarrowing()
{
    std::cout << "===== 陷阱 2：窄化轉換 =====\n";
    
    // --- 2.1 浮點數到整數 ---
    std::cout << "\n--- 2.1 浮點數到整數 ---\n";
    
    double d = 3.14;

    int a(d);       // 合法，a = 3
    int b = d;      // 合法，b = 3
    // int c{d};    // 編譯錯誤！窄化轉換
    // int e = {d}; // 編譯錯誤！窄化轉換

    // 常量表達式可以（如果值在範圍內且無小數）
    int f{3};     // 合法，3 可以精確表示為 int
    // int g{3.14}; // 錯誤！有小數部分

    std::cout << "int a(3.14) = " << a << " (允許，但截斷)\n";
    std::cout << "int b = 3.14 = " << b << " (允許，= 也不擋窄化)\n";
    std::cout << "int f{3} = " << f << " (允許，整數常量無損失)\n";
    std::cout << "int c{3.14}; // 編譯錯誤！\n";

    // --- 2.2 大整數到小整數 ---
    std::cout << "\n--- 2.2 大整數到小整數 ---\n";

    long long big = 1000;

    int i1(big);     // 合法
    int i2 = big;    // 合法
    // int i3{big};  // 編譯錯誤！即使值實際上放得下

    // 常量表達式可以（如果值在範圍內）
    int i4{100LL};   // 合法，100 在 int 範圍內
    // int i5{10'000'000'000LL};  // 錯誤！超出 int 範圍

    std::cout << "int i1(big) = " << i1 << " (允許)\n";
    std::cout << "int i2 = big = " << i2 << " (允許)\n";
    std::cout << "int i4{100LL} = " << i4 << " (常量在範圍內，允許)\n";
    std::cout << "int i3{big}; // 編譯錯誤，即使 big=1000 放得下！\n";

    // --- 2.3 有號與無號之間 ---
    std::cout << "\n--- 2.3 有號與無號轉換 ---\n";

    int negative = -1;

    unsigned u1(negative);    // 合法，但結果很大
    unsigned u2 = negative;   // 合法，但結果很大
    // unsigned u3{negative}; // 編譯錯誤！

    unsigned u4{0};           // 合法
    // unsigned u5{-1};       // 錯誤！負數常量

    std::cout << "unsigned u1(-1) = " << u1 << " (允許，但危險)\n";
    std::cout << "unsigned u2 = -1 = " << u2 << " (允許，但危險)\n";
    std::cout << "unsigned u4{0}  = " << u4 << " (常量 0，合法)\n";
    std::cout << "unsigned u3{negative}; // 編譯錯誤！\n";

    // --- 2.4 指標相關 ---
    std::cout << "\n--- 2.4 指標到 bool ---\n";

    int* ptr = nullptr;

    bool b1(ptr);    // 合法
    bool b2 = ptr;   // 合法
    // bool b3{ptr}; // 編譯錯誤！C++11 禁止指標到 bool 的窄化

    std::cout << "bool b1(nullptr) = " << std::boolalpha << b1 << " (允許)\n";
    std::cout << "bool b2 = nullptr = " << b2 << " (允許)\n";
    std::cout << "bool b3{ptr}; // 編譯錯誤！\n";
}

// ===== 陷阱 2 補充：narrowing 的真正判準是「常量表達式」而非「值放得下」 =====
//
// 這是本章最容易答錯、也最常被面試官拿來篩人的一組對比。
// 標準 [dcl.init.list] 的條件是「來源是常量表達式 且 值可表示」，兩者缺一不可。
// 下面四行只有前兩行能編譯，第三、四行都是編譯錯誤 —— 注意 c 那行的值
// 明明也是 65、明明也放得下，卻依然被拒絕。
//
//     char a{65};              // ✅ 常量 65，且在 char 範圍內
//     const int ci = 65;
//     char b{ci};              // ✅ ci 是常量表達式，值 65 放得下
//     int run = 65;
//     char c{run};             // ❌ error: narrowing conversion of 'run'
//                              //           from 'int' to 'char' [-Wnarrowing]
//     char d{300};             // ❌ 是常量，但 300 > CHAR_MAX（本機 127）
//
// 上述 c、d 兩行以註解保留（放進程式碼會使本檔無法編譯），
// 錯誤訊息為 GCC 15.2.0 本機實測。

void demonstrateNarrowingConstantRule()
{
    std::cout << "\n--- 2.5 判準是「常量表達式」，不是「值放得下」 ---\n";

    // 本機 char 的正負號屬於「實作定義」，先把實測值印出來當佐證
    std::cout << "本機 CHAR_MIN=" << CHAR_MIN << " CHAR_MAX=" << CHAR_MAX
              << "（實作定義；本機 char 為 signed）\n";

    char a{65};              // ✅ 字面常量，放得下
    const int ci = 65;
    char b{ci};              // ✅ const int 是常量表達式，值放得下
    constexpr int ce = 66;
    char c{ce};              // ✅ constexpr 更明確

    int run = 65;            // runtime 變數
    char viaParen(run);      // ✅ () 不做 narrowing 檢查
    // char viaBrace{run};   // ❌ 編譯錯誤：即使值就是 65！

    std::cout << "char a{65}      = '" << a << "'  ✅ 常量且放得下\n";
    std::cout << "char b{ci}      = '" << b << "'  ✅ const int 是常量表達式\n";
    std::cout << "char c{ce}      = '" << c << "'  ✅ constexpr 常量\n";
    std::cout << "char x(run)     = '" << viaParen << "'  ✅ () 一律放行\n";
    std::cout << "char x{run};    ← ❌ 編譯錯誤，因為 run 不是常量表達式\n";
    std::cout << "char d{300};    ← ❌ 編譯錯誤，是常量但超出 CHAR_MAX\n";
    std::cout << "\n重點：編譯器看的是「型別 + 是否為常量表達式」，\n";
    std::cout << "      而不是「這個變數此刻剛好等於多少」。\n";
}

// ===== 陷阱 3：auto 與大括號的陷阱 =====

// ===== 陷阱 3：auto 與大括號（N3922，需要誠實交代版本問題）=====
//
// ★ 教學上必須講清楚的一點：
//   「C++11/14 推導成 initializer_list、C++17 改成 int」這句話描述的是
//   標準條文的沿革，而不是你在現代編譯器上觀察得到的行為。
//   N3922 是以 Defect Report（缺陷報告）形式通過的，意思是「原條文有瑕疵」，
//   因此各家編譯器普遍把它 回溯套用 到舊的 -std= 模式。
//
//   本機實測（GCC 15.2.0，四種模式皆加 -pedantic-errors）：
//     -std=c++11 : auto c{42} → int ；auto d{1,2} → 編譯錯誤
//     -std=c++14 : auto c{42} → int ；auto d{1,2} → 編譯錯誤
//     -std=c++17 : auto c{42} → int ；auto d{1,2} → 編譯錯誤
//     -std=c++20 : auto c{42} → int ；auto d{1,2} → 編譯錯誤
//   錯誤訊息一致為：
//     error: direct-list-initialization of 'auto' requires exactly one element
//
//   所以：本檔的對照表分成「條文怎麼寫」與「本機怎麼跑」兩欄，
//   不會宣稱一個在本機無法重現的版本差異。
//   若要親眼看到 C++11 的原始行為，需要一個尚未套用 N3922 的舊編譯器。

void demonstrateAutoBrace()
{
    std::cout << "\n===== 陷阱 3：auto 與大括號（N3922）=====\n";

    // --- copy-list-initialization：auto x = {...} ---
    std::cout << "\n--- 使用 auto = {...}（copy-list-init）---\n";

    auto a = {1, 2, 3};   // std::initializer_list<int>
    auto b = {42};        // std::initializer_list<int>，即使只有一個元素！

    std::cout << "auto a = {1, 2, 3};\n";
    std::cout << "  是 initializer_list<int>? "
              << std::is_same<decltype(a), std::initializer_list<int>>::value << "\n";
    std::cout << "  size: " << a.size() << "\n";

    std::cout << "auto b = {42};\n";
    std::cout << "  是 initializer_list<int>? "
              << std::is_same<decltype(b), std::initializer_list<int>>::value << "\n";
    std::cout << "  size: " << b.size() << "  ← 只有一個元素也還是 list！\n";

    // --- direct-list-initialization：auto x{...} ---
    std::cout << "\n--- 使用 auto x{value}（direct-list-init）---\n";

    auto c{42};       // N3922 之後：int
    // auto d{1, 2};  // 編譯錯誤：direct-list-init 只允許恰好一個元素

    std::cout << "auto c{42};\n";
    std::cout << "  是 int?                   "
              << std::is_same<decltype(c), int>::value << "\n";
    std::cout << "  是 initializer_list<int>? "
              << std::is_same<decltype(c), std::initializer_list<int>>::value << "\n";
    std::cout << "  value: " << c << "\n";
    std::cout << "auto d{1, 2};  ← 編譯錯誤（本機四種 -std= 模式皆然）\n";

    // --- 對比表：條文 vs 本機 ---
    std::cout << "\n--- 對比表（區分「條文」與「本機實測」）---\n";
    std::cout << "語法             C++11 原始條文          N3922 之後      本機 GCC 15.2\n";
    std::cout << "auto x = {1,2,3} initializer_list<int>   同左            initializer_list<int>\n";
    std::cout << "auto x = {42}    initializer_list<int>   同左            initializer_list<int>\n";
    std::cout << "auto x{42}       initializer_list<int>   int             int（所有 -std=）\n";
    std::cout << "auto x{1,2,3}    initializer_list<int>   ill-formed      編譯錯誤（所有 -std=）\n";
    std::cout << "\n注意：最後兩列的「C++11 原始條文」欄無法在本機重現，\n";
    std::cout << "      因為 N3922 是缺陷報告，GCC 已回溯套用到 -std=c++11。\n";

    // 記憶法：等號 = 才會生出 list；沒有等號的 {} 是「一個值」
    std::cout << "\n記憶法：有『= 』才生 initializer_list；auto x{v} 就是 v 的型別。\n";
}

// ===== 陷阱 4：空大括號的歧義 =====

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

void demonstrateEmptyBraces()
{
    std::cout << "\n===== 陷阱 4：空大括號的歧義 =====\n";
    
    std::cout << "\n--- Widget（有 initializer_list 建構子）---\n";
    
    std::cout << "Widget w1;\n";
    Widget w1;        // 預設建構子
    
    std::cout << "Widget w2{};\n";
    Widget w2{};      // 預設建構子（空大括號優先匹配預設建構子）
    
    std::cout << "Widget w3({});\n";
    Widget w3({});    // initializer_list 建構子（空列表）
    
    std::cout << "Widget w4{{}};\n";
    Widget w4{{}};    // initializer_list 建構子（包含一個預設初始化的 int）
    
    std::cout << "\n--- Gadget（無 initializer_list 建構子）---\n";
    
    std::cout << "Gadget g1;\n";
    Gadget g1;
    
    std::cout << "Gadget g2{};\n";
    Gadget g2{};
    
    std::cout << "\n--- std::vector 的情況 ---\n";
    
    std::vector<int> v1;      // 空 vector
    std::vector<int> v2{};    // 空 vector
    std::vector<int> v3({});  // 空 vector（透過空 initializer_list）
    std::vector<int> v4{{}};  // 包含一個元素 0！
    
    std::cout << "vector<int> v1;     size = " << v1.size() << "\n";
    std::cout << "vector<int> v2{};   size = " << v2.size() << "\n";
    std::cout << "vector<int> v3({}); size = " << v3.size() << "\n";
    std::cout << "vector<int> v4{{}}; size = " << v4.size() 
              << " (元素: " << v4[0] << ")\n";
}

// ===== 陷阱 5：std::vector 的著名陷阱 =====

void demonstrateVectorPitfall()
{
    std::cout << "\n===== 陷阱 5：std::vector 的著名陷阱 =====\n";
    
    std::cout << "\n--- int 版本 ---\n";
    
    std::vector<int> v1(5);       // 5 個元素，都是 0
    std::vector<int> v2{5};       // 1 個元素，值是 5
    std::vector<int> v3(5, 10);   // 5 個元素，都是 10
    std::vector<int> v4{5, 10};   // 2 個元素：5 和 10
    
    auto printVec = [](const std::string& name, const std::vector<int>& v) {
        std::cout << name << " (size=" << v.size() << "): {";
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << v[i];
        }
        std::cout << "}\n";
    };
    
    printVec("v1(5)    ", v1);
    printVec("v2{5}    ", v2);
    printVec("v3(5, 10)", v3);
    printVec("v4{5, 10}", v4);
    
    std::cout << "\n--- string 版本（★ 這裡藏著「退回階段 2」的關鍵證據）---\n";

    std::vector<std::string> s1(3);           // 3 個空字串
    // ★ 重要更正：s2{3} 並不是編譯錯誤！
    //   initializer_list<string> 完全不可行（int 轉不出 string），
    //   階段 1 找不到任何 viable 候選 → 退回階段 2 → 選中 vector(size_type)。
    //   所以它等價於 s1(3)：3 個空字串。本機實測 s2.size() == 3。
    //   （vector(size_type) 是 explicit，但 direct-list-init 允許呼叫
    //     explicit 建構子，所以這裡沒有障礙。）
    std::vector<std::string> s2{3};           // ✅ 可編譯：3 個空字串！
    std::vector<std::string> s3(3, "hello");  // 3 個 "hello"
    std::vector<std::string> s4{"a", "b"};    // 2 個元素

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
    printStrVec("s2{3}         ", s2);   // ★ 真的可以編譯，而且等於 s1(3)
    printStrVec("s3(3, \"hello\")", s3);
    printStrVec("s4{\"a\", \"b\"} ", s4);

    std::cout << "\n★ 對照組結論（本章最重要的一張表）：\n";
    std::cout << "  vector<int>    v{3}  → 1 個元素 3   （list 版本可行 → 階段 1 勝出）\n";
    std::cout << "  vector<string> s{3}  → 3 個空字串   （list 版本不可行 → 退回階段 2）\n";
    std::cout << "  所以「{} 一定走 initializer_list」是錯的說法；\n";
    std::cout << "  正確說法是「list 版本可行時才走它，完全不可行時退回一般建構子」。\n";
}

// ===== 陷阱 6：建構子選擇的微妙差異 =====

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

void demonstrateConstructorSelection()
{
    std::cout << "\n===== 陷阱 6：建構子選擇的微妙差異 =====\n";
    
    std::cout << "\nConverter c1(42);\n";
    Converter c1(42);       // Converter(int)
    
    std::cout << "Converter c2{42};\n";
    Converter c2{42};       // initializer_list<double>！
                            // 42 可以轉換為 double
    
    std::cout << "Converter c3(3.14);\n";
    Converter c3(3.14);     // Converter(double)
    
    std::cout << "Converter c4{3.14};\n";
    Converter c4{3.14};     // initializer_list<double>
    
    std::cout << "\n--- 即使轉換不完美，也優先匹配 initializer_list ---\n";
    
    std::cout << "當 {} 內的型別可以轉換為 initializer_list 的元素型別時，\n";
    std::cout << "即使存在更精確匹配的建構子，也會優先選擇 initializer_list 版本。\n";
}

// ===== 陷阱 7：強制呼叫非 initializer_list 建構子 =====

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

void demonstrateForceConstruct()
{
    std::cout << "\n===== 陷阱 7：如何強制呼叫非 initializer_list 建構子 =====\n";
    
    std::cout << "\n--- 問題：我想呼叫 (int, int) 版本 ---\n";
    
    std::cout << "ForceConstruct f1{5, 10};\n";
    ForceConstruct f1{5, 10};   // initializer_list 版本
    
    std::cout << "\n--- 解決方案 1：使用小括號 ---\n";
    
    std::cout << "ForceConstruct f2(5, 10);\n";
    ForceConstruct f2(5, 10);   // (int, int) 版本
    
    std::cout << "\n--- 解決方案 2：明確型別轉換 ---\n";
    
    // 如果必須使用 {}，可以讓元素無法轉換為 initializer_list 的元素型別
    // 但這通常不實用
    
    std::cout << "\n--- 結論 ---\n";
    std::cout << "當類別有 initializer_list 建構子時：\n";
    std::cout << "  使用 {} → 優先匹配 initializer_list\n";
    std::cout << "  使用 () → 匹配最佳的非 initializer_list 建構子\n";
}

// ===== 陷阱 8：聚合類別的特殊規則 =====

struct Aggregate
{
    int x;
    int y;
    int z;
};

struct NonAggregate
{
    int x;
    int y;
    int z;
    
    NonAggregate() : x{0}, y{0}, z{0} {}  // 有使用者定義的建構子
};

void demonstrateAggregate()
{
    std::cout << "\n===== 陷阱 8：聚合類別的特殊規則 =====\n";
    
    std::cout << "\n--- 聚合類別 Aggregate ---\n";

    Aggregate a1{1, 2, 3};      // 聚合初始化
    // ★ 下面兩行「沒寫滿成員」，-Wextra 會發出 -Wmissing-field-initializers。
    //   這不是誤報：GCC 是刻意提醒你「你可能忘了寫」。但本例正是要示範
    //   「未提供的成員會被值初始化為 0」，所以在此局部關閉該警告，
    //   讓本檔在 -Wall -Wextra 下維持零警告。
    //   實務上請把這個警告當成好事，別全域關掉。
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    Aggregate a2{1, 2};         // z = 0
    Aggregate a3{1};            // y = 0, z = 0
#pragma GCC diagnostic pop
    Aggregate a4{};             // 全部 = 0

    std::cout << "Aggregate a1{1, 2, 3}: x=" << a1.x 
              << ", y=" << a1.y << ", z=" << a1.z << "\n";
    std::cout << "Aggregate a2{1, 2}:    x=" << a2.x 
              << ", y=" << a2.y << ", z=" << a2.z << "\n";
    std::cout << "Aggregate a3{1}:       x=" << a3.x 
              << ", y=" << a3.y << ", z=" << a3.z << "\n";
    std::cout << "Aggregate a4{}:        x=" << a4.x 
              << ", y=" << a4.y << ", z=" << a4.z << "\n";
    
    std::cout << "\n--- 非聚合類別 NonAggregate ---\n";
    
    // NonAggregate n1{1, 2, 3};  // 編譯錯誤！不是聚合類別
    NonAggregate n2{};            // 呼叫預設建構子
    
    std::cout << "NonAggregate n2{}: 呼叫預設建構子\n";
    std::cout << "NonAggregate n1{1,2,3}; // 編譯錯誤！\n";
    
    std::cout << "\n--- 聚合類別的條件 (C++11/14) ---\n";
    std::cout << "1. 沒有使用者宣告的建構子\n";
    std::cout << "2. 沒有私有或保護的非靜態成員\n";
    std::cout << "3. 沒有虛擬函式\n";
    std::cout << "4. 沒有虛擬、私有、保護的基礎類別\n";
}

// ===== 陷阱 9：initializer_list 的生命週期 =====

std::initializer_list<int> dangerousReturn()
{
    return {1, 2, 3};  // 危險！底層陣列是臨時的
}

void demonstrateLifetime()
{
    std::cout << "\n===== 陷阱 9：initializer_list 的生命週期 =====\n";
    
    std::cout << "\n--- 安全的使用方式 ---\n";
    
    // 安全：立即使用
    std::vector<int> v{1, 2, 3};  // 建構子會複製元素
    std::cout << "std::vector<int> v{1, 2, 3}; // 安全：元素被複製\n";
    
    // 安全：在同一表達式中
    for (int x : {1, 2, 3})
    {
        // 底層陣列在整個 for 迴圈期間存活
    }
    std::cout << "for (int x : {1, 2, 3}) { ... } // 安全\n";
    
    std::cout << "\n--- 危險的使用方式 ---\n";
    
    std::cout << "std::initializer_list<int> dangerousReturn() {\n";
    std::cout << "    return {1, 2, 3};  // 危險！\n";
    std::cout << "}\n";
    std::cout << "底層陣列在函式返回後已銷毀，\n";
    std::cout << "使用回傳的 initializer_list 是未定義行為！\n";
    
    // auto list = dangerousReturn();  // 未定義行為！
    
    std::cout << "\n--- 危險：作為成員儲存 ---\n";
    
    std::cout << "class Holder {\n";
    std::cout << "    std::initializer_list<int> data_;  // 危險！\n";
    std::cout << "};\n";
    std::cout << "建構子結束後，底層陣列可能已銷毀\n";
    
    std::cout << "\n--- 正確做法：複製到容器 ---\n";
    
    std::cout << "class SafeHolder {\n";
    std::cout << "    std::vector<int> data_;\n";
    std::cout << "    SafeHolder(std::initializer_list<int> init)\n";
    std::cout << "        : data_(init) {}  // 複製元素，安全\n";
    std::cout << "};\n";
}

// ===== 陷阱 10：嵌套大括號的解析 =====

void demonstrateNestedBraces()
{
    std::cout << "\n===== 陷阱 10：嵌套大括號的解析 =====\n";
    
    std::cout << "\n--- std::pair 的初始化 ---\n";
    
    std::pair<int, int> p1(1, 2);      // 傳統方式
    std::pair<int, int> p2{1, 2};      // C++11
    std::pair<int, int> p3 = {1, 2};   // C++11
    
    std::cout << "pair<int,int> p1(1, 2):   " << p1.first << ", " << p1.second << "\n";
    std::cout << "pair<int,int> p2{1, 2}:   " << p2.first << ", " << p2.second << "\n";
    std::cout << "pair<int,int> p3 = {1,2}: " << p3.first << ", " << p3.second << "\n";
    
    std::cout << "\n--- std::map 的初始化 ---\n";
    
    std::map<std::string, int> m1{
        {"one", 1},
        {"two", 2},
        {"three", 3}
    };
    
    std::cout << "map initialized with nested braces:\n";
    for (const auto& [key, value] : m1)
    {
        std::cout << "  " << key << " -> " << value << "\n";
    }
    
    std::cout << "\n--- vector<vector<int>> ---\n";
    
    std::vector<std::vector<int>> matrix{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
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
}

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 陷阱 1：initializer_list 優先順序 =====
    std::cout << "===== 陷阱 1：initializer_list 優先順序 =====\n\n";
    
    std::cout << "Container c1(5);\n";
    Container c1(5);
    
    std::cout << "\nContainer c2{5};\n";
    Container c2{5};        // initializer_list！不是 size
    
    std::cout << "\nContainer c3(5, 10);\n";
    Container c3(5, 10);    // size, value
    
    std::cout << "\nContainer c4{5, 10};\n";
    Container c4{5, 10};    // initializer_list！
    
    std::cout << "\nContainer c5({5, 10});\n";
    Container c5({5, 10});  // 明確使用 initializer_list
    
    // 陷阱 2
    demonstrateNarrowing();
    
    // 陷阱 3
    demonstrateAutoBrace();
    
    // 陷阱 4
    demonstrateEmptyBraces();
    
    // 陷阱 5
    demonstrateVectorPitfall();
    
    // 陷阱 6
    demonstrateConstructorSelection();
    
    // 陷阱 7
    demonstrateForceConstruct();
    
    // 陷阱 8
    demonstrateAggregate();
    
    // 陷阱 9
    demonstrateLifetime();
    
    // 陷阱 10
    demonstrateNestedBraces();
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 1.6 章：列表初始化的陷阱 — Narrowing Conversion 與優先順序問題1.cpp" -o narrowing1
//
// 版本註記（本機以 -pedantic-errors 逐一實測，g++ 15.2.0）：
//   本檔【必須】用 -std=c++17：第 855 行的 structured bindings（auto [a, b] = ...）
//   是 C++17 新增。以 -std=c++11 -pedantic-errors 編譯會明確報錯：
//     error: structured bindings only available with ‘-std=c++17’
//   注意：若只加 -Wall（不加 -pedantic-errors），GCC 會把它當【擴充】
//   放行、只給一個 -Wc++17-extensions 警告就編過 ——
//   看起來像「C++11 也支援」，其實不是。驗證標準版本一定要用
//   -pedantic-errors，不能只看編不編得過。

// 註 1:編譯時會出現 2 個警告，皆為【刻意保留】的教材內容：
//        • -Winit-list-lifetime（第 784 行 dangerousReturn()）
//          → 這個函式回傳一個指向已消滅暫存陣列的 initializer_list，
//            正是本檔要示範的懸空陷阱。它【只定義、不呼叫】
//            （第 812 行的呼叫刻意保持註解狀態），
//            所以執行時【不會】觸發那個 UB。
//        • -Wunused-variable（x）→ 只為展示宣告語法。

// 註 2:本檔輸出是【完全確定的】，連跑 5 次位元組完全相同。
//      沒有亂數、執行緒、位址或耗時。

// 註 3:輸出中多處寫著「編譯錯誤！」的行（如 int c{3.14};、
//      unsigned u3{negative};、auto d{1, 2};）並不是程式在執行期偵測到錯誤，
//      而是那些程式碼被【註解掉】、由 cout 印出說明文字。
//      窄化轉換是在【編譯期】被擋下來的 —— 想親眼看到錯誤訊息，
//      請把對應的註解取消再編一次。

// 註 4:陷阱 2.3 的 4294967295 = 2^32 − 1，是 unsigned int 為 32 bit 時的結果，
//      屬【實作定義】的型別寬度；-1 轉 unsigned 本身則是標準明定的取模運算，
//      不是 UB。

// === 預期輸出 ===
// ===== 陷阱 1：initializer_list 優先順序 =====
//
// Container c1(5);
//   Container(size_t size=5)
//
// Container c2{5};
//   Container(initializer_list<int>, size=1, values={5})
//
// Container c3(5, 10);
//   Container(size_t size=5, int value=10)
//
// Container c4{5, 10};
//   Container(initializer_list<int>, size=2, values={5, 10})
//
// Container c5({5, 10});
//   Container(initializer_list<int>, size=2, values={5, 10})
// ===== 陷阱 2：窄化轉換 =====
//
// --- 2.1 浮點數到整數 ---
// int a(3.14) = 3 (允許，但截斷)
// int b = 3.14 = 3 (允許，= 也不擋窄化)
// int f{3} = 3 (允許，整數常量無損失)
// int c{3.14}; // 編譯錯誤！
//
// --- 2.2 大整數到小整數 ---
// int i1(big) = 1000 (允許)
// int i2 = big = 1000 (允許)
// int i4{100LL} = 100 (常量在範圍內，允許)
// int i3{big}; // 編譯錯誤，即使 big=1000 放得下！
//
// --- 2.3 有號與無號轉換 ---
// unsigned u1(-1) = 4294967295 (允許，但危險)
// unsigned u2 = -1 = 4294967295 (允許，但危險)
// unsigned u4{0}  = 0 (常量 0，合法)
// unsigned u3{negative}; // 編譯錯誤！
//
// --- 2.4 指標到 bool ---
// bool b1(nullptr) = false (允許)
// bool b2 = nullptr = false (允許)
// bool b3{ptr}; // 編譯錯誤！
//
// ===== 陷阱 3：auto 與大括號（N3922）=====
//
// --- 使用 auto = {...}（copy-list-init）---
// auto a = {1, 2, 3};
//   是 initializer_list<int>? true
//   size: 3
// auto b = {42};
//   是 initializer_list<int>? true
//   size: 1  ← 只有一個元素也還是 list！
//
// --- 使用 auto x{value}（direct-list-init）---
// auto c{42};
//   是 int?                   true
//   是 initializer_list<int>? false
//   value: 42
// auto d{1, 2};  ← 編譯錯誤（本機四種 -std= 模式皆然）
//
// --- 對比表（區分「條文」與「本機實測」）---
// 語法             C++11 原始條文          N3922 之後      本機 GCC 15.2
// auto x = {1,2,3} initializer_list<int>   同左            initializer_list<int>
// auto x = {42}    initializer_list<int>   同左            initializer_list<int>
// auto x{42}       initializer_list<int>   int             int（所有 -std=）
// auto x{1,2,3}    initializer_list<int>   ill-formed      編譯錯誤（所有 -std=）
//
// 注意：最後兩列的「C++11 原始條文」欄無法在本機重現，
//       因為 N3922 是缺陷報告，GCC 已回溯套用到 -std=c++11。
//
// 記憶法：有『= 』才生 initializer_list；auto x{v} 就是 v 的型別。
//
// ===== 陷阱 4：空大括號的歧義 =====
//
// --- Widget（有 initializer_list 建構子）---
// Widget w1;
//   Widget() 預設建構子
// Widget w2{};
//   Widget() 預設建構子
// Widget w3({});
//   Widget(initializer_list<int>, size=0)
// Widget w4{{}};
//   Widget(initializer_list<int>, size=1)
//
// --- Gadget（無 initializer_list 建構子）---
// Gadget g1;
//   Gadget() 預設建構子
// Gadget g2{};
//   Gadget() 預設建構子
//
// --- std::vector 的情況 ---
// vector<int> v1;     size = 0
// vector<int> v2{};   size = 0
// vector<int> v3({}); size = 0
// vector<int> v4{{}}; size = 1 (元素: 0)
//
// ===== 陷阱 5：std::vector 的著名陷阱 =====
//
// --- int 版本 ---
// v1(5)     (size=5): {0, 0, 0, 0, 0}
// v2{5}     (size=1): {5}
// v3(5, 10) (size=5): {10, 10, 10, 10, 10}
// v4{5, 10} (size=2): {5, 10}
//
// --- string 版本（★ 這裡藏著「退回階段 2」的關鍵證據）---
// s1(3)          (size=3): {"", "", ""}
// s2{3}          (size=3): {"", "", ""}
// s3(3, "hello") (size=3): {"hello", "hello", "hello"}
// s4{"a", "b"}  (size=2): {"a", "b"}
//
// ★ 對照組結論（本章最重要的一張表）：
//   vector<int>    v{3}  → 1 個元素 3   （list 版本可行 → 階段 1 勝出）
//   vector<string> s{3}  → 3 個空字串   （list 版本不可行 → 退回階段 2）
//   所以「{} 一定走 initializer_list」是錯的說法；
//   正確說法是「list 版本可行時才走它，完全不可行時退回一般建構子」。
//
// ===== 陷阱 6：建構子選擇的微妙差異 =====
//
// Converter c1(42);
//   Converter(int 42)
// Converter c2{42};
//   Converter(initializer_list<double>, size=1)
// Converter c3(3.14);
//   Converter(double 3.14)
// Converter c4{3.14};
//   Converter(initializer_list<double>, size=1)
//
// --- 即使轉換不完美，也優先匹配 initializer_list ---
// 當 {} 內的型別可以轉換為 initializer_list 的元素型別時，
// 即使存在更精確匹配的建構子，也會優先選擇 initializer_list 版本。
//
// ===== 陷阱 7：如何強制呼叫非 initializer_list 建構子 =====
//
// --- 問題：我想呼叫 (int, int) 版本 ---
// ForceConstruct f1{5, 10};
//   ForceConstruct(initializer_list, size=2)
//
// --- 解決方案 1：使用小括號 ---
// ForceConstruct f2(5, 10);
//   ForceConstruct(int 5, int 10)
//
// --- 解決方案 2：明確型別轉換 ---
//
// --- 結論 ---
// 當類別有 initializer_list 建構子時：
//   使用 {} → 優先匹配 initializer_list
//   使用 () → 匹配最佳的非 initializer_list 建構子
//
// ===== 陷阱 8：聚合類別的特殊規則 =====
//
// --- 聚合類別 Aggregate ---
// Aggregate a1{1, 2, 3}: x=1, y=2, z=3
// Aggregate a2{1, 2}:    x=1, y=2, z=0
// Aggregate a3{1}:       x=1, y=0, z=0
// Aggregate a4{}:        x=0, y=0, z=0
//
// --- 非聚合類別 NonAggregate ---
// NonAggregate n2{}: 呼叫預設建構子
// NonAggregate n1{1,2,3}; // 編譯錯誤！
//
// --- 聚合類別的條件 (C++11/14) ---
// 1. 沒有使用者宣告的建構子
// 2. 沒有私有或保護的非靜態成員
// 3. 沒有虛擬函式
// 4. 沒有虛擬、私有、保護的基礎類別
//
// ===== 陷阱 9：initializer_list 的生命週期 =====
//
// --- 安全的使用方式 ---
// std::vector<int> v{1, 2, 3}; // 安全：元素被複製
// for (int x : {1, 2, 3}) { ... } // 安全
//
// --- 危險的使用方式 ---
// std::initializer_list<int> dangerousReturn() {
//     return {1, 2, 3};  // 危險！
// }
// 底層陣列在函式返回後已銷毀，
// 使用回傳的 initializer_list 是未定義行為！
//
// --- 危險：作為成員儲存 ---
// class Holder {
//     std::initializer_list<int> data_;  // 危險！
// };
// 建構子結束後，底層陣列可能已銷毀
//
// --- 正確做法：複製到容器 ---
// class SafeHolder {
//     std::vector<int> data_;
//     SafeHolder(std::initializer_list<int> init)
//         : data_(init) {}  // 複製元素，安全
// };
//
// ===== 陷阱 10：嵌套大括號的解析 =====
//
// --- std::pair 的初始化 ---
// pair<int,int> p1(1, 2):   1, 2
// pair<int,int> p2{1, 2}:   1, 2
// pair<int,int> p3 = {1,2}: 1, 2
//
// --- std::map 的初始化 ---
// map initialized with nested braces:
//   one -> 1
//   three -> 3
//   two -> 2
//
// --- vector<vector<int>> ---
// matrix:
//   1 2 3
//   4 5 6
//   7 8 9
