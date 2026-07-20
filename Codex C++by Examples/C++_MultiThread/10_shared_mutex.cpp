// ============================================================================
// shared_mutex：讀多寫少時，以一把鎖保護「整體不變式」
// ============================================================================
// 【問題】普通 mutex 讓所有讀者互斥；shared_mutex 允許多個讀者同時持有
// shared ownership，但寫者必須取得 exclusive ownership，並等待所有讀者離開。
// 【同步模型】寫者 unlock 後，下一個成功取得該 mutex 的讀者或寫者會看見先前寫入；
// mutex 的同步關係建立 happens-before，不可用「CPU 應該會刷新 cache」來解釋。
// 【讀者關係】兩個 shared_lock 彼此不排斥，也不需要互相同步，因為它們只能讀取
// 鎖所保護的狀態；任何一條漏鎖的讀或寫路徑仍會造成 data race 與 undefined behavior。
// 【ownership】mutex 與資料放在同一物件，由成員函式集中執行鎖規則；不得把容器內的
// reference、pointer 或 iterator 帶出鎖的生命週期，否則 writer 可能令它失效。
// 【鎖粒度】先在鎖外建立輸入，再縮短 unique lock 區段；但檢查與提交若共同維護
// invariant，就必須留在同一個 exclusive critical section，不能拆成 check-then-act。
// 【複雜度】下例以 std::map 儲存，單筆查找/更新為 O(log n)，完整 snapshot 為 O(n)；
// lock/unlock 的標準複雜度未規定，實際成本受作業系統、讀寫比例與 cache line 爭用影響。
// 【contention】讀多不代表一定較快：shared_mutex 要維護 reader 狀態，短小讀操作可能
// 比 mutex 或 immutable snapshot 慢；寫者也可能被長讀區段延遲，公平性不受標準保證。
// 【生命週期】物件必須活得比所有使用它的 thread 久；main 先 join，再讓物件析構。
// shared_mutex 析構時仍被持有或仍有 waiter，程式行為不符合 mutex 的生命週期契約。
// 【例外】RAII lock 在 map 配置、字串複製或檢查失敗時仍會解鎖；查詢回傳 value，
// 因此複製若拋例外也不會留下懸空 reference。publish 先收 by-value，再原子交換版本。
// 【取消】這些操作都很短，不在持鎖期間等待 I/O 或輪詢 stop token；系統關閉時應先
// 停止產生新工作、join worker，再銷毀 registry，而不是強制中止持鎖執行緒。
// 【不可升級】同一 thread 先持 shared_lock 再 lock exclusive 不是可攜的 upgrade，
// 可能自我 deadlock；應釋放後重鎖並重新驗證條件，或改用支援 upgrade 的其他設計。
// 【不可遞迴】std::shared_mutex 不是 recursive mutex；同一 thread 不可重複取得 ownership。
// 【面試追問】何時改用 copy-on-write/RCU？讀極多、snapshot 可接受、writer 能負擔複製，
// 且 benchmark 證明 reader-side lock 已成瓶頸時；不能只憑「90% read」做結論。

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::logic_error(message);
    assert(condition); // 只重查既有 bool；必要操作不放在 assert 內。
}

class ConcurrentMap {
public:
    void set(std::string key, int value)
    {
        // key 已在鎖外完成複製/移動；只有 map 的提交動作需要獨占。
        const std::unique_lock lock(mutex_);
        values_.insert_or_assign(std::move(key), value);
    }

    [[nodiscard]] std::optional<int> find(const std::string& key) const
    {
        const std::shared_lock lock(mutex_);
        const auto found = values_.find(key);
        if (found == values_.end()) return std::nullopt;
        return found->second; // 回傳 value，不讓 iterator 逃出 critical section。
    }

