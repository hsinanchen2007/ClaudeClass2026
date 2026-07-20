/*
 * C++14 教科書：decltype(auto)
 *
 * decltype(auto) 用 decltype 規則推導整個宣告：能保留 reference、const 與 value category。
 * `decltype(auto) x = expression;` 不能再加 `&`，因 reference 已由 expression 決定。
 * 函式回傳時：`return object.member;` 與 `return (object.member);` 可能不同；括號版本是
 * lvalue expression，會回 T&。這個力量也很危險，回傳 local/temporary reference 會 dangling。
 *
 * 【選擇】要 value/copy 用 auto；要透明 forwarding/accessor 才用 decltype(auto)。
 * 【容器】vector<bool>::operator[] 回 proxy，decltype(auto) 可保留 proxy，但 caller 必須理解。
 * 【常見陷阱】多一層括號可能把 value return 改成 reference return，造成 dangling。
 * 【面試題】auto、decltype(expr)、decltype(auto) 三者如何處理 reference？
 * 【練習】為 practical_setting 寫 const overload，確認回傳 const std::string&。
 */

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace basic {
decltype(auto) first(std::vector<int>& values) {
    return (values.front());  // 括號使 expression 明確是 lvalue，回 int&。
}

auto first_copy(std::vector<int>& values) {
    return values.front();  // auto 傳值推導，回 int。
}

void demo() {
    std::vector<int> values{3, 4};
    static_assert(std::is_same<decltype(first(values)), int&>::value,
                  "decltype(auto) 應保留 front() reference");
    static_assert(std::is_same<decltype(first_copy(values)), int>::value,
                  "auto 應複製 front() value");
    first(values) = 9;
    assert(values.front() == 9 && first_copy(values) == 9);
}
}  // namespace basic

namespace leetcode {
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（不可變陣列區間和）
// 題目：對固定陣列回答多次 [left,right] 總和查詢；[-2,0,3,-5,2,-1] 的 [2,5] 為 -1。
// 為何使用本章主題：此 immutable API 刻意不用 decltype(auto)，而明確回 long long 值；保留 prefix_ reference 反而會破壞封裝。
// 思路：1. 建立多一格的前綴和；2. prefix[i+1] 累加 nums[i]；3. 用兩個前綴值相減回答區間。
// 複雜度：N 為元素數；建構時間 O(N)、每次查詢 O(1)，額外空間 O(N)。
// 易錯點：right 要加一；at 會檢查越界，但程式仍依賴 left<=right，且總和須避免 int 溢位。
// -----------------------------------------------------------------------------
class NumArray {
public:
    explicit NumArray(const std::vector<int>& nums) : prefix_(nums.size() + 1U, 0) {
        for (std::size_t i = 0; i < nums.size(); ++i) {
            prefix_[i + 1U] = prefix_[i] + nums[i];
        }
    }

    long long leetcode_sum_range(std::size_t left, std::size_t right) const {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<long long> prefix_;
};

void leetcode_test() {
    const NumArray values({-2, 0, 3, -5, 2, -1});
    assert(values.leetcode_sum_range(0U, 2U) == 1);
    assert(values.leetcode_sum_range(2U, 5U) == -1);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】可寫設定值存取器
// 情境：管理介面要依 key 取得既有設定字串，並透過回傳值直接更新 map 中的內容。
// 為何使用本章主題：decltype(auto) 套用 map::at 的精確型別，保留 std::string&；普通 auto 只會回傳副本。
// 設計：1. 接收可寫 Configuration；2. 使用 at 查既有 key；3. 將括號化 lvalue 原樣回傳給呼叫端。
// 成本：M 為設定數；std::map 查詢時間 O(log M)、額外空間 O(1)。
// 上線注意：key 不存在會丟 out_of_range；reference 受 config 與元素生命週期約束，erase 後不可再用。
// -----------------------------------------------------------------------------
struct Configuration {
    std::map<std::string, std::string> values;
};

decltype(auto) practical_setting(Configuration& config, const std::string& key) {
    return (config.values.at(key));  // 回 std::string&，讓管理介面可更新既有欄位。
}

void practical_test() {
    Configuration config{{{"mode", "safe"}}};
    practical_setting(config, "mode") = "fast";
    assert(config.values.at("mode") == "fast");
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "decltype(auto)：參考保留、前綴和、設定更新測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_decltype_auto.cpp' -o '/tmp/codex_cpp_C_Cpp14_02_decltype_auto' && '/tmp/codex_cpp_C_Cpp14_02_decltype_auto'
//
// === 預期輸出（節錄）===
// decltype(auto)：參考保留、前綴和、設定更新測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
