// ============================================================================
// Sharded MPMC：分散資料鎖，並用 close/drain 定義完整生命週期
// ============================================================================
// 【問題】多 producer、多 consumer 共用一把 queue mutex 時，所有操作都在同一條
// cache line 排隊；sharding 讓不同 shard 的 deque 可以同時 push/pop，但它不是 lock-free。
// 【資料模型】每個 shard 擁有自己的 mutex 與 deque；元素從成功 push 到成功 pop 之間，
// ownership 屬於 queue。pop 把 int 複製到呼叫者後，該元素才離開 queue 的管理範圍。
// 【同步模型】shard mutex 保護 deque；state_mutex 保護 pending 與 wait predicate；
// lifecycle_mutex 讓多個 push 共享 ownership，而 close 取得 exclusive ownership。
// 【happens-before】producer 在 shard unlock 前完成插入；consumer 之後 lock 同一 shard，
// 因而看見元素。condition variable 只負責 progress，真正的資料可見性仍由 shard mutex 建立。
// 【無漏喚醒】pending 的修改與 wait predicate 都持有 state_mutex；producer 先發布 pending，
// 再 notify_one。這避免「檢查為空後、真正睡著前」的 lost wakeup。
// 【鎖順序】push 固定 lifecycle(shared) -> shard -> state；pop 是 shard -> state；
// close 是 lifecycle(exclusive) -> state。沒有任何路徑反向取得鎖，因此不形成 lock cycle。
// 【close 契約】close 與 push 線性化：已持 shared gate 的 push 會完成；close 返回後的新 push
// 一律回 false。consumer 會排空既有元素，只有 closed && pending==0 才從 wait_pop 回 false。
// 【取消】close 就是此 unbounded queue 的合作式取消/EOF；它喚醒全部 waiter，但不丟棄資料。
// 若要立即取消並丟棄 backlog，應另定 abort 契約，不能偷偷改變 close 的 drain 語意。
// 【生命週期】所有 producer 結束後由 owner 呼叫 close，再 join consumer，最後才析構 queue；
// 析構函式不替呼叫者 join，仍有 thread 存取物件時析構就是 lifetime bug。
// 【例外】deque::push_back 若配置失敗，元素與 pending 都不改；若發布 pending 失敗，
// push 在仍持 shard lock 時 pop_back 回滾。RAII 確保每條 throw 路徑都釋放鎖。
// 【overflow】ShardCount 以 static_assert 禁止 0，避免 modulo-by-zero；round-robin ticket 是
// unsigned atomic，回繞有定義且只影響起始 shard。pending 在遞增前顯式檢查 SIZE_MAX。
// 【複雜度】push 平均 O(1)；try_pop/wait_pop 每次最多掃 S 個 shard，為 O(S)；空間 O(n+S)。
// 【contention】state_mutex 只保護短計數與 predicate，但仍是全域熱點；shard 數太多會增加
// 掃描、mutex/cache footprint，hash 或 round-robin 偏斜也會形成 hot shard，必須量測。
// 【順序】單一 shard 內 FIFO；跨 shard 沒有 global FIFO，因為 consumer 的掃描起點不同。
// 【面試追問】這是否 linearizable MPMC FIFO？push/close 與元素 ownership 可定義線性化點，
// 但整體觀察順序不是單一 FIFO，所以不能宣稱符合全域 FIFO queue 契約。

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::logic_error(message);
    assert(condition); // 只重查既有 bool；queue 操作一律先在 assert 外執行。
}

template <std::size_t ShardCount>
class ShardedQueue {
    static_assert(ShardCount > 0U, "ShardedQueue needs at least one shard");

public:
    ShardedQueue() = default;
    ShardedQueue(const ShardedQueue&) = delete;
    ShardedQueue& operator=(const ShardedQueue&) = delete;

    // 成功回 true；close 已線性化完成則回 false，不接受 close 後的新工作。
    bool push(int value)
    {
        const std::shared_lock lifecycle_lock(lifecycle_mutex_);
        if (closed_) return false;

        const std::size_t ticket = next_push_.fetch_add(1U, std::memory_order_relaxed);
        Shard& shard = shards_[ticket % ShardCount];
        {
            const std::lock_guard shard_lock(shard.mutex);
            shard.values.push_back(value);
            try {
                const std::lock_guard state_lock(state_mutex_);
                if (pending_ == std::numeric_limits<std::size_t>::max()) {
                    throw std::overflow_error("queue size counter exhausted");
                }
                ++pending_;
            } catch (...) {
                shard.values.pop_back();
                throw;
            }
        }
        available_.notify_one();
        return true;
    }

