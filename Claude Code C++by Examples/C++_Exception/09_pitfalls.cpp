// =============================================================================
//  09_pitfalls.cpp  —  Exception 常見陷阱與反面教材
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/exceptions
//    - https://en.cppreference.com/w/cpp/language/destructor   (destructor 與 noexcept)
//    - https://en.cppreference.com/w/cpp/language/noexcept_spec (noexcept 規格)
//
//  集中列出最容易踩的雷。每個陷阱都附「為什麼錯」「怎麼正確做」。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 1：destructor 丟例外                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  destructor 預設 noexcept；throw 出來會直接 terminate。即使你顯式寫
//  noexcept(false)，遇到「另一個 exception 還在 unwinding」時 throw，會立
//  刻 std::terminate（不會被外層 catch 接到）。
//
//  正確做法：
//   * destructor 不丟例外；要做可能失敗的清理，提供「主動呼叫」的方法
//     (close、commit)。例如：
//        File f;
//        f.write(...);
//        f.close();         // 失敗 → throw 在這
//        // f 析構僅作 best-effort，不再 throw
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 2：catch (...) 後不重 throw                           │
//  └────────────────────────────────────────────────────────────┘
//
//  「靜默吞掉」未知例外 → 你失去了診斷錯誤的機會、bug 變成「啞掉的服務」。
//
//      try { ... }
//      catch (...) {
//          // 什麼都不做     ← 反面教材
//      }
//
//  正確：catch (...) 通常要 (a) log 訊息 (b) 重 throw 或轉成業務錯誤。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 3：把例外當控制流                                    │
//  └────────────────────────────────────────────────────────────┘
//
//      // 反面教材：用 throw 跳出多層迴圈
//      try {
//          for (...) for (...) {
//              if (found) throw FoundIt{};
//          }
//      } catch (FoundIt&) { ... }
//
//  例外有 unwinding 成本（雖然「正常路徑」近乎免費，throw 路徑通常微秒
//  級）；更重要：模糊了「正常控制流」與「錯誤」的差別。多層跳出用
//  goto 或拆 helper function。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 4：catch by value                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  見 04_catch_by_ref.cpp — 永遠 catch (const X& e)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 5：rethrow 寫成 `throw e;`                            │
//  └────────────────────────────────────────────────────────────┘
//
//      catch (const std::exception& e) {
//          log(...);
//          throw e;             // ❌ 拷貝 e（slicing 為 std::exception）
//      }
//      catch (const std::exception& e) {
//          log(...);
//          throw;               // ✅ 保留原型別的 rethrow
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 6：在 noexcept 函式裡 throw                           │
//  └────────────────────────────────────────────────────────────┘
//
//  立刻 std::terminate。所以 noexcept 不是「美化裝飾」 — 是強承諾。確
//  保函式內所有可能 throw 的呼叫都已經處理或 wrap 起來。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 陷阱 7：thread 入口拋出未被接                              │
//  └────────────────────────────────────────────────────────────┘
//
//  std::thread 的「入口函式」不被任何 catch 包住、又 throw 出來 →
//  std::terminate。應該在入口函式最外層 try/catch 包好。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範可運行的部分                                       │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：rethrow 寫對 vs 寫錯
//   * Demo 2：catch (...) + 紀錄 + rethrow
// =============================================================================

