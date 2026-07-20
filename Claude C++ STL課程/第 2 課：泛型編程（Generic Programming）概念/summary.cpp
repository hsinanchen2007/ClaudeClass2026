// =============================================================================
//  summary.cpp  —  第二課：泛型編程（Generic Programming）概念．總複習
// =============================================================================
//
// 【主題資訊 Information】
//
//   本檔是第二課的教科書總覽，涵蓋八大重點：
//     1. 為什麼需要泛型編程（從重複程式碼談起）
//     2. 函數重載（Function Overloading）的局限性
//     3. 函數模板（Function Template）的語法與使用
//     4. 類別模板（Class Template）的語法與使用
//     5. 模板特化（Template Specialization）
//     6. 模板中的型別推導（Type Deduction）
//     7. 型別約束（Concepts）簡介（C++20）
//     8. 非型別模板參數（Non-type Template Parameters）
//
//   核心語法一覽：
//     template <typename T> T f(T a, T b);            // 函式模板
//     template <typename T> class C { ... };          // 類別模板
//     template <> bool is_equal<const char*>(...);    // 完全特化
//     template <typename T> class TypeInfo<T*> {...}; // 部分特化（僅類別模板）
//     template <typename T, size_t N> ...             // 非型別模板參數
//
//   標準版本（本機以 -pedantic-errors 逐項實測，非憑印象）：
//     函式／類別模板、特化、非型別模板參數 ......... C++98
//     auto、decltype、尾置回傳型別、variadic .... C++11
//     generic lambda、變數模板、auto 回傳推導 ... C++14
//       （實測：-std=c++11 失敗、-std=c++14 通過）
//     if constexpr、CTAD、fold expression ....... C++17
//       （實測：-std=c++14 失敗、-std=c++17 通過）
//     template<auto> ............................ C++17
//       （實測：-std=c++14 失敗、-std=c++17 通過）
//     concepts、浮點數當非型別模板參數 .......... C++20
//       （實測：-std=c++17 失敗、-std=c++20 通過）
//
//   標頭檔：<iostream> <vector> <string> <type_traits> <typeinfo>
//           <sstream> <queue> <functional>
//
//   本檔以 -std=c++17 編譯。本課唯一需要 C++20 的是「概念10.cpp」（concepts）。
//
// =============================================================================
// 【詳細解釋 Explanation】
// =============================================================================
//
// 【1. 泛型編程到底解決什麼問題】
// 標準答案不是「少寫重複程式碼」。少打字只是附帶效果，真正的核心是：
//
//     讓演算法能作用在「作者當初不知道其存在」的型別上。
//
// 函式庫作者無法窮舉使用者未來會定義的型別 —— 你寫不出 swap_UserMatrix，
// 因為那個型別在你發佈函式庫時還不存在。重載與複製貼上在本質上都做不到
// 這件事，而 template 可以。std::sort 能排序你今天下午才定義的 struct，
// 靠的就是這個能力。
//
// 【2. 為什麼函式重載不夠】
// 重載可以讓呼叫端統一（都寫 my_max），但**函式體仍然是 N 份**。
// 重載改善的是「命名」，不是「重複」：修改成本仍是 O(N)、兩份「應該相同」
// 的程式碼長期必然分岔、而且依然無法服務未知型別。
//   * 重載讓**呼叫者**輕鬆
//   * 模板讓**實作者**輕鬆
// 兩者解決不同層面的問題，而且可以並存（見【4. 多載決議】）。
//
// 【3. template 是編譯期生成程式碼（本課最重要的觀念）】
// `my_max` **本身不是函式**，而是一張「產生函式的藍圖」。編譯器看到
// my_max(3, 5) 時做三件事：
//     1) 推導 T = int                          （template argument deduction）
//     2) 把藍圖中的 T 換成 int，生成真正的函式  （instantiation）
//     3) 對生成出來的函式做完整型別檢查與最佳化
//
// 與 Java / C# 泛型的根本差異（高頻面試題）：
//
//   C++ template ── 編譯期「生成程式碼」（monomorphization）
//     * 每個用到的型別各生成一份專屬機器碼
//     * 執行期零成本，可完整 inline、可針對該型別最佳化
//     * 代價：code bloat 與編譯時間
//
//   Java generics ── 編譯期「抹除型別」（type erasure）
//     * List<String> 與 List<Integer> 在 bytecode 中是**同一個**型別
//     * 型別參數被抹成 Object，編譯器插入 cast
//     * 只有一份程式碼、沒有 bloat，但無法針對型別最佳化
//     * 因此 Java 沒有 List<int>（要裝箱成 Integer）、不能 new T[]
//
//   一句話：**C++ 是「一個模板變成很多份程式碼」，
//           Java 是「很多個型別共用一份程式碼」。**
//   Rust 同屬前者；C# 介於兩者之間（實質型別特化、參考型別共用）。
//
//   本機 g++ 15.2 實測佐證：一個 my_max 模板用 7 種型別各呼叫一次，
//   `nm -C` 在目的檔中可見 7 個獨立符號（char / short / int / long /
//   unsigned long / float / double 各一份），且每個都標記為 W（weak symbol）。
//
// 【4. 模板引數推導與多載決議】
// 推導最反直覺的一點：**推導階段不做隱式轉換**。
//     template <typename T> T my_max(T a, T b);
//     my_max(10, 3.14);   // 編譯失敗！
// 兩個參數共用同一個 T，第一個推導出 int、第二個推導出 double →
// deduction conflict。編譯器沒有義務去找「共同型別」，它只是在做型別比對。
// 規則是「**先推導，後轉換**」：T 定案之後，隱式轉換才會發生 ——
// 所以 my_max<double>(10, 3.14) 反而可行（略過推導，10 正常轉成 double）。
//
// 完整的多載決議流程：
//     1) 收集所有同名候選者（一般函式 + 函式模板）
//     2) 對每個模板嘗試推導；失敗者**安靜地移出候選集**
//        （這就是 SFINAE：Substitution Failure Is Not An Error）
//     3) 對剩下的候選者比較轉換代價
//     4) 最佳者不唯一 → ambiguous；候選集為空 → no matching function
//   關鍵規則：**非模板函式與模板同樣匹配時，非模板優先**。
//   這使得「為熱點型別手寫最佳化版本」成為可能，呼叫端完全不必改。
//
// 【5. 為什麼 template 定義要放在 header（高頻面試題）】
// 因為實例化**必須看得到完整函式本體**才能生成程式碼。C++ 的編譯單位是
// 一個 .cpp 一個 TU，TU 之間互相看不見內容。定義藏在別的 .cpp 時，
// 使用端的 TU 拿不到藍圖，只能留下未定義符號給 linker。
//
//   本機實測（g++ 15.2）：宣告放 header、定義放 tu_def.cpp，
//   兩個 .cpp **各自都能編譯成功**，但連結失敗：
//       undefined reference to `int twice<int>(int)'
//   `nm -C tu_main.o` 顯示該符號狀態為 U（undefined）。
//   這類錯誤的特徵是「編譯全過、只有連結掛掉」，而且訊息完全沒提到 template。
//
//   兩種解法：
//     (a) 定義放進 header（最常見，STL 全部這樣做）
//     (b) 在定義端做 explicit instantiation：`template int twice<int>(int);`
//         本機實測加上這行即可連結成功。適用於型別集合已知且很少的場合，
//         可換取編譯速度並隱藏實作。
//
//   延伸問題：都放 header，10 個 .cpp 都 include，不就 10 份定義違反 ODR？
//   → 不會。模板實例化的符號是 **weak / COMDAT**（nm 顯示 W），
//     linker 會把多份相同定義合併成一份。這個機制叫 **vague linkage**，
//     inline 函式與類別內定義的成員函式也走同一條路。
//
// 【6. 模板特化：為特定型別改寫行為】
//   * **完全特化**（full specialization）：指定所有模板參數。
//     函式模板與類別模板都支援。本檔的 is_equal<const char*> 就是 ——
//     通用版用 ==（對 const char* 是比較**指標位址**，語意錯誤），
//     特化版改用字串內容比較。
//   * **部分特化**（partial specialization）：只指定部分參數或限定形態
//     （如 T*）。**只有類別模板支援，函式模板不支援**。
//     函式模板要達到類似效果，得靠多載或 tag dispatch。
//     本檔的 TypeInfo<T*> 是部分特化，TypeInfo<int> 是完全特化。
//   選擇順序：完全特化 > 部分特化 > 通用版本（越特定越優先）。
//
// 【7. 隱含介面：模板沒寫出來的合約】
// `template <typename T> T my_max(T a, T b) { return a > b ? a : b; }`
// 的本體用了 `a > b`，這等於偷偷要求「T 必須支援 operator>」。
// 這種沒寫在型別系統裡、卻實際存在的要求叫**隱含介面**，
// 是編譯期的鴨子型別。
//
// 它何時被檢查？**兩階段編譯**：
//   * 第一階段（定義時）：只檢查與 T 無關的語法錯誤
//   * 第二階段（實例化時）：T 確定後，才真正檢查 `a > b` 是否合法
// 所以「模板寫錯了但沒人用它，就不會報錯」是真的。這也解釋了模板錯誤訊息
// 為什麼總是指向呼叫那一行、又附一長串 instantiation backtrace。
//
// 【8. 非型別模板參數：模板參數也可以是「值」】
// 模板參數不限於型別，也可以是編譯期常數：
//     template <typename T, int N> class FixedArray { T data_[N]; };
//     template <int N> struct Factorial { static const int value = ...; };
// 這讓「陣列大小」「緩衝區容量」成為型別的一部分 ——
// FixedArray<int, 5> 與 FixedArray<int, 10> 是**兩個不同的型別**。
// std::array<T, N> 正是這個設計，也因此它的 size() 是編譯期常數，
// 且不需要像 vector 那樣在堆積上配置記憶體。
//
// 允許的非型別參數型別：整數與列舉、指標／參考、std::nullptr_t，
// C++20 起還放寬到浮點數與符合條件的字面量類別
// （本機實測：`template <double D> struct S{};` 在 -std=c++17 失敗、
//   -std=c++20 通過）。
//
// 【9. Factorial<5> 的計算發生在編譯期】
//     template <int N> struct Factorial {
//         static const int value = N * Factorial<N-1>::value;
//     };
//     template <> struct Factorial<0> { static const int value = 1; };
// 這是**模板遞迴**（template metaprogramming 的入門形式）：
// Factorial<5>::value 展開時遞迴實例化出 Factorial<4>...Factorial<0>，
// 最終在編譯期算出 120，執行期完全沒有計算。
// 現代 C++ 更推薦用 constexpr 函式達成同樣目的：
//     constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n-1); }
// 可讀性好得多，且同一份程式碼在編譯期與執行期都能用。
// 模板遞迴的缺點是每層都是一次實例化 —— 遞迴深度直接轉成編譯時間與記憶體。
//
// =============================================================================
// 【概念補充 Concept Deep Dive】
// =============================================================================
//
// (A) Code bloat 與編譯時間 —— template 的真實代價（本機實測）
//   實例化不是免費的：
//     * 目的檔大小：同一個類別模板只用 1 種型別實例化時，目的檔 1,744 bytes；
//       改成 200 種不同型別後變成 71,600 bytes（約 41 倍）。
//     * 編譯時間：同一組測試 0.09s → 0.16s。
//     （本機 g++ 15.2 -O0 實測值，非標準保證；換編譯器或最佳化等級會變。）
//   多參數模板會讓這個問題相乘：3 個型別參數、每個 5 種型別 → 最壞 125 份。
//   緩解手段：把與型別無關的邏輯抽到非模板函式（thin template wrapper +
//   fat non-template core）、用 extern template 抑制重複實例化、
//   或改用型別抹除（std::function、虛擬介面）換取體積。
//
// (B)「零成本抽象」的正確理解
//   C++ 的承諾是「不用的功能不必付錢，用了的功能你自己寫也不會更快」，
//   談的一直是**執行期**開銷。它從未承諾編譯時間與二進位大小免費。
//   大型專案編譯時間爆炸，主因往往就是模板。
//
// (C) 每個實例化都是獨立型別
//   vector<int> 與 vector<double> 是兩個毫無關係的型別：
//     * 不能互相賦值（本機實測：conversion from 'vector<int>' to
//       non-scalar type 'vector<double>' requested），即使 int 能轉 double
//     * static 成員**每個實例化各一份**（本機實測：Counter<int>::n 設 5、
//       Counter<double>::n 設 99，互不影響）
//     * vector<Derived*> 不能當 vector<Base*> 用（無共變，這是刻意的設計：
//       否則就能往裡面塞別的 Base 衍生型別而破壞型別安全）
//
// (D) 類別模板的成員函式「用到才實例化」（lazy instantiation）
//   實例化 Box<T> 這個類別時，並**不會**實例化它全部的成員函式，
//   只有實際被呼叫的才會。本機實測：某成員函式本體對某型別根本不合法
//   （對沒有 operator+ 的型別做 v + v），只要從不呼叫就能正常編譯。
//   這正是為什麼 vector<T> 對 T 的要求其實很低 ——
//   是各個成員函式各自有各自的要求，不是一整包強制要求。
//
// (E) 編譯期多型 vs 執行期多型（本課核心結論）
//
//   ┌──────────────┬────────────────────┬─────────────────────┐
//   │              │ 執行期（virtual）   │ 編譯期（template）   │
//   ├──────────────┼────────────────────┼─────────────────────┤
//   │ 介面形式      │ 明確（繼承基底）    │ 隱含（會那個操作就行）│
//   │ 侵入性        │ 侵入式（要改型別）  │ 非侵入式             │
//   │ 決定時機      │ 執行期             │ 編譯期               │
//   │ 分派成本      │ 間接跳躍、無法 inline│ 零、可完全 inline    │
//   │ 物件大小      │ 多一個 vptr        │ 無額外開銷           │
//   │ 程式碼大小    │ 一份               │ 每型別各一份（bloat）│
//   │ 異質容器      │ 可以               │ 不行                 │
//   │ 執行期換實作  │ 可以（外掛/設定檔） │ 不行                 │
//   │ 錯誤訊息      │ 清楚               │ 冗長（實例化時爆炸） │
//   └──────────────┴────────────────────┴─────────────────────┘
//
//   本機實測佐證（g++ 15.2 / x86-64）：
//     * 只含一個 double 的類別：非虛擬版 sizeof = 8，加 virtual 後 = 16
//     * -O2 組語：虛擬呼叫是 `jmp *%rax`（間接跳躍）；
//       模板版被完全 inline 成兩道 mulsd 乘法，連函式呼叫都沒有
//   這也解釋了為什麼 std::sort 通常比 C 的 qsort 快：qsort 的比較器是
//   函式指標（執行期間接呼叫、無法 inline），std::sort 的比較器是模板參數
//   （編譯期已知、可內聯進排序內迴圈）。
//
//   選擇準則：
//     1) 型別編譯期已知且要效能 → 模板
//     2) 型別執行期才定（設定檔、外掛、跨 ABI）→ virtual
//     3) 需要異質容器 → virtual，或 C++17 的 std::variant + std::visit
//        （型別集合封閉時的折衷方案：值語意、無 vtable）
//
// (F) 從 static_assert 到 concepts 的演進
//   要把隱含要求「講出來」，歷史上有三代做法：
//     1) static_assert（C++11）：事後報錯，**不影響多載決議**
//     2) SFINAE / std::enable_if（C++11）：能影響多載決議，但語法極難讀
//     3) concepts（C++20）：前兩者的正統取代品，語法直觀且參與多載決議
//   關鍵差異：static_assert 是「已經選中這個函式，然後才發現型別不對」；
//   concepts 是「這個函式根本不進入候選集」。有多個多載時這個差別是決定性的。
//
//   誠實的補充：concepts 不保證錯誤訊息一定變短。本機實測 ——
//     * 單層模板：無約束 7 行 vs concept 版 23 行（**反而更長**）
//     * 深層巢狀：std::sort 131 行 vs std::ranges::sort 36 行（大幅改善）
//   真正的價值是「直接說出缺了哪條契約」與「正確歸因」，不是行數。
//
// =============================================================================
// 【注意事項 Pay Attention】
// =============================================================================
// 1. typename 與 class 在模板參數列表中完全等價。但在「相依型別」的場合
//    只能用 typename（如 typename T::value_type），那是另一個不同的用法。
// 2. **函式模板不支援部分特化**，只有類別模板支援。想對函式做類似的事，
//    請用多載或 tag dispatch。
// 3. 對 C 字串呼叫比較類模板是經典陷阱：T 被推導成 const char*，
//    `a > b` 比較的是**指標位址**而非內容。它能編譯、能執行，
//    但結果沒有意義（無關物件的指標比較，其結果標準並未指定）。
//    本檔的 is_equal<const char*> 特化正是為了修正這件事。
// 4. `typeid(T).name()` 的輸出是**實作定義**的。本機 g++ 回傳 mangled name
//    （int 是 "i"、double 是 "d"、std::string 是一長串），MSVC 則回傳
//    可讀的全名。要跨平台可讀請用 abi::__cxa_demangle 或 Boost.TypeIndex。
//    **不可依賴它做邏輯判斷**，僅適合除錯輸出。
// 5. 明確指定模板引數會**關掉推導原本提供的保護**。
//    my_max<int>(3.9, 2.1) 會把兩個 double 截斷成 int 得到 3，
//    程式照常編譯執行，小數安靜消失。加 -Wconversion 可讓這類截斷現形。
// 6. 模板的錯誤訊息長度隨巢狀深度爆炸，且錯誤位置會指向**函式庫的原始碼**。
//    那不代表函式庫有 bug —— 99.9% 是你的型別不滿足隱含要求。
//    閱讀順序：先找 `[with T = ...]` → 再找 `required from here` → 最後讀 error。
// 7. 用到什麼標頭就 include 什麼。本檔用了 typeid 故 include <typeinfo>；
//    依賴「某個標頭碰巧間接引入」是不可攜的，換標準庫版本就可能編譯失敗。
// 8. 模板遞迴（如 Factorial）每層都是一次實例化，深度直接轉成編譯時間與
//    記憶體消耗，且編譯器有實作定義的深度上限。現代寫法優先用 constexpr 函式。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】泛型編程總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++ template 和 Java generics 最根本的差別是什麼？
//     答：C++ 是編譯期**生成程式碼**（monomorphization）：每個用到的型別各
//         實例化出一份專屬機器碼，執行期零成本、可完整 inline，
//         代價是 code bloat 與編譯時間。Java 是**型別抹除**：
//         List<String> 與 List<Integer> 在 bytecode 中是同一個型別，
//         型別參數被抹成 Object 並由編譯器插入 cast，只有一份程式碼、
//         但無法針對型別最佳化。
//     追問：所以為什麼 Java 沒有 List<int>，C++ 卻有 vector<int>？
//         → 抹除後型別參數必須能當 Object，基本型別得先裝箱成 Integer。
//           C++ 是真的為 int 生成一份專屬程式碼，vector<int> 裡放的
//           就是實實在在的 int。
//
// 🔥 Q2. 為什麼 template 的定義通常必須放在 header？
//     答：因為實例化需要看得到完整函式本體才能生成程式碼。定義藏在別的 TU 時，
//         使用端拿不到藍圖，只能留下未定義符號 → **編譯全過、連結失敗**
//         （本機實測：undefined reference to `int twice<int>(int)'）。
//         例外解法是在定義端寫 explicit instantiation。
//     追問：10 個 .cpp 都 include，不就 10 份定義違反 ODR 嗎？
//         → 不會。模板實例化的符號是 weak / COMDAT（nm 顯示 W），
//           linker 會合併成一份，機制叫 vague linkage。
//
// 🔥 Q3. my_max(10, 3.14) 為什麼編譯失敗？int 不是可以自動轉 double 嗎？
//     答：兩個參數共用同一個 T，一邊推導出 int、一邊 double →
//         deduction conflict。關鍵是**推導階段不做隱式轉換**：
//         編譯器在做型別比對，沒有義務去找共同型別。
//         規則是「先推導，後轉換」。
//     追問：那 my_max<double>(10, 3.14) 為什麼可以？
//         → 明確指定 T 等於略過推導，參數型別直接是 (double, double)，
//           此時 int 實參照一般規則隱式轉換，完全合法。
//
// 🔥 Q4. 函式模板可以部分特化嗎？
//     答：**不行**。部分特化只有類別模板支援。函式模板只能做完全特化，
//         要達到類似效果得靠多載或 tag dispatch。
//         這也是為什麼很多函式庫把邏輯包成類別模板的 static 成員 ——
//         就為了能用部分特化。
//     追問：完全特化和多載又差在哪？
//         → 多載參與正常的多載決議（且非模板優先於模板）；完全特化不參與，
//           它只是在「已經選中某個模板之後」決定要用哪份實作。
//           所以特化與多載並存時的行為常常出人意料，
//           實務上多數指南建議「優先用多載，少用函式模板特化」。
//
// 🔥 Q5. 編譯期多型與執行期多型該怎麼選？
//     答：型別在編譯期已知且要效能 → 模板（零分派成本、可 inline、無 vptr，
//         代價是 code bloat 與冗長的錯誤訊息）。型別要執行期才定
//         （設定檔、外掛、跨 ABI）或需要異質容器 → virtual
//         （代價是 vptr 與無法 inline）。兩者常並用，不是互斥的競爭關係。
//     追問：有沒有第三條路？
//         → C++17 的 std::variant + std::visit：型別集合封閉且已知時，
//           提供異質容器且不需要 vtable，是兩者之間的折衷。
//
// ⚠️ 陷阱 1. 「template 只是編譯器幫我複製貼上，所以完全零成本。」錯在哪？
//     答：錯在「零成本」被過度延伸。它在**執行期**確實零成本（無虛擬表、
//         無 boxing），但**編譯期**成本很實在。本機實測：同一個類別模板從
//         1 種型別擴到 200 種，目的檔由 1,744 bytes 膨脹到 71,600 bytes、
//         編譯時間 0.09s → 0.16s（g++ 15.2 -O0 實測，非標準保證）。
//     為什麼會錯：把「zero-overhead abstraction」記成「zero cost」。
//         C++ 的承諾談的是**執行期**開銷，從未承諾編譯時間與二進位大小免費。
//
// ⚠️ 陷阱 2. is_equal("hello", "hello") 若沒有那個特化版本，會回傳什麼？
//     答：通用版用 `a == b`，而 T 被推導成 const char*（字串字面量 decay
//         成指標），比較的是**指標位址**而不是字串內容。
//         結果取決於編譯器是否合併相同的字面量，標準並未規定 ——
//         **不可以斷言它一定是 true 或一定是 false**。
//         本檔提供 is_equal<const char*> 完全特化改用字串內容比較，
//         才有確定且正確的語意。
//     為什麼會錯：以為「== 就是比較內容」。對指標而言 == 永遠是比位址；
//         C 字串的「內容相等」必須用 strcmp 或轉成 std::string 才能表達。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>
#include <typeinfo>    // typeid：別依賴間接引入（見【注意事項 7】）
#include <sstream>     // 【日常實務範例 1】泛型設定值轉換
#include <queue>       // 【LeetCode 703】priority_queue
#include <functional>  // std::greater


