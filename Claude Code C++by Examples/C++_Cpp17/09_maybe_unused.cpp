// =============================================================================
//  09_maybe_unused.cpp  —  [[maybe_unused]] (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/attributes/maybe_unused
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、解決什麼問題？                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  -Wunused-* 警告對「真的沒用」的變數很有用，但有些情況「可能用、可能不
//  用」是合理的：
//
//   * 條件編譯：#ifdef DEBUG 才用
//   * 函式參數有時用有時不（為了介面一致性）
//   * static_assert 中只用到「型別」、變數其實沒用到 runtime
//
//  傳統 hack：
//      void f(int x) {
//          (void)x;          // 喚醒「我知道沒用」
//      }
//      或 void f(int /*x*/);
//
//  C++17 的 [[maybe_unused]]：明寫「可能不用」，編譯器不警告。
//
//      void f([[maybe_unused]] int x) {
//          #ifdef DEBUG
//              std::cout << x;
//          #endif
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、可以加在哪？                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   * 函式參數
//   * 區域變數
//   * 函式 / 類別 / enum / typedef
//   * 結構體成員（C++26 才有，C++17 還沒）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：maybe_unused
  - maybe_unused 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - [[maybe_unused]] 表示變數或參數暫時可能不用，避免警告但保留名稱。
  - 它適合跨平台條件編譯或 debug-only 變數，不應拿來掩蓋死程式碼。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】[[maybe_unused]]
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. [[maybe_unused]] 解決什麼問題？
//     答：抑制「未使用」警告，用在「可能用、可能不用」是合理設計的地方：條件編譯
//         下才用到的變數、為了介面一致性而保留但本實作用不到的參數、結構化綁定中
//         不需要的名字。它取代了舊寫法 (void)x;。
//     追問：和直接關掉 -Wunused 差在哪？（-Wunused 是整份關掉，會連真正的漏用一起
//           放行；屬性是逐個標記，其餘位置仍受保護）
//
// ⚠️ 陷阱. 用 assert 檢查的變數，為什麼 release 建置會冒出未使用警告？
//     答：NDEBUG 下 assert 展開成空敘述，那個只被 assert 用到的變數就真的沒人用了。
//         這正是 [[maybe_unused]] 的標準場景。
//     為什麼會錯：多數人在 debug 建置測完就以為沒事，警告只在 release 才出現；
//         而且此時該變數往往仍需保留，不能直接刪掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <cassert>
#include <iostream>
#include <map>
#include <string>

// 條件編譯下「可能不用」
static void process([[maybe_unused]] int code, [[maybe_unused]] const char* tag) {
#ifdef DEBUG
    std::cout << "code=" << code << " tag=" << tag << '\n';
#endif
}

[[maybe_unused]] static int helperUnused() { return 42; }

int main() {
    [[maybe_unused]] int debug_only = 100;     // -DDEBUG 才會用
#ifdef DEBUG
    std::cout << "debug_only = " << debug_only << '\n';
#endif

    // 用在 assert — release 模式下 assert 變空，變數可能「沒用」
    [[maybe_unused]] int sum = 1 + 2;
    assert(sum == 3);

    process(404, "not_found");
    std::cout << "[Demo] done\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：跟 (void)x 哪個好？
    //    A：[[maybe_unused]] 加在「宣告處」 — 整段 scope 不警告，且讀者明
    //       白意圖。(void)x 是局部技巧，需要寫在使用點。新程式碼建議用屬性。
    //
    //  Q2：可以加在「成員變數」嗎？
    //    A：C++17 不行（標準明確排除類別成員）。要避免「未用成員」警告就
    //       自己 init 或加 (void) 取址。C++26 預計開放。
    //
    //  Q3：對 release / debug 雙模式專案非常有用？
    //    A：是。專案用 NDEBUG 切換 assert / log 後，許多變數「只在 debug
    //       用」 — 加 [[maybe_unused]] 一勞永逸。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：interface 一致性 — 某參數在這個實作沒用，但介面要求
    //   工作上常見：callback 介面強制特定簽名，部分 handler 不需要全部參數
    // ─────────────────────────────────────────────────────────
    using Handler = void (*)(int code, const char* msg);
    Handler okHandler = [](int code, [[maybe_unused]] const char* msg) {
        std::cout << "[Demo2] OK code=" << code << '\n';
    };
    Handler errHandler = [](int code, const char* msg) {
        std::cout << "[Demo2] ERR code=" << code << " msg=" << msg << '\n';
    };
    okHandler(200,  "OK");          // okHandler 不用 msg
    errHandler(500, "internal error");

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：structured binding 中只用部分值
    //   工作上常見：std::map 走訪只用 key 或只用 value
    // ─────────────────────────────────────────────────────────
    std::map<std::string, int> stock{{"apple", 5}, {"banana", 3}};
    for ([[maybe_unused]] const auto& [name, count] : stock) {
        // 假設這個 loop 只統計 count 總和，name 沒用到
        std::cout << "[Demo3] count = " << count << '\n';
    }

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 09_maybe_unused.cpp -o 09_maybe_unused

// === 預期輸出 ===
// [Demo] done
// [Demo2] OK code=200
// [Demo2] ERR code=500 msg=internal error
// [Demo3] count = 5
// [Demo3] count = 3