    // 暫時沒有元素就回 false；這不代表 producer 未來不會再 push。
    bool try_pop(int& value)
    {
        std::size_t index = next_pop_.fetch_add(1U, std::memory_order_relaxed) % ShardCount;
        for (std::size_t visited = 0; visited < ShardCount; ++visited) {
            Shard& shard = shards_[index];
            const std::lock_guard shard_lock(shard.mutex);
            if (!shard.values.empty()) {
                const int next = shard.values.front();
                {
                    const std::lock_guard state_lock(state_mutex_);
                    if (pending_ == 0U) {
                        throw std::logic_error("queue pending invariant violated");
                    }
                    --pending_;
                }
                shard.values.pop_front();
                value = next;
                return true;
            }
            index = index + 1U == ShardCount ? 0U : index + 1U;
        }
        return false;
    }

    // 回 true 代表取得一筆；回 false 的唯一條件是 queue 已 close 且完全排空。
    bool wait_pop(int& value)
    {
        for (;;) {
            if (try_pop(value)) return true;

            std::unique_lock state_lock(state_mutex_);
            available_.wait(state_lock, [this] { return pending_ != 0U || closed_; });
            if (pending_ == 0U && closed_) return false;
        }
    }

    void close()
    {
        {
            const std::unique_lock lifecycle_lock(lifecycle_mutex_);
            const std::lock_guard state_lock(state_mutex_);
            if (closed_) return;
            closed_ = true;
        }
        available_.notify_all();
    }

    [[nodiscard]] std::size_t size() const
    {
        const std::lock_guard state_lock(state_mutex_);
        return pending_;
    }

    [[nodiscard]] bool is_closed() const
    {
        const std::lock_guard state_lock(state_mutex_);
        return closed_;
    }

private:
    struct Shard {
        std::mutex mutex;
        std::deque<int> values;
    };

    std::array<Shard, ShardCount> shards_;
    std::atomic<std::size_t> next_push_{0U};
    std::atomic<std::size_t> next_pop_{0U};

    // writer(close) 同時持 lifecycle_mutex_ 與 state_mutex_，所以 closed_ 可由
    // push 在 lifecycle lock 下讀、waiter 在 state lock 下讀，而不形成 data race。
    mutable std::shared_mutex lifecycle_mutex_;
    mutable std::mutex state_mutex_;
    std::condition_variable available_;
    std::size_t pending_{0U};
    bool closed_{false};
};

// raw std::thread 不能讓例外逃出 entry function，否則 std::terminate。
// relay 保存第一個 worker 例外；owner 在所有 join 完成後於原 thread 重拋。
class WorkerErrors {
public:
    void capture_current()
    {
        const std::exception_ptr current = std::current_exception();
        const std::lock_guard lock(mutex_);
        if (!first_) first_ = current;
    }

    void rethrow_if_any() const
    {
        std::exception_ptr error;
        {
            const std::lock_guard lock(mutex_);
            error = first_;
        }
        if (error) std::rethrow_exception(error);
    }

private:
    mutable std::mutex mutex_;
    std::exception_ptr first_;
};

void basic_demo()
{
    ShardedQueue<4> queue;
    const bool accepted_ten = queue.push(10);
    const bool accepted_twenty = queue.push(20);
    if (!accepted_ten || !accepted_twenty) {
        throw std::logic_error("open queue rejected a basic-demo item");
    }

    std::unordered_set<int> observed;
    int value = 0;
    while (queue.try_pop(value)) observed.insert(value);
    expect(observed == std::unordered_set<int>{10, 20}, "basic queue contents mismatch");

    queue.close();
    expect(queue.is_closed(), "queue did not close");
    const bool accepted_after_close = queue.push(30);
    expect(!accepted_after_close, "closed queue accepted a push");
    const bool popped_after_drain = queue.wait_pop(value);
    expect(!popped_after_drain, "closed and drained queue produced an item");
}

