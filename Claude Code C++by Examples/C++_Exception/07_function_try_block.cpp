// =============================================================================
//  07_function_try_block.cpp  —  建構子的 function-try-block
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/function-try-block
//    - https://en.cppreference.com/w/cpp/language/constructor   (建構子初始化序列)
//    - https://en.cppreference.com/w/cpp/language/throw
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、什麼是 function-try-block？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  普通 try block 包在函式內部：
//
//      void f() {
//          try { ... } catch (...) { ... }
//      }
//
//  function-try-block 是「整個函式都在 try 裡」、且 catch 區塊在函式體外：
//
//      void f() try {
//          /* function body */
//      } catch (const std::exception& e) {
//          /* handler */
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、唯一真的有用的場合：建構子的 member-init                │
//  └────────────────────────────────────────────────────────────┘
//
//  建構子的 initializer list（: x_(...), y_(...)）發生 throw，普通 try block
//  抓不到（因為整個函式體還沒開始）。function-try-block 可以：
//
//      class Foo {
//      public:
//          Foo() try : x_(makeX()), y_(makeY()) {
//              // 函式體
//          } catch (const std::exception& e) {
//              // x_ / y_ 哪個失敗都會跳到這裡
//              // 注意：這個 catch 結束後例外會「自動 rethrow」 —
//              //       建構子失敗的物件不存在，例外必須繼續傳出去
//          }
//
//      private:
//          X x_;
//          Y y_;
//      };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、限制                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  * 建構子的 function-try catch block 結束時例外「強制再傳出去」 — 你不
//    能「攔截掉、讓建構子裝沒事」。寫 return; 也沒用。
//  * destructor 的 function-try-block 一樣：catch 後會自動 rethrow。
//  * 一般成員函式 / 自由函式可以「真的攔下」例外（catch 後若不 rethrow
//    就回到普通流程），但對建構子 / destructor 是強制 rethrow。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：member init throw → function-try catch 印 log → 自動 rethrow
//   * Demo 2：對照普通建構子（無 function-try）— 一樣會傳出去，但無 log 機會
// =============================================================================

/*
補充筆記：function_try_block
  - function_try_block 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - function try block 可捕捉建構子初始化列表中拋出的例外。
  - 建構子 function try block catch 後若不重新拋出，建構仍被視為失敗並會再拋出。
*/
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

// 一個會在建構失敗的型別
struct Fragile {
    explicit Fragile(int v) {
        if (v < 0) throw std::invalid_argument{"Fragile: v < 0"};
        std::cout << "  Fragile(" << v << ") ok\n";
    }
};

// 帶 function-try-block 的建構子
class Holder {
public:
    Holder(int a, int b) try : x_(a), y_(b) {
        std::cout << "  Holder body ran\n";   // 只有都成功才會跑到
    } catch (const std::exception& e) {
        std::cout << "  Holder ctor caught: " << e.what()
                  << "  (will auto-rethrow)\n";
        // 不可以 swallow — 這裡結束後例外會自動 rethrow
    }

private:
    Fragile x_;
    Fragile y_;
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：第二個 init 失敗 → function-try catch 紀錄 → 外層接到
    // ─────────────────────────────────────────────────────────
    try {
        std::cout << "[Demo1] try Holder(1, -1)\n";
        Holder h{1, -1};
        std::cout << "[Demo1] not reached\n";
    } catch (const std::exception& e) {
        std::cout << "[Demo1] outer caught: " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：兩個 init 都成功
    // ─────────────────────────────────────────────────────────
    {
        std::cout << "[Demo2] Holder(2, 3)\n";
        Holder h{2, 3};
        (void)h;
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：在 function-try 內 log + 補上 context 訊息再 rethrow
    //   重點：建構失敗時即使無法 swallow 例外，仍可在 catch 區塊內紀錄
    //   「我們正試圖建構什麼物件」，幫忙下游診斷。
    // ─────────────────────────────────────────────────────────
    {
        struct ConfigLoader {
            Fragile cfg;
            std::string source;
            ConfigLoader(int v, std::string src) try
                : cfg(v), source(std::move(src))
            {
                std::cout << "  ConfigLoader: ok from " << source << '\n';
            } catch (const std::exception& e) {
                // 此時 source 可能也沒建構成功，所以盡量只用「caller 已知資訊」
                std::cout << "  ConfigLoader: failed (v=" << v
                          << "), original: " << e.what()
                          << "  (auto-rethrow)\n";
                // 不要寫 throw std::runtime_error{...} — 會壓掉原型別資訊
                // 直接讓 function-try 把原例外 auto-rethrow 出去
            }
        };

        try {
            ConfigLoader bad{-5, "user_input"};
        } catch (const std::exception& e) {
            std::cout << "[ConfigLoader] outer caught: " << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：一般成員函式的 function-try-block — 「真的」可以 swallow
    //   建構/解構是強制 rethrow，一般成員函式則自由：不 throw 就視同處理完。
    // ─────────────────────────────────────────────────────────
    {
        struct Cache {
            int hits = 0;
            // 一般成員函式的 function-try-block：可以選擇 swallow + 給預設值
            int safeLookup(int key) try {
                if (key < 0) throw std::out_of_range{"negative key"};
                ++hits;
                return key * 10;
            } catch (const std::exception& e) {
                std::cout << "  Cache.safeLookup swallow: " << e.what() << '\n';
                return -1;                    // 給預設值，不 rethrow
            }
        };
        Cache c;
        std::cout << "[Cache] " << c.safeLookup(5)  << '\n'; // 50
        std::cout << "[Cache] " << c.safeLookup(-1) << '\n'; // -1
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：第一個 member 失敗，第二個 member 怎麼辦？
    //    A：例外從 x_(...) 拋出時，y_ 還沒構造 → 不會析構（因為從沒存在）。
    //       只有「已成功構造」的成員會被逆序析構（C++ 標準保證）。
    //
    //  Q2：destructor 也能 function-try-block 嗎？
    //    A：可以，但同樣會自動 rethrow，且通常是壞主意（destructor 應 noexcept）。
    //       少數情況用來「log 後 swallow」 — 但別忘記 destructor 預設
    //       noexcept，throw 直接 terminate。要寫請加 noexcept(false)。
    //
    //  Q3：實務中常用嗎？
    //    A：不常。多數情況用「先把資源用 RAII 包好，再交給 member」就夠：
    //         class Holder {
    //             std::unique_ptr<Fragile> x_, y_;
    //             Holder() : x_(std::make_unique<Fragile>(...)),
    //                        y_(std::make_unique<Fragile>(...)) {}
    //         };
    //       function-try-block 是「需要 log / cleanup」的少數場合的解方。
    //
    return 0;
}
