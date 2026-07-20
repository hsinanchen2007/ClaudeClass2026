// =============================================================================
//  02_standard_exceptions.cpp  —  std::exception 階層
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/error/exception
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ std::exception 階層                                        │
//  └────────────────────────────────────────────────────────────┘
//
//      std::exception                     ← 所有標準例外的根
//      ├─ std::logic_error                ← 「程式邏輯」錯：可在 release 前抓到
//      │   ├─ std::invalid_argument       傳入不合理參數
//      │   ├─ std::domain_error           數學上不在 domain
//      │   ├─ std::length_error           容器要超過 max_size
//      │   └─ std::out_of_range           索引超過範圍 (vector::at, map::at)
//      ├─ std::runtime_error              ← 「執行期才能發現」的錯
//      │   ├─ std::range_error            計算結果超過可表達範圍
//      │   ├─ std::overflow_error         算術溢位
//      │   ├─ std::underflow_error        算術下溢
//      │   ├─ std::system_error           系統 / errno 相關
//      │   └─ std::regex_error            (regex 失敗)
//      ├─ std::bad_alloc                  new 失敗
//      ├─ std::bad_cast                   dynamic_cast<&> 失敗
//      ├─ std::bad_typeid                 typeid 對 null pointer
//      ├─ std::bad_weak_ptr               shared_ptr ctor from expired weak_ptr
//      ├─ std::bad_function_call          空 std::function 被呼叫
//      └─ std::ios_base::failure          IO 啟用 exceptions 時拋出
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 兩條主幹：logic_error vs runtime_error                     │
//  └────────────────────────────────────────────────────────────┘
//
//  logic_error   = 程式碼裡的 bug，理論上應該在 review / test 抓到
//                  ex: 傳入空字串呼叫 to_int、索引超界
//  runtime_error = 跟「外部世界」有關，無法在編譯期保證
//                  ex: 檔案不存在、網路失敗、解析使用者輸入失敗
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 共通介面                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      virtual const char* what() const noexcept;
//
//  所有派生類別都必須實作（標準提供的版本通常用一個 std::string 存訊息）。
//  catch 之後直接 e.what() 就能拿到人讀得懂的字串。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//  示範一個函式可能丟「兩種完全不同類別」的例外，呼叫端用最寬的
//  std::exception 接 — 然後依需要分流。
// =============================================================================