// ----------------------------------------------------------------------------
// LeetCode 217：Contains Duplicate。
// 題目契約是輸入唯讀、若任兩索引值相同即回 true；這裡以兩個 task 平行掃描。
// 每個 bucket 的 unordered_set 有獨立鎖，預期總時間 O(n)、最差 O(n^2)、空間 O(n)。
// duplicate 是早停訊號；cancelled 只處理例外，不能把配置失敗誤報成「有重複」。
// future.get 會傳遞 worker 例外，也保證函式返回前 worker 已結束、values 仍然存活。
// ----------------------------------------------------------------------------
template <std::size_t BucketCount>
class ShardedSet {
    static_assert(BucketCount > 0U, "ShardedSet needs at least one bucket");

public:
    bool insert(int value)
    {
        Bucket& bucket = buckets_[std::hash<int>{}(value) % BucketCount];
        const std::lock_guard lock(bucket.mutex);
        return bucket.values.insert(value).second;
    }

private:
    struct Bucket {
        std::mutex mutex;
        std::unordered_set<int> values;
    };
    std::array<Bucket, BucketCount> buckets_;
};

[[nodiscard]] bool contains_duplicate(const std::vector<int>& values)
{
    ShardedSet<8> seen;
    std::atomic<bool> duplicate{false};
    std::atomic<bool> cancelled{false};

    auto scan = [&](std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            if (duplicate.load(std::memory_order_acquire) ||
                cancelled.load(std::memory_order_acquire)) {
                return;
            }
            if (!seen.insert(values[index])) {
                duplicate.store(true, std::memory_order_release);
                return;
            }
        }
    };

    const std::size_t middle = values.size() / 2U;
    auto left = std::async(std::launch::async, [&] {
        try {
            scan(0U, middle);
        } catch (...) {
            cancelled.store(true, std::memory_order_release);
            throw;
        }
    });

    std::exception_ptr right_error;
    try {
        scan(middle, values.size());
    } catch (...) {
        cancelled.store(true, std::memory_order_release);
        right_error = std::current_exception();
    }

    std::exception_ptr left_error;
    try {
        left.get();
    } catch (...) {
        left_error = std::current_exception();
    }
    if (right_error) std::rethrow_exception(right_error);
    if (left_error) std::rethrow_exception(left_error);
    return duplicate.load(std::memory_order_acquire);
}

void leetcode_demo()
{
    const bool repeated = contains_duplicate({1, 2, 3, 1});
    const bool unique = contains_duplicate({1, 2, 3, 4});
    const bool repeated_negative = contains_duplicate({-1, 0, -1});
    const bool empty = contains_duplicate({});
    expect(repeated, "contains_duplicate missed a repeated value");
    expect(!unique, "contains_duplicate reported a false duplicate");
    expect(repeated_negative, "contains_duplicate missed a negative value");
    expect(!empty, "contains_duplicate mishandled empty input");
}

// ----------------------------------------------------------------------------
// 實務：producer 與 consumer 真正重疊執行；最後驗證每個 job exactly once。
// owner 只在所有 producer join 後 close，因此 accepted job 不會被取消或遺失。
// 每個 consumer 只寫自己的 vector；join 後 main 才合併，沒有共享 vector 的 data race。
// ----------------------------------------------------------------------------
void practical_demo()
{
    constexpr int producer_count = 4;
    constexpr int consumer_count = 3;
    constexpr int jobs_per_producer = 250;
    constexpr int total_jobs = producer_count * jobs_per_producer;

    ShardedQueue<8> queue;
    WorkerErrors worker_errors;
    std::array<std::vector<int>, static_cast<std::size_t>(consumer_count)> outputs;
    std::vector<std::thread> consumers;
    consumers.reserve(static_cast<std::size_t>(consumer_count));
    for (int consumer = 0; consumer < consumer_count; ++consumer) {
        consumers.emplace_back([&, consumer] {
            try {
                int job = 0;
                auto& output = outputs[static_cast<std::size_t>(consumer)];
                while (queue.wait_pop(job)) output.push_back(job);
            } catch (...) {
                worker_errors.capture_current();
                queue.close();
            }
        });
    }

    std::atomic<bool> all_pushes_accepted{true};
    std::vector<std::thread> producers;
    producers.reserve(static_cast<std::size_t>(producer_count));
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&, producer] {
            try {
                const int first = producer * jobs_per_producer;
                const int last = first + jobs_per_producer;
                for (int job = first; job < last; ++job) {
                    if (!queue.push(job)) {
                        all_pushes_accepted.store(false, std::memory_order_relaxed);
                        return;
                    }
                }
            } catch (...) {
                worker_errors.capture_current();
                queue.close();
            }
        });
    }

    for (std::thread& producer : producers) producer.join();
    queue.close();
    for (std::thread& consumer : consumers) consumer.join();
    worker_errors.rethrow_if_any();

    std::array<unsigned int, static_cast<std::size_t>(total_jobs)> counts{};
    bool values_in_range = true;
    std::size_t consumed = 0U;
    for (const auto& output : outputs) {
        consumed += output.size();
        for (int job : output) {
            if (job < 0 || job >= total_jobs) {
                values_in_range = false;
            } else {
                ++counts[static_cast<std::size_t>(job)];
            }
        }
    }

    expect(all_pushes_accepted.load(std::memory_order_relaxed),
           "open practical queue rejected a job");
    expect(values_in_range, "consumer observed an out-of-range job");
    expect(consumed == static_cast<std::size_t>(total_jobs),
           "practical queue lost or duplicated a job");
    const bool exactly_once =
        std::all_of(counts.begin(), counts.end(), [](unsigned int count) {
            return count == 1U;
        });
    expect(exactly_once, "practical queue violated exactly-once delivery");
    expect(queue.size() == 0U, "practical queue was not fully drained");
}

