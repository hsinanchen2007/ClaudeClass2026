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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 56. Merge Intervals（合併區間）
// 題目：將重疊區間合併；例如 [[1,4],[4,5]] 因端點相接而合併成 [[1,5]]。
// 為何使用本章主題：排序 lambda 展示 comparator 的安全契約；以 < 建立嚴格弱序，避免 <=
// 造成 comp(x,x) 為 true 而讓 std::sort 行為未定義。
// 思路：依 start、再依 end 排序；逐項追加不重疊區間；重疊時擴張最後結果的 end。
// 複雜度：時間 O(N log N)、額外空間 O(N)，N 是區間數，回傳結果最壞保存 N 筆。
// 易錯點：每個區間應滿足 start<=end；comparator 不可修改元素或依賴會變動的外部狀態。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】不延長 Session 壽命的長期 callback
// 情境：事件系統保存 callback，但 Session 可能先被關閉；呼叫時要安全取得 id 或回報已失效。
// 為何使用本章主題：lambda 捕獲 weak_ptr 而非 this 或 shared_ptr，既避免懸空指標，也不會因
// callback 反向持有 Session 而形成生命週期循環。
// 設計：factory 從 shared_ptr 建立 weak_ptr；每次 callback 先 lock；成功讀 id，失敗回 -1。
// 成本：每次呼叫 O(1)，但 weak_ptr::lock 有原子引用計數成本；closure 保存一個弱引用控制資訊。
// 上線注意：-1 sentinel 可能與合法 id 衝突，宜改 optional；併發析構雖由 lock 保護，Session 內容仍需同步。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_lambda_pitfalls.cpp' -o '/tmp/codex_cpp_C_Lambda_10_lambda_pitfalls' && '/tmp/codex_cpp_C_Lambda_10_lambda_pitfalls'
//
// === 預期輸出（節錄）===
// lambda pitfalls：safe capture、strict comparator、weak callback 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