// ===== 重點一：為什麼需要泛型編程？=====
// 問題：不使用泛型，同一個邏輯需要為每種型別重複寫一次

// 不好的做法（重複程式碼）：
int max_int(int a, int b) { return a > b ? a : b; }
double max_double(double a, double b) { return a > b ? a : b; }
// 如果要支援 float, long, char... 需要繼續重複寫！
// 更致命的是：使用者自訂的型別，函式庫作者根本無法預先寫出來。

// ===== 重點二：函數重載（解決部分問題，但仍有重複）=====
// 函數重載允許同名函數有不同的參數型別
// 但每個版本的函數體幾乎相同，邏輯上是重複的
// —— 重載讓「呼叫者」輕鬆，模板讓「實作者」輕鬆

// ===== 重點三：函數模板（Function Template）=====
// 語法：template <typename T> 返回型別 函數名稱(參數列表) { ... }
// 作用：讓編譯器根據實際使用的型別自動生成對應的函數
// 關鍵字 typename 和 class 在模板中等效，但 typename 更清晰表示「這是一個型別」

// 基本函數模板：
template <typename T>
T my_max(T a, T b) {
    // T 是一個佔位符（placeholder），代表「某種型別」
    // 當你呼叫 my_max(3, 5) 時，編譯器把 T 替換成 int
    // 當你呼叫 my_max(3.14, 2.72) 時，編譯器把 T 替換成 double
    // 隱含要求：T 必須支援 operator> 且可複製
    return a > b ? a : b;
}

