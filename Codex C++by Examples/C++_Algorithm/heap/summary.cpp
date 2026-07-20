/*
 * Heap 演算法家族：面試前完整速查章
 * ==================================
 *
 * API              前置狀態                         結果/複雜度
 * make_heap        任意 random-access 範圍          建 heap，O(N)
 * push_heap        [first,last-1) 是 heap           尾元素入堆，O(log N)
 * pop_heap         [first,last) 是非空 heap         top 移到 last-1，O(log N)
 * sort_heap        [first,last) 是 heap             排序並破壞 heap，O(N log N)
 * is_heap          任意範圍                         驗證，O(N)
 * is_heap_until    任意範圍                         首個違規位置，O(N)
 *
 * 預設 less<> => max-heap，front 最大；greater<> => min-heap，front 最小。
 * 所有操作必須使用同一 comparator，且 comparator 要是 strict weak ordering。
 * heap 不是排序陣列，只保證 parent 優先於 child。演算法不改 vector 大小：
 * push = push_back + push_heap；pop = pop_heap + 讀/move back + pop_back。
 *
 * 選型：
 * - 動態反覆取得極值：heap / priority_queue。
 * - 一次找第 k 大且可改輸入：nth_element 平均 O(N) 通常更好。
 * - 串流 top-k：大小 k 的反向 heap，O(N log k)、O(k) 空間。
 * - 需要任意元素刪除/更新：標準 heap 無 handle，可能需 indexed heap 或 set。
 */

