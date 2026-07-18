/*
================================================================================
【C++_Algorithm/partitioning/summary.cpp】
本章末總整理 — Partitioning (分割) 家族
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Partitioning_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、什麼是「分割 (Partition)」?
--------------------------------------------------------------------------------
  給定一個一元述詞 (predicate),把 range 切成兩段:
    [first, mid)   — 全部 pred 為 true
    [mid, last)    — 全部 pred 為 false

  分割只關心「條件是否成立」,不涉及大小排序;比完整排序便宜很多。

  典型用途:
    - 把資料分成兩群 (有效/無效、男/女、啟用/停用)。
    - quicksort/quickselect 的核心步驟 (但 STL 提供的是通用工具,
      不是直接的 sort 演算法)。

--------------------------------------------------------------------------------
二、本章涵蓋的 API
--------------------------------------------------------------------------------
  std::partition(first, last, pred)                  → iterator (= mid)
    → in-place 分割;順序不保留。

  std::stable_partition(first, last, pred)           → iterator (= mid)
    → in-place 分割;保留兩段內部各自的相對順序。

  std::is_partitioned(first, last, pred)             → bool
    → 檢查 range 是否已依 pred 分割。

  std::partition_point(first, last, pred)            → iterator (= mid)
    → 在「已分割」range 中,以 binary search 找分界點。
    → 與 lower_bound 是親兄弟,差別只在輸入是 value 還是 predicate。

  std::partition_copy(first, last, d_true, d_false, pred)
    → 把符合者寫到 d_true,不符合者寫到 d_false (不修改原 range)。

--------------------------------------------------------------------------------
三、複雜度
--------------------------------------------------------------------------------
  API                  時間              空間              穩定性
  ──────────────────  ──────────────────  ────────────────  ──────────
  partition            O(N) 比較 + O(N/2) 交換  O(1)         不穩定
  stable_partition     O(N) (記憶體足夠時)      O(N) 暫存    穩定
                       O(N log N) (退化)
  partition_copy       O(N)                     O(1)         穩定
  is_partitioned       O(N)                     O(1)         —
  partition_point      O(log N) 比較            O(1)         —
                       O(N) 迭代器位移          (非 RA 時)

  ★ stable_partition 需要 N 大小的暫存緩衝;若 OS 配置失敗會退化到 O(N log N)。

--------------------------------------------------------------------------------
四、partition_point vs lower_bound 對照
--------------------------------------------------------------------------------
  兩者都做 binary search,前置條件都是「資料已分割/已排序」。

  lower_bound:
      輸入是 value;內部比較用 comp(elem, value)
      找「第一個 !(elem < value)」(即第一個 >= value)。

  partition_point:
      輸入是 predicate;內部用 pred(elem)
      找「第一個 pred(elem) == false」。

  其實 lower_bound 可以看成 partition_point 的特例:
      partition_point(first, last, [&](auto& x){ return x < value; })
      == lower_bound(first, last, value)

--------------------------------------------------------------------------------
五、迭代器需求
--------------------------------------------------------------------------------
  - partition:ForwardIterator (但 BidirectionalIterator 可達更好效能)
  - stable_partition:BidirectionalIterator
  - partition_point:ForwardIterator
                    (RandomAccessIterator 才有 O(log N) 時間;否則 O(N) 迭代)
  - is_partitioned / partition_copy:InputIterator

--------------------------------------------------------------------------------
六、何時用 partition、何時用 stable_partition?
--------------------------------------------------------------------------------
  - 只要「分群」結果不在乎內部順序 → partition (省記憶體、快)。
  - 需要保留原順序 (例如 VIP 名單前置且依註冊時間排) → stable_partition。
  - 不想修改原資料 → partition_copy (寫到兩個目的端)。

--------------------------------------------------------------------------------
七、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. partition_point 前置條件:資料必須已分割。
     - 如果是「乍看排序」但 predicate 結果不單調 → UB。
     - 例如 is_even 的結果順序為 T T F F T → partition_point 結果未定義。

  2. partition 不保證兩邊內部任何順序:
     - 不要把它當 sort 用,也別期望「false 段」會以原順序保留。

  3. stable_partition 需要額外記憶體:
     - 對嵌入式或記憶體受限環境要注意。

  4. partition_copy 兩個目的端:
     - 必須各自有空間 (或用 inserter 包裝)。

  5. predicate 必須是純函式 (deterministic):
     - 同樣的元素呼叫多次 pred 結果要相同;否則演算法行為不可預期。

  6. 不要修改 predicate 內部狀態:
     - is_partitioned/partition 多次呼叫 pred,有狀態 lambda 會被連續觸發。

--------------------------------------------------------------------------------
八、選擇準則速查表
--------------------------------------------------------------------------------
  需求                                          選擇
  ──────────────────────────────────────────  ────────────────────
  分成「符合 / 不符合」兩群 (不在乎順序)         partition
  分群但兩段內部要保留原順序                      stable_partition
  寫到兩個目的端 (不動原)                         partition_copy
  確認資料已分割                                  is_partitioned
  在已分割資料二分搜尋分界點                      partition_point

--------------------------------------------------------------------------------
九、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 過濾錯誤 vs 正常 log → partition_copy 寫到兩檔。
  - VIP 用戶前置處理 (保留註冊順序) → stable_partition。
  - 監控系統的「第一個變壞時間點」 → partition_point。
  - 偵測資料是否照狀態分區存放 → is_partitioned。
  - Bin packing 起步:先 partition 出「大物件」與「小物件」。
================================================================================
*/
#include <algorithm>
#include <iostream>
#include <vector>

static void print(const std::vector<int>& v, const char* label) {
    std::cout << label << ":";
    for (int x : v) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_partition() {
    std::cout << "\n[demo_partition]\n";

    std::vector<int> v{1, 2, 3, 4, 5, 6, 7};
    auto is_even = [](int x) { return (x % 2) == 0; };

    auto mid = std::partition(v.begin(), v.end(), is_even);
    print(v, "  after partition(even first)");
    std::cout << "  evens count=" << (mid - v.begin()) << "\n";

    std::cout << "  is_partitioned? "
              << std::is_partitioned(v.begin(), v.end(), is_even) << "\n";

    // 在已分割的 range 上做 binary search 找分界點
    auto pp = std::partition_point(v.begin(), v.end(), is_even);
    std::cout << "  partition_point index=" << (pp - v.begin()) << "\n";
}

static void demo_stable_partition() {
    std::cout << "\n[demo_stable_partition]\n";
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7};
    auto is_even = [](int x) { return (x % 2) == 0; };

    std::stable_partition(v.begin(), v.end(), is_even);
    print(v, "  after stable_partition (兩段內部各自保留原順序)");
    // 期望:2 4 6 1 3 5 7
}

int main() {
    demo_partition();
    demo_stable_partition();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
