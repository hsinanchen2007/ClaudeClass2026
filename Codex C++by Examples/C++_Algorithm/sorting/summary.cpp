/*
 * Sorting algorithms 面試前總複習
 * ================================
 * 同目錄 API：sort、stable_sort、partial_sort、partial_sort_copy、nth_element、
 * is_sorted/is_sorted_until。先問「要全部有序、前 K 有序、只要第 K、還是只驗證」。
 *
 * 【選擇表】
 * 全部排序、不需保留等價次序                  sort
 * 全部排序、等價元素需維持原次序              stable_sort
 * 原地取得已排序 top-K                       partial_sort
 * 保留來源，把已排序 top-K 複製到輸出          partial_sort_copy
 * 只要第 K 值/partition，不需 top-K 內排序     nth_element
 * 驗證 invariant / 找第一個逆序               is_sorted / is_sorted_until
 *
 * 【複雜度】
 * sort：O(N log N) comparisons。stable_sort 有足夠額外 buffer 時 O(N log N)，
 * 無法取得 buffer 時為 O(N log^2 N) comparisons。
 * partial_sort：O(N log K)。partial_sort_copy 類似且另寫 K 個輸出。
 * nth_element：平均 O(N) comparisons（不是最壞線性保證），兩側不排序。
 * is_sorted：O(N)，可早退。
 *
 * 【共同契約】
 * comparator 必須 strict weak ordering：comp(x,x)==false、非對稱、傳遞，且等價
 * 關係也傳遞。`<=`、會變動的 capture、未處理 NaN 都可能破壞契約。
 */

