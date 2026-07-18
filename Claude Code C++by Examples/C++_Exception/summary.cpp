/*
================================================================================
【C++_Exception/summary.cpp】

本目錄主題：C++ 例外（exception）與例外安全（exception safety）

你需要掌握的核心：
  - throw / try / catch 的基本語法
  - 標準例外階層：std::exception、std::runtime_error、std::logic_error...
  - catch 的正確姿勢：通常「catch(const std::exception&)」
  - noexcept 的意義與用法（特別是 move ctor/assign 影響容器效能）
  - RAII：用物件生命週期管理資源，讓例外發生時也能自動釋放
  - function-try-block、nested_exception（偏進階，這裡用筆記提示）

本 summary 原則：
  - 不加入 題庫 類範例
  - C++17 可編譯

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Exception/C++_Exception summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Exception/C++_Exception summary 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Exception/C++_Exception summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// -----------------------------------------------------------------------------
// 【重點 1】基本 throw / catch（用標準例外型別）
// -----------------------------------------------------------------------------
static int parse_positive_int(const std::string& s) {
    // 這裡用 stoi 示範；實務可用 from_chars（更快且不丟例外）
    int v = std::stoi(s);
    if (v <= 0) {
        throw std::out_of_range("value must be positive");
    }
    return v;
}

static void demo_basics() {
    std::cout << "\n[demo_basics]\n";

    for (const std::string& s : {"10", "-3", "abc"}) {
        try {
            int v = parse_positive_int(s);
            std::cout << "  ok: " << s << " -> " << v << "\n";
        } catch (const std::invalid_argument& e) {
            std::cout << "  invalid_argument: " << e.what() << "\n";
        } catch (const std::out_of_range& e) {
            std::cout << "  out_of_range: " << e.what() << "\n";
        }
    }
}

// -----------------------------------------------------------------------------
// 【重點 2】catch 要用 reference（避免 slicing）
// -----------------------------------------------------------------------------
static void demo_catch_by_ref() {
    std::cout << "\n[demo_catch_by_ref]\n";

    try {
        throw std::runtime_error("something failed");
    } catch (const std::exception& e) {
        // ✅ 正確：保留動態型別資訊、避免 slicing
        std::cout << "  caught std::exception: " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------------------
// 【重點 3】RAII：例外安全的根本（不要手寫 new/delete）
// -----------------------------------------------------------------------------
struct Resource {
    explicit Resource(const char* name) : name_(name) {
        std::cout << "  acquire " << name_ << "\n";
    }
    ~Resource() {
        std::cout << "  release " << name_ << "\n";
    }
    const char* name_;
};

static void demo_raii() {
    std::cout << "\n[demo_raii]\n";
    try {
        Resource r1("A");
        Resource r2("B");
        throw std::runtime_error("boom");
    } catch (const std::exception& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }
    // r1/r2 會在丟例外時照樣解構釋放（這就是 RAII 的威力）
}

// -----------------------------------------------------------------------------
// 【重點 4】noexcept：承諾不丟例外（破壞就 std::terminate）
// -----------------------------------------------------------------------------
// 重要觀念：
//   - noexcept 函式若丟出例外 → 直接 std::terminate（程式終止）
//   - 容器（像 vector）在搬移元素時，若 move ctor 是 noexcept，才能放心用 move
//     否則常會退回用 copy 來維持 strong guarantee（效能差很多）
struct MoveMaybeThrow {
    MoveMaybeThrow() = default;
    MoveMaybeThrow(const MoveMaybeThrow&) = default;
    MoveMaybeThrow(MoveMaybeThrow&&) /* not noexcept */ {}
};

struct MoveNoThrow {
    MoveNoThrow() = default;
    MoveNoThrow(const MoveNoThrow&) = default;
    MoveNoThrow(MoveNoThrow&&) noexcept {}
};

static void demo_noexcept() {
    std::cout << "\n[demo_noexcept]\n";
    std::cout << "  MoveMaybeThrow move noexcept? " << noexcept(MoveMaybeThrow(std::declval<MoveMaybeThrow&&>())) << "\n";
    std::cout << "  MoveNoThrow    move noexcept? " << noexcept(MoveNoThrow(std::declval<MoveNoThrow&&>())) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】例外安全等級（筆記）
// -----------------------------------------------------------------------------
// - no-throw guarantee：保證不丟例外
// - strong guarantee  ：失敗時不改變狀態（像交易：要嘛成功要嘛回滾）
// - basic guarantee   ：失敗時狀態仍然有效、資源不洩漏，但內容可能部分更新
// - no guarantee      ：什麼都不保證（應避免）
static void demo_safety_levels_note() {
    std::cout << "\n[demo_safety_levels_note]\n";
    std::cout << "  no-throw / strong / basic 的差異，重點在『失敗後狀態是否可預期』\n";
}

int main() {
    demo_basics();
    demo_catch_by_ref();
    demo_raii();
    demo_noexcept();
    demo_safety_levels_note();

    std::cout << "\n[done]\n";
    return 0;
}

