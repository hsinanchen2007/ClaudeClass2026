// ============================================================================
// deque：兩端常數時間操作的分段序列
// ============================================================================
// deque（double-ended queue）通常由多個固定大小區塊與索引表組成，不保證整體
// 連續。因此有 O(1) operator[]，卻不能假設 &d[i] + 1 == &d[i+1]，也沒有可把
// 全部元素當單一 buffer 的 data()。它的核心優勢是 push/pop_front 與
// push/pop_back 都是 O(1) 攤銷。
//
// 中間 insert/erase 仍為 O(n)。iterator 失效規則比 vector 複雜，對兩端插入時
// reference 通常較穩定但 iterator 可能失效；可攜程式應在修改後重新取得 iterator。

#include <cassert>
#include <cstddef>
#include <deque>
#include <iostream>
#include <string>

void basic_demo()
{
    std::deque<int> tasks{20, 30};
    tasks.push_front(10);
    tasks.push_back(40);
    assert(tasks.front() == 10 && tasks.back() == 40);
    assert(tasks.at(2) == 30);

    tasks.pop_front();
    tasks.pop_back();
    assert((tasks == std::deque<int>{20, 30}));
}

// ----------------------------------------------------------------------------
// LeetCode 933：Number of Recent Calls
// ----------------------------------------------------------------------------
// timestamp 單調增加，所以過期資料只會出現在前端。每筆 timestamp 恰好進出
// deque 各一次，n 次呼叫總時間 O(n)，單次攤銷 O(1)，空間 O(window)。
class RecentCounter {
public:
    std::size_t ping(int timestamp)
    {
        timestamps_.push_back(timestamp);
        const int oldest_allowed = timestamp - 3'000;
        while (!timestamps_.empty() && timestamps_.front() < oldest_allowed) {
            timestamps_.pop_front();
        }
        return timestamps_.size();
    }

private:
    std::deque<int> timestamps_;
};

void leetcode_demo()
{
    RecentCounter counter;
    const std::size_t at_1 = counter.ping(1);
    const std::size_t at_100 = counter.ping(100);
    const std::size_t at_3_001 = counter.ping(3'001);
    const std::size_t at_3_002 = counter.ping(3'002);
    assert(at_1 == 1U);
    assert(at_100 == 2U);
    assert(at_3_001 == 3U);
    assert(at_3_002 == 3U);  // 1 已落在窗口之外。
}

// ----------------------------------------------------------------------------
// 實務：一般工作排尾端，緊急工作排前端
// ----------------------------------------------------------------------------
class DispatchQueue {
public:
    void submit(std::string job) { jobs_.push_back(std::move(job)); }
    void submit_urgent(std::string job) { jobs_.push_front(std::move(job)); }

    std::string take()
    {
        assert(!jobs_.empty());
        std::string job = std::move(jobs_.front());
        jobs_.pop_front();
        return job;
    }

private:
    std::deque<std::string> jobs_;
};

void practical_demo()
{
    DispatchQueue queue;
    queue.submit("compile");
    queue.submit("test");
    queue.submit_urgent("security-patch");
    assert(queue.take() == "security-patch");
    assert(queue.take() == "compile");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "deque：兩端操作、滑動窗口與工作佇列測試通過\n";
}

// 【陷阱】deque 不是連續記憶體；需要 C API buffer、SIMD 連續掃描時通常選 vector。
// 【陷阱】對空 deque 呼叫 front/back/pop 是 UB，先檢查 empty。
// 【面試】為何 std::queue 預設底層是 deque？因為需要 O(1) back push/front pop。
// 【練習】用 deque 實作固定容量 ring-like buffer，滿時先 pop_front。
