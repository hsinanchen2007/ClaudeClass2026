// ============================================================================
// C++ 例外處理總複習：錯誤模型、RAII、保證層級與邊界設計
// ============================================================================
//
// 【本章地圖：對應 01～09】
//   basics / standard exceptions / what to throw / catch by reference / noexcept /
//   RAII safety / function-try-block / nested exception / pitfalls
//
// 【先選錯誤表示，不要看見失敗就 throw】
//   狀況                                  建議表示
//   查不到、queue 空、正常分支             optional / iterator / bool / expected
//   呼叫者違反可立即檢查的前置條件         invalid_argument / out_of_range
//   函式無法履行承諾，需跨多層找 handler    throw typed exception by value
//   系統錯誤且需 error_code 細節            std::system_error 或 error_code overload
//   destructor/rollback                    不可讓 exception 逃出；記錄或保存狀態
//   thread/C ABI/main/event-loop 邊界        catch all，轉 exit code/status/log
//
// 【標準例外族譜速查】
//   std::exception
//   ├─ logic_error：程式契約/輸入邏輯（invalid_argument, domain_error,
//   │                length_error, out_of_range）
//   ├─ runtime_error：執行環境失敗（range_error, overflow_error,
//   │                  underflow_error；system_error 另帶 error_code）
//   ├─ bad_alloc / bad_cast / bad_optional_access / bad_variant_access
//   └─ 自訂型別應繼承最貼近的標準 base，並保存機器可讀欄位，不只 what() 字串。
//
// 【throw / catch 規則】
//   - `throw Error(args);`：by value；runtime 會建立 exception object。
//   - `catch (const Error& e)`：by const reference，避免 slicing 與不必要 copy。
//   - handler 由最具體排到 `std::exception`，最後才 `catch (...)`。
//   - 原樣重拋用 `throw;`；`throw e;` 會建立新物件、可能 slicing 且重設來源位置。
//   - exception object 活到最後一個 active handler 結束；若 exception_ptr 仍持有它，
//     lifetime 會延長到最後一份相關 exception_ptr 釋放。不要保存 handler 內的裸 reference。
//   - constructor throw 時，已完成建構的 members/bases 會析構；物件本身 destructor 不跑。
//
// 【四種 exception safety guarantee】
//   no-throw：承諾不失敗；destructor、swap、move 常希望達到。
//   strong：失敗後 observable state 不變；copy-and-swap / prepare-then-commit。
//   basic：失敗後仍合法、無 leak，但內容可能已改變。
//   none：連 invariant 都可能破壞；production API 應避免或清楚隔離。
//
// 【RAII 與 stack unwinding】
//   每個資源都由 object lifetime 管理：unique_ptr、vector、fstream、lock_guard 或自訂 handle。
//   unwinding 會反向析構已完成建構的 local objects，因此 cleanup 不應散在 catch 中。
//   destructor 預設隱含 noexcept；若它丟例外且同時正在 unwinding，std::terminate。
//
// 【noexcept 必懂】
//   - `noexcept` 是契約，不是「忽略例外」；違反就 terminate。
//   - conditional noexcept：`noexcept(noexcept(expr))` 可把底層保證傳上來。
//   - vector reallocation 常在 move 可能 throw 且 type 可 copy 時改用 copy，維持 strong
//     guarantee；因此 move constructor 適合在真的不會失敗時標 noexcept。
//   - `noexcept(f())` operator 不會呼叫 f，只在 compile time 查詢宣告。
//
// 【function-try-block 與 nested exception】
//   constructor function-try-block 可攔 base/member initializer 的例外；catch 結束後仍會
//   自動重拋，因為物件沒有成功建立，不能假裝成功。一般 function 通常用普通 try 即可。
//   `throw_with_nested` 在每一層加 context；診斷時用 rethrow_if_nested 遞迴保留 root cause。
//
// 【效能與邊界】
//   主流 ABI 在不 throw 的 hot path 常接近 zero-cost，但 throw/unwind 很昂貴；不要用例外
//   表達高頻正常分支。複雜度取決於 unwind frame/destructor 數，而非固定 O(1)。
//
// 【面試快問快答】
//   Q: 例外能抓 segmentation fault/UB 嗎？ A: 標準 C++ 例外不能把 UB 變安全。
//   Q: catch(...) 後可直接忽略嗎？ A: 邊界可 catch，但必須記錄/轉狀態；沉默成功最危險。
//   Q: 為何 constructor 不回錯誤碼？ A: 建構完成應代表 invariant 成立；否則 throw 或 factory。
//   Q: 何謂 strong guarantee？ A: 失敗看起來像操作從未發生，不等於「不會失敗」。
// ============================================================================

