/*
 * Lambda 教科書 10：常見陷阱與安全替代
 *
 * 1. 回傳 `[&local]` callback：local 已析構，呼叫時 dangling reference/UB。
 * 2. 長期 callback capture `[this]`：object 死亡後 pointer dangling；可 capture shared owner，
 *    或 capture weak_ptr 並在每次呼叫 lock。
 * 3. comparator 使用 <=、隨機值或會變 key：不滿足 strict weak ordering，sort 是 UB。
 * 4. 同一 mutable closure 跨執行緒更新 state：沒有同步就 data race。
 * 5. `[=]`/`[&]` 默默多 capture；review 難以看出 ownership/lifetime，優先明列。
 * 6. algorithm predicate 有外部副作用：call 次數/順序未必符合直覺，exception 留半成品。
 *
 * 危險程式只放註解，預設路徑不觸發 UB/data race。安全原則：先決定 callback 最長生命週期，
 * 再決定 capture value、shared ownership、weak observation 或同步策略。
 * 【面試題】weak_ptr callback 為何不是保證執行？owner 可已消失，lock 失敗就應跳過。
 * 【練習】讓 practical_callback 在 owner 消失時回傳 optional<int> 而非 -1 sentinel。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace basic {
auto safe_value_callback(int value) {
    return [value] { return value; };  // value copy 活在 closure 中。
}

void demo() {
    const auto callback = safe_value_callback(42);
    assert(callback() == 42);
    // auto bad(){ int local=42; return [&local]{ return local; }; } // 呼叫時 dangling。
}
}  // namespace basic

namespace leetcode {
// LeetCode 56：Merge Intervals。comparator 必須用 <，不可用 <=。
using Interval = std::pair<int, int>;

std::vector<Interval> leetcode_merge(std::vector<Interval> intervals) {
    std::sort(intervals.begin(), intervals.end(),
              [](const Interval& left, const Interval& right) {
                  if (left.first != right.first) return left.first < right.first;
                  return left.second < right.second;
              });
    std::vector<Interval> result;
    for (const Interval& current : intervals) {
        if (result.empty() || result.back().second < current.first) result.push_back(current);
        else result.back().second = std::max(result.back().second, current.second);
    }
    return result;
}

void leetcode_test() {
    assert(leetcode_merge({{1, 4}, {4, 5}}) == std::vector<Interval>({{1, 5}}));
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
class Session {
public:
    explicit Session(int id) : id_(id) {}
    int id() const noexcept { return id_; }
private:
    int id_;
};

auto practical_callback(const std::shared_ptr<Session>& session) {
    const std::weak_ptr<Session> weak = session;
    return [weak] {
        if (const std::shared_ptr<Session> alive = weak.lock()) return alive->id();
        return -1;  // owner 已消失，安全地略過；不解參考 dangling this。
    };
}

void practical_test() {
    auto session = std::make_shared<Session>(7);
    const auto callback = practical_callback(session);
    assert(callback() == 7);
    session.reset();
    assert(callback() == -1);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "lambda pitfalls：safe capture、strict comparator、weak callback 測試通過\n";
}