/*
補充筆記：standard_exceptions
  - standard_exceptions 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - 標準例外都繼承自 std::exception，what() 提供診斷字串。
  - out_of_range、invalid_argument、runtime_error 等型別名稱應反映錯誤性質，方便呼叫端分類處理。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::exception 階層
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 標準例外階層長什麼樣？logic_error 與 runtime_error 怎麼選？
//     答：根是 std::exception（有虛擬的 what()），主要兩支：std::logic_error
//     （invalid_argument、domain_error、length_error、out_of_range）代表程式邏輯錯誤、
//     理論上可事先避免，也就是「呼叫者的 bug」（前置條件被違反）；std::runtime_error
//     （range_error、overflow_error、underflow_error、system_error）代表執行期才知道的
//     環境問題（檔案不存在、網路斷線）。另外還有 bad_alloc、bad_cast、bad_typeid、
//     bad_function_call、bad_optional_access、bad_variant_access。
//     追問：自訂例外要繼承誰？（通常繼承 std::runtime_error，它已幫你管理 what() 的
//     字串儲存；而且自訂例外的複製建構函式不可拋，否則傳播過程可能直接 terminate）
//
// 🔥 Q2. std::bad_alloc 該怎麼處理？
//     答：實務上多數程式「不要 catch 它」，讓它終止程式。理由：① 在 Linux 的 overcommit
//     機制下，new 常常「成功」但真正碰到記憶體時被 OOM killer 殺掉，bad_alloc 根本不會
//     被拋出 ② 已經記憶體不足時，catch 區塊裡的任何配置（包括組 log 字串）都可能再次
//     失敗 ③ 對多數應用而言記憶體耗盡是不可恢復的。只有長時間執行、且能明確釋放大塊
//     快取的伺服器才值得處理。不想拋例外可用 new (std::nothrow) T，失敗時回 nullptr。
//
// ⚠️ 陷阱. 為什麼衍生類別的 catch 必須寫在基底類別之前？
//     答：因為 catch 子句是「由上而下，第一個可行匹配者勝」，不像函式重載那樣找最佳
//     匹配。基底類別的 catch 接得住衍生類別的例外，所以把 catch (const std::exception&)
//     寫在 catch (const std::out_of_range&) 前面，後者就永遠不可達（多數編譯器會警告
//     handler is unreachable）。同理 catch (...) 必須放在最後。
//     為什麼會錯：把 catch 的匹配想成重載決議那種「挑最貼近的」，實際上它是依序試。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// 假設這是個解析年齡的函式
static int parseAge(const std::string& s) {
    if (s.empty()) {
        // 空字串明顯是 caller 的 bug
        throw std::invalid_argument{"parseAge: empty string"};
    }
    int age = 0;
    try {
        age = std::stoi(s);                  // 可能 throw invalid_argument / out_of_range
    } catch (const std::out_of_range&) {
        throw std::out_of_range{"parseAge: number out of int range: " + s};
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument{"parseAge: not a number: " + s};
    }
    if (age < 0 || age > 200) {
        throw std::out_of_range{"parseAge: unrealistic age: " + std::to_string(age)};
    }
    return age;
}

int main() {
    std::vector<std::string> inputs{"42", "abc", "9999999999999", "-5", ""};

    for (auto& s : inputs) {
        try {
            int age = parseAge(s);
            std::cout << "[ok]   parseAge(\"" << s << "\") = " << age << '\n';
        }
        catch (const std::out_of_range& e) {
            std::cout << "[oor]  " << e.what() << '\n';
        }
        catch (const std::invalid_argument& e) {
            std::cout << "[arg]  " << e.what() << '\n';
        }
        catch (const std::exception& e) {        // 最寬的接住其他
            std::cout << "[std]  " << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：vector::at 與 vector::operator[] 行為差異
    //   .at(i) 越界 → 丟 std::out_of_range（可以 catch）
    //   .operator[](i) 越界 → 未定義行為（UB，不會 throw）
    //   生產程式碼若希望「真的查邊界」，請用 at；熱迴圈內信得過用 [].
    // ─────────────────────────────────────────────────────────
    {
        std::vector<int> v{10, 20, 30};
        try {
            std::cout << "[at] v.at(5) = " << v.at(5) << '\n';   // throws
        } catch (const std::out_of_range& e) {
            std::cout << "[at] caught out_of_range: " << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：std::bad_alloc — 模擬 new 失敗
    //   工作上若處理巨大 vector / 圖片 buffer，要意識到分配可能失敗。
    //   注意：申請過大會在某些平台直接 throw bad_alloc；用 try/catch 包就能
    //   把「out of memory」轉成業務錯誤、給使用者友善的訊息。
    // ─────────────────────────────────────────────────────────
    {
        try {
            // 故意申請一個不太可能配到的巨大 vector
            std::vector<char> huge(static_cast<size_t>(-1) / 2);
            std::cout << "[bad_alloc] unexpectedly allocated " << huge.size() << '\n';
        } catch (const std::bad_alloc& e) {
            std::cout << "[bad_alloc] caught: " << e.what() << '\n';
        } catch (const std::exception& e) {
            std::cout << "[bad_alloc] caught std exception: " << e.what() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：catch 的順序重要嗎？
    //    A：很重要。從上到下找第一個匹配的 handler；如果先寫 catch
    //       (const std::exception& e) 就會吞掉所有派生類，後面 catch
    //       (const std::out_of_range&) 永遠不會執行。原則：先具體、後泛用。
    //
    //  Q2：要不要自訂 exception class？
    //    A：可以但不必到處做。原則：標準類已能表達就用標準（runtime_error 等）；
    //       要區別「自家專屬錯誤領域」時才繼承 std::runtime_error / std::exception。
    //
    //  Q3：what() 為什麼是 const char*？
    //    A：歷史相容、且不會在 catch 中觸發另一次 allocation。
    //       訊息存在派生類的 std::string 成員裡，what() 回傳 c_str()。
    //
    return 0;
}