/*
==============================================================================
【面試深挖：Sorting】

A1｜`std::sort` 保證是 quicksort/introsort 嗎？
答：標準保證語意與 O(n log n) comparisons，不規定實作演算法。常見 introsort 是
implementation strategy；面試應把「標準保證」與「libstdc++/libc++ 實作」分開。

A2｜`sort` 與 `stable_sort` 如何選？
答：stable_sort 保留 equivalent elements 的原順序，適合先按次要 key、再按主要 key。
它可能用額外記憶體；記憶體不足時實作可採較慢 fallback。若不需穩定性，sort 通常更省。

A3｜comparator 的 strict weak ordering 包含什麼？
答：comp(x,x)=false、關係具 transitivity，且 equivalence 也具 transitivity。用 <=、
含 NaN 未定 policy、讀取會變動狀態，都可能破壞契約；後果不是只排得不好，而是不受保證。

A4｜為何 `list` 不能用 `std::sort`？
答：std::sort 要 RandomAccessIterator；list 只提供 bidirectional iterator。
應用 list::sort，它透過 relink nodes 排序，不需搬移 element。

A5｜`partial_sort`、`nth_element`、完整 sort 怎麼選？
答：要前 k 個且已排序用 partial_sort，約 O(n log k)；只要第 k 個及兩側 partition 用
nth_element，平均線性；要完整次序才 sort。不要為 top-k 無條件排序 n 個元素。

A6｜`nth_element` 後左右各自有序嗎？
答：沒有。nth 位置是完整排序後該在的值，左邊都不大於它、右邊都不小於它，但群內任意。
若輸出前 k 名還要有序，再 sort 左段。

A7｜多欄排序應寫多次 sort 還是一個 comparator？
答：可用 lexicographical tuple key 一次表達；或 stable_sort 先次要、後主要。
比較器必須處理 tie，且避免重複昂貴 key extraction，可先 decorate-sort-undecorate。

A8｜parallel execution policy 有何例外陷阱？
答：標準 execution-policy overload 中，使用者 function 若丟例外，常會呼叫 terminate
（對標準 policy 的規則需查該 overload）；不可假設像一般 sort 一樣可 catch。

A9｜排序 pointer 時預設比較地址還是 pointed value？
答：比較 pointer value；若想按物件內容，comparator 必須解引用並處理 null/lifetime。
容器內 pointer ownership 不清時，排序只是暴露更大的生命週期問題。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Candidate {
    int id;
    int score;
    int arrival;
};

const auto better_candidate = [](const Candidate& lhs, const Candidate& rhs) {
    if (lhs.score != rhs.score) {
        return lhs.score > rhs.score;
    }
    return lhs.id < rhs.id;
};

// 基礎整合：保留來源，複製 top-K，並驗證結果 sorted。
std::vector<Candidate> basic_select_top(const std::vector<Candidate>& input,
                                        std::size_t k) {
    std::vector<Candidate> output(std::min(k, input.size()));
    const auto end = std::partial_sort_copy(input.begin(), input.end(),
                                            output.begin(), output.end(),
                                            better_candidate);
    output.erase(end, output.end());
    assert(std::is_sorted(output.begin(), output.end(), better_candidate));
    return output;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 75. Sort Colors（顏色分類）
// 題目：原地將只含 0、1、2 的 nums 排成同色相鄰且依 0、1、2 順序；例如
// [2,0,2,1,1,0] 變 [0,0,1,1,2,2]。
// 為何使用本章主題：本章以 std::sort 作 correctness baseline，時間 O(N log N)；
// 正式最佳解應用 counting 或 Dutch national flag 達 O(N)。
// 思路：1. 對完整 nums 直接 sort；2. 原地得到非遞減顏色序列。
// 複雜度：時間 O(N log N)、額外空間通常 O(log N)，N 為 nums 的元素數。
// 易錯點：這不是題目期望的一趟最佳解；通用函式若不信任輸入，應驗證只含 0、1、2。
// -----------------------------------------------------------------------------
void leetcode_sort_colors(std::vector<int>& nums) {
    std::sort(nums.begin(), nums.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Latency p50/p95 摘要
// 情境：唯讀 latency samples 要產生 p50 與 p95，採 floor((N-1)*p) 且不插值；空值、
// 非有限樣本或非法 p 必須拒絕。
// 為何使用本章主題：select_quantile 在副本上以 nth_element 平均線性定位單一分位；
// practical wrapper 呼叫兩次，保留原輸入順序並明確固定 quantile 定義。
// 設計：1. 驗證樣本與 p；2. 計算 floor index；3. nth_element 取得值；4. 分別求 p50/p95。
// 成本：平均時間 O(N)、額外空間 O(N)，N 為樣本數；實際建立兩份副本並各選取一次。
// 上線注意：大量 quantile 應考慮一次 sort 或 sketch；NaN 會破壞排序契約，API 目前以例外拒絕。
// -----------------------------------------------------------------------------
struct LatencySummary {
    double p50;
    double p95;
};

double select_quantile(std::vector<double> samples, double p) {
    if (samples.empty()) {
        throw std::invalid_argument("select_quantile: samples must not be empty");
    }
    if (!std::isfinite(p) || p < 0.0 || p > 1.0) {
        throw std::invalid_argument("select_quantile: p must be finite and in [0, 1]");
    }
    if (!std::all_of(samples.begin(), samples.end(), [](double sample) {
            return std::isfinite(sample);
        })) {
        throw std::invalid_argument("select_quantile: every sample must be finite");
    }
    const std::size_t index = static_cast<std::size_t>(
        p * static_cast<double>(samples.size() - 1U));
    const auto nth = samples.begin() + static_cast<std::ptrdiff_t>(index);
    std::nth_element(samples.begin(), nth, samples.end());
    return *nth;
}

LatencySummary practical_latency_summary(const std::vector<double>& samples) {
    return {select_quantile(samples, 0.50), select_quantile(samples, 0.95)};
}

int main() {
    const std::vector<Candidate> candidates{{3, 80, 0}, {2, 95, 1},
                                            {1, 95, 2}, {4, 70, 3}};
    const auto top = basic_select_top(candidates, 2U);
    assert(top.size() == 2U && top[0].id == 1 && top[1].id == 2);
    assert(candidates[0].id == 3);  // partial_sort_copy 不改來源。

    std::vector<int> colors{2, 0, 2, 1, 1, 0};
    leetcode_sort_colors(colors);
    assert((colors == std::vector<int>{0, 0, 1, 1, 2, 2}));

    const LatencySummary latency = practical_latency_summary({10, 20, 30, 40, 50});
    assert(std::abs(latency.p50 - 30.0) < 1e-12);
    assert(std::abs(latency.p95 - 40.0) < 1e-12); // floor(3.8)=3。

    const auto rejects_quantile = [](std::vector<double> samples, double p) {
        try {
            static_cast<void>(select_quantile(std::move(samples), p));
        } catch (const std::invalid_argument&) {
            return true;
        }
        return false;
    };
    assert(rejects_quantile({}, 0.5));
    assert(rejects_quantile({1.0}, -0.01));
    assert(rejects_quantile({1.0}, 1.01));
    assert(rejects_quantile({1.0}, std::numeric_limits<double>::infinity()));
    assert(rejects_quantile({1.0}, std::numeric_limits<double>::quiet_NaN()));
    assert(rejects_quantile({std::numeric_limits<double>::quiet_NaN()}, 0.5));

    std::vector<Candidate> fifo{{1, 5, 0}, {2, 9, 1}, {3, 5, 2}};
    std::stable_sort(fifo.begin(), fifo.end(),
                     [](const Candidate& lhs, const Candidate& rhs) {
                         return lhs.score > rhs.score;
                     });
    assert(fifo[1].id == 1 && fifo[2].id == 3);

    std::cout << "Sorting summary：top-K、LC75、quantile 與穩定排序測試通過\n";
}

/*
 * 【陷阱總表】
 * 1. comparator 絕不可用 <=；comp(x,x) 必須 false。
 * 2. sort 不穩定；等價項相對順序不可寫死在測試。
 * 3. stable_sort 穩定但可能配置 buffer；不是免費替代。
 * 4. nth_element 只有 *nth 與兩側 partition 保證，兩側內部不排序。
 * 5. partial_sort 的 middle 是前 K 區間尾端，不包含 middle。
 * 6. partial_sort_copy 的 K 由 output size 決定，reserve 不等於 size。
 * 7. partial_sort_copy 回傳實際 output end；input 少於 K 時尾端未寫。
 * 8. is_sorted 允許 duplicate；嚴格遞增需另檢查 adjacent equality。
 * 9. is_sorted_until 回第一個造成 inversion 的後元素。
 * 10. 所有原地排序都改變位置 identity，即使 iterator storage 未失效。
 * 11. float NaN 讓普通 `<` 無法形成使用者期待的 total order；先清洗或自訂規則。
 * 12. comparator 不應有副作用或讀取會變動狀態；呼叫次數/順序未指定。
 * 13. distance squared 要升格後再乘，避免 int overflow。
 * 14. percentile 有多種定義；API 必須記錄 index/插值規則。
 *
 * 【面試快問快答】
 * Q: top-K 怎麼選？
 * A: K 小且 batch、前 K 要有序用 partial_sort；只要 threshold 用 nth_element；
 *    streaming 用固定大小 heap；K 接近 N 可直接 sort。
 * Q: sort 複雜度保證？
 * A: O(N log N) comparisons；常見 introsort 只是實作策略。
 * Q: nth_element 與 stable_sort 的複雜度保證？
 * A: nth_element 是平均 O(N) comparisons，不是最壞 O(N)；stable_sort 有足夠額外
 *    buffer 時 O(N log N)，配置不到 buffer 時仍穩定，但為 O(N log^2 N)。
 * Q: stable 的用途？
 * A: 等價鍵保留先前順序，例如 FIFO、連續多鍵排序、UI 穩定顯示。
 * Q: nth_element 後第 K 前都是小於 *nth 嗎？
 * A: 依 comparator 保證前半沒有元素應排在 nth 之後；duplicate/equivalence 要精確
 *    表述，不能武斷說全部嚴格小於。
 * Q: 為何 comparator 要 strict weak ordering？
 * A: 排序需要一致可傳遞的「在前」關係；違反時演算法行為不具可靠契約。
 * Q: 何時排序 index？
 * A: payload 大、不可搬、需多種 view 或保留來源；代價是間接存取與 locality。
 *
 * 【選型流程】
 * 先問只驗證嗎？是 -> is_sorted。否則只需 order statistic -> nth_element。
 * 需要 K 個且 K<N：前 K 要排序 -> partial_sort/copy；來源要保留就 copy。
 * 需要全部排序：等價順序有語意 -> stable_sort；否則 sort。
 *
 * 【實務工程】
 * - 排序前明確定義 null、NaN、大小寫、locale、tie-breaker；本檔 latency API 對
 *   empty、非 finite sample，以及非 finite/超出 [0,1] 的 p 一律丟 invalid_argument。
 * - 分散式結果要 deterministic，加入唯一 id 作最後鍵。
 * - 大型物件排序 index，完成後視 workload 決定是否 materialize contiguous result。
 * - trust boundary 驗證 sorted invariant；內部由型別/封裝維持，不必每次重掃。
 * - benchmark 需包含 realistic comparator cost 與搬移成本，不只隨機 int。
 *
 * 【面試前自問】
 * - 能否在 30 秒選出 sort/partial/nth/heap？
 * - 能否說明每個演算法完成後「哪些區域有序」？
 * - 能否寫一個具有完整 tie-breaker 的 comparator？
 * - 能否指出 quantile 定義與偶數 median 處理？
 * - 能否測 empty、K=0、K=N、all equal、already sorted、reverse sorted？
 *
 * 練習：
 * 1. LC75 改 Dutch national flag O(N)，與 sort baseline 交叉測試。
 * 2. 實作 streaming top-K min-heap，分析 O(N log K) 與記憶體 O(K)。
 * 3. 對 NaN 定義「一律排最後」的 total-order comparator，測正負零與 infinity。
 * 4. 用兩次 nth_element 求偶數 median，避免中間值相加 overflow。
 * 5. property-test comparator 的 irreflexive/asymmetric/transitive。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Algorithm_sorting_summary' && '/tmp/codex_cpp_C_Algorithm_sorting_summary'
//
// === 預期輸出（節錄）===
// Sorting summary：top-K、LC75、quantile 與穩定排序測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
