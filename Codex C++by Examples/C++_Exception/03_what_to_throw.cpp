// ============================================================================
// 課題 3：應該 throw 什麼、何時不要 throw
// ============================================================================
//
// Throw value object（通常繼承 std::runtime_error/logic_error），不要 throw pointer/new，
// 否則 ownership 不清；不要 throw int/string，會失去標準 hierarchy。自訂 exception 可
// 帶 structured fields（error code/path/id），caller 不必解析 message。
//
// 預期且頻繁的「沒有結果」用 optional；需要詳細但預期的錯誤可用 expected（C++23）或
// error_code/result type。Exception 適合無法履約且正常 caller 不逐次分支的 failure。
// Constructor 無法回 status，建立不合法 object 時 throw 是合理選擇。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class ParseError : public std::runtime_error {
public:
    ParseError(std::size_t position, std::string message)
        : std::runtime_error(std::move(message)), position_(position) {}
    std::size_t position() const noexcept { return position_; }
private:
    std::size_t position_;
};

int parse_digit(char character, std::size_t position)
{
    if (character < '0' || character > '9') throw ParseError(position, "expected digit");
    return character - '0';
}

void basic_example()
{
    assert(parse_digit('7', 0U) == 7);
    bool structured = false;
    try { (void)parse_digit('x', 4U); }
    catch (const ParseError& error) { structured = error.position() == 4U; }
    assert(structured);
    std::cout << "[基礎] custom ParseError preserves position=4\n";
}

// LeetCode 704：Binary Search。找不到是題目正常結果，回 -1，不丟 exception。
int search(const std::vector<int>& nums, int target)
{
    std::size_t left = 0U;
    std::size_t right = nums.size();
    while (left < right) {
        const std::size_t middle = left + (right - left) / 2U;
        if (nums.at(middle) < target) left = middle + 1U;
        else right = middle;
    }
    return left < nums.size() && nums.at(left) == target ? static_cast<int>(left) : -1;
}

void leetcode_704_example()
{
    assert(search({-1, 0, 3, 5, 9, 12}, 9) == 4);
    assert(search({-1, 0, 3, 5, 9, 12}, 2) == -1);
    std::cout << "[LeetCode 704] not found is -1, not exception\n";
}

// 實務：cache miss 是預期狀態，用 optional；corrupt cache format 才 throw ParseError。
std::optional<int> lookup(const std::vector<std::pair<std::string, int>>& entries,
                          const std::string& key)
{
    for (const auto& entry : entries) if (entry.first == key) return entry.second;
    return std::nullopt;
}

void practical_example()
{
    const std::vector<std::pair<std::string, int>> cache{{"model", 42}};
    assert(lookup(cache, "model").value() == 42);
    assert(!lookup(cache, "missing").has_value());
    std::cout << "[實務] optional expresses ordinary cache miss\n";
}

int main()
{
    basic_example();
    leetcode_704_example();
    practical_example();
}

// 易錯與面試：不要用 exception 表示普通 miss/loop termination；也不要以 sentinel -1
// 混入合法 domain。exception、optional/expected、error_code 的選擇取決於失敗頻率與 API 邊界。
// 練習：設計 Result<T,Error>，比較它與 exception 在連續三層呼叫中的樣板碼。
// 複雜度：binary search O(log N)；cache lookup 平均 O(1)，正常 miss 不該支付 unwinding 成本。
// 生命週期：optional 以值擁有結果；自訂 exception 的 what() storage 則由 exception object 擁有。
