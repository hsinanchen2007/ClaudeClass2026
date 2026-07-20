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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】enum class（C++11）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. enum class 相對傳統 unscoped enum 的優點？
//     答：三點——(1) 作用域：列舉值不洩漏到外層，Color::Red 與 Fruit::Red 不會
//         撞名；(2) 強型別：不隱式轉成 int，要 static_cast，也擋掉不同 enum 之間
//         的誤比較；(3) 可指定底層型別 enum class E : uint8_t {...}，精準控制
//         ABI / 序列化大小。
//     追問：指定底層型別是 scoped enum 專屬的嗎？（不是。C++11 起 unscoped enum
//           也能指定底層型別，這點常被誤答成 enum class 獨有）
//
// 🔥 Q2. enum class 可以前向宣告嗎？
//     答：可以。scoped enum 的底層型別預設為 int，大小已知，所以直接寫
//         enum class Color; 就合法。unscoped enum 則必須顯式寫出底層型別
//         （enum Legacy : int;）才能前向宣告，只寫 enum Legacy; 會編譯錯。
//         （本機 g++ -std=c++11 -pedantic-errors 實測兩者行為如上）
//
// ⚠️ 陷阱. enum class 不能隱式轉 int，那兩個 enum class 值可以用 < 直接比大小嗎？
//     答：可以。同一個 scoped enum 型別之間，==、!=、<、> 等關係運算子都能直接
//         使用，完全不需要 static_cast：
//           Color a = Color::Red, b = Color::Blue;   bool lt = a < b;   // ✅ OK
//         被禁掉的是「隱式轉成 int」與「跨不同 enum 型別比較」——後者如
//         Color::Red == Fruit::Apple 才會 error: no match for 'operator=='。
//         （本機 g++ -std=c++11 -pedantic-errors 實測兩種情形）
//     為什麼會錯：把「不隱式轉 int」過度推論成「任何比較都得先 cast」。實務後果
//         就在本檔下方的 logIfAbove：它把兩個 LogLevel 都 cast 成 int 才比大小，
//         那兩個 static_cast 其實是多餘的，直接寫 current >= threshold 即可。
// ═══════════════════════════════════════════════════════════════════════════

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
        // ⚠️ 更正：【同型別】的 enum class 可以直接比較（==、!=、<、> 都合法）。
        //    不能做的是「與整數或與不同 enum 型別」比較——那才需要 cast。
        if (static_cast<int>(current) >= static_cast<int>(threshold)) {
            std::cout << "[log] " << msg << '\n';
        }
    };
    logIfAbove(LogLevel::Info,  LogLevel::Warn, "skipped (Info < Warn)");
    logIfAbove(LogLevel::Error, LogLevel::Warn, "shown (Error >= Warn)");

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 12_enum_class.cpp -o 12_enum_class

// === 預期輸出 ===
// [Demo1] Color::Red = 0 (Red)
// [Demo1] LegacyColor::LRed = 0
// [Demo2] sizeof(Color) = 1  sizeof(Mode) = 4
// [Demo3] Red Green Blue
// [Demo4] wire byte=2 round-trip=Blue
// [LC657] "UD"   => true
// [LC657] "LL"   => false
// [log] shown (Error >= Warn)
