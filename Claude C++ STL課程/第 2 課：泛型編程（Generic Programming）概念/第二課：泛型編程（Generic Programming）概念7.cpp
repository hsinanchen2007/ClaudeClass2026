// =============================================================================
//  第二課：泛型編程（Generic Programming）概念7.cpp
//   —  每個實例化都是一個獨立的型別：vector<int> 與 vector<string> 毫無關係
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     std::vector<int>          numbers;   // 一個型別
//     std::vector<std::string>  words;     // 另一個完全不同的型別
//
//   標準版本：類別模板 C++98 起
//             （本檔另示範 C++17 的 CTAD，故以 -std=c++17 編譯）
//   標頭檔  ：<iostream>、<vector>、<string>
//
//   std::vector 的實際宣告（簡化）：
//     template <typename T, typename Allocator = std::allocator<T>>
//     class vector;
//   —— 它有兩個型別參數，第二個有預設引數，所以平常只寫 vector<int>。
//
// 【詳細解釋 Explanation】
//
// 【1. 「同一個 template」不代表「同一個型別」】
// 這是初學者最容易誤解的一點。vector<int> 與 vector<std::string> 是由同一份
// 原始碼生成的，但它們是**兩個毫無關係的獨立型別**，關係大約等同於
// `struct A` 與 `struct B` —— 只是碰巧長得像。具體後果：
//
//   * 不能互相賦值或轉換，即使元素型別之間可以轉換：
//         std::vector<int> a{1,2};
//         std::vector<double> b = a;      // 編譯錯誤
//     本機 g++ 15.2 實測錯誤訊息：
//         error: conversion from 'vector<int>' to non-scalar type
//                'vector<double>' requested
//     注意 int 明明可以隱式轉 double，但**容器不會因此可以轉換** ——
//     模板實例化之間沒有繼承關係、沒有隱式轉換路徑。
//
//   * 函式參數寫 vector<int> 就只吃 vector<int>。要寫一個「吃任何 vector」
//     的函式，函式本身必須也是模板：
//         template <typename T> void f(const std::vector<T>& v);
//
//   * 各自的 static 成員互相獨立（見下）。
//
// 【2. static 成員是 per-instantiation，不是 per-template】
// 若類別模板有 static 資料成員，**每一個實例化各有一份**：
//     template <typename T> struct Counter { static int n; };
//     Counter<int>::n    與   Counter<double>::n   是兩個不同的變數
// 本機實測：分別設為 5 與 99，讀回來就是 5 與 99，互不影響。
//
// 這個性質常被用來做「每型別各一份」的註冊表或 ID 產生器，是許多
// type-erasure 與反射框架的基礎技巧。反過來說，如果你以為 static 是
// 「整個模板共用一份」而拿它當全域計數器，行為會完全不符預期。
//
// 【3. 這也是 code bloat 的根源】
// 既然每個實例化都是獨立型別、擁有獨立的成員函式，那麼用了 vector<int>、
// vector<double>、vector<std::string>，就等於在二進位檔裡放了三套 vector 的
// 程式碼。這正是概念2.cpp 量到的現象（本機實測：同一個類別模板從 1 種型別
// 擴到 200 種型別，目的檔由 1,744 bytes 膨脹到 71,600 bytes，g++ 15.2 -O0）。
//
// 標準庫對此有個著名的對策：`std::vector<bool>` 是一個**特化**版本，
// 用位元壓縮儲存以節省空間。但它因此不是真正的容器（operator[] 回傳的是
// 代理物件而非 bool&），是 C++ 標準庫公認的設計失誤，實務上常被建議
// 改用 std::vector<char> 或 std::deque<bool>。這個例子說明：
// 「為特定型別做特化」很強大，但也可能破壞泛型介面的一致性。
//
// 【概念補充 Concept Deep Dive】
//
// (A) sizeof(vector<T>) 不隨 T 改變
//   直覺上「裝 string 的 vector 應該比裝 int 的大」，但實際上
//   本機 g++ 15.2 / libstdc++ 實測 sizeof(std::vector<int>) 為 24 bytes，
//   而 vector<std::string> 也是 24 bytes。原因是 vector 物件本身只存三個
//   指標（begin / end / capacity end），元素全部在堆積上。
//   （24 bytes 是 libstdc++ x86-64 的實作細節，不是標準保證；
//     MSVC 等其他實作可能不同。）
//   所以「不同實例化型別不同」指的是**型別身分與成員函式**不同，
//   不必然是物件大小不同。
//
// (B) CTAD 讓你少寫型別，但型別依然是被決定的
//   C++17 起可以寫 std::vector v{1, 2, 3};，編譯器推導出 vector<int>。
//   本機以 -pedantic-errors 實測：這行在 -std=c++14 失敗、-std=c++17 通過。
//   重點是 CTAD 只是**省略書寫**，v 的型別仍然在編譯期被固定為 vector<int>，
//   之後不可能塞 std::string 進去。這與 Python 的 list 有本質差異：
//   C++ 容器的元素型別是型別系統的一部分，不是執行期的性質。
//
// (C) 想寫「吃任何容器」的函式該怎麼辦
//   三種層次，抽象程度遞增：
//     1) template <typename T> void f(const std::vector<T>&);   // 只吃 vector
//     2) template <typename C> void f(const C& c);              // 吃任何容器
//     3) template <typename It> void f(It first, It last);      // 吃任何區間
//   STL 演算法一律採用第 3 種（iterator pair），因為它連 C 陣列、
//   自訂資料結構、甚至輸入串流都能吃 —— 這是第三課「六大組件」的核心設計。
//
// 【注意事項 Pay Attention】
// 1. vector<int> 不能賦值給 vector<double>，即使 int 能轉 double。
//    要轉換必須逐元素進行，例如用 std::transform 或
//    vector<double> b(a.begin(), a.end());（走的是 iterator 範圍建構子）。
// 2. vector<Derived> 與 vector<Base> 之間**沒有**任何關係，即使
//    Derived 繼承自 Base。把 vector<Derived*> 當 vector<Base*> 傳遞會編譯失敗；
//    這是刻意的設計（若允許，就能往裡面塞別的 Base 衍生型別而破壞型別安全）。
// 3. static 成員是每個實例化各一份。用它當「全模板共用計數器」會得到
//    完全不同的結果。
// 4. std::vector<bool> 是特化版本，行為與其他 vector<T> 不一致
//    （operator[] 回傳代理物件、不能取元素位址、auto& 綁定行為不同）。
//    需要「一堆 bool」時通常建議改用 std::vector<char>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】模板實例化與型別身分
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::vector<int> 和 std::vector<double> 是同一個類別嗎？
//        既然 int 可以隱式轉 double，容器可以互相賦值嗎？
//     答：不是同一個類別，是兩個完全獨立的型別，彼此沒有繼承或轉換關係。
//         不能互相賦值 —— 本機實測錯誤訊息為
//         conversion from 'vector<int>' to non-scalar type 'vector<double>' requested。
//         元素型別可以轉換，不代表容器型別可以轉換。
//     追問：那要怎麼把 vector<int> 轉成 vector<double>？
//         → 逐元素轉換，例如 std::vector<double> b(a.begin(), a.end());
//           或用 std::transform。
//
// 🔥 Q2. 類別模板裡的 static 成員，是整個模板共用一份，還是每個實例化各一份？
//     答：每個實例化各一份。Counter<int>::n 與 Counter<double>::n 是兩個
//         獨立的變數（本機實測分別設 5 與 99，互不影響）。
//     追問：這個特性可以拿來做什麼？
//         → 「每個型別各一份」的註冊表、型別 ID 產生器、per-type 統計，
//           是許多序列化與反射框架的基礎技巧。
//
// ⚠️ 陷阱. Derived 繼承自 Base，那 vector<Derived*> 可以傳給
//          接收 vector<Base*> 的函式嗎？
//     答：不行，編譯失敗。模板實例化之間沒有繼承關係 ——
//         Derived* 與 Base* 之間的轉換關係，完全不會傳遞到
//         vector<Derived*> 與 vector<Base*> 身上。
//     為什麼會錯：把元素的 is-a 關係誤以為會「繼承」到容器上（這種性質叫
//         covariance，共變）。C++ 刻意不提供，因為它會破壞型別安全：
//         若能把 vector<Derived*> 當 vector<Base*>，就能往裡面 push_back
//         一個「別的 Base 衍生型別」，之後原本的持有者取出來就拿到錯的型別。
//         Java 的陣列允許共變，代價正是執行期的 ArrayStoreException。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>

