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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 412. Fizz Buzz（Fizz Buzz）
// 題目：產生 1..n 的字串；3 的倍數用 Fizz、5 的倍數用 Buzz、同時為倍數用 FizzBuzz，否則輸出數字。
// 為何使用本章主題：非倍數分支需把 int 轉成 string；標準庫支援時用 format("{}", value)，
//       否則以 to_string fallback，格式化不是倍數判斷本身的演算法。
// 思路：1. 逐一走 1..n；2. 先判 15；3. 再判 3/5；4. 其他值格式化成十進位。
// 複雜度：時間 O(N*D)、額外空間 O(N*D)，D 是數值輸出平均位數，空間主要是答案集合。
// 易錯點：15 必須先於 3 與 5；feature-test 分支要保持輸出一致，n<=0 時答案為空。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】工作耗時紀錄格式化
// 情境：把 job id 與秒數輸出成固定欄位順序，elapsed 一律顯示三位小數，例如 1.25 成為 1.250s。
// 為何使用本章主題：std::format 將欄位、型別與精度放在同一格式字串，較手動串接清楚；
//       fallback 用 local ostringstream 隔離 flags，避免污染 caller stream。
// 設計：1. 依 feature test 選格式器；2. 寫 job 整數；3. 以 fixed/三位小數寫 seconds。
// 成本：時間與輸出字元數 O(L)、額外空間 O(L)，L 是結果長度；格式器可能配置。
// 上線注意：格式化不會做 JSON/HTML escaping；浮點 round policy、非有限值與 runtime format_error 要明定。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'format.cpp' -o '/tmp/codex_cpp_C_String_format' && '/tmp/codex_cpp_C_String_format'
//
// === 預期輸出（節錄）===
// format: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