// close 與 push 刻意競跑：只把 push 回 true 的 job 納入承諾，最後逐筆核對。
// 這直接測試 lifecycle gate 的線性化邊界，而不是靠 sleep 猜排程先後。
void close_race_demo()
{
    constexpr int producer_count = 4;
    constexpr int attempts_per_producer = 2'000;
    ShardedQueue<4> queue;
    WorkerErrors worker_errors;
    std::array<std::vector<int>, static_cast<std::size_t>(producer_count)> accepted;
    std::atomic<int> producers_started{0};

    std::vector<std::thread> producers;
    producers.reserve(static_cast<std::size_t>(producer_count));
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&, producer] {
            bool announced = false;
            try {
                auto& own = accepted[static_cast<std::size_t>(producer)];
                const int first = producer * attempts_per_producer;

                if (queue.push(first)) own.push_back(first);
                producers_started.fetch_add(1, std::memory_order_release);
                announced = true;
                for (int offset = 1; offset < attempts_per_producer; ++offset) {
                    const int job = first + offset;
                    if (!queue.push(job)) return;
                    own.push_back(job);
                }
            } catch (...) {
                if (!announced) {
                    producers_started.fetch_add(1, std::memory_order_release);
                }
                worker_errors.capture_current();
                queue.close();
            }
        });
    }

    std::thread closer([&] {
        while (producers_started.load(std::memory_order_acquire) != producer_count) {
            std::this_thread::yield();
        }
        try {
            queue.close();
        } catch (...) {
            worker_errors.capture_current();
        }
    });

    for (std::thread& producer : producers) producer.join();
    closer.join();
    worker_errors.rethrow_if_any();

    std::vector<int> expected;
    for (const auto& own : accepted) {
        expected.insert(expected.end(), own.begin(), own.end());
    }
    std::vector<int> observed;
    int job = 0;
    while (queue.wait_pop(job)) observed.push_back(job);
    std::sort(expected.begin(), expected.end());
    std::sort(observed.begin(), observed.end());
    expect(observed == expected, "close race lost or invented an accepted job");
    expect(queue.is_closed(), "close race left queue open");
    expect(queue.size() == 0U, "close race queue was not fully drained");
}

int main()
{
    try {
        basic_demo();
        leetcode_demo();
        practical_demo();
        close_race_demo();
        std::cout << "sharded MPMC：close/drain、例外傳遞與 exactly-once 測試通過\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "sharded MPMC 範例失敗：" << error.what() << '\n';
        return 1;
    }
}

// 【易錯】用 atomic pending 配 condition_variable，卻不在同一 state mutex 下改 predicate，
// 仍可能漏通知；atomic 本身不會自動補上 condition-variable protocol。
// 【易錯】close 後立即讓 consumer 返回會遺失 backlog；必須明訂 drain 或 abort 二選一。
// 【易錯】同時鎖多個 shard 做全域 snapshot 時，要用固定順序或 scoped_lock 防 deadlock。
// 【面試追問】怎麼做 bounded queue？增加容量、not_full predicate 與 producer cancellation，
// 且 close 必須同時喚醒 blocked producer/consumer；不能只在 push 前讀一次 size。
// 【練習】加入 per-shard metrics，量測 1/2/4/8/16 shards 的吞吐與 p99 latency。
