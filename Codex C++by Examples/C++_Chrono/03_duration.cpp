// ============================================================================
// 課題 3：duration、ratio、conversion、floor/round
// ============================================================================
//
// duration<Rep,Period> 的 Period 是每 tick 幾秒，例如 milliseconds 是 ratio<1,1000>。
// 精度不損失的 conversion 可 implicit（seconds→milliseconds）；可能損失的方向必須
// duration_cast（milliseconds→seconds）。C++17 有 floor/ceil/round 可明定政策。
//
// `duration_cast<seconds>(1500ms)==1s` 是 toward-zero truncation。負 duration 時 floor 與
// cast 不同：floor(-1500ms)=-2s，cast=-1s。持久化裸 count 時必須同時固定 unit/width。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono_literals;

void basic_example()
{
    const std::chrono::milliseconds precise = 1'500ms;
    const std::chrono::seconds truncated = std::chrono::duration_cast<std::chrono::seconds>(precise);
    assert(truncated == 1s);
    assert(std::chrono::round<std::chrono::seconds>(precise) == 2s);
    assert(std::chrono::floor<std::chrono::seconds>(-1'500ms) == -2s);
    std::cout << "[基礎] 1500ms cast=1s round=2s; floor(-1500ms)=-2s\n";
}

// LeetCode 539：Minimum Time Difference。
// 將 HH:MM 解析成 minutes duration，排序後比較相鄰與跨午夜 gap。
std::chrono::minutes parse_time(const std::string& text)
{
    const int hours = std::stoi(text.substr(0U, 2U));
    const int minutes = std::stoi(text.substr(3U, 2U));
    return std::chrono::hours(hours) + std::chrono::minutes(minutes);
}

int find_min_difference(const std::vector<std::string>& points)
{
    if (points.size() < 2U) throw std::invalid_argument("at least two times required");
    std::vector<std::chrono::minutes> times;
    for (const std::string& point : points) times.push_back(parse_time(point));
    std::sort(times.begin(), times.end());
    std::chrono::minutes minimum{24 * 60};
    for (std::size_t index = 1U; index < times.size(); ++index) {
        minimum = std::min(minimum, times.at(index) - times.at(index - 1U));
    }
    minimum = std::min(minimum, 24h - times.back() + times.front());
    return static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(minimum).count());
}

void leetcode_539_example()
{
    assert(find_min_difference({"23:59", "00:00"}) == 1);
    assert(find_min_difference({"00:00", "04:00", "22:00"}) == 120);
    std::cout << "[LeetCode 539] minimum gaps 1 and 120 minutes\n";
}

// 實務：backoff 以 duration 算，不把毫秒與秒裸整數混在一起。
std::chrono::milliseconds exponential_backoff(unsigned attempt)
{
    const unsigned capped = std::min(attempt, 6U);
    return 100ms * (1U << capped);
}

void practical_example()
{
    assert(exponential_backoff(0U) == 100ms);
    assert(exponential_backoff(3U) == 800ms);
    assert(exponential_backoff(100U) == 6'400ms);
    std::cout << "[實務] typed exponential backoff capped at 6400ms\n";
}

int main()
{
    basic_example();
    leetcode_539_example();
    practical_example();
}

// 易錯與面試：duration_cast 對整數 duration 轉成較粗單位時會截斷，不是四捨五入；
// mixed units 應讓 common_type/chrono operators 處理，不要先 `.count()` 變裸數字再相加。
// 練習：強化 parse_time，拒絕格式錯誤與 24:00/12:60。
// 複雜度與生命週期：duration conversion/arithmetic 對固定 rep 是 O(1)；duration 按值擁有
// count，不引用原變數，但 integral rep 仍可能 overflow，需在 boundary 先驗範圍。
