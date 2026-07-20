// ============================================================================
// 課題 2：std::cout/std::cin 與 formatted extraction
// ============================================================================
//
// `cin >> value` 先跳 leading whitespace，再依型別解析；失敗設 failbit，value 通常不應
// 再使用。正確 loop 是 `while (input >> value)`，不是 while(!eof())：eof 只在一次讀取
// 嘗試越過結尾後才設，後者會多處理一次舊值。
//
// cin 預設 tie 到 cout，讀前 flush prompt；關閉 tie/sync 可加速競賽 I/O，但之後不要
// 無規則混 printf/scanf。library function 不應直接讀 global cin，接 istream& 才可測。
// ============================================================================

#include <cassert>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

std::vector<int> read_all_ints(std::istream& input)
{
    std::vector<int> values;
    for (int value = 0; input >> value;) values.push_back(value);
    return values;
}

void basic_example()
{
    std::istringstream input("1 2\n3 4");
    const auto values = read_all_ints(input);
    assert((values == std::vector<int>{1, 2, 3, 4}));
    assert(input.eof());
    std::cout << "[基礎] extraction treats spaces/newlines as whitespace\n";
}

// LeetCode 1480：Running Sum。模擬競賽 stdin：先讀 n，再讀 n 個值，驗讀取成功。
std::vector<int> running_sum(std::vector<int> nums)
{
    std::partial_sum(nums.begin(), nums.end(), nums.begin());
    return nums;
}

void leetcode_1480_example()
{
    std::istringstream input("4 1 2 3 4");
    std::size_t count = 0U;
    assert(input >> count);
    std::vector<int> nums(count);
    for (int& value : nums) assert(input >> value);
    assert((running_sum(nums) == std::vector<int>{1, 3, 6, 10}));
    std::cout << "[LeetCode 1480] parsed n values, running sums=1,3,6,10\n";
}

// 實務：不接受中途 malformed token，回傳 false 並不使用 partial result。
bool parse_exact_three(std::istream& input, std::vector<int>& destination)
{
    std::vector<int> candidate(3U);
    for (int& value : candidate) if (!(input >> value)) return false;
    std::string extra;
    if (input >> extra) return false;
    destination = std::move(candidate);
    return true;
}

void practical_example()
{
    std::vector<int> values{9};
    std::istringstream good("1 2 3");
    const bool good_parsed = parse_exact_three(good, values);
    assert(good_parsed);
    assert((values == std::vector<int>{1, 2, 3}));
    std::istringstream bad("1 x 3");
    const bool bad_rejected = !parse_exact_three(bad, values);
    assert(bad_rejected);
    assert((values == std::vector<int>{1, 2, 3}));
    std::cout << "[實務] parse-then-commit preserves destination on failure\n";
}

int main()
{
    basic_example();
    leetcode_1480_example();
    practical_example();
}

// 易錯與面試：`while (!in.eof())` 會多處理一次失敗讀取；應把 extraction 本身放條件。
// 解析先寫 temporary、完整成功才 commit，可避免半筆資料污染既有狀態。
// 練習：讀失敗後用 clear()+ignore() 恢復互動輸入；說明 batch parser 為何常直接失敗。
// 複雜度：formatted extraction 與 token 長度成正比；flush 可能額外觸發昂貴裝置 I/O。
// 生命週期：cin/cout 是 static-lifetime streams；函式借用 reference，不應保存 local streambuf 指標。
