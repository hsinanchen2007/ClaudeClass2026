// =============================================================================
//  08_nested_exception.cpp  —  std::throw_with_nested / rethrow_if_nested
// =============================================================================
//  參考：
//    https://en.cppreference.com/w/cpp/error/throw_with_nested
//    https://en.cppreference.com/w/cpp/error/rethrow_if_nested
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼需要 nested exception？                          │
//  └────────────────────────────────────────────────────────────┘
//
//  常見場景：「下層 throw 了一個底層錯誤；上層想加上『發生在做什麼操作時』
//  的 context，但又不想丟失原始錯誤資訊」。
//
//  例：
//
//      void readConfig(const std::string& path) {
//          try {
//              parseFile(path);                  // 可能 throw runtime_error
//          } catch (...) {
//              std::throw_with_nested(
//                std::runtime_error{"failed to read config: " + path}
//              );
//          }
//      }
//
//  此時例外有「兩層」：
//   * 外層：runtime_error("failed to read config: ...")
//   * 內層：原始 parseFile 拋的例外
//
//  catch 端可以用 std::rethrow_if_nested 把內層拿出來「往下挖」：
//
//      try { readConfig("a.cfg"); }
//      catch (const std::exception& e) {
//          printRecursive(e);   // 印整條 chain
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、API                                                    │
//  └────────────────────────────────────────────────────────────┘
//
//      std::throw_with_nested(MyEx{...})
//          建立一個「同時繼承 MyEx 與 std::nested_exception」的物件並 throw；
//          內部會自動把當前正在處理的 exception 存起來。
//
//      std::rethrow_if_nested(e)
//          若 e 是 nested_exception，把它儲存的內層 exception 重新 throw；
//          否則什麼都不做。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * 三層呼叫鏈，每一層加一個 context；最外層用遞迴 catch 印整條 chain
// =============================================================================

/*
補充筆記：nested_exception
  - nested_exception 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - nested_exception 可保留低階例外，同時在高階加入更多上下文。
  - 使用 throw_with_nested 後，上層可遞迴印出完整原因鏈。
*/
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

static void parseLine(const std::string& s) {
    if (s.empty()) {
        throw std::invalid_argument{"parseLine: empty"};
    }
}

static void parseFile(const std::string& path) {
    try {
        parseLine("");                            // 故意觸發底層錯誤
    } catch (...) {
        std::throw_with_nested(
            std::runtime_error{"parseFile: " + path});
    }
}

static void readConfig(const std::string& path) {
    try {
        parseFile(path);
    } catch (...) {
        std::throw_with_nested(
            std::runtime_error{"readConfig failed"});
    }
}

// 遞迴印整條 exception chain
static void printRecursive(const std::exception& e, int level = 0) {
    std::cout << std::string(level * 2, ' ')
              << "- " << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& inner) {
        printRecursive(inner, level + 1);
    } catch (...) {
        std::cout << std::string((level + 1) * 2, ' ')
                  << "- (unknown nested)\n";
    }
}

int main() {
    try {
        readConfig("server.cfg");
    } catch (const std::exception& e) {
        std::cout << "[chain]\n";
        printRecursive(e);
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：把 nested chain 「攤平成一條字串」 — 寫 log 時超方便
    //   常見需求：把 chain 變成 "high: ...; caused by: low: ..." 一行
    // ─────────────────────────────────────────────────────────
    {
        // 收集 chain 上每層 what() 成一個字串
        std::string flat;
        auto collect = [&flat](const std::exception& e, auto& self) -> void {
            if (!flat.empty()) flat += "; caused by: ";
            flat += e.what();
            try { std::rethrow_if_nested(e); }
            catch (const std::exception& inner) { self(inner, self); }
            catch (...) { /* 忽略非 std 例外 */ }
        };
        try {
            readConfig("user.cfg");
        } catch (const std::exception& e) {
            collect(e, collect);
            std::cout << "[flat] " << flat << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：API gateway 風格 — 內部錯誤被包成「業務錯誤」對外
    //   工作場景：上層只想看「failed to charge user」，但保留底層原因供
    //   debug；nested exception 是這種「公開 vs 內部訊息」的最佳工具。
    // ─────────────────────────────────────────────────────────
    {
        struct ChargeError : public std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        auto chargeUser = [](int userId) {
            try {
                throw std::runtime_error{"db: connection refused"};
            } catch (...) {
                std::throw_with_nested(ChargeError{
                    "failed to charge user " + std::to_string(userId)});
            }
        };
        try {
            chargeUser(42);
        } catch (const ChargeError& e) {
            std::cout << "[public] " << e.what() << '\n';
            // 想要 debug 才挖底層
            try { std::rethrow_if_nested(e); }
            catch (const std::exception& inner) {
                std::cout << "[internal] root cause: " << inner.what() << '\n';
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：可以加多深的 chain？
    //    A：理論上沒上限。實務上建議「保留最相關的 1~3 層」 — 太多會像
    //       Java 的 stack trace 一樣冗長難讀。
    //
    //  Q2：什麼時候用 nested、什麼時候直接 throw 新的？
    //    A：要保留「下層的根本原因」就用 nested；只是「轉換到新型別」可
    //       直接 throw 新的（用 e.what() 內嵌訊息）。
    //
    //  Q3：std::throw_with_nested 對非 class 型別會怎樣？
    //    A：要求 throw 的是 class type — 內部要 multiple inheritance from
    //       std::nested_exception。throw_with_nested(42) 不能編譯。
    //
    return 0;
}