// static 成員 per-instantiation 的示範
template <typename T>
struct TypeCounter {
    static int instantiation_id;
};

// 每個實例化各有一份定義
template <typename T>
int TypeCounter<T>::instantiation_id = 0;

// 「吃任何 vector」必須自己也是模板
template <typename T>
std::size_t count_elements(const std::vector<T>& v) {
    return v.size();
}

int main() {
    // vector<int> 是一個型別
    // vector<std::string> 是另一個型別
    // 它們是從同一個 template 實例化出來的不同類別
    std::cout << "=== 同一個 template，不同的型別 ===" << std::endl;

    std::vector<int> numbers = {1, 2, 3, 4, 5};
    std::vector<std::string> words = {"Hello", "World"};

    std::cout << "numbers[0] = " << numbers[0] << std::endl;
    std::cout << "words[0] = " << words[0] << std::endl;

    std::cout << "\n=== 不能互相轉換（即使 int 能轉 double）===" << std::endl;
    // std::vector<double> bad = numbers;   // 編譯錯誤！
    //   error: conversion from 'vector<int>' to non-scalar type
    //          'vector<double>' requested
    std::cout << "std::vector<double> bad = numbers;  無法編譯" << std::endl;
    // 正確做法：逐元素轉換（走 iterator 範圍建構子）
    std::vector<double> converted(numbers.begin(), numbers.end());
    std::cout << "逐元素轉換後 converted[0] = " << converted[0]
              << "（型別已是 double）" << std::endl;

    std::cout << "\n=== sizeof：物件本身不隨元素型別改變 ===" << std::endl;
    std::cout << "sizeof(std::vector<int>)         = "
              << sizeof(std::vector<int>) << std::endl;
    std::cout << "sizeof(std::vector<std::string>) = "
              << sizeof(std::vector<std::string>) << std::endl;
    std::cout << "（vector 物件只存三個指標，元素在堆積上；"
              << "此為 libstdc++ x86-64 實測值，非標準保證）" << std::endl;

    std::cout << "\n=== static 成員是 per-instantiation ===" << std::endl;
    TypeCounter<int>::instantiation_id = 5;
    TypeCounter<double>::instantiation_id = 99;
    std::cout << "TypeCounter<int>::instantiation_id    = "
              << TypeCounter<int>::instantiation_id << std::endl;
    std::cout << "TypeCounter<double>::instantiation_id = "
              << TypeCounter<double>::instantiation_id
              << "   <- 兩個獨立的變數，互不影響" << std::endl;

    std::cout << "\n=== 要吃任何 vector，函式自己也得是模板 ===" << std::endl;
    std::cout << "count_elements(numbers) = " << count_elements(numbers)
              << std::endl;
    std::cout << "count_elements(words)   = " << count_elements(words)
              << std::endl;

    std::cout << "\n=== CTAD（C++17）：省略型別書寫，但型別依然固定 ===" << std::endl;
    std::vector v{10, 20, 30};      // 推導成 std::vector<int>
    std::cout << "std::vector v{10, 20, 30}; -> 推導為 vector<int>, v.size() = "
              << v.size() << std::endl;
    std::cout << "（-pedantic-errors 實測：此語法 C++14 失敗、C++17 通過）"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念7.cpp -o concept7

// （vector 物件只存三個指標，元素在堆積上；此為 libstdc++ x86-64 實測值，非標準保證）

// === 預期輸出 ===
// === 同一個 template，不同的型別 ===
// numbers[0] = 1
// words[0] = Hello
//
// === 不能互相轉換（即使 int 能轉 double）===
// std::vector<double> bad = numbers;  無法編譯
// 逐元素轉換後 converted[0] = 1（型別已是 double）
//
// === sizeof：物件本身不隨元素型別改變 ===
// sizeof(std::vector<int>)         = 24
// sizeof(std::vector<std::string>) = 24
//
// === static 成員是 per-instantiation ===
// TypeCounter<int>::instantiation_id    = 5
// TypeCounter<double>::instantiation_id = 99   <- 兩個獨立的變數，互不影響
//
// === 要吃任何 vector，函式自己也得是模板 ===
// count_elements(numbers) = 5
// count_elements(words)   = 2
//
// === CTAD（C++17）：省略型別書寫，但型別依然固定 ===
// std::vector v{10, 20, 30}; -> 推導為 vector<int>, v.size() = 3
// （-pedantic-errors 實測：此語法 C++14 失敗、C++17 通過）
