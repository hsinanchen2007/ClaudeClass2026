/*
 * C++11 教科書：enum class（scoped enumeration）
 *
 * enum class 的 enumerator 必須用 Type::Value 存取，不會污染外層 namespace，也不會
 * 隱式轉成 int；這能防止不同列舉互相比較或傳錯 API。可指定 underlying type，適合
 * protocol/serialization，但外部格式仍應顯式 encode，不要直接 memcpy enum。
 *
 * 【轉換】需要數值時用 static_cast<Underlying>(value)。反向 cast 前要驗證數值範圍。
 * 【switch】列出所有 enumerator 且不放 default，可讓 compiler warning 提醒新增狀態；
 * 若輸入可能含非法 cast 值，則仍需 defensive handling。
 * 【位元旗標】enum class 不自帶 operator|，需自行定義或用專門 flags wrapper。
 * 【常見陷阱】由未驗證整數 static_cast 成 enum class 後，值可能不是任何 enumerator。
 * 【面試題】enum 與 enum class 的 scope、conversion、underlying type 有何差異？
 */

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace basic {
enum class Color : unsigned char { red = 1, green = 2, blue = 3 };

const char* name(Color color) {
    switch (color) {
        case Color::red: return "red";
        case Color::green: return "green";
        case Color::blue: return "blue";
    }
    throw std::invalid_argument("未知 Color");
}

void demo() {
    const Color color = Color::green;
    assert(std::string(name(color)) == "green");
    assert(static_cast<unsigned char>(color) == 2U);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 682. Baseball Game（棒球比賽計分）
// 題目：依序處理整數、+、D、C 操作並回傳有效分數總和；["5","2","C","D","+"] 得 30。
// 為何使用本章主題：先將 token 分類成 scoped Operation，讓 switch 分支不與其他整數或列舉狀態混用。
// 思路：1. 將 token 映射為四種操作；2. 依操作新增、加總、加倍或取消分數；3. 最後累加有效記錄。
// 複雜度：N 為操作數；時間 O(N)、額外空間 O(N)。
// 易錯點：+ 需要至少兩筆、D/C 需要至少一筆；本程式依賴題目保證操作有效，數字解析仍可能丟例外。
// -----------------------------------------------------------------------------
enum class Operation { score, add_last_two, double_last, cancel_last };

int cal_points(const std::vector<std::string>& operations) {
    std::vector<int> scores;
    for (std::vector<std::string>::const_reference token : operations) {
        Operation operation = Operation::score;
        if (token == "+") operation = Operation::add_last_two;
        else if (token == "D") operation = Operation::double_last;
        else if (token == "C") operation = Operation::cancel_last;

        switch (operation) {
            case Operation::score:
                scores.push_back(std::stoi(std::string(token)));
                break;
            case Operation::add_last_two:
                scores.push_back(scores[scores.size() - 1U] + scores[scores.size() - 2U]);
                break;
            case Operation::double_last:
                scores.push_back(scores.back() * 2);
                break;
            case Operation::cancel_last:
                scores.pop_back();
                break;
        }
    }
    int total = 0;
    for (const int score : scores) total += score;
    return total;
}

void test() { assert(cal_points({"5", "2", "C", "D", "+"}) == 30); }
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】背景工作狀態轉移驗證
// 情境：排程工作只能由 queued 進 running，再由 running 結束為 succeeded 或 failed。
// 為何使用本章主題：enum class 將 JobState 限定在自己的 scope，避免與其他整數狀態或列舉隱式混用。
// 設計：1. 列出 queued->running；2. 列出 running->兩種終態；3. 其餘組合一律拒絕。
// 成本：每次檢查時間與空間皆 O(1)。
// 上線注意：新增 cancelled 等狀態時要同步更新所有轉移；並行 worker 仍需以交易或 compare-and-swap 保護狀態。
// -----------------------------------------------------------------------------
enum class JobState { queued, running, succeeded, failed };

bool may_transition(JobState from, JobState to) {
    return (from == JobState::queued && to == JobState::running) ||
           (from == JobState::running &&
            (to == JobState::succeeded || to == JobState::failed));
}

void test() {
    assert(may_transition(JobState::queued, JobState::running));
    assert(!may_transition(JobState::succeeded, JobState::running));
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "enum class：強型別狀態、Baseball Game、工作流測試通過\n";
}

// 【延伸練習】新增 cancelled 狀態；讓 compiler 幫你找出所有必須更新的 switch/transition。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_enum_class.cpp' -o '/tmp/codex_cpp_C_Cpp11_12_enum_class' && '/tmp/codex_cpp_C_Cpp11_12_enum_class'
//
// === 預期輸出（節錄）===
// enum class：強型別狀態、Baseball Game、工作流測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
