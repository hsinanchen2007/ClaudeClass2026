// ============================================================================
// C++20 容器總複習：選擇、複雜度、失效規則與面試整合題
// ============================================================================
// 本檔設計成面試前可單獨閱讀的「濃縮教材」。若只記一條：
//     沒有明確反證時先用 vector；再依 access pattern 換容器。
// 不要只背 Big-O。CPU cache、配置次數、元素大小、順序需求、最差延遲與 handle
// 穩定性，都可能比漸進複雜度更重要。
//
// ┌──────────────────────┬──────────────────────────┬───────────────────────────┐
// │ 需求                 │ 優先候選                 │ 為什麼                    │
// ├──────────────────────┼──────────────────────────┼───────────────────────────┤
// │ 一般可變長序列       │ vector                   │ 連續、cache 友善、索引 O1 │
// │ 編譯期固定長度       │ array                    │ value type、無 heap       │
// │ 兩端頻繁進出         │ deque                    │ push/pop 兩端 O1          │
// │ 穩定節點、已知點插刪 │ list / forward_list      │ 插刪 O1，但定位 O(n)      │
// │ 不擁有連續資料 view  │ span                     │ pointer+size、零拷貝      │
// │ sorted unique key    │ set / map                │ O(log n)、bounds/range    │
// │ sorted duplicate key │ multiset / multimap      │ equal_range               │
// │ 快速 membership/key  │ unordered_set / _map     │ 平均 O1、無排序           │
// │ hash duplicate key   │ unordered_multi*         │ 平均 O1 + equal_range     │
// │ LIFO / FIFO / top    │ stack / queue / priority │ adapter 限制介面          │
// └──────────────────────┴──────────────────────────┴───────────────────────────┘
//
// 【核心複雜度速查】n=元素數，k=匹配數
// vector/array/deque index O(1)；list/forward_list 第 i 個 O(n)。
// vector 尾插攤銷 O(1)，中間插刪 O(n)；deque 兩端攤銷 O(1)，中間 O(n)。
// list 已知 iterator 插刪 O(1)，但 find 位置 O(n)。
// map/set 系列 find/insert O(log n)；erase(iterator) 攤銷 O(1)，erase(key) 為
// O(log n + count)，range erase 為 O(log n + distance)。equal_range 走訪另加 O(k)。
// unordered 系列平均 O(1)，最差 O(n)；匹配多筆另加 O(k)。
// priority_queue top O(1)，push/pop O(log n)；queue/stack front/top 操作 O(1)。
// span 建立/subspan O(1)，演算法成本由走訪元素決定。
//
// 【iterator / reference / pointer 失效面試表】
// 1. vector：重配置 => 全部失效；未重配置 insert/erase => 操作點及後方失效。
// 2. deque：規則依操作位置而異；兩端插入也可能使 iterator 失效，修改後重取最穩。
// 3. list/forward_list：insert/splice 保留既有 handle；erase 只刪除目標 handle。
// 4. map/set：insert 保留既有 handle；erase 只刪除目標；key 不可原地修改。
// 5. unordered：rehash 使 iterator 失效；未 erase 元素的 reference/pointer 保留。
// 6. span：自身 iterator 是否有效完全依底層來源；來源死亡/擴容即可能懸空。
// 7. adapter：不公開 iterator；top/front reference 在 pop 後失效。
//
// 【常見陷阱／容易答錯的 API】
// - reserve 改 capacity，不改 size；resize 改 size 並建構/銷毀元素。
// - map/unordered_map 的 [] 會插入；唯讀用 contains/find/at。
// - erase(key) 在 multi 容器會刪全部相等項；erase(iterator) 才只刪一筆。
// - set/map 的「key 相同」由 Compare 等價定義；hash 容器由 KeyEqual 定義。
// - unordered 遍歷順序不是契約；priority_queue 也不是完整排序結果。
// - stack/queue 的 pop 不回值，先 move top/front，再 pop。
// - span 不擁有、不延長生命，也不表示 null-terminated string。
// - deque 不保證整體連續，不能把 &d[0] 當成涵蓋全容器的 C array。
// - forward_list 只有 before_begin/insert_after/erase_after；單向結構沒有 size/back。
// - list::splice 可 O(1) 轉移節點（同 allocator 等契約下），list::sort 不需 random access。
// - unordered 的 load_factor=max_load_factor 門檻會觸發 rehash；大量插入前 reserve。
//
// 【例外安全與 ownership】
// 標準容器擁有元素；span 不擁有。push/insert 可能因 allocation/constructor 丟例外。
// 多數操作承諾 strong 或 basic guarantee，但依元素 move/copy 特性與指定操作而異，
// 不可籠統宣稱所有容器操作「失敗完全不變」。RAII 容器本身會在 scope 結束釋放。

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------
// 基礎整合：用 access pattern 驗證各家角色
// ----------------------------------------------------------------------------
void basic_container_tour()
{
    std::array<int, 3> fixed{1, 2, 3};
    std::vector<int> contiguous(fixed.begin(), fixed.end());
    std::span<const int> view = contiguous;
    assert(std::accumulate(view.begin(), view.end(), 0) == 6);

    std::deque<int> two_ended{2};
    two_ended.push_front(1);
    two_ended.push_back(3);
    assert(two_ended.front() == 1 && two_ended.back() == 3);

    std::list<int> stable_nodes{1, 3};
    stable_nodes.insert(std::next(stable_nodes.begin()), 2);
    std::list<int> incoming{0};
    stable_nodes.splice(stable_nodes.begin(), incoming);
    stable_nodes.sort();
    assert(incoming.empty());
    assert((std::vector<int>(stable_nodes.begin(), stable_nodes.end()) ==
            std::vector<int>{0, 1, 2, 3}));

    std::forward_list<int> one_way{2, 3};
    one_way.push_front(1);
    const auto inserted = one_way.insert_after(one_way.before_begin(), 0);
    one_way.erase_after(inserted);  // 刪除原本的 1；after API 是單向串列關鍵。
    assert(std::distance(one_way.begin(), one_way.end()) == 3);
    assert((std::vector<int>(one_way.begin(), one_way.end()) ==
            std::vector<int>{0, 2, 3}));

    std::set<int> ordered_unique{3, 1, 2, 2};
    std::multiset<int> ordered_bag{2, 2, 3};
    assert(*ordered_unique.begin() == 1 && ordered_unique.size() == 3U);
    assert(ordered_bag.count(2) == 2U);

    std::map<int, std::string> ordered_index{{1, "one"}, {2, "two"}};
    std::multimap<int, std::string> ordered_multi{{1, "a"}, {1, "b"}};
    assert(ordered_index.at(2) == "two" && ordered_multi.count(1) == 2U);

    std::unordered_set<int> hashed_unique{1, 2, 2};
    std::unordered_multiset<int> hashed_bag{1, 1, 2};
    std::unordered_map<int, std::string> hashed_index{{7, "seven"}};
    std::unordered_multimap<int, std::string> hashed_multi{{7, "a"}, {7, "b"}};
    assert(hashed_unique.size() == 2U && hashed_bag.count(1) == 2U);
    assert(hashed_index.at(7) == "seven" && hashed_multi.count(7) == 2U);
    hashed_index.reserve(64U);  // 依預期元素量控制 rehash，而非依賴目前 bucket 數。
    assert(hashed_index.load_factor() <= hashed_index.max_load_factor());

    std::stack<int> lifo;
    std::queue<int> fifo;
    std::priority_queue<int> highest;
    for (const int value : {1, 3, 2}) {
        lifo.push(value);
        fifo.push(value);
        highest.push(value);
    }
    assert(lifo.top() == 2);     // 最後加入。
    assert(fifo.front() == 1);   // 最先加入。
    assert(highest.top() == 3);  // 優先權最高。
}

