/*
 * Sorted set algorithms 面試速查總結
 * ===================================
 * 同目錄八章：merge、inplace_merge、includes、set_union、set_intersection、
 * set_difference、set_symmetric_difference，以及本 summary。
 *
 * 最重要共同前置條件：所有輸入必須依「同一 comparator」排序。這些演算法操作的
 * 是 sorted ranges，不要求容器型別為 std::set；vector 通常有更好 locality。
 *
 * 【選擇表與重複次數】（A 中 m 次、B 中 n 次）
 * merge                     m+n；保留兩邊全部元素
 * set_union                 max(m,n)
 * set_intersection          min(m,n)
 * set_difference A-B        max(m-n,0)
 * set_symmetric_difference  |m-n|
 * includes                  檢查每個值 m>=n
 * inplace_merge             同容器相鄰兩 sorted runs 合成一段
 *
 * 【複雜度】
 * 雙範圍 merge/set 系列皆 O(N+M) 線性掃描；輸出大小依上表。
 * inplace_merge 有 buffer 常見 O(N)，無 buffer 時比較/搬移可到 O(N log N)。
 * 額外空間通常由輸出容器承擔；back_inserter 可能觸發 reallocation。
 */

/*
==============================================================================
【面試深挖：Set Algorithms】

A1｜set_union 等演算法可直接吃 unordered_set 嗎？
答：它們要求兩個 input ranges 依同一 comparator 排序；容器名稱有 set 不代表 iteration
一定符合。ordered set 可以，unordered_set 必須先排序副本或改用 hash-based 作法。

A2｜重複元素的 multiplicity 如何計算？
答：union 取兩邊最大次數；intersection 取最小；difference 取 max(countA-countB,0)；
symmetric_difference 取絕對差。這些是 multiset 語意，不是單純 boolean presence。

A3｜為何常搭配 `back_inserter`？
答：output iterator 會逐項寫結果，空 vector 的 begin 沒有可寫元素；back_inserter 轉成
push_back。若已知上限可 reserve，兼顧安全與配置成本。

A4｜`merge` 與 `inplace_merge` 的差別？
答：merge 合併兩個獨立 sorted ranges 到另一個 output；inplace_merge 假設同一 range
的 [first,middle) 與 [middle,last) 各自已排序，再原地合成。

A5｜`includes` 在問什麼？
答：判斷第二個 sorted multiset 是否為第一個的子 multiset，重複次數也算。例如 A 只有
一個 2，就不包含有兩個 2 的 B。

A6｜set algorithm 如何判斷「相等」？
答：用 comparator equivalence：!comp(a,b) && !comp(b,a)，不一定呼叫 operator==。
若 comparator 只比 id，其他欄不同的 record 仍可能視為同 key。

A7｜複雜度為何通常是 O(n+m)？
答：兩個 iterator 單調向前，類似 merge step；這正是付出「兩邊先排序」後得到的好處。
若只有一次操作，排序成本可能主導；大量操作則 ordered representation 可攤提。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

struct FileRecord {
    int id;
    std::string checksum;
};

const auto by_id = [](const FileRecord& lhs, const FileRecord& rhs) {
    return lhs.id < rhs.id;
};

// 基礎整合：兩份 sorted manifest 產生 missing/extra/common。
struct ManifestDiff {
    std::vector<FileRecord> missing_on_replica;
    std::vector<FileRecord> extra_on_replica;
    std::vector<FileRecord> common_from_primary;
};

ManifestDiff basic_manifest_diff(const std::vector<FileRecord>& primary,
                                 const std::vector<FileRecord>& replica) {
    assert(std::is_sorted(primary.begin(), primary.end(), by_id));
    assert(std::is_sorted(replica.begin(), replica.end(), by_id));
    ManifestDiff diff;
    std::set_difference(primary.begin(), primary.end(), replica.begin(), replica.end(),
                        std::back_inserter(diff.missing_on_replica), by_id);
    std::set_difference(replica.begin(), replica.end(), primary.begin(), primary.end(),
                        std::back_inserter(diff.extra_on_replica), by_id);
    std::set_intersection(primary.begin(), primary.end(), replica.begin(), replica.end(),
                          std::back_inserter(diff.common_from_primary), by_id);
    return diff;
}

// LeetCode 349：Intersection of Two Arrays，distinct result。
std::vector<int> leetcode_intersection(std::vector<int> first,
                                       std::vector<int> second) {
    std::sort(first.begin(), first.end());
    first.erase(std::unique(first.begin(), first.end()), first.end());
    std::sort(second.begin(), second.end());
    second.erase(std::unique(second.begin(), second.end()), second.end());
    std::vector<int> output;
    std::set_intersection(first.begin(), first.end(), second.begin(), second.end(),
                          std::back_inserter(output));
    return output;
}

// 實務：將兩個已排序 audit streams 合併；相同 timestamp 時 primary 先出。
struct Audit {
    int timestamp;
    std::string message;
};

std::vector<Audit> practical_merge_audit(const std::vector<Audit>& primary,
                                         const std::vector<Audit>& secondary) {
    const auto less_time = [](const Audit& lhs, const Audit& rhs) {
        return lhs.timestamp < rhs.timestamp;
    };
    assert(std::is_sorted(primary.begin(), primary.end(), less_time));
    assert(std::is_sorted(secondary.begin(), secondary.end(), less_time));
    std::vector<Audit> output;
    output.reserve(primary.size() + secondary.size());
    std::merge(primary.begin(), primary.end(), secondary.begin(), secondary.end(),
               std::back_inserter(output), less_time);
    return output;
}

int main() {
    const std::vector<FileRecord> primary{{1, "a"}, {2, "b"}, {4, "d"}};
    const std::vector<FileRecord> replica{{2, "B"}, {3, "c"}, {4, "d"}};
    const ManifestDiff diff = basic_manifest_diff(primary, replica);
    assert(diff.missing_on_replica.size() == 1U &&
           diff.missing_on_replica[0].id == 1);
    assert(diff.extra_on_replica.size() == 1U && diff.extra_on_replica[0].id == 3);
    assert(diff.common_from_primary.size() == 2U);
    assert(diff.common_from_primary[0].checksum == "b"); // payload 取第一範圍。

    assert((leetcode_intersection({1, 2, 2, 1}, {2, 2}) ==
            std::vector<int>{2}));

    const auto audit = practical_merge_audit(
        {{1, "P1"}, {3, "P3"}}, {{1, "S1"}, {2, "S2"}});
    assert(audit[0].message == "P1" && audit[1].message == "S1");

    std::vector<int> runs{1, 4, 8, 2, 3, 9};
    std::inplace_merge(runs.begin(), runs.begin() + 3, runs.end());
    assert(std::is_sorted(runs.begin(), runs.end()));
    const std::vector<int> required{2, 8};
    assert(std::includes(runs.begin(), runs.end(), required.begin(), required.end()));

    std::cout << "Set operations summary：manifest、LC349 與 audit merge 測試通過\n";
}

/*
 * 【陷阱總表】
 * 1. 未排序輸入破壞前置條件；不是「結果可能慢」，而是結果不可信。
 * 2. 兩邊 comparator 必須一致；case folding、locale、projection 都要相同。
 * 3. set algorithms 採 multiset 語意；單側 duplicate 不會自動 unique。
 * 4. merge 的 duplicate 數是 m+n，union 是 max，intersection 是 min。
 * 5. difference 有方向；A-B 與 B-A 不同。symmetric diff 失去來源標記。
 * 6. equivalent key 不等於完整 object 相等；輸出通常從第一範圍取 payload。
 * 7. reserve 不建立元素；要 output.begin() 必須 resize，否則用 back_inserter。
 * 8. output 不可覆蓋尚未讀完的 input；相鄰同容器 runs 用 inplace_merge。
 * 9. inplace_merge 不改 size，卻重排 identity；舊位置 cache 失效。
 * 10. callback/搬移拋例外時可能已有部分輸出或排列，沒有自動 rollback。
 * 11. 權限、manifest 理論上 unique 時，duplicate 應先當資料品質錯誤處理。
 * 12. comparator 用 <= 不是 strict weak ordering，所有 sorted invariant 都會崩壞。
 *
 * 【面試快問快答】
 * Q: 為何 sorted vector 的 set operation 可 O(N+M)？
 * A: 雙指標只向前，每次至少推進一側。
 * Q: merge 是否 stable？
 * A: 是；等價時先取第一範圍，各來源內相對順序保留。
 * Q: set_union 是否等於 concat 後 unique？
 * A: 對 distinct set 類似；對 multiset 不同，union 保留 max multiplicity。
 * Q: includes 與逐項 binary_search 怎麼選？
 * A: size 相近用線性 includes；需求極小、superset random access 時 M log N 可合理。
 * Q: inplace_merge 的 middle 是什麼？
 * A: 第二個 sorted run 的起點，且左右兩半已各自 sorted。
 *
 * 【選型】
 * 保留全部來源 -> merge；相鄰 runs 原地 -> inplace_merge；只驗子集 -> includes；
 * 合併可用集合 -> union；共同可用 -> intersection；待處理 -> A-B；漂移 -> symmetric。
 * 若要來源標記或 payload conflict resolution，標準 set algorithm 可能不夠，手寫
 * 雙指標狀態機通常更清楚。
 *
 * 【生命週期與工程】
 * 輸入 snapshot 在整次操作必須穩定。concurrent mutation 需鎖或 immutable snapshot。
 * 大輸出先估上限 reserve 可減少配置，但不要在不可信尺寸相加時忽略 overflow。
 * 分散式資料若不在同一排序/normalization 版本，先 canonicalize 再比較。
 *
 * 【面試前自問】
 * - 我能立即寫出五種 multiplicity 公式嗎？
 * - 我能說明等價 Record 的 payload 來自哪邊嗎？
 * - 我能分辨 merge、union、inplace_merge 嗎？
 * - 我能為空集合、全部重複、完全不交集設計測試嗎？
 * - 我能證明 comparator 與 canonical ordering 一致嗎？
 *
 * 練習：
 * 1. 為 main 末尾 includes 測試加入更多 duplicate，驗證 multiset 次數；注意現有
 *    `required` 是具名範圍，begin/end 必須來自同一個仍存活的物件。
 * 2. 實作 tagged three-way manifest diff：added/removed/changed。
 * 3. 合併 K 個 sorted streams，使用 min-heap 並加入 deterministic tie-breaker。
 * 4. 為 case-insensitive key 建立同一 comparator/canonicalizer，測 Unicode 邊界。
 * 5. 比較 sorted vector set operation 與 unordered_set 在不同資料量的效能。
 */
