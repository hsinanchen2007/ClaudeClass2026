// =============================================================================
//  第 1.1 章：auto 關鍵字 — 自動型別推導（C++11 基準示範檔）
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/auto
//
// 【主題資訊 Information】
//   語法：  auto        識別字 = 初始化式;   // 按值推導
//           auto&       識別字 = 初始化式;   // 左值參考
//           const auto& 識別字 = 初始化式;   // 常數左值參考
//           auto&&      識別字 = 初始化式;   // 轉發參考(forwarding reference)
//           auto*       識別字 = 初始化式;   // 明確標示為指標
//   標準版本：C++11（本檔全部內容都在 C++11 範圍內，可用 -std=c++11 編譯）
//   標頭檔：  不需要。auto 是語言關鍵字，不是函式庫設施
//   執行期成本：零。auto 純粹是編譯期的型別代換，不產生任何額外指令
//
//   ※ 本檔刻意「只用 C++11」，用途是當作版本基準線。
//      auto 之後在 C++14 / C++17 / C++20 的演進（回傳型別推導、泛型 lambda、
//      template<auto>、CTAD、簡化函式模板…）請看同章的 summary.cpp。
//
// 【詳細解釋 Explanation】
//
// 【1. auto 解決的不是「懶得打字」，而是「型別無法命名」】
//   大多數教材把 auto 介紹成「少打幾個字的語法糖」，這低估了它。C++11 引入
//   auto 的真正動機，是「有些型別你根本寫不出來」：
//
//     * lambda 的閉包型別(closure type)是編譯器產生的匿名型別，
//       標準明文規定它是 unique unnamed non-union class type，
//       在 C++11 沒有 auto 就完全無法宣告一個具名變數去接住它。
//     * 樣板運算中間結果的型別可能是一長串巢狀相依名稱，
//       例如 map<string, vector<int>>::iterator。
//     * 運算式模板(expression template)函式庫（Eigen、Blaze）刻意回傳
//       內部代理型別，使用者不該、也不需要知道它叫什麼。
//
//   換句話說：auto 讓「型別」從一個必須由人類書寫的東西，
//   變成一個由編譯器計算出來的東西。這是型別系統的分工調整，不只是縮寫。
//
// 【2. auto 的推導規則 = 樣板參數推導規則（這是理解一切的鑰匙）】
//   標準把 auto 的推導直接定義成「比照函式樣板的參數推導」：把
//
//       auto x = expr;
//
//   想像成有一個虛構的樣板函式
//
//       template <typename T> void f(T param);   // auto 對應 T
//       f(expr);                                  // 用 expr 去推導 T
//
//   則 T 推導出什麼，auto 就是什麼。宣告子上寫的 &、const、* 則對應到
//   param 的形狀（T、T&、const T&、T*）。由此可以「推導」出全部規則，
//   不必死背：
//
//     * auto x = expr;        對應 f(T param)        → 按值傳遞
//         → 因為是複製，來源的「頂層 const」與「參考」對副本毫無意義，
//           兩者都被丟棄；陣列與函式退化(decay)成指標。
//     * auto& x = expr;       對應 f(T& param)       → 綁定，不複製
//         → 沒有複製就沒有理由丟棄任何東西，const 完整保留；
//           陣列不退化，int[5] 綁成 int(&)[5]，sizeof 仍是整個陣列。
//     * const auto& x = expr; 對應 f(const T& param) → 唯讀綁定
//         → 可以綁左值、右值、甚至需要隱式轉換的暫時物件。
//     * auto&& x = expr;      對應 f(T&& param)      → 轉發參考
//         → 左值進來是左值參考，右值進來是右值參考（參考摺疊）。
//
//   唯一的例外是大括號初始化：auto x = {1, 2, 3}; 會推導成
//   std::initializer_list<int>，但同一組 {1, 2, 3} 傳給真正的樣板
//   template <typename T> void f(T) 卻推不出 T——它是 non-deduced context。
//   這是 auto 與樣板推導唯一真正分家的地方。
//
// 【3. 「頂層 const」與「底層 const」為什麼要分清楚】
//   const 可以修飾兩個不同的東西，而 auto 只丟棄其中一個：
//
//       const int   ci  = 42;   // 頂層 const：ci 這個「物件」不可改
//       const int*  pci = &ci;  // 底層 const：指向的「目標」不可改，指標本身可改
//       int* const  cpi = &i;   // 頂層 const：指標本身不可改，目標可改
//
//   auto 按值推導丟棄的永遠只有「頂層」那一層，因為副本是新的物件，
//   來源不可改跟副本能不能改毫無關係。底層 const 則必須保留——
//   丟掉它等於憑空取得寫入權限，會直接破壞 const 正確性。所以：
//
//       auto a = pci;   // const int*（底層 const 保留！不是 int*）
//       auto b = cpi;   // int*      （頂層 const 丟棄）
//
// 【概念補充 Concept Deep Dive】
//
//   (A) auto 完全不影響產生的機器碼
//     auto 在編譯的「語意分析」階段就被換成具體型別，之後的中介碼、
//     最佳化、組語產出完全看不到 auto 存在過。
//         auto i = 42;  與  int i = 42;  產生位元組完全相同的目的碼。
//     所以「用 auto 會不會比較慢」這個問題本身就問錯了層次。
//     反倒是「不用 auto」比較容易變慢——見下面 (C) 的隱形複製。
//
//   (B) 為什麼 auto 一定要有初始化式
//     推導的資訊來源只有初始化式一個。沒有初始化式，編譯器手上沒有任何
//     可用來解方程式的已知數，因此 `auto x;` 不是「先放著之後再決定」，
//     而是根本無解 → 編譯錯誤。這也是為什麼 auto 不能用在
//     非靜態資料成員（宣告點沒有初始化式可推）、不能用在陣列宣告子。
//
//   (C) auto 最實際的效能價值：讓「隱形複製」現形
//     經典案例是 std::map 的走訪。map<K,V> 的 value_type 是
//     std::pair<const K, V>——注意 key 帶 const。若寫成
//
//         for (const std::pair<std::string, int>& kv : m)   // 少了 const！
//
//     型別不匹配（pair<string,int> vs pair<const string,int>），編譯器
//     不會報錯，而是「合法地」為每個元素建立一個暫時物件再綁上去——
//     於是每一圈都複製一次 std::string。改寫成 const auto& 就自動正確，
//     零複製。用 auto 反而比手寫型別更不容易出效能問題。
//     （本機實測 sizeof(std::string) = 32 bytes，每圈多一次堆積配置。）
//
//   (D) 陣列退化在記憶體層面到底發生什麼事
//     int arr[5] 在堆疊上是連續 20 bytes（本機實測 sizeof(int)=4）。
//     `auto p = arr;` 產生的 int* 只是一個 8 bytes 的位址值（本機 64 位元），
//     元素個數這個資訊在型別中被永久抹除，之後 sizeof(p)/sizeof(p[0])
//     這種老把戲會算出 8/4 = 2 這種錯誤答案。
//     `auto& r = arr;` 綁定則不複製也不退化，型別仍是 int(&)[5]，
//     sizeof(r) 仍是 20，長度資訊還在。
//
// 【注意事項 Pay Attention】
//   1. auto 丟棄頂層 const 與參考。要保留必須自己寫出 const / &。
//   2. auto s = "Hello"; 推導成 const char*，不是 std::string。
//      需要 std::string 要明確建構；C++14 起可用 "Hello"s 字面值後綴。
//   3. auto 搭配 std::vector<bool> 會拿到代理型別
//      std::vector<bool>::reference，不是 bool（本機實測已驗證）。
//      這個代理物件的生命週期綁在容器上，容器改動後再使用結果不保證正確。
//   4. 同一個宣告式裡的多個 auto 變數必須推導出同一型別，
//      auto a = 1, b = 2.0; 是編譯錯誤。
//   5. auto 不能用於：非靜態資料成員、陣列宣告子、無初始化式的宣告。
//      函式參數要到 C++20 的簡化函式樣板才允許（本檔為 C++11，不示範）。
//   6. 整數除法陷阱與 auto 無關但常一起出現：auto r = 10 / 3; 得到 int 3，
//      不是 3.333。auto 忠實反映運算式的型別，不會幫你「猜」你想要浮點數。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】auto 型別推導（C++11）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto 的推導規則是自成一套，還是沿用既有規則？
//     答：沿用函式樣板的參數推導規則，把 auto 當成樣板參數 T。
//         `auto x = e` 等同按值傳參，丟棄頂層 const 與參考、陣列與函式退化
//         成指標；`auto&`／`const auto&` 是綁定，const 保留且不退化；
//         `auto&&` 是轉發參考。唯一例外是大括號初始化會推成
//         std::initializer_list，而樣板推導對 {} 是 non-deduced context。
//     追問：那 `auto& r = arr;` 的 sizeof 是多少？
//         → 仍是整個陣列的大小（本機 20 bytes），因為綁定不退化。
//
// 🔥 Q2. auto 會丟掉 const，那 `auto p = pci;`（pci 是 const int*）
//         推出來是 int* 還是 const int*？
//     答：const int*。auto 丟棄的只有「頂層 const」，也就是被宣告物件本身
//         的常數性。pci 這個「指標變數」本身不是 const（頂層沒有 const），
//         const 修飾的是它指向的目標，屬於「底層 const」，必須保留。
//         若丟掉底層 const 就等於憑空多出寫入權限，const 正確性會瓦解。
//     追問：那 `int* const cpi` 用 auto 推導呢？
//         → int*。這次 const 在頂層（指標自己不可改），複製時被丟棄。
//
// 🔥 Q3. 為什麼說 auto 有時反而比手寫型別「更快」？
//     答：因為手寫型別可能與實際型別不完全相同，觸發隱式轉換而產生暫時物件。
//         最經典的是走訪 std::map<std::string,int> 時寫成
//         `const std::pair<std::string,int>&`——真正的 value_type 是
//         pair<const std::string,int>，型別不符，於是每圈複製一個
//         暫時 pair（含一次 std::string 堆積配置）。改用 const auto&
//         必定精準命中，零複製。
//     追問：編譯器為什麼不警告？
//         → 因為那是完全合法的程式碼：常數參考本來就允許綁定暫時物件，
//           並延長其生命週期。語意正確，只是慢。
//
// ⚠️ 陷阱. `std::vector<bool> v{true}; auto b = v[0];` 之後
//          `b = false;` 會發生什麼？
//     答：b 的型別不是 bool，而是 std::vector<bool>::reference 代理物件
//         （本機實測 is_same<decltype(b), bool> 為 0）。對它賦值不是改
//         一個獨立的區域副本，而是回頭寫進 v 的內部位元。若原容器已被
//         改動或銷毀，再使用這個代理的結果不受保證。
//     為什麼會錯：腦中把 vector<T> 一律當成「元素型別就是 T」。但
//         vector<bool> 是標準明文規定的特化，為了省空間用位元打包儲存，
//         operator[] 無法回傳 bool&（沒有辦法取得單一位元的位址），
//         只好回傳代理物件。要拿到真正的 bool 請寫
//         `auto b = static_cast<bool>(v[0]);` 或直接寫 `bool b = v[0];`。
// ═══════════════════════════════════════════════════════════════════════════

