// =============================================================================
//  12_enum_class.cpp  —  enum class (C++11) — 強型別 enum
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/enum
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼要 enum class？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  傳統 C++98 enum（unscoped enum）有兩個惱人的問題：
//
//   1) 名字洩漏到外層 scope
//        enum Color { Red, Green, Blue };
//        int Red;        // ❌ 名稱衝突！Red 已經被 enum 用走
//
//   2) 隱式轉成 int
//        enum Color { Red, Green };
//        int x = Red + 1;     // ✅ 編譯通過 — 但語意奇怪
//
//   3) 沒辦法指定底層型別（implementation defined）
//
//  C++11 引入「enum class」：
//
//      enum class Color : uint8_t { Red, Green, Blue };
//      Color c = Color::Red;
//      // int x = c;            // ❌ 編譯錯（不會隱式轉 int）
//      // int y = c + 1;        // ❌
//      int z = static_cast<int>(c);  // ✅ 顯式轉換
//
//  優點：
//   * 強型別 — 不會跟 int 互轉、不會撞名
//   * 可指定底層型別（控制大小）
//   * 用 「Color::Red」這種限定寫法 — 更清楚
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、底層型別與 sizeof                                     │
//  └────────────────────────────────────────────────────────────┘
//
//      enum class Tiny  : uint8_t  { A, B };       // sizeof = 1
//      enum class Med   : uint16_t { A, B };       // sizeof = 2
//      enum class Big   : uint64_t { A, B };       // sizeof = 8
//
//  指定底層型別讓你能精準控制 ABI / 序列化大小。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、配合 switch 使用                                      │
//  └────────────────────────────────────────────────────────────┘
//
//      switch (c) {
//          case Color::Red:   ...; break;
//          case Color::Green: ...; break;
//          case Color::Blue:  ...; break;
//      }
//
//  編譯器（-Wswitch）會檢查「是否所有 enumerator 都處理了」 — enum class
//  比較容易做這檢查（沒洩漏到 scope，case 標籤一定要寫 Color:: 限定）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：enum class 對比 unscoped enum
//   * Demo 2：指定底層型別 + sizeof
//   * Demo 3：switch + 完整覆蓋
//   * Demo 4：底層型別跨界轉換的「正確姿勢」
// =============================================================================

/*
補充筆記：enum_class
  - enum_class 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - enum class 不會自動轉成 int，能避免不同 enum 混用。
  - 使用 enum class 時需要加作用域名稱，例如 Color::Red，這讓大型程式命名更安全。
*/
#include <cstdint>
#include <iostream>
#include <string>

enum class Color : std::uint8_t { Red, Green, Blue };

// 沒指定底層型別 — implementation defined（通常 int）
enum class Mode { Read, Write, Append };

// 老式 unscoped enum（對比用）
enum LegacyColor { LRed, LGreen, LBlue };

static const char* nameOf(Color c) {
    switch (c) {
        case Color::Red:   return "Red";
        case Color::Green: return "Green";
        case Color::Blue:  return "Blue";
    }
    return "?";
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：強型別 vs 弱型別
    // ─────────────────────────────────────────────────────────
    Color c = Color::Red;
    // int n = c;                 // ❌ 編譯錯
    int n = static_cast<int>(c);  // ✅ 顯式
    std::cout << "[Demo1] Color::Red = " << n << " (" << nameOf(c) << ")\n";

    LegacyColor lc = LRed;
    int lci = lc;                 // ✅ unscoped 自動轉 int — 是「壞處」
    std::cout << "[Demo1] LegacyColor::LRed = " << lci << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：底層型別大小
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] sizeof(Color) = " << sizeof(Color)   // 1
              << "  sizeof(Mode) = "        << sizeof(Mode)    // 通常 4
              << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：switch
    // ─────────────────────────────────────────────────────────
    Color picks[] = {Color::Red, Color::Green, Color::Blue};
    std::cout << "[Demo3]";
    for (Color cc : picks) std::cout << ' ' << nameOf(cc);
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：跟整數互轉（顯式）
    //   把 enum class 序列化成 byte / 從 byte 還原
    // ─────────────────────────────────────────────────────────
    Color sender = Color::Blue;
    std::uint8_t wire = static_cast<std::uint8_t>(sender);
    Color receiver = static_cast<Color>(wire);
    std::cout << "[Demo4] wire byte=" << static_cast<int>(wire)
              << " round-trip=" << nameOf(receiver) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：enum class 能當 hash key 嗎？
    //    A：std::hash 對 enum 沒提供特化（C++14 起對 unscoped enum 提供，
    //       enum class 多數實作也支援，但要確認；可以靠 underlying_type
    //       cast 一下）：
    //         std::hash<int>{}(static_cast<int>(c));
    //
    //  Q2：能不能拿 enum class 當 bit flag 用？
    //    A：可以但要自訂 |、& 等運算子（enum class 不會自動有）。我們在
    //       C++_Bit/12_practical_flags.cpp 有完整示範。
    //
    //  Q3：「forward declaration」可以嗎？
    //    A：unscoped enum 在 C++98 不行；C++11 起 enum class（與指定底層
    //       型別的 unscoped enum）可以前向宣告：
    //         enum class Color : uint8_t;   // 前向宣告
    //         enum class Color : uint8_t { Red, Green };  // 之後定義
    //
    // ─────────────────────────────────────────────────────────
    // LeetCode 657. Robot Return to Origin
    //   題意：機器人從 (0,0) 出發，依字串走 U/D/L/R，最後是否回原點。
    //   為什麼放這？把指令 char 對應到強型別的 Direction enum class，
    //                程式語意比直接用 char 清楚。
    // ─────────────────────────────────────────────────────────
    enum class Dir { Up, Down, Left, Right };
    auto charToDir = [](char ch) -> Dir {
        switch (ch) {
            case 'U': return Dir::Up;
            case 'D': return Dir::Down;
            case 'L': return Dir::Left;
            case 'R': default: return Dir::Right;
        }
    };
    auto judgeCircle = [&](const std::string& moves) {
        int x = 0, y = 0;
        for (char ch : moves) {
            switch (charToDir(ch)) {
                case Dir::Up:    ++y; break;
                case Dir::Down:  --y; break;
                case Dir::Left:  --x; break;
                case Dir::Right: ++x; break;
            }
        }
        return x == 0 && y == 0;
    };
    std::cout << std::boolalpha;
    std::cout << "[LC657] \"UD\"   => " << judgeCircle("UD") << '\n';   // true
    std::cout << "[LC657] \"LL\"   => " << judgeCircle("LL") << '\n';   // false

    // ─────────────────────────────────────────────────────────
    // 實用範例：LogLevel — 標準工作上的設定 enum class
    //   工作上常見：log 級別用 enum class 限制比 int 安全很多
    // ─────────────────────────────────────────────────────────
    enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warn, Error };
    auto logIfAbove = [](LogLevel current, LogLevel threshold, const std::string& msg) {
        // enum class 不能直接比較 — 但 underlying 是整數，可以 cast 比
        if (static_cast<int>(current) >= static_cast<int>(threshold)) {
            std::cout << "[log] " << msg << '\n';
        }
    };
    logIfAbove(LogLevel::Info,  LogLevel::Warn, "skipped (Info < Warn)");
    logIfAbove(LogLevel::Error, LogLevel::Warn, "shown (Error >= Warn)");

    return 0;
}
