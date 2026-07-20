// ============================================================================
// 課題 12：自訂 Forward Iterator 與 Range
// ============================================================================
//
// 自訂 iterator 的核心不是「塞滿 operators」，而是誠實宣告 semantic contract。
// 本例 CountingIterator 表示 [current, end) 的整數序列，符合 forward iterator：
//   value_type / difference_type / iterator_category / iterator_concept
//   operator*、pre/post ++、==
//
// C++20 algorithm 會看 concepts；舊 algorithm 仍可能透過 iterator_traits 看 typedef。
// 若 operator* 回 temporary，reference 可是 value_type；若回容器元素才通常是 T&。
// 不要宣稱 random_access 卻用 O(N) 實作 +n，否則 algorithm 的複雜度承諾會說謊。
// ============================================================================

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

// 測試在 -DNDEBUG 下也必須保留。
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

class CountingIterator {
public:
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using reference = int;
    using pointer = void;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr explicit CountingIterator(int value = 0) noexcept : value_(value) {}

    [[nodiscard]] constexpr int operator*() const noexcept { return value_; }

    constexpr CountingIterator& operator++() noexcept
    {
        ++value_;
        return *this;
    }

    constexpr CountingIterator operator++(int) noexcept
    {
        CountingIterator old = *this;
        ++(*this);
        return old;
    }

    friend constexpr bool operator==(CountingIterator, CountingIterator) = default;

private:
    int value_{};
};

static_assert(std::forward_iterator<CountingIterator>);

class CountingRange {
public:
    // 【契約】只接受半開區間 [first, last) 且 first <= last。反向區間在 runtime
    // 丟 invalid_argument，避免 iterator 永遠追不到 end，最後對 int 做 signed overflow。
    constexpr CountingRange(int first, int last) : first_(first), last_(last)
    {
        if (first > last) throw std::invalid_argument("CountingRange 要求 first <= last");
    }
    [[nodiscard]] constexpr CountingIterator begin() const noexcept
    {
        return CountingIterator(first_);
    }
    [[nodiscard]] constexpr CountingIterator end() const noexcept
    {
        return CountingIterator(last_);
    }

private:
    int first_{};
    int last_{};
};

void basic_example()
{
    const CountingRange range(1, 5); // 半開區間 [1, 5)。
    expect(std::accumulate(range.begin(), range.end(), 0) == 10,
           "[1, 5) 的總和應為 10");
    expect(std::distance(range.begin(), range.end()) == 4,
           "[1, 5) 的距離應為 4");

    const CountingRange empty(5, 5);
    expect(empty.begin() == empty.end(), "first == last 應形成合法空區間");
    std::cout << "[基礎] custom forward range generated 1,2,3,4\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array（一維陣列的動態和）
// 題目：將 nums 轉成前綴和；本例用 [1,5) 產生 [1,2,3,4]，結果為 [1,3,6,10]。
// 為何使用本章主題：這是自訂 iterator 教學改寫，CountingRange 取代題目現成陣列，證明標準 algorithm 可消費該 range。
// 思路：建立合法半開計數範圍；由 iterators materialize vector；partial_sum 原地計算前綴和。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 last-first；輸出本身即占 O(N)。
// 易錯點：first>last 必須拒絕；接近 INT_MAX 不可遞增越界；iterator category 必須誠實維持 multi-pass 契約。
// -----------------------------------------------------------------------------
std::vector<int> running_sum(int first, int last)
{
    const CountingRange range(first, last);
    std::vector<int> result(range.begin(), range.end());
    std::partial_sum(result.begin(), result.end(), result.begin());
    return result;
}

void leetcode_1480_example()
{
    expect((running_sum(1, 5) == std::vector<int>{1, 3, 6, 10}),
           "running sum 結果錯誤");
    expect(running_sum(3, 3).empty(), "空區間的 running sum 應為空");
    std::cout << "[LeetCode 1480] standard partial_sum accepted custom iterators\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】惰性產生連續工單識別碼
// 情境：要搜尋 1000 到 1003 的工單 id，但不想先配置只為保存連號整數的 vector。
// 為何使用本章主題：CountingRange 以兩個小 iterator 表示序列，標準 find 可直接消費，不需 materialize 全部 id。
// 設計：建立 [1000,1004)；find 1002 並驗命中；再搜尋不存在值並驗 end sentinel。
// 成本：搜尋時間 O(N)、range 額外空間 O(1)，N 為 id 範圍長度。
// 上線注意：range 不應跨越 int 上限；不同 range 的 iterators 不可混比；大量查存在性可直接做範圍算術而非線性 find。
// -----------------------------------------------------------------------------
void practical_example()
{
    const CountingRange order_ids(1000, 1004);
    const auto found = std::find(order_ids.begin(), order_ids.end(), 1002);
    expect(found != order_ids.end() && *found == 1002, "應找到工單 1002");
    expect(std::find(order_ids.begin(), order_ids.end(), 9999) == order_ids.end(),
           "不存在的工單不應被找到");
    std::cout << "[實務] lazy order-id range works with std::find\n";
}

void boundary_example()
{
    bool reversed_rejected = false;
    try {
        const CountingRange reversed(5, 4);
        static_cast<void>(reversed);
    } catch (const std::invalid_argument&) {
        reversed_rejected = true;
    }
    expect(reversed_rejected, "first > last 的區間必須在 runtime 被拒絕");

    const int maximum = std::numeric_limits<int>::max();
    const CountingRange near_maximum(maximum - 2, maximum);
    const std::vector<int> values(near_maximum.begin(), near_maximum.end());
    expect((values == std::vector<int>{maximum - 2, maximum - 1}),
           "接近 INT_MAX 的合法區間不可 overflow");
}

int main()
{
    basic_example();
    leetcode_1480_example();
    practical_example();
    boundary_example();
}

// 易錯：宣告比實作更強的 category 會讓 algorithm 的正確性或複雜度假設失真。
// 面試自問：iterator_category 與 iterator_concept 為何可能不同？
// 練習：加入 sentinel type，讓 end 不必與 iterator 同型別（C++20 ranges 模型）。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_custom_iterator.cpp' -o '/tmp/codex_cpp_C_Iterator_12_custom_iterator' && '/tmp/codex_cpp_C_Iterator_12_custom_iterator'
//
// === 預期輸出（節錄）===
// [實務] lazy order-id range works with std::find
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
