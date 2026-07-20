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

// LeetCode 394：Decode String。題目保證合法；production wrapper 加 input context 並保留
// parser root cause。簡化 parser 支援 k[letters] nested 結構。
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

// 實務：boundary 保留 chain 給 log；user UI 可只顯示最外層安全訊息。
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
