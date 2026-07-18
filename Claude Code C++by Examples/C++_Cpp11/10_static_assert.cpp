// =============================================================================
//  10_static_assert.cpp  —  static_assert (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/static_assert
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 static_assert？                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  「編譯期斷言」 — 給定一個編譯期可求值的 bool 條件，false 時編譯失敗。
//
//      static_assert(condition, "error message");
//
//  C++17 起 message 可省略（直接拿 condition 的字串當訊息）。
//
//  例：
//      static_assert(sizeof(int) >= 4, "we assume 32-bit int");
//      static_assert(sizeof(void*) == 8, "expect 64-bit platform");
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、跟 runtime assert 的差異                              │
//  └────────────────────────────────────────────────────────────┘
//
//   assert(cond)     <cassert>，runtime 失敗終止程式（release 預設關掉）
//   static_assert    編譯期；條件必須是常數表達式；release/debug 一律檢查
//
//  static_assert 的真價值：把「平台 / 結構假設」明寫進程式碼，搬到別平台
//  就直接編譯錯，不會跑起來才神秘炸掉。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、典型用途                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 平台 / ABI 假設：sizeof(int) == 4、sizeof(void*) == 8
//   * 結構記憶體布局：sizeof(MyHeader) == 16（serialization 必要假設）
//   * Template 約束：static_assert(std::is_integral<T>::value, ...)
//   * 編譯期計算結果驗證：static_assert(factorial(5) == 120)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、與 type_traits 搭配                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      template <class T>
//      void f(T x) {
//          static_assert(std::is_integral<T>::value,
//                        "f only accepts integer types");
//          ...
//      }
//
//  比 SFINAE / concept (C++20) 更直接 — 但錯誤訊息出在「使用點內部」，
//  較不易讀。對 「pre-C++20 想加 type 約束」很實用。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：平台假設
//   * Demo 2：constexpr 計算結果驗證
//   * Demo 3：template 中 type 約束
// =============================================================================

/*
補充筆記：static_assert
  - static_assert 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - static_assert 在編譯期檢查條件，常用於 template 約束與平台假設。
  - 錯誤訊息應寫成人能理解的句子，否則 template 失敗時很難定位原因。
*/
#include <cstdint>
#include <iostream>
#include <type_traits>

constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

template <class T>
T sumOnly(T a, T b) {
    static_assert(std::is_arithmetic<T>::value,
                  "sumOnly: T must be arithmetic (int / double / etc.)");
    return a + b;
}

// 假設我們序列化會用到 32-bit pack 的 header
struct alignas(4) Header {
    char     magic[4];
    uint32_t length;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：平台假設
    // ─────────────────────────────────────────────────────────
    static_assert(sizeof(int) >= 4, "we require >= 32-bit int");
    static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
    static_assert(sizeof(Header) == 8, "Header must pack into 8 bytes");
    std::cout << "[Demo1] sizeof(int)="    << sizeof(int)
              << " sizeof(Header)="        << sizeof(Header) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：constexpr 結果驗證
    // ─────────────────────────────────────────────────────────
    static_assert(factorial(5) == 120, "factorial wrong");
    static_assert(factorial(0) == 1, "factorial(0) should be 1");
    std::cout << "[Demo2] factorial 1..6:";
    for (int i = 1; i <= 6; ++i) std::cout << ' ' << factorial(i);
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：template type 約束
    // ─────────────────────────────────────────────────────────
    auto x = sumOnly(1, 2);          // OK
    auto y = sumOnly(1.5, 2.5);      // OK
    // auto z = sumOnly(std::string{"a"}, std::string{"b"}); // ❌ 編譯錯
    std::cout << "[Demo3] sumOnly(1,2)=" << x
              << " sumOnly(1.5,2.5)=" << y << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：static_assert(false) 會編譯失敗嗎？
    //    A：在 template 外會。在 template 中，「依賴 template parameter」
    //       的 false 才會等到實例化時才檢查；不依賴的會立刻失敗。
    //       如果想「永遠失敗到實例化」，需技巧：
    //         template <class T>
    //         static_assert(sizeof(T*) == 0, "...");
    //
    //  Q2：static_assert 失敗時如何看清楚錯誤？
    //    A：訊息要清楚 — 不要只 "error"，要寫明「假設是什麼、違反了什麼」。
    //       搬到別 ABI 時看到訊息能立刻知道問題。
    //
    //  Q3：C++17 的「無 message」版怎麼寫？
    //    A：static_assert(condition); 編譯器會把條件字面量當訊息印出。但
    //       自訂訊息仍然好讀許多 — 一般還是建議寫。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：序列化結構 — 確保結構大小恆定
    //   工作上常見：跨平台二進位序列化，packed 結構大小錯了就 ABI 崩。
    // ─────────────────────────────────────────────────────────
    struct PacketHeader {
        uint16_t magic;
        uint16_t version;
        uint32_t size;
    };
    static_assert(sizeof(PacketHeader) == 8, "PacketHeader must be 8 bytes");
    std::cout << "[Demo4] sizeof(PacketHeader) = " << sizeof(PacketHeader) << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：用 static_assert 鎖定 enum 範圍
    //   工作上常見：定義 enum 後保證底層型別、最大值不變
    // ─────────────────────────────────────────────────────────
    enum class LogLevel : uint8_t { Trace, Debug, Info, Warn, Error, Fatal };
    static_assert(sizeof(LogLevel) == 1, "LogLevel must fit in 1 byte (network ABI)");
    static_assert(static_cast<uint8_t>(LogLevel::Fatal) < 16, "LogLevel max < 16");
    std::cout << "[Demo5] LogLevel ABI invariants verified\n";

    return 0;
}
