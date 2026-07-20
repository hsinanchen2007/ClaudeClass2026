// =============================================================================
//  03_what_to_throw.cpp  —  該丟什麼，不該丟什麼
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/throw
//    - https://en.cppreference.com/w/cpp/error/exception        (std::exception 階層)
//    - https://en.cppreference.com/w/cpp/error/runtime_error
//    - https://en.cppreference.com/w/cpp/error/logic_error
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、不要 throw 基本型別                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  語法允許 throw 42; throw "oops"; — 但極不建議：
//
//   * catch 端要寫 catch (int)、catch (const char*)，跟其他「正常」
//     例外無法統一處理（無 what()、無階層）
//   * 沒有有意義訊息
//   * 跨函式庫邊界混用會極混亂（你 throw int，我 throw string）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、應該 throw 繼承 std::exception 的物件                  │
//  └────────────────────────────────────────────────────────────┘
//
//  這樣呼叫端只要寫一次 catch (const std::exception& e) 就能接住所有家族
//  成員，並 e.what() 取訊息。
//
//      throw std::runtime_error{"db connect failed: " + url};
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、自訂 exception class                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  若想讓使用者「能 catch 你模組專屬的錯」，繼承 std::runtime_error 是最
//  簡單的做法 — 不用自己實作 what()：
//
//      class DbError : public std::runtime_error {
//      public:
//          using std::runtime_error::runtime_error;   // 繼承建構子
//      };
//
//  進階：你可以多塞欄位（如 error code、自家 enum），呼叫端用 dynamic_cast
//  或 catch (const DbError& e) 才能拿到。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：throw int 的反面教材
//   * Demo 2：自訂 DbError 並用 catch (const std::exception&) 接
//   * Demo 3：DbError 帶額外欄位 — 用 catch (const DbError&) 取
// =============================================================================