// 檔案名稱: 第 1.1 章：auto 關鍵字 — 自動型別推導1.cpp
// 說明: 展示 C++11 auto 關鍵字的各種用法

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <typeinfo>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 INI／.env 風格的設定檔行
//
// 場景：幾乎所有服務啟動時都要讀設定檔，格式通常是
//         # 這是註解
//         timeout = 30
//         host=127.0.0.1     # 行尾註解
//       需要處理的雜訊有：註解行、行尾註解、等號兩側空白、空行。
//
// 為什麼用到本主題：這段程式碼裡每一個中間結果的型別
//   （std::string::size_type、std::map 的迭代器）都冗長且與演算法無關，
//   用 auto 讓讀者的注意力留在「解析邏輯」而不是「型別拼字」上。
//   注意 pos 這類變數用 auto 還有正確性上的好處：find 回傳的是
//   size_type（無號），手寫成 int 會在 64 位元平台被截斷。
//
// 本函式全程只用 C++11 設施（無 structured bindings、無 string_view）。
// -----------------------------------------------------------------------------
static std::string trim(const std::string& s)
{
    const std::string ws = " \t\r\n";
    auto first = s.find_first_not_of(ws);          // auto → std::string::size_type
    if (first == std::string::npos) return "";     // 整行都是空白
    auto last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

static std::map<std::string, std::string> parseConfig(const std::vector<std::string>& lines)
{
    std::map<std::string, std::string> cfg;

    for (auto it = lines.begin(); it != lines.end(); ++it)   // C++11 迭代器寫法
    {
        auto line = trim(*it);
        if (line.empty() || line[0] == '#') continue;        // 空行與整行註解

        // 去掉行尾註解（# 之後的內容）
        auto hash = line.find('#');
        if (hash != std::string::npos) line = trim(line.substr(0, hash));
        if (line.empty()) continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;               // 沒有 '=' 視為格式錯誤，略過

        auto key   = trim(line.substr(0, eq));
        auto value = trim(line.substr(eq + 1));
        if (!key.empty()) cfg[key] = value;
    }
    return cfg;
}

int main()
{
    // ===== 1. 基本型別推導 =====
    std::cout << "===== 1. 基本型別推導 =====\n";

    auto i = 42;           // int
    auto d = 3.14;         // double
    auto f = 3.14f;        // float (因為有 f 後綴)
    auto c = 'A';          // char
    auto b = true;         // bool
    auto ll = 100LL;       // long long (因為有 LL 後綴)

    std::cout << "i = " << i << " (int)\n";
    std::cout << "d = " << d << " (double)\n";
    std::cout << "f = " << f << " (float)\n";
    std::cout << "c = " << c << " (char)\n";
    std::cout << "b = " << std::boolalpha << b << " (bool)\n";
    std::cout << "ll = " << ll << " (long long)\n\n";

    // ===== 2. 字串型別推導 =====
    std::cout << "===== 2. 字串型別推導 =====\n";

    auto str1 = "Hello";              // const char* (字串字面值)
    auto str2 = std::string("World"); // std::string

    std::cout << "str1 = " << str1 << " (const char*)\n";
    std::cout << "str2 = " << str2 << " (std::string)\n\n";

    // ===== 3. 容器迭代器 — auto 最常見的用途 =====
    std::cout << "===== 3. 容器迭代器 =====\n";

    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // C++03 寫法 (冗長)
    // std::vector<int>::iterator it = numbers.begin();

    // C++11 使用 auto (簡潔)
    std::cout << "vector 內容: ";
    for (auto it = numbers.begin(); it != numbers.end(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << "\n\n";

    // ===== 4. 複雜型別的簡化 =====
    std::cout << "===== 4. 複雜型別的簡化 =====\n";

    std::map<std::string, std::vector<int>> data;
    data["Alice"] = {90, 85, 88};
    data["Bob"] = {78, 82, 80};

    // 沒有 auto 的寫法:
    // std::map<std::string, std::vector<int>>::iterator mapIt = data.begin();

    // 使用 auto 的寫法:
    for (auto mapIt = data.begin(); mapIt != data.end(); ++mapIt)
    {
        std::cout << mapIt->first << " 的成績: ";
        for (auto score : mapIt->second)  // 範圍式 for 也用了 auto
        {
            std::cout << score << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // ===== 5. const 與參考的處理 =====
    std::cout << "===== 5. const 與參考的處理 =====\n";

    int x = 100;
    const int cx = 200;
    int& rx = x;

    auto a1 = x;     // int (複製)
    auto a2 = cx;    // int (頂層 const 被忽略，是複製)
    auto a3 = rx;    // int (參考被忽略，是複製)

    // 明確保留 const 或參考
    const auto a4 = x;     // const int
    auto& a5 = x;          // int&
    const auto& a6 = x;    // const int&
    auto& a7 = cx;         // const int& (底層 const 被保留)

    // 用 static_assert 在「編譯期」證明上面的推導結果，
    // 這比印出 typeid().name() 可靠（typeid 會丟掉 const 與參考）。
    static_assert(std::is_same<decltype(a1), int>::value,        "a1 應為 int");
    static_assert(std::is_same<decltype(a2), int>::value,        "a2 應為 int（頂層 const 被丟棄）");
    static_assert(std::is_same<decltype(a3), int>::value,        "a3 應為 int（參考被丟棄）");
    static_assert(std::is_same<decltype(a4), const int>::value,  "a4 應為 const int");
    static_assert(std::is_same<decltype(a5), int&>::value,       "a5 應為 int&");
    static_assert(std::is_same<decltype(a6), const int&>::value, "a6 應為 const int&");
    static_assert(std::is_same<decltype(a7), const int&>::value, "a7 應為 const int&（底層 const 保留）");

    // 驗證 a5 確實是參考
    a5 = 999;
    std::cout << "修改 a5 後, x = " << x << " (證明 a5 是 x 的參考)\n";

    // 對照：a4 是「複製」，不會跟著變；a6 是「參考」，會跟著變。
    // 這一組對照是理解「auto 按值 vs auto& 綁定」最直接的證據。
    std::cout << "a4 (const auto  = 複製) = " << a4 << " (停留在賦值前的舊值)\n";
    std::cout << "a6 (const auto& = 參考) = " << a6 << " (跟著 x 一起變成新值)\n";
    std::cout << "a1 = " << a1 << ", a2 = " << a2 << ", a3 = " << a3
              << " (三者都是獨立副本)\n";
    std::cout << "a7 (auto& 綁到 const int) = " << a7 << " (唯讀，無法透過 a7 修改 cx)\n\n";

    // ===== 6. 指標的處理 =====
    std::cout << "===== 6. 指標的處理 =====\n";

    int value = 42;
    int* ptr = &value;

    auto p1 = ptr;    // int* (指標被保留)
    auto* p2 = ptr;   // int* (明確表示是指標，效果相同)

    std::cout << "*p1 = " << *p1 << "\n";
    std::cout << "*p2 = " << *p2 << "\n";

    // 底層 const 不會被丟棄：pci 指向常數，複製出來的指標仍指向常數
    const int* pci = &value;
    auto pc = pci;                 // const int*，不是 int*
    static_assert(std::is_same<decltype(pc), const int*>::value,
                  "底層 const 必須保留");
    std::cout << "auto pc = pci;  → const int* (底層 const 保留，不可透過 pc 寫入)\n\n";

    // ===== 7. 陣列退化為指標 =====
    std::cout << "===== 7. 陣列退化為指標 =====\n";

    int arr[5] = {10, 20, 30, 40, 50};

    auto arrAuto = arr;     // int* (陣列退化為指標)
    auto& arrRef = arr;     // int(&)[5] (參考保留陣列型別)

    std::cout << "arrAuto[0] = " << arrAuto[0] << "\n";
    std::cout << "sizeof(arr) = " << sizeof(arr) << " bytes\n";
    std::cout << "sizeof(arrAuto) = " << sizeof(arrAuto) << " bytes (指標大小)\n";
    std::cout << "sizeof(arrRef) = " << sizeof(arrRef) << " bytes (保留陣列大小)\n";
    std::cout << "  ※ 以上 sizeof 數值為實作定義，本機(x86-64 Linux, g++ 15.2)實測:\n";
    std::cout << "     sizeof(int)=" << sizeof(int)
              << ", sizeof(int*)=" << sizeof(int*) << "\n\n";

    // ===== 8. 同一行宣告多個變數 =====
    std::cout << "===== 8. 同一行宣告多個變數 =====\n";

    // 所有變數必須推導為相同型別
    auto v1 = 1, v2 = 2, v3 = 3;  // 全部是 int，合法
    // auto v4 = 1, v5 = 3.14;    // 錯誤！int 和 double 不同型別

    std::cout << "v1 = " << v1 << ", v2 = " << v2 << ", v3 = " << v3 << "\n\n";

    // ===== 9. auto 與運算式結果 =====
    std::cout << "===== 9. auto 與運算式結果 =====\n";

    int a = 10;
    int b_val = 3;

    auto sum = a + b_val;        // int
    auto division = a / b_val;   // int (整數除法)
    auto realDiv = a / 3.0;      // double (因為 3.0 是 double)

    std::cout << "10 + 3 = " << sum << " (int)\n";
    std::cout << "10 / 3 = " << division << " (int，整數除法)\n";
    std::cout << "10 / 3.0 = " << realDiv << " (double)\n\n";

    // ===== 10. 日常實務：解析設定檔 =====
    std::cout << "===== 10. 日常實務: 解析設定檔 =====\n";

    std::vector<std::string> lines;
    lines.push_back("# 服務設定檔");
    lines.push_back("");
    lines.push_back("host = 127.0.0.1");
    lines.push_back("port=8080          # 監聽埠");
    lines.push_back("  timeout  =  30  ");
    lines.push_back("這行沒有等號會被略過");
    lines.push_back("log_level = debug");

    auto cfg = parseConfig(lines);
    std::cout << "解析出 " << cfg.size() << " 組設定:\n";
    for (auto it = cfg.begin(); it != cfg.end(); ++it)
    {
        std::cout << "  [" << it->first << "] = [" << it->second << "]\n";
    }

    return 0;
}



// // 1. 函式參數 (C++11/14 不行，C++20 才可以)
// void func(auto x);  // C++11/14 錯誤，C++20 合法

// // 2. 類別的非靜態成員變數
// class MyClass
// {
//     auto value = 10;  // 錯誤！
// };

// // 3. 沒有初始化
// auto x;  // 錯誤！必須初始化

// // 4. 陣列宣告
// auto arr[5] = {1, 2, 3, 4, 5};  // 錯誤！

// 編譯: g++ -std=c++11 -Wall -Wextra "第 1.1 章：auto 關鍵字 — 自動型別推導1.cpp" -o auto_demo
//
// ※ 本檔已用 g++ -std=c++11 -pedantic-errors 驗證，全檔不含任何 C++11 以後的語法。

// === 預期輸出 ===
// ===== 1. 基本型別推導 =====
// i = 42 (int)
// d = 3.14 (double)
// f = 3.14 (float)
// c = A (char)
// b = true (bool)
// ll = 100 (long long)
//
// ===== 2. 字串型別推導 =====
// str1 = Hello (const char*)
// str2 = World (std::string)
//
// ===== 3. 容器迭代器 =====
// vector 內容: 1 2 3 4 5
//
// ===== 4. 複雜型別的簡化 =====
// Alice 的成績: 90 85 88
// Bob 的成績: 78 82 80
//
// ===== 5. const 與參考的處理 =====
// 修改 a5 後, x = 999 (證明 a5 是 x 的參考)
// a4 (const auto  = 複製) = 100 (停留在賦值前的舊值)
// a6 (const auto& = 參考) = 999 (跟著 x 一起變成新值)
// a1 = 100, a2 = 200, a3 = 100 (三者都是獨立副本)
// a7 (auto& 綁到 const int) = 200 (唯讀，無法透過 a7 修改 cx)
//
// ===== 6. 指標的處理 =====
// *p1 = 42
// *p2 = 42
// auto pc = pci;  → const int* (底層 const 保留，不可透過 pc 寫入)
//
// ===== 7. 陣列退化為指標 =====
// arrAuto[0] = 10
// sizeof(arr) = 20 bytes
// sizeof(arrAuto) = 8 bytes (指標大小)
// sizeof(arrRef) = 20 bytes (保留陣列大小)
//   ※ 以上 sizeof 數值為實作定義，本機(x86-64 Linux, g++ 15.2)實測:
//      sizeof(int)=4, sizeof(int*)=8
//
// ===== 8. 同一行宣告多個變數 =====
// v1 = 1, v2 = 2, v3 = 3
//
// ===== 9. auto 與運算式結果 =====
// 10 + 3 = 13 (int)
// 10 / 3 = 3 (int，整數除法)
// 10 / 3.0 = 3.33333 (double)
//
// ===== 10. 日常實務: 解析設定檔 =====
// 解析出 4 組設定:
//   [host] = [127.0.0.1]
//   [log_level] = [debug]
//   [port] = [8080]
//   [timeout] = [30]
