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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（不可變陣列區間和）
// 題目：先接收整數陣列，再多次查詢 left 到 right 的總和；例如 [-2,0,3,-5,2,-1] 的 [0,2] 為 1。
// 為何使用本章主題：decltype 精確驗證 prefix_[0] 是 long long&；查詢仍回傳值，避免把內部儲存參考洩漏出去。
// 思路：1. 建立前綴和 prefix[i+1]；2. 查詢時以 prefix[right+1]-prefix[left] 消去區間外元素。
// 複雜度：N 為陣列長度；建構時間 O(N)、每次查詢 O(1)，額外空間 O(N)。
// 易錯點：右端要取 right+1；left/right 越界時 at 會丟例外，且大量總和需用 long long 避免 int 溢位。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】可寫設定欄位存取器
// 情境：管理工具要依字串 key 取得 Record 欄位，並直接修改 map 中的整數值。
// 為何使用本章主題：trailing return 搭配 decltype 保留 operator[] 的 int&，容器實作變更時簽章也會跟著運算式更新。
// 設計：1. 接收仍存活的 Record；2. 以 key 呼叫 fields[key]；3. 將實際 reference 回傳給呼叫端賦值。
// 成本：M 為欄位數；std::map 查找或插入時間 O(log M)，額外空間在缺 key 時為一個新節點。
// 上線注意：operator[] 會默默建立不存在的 key；回傳參考受 Record 與該元素生命週期約束，元素被 erase 後即失效。
// -----------------------------------------------------------------------------
struct Record {
    std::map<std::string, int> fields;
};

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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_decltype.cpp' -o '/tmp/codex_cpp_C_Cpp11_02_decltype' && '/tmp/codex_cpp_C_Cpp11_02_decltype'
//
// === 預期輸出（節錄）===
// decltype：型別規則、前綴和、欄位參考測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
