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

// -----------------------------------------------------------------------------
// 【日常實務範例】可注入 sink 的單筆同步 log
// 情境：將 level/message 組成一行，換行、CR、quote 先 escape；測試接 ostringstream，production 可接檔案或 cout。
// 為何使用本章主題：write_log 接 ostream& 以抽象 sink，osyncstream 在 local buffer 組完後一次提交，
//       相較多次直接 `destination <<` 可避免同程序多執行緒把一筆紀錄交錯。
// 設計：1. escape message 的危險字元；2. 以 osyncstream 包 sink；3. 組完整欄位與 newline；4. scope 結束提交。
// 成本：時間與暫存空間 O(N)，N 是 message bytes；提交還有 sink lock、格式化與實際 I/O 延遲。
// 上線注意：目前 escaping 未涵蓋反斜線、tab 與全部 control bytes；sink 生命週期需涵蓋呼叫，
//       osyncstream 不提供跨程序原子性、rotation 或 durability，flush policy 必須另訂。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 359. Logger Rate Limiter（紀錄器速率限制）
// 題目：相同 message 在前次允許輸出後 10 秒內不得再次輸出；foo 在 t=1、t=2、t=11 的結果為 true/false/true。
// 為何使用本章主題：演算法以 unordered_map 保存每個 message 的下一次允許時間，只有 true
//       才應交給 ostream logging transport；iostream 本身不負責 rate limit。
// 思路：1. 查 message；2. timestamp 小於 next 時拒絕；3. 否則更新 next=timestamp+10；4. 允許輸出。
// 複雜度：每次平均時間 O(1)、最壞 O(U)，空間 O(U)，U 是不同 message 數。
// 易錯點：長期程序 map 會持續成長，需淘汰舊 key；timestamp+10 要防 overflow，並發呼叫也需同步。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_practical_log.cpp' -o '/tmp/codex_cpp_C_IOStream_10_practical_log' && '/tmp/codex_cpp_C_IOStream_10_practical_log'
//
// === 預期輸出（節錄）===
// [實務] injectable ostream makes logger testable
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
