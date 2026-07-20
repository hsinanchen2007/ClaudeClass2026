// ============================================================================
// 課題 8：std::nested_exception / throw_with_nested 保留 error chain
// ============================================================================
//
// 每層只看到低階「stoi invalid」不知是哪個檔/欄位；只改成新 runtime_error 又會丟掉
// root cause。`throw_with_nested(ContextError(...))` 將目前 exception 嵌入新 exception，
// boundary 可遞迴 `rethrow_if_nested` 印出完整因果鏈。
//
// Context 應增加新資訊，不重複空話；避免在每層 catch/throw 造成 noisy chain。標準 nested
// exception 不保存 stack trace，C++23 stacktrace 或 logging/tracing 可另補。
// ============================================================================

#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int parse_worker_count(const std::string& text)
{
    try {
        const int value = std::stoi(text);
        if (value <= 0) throw std::out_of_range("must be positive");
        return value;
    } catch (...) {
        std::throw_with_nested(std::runtime_error("invalid field: workers"));
    }
}

std::string exception_chain(const std::exception& error)
{
    std::string result = error.what();
    try {
        std::rethrow_if_nested(error);
    } catch (const std::exception& nested) {
        result += " <- " + exception_chain(nested);
    } catch (...) {
        result += " <- non-standard exception";
    }
    return result;
}

void basic_example()
{
    std::string chain;
    try { (void)parse_worker_count("abc"); }
    catch (const std::exception& error) { chain = exception_chain(error); }
    assert(chain.find("invalid field: workers") != std::string::npos);
    assert(chain.find("stoi") != std::string::npos);
    std::cout << "[基礎] nested chain: " << chain << '\n';
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 394. Decode String（字串解碼）
// 題目：將 k[encoded] 遞迴展開；例如 3[a]2[bc] 得 aaabcbc，3[a2[c]] 得 accaccacc。
// 為何使用本章主題：原題保證格式合法；production wrapper 以 throw_with_nested 加入完整 input context，同時保留缺括號的 parser root cause。
// 思路：1. 一般字元直接追加。2. 解析 repeat 與左括號。3. 遞迴解碼內層並驗右括號。4. 重複追加，最外層再驗無尾隨 ]。
// 複雜度：若輸出長度為 O，解碼至少時間/空間 O(O)；遞迴額外 stack O(D)，D 為最大巢狀深度。
// 易錯點：repeat 累積、輸出長度與遞迴深度都可能溢位或耗盡資源；nested exception 只保存原因鏈，不等於輸入限制。
// -----------------------------------------------------------------------------
std::string decode_at(const std::string& text, std::size_t& index)
{
    std::string output;
    while (index < text.size() && text.at(index) != ']') {
        if (text.at(index) < '0' || text.at(index) > '9') {
            output += text.at(index++);
            continue;
        }
        int repeat = 0;
        while (index < text.size() && text.at(index) >= '0' && text.at(index) <= '9') {
            repeat = repeat * 10 + (text.at(index++) - '0');
        }
        if (index >= text.size() || text.at(index++) != '[') throw std::invalid_argument("missing [");
        const std::string nested = decode_at(text, index);
        if (index >= text.size() || text.at(index++) != ']') throw std::invalid_argument("missing ]");
        for (int count = 0; count < repeat; ++count) output += nested;
    }
    return output;
}

std::string decode_string(const std::string& text)
{
    try {
        std::size_t index = 0U;
        std::string result = decode_at(text, index);
        if (index != text.size()) throw std::invalid_argument("unexpected ]");
        return result;
    } catch (...) {
        std::throw_with_nested(std::runtime_error("cannot decode: " + text));
    }
}

void leetcode_394_example()
{
    assert(decode_string("3[a]2[bc]") == "aaabcbc");
    assert(decode_string("3[a2[c]]") == "accaccacc");
    std::cout << "[LeetCode 394] nested decoder outputs expected strings\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析失敗的分層診斷日誌
// 情境：輸入 2[abc 缺少右括號，操作端需要看到高層 cannot decode context 與低層 missing ] 原因，使用者介面則只顯示安全摘要。
// 為何使用本章主題：exception_chain 透過 rethrow_if_nested 遞迴還原因果，比只丟新的 runtime_error 或拼接單層 what 保留更多型別化上下文。
// 設計：1. boundary 呼叫 decoder。2. 捕捉最外層 std::exception。3. 遞迴展開 nested causes。4. 驗證並輸出完整診斷鏈。
// 成本：D 層 chain 需 O(D) 次重拋與字串配置；訊息總長度另決定 I/O 成本。
// 上線注意：輸入、路徑或 secret 需遮罩並限制 chain 長度；遞迴 logger 也要處理非標準例外與 logging failure。
// -----------------------------------------------------------------------------
void practical_example()
{
    try { (void)decode_string("2[abc"); }
    catch (const std::exception& error) {
        const std::string chain = exception_chain(error);
        assert(chain.find("cannot decode") != std::string::npos);
        assert(chain.find("missing ]") != std::string::npos);
        std::cout << "[實務] decode failure retains input and root cause\n";
    }
}

int main()
{
    basic_example();
    leetcode_394_example();
    practical_example();
}

// 易錯與面試：只寫 `throw runtime_error("high level")` 會遺失 root cause；nested exception
// 保存 chain，但輸出時需遞迴 rethrow_if_nested，且對外訊息仍要避免洩漏敏感路徑/內容。
// 練習：為 parser 加位置欄位，讓 nested chain 顯示 byte offset。
// 複雜度：建立/列印 nested chain 是 O(D)，D 為 context 層數；每層可能另配置 message。
// 生命週期：nested exception 由 exception_ptr 類機制保存，即使原 catch scope 結束仍可重拋檢視。

/*
 * 【教科書補充：parser 的資源上限】
 * - repeat 數字累積、展開後長度與遞迴深度都需上限；overflow/stack exhaustion 不是普通 parse error。
 * - grammar 應拒絕 stray bracket、缺右括號、尾隨垃圾與空數字，不可只解析可辨識前綴。
 * - nested_exception 保留原因鏈，但每層 context 也要限長，避免錯誤訊息本身耗盡記憶體。
 * - production 可改 iterative stack parser，以顯式 depth/size budget 取代未受控 recursion。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_nested_exception.cpp' -o '/tmp/codex_cpp_C_Exception_08_nested_exception' && '/tmp/codex_cpp_C_Exception_08_nested_exception'
//
// === 預期輸出（節錄）===
// [實務] decode failure retains input and root cause
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
