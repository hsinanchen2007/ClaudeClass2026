// =============================================================================
//  14_raw_string.cpp  —  原始字串字面量 R"(...)" (C++11)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/string_literal
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼需要 raw string？                                │
//  └────────────────────────────────────────────────────────────┘
//
//  寫 regex / Windows 路徑 / JSON 內嵌字串時，反斜線地獄：
//
//      // 想要 regex：(\d+)\s*:\s*(\d+)
//      const char* re = "(\\d+)\\s*:\\s*(\\d+)";
//
//  C++11 raw string：「裡面內容原封不動取用，不解 escape」：
//
//      const char* re = R"((\d+)\s*:\s*(\d+))";
//      //               ^^^                ^^^
//      //              R"(  ...           )"
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      R"delimiter(content)delimiter"
//
//   * delimiter 可以是 0~16 個字元（不含括號、空白、反斜線）
//   * content 完全照寫，連 \、"、換行都不解
//
//  例：
//      R"(hello "world")"        → hello "world"
//      R"(line1
//        line2)"                  → 真的兩行字串
//      R"foo(internal "(": ok)foo"  → 內容含 ) — 用自訂 delim 區隔
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、用途                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * regex pattern
//   * Windows 檔案路徑：R"(C:\Users\me\file.txt)"
//   * 內嵌 JSON / SQL / shell command
//   * 多行 SQL / template
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：跟普通字串對比
//   * Demo 2：多行 SQL
//   * Demo 3：自訂 delimiter 處理含 ) 的字串
// =============================================================================

/*
補充筆記：raw_string
  - raw_string 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - raw string literal 適合正規表示式、JSON、SQL 等含大量反斜線的文字。
  - 自訂 delimiter 可避免內容中出現 )" 時提早結束字串。
*/
#include <iostream>
#include <string>

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：跟普通字串對比
    // ─────────────────────────────────────────────────────────
    const char* old_re = "(\\d+)\\s*:\\s*(\\d+)";
    const char* raw_re = R"((\d+)\s*:\s*(\d+))";
    std::cout << "[Demo1] old: " << old_re << '\n';
    std::cout << "[Demo1] raw: " << raw_re << '\n';

    const char* path = R"(C:\Users\hsinan\code\file.txt)";
    std::cout << "[Demo1] path: " << path << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：多行 SQL — 真的有換行
    // ─────────────────────────────────────────────────────────
    const std::string sql = R"(
SELECT id, name
  FROM users
 WHERE active = 1
   AND age > 18;
)";
    std::cout << "[Demo2] SQL:" << sql;

    // ─────────────────────────────────────────────────────────
    // Demo 3：自訂 delimiter — 字串內部含 ) 不會誤當結尾
    // ─────────────────────────────────────────────────────────
    const std::string code = R"py(
def f(x):
    return (x + 1) * 2     # 這行有 ) 不會跟 R"( 衝突
)py";
    std::cout << "[Demo3] python:" << code;

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：raw string 真的「不解任何 escape」嗎？
    //    A：對 — 包括 \n、\t、\\ 都按字面看。要在 raw string 裡放真實的
    //       「換行」，直接按 Enter 即可，那個換行就是 '\n'。
    //
    //  Q2：raw string 可以跟 prefix 結合嗎？
    //    A：可以：u8R"(...)"（UTF-8 raw）、LR"(...)"（wide raw）、
    //       u"R"(...)"（char16_t raw）等。常用 u8R 表達確定是 UTF-8 編碼。
    //
    //  Q3：怎麼知道 delimiter 該選什麼？
    //    A：原則：選一個內容裡「絕不會出現」的 token。常見 R"foo(...)foo"、
    //       R"x(...)x" — 短就好。內容含 )foo 時換 R"bar(...)bar"。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：內嵌 JSON 設定 — 多行 + 雙引號免 escape
    //   工作上常見：測試裡 hard-code 一段 JSON 樣本。
    // ─────────────────────────────────────────────────────────
    const std::string config_json = R"({
  "name": "demo",
  "host": "localhost",
  "port": 8080,
  "paths": ["/api", "/health"]
})";
    std::cout << "[Demo4] config_json:" << config_json << '\n';

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：HTML/XML 樣板 — 雙引號免 escape
    //   工作上常見：產生 HTTP response、SVG 樣板
    // ─────────────────────────────────────────────────────────
    const std::string html = R"(<a href="https://example.com" target="_blank">click me</a>)";
    std::cout << "[Demo5] html: " << html << '\n';

    return 0;
}
