// ============================================================================
// Parallel Merge Sort：有界 fork-join、穩定合併、例外收斂與合作式取消
// ============================================================================
// 【問題】merge sort 將 [first,last) 分成互不重疊的左右半部，各自排序後再 merge；
// 兩半可以平行，但若每層都開 thread，task 數與排程成本會壓過排序本身。
// 【介面契約】iterator 必須是同一個有效範圍的 random-access iterator；Compare 必須
// CopyConstructible 且形成 strict weak ordering。違反 iterator/比較契約屬呼叫端錯誤。
// 【同步模型】左右 task 只寫各自半開區間，所以不需 mutex；future.get 完成 join，
// worker 的寫入 happens-before 後續 inplace_merge，merge 絕不與子排序同時碰同一元素。
// 【ownership】呼叫者在函式返回前不得修改、搬移或銷毀底層容器；每個 task 擁有自己的
// comparator 副本。若副本內共享可變指標，該共享狀態仍須由 comparator 自己同步。
// 【生命週期】std::launch::async 強制非 deferred task；每條正常、取消與例外路徑都會
// 呼叫 future.get。沒有 detached thread，也不讓 iterator/reference 活過輸入範圍。
// 【有界平行】parallel_depth 限制 fork 層數，min_parallel_size 避免小區間開 task；
// 同時存在的 task 數受兩者與 n 共同限制，程式不計算 1 << depth，避免 shift overflow。
// 【取消】stop_token 只在遞迴與 merge 前的安全點檢查；已進行的 inplace_merge 不強停。
// 回 cancelled 時範圍仍含有效物件，但可能只局部有序，呼叫者不可把它當排序結果使用。
// 【例外】worker 例外由 future.get 重新拋出；一側失敗會設 internal_cancel，另一側在
// 安全點停止。即使左右同時失敗也會先收割兩個分支，再依明確優先序重拋其中一個。
// 【例外保證】配置、元素 move 或 comparator 若拋例外，只保證 range 內物件仍可析構；
// 不承諾原順序或已完成排序。需要 strong guarantee 時應排序副本，成功後再 swap。
// 【複雜度】若 inplace_merge 取得足夠暫存，總比較 O(n log n)、額外空間最多 O(n)；
// 在無足夠額外記憶體的實作路徑，單次 merge 可到 O(n log n)，整體最差 O(n log^2 n)。
// 【contention】元素沒有鎖爭用，但 task 建立、allocator、最後幾層 merge 與記憶體頻寬
// 仍會成瓶頸；平行度超過核心或 NUMA locality 不佳時，更多 task 反而更慢。
// 【穩定性】左右遞迴都穩定，std::inplace_merge 也穩定，所以相等 key 保留輸入順序。
// 【overflow】中點用 first + (last-first)/2，不把兩個索引相加；threshold 至少為 2，
// 深度只遞減且遞迴層數也受區間長度限制，不會因 unsigned depth 回繞。
// 【deadlock】演算法本身不持 mutex 等 future；但 comparator 若取得外部鎖，呼叫者不可
// 在持有同一把鎖時啟動排序，否則 callback 式重入仍可能造成自我 deadlock。
// 【面試追問】為何不用平行 quicksort？merge sort 穩定且最差界清楚；代價是額外記憶體
// 與 merge bandwidth。quicksort locality 較佳，但 pivot 不良時需 introsort 才守住最差界。

#include <algorithm>
#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

enum class SortStatus : std::uint8_t {
    completed,
    cancelled,
};

void expect(bool condition, const char* message)
{
    if (!condition) throw std::logic_error(message);
    assert(condition); // 排序與取消操作已先完成；assert 本身沒有副作用。
}

