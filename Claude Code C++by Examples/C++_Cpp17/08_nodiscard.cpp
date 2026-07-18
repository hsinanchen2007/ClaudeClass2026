// =============================================================================
//  08_nodiscard.cpp  —  [[nodiscard]] (C++17)
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/attributes/nodiscard
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 [[nodiscard]]？                                 │
//  └────────────────────────────────────────────────────────────┘
//
//  「呼叫端忘記用回傳值就警告」的屬性。
//
//      [[nodiscard]] int parse();
//
//      int main() {
//          parse();         // ⚠️ 警告：discarding return value
//          int n = parse(); // ✅ OK
//      }
//
//  典型用途：「忽略回傳值會 bug」的函式 — error code、status flag、新建
//  物件、容器 size 等。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、可以加在哪？                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   * 函式宣告
//   * class / struct（影響「回傳這個型別的所有函式」 — 工廠返回值都會檢查）
//   * enum（同理）
//
//      class [[nodiscard]] FileHandle { ... };     // 任何回 FileHandle 的函式
//                                                   // 呼叫端忽略結果就警告
//
//  C++20 加了帶訊息版：
//
//      [[nodiscard("don't ignore the error code!")]]
//      int doIt();
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：函式級 nodiscard
//   * Demo 2：class 級 nodiscard
//
//   注意：「忽略」會產生 warning — 我們把警告壓掉以免 -Wall -Wextra 把它
//   當錯誤；實務上你會看到 warning 並去修。
// =============================================================================

/*
補充筆記：nodiscard
  - nodiscard 是現代 C++ 語法或標準庫特性；學習時要把「少寫字」和「語意更精確」分開看。
  - auto 讓型別由初始化式推導，但會丟掉 top-level const/reference；需要保留引用語意時要寫 auto&、const auto& 或 decltype(auto)。
  - brace initialization 能減少未初始化與 narrowing，但遇到 initializer_list overload 可能選到不同建構子。
  - constexpr、static_assert、if constexpr 把部分錯誤和計算提前到編譯期，能讓 template 和常數邏輯更清楚。
  - 屬性如 [[nodiscard]]、[[maybe_unused]]、[[fallthrough]] 是對編譯器和讀者的意圖標記，不應拿來掩蓋設計問題。
  - string_view、optional、variant、structured binding 等特性改善介面表達力，但也帶來生命週期或狀態檢查責任。
  - [[nodiscard]] 適合標記錯誤碼、optional、資源 handle 等不可忽略的回傳值。
  - 不要濫用 nodiscard 到所有函式；太多警告會讓真正重要的警告被忽略。
*/
#include <iostream>
#include <string>
#include <utility>

[[nodiscard]] int parseAge(const std::string& s) {
    return s.empty() ? -1 : std::stoi(s);
}

class [[nodiscard]] Result {
public:
    Result(int code, std::string msg) : code_(code), msg_(std::move(msg)) {}
    int code() const { return code_; }
    const std::string& msg() const { return msg_; }
private:
    int code_;
    std::string msg_;
};

static Result doWork() {
    return {0, "ok"};
}

int main() {
    int age = parseAge("42");
    std::cout << "[Demo1] age = " << age << '\n';

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    parseAge("99");                  // ⚠️ 故意忽略 — 編譯器會警告
    doWork();                         // ⚠️ 同樣警告
#pragma GCC diagnostic pop

    Result r = doWork();
    std::cout << "[Demo2] code=" << r.code() << " msg=" << r.msg() << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：警告會升級為錯誤嗎？
    //    A：預設不會 — 是 -Wunused-result 警告。要強制 error 用
    //       -Werror=unused-result。許多大專案會啟用。
    //
    //  Q2：明知要忽略怎麼辦？
    //    A：用 (void) 顯式拋掉：
    //         (void)parseAge("99");
    //       明寫意圖 — 編譯器不再警告。
    //
    //  Q3：class 級 nodiscard 的細微規則？
    //    A：「prvalue of class type」被 discard 時警告 — 也就是「函式回傳
    //       這型別」的回傳值忽略。對「move 進來的 lvalue」不警告。
    //
    // ─────────────────────────────────────────────────────────
    // 實用範例 1：錯誤碼 guard — 強迫呼叫端檢查回傳值
    //   工作上常見：寫底層 API（檔案、網路），錯誤碼不能被忽略。
    // ─────────────────────────────────────────────────────────
    enum class [[nodiscard]] IoStatus { Ok, NotFound, PermDenied, IoError };
    auto openFile = [](const std::string& path) -> IoStatus {
        if (path.empty()) return IoStatus::NotFound;
        return IoStatus::Ok;
    };
    if (auto st = openFile("config.ini"); st != IoStatus::Ok) {
        std::cout << "[Demo3] open failed, code=" << static_cast<int>(st) << '\n';
    } else {
        std::cout << "[Demo3] open success\n";
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    openFile("");      // ⚠️ 編譯器會警告：discarded IoStatus
#pragma GCC diagnostic pop

    // ─────────────────────────────────────────────────────────
    // 實用範例 2：「建造一個物件就要拿走」 — Builder 模式回傳
    //   工作上常見：builder.build() 忘了接住就白做工
    // ─────────────────────────────────────────────────────────
    class Request {
    public:
        std::string url;
        int timeout_ms;
    };
    class RequestBuilder {
    public:
        RequestBuilder& setUrl(std::string u)   { r_.url = std::move(u); return *this; }
        RequestBuilder& setTimeout(int t)        { r_.timeout_ms = t; return *this; }
        // 注意：[[nodiscard("msg")]] 帶訊息是 C++20；C++17 只能寫 [[nodiscard]]
        [[nodiscard]] Request build() { return std::move(r_); }
    private:
        Request r_{};
    };
    auto req = RequestBuilder{}.setUrl("https://example.com").setTimeout(5000).build();
    std::cout << "[Demo4] built: " << req.url << " timeout=" << req.timeout_ms << '\n';

    return 0;
}
