// =============================================================================
//  04_catch_by_ref.cpp  —  為什麼 catch 要 by const&
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/language/try_catch     (try/catch 規則)
//    - https://en.cppreference.com/w/cpp/error/exception        (std::exception)
//    - https://en.cppreference.com/w/cpp/language/object#Object_slicing
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三種寫法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      catch (E e)           by value      ❌ 會「切片」（slicing）
//      catch (E& e)          by ref        ⚠️ 沒有 const，可能誤改
//      catch (const E& e)    by const ref  ✅ 推薦寫法
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼 by value 會切片？                                  │
//  └────────────────────────────────────────────────────────────┘
//
//  例：階層 Base ← Derived，throw 一個 Derived 物件
//
//      catch (Base e) { ... }
//
//  進到 catch 時，Derived 物件被「拷貝」進 e — 但 e 的型別是 Base，所以
//  Derived 獨有的成員 / virtual function 行為都「被切掉」了。
//
//  catch (const Base& e) 不切片 — e 真的指向例外物件，virtual 派發到
//  Derived。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 為什麼要 const？                                          │
//  └────────────────────────────────────────────────────────────┘
//
//  例外物件「拷貝權」屬於例外機制，使用者不應該改它。const& 表達這個意
//  圖；同時若你想 rethrow（throw;），物件保持原樣。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：catch by value → 看到的是切片後的 base
//   * Demo 2：catch by const& → 看到完整的 Derived 行為
// =============================================================================

