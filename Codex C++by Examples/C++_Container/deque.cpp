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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 933. Number of Recent Calls（最近的請求次數）
// 題目：每次 ping(t) 回傳 [t-3000,t] 內請求數，且 t 嚴格遞增；例如 1,100,3001,3002 得 1,2,3,3。
// 為何使用本章主題：deque 可在尾端加入新時間，並從前端 O(1) 移除已過期的最舊時間。
// 思路：新時間 push_back；計算窗口下界；反覆 pop_front 過期值；回傳目前容器大小。
// 複雜度：N 次呼叫總時間 O(N)、單次攤銷 O(1)，額外空間 O(W)，W 為窗口內請求數。
// 易錯點：解法依賴時間戳遞增；下界包含 t-3000；空 deque 不可讀 front，時間減法也要防型別溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】具緊急插隊能力的工作派送佇列
// 情境：一般編譯與測試工作依到達順序排隊，但安全修補工作必須插到下一個處理位置。
// 為何使用本章主題：deque 同時提供前後端常數時間插入；vector 前端插入需搬移，queue 又不開放 push_front。
// 設計：一般工作 push_back；緊急工作 push_front；take 移出 front 後再 pop_front。
// 成本：提交與取件皆為攤銷 O(1)，空間 O(J)，J 為尚未處理的工作數。
// 上線注意：空佇列不能只靠 assert 防護；多執行緒需同步與關閉協定，緊急工作也需避免餓死一般工作。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：deque 不是「永遠不失效的 vector」】
 * - take/front 要求非空；assert 不可作為 release 的輸入驗證。
 * - 端點與中間 insert/erase 對 iterator/reference/past-the-end 的規則不同，寫 API 前要逐操作查表。
 * - RecentCounter 依賴 timestamp 單調遞增；若資料可亂序，deque front 不再代表最舊可淘汰事件。
 * - `timestamp-3000` 對極小整數可能溢位；時間 domain/型別與範圍要先定義。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'deque.cpp' -o '/tmp/codex_cpp_C_Container_deque' && '/tmp/codex_cpp_C_Container_deque'
//
// === 預期輸出（節錄）===
// deque：兩端操作、滑動窗口與工作佇列測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