    [[nodiscard]] std::map<std::string, int> snapshot() const
    {
        const std::shared_lock lock(mutex_);
        return values_; // 複製期間 writer 不能改 map；複製失敗時 RAII 仍會解鎖。
    }

private:
    mutable std::shared_mutex mutex_;
    std::map<std::string, int> values_;
};

void basic_demo()
{
    ConcurrentMap values;
    values.set("answer", 42);

    auto reader_a = std::async(std::launch::async, [&] {
        return values.find("answer");
    });
    auto reader_b = std::async(std::launch::async, [&] {
        return values.find("answer");
    });
    const std::optional<int> first = reader_a.get();
    const std::optional<int> second = reader_b.get();

    expect(first == 42, "first reader observed a wrong value");
    expect(second == 42, "second reader observed a wrong value");
    expect(!values.find("missing").has_value(), "missing key unexpectedly exists");
    expect(values.snapshot().size() == 1U, "ConcurrentMap snapshot size mismatch");
}

// ----------------------------------------------------------------------------
// LeetCode 981：Time Based Key-Value Store 的完整多 key 契約
// set(key, value, timestamp)：同一 key 的 timestamp 必須嚴格遞增且大於 0。
// get(key, t)：回傳 timestamp <= t 的最新 value；找不到時依題意回傳空字串。
// 外層 map 查 key 為 O(log k)，內層 vector 二分搜尋為 O(log m)，set 攤銷 O(log k)。
// ----------------------------------------------------------------------------
class TimeMap {
public:
    void set(std::string key, std::string value, int timestamp)
    {
        if (timestamp <= 0) {
            throw std::invalid_argument("timestamp must be positive");
        }

        const std::unique_lock lock(mutex_);
        auto& entries = history_[std::move(key)];
        if (!entries.empty() && timestamp <= entries.back().first) {
            throw std::invalid_argument("timestamps for one key must increase");
        }
        entries.emplace_back(timestamp, std::move(value));
    }

    [[nodiscard]] std::string get(const std::string& key, int timestamp) const
    {
        const std::shared_lock lock(mutex_);
        const auto found = history_.find(key);
        if (found == history_.end()) return {};

        const auto& entries = found->second;
        const auto after = std::upper_bound(
            entries.begin(), entries.end(), timestamp,
            [](int target, const Entry& entry) { return target < entry.first; });
        return after == entries.begin() ? std::string{} : std::prev(after)->second;
    }

private:
    using Entry = std::pair<int, std::string>;
    mutable std::shared_mutex mutex_;
    std::map<std::string, std::vector<Entry>> history_;
};

void leetcode_demo()
{
    TimeMap store;
    store.set("foo", "bar", 1);
    expect(store.get("foo", 1) == "bar", "TimeMap exact lookup failed");
    expect(store.get("foo", 3) == "bar", "TimeMap floor lookup failed");
    store.set("foo", "bar2", 4);
    store.set("language", "C++20", 2);
    expect(store.get("foo", 4) == "bar2", "TimeMap replacement lookup failed");
    expect(store.get("foo", 5) == "bar2", "TimeMap latest lookup failed");
    expect(store.get("language", 1).empty(), "TimeMap accepted a future value");
    expect(store.get("missing", 99).empty(), "TimeMap missing-key contract failed");

    bool rejected_out_of_order = false;
    try {
        store.set("foo", "stale", 4);
    } catch (const std::invalid_argument&) {
        rejected_out_of_order = true;
    }
    expect(rejected_out_of_order, "TimeMap accepted an out-of-order timestamp");
    expect(store.get("foo", 4) == "bar2", "rejected TimeMap write changed state");
}

// ----------------------------------------------------------------------------
// 實務：功能旗標以「版本 + 全部欄位」一起發布，reader 取得一致 snapshot。
// 若逐欄各鎖一次，reader 可能看到跨版本組合；shared_mutex 保護的是整體 invariant。
// version 到 UINT64_MAX 時拒絕發布，避免無號整數回繞讓監控誤認為舊版本。
// ----------------------------------------------------------------------------
struct FlagSnapshot {
    std::uint64_t version;
    std::map<std::string, bool> flags;