namespace detail {

template <std::random_access_iterator RandomIterator, class Compare>
    requires std::sortable<RandomIterator, Compare>
bool parallel_merge_sort_impl(RandomIterator first,
                              RandomIterator last,
                              Compare compare,
                              unsigned int parallel_depth,
                              std::size_t min_parallel_size,
                              const std::stop_token& stop_token,
                              std::atomic<bool>& internal_cancel)
{
    if (stop_token.stop_requested() ||
        internal_cancel.load(std::memory_order_acquire)) {
        return false;
    }

    const std::iter_difference_t<RandomIterator> length = last - first;
    if (length <= 1) return true;
    const RandomIterator middle = first + length / 2;
    const bool should_fork =
        parallel_depth > 0U &&
        static_cast<std::size_t>(length) >= min_parallel_size;

    if (should_fork) {
        // 所有 comparator copy 都在啟動 task 前完成，兩分支不共用 comparator 物件。
        Compare left_compare(compare);
        Compare right_compare(compare);
        const unsigned int child_depth = parallel_depth - 1U;

        auto left = std::async(
            std::launch::async,
            [first, middle, child_depth, min_parallel_size, stop_token,
             &internal_cancel, child_compare = std::move(left_compare)]() mutable {
                try {
                    return parallel_merge_sort_impl(
                        first, middle, std::move(child_compare), child_depth,
                        min_parallel_size, stop_token, internal_cancel);
                } catch (...) {
                    internal_cancel.store(true, std::memory_order_release);
                    throw;
                }
            });

        bool right_completed = false;
        std::exception_ptr right_error;
        try {
            right_completed = parallel_merge_sort_impl(
                middle, last, std::move(right_compare), child_depth,
                min_parallel_size, stop_token, internal_cancel);
        } catch (...) {
            internal_cancel.store(true, std::memory_order_release);
            right_error = std::current_exception();
        }

        bool left_completed = false;
        std::exception_ptr left_error;
        try {
            left_completed = left.get();
        } catch (...) {
            left_error = std::current_exception();
        }

        // 固定先重拋目前 thread 的右分支；無論哪邊失敗，上面都已 join 左 task。
        if (right_error) std::rethrow_exception(right_error);
        if (left_error) std::rethrow_exception(left_error);
        if (!left_completed || !right_completed) return false;
    } else {
        Compare left_compare(compare);
        if (!parallel_merge_sort_impl(first, middle, std::move(left_compare), 0U,
                                      min_parallel_size, stop_token,
                                      internal_cancel)) {
            return false;
        }

        Compare right_compare(compare);
        if (!parallel_merge_sort_impl(middle, last, std::move(right_compare), 0U,
                                      min_parallel_size, stop_token,
                                      internal_cancel)) {
            return false;
        }
    }

    if (stop_token.stop_requested() ||
        internal_cancel.load(std::memory_order_acquire)) {
        return false;
    }

    std::inplace_merge(first, middle, last, compare);
    return true;
}

} // namespace detail

template <std::random_access_iterator RandomIterator, class Compare>
    requires std::sortable<RandomIterator, Compare>
[[nodiscard]] SortStatus parallel_merge_sort(
    RandomIterator first,
    RandomIterator last,
    Compare compare,
    unsigned int parallel_depth,
    std::size_t min_parallel_size = 2'048U,
    std::stop_token stop_token = {})
{
    if (min_parallel_size < 2U) {
        throw std::invalid_argument("min_parallel_size must be at least two");
    }

    std::atomic<bool> internal_cancel{false};
    try {
        const bool completed = detail::parallel_merge_sort_impl(
            first, last, std::move(compare), parallel_depth, min_parallel_size,
            stop_token, internal_cancel);
        return completed ? SortStatus::completed : SortStatus::cancelled;
    } catch (...) {
        internal_cancel.store(true, std::memory_order_release);
        throw;
    }
}

struct CountingLess {
    // mutable state 故意不使用 atomic；安全性來自每個 task 都取得 comparator 副本。
    mutable std::size_t comparisons{0U};

    bool operator()(int left, int right) const
    {
        ++comparisons;
        return left < right;
    }
};

void basic_demo()
{
    std::vector<int> values{5, 1, 4, 2, 8, 3, 8, -1};
    const SortStatus status = parallel_merge_sort(
        values.begin(), values.end(), CountingLess{}, 2U, 2U);
    expect(status == SortStatus::completed, "basic merge sort was cancelled");
    expect(std::ranges::is_sorted(values), "basic merge sort result is not sorted");

    std::vector<int> empty;
    const SortStatus empty_status = parallel_merge_sort(
        empty.begin(), empty.end(), std::less<int>{}, 2U);
    expect(empty_status == SortStatus::completed, "empty range did not complete");

    bool bad_threshold_rejected = false;
    try {
        static_cast<void>(parallel_merge_sort(
            values.begin(), values.end(), std::less<int>{}, 1U, 1U));
    } catch (const std::invalid_argument&) {
        bad_threshold_rejected = true;
    }
    expect(bad_threshold_rejected, "invalid parallel threshold was accepted");
}

