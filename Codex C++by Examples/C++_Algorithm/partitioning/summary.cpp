/*
 * Partitioning algorithms 面試速查總結
 * =====================================
 * 同目錄 API：is_partitioned、partition、stable_partition、partition_copy、
 * partition_point。共同模型是 predicate 產生 true/false 兩群，不是完整排序。
 *
 * 【選擇表】
 * 只驗證資料 invariant                         is_partitioned
 * 原地分組、不在意群組內順序                   partition
 * 原地分組、必須保留群組內順序                 stable_partition
 * 保留來源並分流到兩個目的地                   partition_copy
 * 已分區後快速取得 true/false 邊界              partition_point
 *
 * 【複雜度】
 * is_partitioned / partition / partition_copy：O(N)。
 * stable_partition：有 buffer 常見 O(N)，無 buffer 可 O(N log N) swap。
 * partition_point：random access O(log N)；forward iterator 的前進總量可 O(N)。
 *
 * 【範圍與生命週期】
 * 原地分區會搬動元素。vector 的 iterator 通常仍指向某個位置，但該位置已可能是
 * 另一個業務物件；不要把「iterator 未失效」誤解成「仍指向原物件」。輸出分流時
 * 兩個目的地不可不安全重疊，back_inserter 所屬容器也必須活到操作完成。
 */

