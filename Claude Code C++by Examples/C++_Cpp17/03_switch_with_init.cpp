// =============================================================================
//  03_switch_with_init.cpp  —  switch 帶 init statement (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/switch
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      switch (init_statement; expr) {
//          case A: ...
//          case B: ...
//      }
//
//  跟 if 帶 init 一樣 — 把臨時變數的 scope 限縮在 switch 內。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、典型場景                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 「拿到一個 token / status，根據它分流處理」
//   * 「先計算一個值，再用它做 switch」 — 計算結果不污染外層
//
//  例：
//
//      switch (auto status = parse(input); status) {
//          case Status::OK:    handle(input); break;
//          case Status::Retry: schedule(); break;
//          case Status::Fail:  log(input); break;
//      }
//      // status 已出 scope
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：HTTP-like status code 分流
//   * Demo 2：用 switch 帶 init 在「迴圈中」處理多種事件
// =============================================================================

/*
補充筆記：switch_with_init
  - switch_with_init 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - switch init-statement 適合先計算分類值，再依 case 分支處理。
  - init 變數作用域涵蓋整個 switch，case 內若要宣告變數仍需注意大括號。
*/
#include <iostream>
#include <string>
#include <vector>

enum class Status { OK = 200, NotFound = 404, ServerError = 500, Retry = 503 };

static Status classify(int code) {
    if (code == 200) return Status::OK;
    if (code == 404) return Status::NotFound;
    if (code == 503) return Status::Retry;
    return Status::ServerError;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：HTTP-like 分流
    // ─────────────────────────────────────────────────────────
    int codes[] = {200, 404, 500, 503};
    for (int c : codes) {
        switch (auto s = classify(c); s) {
            case Status::OK:
                std::cout << "[Demo1] " << c << " → OK\n";
                break;
            case Status::NotFound:
                std::cout << "[Demo1] " << c << " → NotFound\n";
                break;
            case Status::Retry:
                std::cout << "[Demo1] " << c << " → Retry\n";
                break;
            case Status::ServerError:
                std::cout << "[Demo1] " << c << " → ServerError\n";
                break;
        }
        // s 已出 scope — 不會跨 case 殘留
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 switch 帶 init 處理「混合事件」
    //   想像一個 event queue 處理迴圈
    // ─────────────────────────────────────────────────────────
    struct Event { int kind; int payload; };
    std::vector<Event> events{{1, 10}, {2, 20}, {3, 30}, {1, 40}};

    int count1 = 0, count2 = 0, total = 0;
    for (auto& e : events) {
        // init: 直接把 payload * 2 算好；switch 拿 e.kind 分流
        switch (auto doubled = e.payload * 2; e.kind) {
            case 1: ++count1; total += doubled; break;
            case 2: ++count2; total += doubled; break;
            default: total += e.payload; break;
        }
    }
    std::cout << "[Demo2] count1=" << count1 << " count2=" << count2
              << " total=" << total << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：init 變數可以在 case 內 redeclare 嗎？
    //    A：不行 — 它是「switch 整體 scope」內可見的；case 內再宣告同名
    //       會 shadow（編譯器可能警告）。
    //
    //  Q2：和「if 帶 init」相比，這個更實用還是更少用？
    //    A：通常 if 帶 init 用得更多（map.find 等 idiom 太常見）；switch
    //       帶 init 在「parse 結果分流」型場景有用。
    //
    //  Q3：跟 [[fallthrough]] 結合？
    //    A：可以，沒衝突。常見「parse + 多分支」程式碼會兩個一起用。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：根據檔案副檔名分流
    //   工作上常見：HTTP server 根據副檔名決定 Content-Type
    // ─────────────────────────────────────────────────────────
    auto extOf = [](const std::string& path) {
        auto pos = path.rfind('.');
        return pos == std::string::npos ? std::string{} : path.substr(pos);
    };
    enum class Ext { Cpp, Hpp, Txt, Other };
    auto extEnum = [](const std::string& s) {
        if (s == ".cpp") return Ext::Cpp;
        if (s == ".hpp" || s == ".h") return Ext::Hpp;
        if (s == ".txt") return Ext::Txt;
        return Ext::Other;
    };
    for (const char* path : {"main.cpp", "header.hpp", "readme.txt", "image.png"}) {
        switch (auto e = extEnum(extOf(path)); e) {
            case Ext::Cpp:   std::cout << "[Demo3] " << path << " => C++ source\n"; break;
            case Ext::Hpp:   std::cout << "[Demo3] " << path << " => C++ header\n"; break;
            case Ext::Txt:   std::cout << "[Demo3] " << path << " => text file\n"; break;
            case Ext::Other: std::cout << "[Demo3] " << path << " => unknown\n"; break;
        }
        // e 已出 scope — 下一輪重來
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：retry 策略 — 根據錯誤 code 計算 backoff
    //   工作上常見：HTTP client retry，根據 status 算等多久
    // ─────────────────────────────────────────────────────────
    auto computeBackoff = [](int code) {
        switch (auto kind = code / 100; kind) {
            case 4: return 0;          // 4xx 不重試
            case 5: return 1000;       // 5xx 等 1 秒
            default: return -1;        // 其它不認識
        }
    };
    std::cout << "[Demo4] backoff(404)=" << computeBackoff(404) << "ms\n";
    std::cout << "[Demo4] backoff(503)=" << computeBackoff(503) << "ms\n";

    return 0;
}