// ----------------------------------------------------------------------------
// LeetCode 347：Top K Frequent Elements
// ----------------------------------------------------------------------------
// 組合 unordered_map（frequency）與 priority_queue（保留 top-k）。
// n 筆輸入、u 種值：計數平均 O(n)，heap O(u log k)，空間 O(u+k)。
using FrequencyEntry = std::pair<int, int>;  // frequency, value

std::vector<int> top_k_frequent(const std::vector<int>& numbers, std::size_t k)
{
    std::unordered_map<int, int> frequency;
    frequency.reserve(numbers.size());
    for (const int number : numbers) {
        ++frequency[number];
    }

    // min-heap：頻率較小者在 top；同頻率時 value 較小者先被淘汰。
    std::priority_queue<FrequencyEntry,
                        std::vector<FrequencyEntry>,
                        std::greater<FrequencyEntry>> best;
    for (const auto& [value, count] : frequency) {
        best.emplace(count, value);
        if (best.size() > k) {
            best.pop();
        }
    }

    std::vector<int> result;
    result.reserve(best.size());
    while (!best.empty()) {
        result.push_back(best.top().second);
        best.pop();
    }
    std::ranges::sort(result);  // 題目接受任意順序；測試採 deterministic 表示。
    return result;
}

void leetcode_demo()
{
    assert((top_k_frequent({1, 1, 1, 2, 2, 3}, 2U) ==
            std::vector<int>{1, 2}));
    assert((top_k_frequent({4}, 1U) == std::vector<int>{4}));
}

