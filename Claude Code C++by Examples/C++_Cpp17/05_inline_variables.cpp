// =============================================================================
//  05_inline_variables.cpp  —  inline 變數 (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/inline
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、解決什麼問題？                                         │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 之前，header 裡如果想「定義」(不是宣告) 一個變數 / 物件，會違反
//  ODR（One Definition Rule） — 多個 cpp 引入後 link 失敗：
//
//      // header.h
//      int counter = 0;       // ❌ 多個 .cpp include 會撞名
//
//  解法（C++17 之前）：
//   * 把定義放某個 .cpp 內、header 只 declare extern
//   * 或寫個 inline function 包起來（Meyers' Singleton）
//
//  C++17 起 inline 可以加在「變數」前面：
//
//      // header.h
//      inline int counter = 0;     // ✅ ODR-safe
//
//  多個 .cpp include 時 linker 會挑一份 — 不會撞名。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、最常見場景                                             │
//  └────────────────────────────────────────────────────────────┘
//
//   1) header-only library 中的 global 常數 / 物件
//   2) class 內的 static 成員定義（傳統需要分檔）
//
//      class Foo {
//      public:
//          static inline int s_count = 0;     // C++17 起就地定義 ✅
//      };
//      // 不必再到 .cpp 寫 int Foo::s_count = 0;
//
//   3) constexpr 變數隱含 inline（C++17 起）
//      constexpr int N = 100;        // 自動 inline
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//  注意：本檔是「單檔」 — 我們示範的是 inline 變數的「就地定義」省略
//  外部 .cpp 定義的便利。
//
//   * Demo 1：class static inline
//   * Demo 2：namespace 級 inline 全域常數
// =============================================================================

/*
補充筆記：inline_variables
  - inline_variables 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - inline variable 允許變數定義放在 header 並被多個 translation unit 包含。
  - 它常用於 header-only library 的常數或 template 靜態資料成員。
*/
#include <iostream>
#include <string>

class Counter {
public:
    static inline int total = 0;          // ✅ C++17 就地定義 OK

    static void inc() { ++total; }
    static int get() { return total; }
};

namespace Cfg {
    inline const std::string version = "1.0.0";   // header 中安全定義
    inline constexpr int maxRetries = 3;          // constexpr 隱含 inline
}

// 實用範例：檔案 scope 的 inline 常數中心
namespace Limits {
    inline constexpr int MAX_USERS    = 10000;
    inline constexpr int MAX_RETRIES  = 3;
    inline constexpr int TIMEOUT_MS   = 30000;
    inline const std::string APP_NAME = "demo-app";   // string 需 inline
}

// 實用範例：用 inline static 取代分檔定義
class Stats {
public:
    static inline int requests = 0;
    static inline int errors   = 0;
    static void hit()      { ++requests; }
    static void failed()   { ++errors; }
    static void report() {
        std::cout << "[Stats] req=" << requests << " err=" << errors << '\n';
    }
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：class static inline
    // ─────────────────────────────────────────────────────────
    Counter::inc();
    Counter::inc();
    Counter::inc();
    std::cout << "[Demo1] Counter::total = " << Counter::get() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：namespace 級
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] version=" << Cfg::version
              << " maxRetries=" << Cfg::maxRetries << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：inline 變數會被「inline 進每個使用點」嗎？
    //    A：不會。「inline」這個關鍵字只是「ODR 例外」 — 允許多份定義；
    //       runtime 仍是「全程式只一份地址」。compiler 不會把變數內容直
    //       接 inline 替換 (除非它是 constexpr / 可被優化掉)。
    //
    //  Q2：必須加 const 嗎？
    //    A：不必。inline int n = 0; 也是合法的 — 就是個全域 mutable 變數。
    //       但 mutable global 通常該避免，加 const 比較常見。
    //
    //  Q3：跟 static 的關係？
    //    A：static 在 namespace 級表示「internal linkage」 — 每個 .cpp 自
    //       己一份。inline 是「external linkage 但允許多份定義」 — 全 link
    //       後仍是同一份。語意完全不同。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：全域常數中心 — 跨 .h 共用的 inline 常數
    //   工作上常見：HTTP status / 設定預設值放 header，多個 .cpp 共用一份。
    //   （Limits 與 Stats 已定義在檔案 scope，見下方）
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo3] " << Limits::APP_NAME
              << " max_users=" << Limits::MAX_USERS
              << " timeout="   << Limits::TIMEOUT_MS << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：Singleton 計數器 — 用 inline static 避免分檔定義
    //   工作上常見：監控全域計數器（請求數、錯誤數），header-only library
    // ─────────────────────────────────────────────────────────
    Stats::hit(); Stats::hit(); Stats::failed(); Stats::hit();
    Stats::report();

    return 0;
}