// 多個型別參數的模板：兩個參數各自獨立推導，不會產生 deduction conflict
template <typename T, typename U>
void print_pair(T first, U second) {
    std::cout << "(" << first << ", " << second << ")" << std::endl;
}

// 模板函數也可以有非模板參數：
template <typename T>
T clamp(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// 多載決議示範：非模板函式與模板同樣匹配時，非模板優先
template <typename T>
const char* which_one(T) { return "泛型模板版"; }
const char* which_one(int) { return "非模板版（優先勝出）"; }


// ===== 重點四：類別模板（Class Template）=====
// 類別模板讓容器類別可以儲存任意型別的元素
// 這是 STL 容器（vector, list, set...）的實作基礎！
// 注意：每個實例化都是**獨立的型別** —— MyPair<int,int> 與 MyPair<int,double>
//       之間沒有任何轉換關係。

// 簡單的泛型 pair 類別：
template <typename First, typename Second>
class MyPair {
public:
    First first;
    Second second;

    // 建構子：初始化兩個成員
    MyPair(First f, Second s) : first(f), second(s) {}

    // 輸出方法
    void print() const {
        std::cout << "MyPair(" << first << ", " << second << ")" << std::endl;
    }
};

// 簡單的泛型 Stack（堆疊）：
template <typename T>
class MyStack {
private:
    std::vector<T> data_;  // 內部使用 vector 儲存元素

public:
    // push：放入元素（放在頂端）
    void push(const T& value) {
        data_.push_back(value);
    }

    // pop：取出頂端元素（後進先出 LIFO）
    // 注意：與 std::stack 一樣回傳 void 而非元素 —— 這是例外安全的考量，
    //       若 pop 要回傳值，複製過程中拋例外就會讓元素永久遺失。
    void pop() {
        if (!data_.empty()) {
            data_.pop_back();
        }
    }

    // top：查看頂端元素（不移除）
    T& top() {
        return data_.back();
    }

    const T& top() const {
        return data_.back();
    }

    // empty：檢查是否為空
    bool empty() const {
        return data_.empty();
    }

    // size：元素個數
    size_t size() const {
        return data_.size();
    }
};

// 固定大小的泛型陣列（使用非型別模板參數）：
// FixedArray<int,5> 與 FixedArray<int,10> 是兩個**不同的型別**
template <typename T, int N>
class FixedArray {
private:
    T data_[N];  // 大小在編譯期固定

public:
    FixedArray() {
        for (int i = 0; i < N; ++i) {
            data_[i] = T{};  // 值初始化
        }
    }

    T& operator[](int index) {
        return data_[index];
    }

    const T& operator[](int index) const {
        return data_[index];
    }

    int size() const { return N; }

    void print() const {
        std::cout << "FixedArray<" << N << ">: ";
        for (int i = 0; i < N; ++i) {
            std::cout << data_[i] << " ";
        }
        std::cout << std::endl;
    }
};


// ===== 重點五：模板特化（Template Specialization）=====
// 問題：有時候通用模板對特定型別的行為不對，需要提供「特化版本」
// 例如：比較 C 字串（char*）不能用 == 運算子（那是比較指標位址）

// 通用版本（適用大多數型別）：
template <typename T>
bool is_equal(T a, T b) {
    return a == b;
}

// 針對 const char* 的完全特化版本：
// 沒有這個特化，is_equal("hello","hello") 比較的是指標位址，
// 結果取決於編譯器是否合併相同字面量 —— 標準未規定（見【陷阱 2】）
template <>
bool is_equal<const char*>(const char* a, const char* b) {
    // 字串比較需要用內容比較，不能用 ==（== 比較的是指標位址）
    return std::string(a) == std::string(b);
}

// 部分特化（Partial Specialization）只能用於類別模板：
template <typename T>
class TypeInfo {
public:
    static void print() {
        std::cout << "一般型別" << std::endl;
    }
};

// 針對指標型別的部分特化（函式模板做不到這件事）：
template <typename T>
class TypeInfo<T*> {
public:
    static void print() {
        std::cout << "指標型別" << std::endl;
    }
};

// 針對 int 的完全特化：
template <>
class TypeInfo<int> {
public:
    static void print() {
        std::cout << "整數型別（int）" << std::endl;
    }
};


// ===== 重點六：模板中的型別推導（Type Deduction）=====
// C++ 編譯器可以從函數呼叫的參數自動推導模板參數型別
// 不需要顯式指定型別（但可以顯式指定）
// 關鍵：推導階段**不做隱式轉換**，是型別比對而非尋找共同型別

template <typename T>
void show_type(T value) {
    // typeid(T).name() 的輸出是「實作定義」的：
    //   g++ 回傳 mangled name（int="i"、double="d"、char="c"）
    //   MSVC 回傳可讀的全名
    // 不可依賴它做邏輯判斷，僅適合除錯輸出（見【注意事項 4】）
    std::cout << "值: " << value
              << " | 型別（概略）: " << typeid(T).name() << std::endl;
}

// auto 與型別推導（C++11 起）：
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b) {
    // 尾置返回型別（trailing return type，C++11）
    // decltype(a + b) 讓編譯器推導 a + b 的型別作為返回型別
    // C++14 起可直接寫 auto add(T a, U b) 讓回傳型別自動推導
    //   （本機 -pedantic-errors 實測：C++11 失敗、C++14 通過）
    return a + b;
}


