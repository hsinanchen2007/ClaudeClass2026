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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 539. Minimum Time Difference（最小時間差）
// 題目：輸入至少兩個 HH:MM 時刻，求 24 小時循環中的最小分鐘差；例如 23:59 與 00:00 的答案是 1。
// 為何使用本章主題：hours 與 minutes 可直接組成 minutes duration，跨午夜差值也能以 24h 明確表示，不需混用裸整數單位。
// 思路：1. 解析每個字串為午夜後分鐘數。2. 排序。3. 比較所有相鄰差。4. 再比較首尾跨午夜差。
// 複雜度：N 個時刻的時間 O(N log N)、額外空間 O(N)，N 為 timePoints 數量。
// 易錯點：不可漏掉最後與第一個時刻的環形差；production parser 還須嚴格拒絕格式錯誤與超出 00:00..23:59 的值。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】服務重試的指數退避間隔
// 情境：網路請求失敗後從 100ms 開始倍增等待，並把最大間隔限制在 6400ms，避免持續壓迫故障服務。
// 為何使用本章主題：chrono duration 讓乘法結果仍帶毫秒單位，比回傳 unsigned 裸值更不易在 sleep 或排程 API 處誤用。
// 設計：1. 將 attempt 上限設為 6。2. 以 2 的 capped 次方放大 100ms。3. 回傳 milliseconds 給排程層。
// 成本：每次計算時間 O(1)、空間 O(1)，真正等待與重試 I/O 不在此函式內。
// 上線注意：實際系統通常還要加入 jitter、總 deadline 與可取消等待；位移前必須封頂以避免整數溢位或未定義行為。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_duration.cpp' -o '/tmp/codex_cpp_C_Chrono_03_duration' && '/tmp/codex_cpp_C_Chrono_03_duration'
//
// === 預期輸出（節錄）===
// [實務] typed exponential backoff capped at 6400ms
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
