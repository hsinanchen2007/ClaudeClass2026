/*
 * Binary Search 家族：面試前完整速查章
 * =====================================
 *
 * 一、選擇表（升冪、半開區間 [first,last)）
 * ---------------------------------------------------------------
 * 需求                         API              回傳
 * 只問是否存在                 binary_search    bool
 * 第一個 >= x / 最左插入點     lower_bound      iterator
 * 第一個 >  x / 最右插入點     upper_bound      iterator
 * 所有等價 x                   equal_range      [lower, upper)
 * <x 數量                      distance(begin, lower)
 * <=x 數量                     distance(begin, upper)
 * x 的個數                     distance(lower, upper)
 *
 * 二、共同契約
 * 1. 資料必須依同一比較規則 partitioned/sorted；否則答案不可信。
 * 2. 比較器必須是 strict weak ordering：不可同時 a<b 與 b<a，等價要可傳遞。
 * 3. 等價是 !comp(a,b) && !comp(b,a)，不保證呼叫 operator==。
 * 4. 比較 O(log N)；list/forward_list 的 iterator 位移可能 O(N)。
 * 5. 演算法不保存 iterator；回傳值的生命與失效規則由原容器決定。
 *
 * 三、手寫二分搜尋的不變量
 * 維持答案位於 [lo, hi)，每輪取 mid=lo+(hi-lo)/2，避免 (lo+hi) 溢位。
 * 找 lower_bound 時：a[mid] < x => lo=mid+1，否則 hi=mid。
 * 結束 lo==hi，就是第一個不小於 x 的位置。區間縮小是避免無限迴圈的核心。
 *
 * 四、常見陷阱
 * - 排序用 greater<>，搜尋仍用預設 less<>。
 * - 取得 vector iterator 後 insert，reallocation 使舊 iterator 全失效。
 * - 用 binary_search 找索引；它只回 bool。
 * - 對浮點 NaN、隨時間變動的 comparator、<= comparator 做排序。
 * - 把 upper_bound 誤記成 >=；它是嚴格 >。
 */

