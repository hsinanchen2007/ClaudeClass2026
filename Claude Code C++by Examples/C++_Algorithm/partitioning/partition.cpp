// ============================================================
// std::partition
// 分類 (Category): Partitioning operations (分割演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partition
//   * https://cplusplus.com/reference/algorithm/partition/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::partition 解的問題:
//
//   「給我一段資料和一個述詞 p。把所有符合 p 的元素重排到前段,
//    所有不符合的重排到後段。回傳『分界線』。」
//
// 結果像這樣:
//
//   [█████符合p█████ | □□□□□不符合p□□□□□]
//                    ↑ 回傳的迭代器指這裡 (後段第一個位置)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼 partition 是個重要演算法?                       │
// └────────────────────────────────────────────────────────────┘
//
// partition 是「Quicksort」的核心步驟。Quicksort 的本質就是:
//
//   1. 選一個 pivot。
//   2. partition 整個陣列成 [< pivot] [>= pivot]。
//   3. 對兩半遞迴 quicksort。
//
// 所以你看到的所有 std::sort、std::nth_element 內部,大量使用
// partition 的概念。它也是「Dutch National Flag (LC 75 顏色排序)」、
// 「Quickselect」等經典演算法的基礎。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、partition vs stable_partition vs partition_copy        │
// └────────────────────────────────────────────────────────────┘
//
//   ┌─────────────────────┬──────────────────────────────────┐
//   │ partition           │ 原地,「不」保留組內相對順序     │
//   │ stable_partition    │ 原地,「保留」組內相對順序        │
//   │ partition_copy      │ 不改原資料,把兩組分別寫到兩個輸出│
//   └─────────────────────┴──────────────────────────────────┘
//
// 「不保留順序」聽起來很糟,但它換來:
//   * 不需要 O(N) 暫存空間。
//   * O(N/2) 次 swap (RandomAccess)。
//
// 真的需要保序就用 stable_partition (代價是要 O(N) 空間)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class UnaryPred>
//   FwdIt partition(FwdIt first, FwdIt last, UnaryPred p);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   分割點:指向「第一個不滿足 p 的元素」(後段的開頭)。
//
//   從這個迭代器你可以推導出:
//     * 滿足 p 的元素數量 = distance(first, 分割點)
//     * 不滿足 p 的元素數量 = distance(分割點, last)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間 (RandomAccessIt): N 次述詞呼叫,最多 N/2 次 swap
//   時間 (ForwardIt):     N 次述詞呼叫,最多 N 次 swap
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 「不保證」相對順序!需要保序請用 stable_partition。
//   2. 想找分割點而不重排,且範圍已分割 → partition_point (O(log N))。
//   3. 想保留原資料同時得到兩組 → partition_copy。
//   4. 對 forward-only 的容器 (例如 std::forward_list) 也適用。
//
// ============================================================

