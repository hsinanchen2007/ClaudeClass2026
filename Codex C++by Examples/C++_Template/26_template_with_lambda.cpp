/*
 * 第 26 章：模板與 lambda
 *
 * 泛型 lambda 的 `auto` 參數其實讓 closure type 的 operator() 成為函式模板。
 * C++20 可寫明確模板參數：`[]<typename T>(T value)`，方便同一 T 重複使用、加 requires，
 * 或取得型別名稱。lambda 的 closure type 是匿名且唯一；以 auto 保存可保留具體型別與 inline。
 */

#include <algorithm>
#include <cassert>
#include <concepts>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

template <typename Range, typename Function>
auto transform_to_vector(const Range& values, Function function) {
    using Result = decltype(function(*values.begin()));
    std::vector<Result> output;
    output.reserve(values.size());
    for (const auto& value : values) {
        output.push_back(function(value));
    }
    return output;
}

// LeetCode 349：Intersection of Two Arrays。lambda 模板統一處理可轉成 int 的值。
std::vector<int> leetcode_intersection(const std::vector<int>& left,
                                       const std::vector<int>& right) {
    const std::unordered_set<int> members(left.begin(), left.end());
    std::unordered_set<int> emitted;
    std::vector<int> result;
    const auto emit_if_present = [&]<std::convertible_to<int> T>(T value) {
        const int normalized = static_cast<int>(value);
        if (members.contains(normalized) && emitted.insert(normalized).second) {
            result.push_back(normalized);
        }
    };
    for (int value : right) {
        emit_if_present(value);
    }
    std::sort(result.begin(), result.end());
    return result;
}

struct Employee {
    std::string name;
    int level{};
    int score{};
};

template <typename Projection>
void sort_employees(std::vector<Employee>& employees, Projection project) {
    std::sort(employees.begin(), employees.end(), [&](const Employee& a, const Employee& b) {
        return project(a) < project(b);
    });
}

// 【實務情境】報表可替換排序投影，而不讓 Employee 綁死單一 operator<。
void practical_projection_test() {
    std::vector<Employee> employees{{"Ada", 3, 90}, {"Linus", 5, 80}, {"Grace", 4, 95}};

    // 顯式模板 lambda：同一 projection 可回傳任意 totally_ordered 欄位。
    const auto by_score = []<typename T>(const T& item) requires requires { item.score; } {
        return item.score;
    };
    sort_employees(employees, by_score);
    assert(employees.front().name == "Linus");
    assert(employees.back().name == "Grace");

    const auto names = transform_to_vector(employees, [](const Employee& e) { return e.name; });
    assert((names == std::vector<std::string>{"Linus", "Ada", "Grace"}));
}

int main() {
    const auto twice = [](auto value) { return value + value; };
    assert(twice(3) == 6);
    assert(twice(std::string{"C++"}) == "C++C++");

    assert((leetcode_intersection({1, 2, 2, 1}, {2, 2}) == std::vector<int>{2}));
    assert((leetcode_intersection({4, 9, 5}, {9, 4, 9, 8, 4}) == std::vector<int>{4, 9}));

    practical_projection_test();
    std::cout << "模板與 lambda 測試完成\n";
}

/*
 * 【生命週期】capture by reference 的 lambda 不得活過被捕捉物件；存進 callback queue 時尤其危險。
 * 【陷阱】不同 lambda expression 即使內容相同也是不同 closure type；需要統一型別可用 std::function。
 * 【效能】模板接收 lambda 能 inline；std::function 提供 erasure，但可能有間接呼叫/配置成本。
 * 【面試】generic lambda 何時出現？C++14；顯式 template parameter list 是 C++20。
 * 【練習】為 transform_to_vector 加 concept，要求 Range 有 size/begin 且 Function 可呼叫。
 */

/*
 * 【教科書補充：transform result 與 range constraint】
 * - `decltype(function(*begin))` 可能是 T& 或 void，直接形成 vector<T&>/vector<void> 會失敗。
 * - 通用版本應用 invoke_result_t 再依 ownership 需求 remove_cvref_t，並 constraint callable/result。
 * - 目前還依賴 member size/begin/end，並非所有 range；若要泛用應使用 ranges::begin/size 等 CPO。
 * - sort projection 必須在排序期間穩定，且比較結果形成 strict weak ordering；不可讀會變動的外部 key。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '26_template_with_lambda.cpp' -o '/tmp/codex_cpp_C_Template_26_template_with_lambda' && '/tmp/codex_cpp_C_Template_26_template_with_lambda'
//
// === 預期輸出（節錄）===
// 模板與 lambda 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
