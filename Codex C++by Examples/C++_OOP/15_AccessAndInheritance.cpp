// ============================================================================
// 課題 15：public/protected/private inheritance 與 access control
// ============================================================================
//
// public inheritance 保留 Base public 為 public，表達 is-a。protected inheritance 將
// Base public/protected 變 protected；private inheritance 將它們變 private，對外不再
// 是 Base。後兩者較少用，通常 composition 更清楚。
//
// Base 的 private member 無論哪種 inheritance 都不能由 Derived 直接碰；protected
// member 可碰，但會讓所有 derived classes 綁死內部 representation。較穩定的設計是
// private data + protected behavior。
//
// 【常見選擇】介面/多型通常 public inheritance；implementation reuse 通常 composition。
// 【陷阱】忘記寫 `public`：class 的 base access 預設 private，struct 預設 public。
// ============================================================================

#include <cassert>
#include <deque>
#include <iostream>
#include <string>

class RequestTracker {
public:
    int request_count() const { return request_count_; }

protected:
    void record_request() { ++request_count_; } // 提供 behavior，不暴露 writable data。

private:
    int request_count_ = 0;
};

class HealthService : public RequestTracker {
public:
    std::string check()
    {
        record_request();
        return "ok";
    }
};

void basic_example()
{
    HealthService service;
    assert(service.check() == "ok");
    assert(service.request_count() == 1);
    std::cout << "[基礎] public inheritance 保留 request_count API\n";
}

// LeetCode 933：Number of Recent Calls。
// TimestampWindow 是 implementation helper，RecentCounter 不應對外宣稱「is-a window」，
// 因此用 private inheritance 示範；production code 多半改 composition 更直白。
class TimestampWindow {
protected:
    void push_timestamp(int time) { times_.push_back(time); }
    void discard_before(int lower_bound)
    {
        while (!times_.empty() && times_.front() < lower_bound) {
            times_.pop_front();
        }
    }
    std::size_t timestamp_count() const { return times_.size(); }

private:
    std::deque<int> times_;
};

class RecentCounter : private TimestampWindow {
public:
    int ping(int time)
    {
        push_timestamp(time);
        discard_before(time - 3'000);
        return static_cast<int>(timestamp_count());
    }
};

void leetcode_933_example()
{
    RecentCounter counter;
    const int at_1 = counter.ping(1);
    const int at_100 = counter.ping(100);
    const int at_3_001 = counter.ping(3'001);
    const int at_3_002 = counter.ping(3'002);
    assert(at_1 == 1);
    assert(at_100 == 2);
    assert(at_3_001 == 3);
    assert(at_3_002 == 3);
    std::cout << "[LeetCode 933] private base helper 維護 3000ms window\n";
}

// 實務案例：composition 版本通常更清楚，JobQueue「has-a」Metrics，而非 is-a。
class Metrics {
public:
    void increment() { ++jobs_; }
    int jobs() const { return jobs_; }

private:
    int jobs_ = 0;
};

class JobQueue {
public:
    void submit() { metrics_.increment(); }
    int submitted_jobs() const { return metrics_.jobs(); }

private:
    Metrics metrics_; // composition 明白表達 ownership。
};

void practical_example()
{
    JobQueue queue;
    queue.submit();
    queue.submit();
    assert(queue.submitted_jobs() == 2);
    std::cout << "[實務] composition: JobQueue has Metrics\n";
}

int main()
{
    basic_example();
    leetcode_933_example();
    practical_example();
}

// 練習：把 RecentCounter 改成 composition，比較 public header 的可讀性。
// 複雜度：public/protected/private inheritance 是 compile-time 可見性規則，沒有 runtime 成本。
// 生命週期：base subobject 嵌在 derived object 內，不能比 complete derived object 單獨活得更久。