// ===== 重點七：Concepts（C++20 型別約束）=====
// 問題：傳統模板的錯誤訊息非常難以閱讀，且要求不可見
// Concepts 讓你可以在模板上加入「約束」，把要求寫進簽名
// 本檔以 C++17 編譯，故以下用 C++11/17 的替代手段示範；
// 真正的 concepts 見「概念10.cpp」（需 -std=c++20）

// 手段一：static_assert 模擬約束（C++11）
// 缺點：**不影響多載決議** —— 是「已經選中才報錯」
template <typename T>
T safe_sqrt(T value) {
    static_assert(std::is_floating_point<T>::value,
                  "safe_sqrt 只接受浮點型別（float 或 double）！");
    // 真實情境中應該 #include <cmath> 並使用 std::sqrt
    return value;  // 簡化示範
}

// 手段二：std::enable_if（C++11 SFINAE 技巧）
// SFINAE = Substitution Failure Is Not An Error（替換失敗不是錯誤）
// 優點：能影響多載決議（不滿足者安靜退出候選集）
// 缺點：語法極難讀 —— 這正是 C++20 concepts 要取代的東西
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
only_for_integers(T value) {
    std::cout << "這個函數只接受整數！值：" << value << std::endl;
    return value;
}


// ===== 重點八：非型別模板參數（Non-type Template Parameters）=====
// 模板參數不只可以是「型別」，也可以是「值」
// 常用於在編譯期確定陣列大小、緩衝區大小等