/*
補充筆記：catch_by_ref
  - catch_by_ref 屬於例外處理；例外用來把錯誤從偵測處傳到能處理的地方，而不是取代一般 if 流程。
  - throw by value, catch by const reference 是常見規則，可避免 slicing 並保留多型例外資訊。
  - 解構子和 noexcept 函式不應讓例外逃出；違反 noexcept 會呼叫 std::terminate。
  - RAII 是例外安全基礎，因為 stack unwinding 會自動解構已建立物件。
  - 例外安全常分為 basic guarantee、strong guarantee、nothrow guarantee；修改資料結構時要知道失敗後物件狀態是否仍有效。
  - 不要丟裸指標或字串字面值作為主要錯誤通道；標準例外或自訂 exception 型別較能保存語意。
  - catch by value 會複製並可能 slicing；catch const std::exception& 可保留多型資訊。
  - catch 非 const reference 很少需要，因為錯誤處理通常不應修改例外物件。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】為什麼 catch 要用 const&
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼要 catch by const reference？
//     答：三個理由：① 避免 object slicing——catch by value 時若拋的是衍生類別，會被切成
//     基底類別，丟失衍生資訊與多型行為（throw Der(); 被 catch (Base b) 接到時，b 的
//     虛擬函式呼叫的是 Base 的版本；改成 catch (const Base& b) 才會是 Der 的版本）
//     ② 避免不必要的複製——例外物件的複製本身也可能拋例外 ③ const 表達「不修改例外
//     物件」的意圖（要修改後再 rethrow 才用非 const 參考）。口訣：throw by value,
//     catch by reference。
//     追問：為什麼不 catch by pointer？（誰負責 delete 不清楚，而且接不到非指標的例外）
//
// 🔥 Q2. 多個 catch 的匹配規則是什麼？
//     答：由上而下依序試，第一個可行匹配者勝，不做「最佳匹配」。所以衍生類別的 catch
//     必須寫在基底類別之前，否則永遠不可達；catch (...) 必須放最後。這與函式重載決議
//     的規則完全不同，是常見的區辨考點。
//
// Q3. catch (...) 有什麼用？有什麼問題？
//     答：合理用途是在執行緒或 main 的最外層當「最後防線」，記 log 後 throw; 往上傳或
//     優雅結束，以及配合 std::throw_with_nested 做例外轉譯。問題是拿不到任何例外資訊
//     （除非用 try { throw; } catch (const std::exception& e) 這種「rethrow 再細分」的
//     手法，或用 std::current_exception() 存起來），而且 catch (...) {} 靜默吞掉例外
//     是嚴重反模式——它會把 bug 藏起來。
// ═══════════════════════════════════════════════════════════════════════════

#include <exception>
#include <iostream>
#include <string>

class BaseEx : public std::exception {
public:
    explicit BaseEx(std::string m) : msg_(std::move(m)) {}
    const char* what() const noexcept override { return msg_.c_str(); }
    virtual std::string identity() const { return "BaseEx"; }
private:
    std::string msg_;
};

class DerivedEx : public BaseEx {
public:
    using BaseEx::BaseEx;
    std::string identity() const override { return "DerivedEx"; }
};

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：catch by value → object slicing
    // ─────────────────────────────────────────────────────────
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcatch-value"
    try {
        throw DerivedEx{"oops 1"};
    } catch (BaseEx e) {                      // 切片！(故意保留作教材)
        std::cout << "[byValue] " << e.what()
                  << " | identity = " << e.identity()
                  << "  (預期 BaseEx，因為被切片)\n";
    }
#pragma GCC diagnostic pop

    // ─────────────────────────────────────────────────────────
    // Demo 2：catch by const& → 不切片，virtual 還在
    // ─────────────────────────────────────────────────────────
    try {
        throw DerivedEx{"oops 2"};
    } catch (const BaseEx& e) {
        std::cout << "[byRef]   " << e.what()
                  << " | identity = " << e.identity()
                  << "  (預期 DerivedEx — virtual 正確派發)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 A：rethrow 寫對 vs 寫錯（呼應 09_pitfalls，但用本檔的階層）
    //   throw;   ✅ 保留原型別
    //   throw e; ❌ 拷貝 e → 丟失派生型別資訊（slicing）
    // ─────────────────────────────────────────────────────────
    {
        auto inner = [] { throw DerivedEx{"important"}; };

        // 寫法 A：throw; — 保留型別
        try {
            try { inner(); }
            catch (const BaseEx& e) {
                std::cout << "[good] log: " << e.what() << "  (rethrowing)\n";
                throw;                            // 保留 DerivedEx 型別
            }
        } catch (const DerivedEx& e) {
            std::cout << "[good] outer sees DerivedEx: " << e.identity() << '\n';
        }

        // 寫法 B：throw e; — 失去派生型別
        try {
            try { inner(); }
            catch (const BaseEx& e) {
                std::cout << "[bad]  log: " << e.what() << "  (throw e;)\n";
                throw e;                          // ❌ 切成 BaseEx 拷貝
            }
        } catch (const DerivedEx&) {
            std::cout << "[bad]  outer DerivedEx? — 不會跑到\n";
        } catch (const BaseEx& e) {
            std::cout << "[bad]  outer only sees BaseEx: "
                      << e.identity() << '\n';
        }
    }

    // ─────────────────────────────────────────────────────────
    // 實用範例 B：用 const& 接住 std::exception 統一印錯誤訊息
    //   工作上最常用的 catch — 一條就接所有標準例外與自訂例外
    //   (前提：自訂例外都繼承 std::exception)
    // ─────────────────────────────────────────────────────────
    {
        auto runTask = [](int kind) {
            if (kind == 1) throw std::runtime_error{"task: runtime"};
            if (kind == 2) throw std::out_of_range{"task: oor"};
            if (kind == 3) throw DerivedEx{"task: app derived"};
        };
        for (int k : {1, 2, 3}) {
            try { runTask(k); }
            catch (const std::exception& e) {
                std::cout << "[uniform] caught: " << e.what() << '\n';
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：標準 std::exception 階層的 what() 是 virtual 嗎？
    //    A：是。catch (const std::exception& e) 後 e.what() 會 dispatch
    //       到實際物件的 what()。如果 catch by value 就會 slice 掉這層。
    //
    //  Q2：catch (const std::exception& e) 之後在裡面 throw e; 跟 throw;
    //      差在哪？
    //    A：throw e; 會「拷貝 e 出去」 — 失去原本的派生型別（slicing）。
    //       throw; （不寫物件）才是 rethrow，保留原物件。所以 rethrow 永
    //       遠寫 throw;，不要寫 throw e;。
    //
    //  Q3：可不可以對 e 做修改？
    //    A：去掉 const 雖然能改，但邏輯上很怪 — 你正在處理一個錯誤，改它
    //       的訊息容易混亂下游。要記額外資訊，用 std::throw_with_nested
    //       (見 08 號檔)。
    //
    return 0;
}
