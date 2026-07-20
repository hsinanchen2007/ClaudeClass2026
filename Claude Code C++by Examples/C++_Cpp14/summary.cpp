/*
================================================================================
【C++_Cpp14/summary.cpp】

本目錄主題：C++14 常用語法（在 C++11 基礎上「更順手」）

常見重點（對應本目錄檔案命名）：
  - auto 回傳型別推導（函式回傳 auto）
  - generic lambda（[](auto x){...}）
  - init-capture（[x = expr]）
  - binary literals（0b1010）
  - [[deprecated]] attribute
  - make_unique（C++14 新增，unique_ptr 的正確工廠函式）

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯（但示範的語法本質是 C++14）

編譯：
  g++ -std=c++14 -Wall -Wextra summary.cpp -o summary && ./summary
（用 -std=c++17 也可）
================================================================================
*/

/*
補充筆記：C++_Cpp14/C++_Cpp14 summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Cpp14/C++_Cpp14 summary 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Cpp14/C++_Cpp14 summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++14 總覽（版本分界是本目錄的核心考點）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++14 有哪些新特性？（開放題，用「語言 / 標準庫」兩層來答）
//     答：語言層——函式回傳型別推導 auto f()、decltype(auto)、放寬的 constexpr、
//         泛型 lambda [](auto)、lambda init capture [p = std::move(x)]、變數模板、
//         二進位字面值 0b、數字分隔符 '、[[deprecated]]、放寬的聚合初始化
//         （有成員預設值 NSDMI 仍可為 aggregate）。
//         標準庫層——std::make_unique、_t 別名（enable_if_t / decay_t）、
//         std::integer_sequence / index_sequence、std::exchange、
//         透明比較子 std::less<> 帶來的異質查找、std::get<T>(tuple) 依型別取值、
//         std::shared_timed_mutex、std::quoted。棄用方面：C++14 棄用的是
//         std::random_shuffle；std::auto_ptr 早在 C++11 就已棄用（常被歸錯到
//         C++14），兩者都要到 C++17 才真正從標準庫移除。
//     追問：一句話定位 C++14？（C++11 的 minor release／補完版，
//           沒有新的大範式，全是把 C++11 沒做完的地方補齊）
//
// 🔥 Q2. C++14 相對 C++11 最重要的改進是哪幾個？（收尾綜合題）
//     答：三個一組來答——
//         (1) 放寬的 constexpr：編譯期計算從「遞迴＋三元的黑魔法」變成寫一般函式，
//             實務影響最大；
//         (2) 回傳型別推導 + decltype(auto)：泛型 wrapper 與轉發函式大幅簡化；
//         (3) 泛型 lambda + init capture：lambda 從「只能捕獲既有變數的小函式」
//             變成可移動捕獲、可泛型的一等公民。
//         要補第四項就講 std::make_unique 補完了 C++11 智慧指標的缺口。
//
// Q3. 哪些「常被誤認成 C++11」的特性其實是 C++14？
//     答：變數模板、std::make_unique、_t 別名、泛型 lambda、lambda init capture、
//         放寬的 constexpr、decltype(auto)、auto 回傳型別推導、二進位字面值、
//         數字分隔符、[[deprecated]]、std::exchange、integer_sequence。
//         這批全部可以用 -std=c++11 -pedantic-errors 編一次逼出錯誤來驗證，
//         唯一的例外是 [[deprecated]]（見 07 檔：GCC 在 C++11 模式下照樣接受）。
//
// ⚠️ 陷阱. 那反過來——哪些東西常被誤認成 C++14，其實要 C++17 才有？
//     答：fold expression、if constexpr、std::void_t、CTAD（類別模板實參推導）、
//         guaranteed copy elision、_v 結尾的變數模板（如 std::is_integral_v）、
//         std::shared_mutex、std::scoped_lock、std::apply、結構化綁定、string_view。
//     為什麼會錯：這幾個常跟 C++14 的東西一起出現在同一段現代 C++ 教學裡，
//         而且彼此有「機制上的血緣」，很容易被連坐歸到同一版。三組最容易混的是：
//         變數模板是 C++14、但標準庫的 _v 別名是 C++17；
//         std::shared_timed_mutex 是 C++14、std::shared_mutex 是 C++17；
//         有序容器（map/set）的異質查找是 C++14、無序容器要到 C++20。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【重點 1】auto 回傳型別（C++14）：讓編譯器推導回傳型別
// -----------------------------------------------------------------------------
// C++11 若要推導回傳型別通常需要 trailing return type + decltype。
// C++14 起可直接寫 auto f(...) { return ...; }，由 return 表達式推導。
static auto add(int a, int b) {
    return a + b; // 推導為 int
}

// -----------------------------------------------------------------------------
// 【重點 2】generic lambda（C++14）：lambda 參數用 auto
// -----------------------------------------------------------------------------
static void demo_generic_lambda() {
    std::cout << "\n[demo_generic_lambda]\n";

    auto twice = [](auto x) { return x + x; };
    std::cout << "  twice(3)=" << twice(3) << "\n";
    std::cout << "  twice(string)=" << twice(std::string("ab")) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】init-capture（C++14）：捕獲一個「表達式結果」
// -----------------------------------------------------------------------------
static void demo_init_capture() {
    std::cout << "\n[demo_init_capture]\n";

    std::string s = "hello";
    auto f = [t = std::move(s)]() { return t.size(); };
    std::cout << "  moved capture size=" << f() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】binary literal（0b...）
// -----------------------------------------------------------------------------
static void demo_binary_literal() {
    std::cout << "\n[demo_binary_literal]\n";
    std::uint32_t mask = 0b1010'1100u;
    std::cout << "  mask=" << mask << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】[[deprecated]]：標記不建議使用的 API
// -----------------------------------------------------------------------------
[[deprecated("use new_api() instead")]]
static int old_api() {
    return 1;
}

static int new_api() { return 2; }

static void demo_deprecated() {
    std::cout << "\n[demo_deprecated]\n";
    // old_api(); // 解除註解會在編譯期給 warning（提醒你替換）
    std::cout << "  new_api()=" << new_api() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 6】std::make_unique（C++14）：建立 unique_ptr 的標準方式
// -----------------------------------------------------------------------------
struct Widget {
    explicit Widget(int x) : x(x) {}
    int x{};
};

static void demo_make_unique() {
    std::cout << "\n[demo_make_unique]\n";
    auto p = std::make_unique<Widget>(42);
    std::cout << "  Widget.x=" << p->x << "\n";
}

int main() {
    std::cout << "[add] " << add(2, 3) << "\n";
    demo_generic_lambda();
    demo_init_capture();
    demo_binary_literal();
    demo_deprecated();
    demo_make_unique();

    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// [add] 5
//
// [demo_generic_lambda]
//   twice(3)=6
//   twice(string)=abab
//
// [demo_init_capture]
//   moved capture size=5
//
// [demo_binary_literal]
//   mask=172
//
// [demo_deprecated]
//   new_api()=2
//
// [demo_make_unique]
//   Widget.x=42
//
// [done]
