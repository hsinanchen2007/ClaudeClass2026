// ============================================================================
// 課題 10：實務 logging - 一筆訊息一次提交、escaping、flush policy
// ============================================================================
//
// 多次 `cout <<` 可能與其他 thread 交錯；C++20 osyncstream 先在 local buffer 組一筆，
// destruction/emit 時對 wrapped stream 原子提交字元序列。它不保證跨 process、不負責
// log rotation/durability。高頻 logging 應用成熟 asynchronous logger。
//
// Structured log 必須 escape newline/quote/control chars，否則一筆 user input 可偽造多行。
// 每行 endl flush 很慢；error/fatal 或 shutdown 才依 policy flush。
// ============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <syncstream>
#include <unordered_map>

std::string escape_log(const std::string& input)
{
    std::string output;
    for (const char character : input) {
        if (character == '\n') output += "\\n";
        else if (character == '\r') output += "\\r";
        else if (character == '"') output += "\\\"";
        else output += character;
    }
    return output;
}

void write_log(std::ostream& destination, const std::string& level, const std::string& message)
{
    std::osyncstream line(destination);
    line << "level=" << level << " message=\"" << escape_log(message) << "\"\n";
}

void basic_example()
{
    std::ostringstream output;
    write_log(output, "INFO", "build\ndone");
    assert(output.str() == "level=INFO message=\"build\\ndone\"\n");
    std::cout << "[基礎] one escaped log record emitted atomically\n";
}

// LeetCode 359：Logger Rate Limiter；只有 shouldPrint=true 時才交給 logging transport。
class Logger {
public:
    bool shouldPrintMessage(int timestamp, const std::string& message)
    {
        const auto found = next_.find(message);
        if (found != next_.end() && timestamp < found->second) return false;
        next_[message] = timestamp + 10;
        return true;
    }
private:
    std::unordered_map<std::string, int> next_;
};

void leetcode_359_example()
{
    Logger logger;
    assert(logger.shouldPrintMessage(1, "foo"));
    assert(!logger.shouldPrintMessage(2, "foo"));
    assert(logger.shouldPrintMessage(11, "foo"));
    std::cout << "[LeetCode 359] rate limiter emits at t=1 and t=11\n";
}

// 實務：logger 接 ostream，unit test 可 capture，production 可接 file/cout。
void practical_example()
{
    std::ostringstream capture;
    write_log(capture, "WARN", "disk 90%");
    assert(capture.str().find("level=WARN") == 0U);
    std::cout << "[實務] injectable ostream makes logger testable\n";
}

int main()
{
    basic_example();
    leetcode_359_example();
    practical_example();
}

// 易錯與面試：多次 `<<` 不是跨 threads 的一筆原子 log；先在 local buffer 組整筆，再用
// mutex/synchronized stream 一次提交。flush 每筆代價高，但 crash-sensitive audit 需明定政策。
// 練習：補 JSON escaping（backslash/tab/control Unicode），或改用成熟 JSON library。
// 複雜度：格式化一筆 log 是 O(message bytes)；同步 sink 的 lock/I/O latency 可能主導成本。
// 生命週期：logger 借用 ostream 時，sink 必須比 logger 活更久；osyncstream 解構時才提交 chunk。
