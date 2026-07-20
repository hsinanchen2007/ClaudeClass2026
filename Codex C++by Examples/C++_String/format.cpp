/*
 * C++20 std::format：型別安全、位置清楚的文字格式化
 *
 * `{}` 是 replacement field，`{:04}`、`{:.2f}` 等指定寬度/精度。format 回傳新 string；
 * format_to 可寫入 output iterator。格式字串與參數不匹配會在可檢查情況編譯失敗，
 * runtime format string 則可能丟 format_error。不同標準庫支援時間曾落後，需實機驗證。
 *
 * 本機若 <format> 尚不可用，本檔用 feature-test fallback 保持 C++20 可建置，並清楚
 * 標記非等價的有限替代；有支援時一定執行真正 std::format。
 */

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if __has_include(<format>)
#include <format>
#endif

namespace {

std::string format_user(const std::string& name, const int score) {
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    return std::format("user={} score={:04}", name, score);
#else
    std::ostringstream output;
    output << "user=" << name << " score=";
    output.width(4);
    static_cast<void>(output.fill('0'));
    output << score;
    return output.str();
#endif
}

void basic_demo() {
    assert(format_user("Ada", 7) == "user=Ada score=0007");
}

// LeetCode 412（Fizz Buzz）：一般數字以 format/to_string 產生。
std::vector<std::string> leetcode_fizz_buzz(const int n) {
    std::vector<std::string> answer;
    for (int value = 1; value <= n; ++value) {
        if (value % 15 == 0) answer.emplace_back("FizzBuzz");
        else if (value % 3 == 0) answer.emplace_back("Fizz");
        else if (value % 5 == 0) answer.emplace_back("Buzz");
        else {
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
            answer.push_back(std::format("{}", value));
#else
            answer.push_back(std::to_string(value));
#endif
        }
    }
    return answer;
}

// 實務：固定欄位順序的 log，格式化改善可讀性；資料 escaping 仍需額外處理。
std::string practical_log_record(const int job_id, const double seconds) {
#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
    return std::format("job={} elapsed={:.3f}s", job_id, seconds);
#else
    std::ostringstream output;
    output.setf(std::ios::fixed);
    output.precision(3);
    output << "job=" << job_id << " elapsed=" << seconds << 's';
    return output.str();
#endif
}

}  // namespace

int main() {
    basic_demo();
    const auto values = leetcode_fizz_buzz(5);
    assert((values == std::vector<std::string>{"1", "2", "Fizz", "4", "Buzz"}));
    assert(practical_log_record(42, 1.25) == "job=42 elapsed=1.250s");
    std::cout << "format: tests passed\n";
}

/*
 * 【陷阱與面試】
 * - format 不會自動做 JSON/SQL/HTML escaping；格式化與安全編碼是不同責任。
 * - 動態格式字串在新標準可用 vformat/make_format_args，但要處理 format_error。
 * - 大量輸出可用 format_to(back_inserter(buffer),...) 降低中間 string。
 * - `{:04}` 的 4 是最小寬度，不是截斷上限。
 * 【練習】格式化十六進位 ID（`0x{:08x}`）並加入邊界測試。
 */
