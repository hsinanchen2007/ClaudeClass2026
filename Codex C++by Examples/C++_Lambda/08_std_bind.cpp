/*
 * Lambda 教科書 08：std::bind
 *
 * std::bind 固定 callable 的部分 arguments，留下 std::placeholders::_1... 給未來呼叫。
 * bound arguments 預設 decay-copy；要保存 reference 必須 std::ref/std::cref。member function
 * pointer 可綁 object/pointer。回傳的 bind object 型別未指定，通常用 auto 保存。
 *
 * 【現代建議】lambda 通常更清楚、型別診斷更好，也能精準控制 move/reference；bind 在維護
 * 舊 callback adapters 或需要 composition 時仍會遇到。nested bind expression 規則尤其難讀。
 * 【placeholder】_1 代表未來 call 的第一 argument，不是 bind 當下第一個 argument。
 * 【生命週期】std::ref 不延長對象 lifetime；callback 活得更久時仍會 dangling。
 * 【常見陷阱】bound argument 預設 decay-copy；忘記 std::ref 會只改副本，nested bind 又難讀。
 * 【面試題】為何 `std::bind(f, x)` 修改不到原 x？預設複製，要 `std::ref(x)`。
 */

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace {

// 不使用 assert，讓測試在 -DNDEBUG 建置中仍會執行。
void expect(const bool condition) {
    if (!condition) std::abort();
}

}  // namespace

namespace basic {
int multiply(int left, int right) { return left * right; }

void demo() {
    using namespace std::placeholders;
    const auto double_value = std::bind(&multiply, _1, 2);
    expect(double_value(6) == 12);

    int value = 1;
    const auto increment = std::bind([](int& target) { ++target; }, std::ref(value));
    increment();
    expect(value == 2);
}
}  // namespace basic

namespace leetcode {
bool is_bad(int version, int first_bad) { return version >= first_bad; }

// LeetCode 278：First Bad Version。bind 固定 first_bad，留下 version placeholder。
int leetcode_first_bad_version(int versions, int first_bad) {
    using namespace std::placeholders;
    const auto bad = std::bind(&is_bad, _1, first_bad);
    int left = 1;
    int right = versions;
    while (left < right) {
        const int middle = left + (right - left) / 2;
        if (bad(middle)) right = middle;
        else left = middle + 1;
    }
    return left;
}

void leetcode_test() {
    expect(leetcode_first_bad_version(5, 4) == 4);
    expect(leetcode_first_bad_version(1, 1) == 1);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
class Worker {
public:
    explicit Worker(int factor) : factor_(factor) {}
    int process(int value) const { return value * factor_; }
private:
    int factor_;
};

// task 取得 Worker ownership；傳入 temporary 時，物件會被移入 bind object 而不會懸空。
auto practical_task(Worker worker) {
    using namespace std::placeholders;
    return std::bind(&Worker::process, std::move(worker), _1);
}

void practical_test() {
    const auto task = practical_task(Worker{3});
    expect(task(7) == 21);
    // task 自己持有 Worker，因此離開 factory 後仍可安全呼叫。
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "std::bind：placeholder/reference、First Bad Version、owned task 測試通過\n";
}
