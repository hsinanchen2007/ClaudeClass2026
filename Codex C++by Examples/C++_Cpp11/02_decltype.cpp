/*
 * C++11 教科書：decltype 精確取得運算式型別
 *
 * 【規則速查】
 * - decltype(name) 若 name 是未加括號的變數/成員名稱，得到宣告時型別。
 * - 其他運算式依 value category：lvalue -> T&、xvalue -> T&&、prvalue -> T。
 * - 因此 decltype(x) 可能是 int，而 decltype((x)) 是 int&；多一層括號會改語意。
 * - decltype 不會執行運算式，可配 std::declval<T>() 詢問無法實際建構的型別。
 *
 * 【用途】泛型回傳型別、traits、保留 operator[] 的 reference/proxy 型別。
 * 【陷阱】decltype(auto) 是 C++14；回傳區域變數的 decltype((x)) 會造成 dangling。
 * 【面試題】decltype((std::move(x))) 是什麼？答案是 T&&。
 * 【練習】讓 practical::field 支援 const Record，觀察回傳型別如何改變。
 */

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace basic {
void demo() {
    int number = 3;
    const int fixed = 8;
    static_assert(std::is_same<decltype(number), int>::value,
                  "未括號變數名稱應取得宣告型別");
    static_assert(std::is_same<decltype((number)), int&>::value,
                  "括號 lvalue expression 應得到 T&");
    static_assert(std::is_same<decltype(fixed), const int>::value,
                  "decltype 應保留宣告中的 const");
    static_assert(std::is_same<decltype(std::move(number)), int&&>::value,
                  "xvalue expression 應得到 T&&");
}

// C++11 trailing return type 可在參數已進入 scope 後，計算 a+b 的真正型別。
template <class Left, class Right>
auto add(Left left, Right right) -> decltype(left + right) {
    return left + right;
}
}  // namespace basic

namespace leetcode {
// LeetCode 303：Range Sum Query - Immutable 的精簡版本。
// decltype(prefix_[0]) 精確是 long long&；對外仍明確回傳 long long，避免洩漏參考。
class NumArray {
public:
    explicit NumArray(const std::vector<int>& nums) : prefix_(nums.size() + 1U, 0) {
        for (std::size_t i = 0; i < nums.size(); ++i) {
            prefix_[i + 1U] = prefix_[i] + nums[i];
        }
        static_assert(std::is_same<decltype(prefix_[0]), long long&>::value,
                      "vector 非 const operator[] 應回傳 lvalue reference");
    }

    long long sum_range(std::size_t left, std::size_t right) const {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<long long> prefix_;
};

void test() {
    const NumArray sums({-2, 0, 3, -5, 2, -1});
    assert(sums.sum_range(0, 2) == 1);
    assert(sums.sum_range(2, 5) == -1);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
struct Record {
    std::map<std::string, int> fields;
};

// 實務：回傳 map::operator[] 的實際 reference；若容器型別日後改變，簽名仍跟著更新。
auto field(Record& record, const std::string& key)
    -> decltype(record.fields[key]) {
    return record.fields[key];
}

void test() {
    Record record{{{"retry", 2}}};
    field(record, "retry") = 5;
    assert(record.fields.at("retry") == 5);
    assert(basic::add(2, 0.5) == 2.5);
}
}  // namespace practical

void leetcode_test() { leetcode::test(); }
void practical_test() { practical::test(); }

int main() {
    basic::demo();
    leetcode_test();
    practical_test();
    std::cout << "decltype：型別規則、前綴和、欄位參考測試通過\n";
}