/*
==============================================================================
【面試深挖：Heap 與 Priority Queue】

A1｜為何 heap 可用連續陣列而不需要 child pointer？
答：binary heap 是 complete binary tree，level-order 中間沒有洞。0-based index 的
left=2i+1、right=2i+2、parent=(i-1)/2；因此省指標且 locality 好。

A2｜為何 `make_heap` 是 O(n)，不是 n 次插入的 O(n log n)？
答：bottom-up heapify 從最後一個內部節點向上 sift-down。高度高的節點很少；
各高度工作量加權後是線性。若逐項 push_heap，才是 O(n log n)。

A3｜`pop_heap` 做了什麼，為何容器大小沒變？
答：它交換 root 與最後元素，再修復 [first,last-1) 的 heap；被取出的最大值留在尾端。
呼叫者仍需 `pop_back` 才真正刪除。只呼叫 pop_heap 會讓人誤以為元素已消失。

A4｜`priority_queue<T, ..., Compare>` 的 Compare 為何看起來「反向」？
答：top 是依 Compare 排序後最後的元素；預設 less 形成 max-heap，greater 形成 min-heap。
不要把 comparator 解讀成「誰優先」，而要解讀成 strict weak ordering。

A5｜heap 與 ordered set 怎麼選？
答：只反覆取極值且 push/pop 時用 heap，常數與 locality 通常更好；需要任意 key 查找、
erase、lower_bound 或有序遍歷時用 tree。priority_queue 不提供任意元素 iterator/erase。

A6｜Dijkstra 為何常把同一節點多次放進 priority_queue？
答：標準 priority_queue 沒有 decrease-key。常見作法是推入新版距離，pop 時若不是目前
最短值就丟棄，稱 lazy deletion。正確性靠距離檢查，不靠 queue 內沒有舊項。

A7｜heap 是 stable 嗎？相同 priority 如何固定先後？
答：標準 heap 不保證穩定。把 monotonically increasing sequence number 加進 key，
comparator 先比 priority、再比 sequence，才可明確實作 FIFO tie-break。

A8｜heap algorithms 要求哪種 iterator？
答：需要 LegacyRandomAccessIterator，因為要用 index distance 跳父子。這也是 list
不能直接套 make_heap 的原因；「可以線性走到孩子」不代表滿足演算法契約。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct Task {
    int priority;
    std::size_t sequence;
    std::string name;
};

// priority 大者先；同 priority 讓 sequence 小（較早到）者先。
struct LowerPriority {
    bool operator()(const Task& lhs, const Task& rhs) const {
        if (lhs.priority != rhs.priority) {
            return lhs.priority < rhs.priority;
        }
        return lhs.sequence > rhs.sequence;
    }
};

class Scheduler {
public:
    void submit(int priority, std::string name) {
        tasks_.push_back(Task{priority, next_sequence_++, std::move(name)});
        std::push_heap(tasks_.begin(), tasks_.end(), LowerPriority{});
    }

    // 空佇列是正常執行期狀態，以 nullopt 回報；assert 不能當 API 前置條件檢查。
    std::optional<std::string> dispatch() {
        if (tasks_.empty()) {
            return std::nullopt;
        }
        std::pop_heap(tasks_.begin(), tasks_.end(), LowerPriority{});
        std::string name = std::move(tasks_.back().name);
        tasks_.pop_back();
        return name;
    }

    bool valid() const {
        return std::is_heap(tasks_.begin(), tasks_.end(), LowerPriority{});
    }

private:
    std::vector<Task> tasks_;
    std::size_t next_sequence_{0};
};

// LeetCode 1046 的核心操作，用來複習 push/pop 成對規則。
int leetcode_smash(std::vector<int> stones) {
    std::make_heap(stones.begin(), stones.end());
    while (stones.size() >= 2U) {
        std::pop_heap(stones.begin(), stones.end());
        const int y = stones.back();
        stones.pop_back();
        std::pop_heap(stones.begin(), stones.end());
        const int x = stones.back();
        stones.pop_back();
        if (y > x) {
            stones.push_back(y - x);
            std::push_heap(stones.begin(), stones.end());
        }
    }
    return stones.empty() ? 0 : stones.front();
}

std::vector<std::string> practical_dispatch_all() {
    Scheduler scheduler;
    scheduler.submit(5, "normal-A");
    scheduler.submit(10, "urgent");
    scheduler.submit(5, "normal-B");
    assert(scheduler.valid());

    std::vector<std::string> dispatched;
    while (auto name = scheduler.dispatch()) {
        dispatched.push_back(std::move(*name));
    }
    assert(scheduler.valid());  // 空範圍也是合法 heap。
    return dispatched;
}

int main() {
    assert((practical_dispatch_all() ==
            std::vector<std::string>{"urgent", "normal-A", "normal-B"}));

    assert(leetcode_smash({2, 7, 4, 1, 8, 1}) == 1);

    Scheduler empty_scheduler;
    assert(!empty_scheduler.dispatch().has_value());
    assert(empty_scheduler.valid());
    std::cout << "Heap 家族整合複習完成\n";
}

/*
 * 面試 Q&A
 * Q: make_heap 為何 O(N) 而不是 O(N log N)？
 * A: bottom-up sift-down；大量葉端節點高度很低，加權總和為線性。
 * Q: pop_heap 為何不刪元素？
 * A: iterator 演算法不能改未知容器的 size，且讓呼叫者有機會先取出尾值。
 * Q: priority_queue 與 heap 演算法怎麼選？
 * A: 一般 queue 用前者封裝較安全；需批次 make/sort、直接存取底層時用後者。
 * Q: 相同 priority 是否 FIFO？
 * A: heap 不穩定；需像本例把 sequence 納入 comparator。
 * Q: 空 heap 可以 pop_heap/front 嗎？
 * A: 不可。公開 API 應先檢查並回 optional/error（或丟明定例外）；assert 只適合
 *    偵錯 invariant，NDEBUG 下會消失，不能負責不可信輸入。
 *
 * 練習：
 * 1. 實作固定容量 top-k latency；2. 加入 cancel(task_id) 並分析為何 O(N)；
 * 3. 比較 nth_element、partial_sort、大小 k heap 在 N 很大、k 很小時的成本。
 *
 * 【選擇決策表】
 * - 每次只看最大/最小：priority_queue 或 heap。
 * - 要同時快速查最小與最大：單一 binary heap 不夠，可考慮 multiset。
 * - 要第 k 大一次：nth_element；要前 k 名排序：partial_sort。
 * - 要串流第 k 大：大小 k 的 min-heap。
 * - priority 更新：std heap 沒有 decrease-key handle，考慮重複 push + lazy delete。
 *
 * 【複雜度推導】
 * heap 高度 floor(log2 N)，push 上浮與 pop 下沉最多走一條根葉路徑，因此 O(log N)。
 * top 查 front 為 O(1)。建立 heap 採 bottom-up，所有節點高度工作加權和為 O(N)。
 * sort_heap 做 N 次 pop，故 O(N log N)。
 *
 * 【記憶體與 iterator】
 * heap 演算法本身原地，但 vector 成長可能配置。push_back 前保存的 pointer/reference
 * 可能因 reallocation 全失效。pop_back 使尾元素 reference 失效。演算法不保存任何
 * iterator；每次操作都應從現有容器重新取得 begin/end。
 *
 * 【測試清單】
 * 空 heap（只能驗證/建立，dispatch 應回 nullopt，不能 pop/front）、單元素、重複 priority、已是 heap、
 * 反向 comparator、tie FIFO、連續 push/pop、序列化壞資料、極端 key 都要測。
 * 每次開發期操作後可 assert is_heap；正式高流量路徑不應每輪 O(N) 驗證。
 *
 * 【LeetCode 複習】LC1046 要先取出兩個最大值，再視差值決定是否 push；不要在
 * 還持有 vector iterator 時 push_back，因重配可能使 iterator 失效。
 * 【實務複習】排程 tie 若要 FIFO，sequence 必須是 comparator 的一部分；heap
 * 本身不穩定。實務資料若來自磁碟，先 is_heap 驗證或 make_heap 修復。
 * 【最後陷阱】priority_queue 的 Compare 語意常被看反：front 是依 Compare 排在
 * 最後、也就是最高 priority 的元素；用小例子測 comparator 比背名稱可靠。
 */