/*
補充筆記：pitfalls
  - pitfalls 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - 不要在 destructor 中讓例外逃出，特別是 stack unwinding 已在進行時。
  - 不要用例外控制一般迴圈流程；例外應保留給異常或無法就地處理的錯誤。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】例外常見陷阱
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼解構函式不能拋例外？
//     答：若解構函式是在 stack unwinding 過程中被呼叫（已經有一個例外在傳播），此時再
//     逃出第二個例外，執行時期無法決定該處理哪一個，於是直接呼叫 std::terminate()
//     （預設 std::abort()），程式當場死亡、其餘物件不再解構。而且自 C++11 起解構函式
//     預設隱含 noexcept(true)，所以即使不在 unwinding 過程中，只要例外逃出解構函式，
//     一樣直接 terminate。正確做法是在解構函式內把可能拋的操作包在 try-catch 裡吞掉或
//     記 log；若呼叫者需要知道結果，就另外提供一個「可拋的顯式 close()」。
//     追問：那 fstream 為什麼要提供 close()？（正是這個道理——解構時的 flush 失敗會被
//     靜默吞掉，要檢查寫入是否成功就得顯式 close 再檢查狀態）
//
// 🔥 Q2. std::terminate 會在哪些情況被呼叫？
//     答：主要有：① 例外傳播到 main 之外仍未被捕捉 ② 例外逃出 noexcept 函式 ③ 例外
//     逃出解構函式（C++11 起隱含 noexcept）④ 在 unwinding 過程中解構函式又拋出例外
//     ⑤ 例外逃出 std::thread 的可呼叫物件、或逃出 main／靜態物件的建構或解構
//     ⑥ 在沒有正在處理的例外時執行 throw; ⑦ std::thread 物件在 joinable 狀態下被解構
//     （未 join 也未 detach——這條常被忽略但很常考）。預設行為是 std::abort()，可用
//     std::set_terminate 安裝自訂 handler。
//     追問：terminate 時解構函式會跑嗎？（不保證，標準允許實作不做 unwinding）
//
// ⚠️ 陷阱 1. 「只要同時有兩個例外存在就會 terminate」——這句話對嗎？
//     答：不精確。正確描述是「在 stack unwinding 過程中，若某個解構函式讓例外逃出，
//     才會 terminate」。同時存在兩個例外本身完全合法：
//     try { throw 1; } catch (int) { try { throw 2; } catch (int) { } }
//     這裡確實有兩個例外先後存在，但第二個被自己的 handler 接住了，沒有在 unwinding
//     途中逃出解構函式，所以毫無問題。關鍵不是「幾個例外」，而是「有沒有例外逃出正在
//     unwinding 的解構函式或 noexcept 函式」。
//     為什麼會錯：把「兩個例外」這個表象當成規則，而不是背後「無法決定要處理哪一個」
//     的真正原因。
//
// ⚠️ 陷阱 2. 這個 operator= 為什麼連基本保證都沒有？
//     答：delete[] data_; data_ = new int[rhs.n_]; ... 這種寫法有兩個獨立缺陷：
//     ① 例外不安全——new 拋 bad_alloc 時 data_ 已是 dangling pointer、n_ 還是舊值，
//     物件處於破損狀態，後續解構會 double free ② 未處理 self-assignment——w = w 時
//     先 delete[] 再讀 rhs.data_ 就是讀已釋放記憶體。修法一是 copy-and-swap（by-value
//     參數自動解決兩個問題）；修法二是「先配置後提交」：先 new 到臨時指標（此時 *this
//     未動），成功後才 delete 舊的並接上——通用原則是把所有可能拋例外的操作放前面，
//     把不可能失敗的提交放最後。
//     為什麼會錯：習慣「先清乾淨再重建」的直覺順序，但那讓失敗發生在「已經毀掉舊狀態」
//     之後。
// ═══════════════════════════════════════════════════════════════════════════

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <typeinfo>

class SpecialEx : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

static void deepest() {
    throw SpecialEx{"deepest error"};
}

// ❌ 寫法 A：throw e; — 把例外切成 base
static void rethrowBad() {
    try { deepest(); }
    catch (const std::exception& e) {
        std::cout << "  [bad] re-throwing as base...\n";
        throw e;     // 拷貝出 std::exception，遺失型別
    }
}

// ✅ 寫法 B：throw; — 保留原型別
static void rethrowGood() {
    try { deepest(); }
    catch (const std::exception&) {
        std::cout << "  [good] re-throwing original...\n";
        throw;       // 保留 SpecialEx
    }
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：對照兩種 rethrow
    // ─────────────────────────────────────────────────────────
    try { rethrowBad(); }
    catch (const SpecialEx& e) {
        std::cout << "[Demo1-A] still SpecialEx: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cout << "[Demo1-A] only base now: typeid=" << typeid(e).name()
                  << " what=" << e.what() << '\n';
    }

    try { rethrowGood(); }
    catch (const SpecialEx& e) {
        std::cout << "[Demo1-B] preserved SpecialEx: " << e.what() << '\n';
    }
    catch (const std::exception& e) {
        std::cout << "[Demo1-B] only base now: typeid=" << typeid(e).name() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：catch (...) 標準姿勢 — log + rethrow
    // ─────────────────────────────────────────────────────────
    try {
        try {
            throw 3.14;                      // 故意丟個 double（反面教材也展示）
        } catch (...) {
            std::cout << "[Demo2] inner caught unknown — logging then rethrow\n";
            throw;
        }
    } catch (const std::exception& e) {
        std::cout << "[Demo2] outer std::exception: " << e.what() << '\n';
    } catch (...) {
        std::cout << "[Demo2] outer caught non-std exception\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：destructor 內部一定要把例外吃掉 — 否則程式會被 terminate
    //   做法：destructor 內 try/catch 包住所有可能 throw 的清理動作，
    //         失敗就 log，不要讓例外逃出。
    // ─────────────────────────────────────────────────────────
    {
        struct LoggerHandle {
            int id;
            explicit LoggerHandle(int i) : id(i) {
                std::cout << "  LoggerHandle " << id << " open\n";
            }
            ~LoggerHandle() {
                // 假裝 flush 可能失敗 — 但 destructor 必須吃掉例外
                try {
                    if (id < 0) throw std::runtime_error{"flush failed"};
                    std::cout << "  LoggerHandle " << id << " closed cleanly\n";
                } catch (const std::exception& e) {
                    // 千萬不要讓例外逃出 destructor
                    std::cout << "  LoggerHandle " << id
                              << " dtor swallowed: " << e.what() << '\n';
                }
            }
        };
        LoggerHandle a{1};       // 正常關
        LoggerHandle b{-1};      // 假裝 flush 失敗，但 dtor 不會 throw
        std::cout << "[dtor-safe] main flow ok\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：catch(...) 的「合理用法」 — 邊界 / log + rethrow
    //   thread 入口、API 邊界這類「外部不認識自家例外」的地方適合用 catch(...)
    //   先 log、再用 throw; 重新丟給 framework 或上層。
    // ─────────────────────────────────────────────────────────
    {
        auto boundary = [](auto fn) {
            try {
                fn();
            } catch (const std::exception& e) {
                std::cout << "[boundary] std::exception: " << e.what() << '\n';
                // 在真實系統會 rethrow 給 framework：throw;
            } catch (...) {
                std::cout << "[boundary] unknown exception (would rethrow)\n";
                // throw;  ← 真實情境保留型別資訊
            }
        };
        boundary([] { throw std::runtime_error{"work failed"}; });
        boundary([] { throw 42; });    // 非 std 型別
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：怎麼寫 destructor 才不丟例外？
    //    A：
    //      (a) 不要 throw — 用 if/return 或 log 處理錯誤
    //      (b) 真的可能失敗的清理動作，提供主動函式 (close/commit)
    //      (c) 盡量在 destructor 內 catch 自家可能 throw 的東西
    //
    //  Q2：noexcept 加了結果發現裡面有可能 throw，怎麼辦？
    //    A：拆兩層 — 把可能 throw 的部分挪到 helper 函式（不加 noexcept），
    //       對外保持 noexcept；或刪掉 noexcept 標註，誠實標示。
    //
    //  Q3：error code 跟 exception 怎麼選？
    //    A：原則：「真的少見、真的該炸出去」用 exception；「正常會發生、
    //       caller 經常要 handle」用 error code / std::optional / std::expected
    //       (C++23)。例：parse 字串失敗算正常 → 用 optional/expected；
    //       連 DB 失敗算少見 → 用 exception。
    //
    return 0;
}