/*
==============================================================================
【面試深挖：Partitioning】

A1｜`partition` 與 `stable_partition` 的差別？
答：兩者都把 predicate=true 放前面；stable 另保留各組內原順序，通常需要額外記憶體
或更多操作。若順序是 API observable behavior，就不能只因較快換成不穩定版本。

A2｜`partition_point` 的前置條件？
答：range 必須已依同一 predicate partitioned，否則結果無保證。它用二分找 true/false
邊界；先呼叫 is_partitioned 可驗證，但那會額外 O(n)，不一定適合 hot path。

A3｜partition 與 sort 的差別？
答：partition 只保證兩群，不保證群內順序或完整排序，通常 O(n)；sort 建立全序
O(n log n)。只需要把有效/無效、低/高風險分組時，完整排序是過度工作。

A4｜三色旗問題如何與標準演算法連結？
答：可先 partition < pivot，再在右段 partition == pivot；或一次 Dutch National Flag。
前者組合簡潔，後者單 pass 但 invariant 較難寫，面試要能說明 [low,mid,high) 各區語意。

A5｜quicksort 的 partition 為何影響 worst case？
答：pivot 極端不平衡會形成 O(n²)；random/median 策略降低常見風險，但不一定消除理論
worst case。std::sort 有複雜度保證，具體是否 introsort 是實作策略。

A6｜`partition_copy` 的輸出有什麼責任？
答：呼叫者提供兩個可寫 output ranges，容量必須足夠且不可造成不允許的重疊。
back_inserter 可自動增長 container，但會有 allocation；預留容量可避免反覆擴張。

A7｜predicate 可依賴會改變的外部狀態嗎？
答：技術上某些 sequential call 可執行，但會破壞「同一元素分類一致」的推理；
parallel policy 更可能 data race。分類規則應是對輸入的穩定純判斷。

A8｜如何驗證 partition 結果而不假設群內順序？
答：檢查 boundary 前皆 true、後皆 false，或用 is_partitioned；不要拿預期完整序列比較，
因為 non-stable partition 允許多種合法排列。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

struct WorkItem {
    int id;
    int score;
    std::string owner;
};

bool is_actionable(const WorkItem& item) {
    return item.score >= 50;
}

// 基礎整合：穩定分區，並回傳分界 offset 而非長期保存 iterator。
std::size_t basic_prioritize(std::vector<WorkItem>& items) {
    const auto boundary = std::stable_partition(items.begin(), items.end(),
                                                is_actionable);
    return static_cast<std::size_t>(std::distance(items.begin(), boundary));
}

// LeetCode 922：Sort Array By Parity II；先分流，再交錯合併。
std::vector<int> leetcode_sort_array_by_parity_ii(const std::vector<int>& nums) {
    std::vector<int> even;
    std::vector<int> odd;
    even.reserve(nums.size() / 2U);
    odd.reserve(nums.size() / 2U);
    std::partition_copy(nums.begin(), nums.end(), std::back_inserter(even),
                        std::back_inserter(odd),
                        [](int value) { return value % 2 == 0; });
    // LC922 保證偶數與奇數各半；可重用函式仍須自行守住契約。
    // assert 在 -DNDEBUG 會消失，因此不能用它阻止後面的 odd[i] 越界。
    if (even.size() != odd.size()) {
        throw std::invalid_argument("parity counts must be equal");
    }

    std::vector<int> answer(nums.size());
    for (std::size_t i = 0; i < even.size(); ++i) {
        answer[i * 2U] = even[i];
        answer[i * 2U + 1U] = odd[i];
    }
    return answer;
}

// 實務：把 actionable 與 deferred 複製分流，來源快照保持不變。
struct RoutedWork {
    std::vector<WorkItem> actionable;
    std::vector<WorkItem> deferred;
};

RoutedWork practical_route_work(const std::vector<WorkItem>& input) {
    RoutedWork output;
    output.actionable.reserve(input.size());
    output.deferred.reserve(input.size());
    std::partition_copy(input.begin(), input.end(),
                        std::back_inserter(output.actionable),
                        std::back_inserter(output.deferred), is_actionable);
    return output;
}

int main() {
    std::vector<WorkItem> queue{{1, 20, "A"}, {2, 80, "B"},
                                {3, 70, "C"}, {4, 10, "D"}};
    const std::size_t ready_count = basic_prioritize(queue);
    assert(ready_count == 2U);
    assert(queue[0].id == 2 && queue[1].id == 3);  // stable true group。
    assert(queue[2].id == 1 && queue[3].id == 4);  // stable false group。
    assert(std::is_partitioned(queue.begin(), queue.end(), is_actionable));

    const auto boundary = std::partition_point(queue.begin(), queue.end(),
                                               is_actionable);
    assert(static_cast<std::size_t>(std::distance(queue.begin(), boundary)) == 2U);

    const auto parity = leetcode_sort_array_by_parity_ii({4, 2, 5, 7});
    for (std::size_t i = 0; i < parity.size(); ++i) {
        assert(parity[i] % 2 == static_cast<int>(i % 2U));
    }
    bool invalid_parity_rejected = false;
    try {
        static_cast<void>(leetcode_sort_array_by_parity_ii({2}));
    } catch (const std::invalid_argument&) {
        invalid_parity_rejected = true;
    }
    assert(invalid_parity_rejected);

    const std::vector<WorkItem> snapshot{{10, 99, "ops"}, {11, 1, "dev"}};
    const RoutedWork routed = practical_route_work(snapshot);
    assert(routed.actionable.size() == 1U && routed.actionable[0].id == 10);
    assert(routed.deferred.size() == 1U && routed.deferred[0].id == 11);
    assert(snapshot[0].id == 10);  // copy 版本不修改來源。

    std::cout << "Partitioning summary：五個 API 整合測試通過\n";
}

/*
 * 【重要前置條件與陷阱】
 * 1. partitioned 只要求所有 true 在所有 false 前，不要求各群組排序。
 * 2. partition_point 的範圍必須已用相同 predicate 分區；否則不是退化為線性，
 *    而是結果不具保證。debug 可 assert is_partitioned。
 * 3. partition 不穩定，測試應驗 predicate invariant，不應硬寫某個元素排列。
 * 4. stable_partition 保留兩群各自原順序，但可能配置 buffer。
 * 5. partition_copy 需要兩個有效 output；reserve 不產生元素，搭配 begin 會越界，
 *    應 resize 或使用 back_inserter。
 * 6. output iterator 若來自同一 vector，任一 push_back reallocate 都可能使另一個
 *    iterator 失效；通常使用兩個獨立容器。
 * 7. predicate 應是純分類且同一輸入得到同一結果；依時鐘/亂數會破壞 invariant。
 * 8. 原地搬動後位置與業務 identity 分離；外部索引、pointer map 可能需要重建。
 * 9. callback 丟例外時可能已有部分交換/輸出，不保證 transaction rollback。
 * 10. concurrent writer 會讓 is_partitioned 與 partition_point 間出現 TOCTOU。
 * 11. LeetCode 題目給的輸入保證不是一般 library 契約；抽成可重用函式後要在 runtime
 *     驗證。只用 assert 會讓 release build 失去防線。
 *
 * 【面試快問快答】
 * Q: partition 的回傳值？
 * A: 第一個 predicate=false，也就是 [begin,p) true、[p,end) false。
 * Q: stable_partition 和 stable_sort？
 * A: 前者只分兩群，後者建立完整弱序；需求只有分類時前者更直接。
 * Q: partition_point 為何可二分？
 * A: predicate 結果是 true...true,false...false 的單調布林序列。
 * Q: forward_list 的 partition_point 是 O(log N) 嗎？
 * A: predicate 次數對數，但 iterator advance 總步數可線性。
 * Q: LC283 可直接 stable_partition 嗎？
 * A: 語意正確，但題目要求原地/最少空間時，實作可能配置；面試應寫雙指標。
 * Q: partition_copy 是否穩定？
 * A: 是，各輸出按來源掃描順序寫入，群組內相對順序保留。
 *
 * 【選擇流程】
 * 先問要不要改來源。不要改 -> partition_copy；只檢查 -> is_partitioned。
 * 要原地改，再問群組內順序是否有意義：有 -> stable_partition；無 -> partition。
 * 已有 invariant 且只找 boundary -> partition_point，不要重新 partition。
 *
 * 【實務案例】
 * - request admission：accepted/rejected 用 partition_copy，保留 audit source。
 * - scheduler：ready/not-ready 原地 partition，若同優先權 FIFO 則 stable_partition。
 * - rollout：healthy 前綴後 unhealthy，partition_point 找第一個失敗版本。
 * - UI：pinned items 置頂且保持使用者順序，stable_partition。
 * - 資料清洗：valid/invalid 雙流，invalid 保留錯誤資訊，不只丟棄。
 *
 * 【面試前自問】
 * - 能否說明空範圍、全 true、全 false 時 boundary 在哪？
 * - 能否分辨「iterator 沒失效」與「仍代表同一元素」？
 * - 能否說明 stable 成本與 partition_copy 容量契約？
 * - 能否用 predicate 單調性推導 partition_point？
 *
 * 練習：
 * 1. 為 RoutedWork 加 rejected reason，維持 input index。
 * 2. 用 rotate 遞迴實作 stable partition，分析無 buffer 複雜度。
 * 3. 實作三路分類（high/medium/low），比較兩次 partition 與 counting buckets。
 * 4. 對一百萬個大型物件 benchmark 原地 swap 與 index partition。
 * 5. 建立 property test：partition 後 is_partitioned 必為 true，且 multiset 不變。
 */
