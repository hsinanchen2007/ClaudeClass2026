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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外與例外安全總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 三種例外安全保證分別是什麼？各舉一個標準庫的例子。
//     答：基本保證——不洩漏資源、物件維持有效但可能已改變的狀態（最低可接受標準）；
//     強保證——commit-or-rollback，要嘛完全成功、要嘛可觀察狀態完全不變；nothrow 保證
//     ——絕不拋出，通常以 noexcept 標記。std::vector::push_back 在元素的移動建構是
//     noexcept 時提供強保證；若移動建構可能拋，標準庫改用 std::move_if_noexcept 退回
//     複製來維持強保證，而對不可複製的型別就只剩基本保證。解構函式、swap、移動操作
//     則屬於必須 nothrow 的那一類，因為它們是回滾機制本身的基礎。
//     追問：為什麼不是所有函式都該提供強保證？（強保證常需複製整份資料，對巨大結構
//     只為一次 append 而全複製並不合理）
//
// 🔥 Q2. 什麼是「例外中立（exception neutral）」？
//     答：指泛型程式碼（模板、容器、演算法）不吞掉也不轉換使用者型別拋出的例外，只讓它
//     原封不動往上傳播，同時自己維持不變式、不洩漏資源。例如 std::vector<T> 不知道 T 的
//     複製建構函式會拋什麼，它的責任是「讓那個例外傳出去，同時保證 vector 自身仍然有效」。
//     這是函式庫設計的核心原則：函式庫負責自己的不變式，不替使用者決定如何處理錯誤。
//     追問：這需要到處寫 catch 嗎？（通常不需要，RAII 讓例外中立幾乎自動達成）
//
// Q3. 例外的「zero-cost」模型與例外 vs 錯誤碼的判準？
//     答：zero-cost 指的是未拋例外時正常路徑沒有額外指令（unwinding 資訊放在獨立的唯讀
//     區段），而不是「例外免費」——真的拋出時要查表、解析 unwind 資訊、跑解構函式、做
//     RTTI 匹配，成本比回傳錯誤碼高好幾個數量級。因此：錯誤罕見、偵測處與處理處隔多層、
//     建構函式或運算子失敗（沒有回傳值可用）時用例外；失敗是常規且頻繁、效能關鍵迴圈、
//     或要跨 ABI 邊界時用錯誤碼、std::optional 或 C++23 的 std::expected。
// ═══════════════════════════════════════════════════════════════════════════

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

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
//
// [demo_basics]
//   ok: 10 -> 10
//   out_of_range: value must be positive
//   invalid_argument: stoi
//
// [demo_catch_by_ref]
//   caught std::exception: something failed
//
// [demo_raii]
//   acquire A
//   acquire B
//   release B
//   release A
//   caught: boom
//
// [demo_noexcept]
//   MoveMaybeThrow move noexcept? 0
//   MoveNoThrow    move noexcept? 1
//
// [demo_safety_levels_note]
//   no-throw / strong / basic 的差異，重點在『失敗後狀態是否可預期』
//
// [done]
