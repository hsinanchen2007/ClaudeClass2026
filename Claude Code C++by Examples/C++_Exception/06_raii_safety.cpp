// =============================================================================
//  06_raii_safety.cpp  —  RAII 與 stack unwinding：例外安全的核心
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/raii          (RAII 慣例)
//    - https://en.cppreference.com/w/cpp/language/exceptions    (stack unwinding)
//    - https://en.cppreference.com/w/cpp/memory/unique_ptr      (智慧指標 RAII)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼 C++ 例外能「安全」？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  C 的 setjmp / longjmp 跳出函式時不會析構自動變數 — 任何 malloc 的記憶
//  體、開啟的檔案，全部洩漏。
//
//  C++ throw 觸發 stack unwinding：
//
//   * 從 throw 處往上回到最近的 try block
//   * 沿途「自動儲存」(stack 上的) 物件按建構順序逆序析構
//   * destructor 自動釋放它們持有的資源（記憶體、鎖、檔案、socket...）
//
//  搭配 RAII（resource acquired in initialization）的習慣 — 用物件的生命週
//  期管資源 — 例外是安全的：throw 不會洩漏。
//
//      void f() {
//          std::ofstream out{"data.txt"};
//          std::lock_guard<std::mutex> g{mu_};
//          throw std::runtime_error{"oops"};
//          //         ↑ unwinding：g 先析構 (釋放鎖)，out 後析構 (關檔)
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、不要用「裸 new / delete / lock / unlock」              │
//  └────────────────────────────────────────────────────────────┘
//
//  反面教材：
//
//      void f() {
//          int* p = new int[100];           // 1) 配記憶體
//          mu_.lock();                      // 2) 上鎖
//          do_work_that_might_throw();      // 3) 中途 throw → unwinding
//          mu_.unlock();                    // ❌ 永遠不會跑到
//          delete[] p;                      // ❌ 永遠不會跑到 — 洩漏
//      }
//
//  改用：
//   * std::unique_ptr / std::vector 管記憶體
//   * std::lock_guard / std::scoped_lock 管鎖
//   * std::ofstream 管檔案
//   * 自訂 RAII guard 管自家資源
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：用 RAII 物件追蹤析構順序（throw 過去仍會析構）
//   * Demo 2：寫一個簡單的「scope_exit」RAII helper
// =============================================================================

/*
補充筆記：raii_safety
  - raii_safety 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - 例外安全依賴 RAII，因為離開 scope 時已建構物件會自動解構。
  - strong guarantee 通常用先建立新狀態、成功後交換的方式達成。
*/
#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

// ─── RAII tracer：建構與析構都印一行 ───
class Tracer {
public:
    explicit Tracer(std::string name) : name_(std::move(name)) {
        std::cout << "  [+] " << name_ << '\n';
    }
    ~Tracer() {
        std::cout << "  [-] " << name_ << '\n';
    }
    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;

private:
    std::string name_;
};

// ─── 簡單的 scope_exit：析構時跑 callable ───
template <class F>
class ScopeExit {
public:
    explicit ScopeExit(F f) : fn_(std::move(f)) {}
    ~ScopeExit() { fn_(); }
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
private:
    F fn_;
};
template <class F>
ScopeExit<F> make_scope_exit(F f) { return ScopeExit<F>{std::move(f)}; }

static void mayThrow(bool doIt) {
    Tracer t1{"inner-1"};
    Tracer t2{"inner-2"};
    if (doIt) {
        throw std::runtime_error{"oops in mayThrow"};
    }
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：throw 時自動變數仍按 RAII 析構
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo1] before try\n";
    try {
        Tracer outer{"outer"};
        mayThrow(true);
        std::cout << "[Demo1] not reached\n";
    } catch (const std::exception& e) {
        std::cout << "[Demo1] caught: " << e.what() << '\n';
    }
    std::cout << "[Demo1] after try\n";
    // 預期析構順序：inner-2 → inner-1 → outer

    // ─────────────────────────────────────────────────────────
    // Demo 2：scope_exit — 把 cleanup 程式碼綁在 scope 上
    //   使用情境：與不擁有 RAII 包裝的 C API 互動
    // ─────────────────────────────────────────────────────────
    {
        std::cout << "[Demo2] open faux resource\n";
        auto guard = make_scope_exit([] {
            std::cout << "[Demo2] cleanup runs (always)\n";
        });

        try {
            throw std::runtime_error{"mid-scope error"};
        } catch (const std::exception& e) {
            std::cout << "[Demo2] caught: " << e.what() << '\n';
        }
        // 不管是否 throw，guard 都會析構 → cleanup 都會跑
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：用 RAII 包 C API（FILE*）
    //   C API 的 fopen / fclose 是 RAII 化的經典案例。
    //   把資源放進有 destructor 的 class，throw 或正常 return 都自動關檔。
    // ─────────────────────────────────────────────────────────
    {
        // 注意：這裡用 unique_ptr + 自訂 deleter，是「最簡潔的 RAII」寫法
        auto closer = [](std::FILE* fp) { if (fp) std::fclose(fp); };
        std::unique_ptr<std::FILE, decltype(closer)> fp(
            std::fopen("tmp_raii_demo.txt", "w"), closer);

        if (fp) {
            std::fputs("RAII wrote this line\n", fp.get());
            std::cout << "[FILE*] wrote via RAII wrapper\n";
        }
        // fp 析構 → fclose 自動跑；即使中途 throw 也一樣
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：throw 中也能 RAII — 寫個交易式的計數器
    //   commit() 不被呼叫時析構就 rollback；模擬「strong exception guarantee」。
    // ─────────────────────────────────────────────────────────
    {
        struct CounterTx {
            int& target;
            int  saved;
            bool committed = false;
            CounterTx(int& t) : target(t), saved(t) {}
            ~CounterTx() {
                if (!committed) target = saved;   // rollback
            }
            void commit() noexcept { committed = true; }
        };

        int counter = 100;
        try {
            CounterTx tx{counter};
            counter += 50;                        // 試圖累加 50
            throw std::runtime_error{"oops"};      // 中途出錯！
            tx.commit();                          // 不會跑到
        } catch (const std::exception& e) {
            std::cout << "[tx] caught: " << e.what() << '\n';
        }
        std::cout << "[tx] counter rolled back to " << counter << '\n'; // 100

        // 正常路徑
        {
            CounterTx tx{counter};
            counter += 25;
            tx.commit();
        }
        std::cout << "[tx] counter after commit = " << counter << '\n'; // 125
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unwinding 時 destructor 又 throw 會怎樣？
    //    A：在 unwinding 期間第二個 exception 浮上來 → 直接 std::terminate。
    //       規則：destructor 不 throw。需要做可能失敗的事，留 close()/commit()
    //       這類「使用者主動呼叫」的方法在 destructor 之外。
    //
    //  Q2：try/catch 跟 RAII 哪個重要？
    //    A：RAII 比 try/catch 重要 — 多數情況根本不用寫 try/catch，只要保
    //       證所有資源都「被 RAII 物件持有」，let it propagate 即可。
    //       try/catch 只在「這層真的能處理錯誤」的邊界才寫（如 API gateway、
    //       retry framework）。
    //
    //  Q3：scope_exit 跟 unique_ptr 自訂 deleter 哪個好？
    //    A：scope_exit 表達「我有一段 cleanup code」 — 跟資源沒一對一綁定；
    //       unique_ptr<T, Deleter> 表達「有一個資源要管理」。後者更精確、
    //       前者更彈性。C++23 把 scope_exit 標準化進 <experimental/scope>。
    //
    return 0;
}
