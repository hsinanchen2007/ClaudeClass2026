// =============================================================================
//  第二課：泛型編程（Generic Programming）概念6.cpp
//   —  類別模板：STL 容器的實作原理，以及成員函式的「用到才實例化」
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     template <typename T>
//     class Box { ... };
//
//     Box<int> b;          // 使用時必須指定型別引數
//     Box b2;              // C++17 前一律非法；C++17 起若能從建構子推導則合法（CTAD）
//
//   標準版本：類別模板 C++98 起；CTAD 為 C++17
//             （本機以 -pedantic-errors 實測：CTAD 在 -std=c++14 失敗、
//               -std=c++17 通過）
//   標頭檔  ：<iostream>、<string>、<stdexcept>
//
//   本檔與標準庫的對應：Box<T> 基本上是一個簡化版的 std::optional<T>（C++17），
//   後者定義於 <optional>，並提供 has_value()、value()、value_or() 等介面。
//
// 【詳細解釋 Explanation】
//
// 【1. 類別模板是「產生類別的藍圖」】
// 與函式模板完全同構：Box 本身不是類別，Box<int>、Box<std::string>、
// Box<double> 才是三個真正的、彼此獨立的類別。編譯器對每個用到的型別引數
// 各生成一份類別定義，各自有自己的成員佈局與成員函式。
//
// 這正是 STL 容器的實作方式。std::vector 的宣告是
//     template <typename T, typename Allocator = std::allocator<T>> class vector;
// 所以 vector<int> 與 vector<std::string> 是兩個不同的類別 ——
// 這件事的完整後果是概念7.cpp 的主題。
//
// 【2. 類別模板與函式模板最大的使用差異：推導】
// 函式模板可以從實參推導型別，呼叫時通常不必寫 <>：
//     my_swap(x, y);              // T 自動推導
// 但類別模板在 C++17 之前**完全沒有推導**，一定要寫出型別引數：
//     Box<int> b;                 // 必須明寫
//     std::pair<int, double> p;   // 必須明寫
// 這就是為什麼 C++17 之前存在大量 make_xxx 輔助函式（std::make_pair、
// std::make_tuple）—— 它們是**函式**模板，可以推導，藉此繞過類別模板不能推導
// 的限制：
//     auto p = std::make_pair(1, 3.14);   // 不必寫 <int, double>
//
// C++17 加入 CTAD（Class Template Argument Deduction，類別模板引數推導）後，
// 這個限制大幅放寬：
//     std::pair p{1, 3.14};       // C++17 起可省略 <int, double>
//     std::vector v{1, 2, 3};     // 推導成 vector<int>
// 本機以 -pedantic-errors 實測：這兩行在 -std=c++14 失敗、-std=c++17 通過。
// 也因此 std::make_pair 在 C++17 之後大多可以功成身退了。
//
// 【3. 成員函式是「用到才實例化」（lazy instantiation）】
// 這是類別模板一個非常重要、卻很少被強調的性質：
//
//     實例化 Box<T> 這個「類別」時，並不會實例化它「全部的成員函式」；
//     只有實際被呼叫（被 odr-used）的成員函式才會被實例化。
//
// 本機 g++ 15.2 實測驗證：定義一個類別模板，其中某個成員函式的本體對某型別
// 根本不合法（例如對沒有 operator+ 的型別做 v + v），只要**從不呼叫它**，
// 程式就能正常編譯執行；一旦呼叫，才會出現
//     error: no match for 'operator+' (operand types are 'const NoPlus' and 'const NoPlus')
//
// 這個性質在實務上非常有用：一個容器模板可以提供 sort() 這種需要
// operator< 的成員函式，而使用者若把不可比較的型別放進去、且從不呼叫 sort()，
// 完全不會有問題。std::vector 就是這樣：vector<T> 對 T 的要求其實很低，
// 是各個成員函式各自有各自的要求。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 一份原始碼，多份記憶體佈局
//   Box<int> 內含一個 int 與一個 bool；Box<std::string> 內含一個
//   std::string（本機 libstdc++ 為 32 bytes）與一個 bool。兩者的
//   sizeof、對齊、建構/解構行為全都不同 —— 它們只是碰巧共用同一份原始碼。
//   本檔會實際印出各實例化的 sizeof 佐證這一點。
//   （注意：sizeof 的具體數值屬實作定義，此處為本機 g++ 15.2 / libstdc++
//     x86-64 的實測值，非標準保證。）
//
// (B) 為什麼成員函式定義在類別內就自動具備 inline 語意
//   類別模板的成員函式若在類別定義內寫出本體，它同時具備兩個性質：
//   隱含 inline，且實例化後的符號是 weak / COMDAT。這使得多個 TU 各自
//   include 同一個 header、各自實例化 Box<int>，linker 能把重複定義合併，
//   不會 ODR 衝突（機制與概念2.cpp 說明的 vague linkage 相同）。
//
// (C) Box<T> 與 std::optional<T> 的真實差距
//   本檔的 Box 有個明顯的效率問題：它用 `T content;` 直接放一個 T，
//   代表**即使盒子是空的，T 也已經被預設建構**了。這帶來兩個後果：
//     1) T 必須可預設建構 —— 沒有預設建構子的型別無法放進 Box。
//     2) 空盒子仍付出一次建構成本。
//   std::optional 的做法是用一塊對齊過的原始儲存空間（配合 placement new），
//   只有在真的有值時才建構物件。這是「泛型容器」設計中非常典型的取捨：
//   簡單直觀的寫法 vs 對型別要求最低且零浪費的寫法。
//
// 【注意事項 Pay Attention】
// 1. 原始碼使用了 std::runtime_error 卻只 include <iostream> 與 <string>。
//    它在本機能編譯，純粹是因為某個標頭「碰巧」間接引入了 <stdexcept> ——
//    這種相依是不可攜的，換一個標準庫實作或版本就可能編譯失敗。
//    本檔已補上 #include <stdexcept>。**用到什麼就 include 什麼**
//    （include-what-you-use）是必須養成的習慣。
// 2. get() 回傳 T（按值），每次呼叫都會複製一份內容。對 std::string 或大型
//    物件而言這是實質成本；正式設計應提供 `const T& get() const`，
//    或如 std::optional 同時提供多種 value() 重載。
// 3. Box<T> 要求 T 可預設建構（因為 `T content;` 成員）。沒有預設建構子的
//    型別放不進去 —— 這是隱含介面的又一個例子，且錯誤訊息會出現在
//    Box<T> 的建構子裡，而不是使用者寫的那一行。
// 4. 類別模板的成員函式若定義在類別外，語法要重複寫 template 前綴：
//        template <typename T>
//        void Box<T>::put(const T& item) { ... }
//    而且這份定義同樣必須放在 header（原因同函式模板）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】類別模板
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 類別模板和函式模板在「使用」上最大的差異是什麼？
//     答：函式模板能從實參推導型別，呼叫時通常不必寫 <>；類別模板在 C++17
//         之前**完全不能推導**，一定要明寫 Box<int>。這正是 std::make_pair /
//         make_tuple 這類輔助函式存在的原因 —— 它們是函式模板，藉推導繞過限制。
//         C++17 的 CTAD 讓 std::pair p{1, 3.14} 這種寫法成為可能
//         （本機 -pedantic-errors 實測：C++14 失敗、C++17 通過）。
//     追問：CTAD 出現後 make_pair 就沒用了嗎？
//         → 大部分場合是的。但 make_pair 會對引數做 decay（陣列轉指標、
//           去掉參考），語意與 CTAD 不完全相同，既有程式碼不能無腦替換。
//
// 🔥 Q2. 實例化一個 Box<int>，它的所有成員函式都會被編譯出來嗎？
//     答：不會。類別模板的成員函式是**用到才實例化**（lazy instantiation）。
//         本機實測：某成員函式的本體對該型別根本不合法（對沒有 operator+ 的
//         型別做 v + v），只要從不呼叫它就能正常編譯；一呼叫才報
//         no match for 'operator+'。
//     追問：這個性質有什麼實際價值？
//         → 容器模板可以提供「只有部分型別支援」的成員函式，而不影響其他型別
//           的使用者。vector<T> 對 T 的要求其實是「各成員函式各自要求」，
//           不是一整包強制要求。
//
// ⚠️ 陷阱. Box<T> 裡寫 `T content;`，對 T 有什麼你沒注意到的要求？
//     答：要求 T **可預設建構**。因為 Box 的建構子只初始化 has_content，
//         content 是被預設建構的。沒有預設建構子的型別（例如只有帶參數建構子
//         的類別）根本放不進 Box，而錯誤訊息會指向 Box 的建構子，
//         而不是使用者寫 Box<Foo> 的那一行。
//     為什麼會錯：以為「盒子是空的就沒有物件」。實際上 `T content;` 這個成員
//         永遠存在，空盒子只是 has_content 為 false 而已 —— 物件早就被建構了。
//         std::optional 正是為了避免這一點，才改用原始儲存空間 + placement new。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <stdexcept>   // std::runtime_error：原始碼漏了這個，見【注意事項 1】