// ----------------------------------------------------------------------------
// LeetCode 912：Sort an Array。
// 契約：接收 nums 的副本並回傳非遞減排列；不改呼叫端原 vector，重複值與負數皆可。
// 題目 n <= 5*10^4，使用 int 比較而不做加減，因此 value 不會引入 signed overflow。
// 排序若因資源或 comparator 例外失敗就向上拋出，不回傳看似成功的部分結果。
// ----------------------------------------------------------------------------
[[nodiscard]] std::vector<int> sort_array(std::vector<int> numbers)
{
    const SortStatus status = parallel_merge_sort(
        numbers.begin(), numbers.end(), std::less<int>{}, 3U, 1'024U);
    if (status != SortStatus::completed) {
        throw std::runtime_error("sort_array was cancelled");
    }
    return numbers;
}

void leetcode_demo()
{
    const std::vector<int> first = sort_array({5, 2, 3, 1});
    const std::vector<int> repeated = sort_array({5, 1, 1, 2, 0, 0});
    const std::vector<int> empty = sort_array({});
    expect(first == std::vector<int>{1, 2, 3, 5}, "LeetCode case one failed");
    expect(repeated == std::vector<int>{0, 0, 1, 1, 2, 5},
           "LeetCode duplicate case failed");
    expect(empty.empty(), "LeetCode empty case failed");
}

// ----------------------------------------------------------------------------
// 實務：依 score 排序事件；相同 score 必須保留原 sequence，供重播與稽核。
// 若 stop 已提出，函式明確回 cancelled 且呼叫端丟棄部分結果，不把取消偽裝成成功。
// ----------------------------------------------------------------------------
struct Record {
    int score;
    int sequence;
    std::string name;
};

void practical_demo()
{
    std::vector<Record> records{
        {20, 0, "A"}, {10, 1, "B"}, {20, 2, "C"}, {10, 3, "D"}};
    const SortStatus status = parallel_merge_sort(
        records.begin(), records.end(),
        [](const Record& left, const Record& right) {
            return left.score < right.score;
        },
        2U, 2U);
    expect(status == SortStatus::completed, "record sort was cancelled");
    expect(records[0].name == "B" && records[1].name == "D",
           "stable order for score 10 was lost");
    expect(records[2].name == "A" && records[3].name == "C",
           "stable order for score 20 was lost");

    std::vector<int> cancelled_values{3, 2, 1};
    const std::vector<int> original = cancelled_values;
    std::stop_source stop_source;
    stop_source.request_stop();
    const SortStatus cancelled = parallel_merge_sort(
        cancelled_values.begin(), cancelled_values.end(), std::less<int>{},
        2U, 2U, stop_source.get_token());
    expect(cancelled == SortStatus::cancelled, "pre-cancelled sort reported success");
    expect(cancelled_values == original,
           "pre-cancelled sort changed input before its first safe point");
}

struct ThrowingLess {
    std::atomic<int>* calls;

    bool operator()(int left, int right) const
    {
        if (calls->fetch_add(1, std::memory_order_relaxed) >= 0) {
            throw std::runtime_error("comparison failed");
        }
        return left < right;
    }
};

void exception_demo()
{
    std::vector<int> values{4, 3, 2, 1};
    std::atomic<int> calls{0};
    bool propagated = false;
    try {
        static_cast<void>(parallel_merge_sort(
            values.begin(), values.end(), ThrowingLess{&calls}, 2U, 2U));
    } catch (const std::runtime_error&) {
        propagated = true;
    }
    expect(propagated, "comparator exception was not propagated");
    expect(calls.load(std::memory_order_relaxed) >= 1,
           "throwing comparator was never invoked");
}

int main()
{
    try {
        basic_demo();
        leetcode_demo();
        practical_demo();
        exception_demo();
        std::cout << "parallel mergesort：fork-join、stable、取消與例外測試通過\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "parallel mergesort 範例失敗：" << error.what() << '\n';
        return 1;
    }
}

// 【易錯】Compare 寫 left <= right 不是 strict weak ordering，標準演算法前置條件失效。
// 【易錯】省略 launch::async 可能得到 deferred future，程式正確但根本沒有平行執行。
// 【易錯】右分支拋例外就直接離開而不 get 左 future，會遺失 worker 例外與生命週期語意。
// 【面試追問】為何不能在 cancel 時 detach？iterator 指向呼叫者容器，函式返回後會懸空。
// 【面試追問】如何改善 allocation？預先配置 O(n) scratch buffer，按 disjoint offset 分給
// 子 task，並以 ping-pong source/destination merge，可固定 O(n log n) 時間但實作更複雜。
// 【練習】以硬體核心數推導 depth 上限，並 benchmark threshold、資料型別大小與 NUMA。