/*
==============================================================================
【面試深挖：Exceptions 與 Error Handling】

E1｜C++ 沒有 finally，如何保證清理？
答：RAII。把資源放進 destructor 不拋的 object，正常 return 與 stack unwinding 都會清理。
scope guard 可處理不是單一資源的 rollback/commit action。

E2｜為何 catch 通常寫 `const std::exception&`？
答：避免 copy/slicing，保留 dynamic what()/type；按 value catch base 會切掉 derived。
若要修改 exception 才去掉 const，而且例外型別通常設計成 immutable diagnostic。

E3｜`throw;` 與 `throw e;` 的差別？
答：throw; rethrow 目前例外並保留 dynamic type；throw e 建立新例外，若 e 是 base reference
可能 slicing，stack trace/context 也不同。只能在 active handler 中使用裸 throw。

E4｜constructor 拋例外時 destructor 會怎樣？
答：最終 object 未完成，因此其 destructor 不呼叫；已完成的 bases/members 依反序自動析構。
constructor body 中手動取得的 raw resource 若未包 RAII 會洩漏。

E5｜destructor 可以拋嗎？
答：語言可宣告 noexcept(false)，但工程上幾乎不應讓例外逃出。若 stack unwinding 期間
destructor 又丟出未處理例外會 terminate；預設 exception specification 也常使違反直接 terminate。

E6｜basic、strong、nothrow guarantee？
答：basic：失敗後 invariant 成立且不洩漏；strong：操作像 transaction，失敗無可觀察改變；
nothrow：承諾不拋。不是每個 API 都該強保證，要依成本與可 commit 架構選。

E7｜`noexcept` 的價值與風險？
答：成為介面契約並允許最佳化/容器選 move；若例外逃出會 terminate。只有真正能保證時標，
或在 boundary 內 catch/轉換。noexcept(expression) 可依 member operation 條件化。

E8｜例外是否「零成本」？
答：常見 table-based ABI 讓未拋的正常路徑接近零額外指令，但 binary metadata、code size、
throw/unwind 成本仍高；某些 target 可能用其他模型。zero-cost 不是「throw 免費」。

E9｜跨 thread 如何傳遞例外？
答：例外不會自動跨 thread；thread function 未捕獲會 terminate。用 promise/future 或
`exception_ptr=current_exception()` 保存，再由接收 thread `rethrow_exception`。

E10｜`nested_exception` 解決什麼？
答：高層轉成 domain error 時保留低層 cause chain，而不是只拼 what 字串。
`throw_with_nested` 與 recursive rethrow 可輸出完整 context。

E11｜function-try-block 的主要用途？
答：constructor 可捕捉 initializer list/base/member construction 的例外以記錄/轉換；
但 handler 結束通常仍須拋，因 object 沒建好。不能在 handler 安全使用未建成 members。

E12｜exceptions 與 error code 怎麼選？
答：exception 適合無法就地處理、低頻失敗並配 RAII；expected/error code 適合預期性、
高頻、跨 ABI 或禁例外環境。不要在同一 API 隨機混用兩種 contract。

E13｜例外可穿越 C ABI、plugin 或不同 runtime 嗎？
答：不應假設。跨語言/C boundary 必須 catch all 並轉成穩定 error representation；
不同 compiler/runtime/flags 的 exception ABI 也可能不相容。

E14｜只捕 `...` 然後忽略有何問題？
答：吞掉 failure 會讓程式以破壞的 state 繼續。catch-all 應位於明確 boundary，
記錄 context、清理後轉換或終止；不可當作「提高穩定性」的萬用膠帶。

E15｜舊式 `throw(T1,T2)`／`throw()` exception specification 發生了什麼？
答：dynamic exception specification 在 C++11 deprecated、C++17 移除；它曾以 runtime 檢查與
`unexpected` 處理違規。`throw()` 自 C++17 被視為 non-throwing；現代程式用可由型別系統查詢的
`noexcept`/`noexcept(expr)`，不能把舊教材規則不分版本直接套用。

E16｜「zero-cost exception」與 SjLj/table-based unwinding 差在哪？
答：這是 ABI/實作策略，不是 C++ 標準保證。常見 table-based 模型讓正常路徑幾乎不執行額外
檢查，但增加 metadata/code size，throw 時查表與 unwind 很昂貴；SjLj 類模型維護跳轉狀態，正常
路徑也可能有成本。精確說法是「某些 ABI 不丟時成本很低」，而非 exception 真正零成本。
==============================================================================
*/

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class ConfigError : public std::runtime_error {
public:
    ConfigError(std::string key, const std::string& reason)
        : std::runtime_error("config '" + key + "': " + reason), key_(std::move(key)) {}

    [[nodiscard]] const std::string& key() const noexcept { return key_; }

private:
    std::string key_;
};

int parse_positive(const std::string& key, const std::string& text)
{
    try {
        std::size_t consumed = 0U;
        const int value = std::stoi(text, &consumed);
        if (consumed != text.size() || value <= 0) {
            throw ConfigError(key, "must be a positive integer");
        }
        return value;
    } catch (const std::invalid_argument&) {
        throw ConfigError(key, "not an integer");
    } catch (const std::out_of_range&) {
        throw ConfigError(key, "integer is outside int range");
    }
}