template <typename T>
class Box {
private:
    T content;         // 注意：即使盒子是空的，這個 T 也已經被預設建構了
    bool has_content;

public:
    Box() : has_content(false) {}

    void put(const T& item) {
        content = item;
        has_content = true;
    }

    T get() const {
        if (!has_content) {
            throw std::runtime_error("Box is empty!");
        }
        return content;
    }

    bool is_empty() const {
        return !has_content;
    }

    // 這個成員函式對「不支援 operator+」的型別是不合法的，
    // 但只要從不呼叫它，就永遠不會被實例化，也就不會報錯。
    // 這就是 lazy instantiation（本機 g++ 15.2 實測驗證）。
    T doubled() const {
        if (!has_content) {
            throw std::runtime_error("Box is empty!");
        }
        return content + content;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】設定項目：區分「沒有設定」與「設定成 0 / 空字串」
//   情境：讀取設定檔時，「retry_count 沒有寫」與「retry_count = 0」是兩件事。
//         前者應該套用預設值，後者代表使用者明確要求不重試。
//         用 int 之類的裸型別無法表達這個差異，只能靠 -1 這種哨兵值硬湊。
//   為何用泛型：ConfigValue 對 int、bool、std::string 全部通用；
//               每加一個設定型別都不必再寫一次「有沒有設定」的邏輯。
//   （正式專案可直接用 C++17 的 std::optional<T>，這裡用 Box 呈現其原理。）
// -----------------------------------------------------------------------------
template <typename T>
T config_or_default(const Box<T>& slot, const T& fallback) {
    return slot.is_empty() ? fallback : slot.get();
}

int main() {
    std::cout << "=== 同一份藍圖，三個獨立的類別 ===" << std::endl;

    // 裝 int 的盒子
    Box<int> int_box;
    int_box.put(42);
    std::cout << "int_box contains: " << int_box.get() << std::endl;

    // 裝 string 的盒子
    Box<std::string> string_box;
    string_box.put("Hello, STL!");
    std::cout << "string_box contains: " << string_box.get() << std::endl;

    // 裝 double 的盒子
    Box<double> double_box;
    double_box.put(3.14159);
    std::cout << "double_box contains: " << double_box.get() << std::endl;

    std::cout << "\n=== 各實例化的記憶體佈局並不相同 ===" << std::endl;
    std::cout << "sizeof(Box<int>)         = " << sizeof(Box<int>) << std::endl;
    std::cout << "sizeof(Box<double>)      = " << sizeof(Box<double>) << std::endl;
    std::cout << "sizeof(Box<std::string>) = " << sizeof(Box<std::string>)
              << std::endl;
    std::cout << "（以上為本機 g++ 15.2 / libstdc++ x86-64 實測值，非標準保證）"
              << std::endl;

    std::cout << "\n=== lazy instantiation：用到才實例化 ===" << std::endl;
    std::cout << "int_box.doubled() = " << int_box.doubled()
              << "   <- int 支援 operator+，這裡才實例化 doubled()" << std::endl;
    std::cout << "string_box.doubled() = " << string_box.doubled()
              << "   <- std::string 的 + 是字串串接" << std::endl;

    std::cout << "\n=== 空盒子：拋出例外 ===" << std::endl;
    Box<int> empty_box;
    std::cout << "empty_box.is_empty() = " << std::boolalpha
              << empty_box.is_empty() << std::endl;
    try {
        empty_box.get();
    } catch (const std::runtime_error& e) {
        std::cout << "get() 拋出例外: " << e.what() << std::endl;
    }

    std::cout << "\n=== 日常實務：設定值與預設值 ===" << std::endl;
    Box<int> retry_count;          // 設定檔沒寫 retry_count
    Box<int> timeout_ms;
    timeout_ms.put(0);             // 設定檔明確寫了 timeout_ms = 0
    Box<std::string> endpoint;
    endpoint.put("api.internal:8080");

    std::cout << "retry_count（未設定，套用預設 3）= "
              << config_or_default(retry_count, 3) << std::endl;
    std::cout << "timeout_ms（明確設為 0，不套預設 5000）= "
              << config_or_default(timeout_ms, 5000)
              << "   <- 這就是「沒設定」與「設成 0」的差別" << std::endl;
    std::cout << "endpoint = "
              << config_or_default(endpoint, std::string("localhost:80"))
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第二課：泛型編程（Generic Programming）概念6.cpp -o concept6

// （以上為本機 g++ 15.2 / libstdc++ x86-64 實測值，非標準保證）

// === 預期輸出 ===
// === 同一份藍圖，三個獨立的類別 ===
// int_box contains: 42
// string_box contains: Hello, STL!
// double_box contains: 3.14159
//
// === 各實例化的記憶體佈局並不相同 ===
// sizeof(Box<int>)         = 8
// sizeof(Box<double>)      = 16
// sizeof(Box<std::string>) = 40
//
// === lazy instantiation：用到才實例化 ===
// int_box.doubled() = 84   <- int 支援 operator+，這裡才實例化 doubled()
// string_box.doubled() = Hello, STL!Hello, STL!   <- std::string 的 + 是字串串接
//
// === 空盒子：拋出例外 ===
// empty_box.is_empty() = true
// get() 拋出例外: Box is empty!
//
// === 日常實務：設定值與預設值 ===
// retry_count（未設定，套用預設 3）= 3
// timeout_ms（明確設為 0，不套預設 5000）= 0   <- 這就是「沒設定」與「設成 0」的差別
// endpoint = api.internal:8080
