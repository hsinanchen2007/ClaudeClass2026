// =============================================================================
//  13_user_defined_literals.cpp  —  User-Defined Literals (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/user_literal
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 UDL？                                          │
//  └────────────────────────────────────────────────────────────┘
//
//  C++ 內建字面量有 1, 1u, 1L, 3.14f 之類的 suffix。C++11 起讓你「自定義
//  suffix」 — 寫一個 operator"" _suffix 函式：
//
//      constexpr long double operator""_km(long double v) { return v * 1000.0; }
//      constexpr long double operator""_mile(long double v) { return v * 1609.34; }
//
//      auto d = 3.0_km;            // = 3000.0（公尺）
//      auto m = 1.0_mile;          // = 1609.34
//
//  Suffix 必須以底線 `_` 開頭（標準保留無底線版給 std）。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、可以接什麼參數型別？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      operator""_x(unsigned long long)         // 整數字面量
//      operator""_x(long double)                // 浮點字面量
//      operator""_x(const char* s, std::size_t n)  // 字串字面量
//      operator""_x(char)                        // char
//      operator""_x(char32_t)                    // unicode
//      operator""_x(const char* s)               // raw 字面量（編譯期解析）
//
//  常見：
//      auto t = 100ms;            // <chrono> std::chrono_literals
//      auto s = "hello"s;         // <string> std::string_literals
//      auto v = 0xFF_color;       // 自訂顏色字面量
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、為什麼有用？                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   * 表達單位：1.5_km、200ms — 直觀、避免單位錯誤
//   * 簡化字面量建構：100us、3.14_deg
//   * 自訂強型別字面量（避免「1500 是毫秒還是秒」混淆）
//
//  注意：標準函式庫的「無底線」suffix（s, ms, h, sv 等）只能由標準提供；
//  使用者自定義必須加底線。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：距離單位（_km / _m / _mile）
//   * Demo 2：時間單位（_h / _min / _s 自家版）
//   * Demo 3：建立自訂字串型別
// =============================================================================

/*
補充筆記：user_defined_literals
  - user_defined_literals 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - user-defined literal 可讓數值或字串帶上單位語意，例如 10_km 或 "abc"_sv。
  - literal operator 應保持簡單清楚，過度使用會讓語法像魔法，降低可讀性。
*/
#include <iostream>
#include <string>

// ─── 距離 ───
constexpr long double operator""_km(long double v)   { return v * 1000.0L; }
constexpr long double operator""_m(long double v)    { return v; }
constexpr long double operator""_mile(long double v) { return v * 1609.344L; }

// ─── 時間（自家版本，避開 std::chrono 的內建）───
constexpr long long operator""_hr(unsigned long long h) { return h * 3600; }
constexpr long long operator""_minutes(unsigned long long m) { return m * 60; }
constexpr long long operator""_sec(unsigned long long s) { return s; }

// ─── 字串字面量 → 自訂 class ───
struct Tag { std::string name; };
Tag operator""_tag(const char* s, std::size_t n) {
    return Tag{std::string{s, n}};
}

// ─── byte 大小（實用範例會用到）───
constexpr long long operator""_KB(unsigned long long v) { return v * 1024LL; }
constexpr long long operator""_MB(unsigned long long v) { return v * 1024LL * 1024LL; }

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：距離 — 強迫所有計算回到「公尺」
    // ─────────────────────────────────────────────────────────
    long double total = 5.0_km + 200.0_m + 1.0_mile;
    std::cout << "[Demo1] 5km + 200m + 1mile = " << total << " m\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：時間
    // ─────────────────────────────────────────────────────────
    long long t = 2_hr + 30_minutes + 15_sec;
    std::cout << "[Demo2] 2h + 30m + 15s = " << t << " seconds\n";

    // ─────────────────────────────────────────────────────────
    // Demo 3：字串 → Tag
    // ─────────────────────────────────────────────────────────
    Tag t1 = "important"_tag;
    Tag t2 = "draft"_tag;
    std::cout << "[Demo3] tags: " << t1.name << ", " << t2.name << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：必須有底線嗎？
    //    A：是。標準保留「無底線」給標準函式庫（如 1ms, "abc"s）。使用者
    //       UDL 必須以 _ 開頭，否則編譯失敗或有未定義行為。
    //
    //  Q2：為什麼 operator""_x(unsigned long long) 是整數版？
    //    A：標準規定整數字面量「總是」用 unsigned long long 接，浮點字面
    //       量用 long double 接。不能寫 operator""_x(int)。
    //
    //  Q3：可以結合 chrono_literals 嗎？
    //    A：可以 using namespace std::chrono_literals; 然後混用：
    //         auto t = 200ms + 1_sec;     // 注意自家 _sec 與標準 ms 型別不同
    //       通常建議「整個專案統一一套」 — 別自己再發明 _sec 撞名。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：用先前的 _KB / _MB 表達 buffer 大小
    //   工作上常見：表達 buffer 大小用 _KB / _MB 比裸數字直觀。
    //   （UDL 必須定義在「檔案 scope」 — 見檔案上方的 operator""_KB 等。）
    // ─────────────────────────────────────────────────────────
    long long bufSize = 4_KB;                    // = 4096
    long long pagePool = 16_KB + 32_KB;          // 48 KB
    std::cout << "[Demo4] bufSize = "  << bufSize  << " bytes\n";
    std::cout << "[Demo4] pagePool = " << pagePool << " bytes\n";

    // 配合距離 UDL 表達多單位混合
    long double threshold = 100.0_m + 0.5_km;
    std::cout << "[Demo4] sensor threshold = " << threshold << " m\n";

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：用 _tag 字串 UDL 簡化標記建構
    //   工作上常見：建立 enum-like 標籤或事件名稱
    // ─────────────────────────────────────────────────────────
    Tag tags[] = {"login"_tag, "logout"_tag, "click"_tag};
    std::cout << "[Demo5] tags:";
    for (const auto& t : tags) std::cout << ' ' << t.name;
    std::cout << '\n';

    return 0;
}
