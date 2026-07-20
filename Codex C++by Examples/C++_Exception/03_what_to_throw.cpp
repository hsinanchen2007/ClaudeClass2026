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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 704. Binary Search（二分搜尋）
// 題目：在升冪且元素不重複的陣列尋找 target，存在則回 index，否則回 -1；例如找 9 得 4、找 2 得 -1。
// 為何使用本章主題：找不到是此 API 的預期結果，不應 throw；題目指定的 -1 sentinel 比 unwinding 更直接表達普通分支。
// 思路：1. 維持 [left,right) 搜尋區間。2. 比較 middle。3. 將不可能的一半排除。4. 驗證最後候選或回 -1。
// 複雜度：N 個元素的時間 O(log N)、額外空間 O(1)。
// 易錯點：輸入必須升冪；用 left+(right-left)/2 避免 index 加法溢位，並區分「找不到」與輸入契約失效。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】記憶體快取查詢的普通 miss
// 情境：從少量 key/value entries 查 model 設定；key 不存在很常見，caller 要能不靠例外分支處理。
// 為何使用本章主題：optional<int> 把「有值或 miss」放進回傳型別；只有資料格式損壞等無法履約狀況才適合 ParseError。
// 設計：1. 逐筆比較 key。2. 命中時按值回傳 int。3. 掃描完未命中回 nullopt。4. caller 用 has_value 分支。
// 成本：E 筆 entries 的最壞時間 O(E)、額外空間 O(1)；本例是 vector 線性查找而非 hash cache。
// 上線注意：optional 不含 miss 原因；若需區分過期、權限或後端故障，應使用 expected/result，且回傳值不得引用已失效 entry。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_what_to_throw.cpp' -o '/tmp/codex_cpp_C_Exception_03_what_to_throw' && '/tmp/codex_cpp_C_Exception_03_what_to_throw'
//
// === 預期輸出（節錄）===
// [實務] optional expresses ordinary cache miss
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