// 示範：編譯期計算陣列大小
// 參數**刻意不寫名字**（只寫 T (&)[N]）：函式只需要型別中的 N，
// 從不使用那個引數本身。寫成 T (&arr)[N] 會觸發 -Wunused-parameter 警告。
// 這是泛型程式碼中很常見的寫法 —— 引數只是用來「攜帶型別資訊」。
//
// 有趣的是：這個警告只有在 array_size **真的被呼叫**時才會出現。
// 沒有人呼叫它，模板就不會被實例化，本體也就不會被檢查（見【概念補充 D】
// 的 lazy instantiation）—— 這正是本課講的兩階段編譯在實務上的體現。
template <typename T, size_t N>
size_t array_size(T (&)[N]) {
    // 這個函數接受一個 C 陣列的引用，從型別中提取大小 N
    // 注意參數是 T(&)[N]（陣列參考）而非 T* —— 綁到參考不會 decay，
    // 所以 N 推導得出來。這正是 std::size（C++17）的實作原理。
    return N;
}

// 編譯期計算（模板遞迴）：
template <int N>
struct Factorial {
    // 使用模板遞迴在編譯期計算階乘
    // 每一層都是一次實例化 —— 深度直接轉成編譯時間
    static const int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    // 遞迴基底：0! = 1
    static const int value = 1;
};

// 現代寫法：constexpr 函式（C++11 起，C++14 起可用迴圈與多敘述）
// 可讀性遠優於模板遞迴，且編譯期／執行期都能用
constexpr int factorial_constexpr(int n) {
    return n <= 1 ? 1 : n * factorial_constexpr(n - 1);
}


// ===== 型別推導與 auto 的關係 =====
// auto 在 C++11 中引入，讓編譯器自動推導變數型別
// auto 的推導規則與函數模板的型別推導規則幾乎相同
// （唯一著名的差異：auto 對 { } 初始化列表會推導成 std::initializer_list）

void demo_auto_deduction() {
    auto x = 42;          // x 是 int
    auto y = 3.14;        // y 是 double
    auto z = "hello";     // z 是 const char*（不是 std::string！）
    auto v = std::vector<int>{1, 2, 3};  // v 是 vector<int>

    std::cout << "auto x: " << x << std::endl;
    std::cout << "auto y: " << y << std::endl;
    std::cout << "auto z: " << z << std::endl;
    std::cout << "auto v size: " << v.size() << std::endl;
}


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 155. Min Stack
//   題目：設計一個堆疊，支援 push / pop / top，並能在 **O(1)** 取得最小值。
//   為什麼用到本主題：這是類別模板最自然的應用 —— 題目雖然只要求 int，
//         但「維護最小值」的邏輯與元素型別完全無關。
//         寫成 template 之後，同一份程式碼對 int、double、std::string
//         全部適用（只要該型別支援 operator<）。
//   解法：用兩個堆疊。data_ 存所有元素，min_ 只在「新元素 <= 目前最小值」
//         時才同步 push，pop 時若彈出的正是目前最小值就一起彈。
//   複雜度：全部操作 O(1)，額外空間最壞 O(n)。
//   隱含介面：T 需支援 operator< 與可複製。
// -----------------------------------------------------------------------------
template <typename T>
class MinStack {
private:
    std::vector<T> data_;
    std::vector<T> min_;    // 單調堆疊：頂端永遠是目前的最小值

public:
    void push(const T& v) {
        data_.push_back(v);
        // 用 <= 而非 < ：重複的最小值也要記錄，否則 pop 時會提早丟失最小值
        if (min_.empty() || !(min_.back() < v)) {
            min_.push_back(v);
        }
    }

    void pop() {
        if (data_.empty()) return;
        // 彈出的若正是目前最小值，min_ 也要同步彈出
        if (!min_.empty() && !(min_.back() < data_.back())
                          && !(data_.back() < min_.back())) {
            min_.pop_back();
        }
        data_.pop_back();
    }

    const T& top() const { return data_.back(); }
    const T& getMin() const { return min_.back(); }
    bool empty() const { return data_.empty(); }
};


// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 703. Kth Largest Element in a Stream
//   題目：設計一個類別，持續接收數字串流，每次回報「目前第 k 大」的元素。
//   為什麼用到本主題：這題的標準解法直接用上了模板參數作為「策略」——
//         std::priority_queue 的完整宣告是
//             template <typename T,
//                       typename Container = std::vector<T>,
//                       typename Compare   = std::less<typename Container::value_type>>
//             class priority_queue;
//         預設的 std::less 給出**大頂堆**；把第三個模板參數換成
//         std::greater<int> 就變成**小頂堆**。
//         這是「用型別當參數來抽換行為」的典型範例 —— 比較邏輯在編譯期
//         就被決定並內聯，沒有任何執行期分派成本。
//   解法：維護一個大小為 k 的小頂堆，堆頂就是第 k 大。
//   複雜度：每次 add 為 O(log k)，空間 O(k)。
// -----------------------------------------------------------------------------
class KthLargest {
private:
    // 第三個模板參數 std::greater<int> 把預設的大頂堆換成小頂堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> heap_;
    size_t k_;

public:
    KthLargest(int k, const std::vector<int>& nums) : k_(static_cast<size_t>(k)) {
        for (int n : nums) add(n);
    }