// ----------------------------------------------------------------------------
// 實務整合：事件管線
// ----------------------------------------------------------------------------
// 需求：
// 1. 保留輸入順序（vector）；2. 去除重複 event id（unordered_set）；
// 3. 聚合服務錯誤數（unordered_map）；4. 產出名稱排序報表（map）；
// 5. 保存最近三個 id（deque）；6. 找錯誤最多服務（priority_queue）。
// 這展示「一個系統可以各取所長」，不是硬選一個容器做所有事。
struct Event {
    int id;
    std::string service;
    bool failed;
};

struct Report {
    std::map<std::string, int> errors_by_service;
    std::vector<int> recent_ids;
    std::string noisiest_service;
};

Report build_report(std::span<const Event> events)
{
    std::unordered_set<int> seen_ids;
    std::unordered_map<std::string, int> errors;
    std::deque<int> recent;

    seen_ids.reserve(events.size());
    errors.reserve(events.size());
    for (const Event& event : events) {
        if (!seen_ids.insert(event.id).second) {
            continue;  // duplicate event 不重複計費/告警。
        }
        recent.push_back(event.id);
        if (recent.size() > 3U) {
            recent.pop_front();
        }
        if (event.failed) {
            ++errors[event.service];
        }
    }

    std::map<std::string, int> ordered(errors.begin(), errors.end());
    using Ranked = std::pair<int, std::string>;
    std::priority_queue<Ranked> ranking;
    for (const auto& [service, count] : ordered) {
        ranking.emplace(count, service);
    }

    return {std::move(ordered),
            std::vector<int>(recent.begin(), recent.end()),
            ranking.empty() ? std::string{} : ranking.top().second};
}

void practical_demo()
{
    const std::vector<Event> events{
        {1, "api", true}, {2, "db", true}, {1, "api", true},
        {3, "api", false}, {4, "db", true}};
    const Report report = build_report(events);
    assert(report.errors_by_service.at("api") == 1);
    assert(report.errors_by_service.at("db") == 2);
    assert((report.recent_ids == std::vector<int>{2, 3, 4}));
    assert(report.noisiest_service == "db");
}

int main()
{
    basic_container_tour();
    leetcode_demo();
    practical_demo();
    std::cout << "Container summary：選擇、Top-K 與事件管線全部通過\n";
}

// ============================================================================
// 面試快問快答
// ============================================================================
// Q1: vector push_back 一定 O(1) 嗎？
// A : 單次不一定；重配置時 O(n)，一串操作的攤銷成本 O(1)。
//
// Q2: list 中間 insert O(1)，所以一定比 vector 快？
// A : 否。定位可能 O(n)，節點配置與 cache miss 也昂貴；已持有穩定 iterator 且元素
//     搬移昂貴時才更可能獲益。
//
// Q3: map 與 unordered_map 如何選？
// A : 要排序/bounds/穩定最差延遲/deterministic traversal 選 map；只要 lookup 且 hash
//     良好通常選 unordered_map。也要比較記憶體與 rehash 行為。
//
// Q4: unordered_map rehash 後 reference 是否失效？
// A : iterator 失效；未被 erase 的元素 reference/pointer 仍有效。不要把規則套到 vector。
//
// Q5: reserve 與 resize？
// A : reserve 只確保 capacity；resize 真正改元素數。reserve 後用 [] 寫 size 外仍是 UB。
//
// Q6: set 為何不能修改 *iterator？
// A : 元素同時是排序/hash key，原地修改會破壞容器 invariant。
//
// Q7: priority_queue 的 Compare 為何「反直覺」？
// A : Compare 表示較低優先順序；預設 less 讓最大值不低於其他值而位於 top。
//
// Q8: span 可以避免 copy，代價是什麼？
// A : 不擁有資料，呼叫端必須保證來源的生命週期與位址穩定，且只適用連續資料。
//
// Q9: multi 容器怎麼只刪一份重複值？
// A : find 得 iterator，再 erase(iterator)；erase(key/value) 通常刪全部匹配。
//
// Q10: 如何讓 hash 容器輸出可重現？
// A  : 把結果複製到 vector 後 sort，或轉成 map；不要依賴目前碰巧的 bucket 順序。
//
// 【最後練習】
// 1. 為 build_report 加「每服務最近 2 個 event id」，選擇 map of deque 或 hash map。
// 2. 用 vector + sort 改寫 top_k_frequent，比較 n log n 與 u log k。
// 3. 在 debug STL/ASan 下觀察 vector reallocation 後使用舊 iterator 的錯誤；危險碼
//    只放私人實驗，不要提交到預設執行路徑。
