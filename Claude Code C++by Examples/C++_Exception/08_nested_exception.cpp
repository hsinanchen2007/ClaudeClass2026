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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】nested_exception 與 exception_ptr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::throw_with_nested / rethrow_if_nested 解決什麼問題？
//     答：把「低層例外」包在「高層例外」裡，形成例外鏈（類似 Java 的 caused by），
//     讓上層能同時看到「做什麼失敗了」與「底層的真正原因」：
//     try { parse(file); }
//     catch (...) { std::throw_with_nested(std::runtime_error("解析 " + file + " 失敗")); }
//     接收端則遞迴用 std::rethrow_if_nested 展開，印出完整的因果鏈。
//     追問：為什麼不直接把原因字串接起來就好？（那會丟失原例外的型別，上層無法針對
//     特定型別做處理，只剩一串文字）
//
// 🔥 Q2. std::exception_ptr 是什麼？為什麼跨執行緒需要它？
//     答：它是可拷貝、可跨執行緒傳遞的「例外物件控制代碼」：在 catch 中用
//     std::current_exception() 取得，在別處用 std::rethrow_exception(p) 重新拋出。
//     需要它是因為例外只在單一執行緒的呼叫堆疊內傳播，逃出 thread 函式頂層就是
//     std::terminate()。std::promise::set_exception 與 std::async 正是用這個機制把子
//     執行緒的例外送回主執行緒。
//     追問：std::thread 的可呼叫物件拋例外會怎樣？（直接 std::terminate()；要讓例外被
//     封裝進 future 必須用 std::async 或 std::packaged_task）
//
// ⚠️ 陷阱. 在 catch (...) 裡面，可以完全不知道例外型別就把它保存下來嗎？
//     答：可以，這正是 exception_ptr 的用途——std::current_exception() 不需要知道型別。
//     反過來說，若只是想「查看」型別資訊，catch (...) 本身拿不到任何東西，得用
//     try { throw; } catch (const std::exception& e) 這種「rethrow 再細分」的手法。
//     為什麼會錯：以為 catch (...) 什麼資訊都拿不到、只能放棄；實際上 current_exception()
//     讓你可以先原封不動地保存，稍後再決定怎麼處理。
// ═══════════════════════════════════════════════════════════════════════════

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

// 編譯: g++ -std=c++20 -Wall -Wextra 08_nested_exception.cpp -o 08_nested_exception

// === 預期輸出 ===
// [chain]
// - readConfig failed
//   - parseFile: server.cfg
//     - parseLine: empty
// [flat] readConfig failed; caused by: parseFile: user.cfg; caused by: parseLine: empty
// [public] failed to charge user 42
// [internal] root cause: db: connection refused