    int add(int val) {
        heap_.push(val);
        if (heap_.size() > k_) heap_.pop();   // 只保留最大的 k 個
        return heap_.top();                    // 小頂堆的頂端 = 第 k 大
    }
};


// -----------------------------------------------------------------------------
// 【日常實務範例 1】泛型設定值轉換：把設定檔／環境變數的字串轉成任意型別
//   情境：設定檔讀進來永遠是字串（"8080"、"3.5"、"true"），但程式需要的是
//         int、double、bool。若為每個型別各寫一個 parse_int / parse_double，
//         就回到本課開頭那個重複的問題。
//   為何用泛型：一個 parse_config<T> 服務所有型別，且新增型別時不必改它。
//   設計重點：回傳 bool 表示成功與否、結果由參數帶出，避免用例外處理
//             「設定值格式錯誤」這種可預期的情況。
//             轉換後再檢查 eof()，可攔截 "123abc" 這種「前段能轉、後段是垃圾」
//             的輸入 —— 只檢查 fail() 會讓它矇混過關。
// -----------------------------------------------------------------------------
template <typename T>
bool parse_config(const std::string& raw, T& out) {
    std::istringstream iss(raw);
    iss >> out;
    // fail() 擋掉完全轉不動的；eof() 確保整個字串都被吃完（擋掉 "123abc"）
    return !iss.fail() && iss.eof();
}

// 帶預設值的版本：轉換失敗就沿用預設值（設定檔載入最常見的需求）
template <typename T>
T parse_config_or(const std::string& raw, const T& fallback) {
    T value{};
    return parse_config(raw, value) ? value : fallback;
}


// -----------------------------------------------------------------------------
// 【日常實務範例 2】泛型固定容量快取：任何 key/value 型別都能用
//   情境：服務端常需要一個小型快取（DNS 解析結果、使用者權限、設定片段）。
//         key 可能是 std::string 或 int，value 可能是任何東西。
//   為何用泛型：快取的「查找 → 命中/未命中 → 淘汰」邏輯與型別完全無關。
//         寫成 template 之後，Cache<std::string,int> 與 Cache<int,std::string>
//         共用同一份原始碼（但編譯後是兩份獨立程式碼 —— 這就是 code bloat）。
//   淘汰策略：容量滿時淘汰最舊的（FIFO）。真實系統多用 LRU，
//         這裡用 FIFO 是為了聚焦在泛型本身而非淘汰演算法。
//   隱含介面：Key 需支援 operator==（用於查找）。
// -----------------------------------------------------------------------------
template <typename Key, typename Value>
class FixedCache {
private:
    struct Entry { Key key; Value value; };
    std::vector<Entry> entries_;
    size_t capacity_;
    size_t hits_ = 0;
    size_t misses_ = 0;

public:
    explicit FixedCache(size_t cap) : capacity_(cap) {}

    void put(const Key& k, const Value& v) {
        for (auto& e : entries_) {
            if (e.key == k) { e.value = v; return; }   // 已存在就更新
        }
        if (entries_.size() >= capacity_) {
            entries_.erase(entries_.begin());          // FIFO：淘汰最舊的
        }
        entries_.push_back(Entry{k, v});
    }

    // 回傳是否命中；命中時把值寫進 out
    bool get(const Key& k, Value& out) {
        for (const auto& e : entries_) {
            if (e.key == k) { out = e.value; ++hits_; return true; }
        }
        ++misses_;
        return false;
    }

    size_t hits() const { return hits_; }
    size_t misses() const { return misses_; }
    size_t size() const { return entries_.size(); }
};


// -----------------------------------------------------------------------------
// 【日常實務範例 3】泛型序列化：把任何容器輸出成 CSV 一行
//   情境：把統計結果、批次匯出資料寫成 CSV／log 行。
//         來源可能是 vector<int>、vector<std::string>，
//         甚至是 C 陣列 —— 不該為每種來源各寫一個函式。
//   為何用泛型：這裡用**迭代器區間**（first, last）而非容器型別，
//         抽象層級最高 —— 它連 C 陣列、std::array、list 都吃得下。
//         STL 全部演算法都採用這個介面設計，正是第三課的主題。
//   隱含介面：It 需支援 ++、解參考與 !=；元素需支援 operator<<。
// -----------------------------------------------------------------------------
template <typename It>
std::string to_csv(It first, It last, char sep = ',') {
    std::ostringstream oss;
    bool first_item = true;
    for (It it = first; it != last; ++it) {
        if (!first_item) oss << sep;
        oss << *it;
        first_item = false;
    }
    return oss.str();
}


