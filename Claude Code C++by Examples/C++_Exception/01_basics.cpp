// =============================================================================
//  01_basics.cpp  —  try / throw / catch 基礎
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/language/exceptions
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼需要例外機制？                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  傳統 C 用「回傳錯誤碼」表示失敗：
//
//      int parse(const char* s, int* out);    // 0 成功，非 0 = 錯誤
//
//  問題：
//   * 每個函式都要在呼叫端寫一遍 if (rc != 0) return rc; — 樣板碼到處都是
//   * 建構子沒有 return value，無法表達「失敗」
//   * 跨多層呼叫鏈傳遞錯誤，要層層手寫向上回報
//   * 容易忽略檢查（rc 沒收就忘了）
//
//  C++ 例外機制：在「異常情況」直接 throw，呼叫鏈會自動 unwinding（解開棧
//  幀，析構自動變數），直到被某層 catch 接住。
//
//      try {
//          doWork();              // 任何一層 throw 都會被下面接到
//      } catch (const std::exception& e) {
//          std::cerr << "fail: " << e.what() << '\n';
//      }
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、流程拆解                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   1. throw E{...}      建立一個 E 型別的「例外物件」(在 implementation
//                        提供的 exception storage)
//   2. stack unwinding   從 throw 點往上回到最近的 try block，沿途的「自動
//                        變數」按照建構順序逆序析構（destructor 自動跑）
//   3. catch 配對        從上到下 (依 source order) 找第一個能接住的 handler
//                        匹配規則：完全相同 / public 基底 / 隱式轉換少數情形
//   4. 沒人接            std::terminate() → 程式終止（預設 abort）
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：basic try/throw/catch
//   * Demo 2：throw 的物件是值還是 ref？— 「物件被 copy 進例外儲存」
//   * Demo 3：rethrow（保留型別資訊向外傳）
// =============================================================================

/*
補充筆記：basics
  - basics 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - throw 會中斷目前控制流程，沿呼叫堆疊尋找第一個相容 catch。
  - catch 順序要由具體到一般，否則 base exception 會先攔住 derived exception。
*/
#include <iostream>
#include <stdexcept>
#include <string>

static int divide(int a, int b) {
    if (b == 0) {
        throw std::runtime_error{"divide by zero"};   // 拋出例外
    }
    return a / b;
}

// 中層函式 — 接住、紀錄、再 rethrow（保留原 type）
static int safeDivide(int a, int b) {
    try {
        return divide(a, b);
    } catch (const std::exception& e) {
        std::cerr << "[safeDivide] note: " << e.what() << '\n';
        throw;            // ← 不寫 throw e，要寫 throw; 才能保留原型別
    }
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：基本 try/catch
    // ─────────────────────────────────────────────────────────
    try {
        std::cout << "[Demo1] 6/3 = " << divide(6, 3) << '\n';
        std::cout << "[Demo1] 6/0 = " << divide(6, 0) << '\n';   // throw
        std::cout << "[Demo1] not reached\n";
    } catch (const std::runtime_error& e) {
        std::cout << "[Demo1] caught runtime_error: " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：throw 的物件被「拷貝」進例外儲存
    //   原本 local 變數 unwinding 時就死了，例外物件由 runtime 持有
    // ─────────────────────────────────────────────────────────
    try {
        std::string msg = "local message";
        throw std::runtime_error{msg};        // runtime_error 內部 copy msg
    } catch (const std::runtime_error& e) {
        std::cout << "[Demo2] " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：rethrow 保留型別
    //   safeDivide 內 catch 後 throw; → 外層仍能用 runtime_error 接到
    // ─────────────────────────────────────────────────────────
    try {
        safeDivide(5, 0);
    } catch (const std::runtime_error& e) {
        std::cout << "[Demo3] outer caught: " << e.what() << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：parse 字串成整數的兩種風格
    //   工作上常見對照：
    //     (1) try/catch 風格 — 用 std::stoi 失敗時 throw
    //     (2) error code 風格 — 用 std::from_chars (C++17) 回傳 result
    //   兩者各有適用場景；當「非法輸入很少見」用例外較自然，
    //   「非法輸入是預期的一部分」用 error code 較不擾亂主流程。
    // ─────────────────────────────────────────────────────────
    {
        auto parseStrict = [](const std::string& s) {
            try {
                return std::stoi(s);
            } catch (const std::invalid_argument&) {
                throw std::runtime_error{"parse failed: " + s};
            } catch (const std::out_of_range&) {
                throw std::runtime_error{"out of range: " + s};
            }
        };
        for (const std::string& in : {std::string{"42"}, std::string{"x"}, std::string{"99"}}) {
            try {
                std::cout << "[parse] " << in << " -> " << parseStrict(in) << '\n';
            } catch (const std::exception& e) {
                std::cout << "[parse] " << e.what() << '\n';
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：模擬「資源釋放後可能拋例外」的清理流程
    //   工作場景：在重要工作前 reserve 某資源，做完後嘗試釋放；釋放失敗
    //   只能 log 不該影響主流程 → 用 try/catch 把例外侷限在清理片段。
    // ─────────────────────────────────────────────────────────
    {
        auto doWork = []() { /* 真正的工作 */ };
        auto release = [](bool fail) {
            if (fail) throw std::runtime_error{"release: device busy"};
        };

        doWork();
        try {
            release(true);                       // 假裝釋放失敗
        } catch (const std::exception& e) {
            // 在這裡 swallow + log 是合理的，因為「工作已完成」
            std::cout << "[cleanup] warn: " << e.what() << '\n';
        }
        std::cout << "[cleanup] main flow continues\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：throw 跟 return 速度差很多嗎？
    //    A：對「成功路徑」：例外幾乎沒成本（modern compiler 用 zero-cost
    //       exceptions: table-based unwind）。對「失敗路徑」：throw 比 return
    //       錯誤碼慢很多 — 通常是微秒級別（涉及 stack unwind 與分配 ex 物件）。
    //       所以 exception 適合「真的少見」的錯誤，不要當控制流。
    //
    //  Q2：可以 throw 任何型別嗎？
    //    A：語法上可以 (throw 42; 也行)。但慣例強烈建議「只 throw 繼承 std::exception
    //       的型別」 — 才能用 catch (const std::exception& e) 統一接、印 what()。
    //
    //  Q3：catch (...) 是什麼？
    //    A：catch 任何型別。通常作為「最後屏障」 — 例如 main 的最外層、
    //       thread 入口；裡面一般「log 後 abort」或重 throw 給 framework。
    //       不要用 catch (...) 「靜默吞掉」例外。
    //
    return 0;
}