/*
補充筆記：what_to_throw
  - what_to_throw 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - 應丟能表達錯誤語意的型別，不要丟 int、const char* 這種資訊不足的值。
  - 例外物件應可複製或移動，並保存足夠上下文讓上層能診斷問題。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】該丟什麼、什麼時候該丟
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 該拋什麼型別？不該拋什麼？
//     答：拋「繼承自 std::exception 的類別物件」，而且 by value。理由是 what() 提供統一
//     的診斷介面、可以用 catch (const std::exception&) 兜底、能攜帶結構化資訊。不要拋
//     int、const char*、裸指標（誰負責 delete 不清楚），也不要拋不繼承 std::exception
//     的自訂型別（會從所有兜底的 catch 漏掉）。口訣是 throw by value, catch by reference。
//     追問：自訂例外還要注意什麼？（複製建構函式不可拋出例外）
//
// 🔥 Q2. 什麼時候該用例外，什麼時候該用錯誤碼？
//     答：用例外——偵測錯誤處與處理錯誤處隔了多層呼叫（錯誤碼要層層手傳、極易被忽略）、
//     錯誤是罕見的、建構函式失敗（沒有回傳值可用）、運算子重載失敗（operator[] 沒地方
//     回傳錯誤碼）。用錯誤碼／std::expected／std::optional——「失敗」是常規且頻繁的結果、
//     效能關鍵迴圈、呼叫者就在旁邊立刻要處理、跨 ABI 或語言邊界。反面教材是用例外表達
//     「使用者輸入格式錯誤」這種每秒發生數千次的常態。
//     追問：錯誤碼的主要缺點？（容易被忽略，[[nodiscard]] 可緩解；業務邏輯被 if-else 淹沒）
//
// ⚠️ 陷阱. 例外可以跨越動態函式庫（.so / DLL）邊界嗎？
//     答：技術上在同一編譯器、同一標準庫、同一組旗標下可以，但實務上非常脆弱：需要
//     相同的例外 ABI（Itanium 與 MSVC SEH 完全不同）、相同的標準庫實作與版本（否則
//     RTTI 型別資訊不一致，catch 匹配會失敗），Linux 上還需要正確匯出 typeinfo 符號
//     （-fvisibility=hidden 可能導致跨 .so 的 catch 匹配失敗，是很隱蔽的坑）。穩健的
//     外部 API 邊界應該是 C ABI + 錯誤碼，在邊界處用 catch (...) 把例外攔下來轉成錯誤碼。
//     為什麼會錯：以為「都是 C++ 編的就通」，忽略例外傳播重度依賴 ABI 與 RTTI 的一致性。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stdexcept>
#include <string>

// ─── 自訂 exception ───
class DbError : public std::runtime_error {
public:
    enum class Kind { ConnectionFailed, QueryFailed, Timeout };

    DbError(Kind k, const std::string& msg)
        : std::runtime_error(msg), kind_(k) {}

    Kind kind() const noexcept { return kind_; }

private:
    Kind kind_;
};

// 模擬 DB 操作
static void connect(bool succeed) {
    if (!succeed) {
        throw DbError{DbError::Kind::ConnectionFailed,
                      "could not reach db host"};
    }
}

// 反面教材：throw 一個 int
static void badFunc() {
    throw 42;
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：throw int 的反面 — catch 端醜
    // ─────────────────────────────────────────────────────────
    try {
        badFunc();
    } catch (int n) {                     // 不能用 std::exception 接！
        std::cout << "[Demo1] caught int: " << n
                  << " (no .what(), no class hierarchy — please don't)\n";
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：以 std::exception 接 — 統一處理
    // ─────────────────────────────────────────────────────────
    try {
        connect(false);
    } catch (const std::exception& e) {
        std::cout << "[Demo2] caught: " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：以 DbError 接 — 拿到自家欄位
    // ─────────────────────────────────────────────────────────
    try {
        connect(false);
    } catch (const DbError& e) {
        const char* tag = "?";
        switch (e.kind()) {
            case DbError::Kind::ConnectionFailed: tag = "connect"; break;
            case DbError::Kind::QueryFailed:      tag = "query";   break;
            case DbError::Kind::Timeout:          tag = "timeout"; break;
        }
        std::cout << "[Demo3] DbError(" << tag << "): " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：自訂例外攜帶額外欄位（errno 對應 / retry hint）
    //   工作場景：呼叫端不只想看訊息，還想知道「能不能重試」、原始 errno。
    //   做法：派生自 std::runtime_error，多塞欄位；catch 端取自家欄位用 dynamic 行為。
    // ─────────────────────────────────────────────────────────
    {
        struct NetError : public std::runtime_error {
            int    errno_code;
            bool   retryable;
            NetError(int ec, bool retry, const std::string& msg)
                : std::runtime_error(msg), errno_code(ec), retryable(retry) {}
        };
        auto httpGet = [](bool failNow) {
            if (failNow) throw NetError{110, true, "http: timeout"};
        };
        try {
            httpGet(true);
        } catch (const NetError& e) {
            std::cout << "[NetError] errno=" << e.errno_code
                      << " retryable=" << std::boolalpha << e.retryable
                      << " what=" << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：throw 字串字面值的反面教材 — 為什麼不可以
    //   字串字面值型別是 const char*；catch (const char*) 確實能接到，但：
    //     1. 沒有 std::exception 階層 → 無法跟其他例外共用 catch (const std::exception&)
    //     2. 沒有結構化欄位 → 上層無法分流
    //   正確姿勢：包成 std::runtime_error 或自訂 exception class。
    // ─────────────────────────────────────────────────────────
    {
        try {
            throw "boom";                         // ❌ 反面教材
        } catch (const char* msg) {
            std::cout << "[anti-pattern] caught const char*: " << msg
                      << "  (不能用 std::exception 接！)\n";
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：是不是「乾脆全部 throw runtime_error 就好」？
    //    A：標準 runtime_error 已經足夠 90% 的「真錯誤」場合。需要分類處
    //       理（不同錯誤要不同 retry 策略、不同 log）才值得自訂派生類。
    //
    //  Q2：throw new MyError(...) 行不行？
    //    A：✘ 永遠不要。throw 應該丟「值」(會被自動釋放)；throw 指標會
    //       讓 catch 端要 delete，極易洩漏。
    //
    //  Q3：跨 DLL / shared library throw 自訂例外安全嗎？
    //    A：要保證兩邊用「相同 ABI 編譯」(同 compiler、同 flags、同 stdlib)。
    //       否則 RTTI / vtable 不一致會讓 dynamic_cast 失敗、甚至 crash。
    //       業界常見做法：跨邊界一律以 error code 或 error_code/expected
    //       型別傳遞，避免 throw 跨 DLL。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_what_to_throw.cpp -o 03_what_to_throw

// === 預期輸出 ===
// [Demo1] caught int: 42 (no .what(), no class hierarchy — please don't)
// [Demo2] caught: could not reach db host
// [Demo3] DbError(connect): could not reach db host
// [NetError] errno=110 retryable=true what=http: timeout
// [anti-pattern] caught const char*: boom  (不能用 std::exception 接！)