/*
==============================================================================
【面試深挖：Binary Search】

A1｜`lower_bound` 的真正前置條件是什麼？
答：搜尋區間必須依「element < value」這個判斷完成 partition；通常排序後自然滿足，
但標準要求的是 partitioned，不是字面上的 sorted。若前置條件不成立，結果沒有保證。
追問：自訂 comparator 時，呼叫方向是 comp(element, value)，不可隨意反過來。

A2｜`lower_bound`、`upper_bound`、`equal_range` 如何區分？
答：lower 是第一個「不小於」，upper 是第一個「大於」，兩者形成半開區間
[first_equal, after_last_equal)。equal_range 一次表達整個重複區間；不存在時兩端相等，
那個位置同時也是維持排序的插入點。

A3｜為何在 `list` 上呼叫 `std::lower_bound` 不一定快？
答：比較次數仍為 O(log n)，但 ForwardIterator 無法 O(1) 跳到中點，iterator 前進總量
可能是 O(n)。tree container 應優先用 member `lower_bound`，它沿樹走 O(log n)。
常見誤答：「二分搜尋在任何容器都是 O(log n)」忽略了 iterator movement。

A4｜中點為何不該寫 `(low + high) / 2`？
答：low+high 可能 overflow。index 常寫 low + (high-low)/2；一般整數或 pointer 可用
C++20 `std::midpoint`。但若 loop invariant 或 signed/unsigned 已錯，換公式也救不了。

A5｜如何避免 `while (low <= high)` 的邊界錯誤？
答：先選明確 invariant。推薦半開區間 [low, high)，每輪排除至少一個位置，結束時
low==high；找第一個 true 時，維持 [0,low) false、[high,n) true。先寫 invariant，
再決定更新式，而不是背模板。

A6｜「對答案二分」何時成立？
答：答案空間上必須有單調 predicate，例如容量愈大愈可行。每次檢查可行性成本為 T，
總成本約 O(T log range)。若 predicate 會 true/false 反覆交錯，二分沒有正確性。

A7｜`binary_search` 為何不適合取得位置？
答：它只回 bool；需要 iterator、插入點或重複範圍時用 lower_bound/equal_range。
若資料未排序、只查一次，先排序 O(n log n) 可能不如線性 find；要按查詢批次與更新率選。

A8｜comparator 最常見的面試陷阱是什麼？
答：排序與搜尋必須使用相容的 strict weak ordering。用 `<=`、依會變動的外部狀態，
或 sort 用一套規則、lower_bound 用另一套，會破壞 partition 前提。等價是
!comp(a,b) && !comp(b,a)，不一定等同 operator==。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

struct Record {
    int key;
    std::string payload;
};

// LeetCode 35：手寫 lower_bound，展示面試不變量。
std::size_t leetcode_manual_lower_bound(const std::vector<int>& nums,
                                        int target) {
    std::size_t lo = 0;
    std::size_t hi = nums.size();
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2U;
        if (nums[mid] < target) {
            lo = mid + 1U;
        } else {
            hi = mid;
        }
    }
    return lo;
}

// 實務整合：在排序索引中查詢某 key 的全部 payload。
std::vector<std::string> practical_lookup_all(const std::vector<Record>& index,
                                              int key) {
    const auto lower = std::lower_bound(
        index.begin(), index.end(), key,
        [](const Record& row, int wanted) { return row.key < wanted; });
    const auto upper = std::upper_bound(
        lower, index.end(), key,
        [](int wanted, const Record& row) { return wanted < row.key; });

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(std::distance(lower, upper)));
    for (auto it = lower; it != upper; ++it) {
        result.push_back(it->payload);
    }
    return result;
}

int main() {
    const std::vector<int> nums{1, 3, 3, 7, 10};
    assert(std::binary_search(nums.begin(), nums.end(), 7));
    assert(!std::binary_search(nums.begin(), nums.end(), 2));
    assert(std::lower_bound(nums.begin(), nums.end(), 3) == nums.begin() + 1);
    assert(std::upper_bound(nums.begin(), nums.end(), 3) == nums.begin() + 3);
    assert(std::equal_range(nums.begin(), nums.end(), 3).first == nums.begin() + 1);
    assert(leetcode_manual_lower_bound(nums, 4) == 3U);

    const std::vector<Record> index{{10, "A"}, {20, "B"}, {20, "C"}, {30, "D"}};
    assert((practical_lookup_all(index, 20) ==
            std::vector<std::string>{"B", "C"}));
    assert(practical_lookup_all(index, 25).empty());

    std::cout << "Binary Search 家族整合複習完成\n";
}

/*
 * 面試 Q&A
 * Q1: 已排序 vector 與 std::set 如何選？
 * A1: 讀多寫少可選 vector（cache locality 好）；頻繁插刪且要排序唯一鍵選 set。
 * Q2: lower_bound 找不到時回什麼？
 * A2: 回「可插入位置」，可能是 end；必須先 it!=end 再解參考。
 * Q3: 如何找最後一個 <=x？
 * A3: it=upper_bound(x)；若 it==begin 則不存在，否則 --it。
 * Q4: 為何 comparator 不能寫 <=？
 * A4: comp(x,x) 會為 true，違反 irreflexive，排序與搜尋契約被破壞。
 *
 * 練習：
 * 1. 實作 descending 的 manual_lower_bound。
 * 2. 寫「第一個 timestamp >= start 且 < end」的事件範圍查詢。
 * 3. 比較 vector 與 forward_list 上 lower_bound 的比較次數及 iterator 步數。
 *
 * 五、考前推導模板
 * - first >= x：lower_bound(x)
 * - first > x：upper_bound(x)
 * - last < x：lower_bound(x) 前一格（先防 begin）
 * - last <= x：upper_bound(x) 前一格（先防 begin）
 * - [lo,hi] 範圍：lower_bound(lo) 到 upper_bound(hi)
 * - [lo,hi) 範圍：lower_bound(lo) 到 lower_bound(hi)
 * - floor(x)：last <= x；ceiling(x)：first >= x
 *
 * 六、容器與複雜度選擇
 * sorted vector：查詢 O(log N)、中間插入 O(N)、連續記憶體 locality 好。
 * set/map：查詢與插入 O(log N)，node 配置成本高，iterator 穩定性較好。
 * unordered_set/map：平均 O(1)，沒有順序，不能直接做 lower/upper bound。
 * list：即使 lower_bound 比較 O(log N)，iterator 前進仍可能 O(N)，通常不划算。
 *
 * 七、測試清單
 * 空輸入、單元素、target 小於全部、大於全部、位於頭尾、大量重複、完全不存在、
 * descending comparator、自訂 key、會導致 end 的案例都要覆蓋。對手寫版本再做
 * property test：與 std::lower_bound 在隨機已排序資料上的 index 必須一致。
 *
 * 八、進階面試追問
 * - 如何在未知長度的單調資料流搜尋？先 exponential search 找上界，再二分。
 * - 如何找 rotated sorted array？判斷哪半邊有序，再決定捨棄區間。
 * - 答案是可行性而非陣列元素？對單調 predicate 做 binary search on answer。
 * - 浮點答案何時停止？固定迭代次數通常比 while(error>eps) 更可預測。
 *
 * 九、LeetCode 最後檢查
 * 手寫時先說區間語意，再寫更新式；每輪必須嚴格縮小。用 size_t 時不能寫
 * hi=mid-1 的閉區間模板而不處理 mid==0，否則 unsigned underflow。本文採 [lo,hi)
 * 可讓 hi=mid，並以 lo==hi 自然收斂。
 *
 * 十、實務索引維護
 * sorted vector 若批次更新，常見策略是收集增量、分別排序後 merge，而非每筆在
 * 中間 insert。查詢 thread 需要一致 snapshot，可用 immutable index + atomic
 * shared_ptr 發布新版本；不要讓搜尋期間資料被另一 thread 原地重排。
 *
 * 最後陷阱：O(log N) 描述的是比較/跳躍成本，不包含前置 sort、key 建構、昂貴
 * comparator 或 I/O。面試回答複雜度時要說清楚假設。
 */
