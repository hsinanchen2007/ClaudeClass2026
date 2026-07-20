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
// LeetCode 682：Baseball Game。先把字串 token 分類成 enum，再處理狀態。
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

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
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