void basic_exception_demo()
{
    assert(parse_positive("threads", "8") == 8);
    try {
        static_cast<void>(parse_positive("threads", "bad"));
        assert(false);
    } catch (const ConfigError& error) {
        assert(error.key() == "threads");
        assert(std::string{error.what()}.find("threads") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------
// LeetCode 150：Evaluate Reverse Polish Notation
// LeetCode 保證輸入合法；可重用 library 仍把 malformed input 定義為 typed exception。
// 每個 token push/pop 一次：O(n) time / O(n) stack space。
// ---------------------------------------------------------------------------
int checked_rpn_operation(int left, int right, char operation)
{
    if (operation == '/') {
        if (right == 0) {
            throw std::domain_error("division by zero");
        }
        if (left == std::numeric_limits<int>::min() && right == -1) {
            throw std::overflow_error("division result does not fit int");
        }
        return left / right;
    }

    // int*int、int+int 與 int-int 的精確結果都放得進 long long；先升格才運算，
    // 避免「算完才檢查」已經先觸發 signed-overflow UB。
    long long result = 0;
    if (operation == '+') result = static_cast<long long>(left) + right;
    else if (operation == '-') result = static_cast<long long>(left) - right;
    else result = static_cast<long long>(left) * right;
    if (result < std::numeric_limits<int>::min() ||
        result > std::numeric_limits<int>::max()) {
        throw std::overflow_error("RPN result does not fit int");
    }
    return static_cast<int>(result);
}

int evaluate_rpn(const std::vector<std::string>& tokens)
{
    std::vector<int> stack;
    for (const std::string& token : tokens) {
        const bool is_operator = token.size() == 1U
            && (token == "+" || token == "-" || token == "*" || token == "/");
        if (!is_operator) {
            std::size_t consumed = 0U;
            const int value = std::stoi(token, &consumed);
            if (consumed != token.size()) {
                throw std::invalid_argument("malformed operand");
            }
            stack.push_back(value);
            continue;
        }
        if (stack.size() < 2U) {
            throw std::invalid_argument("missing operand");
        }
        const int right = stack.back();
        stack.pop_back();
        const int left = stack.back();
        stack.pop_back();
        stack.push_back(checked_rpn_operation(left, right, token.front()));
    }
    if (stack.size() != 1U) {
        throw std::invalid_argument("malformed expression");
    }
    return stack.back();
}

void leetcode_demo()
{
    assert(evaluate_rpn({"2", "1", "+", "3", "*"}) == 9);
    assert(evaluate_rpn({"4", "13", "5", "/", "+"}) == 6);
    try {
        static_cast<void>(evaluate_rpn({"1", "0", "/"}));
        assert(false);
    } catch (const std::domain_error&) {
        // 預期路徑。
    }
    for (const std::vector<std::string>& expression : {
             std::vector<std::string>{"2147483647", "1", "+"},
             std::vector<std::string>{"-2147483648", "-1", "/"}}) {
        bool overflow_rejected = false;
        try {
            static_cast<void>(evaluate_rpn(expression));
        } catch (const std::overflow_error&) {
            overflow_rejected = true;
        }
        assert(overflow_rejected);
    }
}

// ---------------------------------------------------------------------------
// 實務：prepare-then-commit 提供 strong guarantee。
// 先在 local candidate 完成所有可能失敗的 parse；全部成功才一次 swap 進正式 state。
// ---------------------------------------------------------------------------
struct ServiceConfig {
    int workers{1};
    int timeout_ms{1'000};
};

class Service {
public:
    void reload(const std::string& workers, const std::string& timeout)
    {
        ServiceConfig candidate;
        candidate.workers = parse_positive("workers", workers);
        candidate.timeout_ms = parse_positive("timeout_ms", timeout);
        config_ = candidate; // trivial members，commit 不丟例外
    }

    [[nodiscard]] const ServiceConfig& config() const noexcept { return config_; }

private:
    ServiceConfig config_;
};

int command_boundary(Service& service,
                     const std::string& workers,
                     const std::string& timeout) noexcept
{
    try {
        service.reload(workers, timeout);
        return 0;
    } catch (const ConfigError&) {
        return 2;
    } catch (...) {
        return 3;
    }
}

void practical_demo()
{
    Service service;
    assert(command_boundary(service, "8", "250") == 0);
    assert(service.config().workers == 8 && service.config().timeout_ms == 250);

    assert(command_boundary(service, "16", "bad") == 2);
    // 第二欄失敗，第一欄不可半套更新：strong guarantee。
    assert(service.config().workers == 8 && service.config().timeout_ms == 250);
}

int main()
{
    basic_exception_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Exception summary: all assertions passed\n";
}