int main() {
    std::cout << "====================================================" << std::endl;
    std::cout << "  第二課：泛型編程（Generic Programming）概念 - 總複習示範" << std::endl;
    std::cout << "====================================================" << std::endl;

    // --- 示範一：函數模板基本使用 ---
    std::cout << "\n--- 示範一：函數模板（my_max）---" << std::endl;
    std::cout << "my_max(3, 5): " << my_max(3, 5) << std::endl;
    std::cout << "my_max(3.14, 2.72): " << my_max(3.14, 2.72) << std::endl;
    std::cout << "my_max('a', 'z'): " << my_max('a', 'z') << std::endl;
    std::cout << "my_max(string, string): "
              << my_max(std::string("apple"), std::string("banana")) << std::endl;
    // my_max(10, 3.14) 會編譯失敗：deduction conflict（T 同時是 int 與 double）
    std::cout << "my_max<double>(10, 3.14): " << my_max<double>(10, 3.14)
              << "  <- 明確指定型別可略過推導" << std::endl;

    // --- 示範二：多型別參數 ---
    std::cout << "\n--- 示範二：多型別參數模板 ---" << std::endl;
    print_pair(42, 3.14);
    print_pair("hello", 100);
    print_pair(true, 'A');

    // --- 示範三：clamp 函數模板 ---
    std::cout << "\n--- 示範三：clamp（限制在範圍內）---" << std::endl;
    std::cout << "clamp(15, 0, 10): " << clamp(15, 0, 10) << std::endl;
    std::cout << "clamp(-5, 0, 10): " << clamp(-5, 0, 10) << std::endl;
    std::cout << "clamp(5, 0, 10): " << clamp(5, 0, 10) << std::endl;

    // --- 示範四：類別模板 ---
    std::cout << "\n--- 示範四：類別模板（MyPair）---" << std::endl;
    MyPair<int, std::string> p1(42, "Hello");
    MyPair<double, bool> p2(3.14, true);
    p1.print();
    p2.print();

    // --- 示範五：泛型 Stack ---
    std::cout << "\n--- 示範五：泛型 Stack ---" << std::endl;
    MyStack<int> int_stack;
    int_stack.push(10);
    int_stack.push(20);
    int_stack.push(30);
    std::cout << "Stack 頂端: " << int_stack.top() << std::endl;
    int_stack.pop();
    std::cout << "pop 後頂端: " << int_stack.top() << std::endl;
    std::cout << "Stack 大小: " << int_stack.size() << std::endl;

    // 字串的 Stack（同一份原始碼，第二份機器碼）
    MyStack<std::string> str_stack;
    str_stack.push("world");
    str_stack.push("hello");
    std::cout << "字串 Stack 頂端: " << str_stack.top() << std::endl;

    // --- 示範六：非型別模板參數（FixedArray）---
    std::cout << "\n--- 示範六：非型別模板參數（FixedArray）---" << std::endl;
    FixedArray<int, 5> fixed_arr;
    for (int i = 0; i < fixed_arr.size(); ++i) {
        fixed_arr[i] = (i + 1) * 10;
    }
    fixed_arr.print();
    // FixedArray<int,5> 與 FixedArray<int,10> 是兩個不同的型別，不可互相賦值
    int raw[7] = {0};
    std::cout << "array_size(raw[7]) = " << array_size(raw)
              << "  <- 從陣列參考的型別中取出 N（std::size 的原理）" << std::endl;

    // --- 示範七：模板特化 ---
    std::cout << "\n--- 示範七：模板特化 ---" << std::endl;
    std::cout << "is_equal(5, 5): " << (is_equal(5, 5) ? "true" : "false") << std::endl;
    std::cout << "is_equal(\"hello\", \"hello\"): "
              << (is_equal("hello", "hello") ? "true" : "false")
              << "  <- 走 const char* 完全特化，比較的是內容" << std::endl;

    TypeInfo<int>::print();          // 使用完全特化版本
    TypeInfo<double>::print();       // 使用通用版本
    TypeInfo<int*>::print();         // 使用指標部分特化版本（函式模板做不到）

    // --- 示範八：型別推導 ---
    std::cout << "\n--- 示範八：型別推導 ---" << std::endl;
    show_type(42);
    show_type(3.14);
    show_type('A');
    show_type(std::string("STL"));
    std::cout << "（typeid().name() 的輸出是實作定義的，g++ 給 mangled name）"
              << std::endl;

    auto result = add(3, 4.5);
    std::cout << "add(3, 4.5) = " << result << std::endl;

    // --- 示範九：auto 型別推導 ---
    std::cout << "\n--- 示範九：auto 型別推導 ---" << std::endl;
    demo_auto_deduction();

    // --- 示範十：編譯期計算 ---
    std::cout << "\n--- 示範十：編譯期計算（Factorial）---" << std::endl;
    std::cout << "5! = " << Factorial<5>::value << std::endl;
    std::cout << "10! = " << Factorial<10>::value << std::endl;
    // 這些值在編譯期就確定了，執行期沒有任何計算！
    std::cout << "constexpr 版 5! = " << factorial_constexpr(5)
              << "  <- 現代寫法，可讀性優於模板遞迴" << std::endl;

    // --- 示範十一：多載決議 ---
    std::cout << "\n--- 示範十一：多載決議（非模板優先）---" << std::endl;
    std::cout << "which_one(42)     -> " << which_one(42) << std::endl;
    std::cout << "which_one(3.14)   -> " << which_one(3.14) << std::endl;
    std::cout << "which_one<int>(42)-> " << which_one<int>(42)
              << "  <- 加 <int> 強制走模板" << std::endl;

    // --- 示範十二：SFINAE 約束 ---
    std::cout << "\n--- 示範十二：enable_if（C++20 concepts 的前身）---" << std::endl;
    only_for_integers(123);
    std::cout << "safe_sqrt(2.0) = " << safe_sqrt(2.0)
              << "  <- static_assert 擋掉非浮點型別" << std::endl;

    // --- LeetCode 155 ---
    std::cout << "\n=== LeetCode 155. Min Stack ===" << std::endl;
    MinStack<int> ms;
    ms.push(-2); ms.push(0); ms.push(-3);
    std::cout << "push(-2), push(0), push(-3)" << std::endl;
    std::cout << "getMin() = " << ms.getMin() << std::endl;   // -3
    ms.pop();
    std::cout << "pop() 後 top() = " << ms.top() << std::endl;    // 0
    std::cout << "pop() 後 getMin() = " << ms.getMin() << std::endl;  // -2

    // 同一份程式碼換個型別就能用 —— 這正是泛型的價值
    MinStack<std::string> ms_str;
    ms_str.push("pear"); ms_str.push("apple"); ms_str.push("orange");
    std::cout << "MinStack<std::string> getMin() = " << ms_str.getMin()
              << "  <- 同一份程式碼，換了元素型別" << std::endl;

    // --- LeetCode 703 ---
    std::cout << "\n=== LeetCode 703. Kth Largest Element in a Stream ===" << std::endl;
    KthLargest kth(3, std::vector<int>{4, 5, 8, 2});
    std::cout << "k=3, nums=[4,5,8,2]" << std::endl;
    std::cout << "add(3)  -> " << kth.add(3) << std::endl;   // 4
    std::cout << "add(5)  -> " << kth.add(5) << std::endl;   // 5
    std::cout << "add(10) -> " << kth.add(10) << std::endl;  // 5
    std::cout << "add(9)  -> " << kth.add(9) << std::endl;   // 8
    std::cout << "add(4)  -> " << kth.add(4) << std::endl;   // 8
    std::cout << "（priority_queue 的第三個模板參數 std::greater<int> "
                 "把大頂堆換成小頂堆）" << std::endl;

    // --- 日常實務 1 ---
    std::cout << "\n=== 日常實務 1：泛型設定值轉換 ===" << std::endl;
    int port = 0;
    double ratio = 0.0;
    std::cout << std::boolalpha;
    std::cout << "parse_config(\"8080\", port)   = " << parse_config("8080", port)
              << ", port = " << port << std::endl;
    std::cout << "parse_config(\"3.5\", ratio)   = " << parse_config("3.5", ratio)
              << ", ratio = " << ratio << std::endl;
    int bad = 0;
    std::cout << "parse_config(\"123abc\", bad)  = " << parse_config("123abc", bad)
              << "  <- eof() 檢查攔下了「前段能轉、後段是垃圾」" << std::endl;
    std::cout << "parse_config(\"abc\", bad)     = " << parse_config("abc", bad)
              << std::endl;
    std::cout << "parse_config_or(\"\", 3000)    = "
              << parse_config_or("", 3000) << "  <- 空值套用預設" << std::endl;
    std::cout << "parse_config_or(\"500\", 3000) = "
              << parse_config_or("500", 3000) << std::endl;

    // --- 日常實務 2 ---
    std::cout << "\n=== 日常實務 2：泛型固定容量快取 ===" << std::endl;
    FixedCache<std::string, int> dns_cache(3);
    dns_cache.put("api.internal", 8080);
    dns_cache.put("db.internal", 5432);
    dns_cache.put("cache.internal", 6379);

    int found = 0;
    std::cout << "get(\"db.internal\")   -> "
              << (dns_cache.get("db.internal", found) ? "命中" : "未命中")
              << ", port = " << found << std::endl;
    std::cout << "get(\"unknown.host\")  -> "
              << (dns_cache.get("unknown.host", found) ? "命中" : "未命中")
              << std::endl;

    dns_cache.put("new.internal", 9090);   // 容量已滿，淘汰最舊的 api.internal
    std::cout << "put(\"new.internal\") 後（容量 3，FIFO 淘汰最舊的）：" << std::endl;
    std::cout << "get(\"api.internal\")  -> "
              << (dns_cache.get("api.internal", found) ? "命中" : "未命中（已被淘汰）")
              << std::endl;
    std::cout << "命中 " << dns_cache.hits() << " 次，未命中 "
              << dns_cache.misses() << " 次，目前 " << dns_cache.size()
              << " 筆" << std::endl;

    // 同一份程式碼，完全不同的 key/value 型別
    FixedCache<int, std::string> user_cache(2);
    user_cache.put(10247, "admin");
    user_cache.put(10248, "viewer");
    std::string role;
    std::cout << "FixedCache<int, std::string> get(10247) -> "
              << (user_cache.get(10247, role) ? role : std::string("未命中"))
              << "  <- 同一份原始碼，兩份獨立機器碼" << std::endl;

    // --- 日常實務 3 ---
    std::cout << "\n=== 日常實務 3：泛型序列化（迭代器區間）===" << std::endl;
    std::vector<int> nums{1, 2, 3, 4, 5};
    std::vector<std::string> names{"alice", "bob", "carol"};
    int c_array[] = {100, 200, 300};

    std::cout << "vector<int>       -> " << to_csv(nums.begin(), nums.end())
              << std::endl;
    std::cout << "vector<string>    -> " << to_csv(names.begin(), names.end())
              << std::endl;
    std::cout << "C 陣列            -> " << to_csv(c_array, c_array + 3)
              << "  <- 迭代器介面連 C 陣列都吃得下" << std::endl;
    std::cout << "自訂分隔符（|）   -> " << to_csv(nums.begin(), nums.end(), '|')
              << std::endl;

    std::cout << "\n====================================================" << std::endl;
    std::cout << "重點整理：" << std::endl;
    std::cout << "  1. 泛型的核心不是少打字，而是服務「作者不知道的型別」" << std::endl;
    std::cout << "  2. 函數模板：template<typename T> 讓函數可以處理多種型別" << std::endl;
    std::cout << "  3. 類別模板：template<typename T> class ... 讓容器可以裝任何型別" << std::endl;
    std::cout << "  4. 模板特化：為特定型別提供不同的實作（部分特化僅限類別模板）" << std::endl;
    std::cout << "  5. 非型別模板參數：模板可以接受值（如整數）作為參數" << std::endl;
    std::cout << "  6. 型別推導：先推導、後轉換；推導階段不做隱式轉換" << std::endl;
    std::cout << "  7. template 是編譯期生成程式碼，與 Java 的型別抹除不同" << std::endl;
    std::cout << "  8. 定義要放 header（實例化需看到本體），weak symbol 解決 ODR" << std::endl;
    std::cout << "  9. 代價是 code bloat 與編譯時間，執行期才是零成本" << std::endl;
    std::cout << " 10. STL 容器和演算法都是類別模板和函數模板" << std::endl;
    std::cout << "====================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ====================================================
//   第二課：泛型編程（Generic Programming）概念 - 總複習示範
// ====================================================
//
// --- 示範一：函數模板（my_max）---
// my_max(3, 5): 5
// my_max(3.14, 2.72): 3.14
// my_max('a', 'z'): z
// my_max(string, string): banana
// my_max<double>(10, 3.14): 10  <- 明確指定型別可略過推導
//
// --- 示範二：多型別參數模板 ---
// (42, 3.14)
// (hello, 100)
// (1, A)
//
// --- 示範三：clamp（限制在範圍內）---
// clamp(15, 0, 10): 10
// clamp(-5, 0, 10): 0
// clamp(5, 0, 10): 5
//
// --- 示範四：類別模板（MyPair）---
// MyPair(42, Hello)
// MyPair(3.14, 1)
//
// --- 示範五：泛型 Stack ---
// Stack 頂端: 30
// pop 後頂端: 20
// Stack 大小: 2
// 字串 Stack 頂端: hello
//
// --- 示範六：非型別模板參數（FixedArray）---
// FixedArray<5>: 10 20 30 40 50
// array_size(raw[7]) = 7  <- 從陣列參考的型別中取出 N（std::size 的原理）
//
// --- 示範七：模板特化 ---
// is_equal(5, 5): true
// is_equal("hello", "hello"): true  <- 走 const char* 完全特化，比較的是內容
// 整數型別（int）
// 一般型別
// 指標型別
//
// --- 示範八：型別推導 ---
// 值: 42 | 型別（概略）: i
// 值: 3.14 | 型別（概略）: d
// 值: A | 型別（概略）: c
// 值: STL | 型別（概略）: NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
// （typeid().name() 的輸出是實作定義的，g++ 給 mangled name）
// add(3, 4.5) = 7.5
//
// --- 示範九：auto 型別推導 ---
// auto x: 42
// auto y: 3.14
// auto z: hello
// auto v size: 3
//
// --- 示範十：編譯期計算（Factorial）---
// 5! = 120
// 10! = 3628800
// constexpr 版 5! = 120  <- 現代寫法，可讀性優於模板遞迴
//
// --- 示範十一：多載決議（非模板優先）---
// which_one(42)     -> 非模板版（優先勝出）
// which_one(3.14)   -> 泛型模板版
// which_one<int>(42)-> 泛型模板版  <- 加 <int> 強制走模板
//
// --- 示範十二：enable_if（C++20 concepts 的前身）---
// 這個函數只接受整數！值：123
// safe_sqrt(2.0) = 2  <- static_assert 擋掉非浮點型別
//
// === LeetCode 155. Min Stack ===
// push(-2), push(0), push(-3)
// getMin() = -3
// pop() 後 top() = 0
// pop() 後 getMin() = -2
// MinStack<std::string> getMin() = apple  <- 同一份程式碼，換了元素型別
//
// === LeetCode 703. Kth Largest Element in a Stream ===
// k=3, nums=[4,5,8,2]
// add(3)  -> 4
// add(5)  -> 5
// add(10) -> 5
// add(9)  -> 8
// add(4)  -> 8
// （priority_queue 的第三個模板參數 std::greater<int> 把大頂堆換成小頂堆）
//
// === 日常實務 1：泛型設定值轉換 ===
// parse_config("8080", port)   = true, port = 8080
// parse_config("3.5", ratio)   = true, ratio = 3.5
// parse_config("123abc", bad)  = false  <- eof() 檢查攔下了「前段能轉、後段是垃圾」
// parse_config("abc", bad)     = false
// parse_config_or("", 3000)    = 3000  <- 空值套用預設
// parse_config_or("500", 3000) = 500
//
// === 日常實務 2：泛型固定容量快取 ===
// get("db.internal")   -> 命中, port = 5432
// get("unknown.host")  -> 未命中
// put("new.internal") 後（容量 3，FIFO 淘汰最舊的）：
// get("api.internal")  -> 未命中（已被淘汰）
// 命中 1 次，未命中 2 次，目前 3 筆
// FixedCache<int, std::string> get(10247) -> admin  <- 同一份原始碼，兩份獨立機器碼
//
// === 日常實務 3：泛型序列化（迭代器區間）===
// vector<int>       -> 1,2,3,4,5
// vector<string>    -> alice,bob,carol
// C 陣列            -> 100,200,300  <- 迭代器介面連 C 陣列都吃得下
// 自訂分隔符（|）   -> 1|2|3|4|5
//
// ====================================================
// 重點整理：
//   1. 泛型的核心不是少打字，而是服務「作者不知道的型別」
//   2. 函數模板：template<typename T> 讓函數可以處理多種型別
//   3. 類別模板：template<typename T> class ... 讓容器可以裝任何型別
//   4. 模板特化：為特定型別提供不同的實作（部分特化僅限類別模板）
//   5. 非型別模板參數：模板可以接受值（如整數）作為參數
//   6. 型別推導：先推導、後轉換；推導階段不做隱式轉換
//   7. template 是編譯期生成程式碼，與 Java 的型別抹除不同
//   8. 定義要放 header（實例化需看到本體），weak symbol 解決 ODR
//   9. 代價是 code bloat 與編譯時間，執行期才是零成本
//  10. STL 容器和演算法都是類別模板和函數模板
// ====================================================