    [[nodiscard]] bool enabled(const std::string& name) const
    {
        const auto found = flags.find(name);
        return found != flags.end() && found->second;
    }
};

class FeatureFlags {
public:
    explicit FeatureFlags(std::uint64_t initial_version = 0U)
        : version_(initial_version)
    {
    }

    void publish(std::map<std::string, bool> next)
    {
        const std::unique_lock lock(mutex_);
        if (version_ == std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("feature flag version exhausted");
        }
        flags_.swap(next); // 標準 allocator/comparator 下不拋例外，兩欄位一起提交。
        ++version_;
    }

    [[nodiscard]] FlagSnapshot snapshot() const
    {
        const std::shared_lock lock(mutex_);
        return FlagSnapshot{version_, flags_};
    }

private:
    mutable std::shared_mutex mutex_;
    std::uint64_t version_;
    std::map<std::string, bool> flags_;
};

void practical_demo()
{
    FeatureFlags registry;
    registry.publish({{"fraud-check", false}, {"new-checkout", false}});

    auto read_snapshots = [&] {
        bool invariant_ok = true;
        for (int sample = 0; sample < 5'000; ++sample) {
            const FlagSnapshot current = registry.snapshot();
            if (current.enabled("new-checkout") != current.enabled("fraud-check")) {
                invariant_ok = false;
            }
        }
        return invariant_ok;
    };

    auto reader_a = std::async(std::launch::async, read_snapshots);
    auto reader_b = std::async(std::launch::async, read_snapshots);

    std::exception_ptr writer_error;
    try {
        for (int generation = 0; generation < 500; ++generation) {
            const bool enabled = generation % 2 == 0;
            registry.publish({{"fraud-check", enabled}, {"new-checkout", enabled}});
        }
    } catch (...) {
        writer_error = std::current_exception();
    }

    bool reader_a_ok = false;
    bool reader_b_ok = false;
    std::exception_ptr reader_error;
    try {
        reader_a_ok = reader_a.get();
    } catch (...) {
        reader_error = std::current_exception();
    }
    try {
        reader_b_ok = reader_b.get();
    } catch (...) {
        if (!reader_error) reader_error = std::current_exception();
    }
    if (writer_error) std::rethrow_exception(writer_error);
    if (reader_error) std::rethrow_exception(reader_error);

    expect(reader_a_ok && reader_b_ok, "reader observed a mixed flag generation");
    expect(registry.snapshot().version == 501U, "feature flag version mismatch");

    FeatureFlags exhausted(std::numeric_limits<std::uint64_t>::max());
    bool overflow_rejected = false;
    try {
        exhausted.publish({{"must-not-commit", true}});
    } catch (const std::overflow_error&) {
        overflow_rejected = true;
    }
    expect(overflow_rejected, "feature flag version overflow was not rejected");
    expect(exhausted.snapshot().flags.empty(), "overflowing publish changed flags");
}

int main()
{
    try {
        basic_demo();
        leetcode_demo();
        practical_demo();
        std::cout << "shared_mutex：同步、TimeMap 與一致快照測試通過\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "shared_mutex 範例失敗：" << error.what() << '\n';
        return 1;
    }
}

// 【易錯】不要在持鎖時呼叫未知 callback；callback 可能重入同物件而自我 deadlock。
// 【易錯】先 shared 查不存在、解鎖、再 exclusive 插入時，必須在 exclusive lock 下重查。
// 【面試追問】shared_mutex 一定避免 writer starvation 嗎？否，標準沒有公平性保證。
// 【面試追問】為何 snapshot 回傳副本？因為鎖釋放後，內部 reference 不再受保護。
// 【練習】量測 50/90/99% 讀比例與不同 critical-section 長度，再和 mutex/CoW 比較。
