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

