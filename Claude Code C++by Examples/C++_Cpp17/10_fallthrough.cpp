// =============================================================================
//  10_fallthrough.cpp  —  [[fallthrough]] (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/attributes/fallthrough
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 [[fallthrough]]？                             │
//  └────────────────────────────────────────────────────────────┘
//
//  switch 的 case 默認 fall-through 是 C/C++ 的歷史包袱 — 忘記 break 是
//  經典 bug 來源。現代編譯器加了 -Wimplicit-fallthrough 警告：
//
//      switch (x) {
//          case 1:
//              do1();
//              // ↓ 沒 break — 會 fall-through 到 case 2 — 你是故意還是漏寫？
//          case 2:
//              do2();
//              break;
//      }
//
//  C++17 引入 [[fallthrough]] 屬性 — 「我故意要 fall-through，請別警告」：
//
//      switch (x) {
//          case 1:
//              do1();
//              [[fallthrough]];      // 明寫意圖
//          case 2:
//              do2();
//              break;
//      }
//
//  位置：必須是「進入下一個 label 之前的最後一條 statement」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
// =============================================================================

/*
補充筆記：fallthrough
  - fallthrough 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - [[fallthrough]] 明確表示 switch case 故意往下執行，避免被誤認為忘記 break。
  - 它應放在空 statement 位置，也就是 case 結尾往下一個 case 前。
*/
#include <iostream>
#include <string>

static void classify(int n) {
    std::cout << "n=" << n << " => ";
    switch (n) {
        case 1:
            std::cout << "one ";
            [[fallthrough]];          // 故意：1 也算進「小整數」
        case 2:
            std::cout << "small ";
            break;
        case 10:
        case 20:
            std::cout << "ten-ish ";
            break;
        default:
            std::cout << "other ";
            break;
    }
    std::cout << '\n';
}

int main() {
    for (int v : {1, 2, 10, 20, 99}) classify(v);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：[[fallthrough]] 跟 // fallthrough 註解差在哪？
    //    A：許多編譯器（GCC/Clang）以前看「// fallthrough」之類的註解就
    //       不警告 — 但這是非標準 hack，跨編譯器不一致。[[fallthrough]] 是
    //       標準屬性、跨平台一致。
    //
    //  Q2：「連續 case 標籤」需要 [[fallthrough]] 嗎？
    //    A：不必。
    //         case 10:
    //         case 20:
    //             do(...);     // 兩個 label 共用 body — 不算 fall-through
    //       fall-through 是「label A body 結束後流入 label B body」 — 兩個
    //       label 之間沒有真正執行「A 的動作」就不算。
    //
    //  Q3：什麼時候用 fallthrough 比較好？
    //    A：「真的有共用邏輯且想要兩條路都跑」時。多數情況「合併 case label」
    //       已經夠用；fallthrough 用在「先跑 A 的部份、再跑 B 的全部」場景。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例：log level 處理 — 高級別會包含低級別動作
    //   工作上常見：Fatal 不只記錯誤也要 flush+exit，Error 只記錯誤。
    //   級別愈高動作愈多 → 用 fallthrough 自然展開「累加」動作。
    // ─────────────────────────────────────────────────────────
    enum class LogLevel { Info = 0, Warn = 1, Error = 2, Fatal = 3 };
    auto handle = [](LogLevel lvl, const std::string& msg) {
        switch (lvl) {
            case LogLevel::Fatal:
                std::cout << "  FLUSH all log buffers (Fatal)\n";
                [[fallthrough]];                // 故意：Fatal 也算 Error
            case LogLevel::Error:
                std::cout << "  WRITE to error log: " << msg << '\n';
                [[fallthrough]];                // Error 也算 Warn
            case LogLevel::Warn:
                std::cout << "  WRITE to warn log: " << msg << '\n';
                [[fallthrough]];                // Warn 也算 Info
            case LogLevel::Info:
                std::cout << "  WRITE to info log: " << msg << '\n';
                break;
        }
    };
    std::cout << "[Demo2] Info:\n";  handle(LogLevel::Info,  "starting");
    std::cout << "[Demo2] Warn:\n";  handle(LogLevel::Warn,  "low memory");
    std::cout << "[Demo2] Fatal:\n"; handle(LogLevel::Fatal, "panic");

    return 0;
}