/*
補充筆記：std::partition
  - std::partition 依 predicate 把資料分成 true 區和 false 區；partition 後同一區內相對順序不一定保留。
  - stable_partition 會保留相對順序，但通常需要更多移動或額外空間；是否穩定要依需求決定。
  - partition_point 只能用在已 partitioned 的範圍；若資料沒有先分區，結果不可靠。
  - predicate 應對同一元素回傳穩定結果；若 predicate 依外部可變狀態改變，分區結果很難推理。
  - partition_copy 需要兩個輸出範圍，分別接 true 和 false 元素；目的地容量仍由呼叫者負責。
  - 分區不是排序；它只保證條件分界，不保證每一區內元素大小順序。
  - std::partition 的 predicate 就是分界線定義；先決定哪些元素應放在 true 區，再看函式是否保留相對順序。
  - std::partition 完成後資料通常只保證被分成兩段，不保證每段內已排序；把 partition 當 sort 使用會得到錯誤假設。
  - std::partition 若回傳 iterator，它通常代表 true 區結尾或第一個 false 位置；使用前要把這個位置當成半開區間邊界理解。
*/
#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 4, 2, 5, 3, 6, 7, 8};

    // --- 範例 1: 把奇數放前面、偶數放後面 ---
    auto mid = std::partition(v.begin(), v.end(),
                              [](int x){ return x % 2 != 0; });
    std::cout << "after partition: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\n  pivot at index " << (mid - v.begin()) << '\n';

    std::cout << "  odd part: ";
    for (auto it = v.begin(); it != mid; ++it) std::cout << *it << ' ';
    std::cout << "\n  even part: ";
    for (auto it = mid; it != v.end(); ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // --- 範例 2: 把大於某值的元素放前面 ---
    std::vector<int> w{1, 2, 3, 4, 5, 6, 7, 8};
    int pivot = 4;
    auto m2 = std::partition(w.begin(), w.end(),
                             [pivot](int x){ return x > pivot; });
    std::cout << "great-than-" << pivot << " at front: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << "(boundary at " << (m2 - w.begin()) << ")\n";

    // === LeetCode / 實務範例 ===
    void leetcode_905_sort_array_by_parity();
    void leetcode_75_sort_colors_concept();
    void practical_separate_logs_by_severity();
    void leetcode_2149_rearrange_sign();
    void practical_quickselect_partition();
    leetcode_905_sort_array_by_parity();
    leetcode_75_sort_colors_concept();
    practical_separate_logs_by_severity();
    leetcode_2149_rearrange_sign();
    practical_quickselect_partition();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 905: 按奇偶排序陣列 (Sort Array By Parity)
// ----------------------------------------------------------------
// 題目:給整數陣列 nums,將所有偶數放在奇數之前,任何順序皆可。
//
// 為什麼用 std::partition:
//   題目「任何順序皆可」 — 不需要穩定 — partition 一行解決。
//
// 解法步驟:
//   std::partition(nums.begin(), nums.end(), [](int x){ return x%2==0; });
//
// 複雜度:時間 O(n),空間 O(1) (in-place)。
void leetcode_905_sort_array_by_parity() {
    std::vector<int> nums{3, 1, 2, 4};
    auto mid = std::partition(nums.begin(), nums.end(),
                              [](int x){ return x % 2 == 0; });
    bool front_all_even = std::all_of(nums.begin(), mid,
                                      [](int x){ return x % 2 == 0; });
    bool back_all_odd  = std::all_of(mid, nums.end(),
                                      [](int x){ return x % 2 != 0; });
    std::cout << "LC905 evens-first OK: " << std::boolalpha
              << (front_all_even && back_all_odd)
              << " (boundary @ " << (mid - nums.begin()) << ")\n";
}

// ----------------------------------------------------------------
// LeetCode 75: 顏色排序 (Sort Colors) — 兩次 partition 解
// ----------------------------------------------------------------
// 題目:陣列只含 0、1、2,要求 in-place 排成 [0..., 1..., 2...]。
//      也叫「Dutch National Flag (荷蘭國旗)」問題。
//
// 為什麼用 std::partition (兩次):
//   * 第一次:把所有 0 放到前段。
//   * 第二次:在剩下的部分,把所有 1 放到前段。
//   * 結果就是 [0..., 1..., 2...]。
//   兩次 O(n),總共 O(n)。
//
// 複雜度:時間 O(n),空間 O(1)。
void leetcode_75_sort_colors_concept() {
    std::vector<int> nums{2, 0, 2, 1, 1, 0};
    auto m1 = std::partition(nums.begin(), nums.end(),
                             [](int x){ return x == 0; });
    std::partition(m1, nums.end(),
                   [](int x){ return x == 1; });
    std::cout << "LC75 sorted: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把錯誤 log 與正常 log 分區 (in-place)
// ----------------------------------------------------------------
// 場景:log 緩衝區把 ERROR 移到前段以便集中觸發告警,INFO/DEBUG 留在後段。
//      不在乎組內順序 (反正後續會依時戳 sort) → 用 partition (非 stable)。
void practical_separate_logs_by_severity() {
    enum class Lv { ERR, INFO };
    struct Log { int id; Lv lv; };
    std::vector<Log> logs{
        {1, Lv::INFO}, {2, Lv::ERR}, {3, Lv::INFO},
        {4, Lv::ERR}, {5, Lv::INFO}, {6, Lv::ERR}
    };
    auto mid = std::partition(logs.begin(), logs.end(),
                              [](const Log& l){ return l.lv == Lv::ERR; });
    std::cout << "Practical errors-first count = "
              << (mid - logs.begin()) << " | layout: ";
    for (auto& l : logs)
        std::cout << "{" << l.id << "," << (l.lv == Lv::ERR ? "E" : "I") << "} ";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2149 概念:正負分區 (Rearrange Array by Sign) — partition 版
// 難度: medium
// ----------------------------------------------------------------
// 題目簡化:把正數放前段、負數放後段 (組內順序不要求)。
//
// 為什麼用 std::partition:
//   不要求穩定順序,partition 是 O(n) in-place 的最佳工具。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_2149_rearrange_sign() {
    std::vector<int> nums{3, -1, 5, -2, -3, 7};
    auto mid = std::partition(nums.begin(), nums.end(),
                              [](int x){ return x > 0; });
    std::cout << "LC2149 pos count=" << (mid - nums.begin()) << " seq:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:快速選擇 (Quickselect) — 找第 K 大元素
// ----------------------------------------------------------------
// 場景:Quickselect 的核心步驟就是 partition + 遞歸。
//      雖然 STL 有 nth_element,自己用 partition 寫的版本能加深對演算法理解。
//      這裡示範一輪 partition 後輸出 pivot 位置。
void practical_quickselect_partition() {
    std::vector<int> v{3, 6, 1, 8, 2, 9, 4};
    int pivot = 4;
    auto mid = std::partition(v.begin(), v.end(),
                              [pivot](int x){ return x < pivot; });
    std::cout << "pivot=" << pivot
              << " smaller count=" << (mid - v.begin())
              << " arrangement:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// after partition: 1 7 3 5 4 6 2 8
//   pivot at index 4
//   odd part: 1 7 3 5
//   even part: 4 6 2 8
// great-than-4 at front: 8 7 6 5 4 3 2 1 (boundary at 4)
//   ↑ 順序依實作而定,只保證「前段全 > 4,後段全 <= 4」
// LC905 evens-first OK: true (boundary @ 2)
// LC75 sorted: 0 0 1 1 2 2
// Practical errors-first count = 3 | layout: 各組內順序依實作而定
// LC2149 pos count=3 seq: ... (順序依實作)
// pivot=4 smaller count=3 arrangement: ... (順序依實作)
