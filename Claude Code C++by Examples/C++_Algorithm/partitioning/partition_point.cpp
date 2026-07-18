// ============================================================
// std::partition_point   (C++11 起)
// 分類 (Category): Partitioning operations (分割演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partition_point
//   * https://cplusplus.com/reference/algorithm/partition_point/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::partition_point 解的問題:
//
//   「給我一段『已分割』的範圍,告訴我『分界線在哪』。」
//
// 它使用「二分搜尋」找到分界,O(log N)。
//
// 「已分割」是前提 — 也就是滿足述詞 p 的元素必須全部在前段,
// 不滿足的全部在後段。否則行為未定義 (UB)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、partition vs partition_point 的差別                    │
// └────────────────────────────────────────────────────────────┘
//
//   * partition       : 主動「重排」資料,使其變成已分割。回傳分界點。
//                       O(N) 時間。
//   * partition_point : 假設資料「已經」分割,只用二分找出分界點。
//                       O(log N) 時間。
//
// 兩者的回傳值「意義相同」 — 都是「第一個不滿足述詞的位置」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼這個函式重要?                                   │
// └────────────────────────────────────────────────────────────┘
//
// 因為「單調 (monotonic) 的資料 + 條件搜尋」是一個非常通用的模式:
//
//   * LC 278 First Bad Version: [good, good, ..., bad, bad, ...] 找第一個 bad
//   * LC 852 Mountain Peak: 上升段 / 下降段的「轉折點」
//   * 系統「狀態變化點」: OK → FAIL 的時刻
//   * 二分答案的核心: 「滿足條件 → 不滿足條件」的邊界值
//
// 任何這類「true...true | false...false」結構,都可以用 partition_point。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、和 lower_bound/upper_bound 的關係                       │
// └────────────────────────────────────────────────────────────┘
//
// lower_bound 是 partition_point 的「特例」:
//
//   std::lower_bound(first, last, value)
//   == std::partition_point(first, last, [v=value](const T& x){ return x < v; })
//
// upper_bound 同理 (述詞為 x <= v)。partition_point 更通用 —
// 你可以放任何單調述詞,不限於「< value」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt, class UnaryPred>
//   FwdIt partition_point(FwdIt first, FwdIt last, UnaryPred p);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、回傳值與複雜度                                         │
// └────────────────────────────────────────────────────────────┘
//
//   回傳: 第一個不滿足 p 的位置 (與 partition 的回傳值同義)
//
//   時間: O(log N) 述詞呼叫
//         RandomAccessIt 為真 O(log N);
//         ForwardIt 述詞 O(log N) 但步進總計 O(N)。
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. **必須**確保範圍已分割,否則 UB!不確定就先 is_partitioned 檢查。
//   2. 全部滿足 → 回 last;全部不滿足 → 回 first。
//   3. 對排序好的範圍,等同 lower_bound 的「條件版」。
//   4. 適合在二分答案、單調序列搜尋等場景。
//
// ============================================================

/*
補充筆記：std::partition_point
  - std::partition_point 依 predicate 把資料分成 true 區和 false 區；partition 後同一區內相對順序不一定保留。
  - stable_partition 會保留相對順序，但通常需要更多移動或額外空間；是否穩定要依需求決定。
  - partition_point 只能用在已 partitioned 的範圍；若資料沒有先分區，結果不可靠。
  - predicate 應對同一元素回傳穩定結果；若 predicate 依外部可變狀態改變，分區結果很難推理。
  - partition_copy 需要兩個輸出範圍，分別接 true 和 false 元素；目的地容量仍由呼叫者負責。
  - 分區不是排序；它只保證條件分界，不保證每一區內元素大小順序。
  - std::partition_point 的 predicate 就是分界線定義；先決定哪些元素應放在 true 區，再看函式是否保留相對順序。
  - std::partition_point 完成後資料通常只保證被分成兩段，不保證每段內已排序；把 partition 當 sort 使用會得到錯誤假設。
  - std::partition_point 若回傳 iterator，它通常代表 true 區結尾或第一個 false 位置；使用前要把這個位置當成半開區間邊界理解。
*/
#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 已分割 — 偶數在前,奇數在後 ---
    std::vector<int> v{2, 4, 6, 8, 1, 3, 5, 7};
    auto it = std::partition_point(v.begin(), v.end(),
                                   [](int x){ return x % 2 == 0; });
    std::cout << "boundary at index " << (it - v.begin())
              << ", first odd = " << *it << '\n';

    // --- 範例 2: 對排序陣列模擬 lower_bound ---
    std::vector<int> sorted{1, 3, 5, 7, 9, 11};
    int target = 6;
    auto p = std::partition_point(sorted.begin(), sorted.end(),
                                  [target](int x){ return x < target; });
    std::cout << "first elem >= " << target << " is " << *p
              << " (at index " << (p - sorted.begin()) << ")\n";

    // --- 範例 3: 全部都滿足 → 回傳 last ---
    std::vector<int> all_even{2, 4, 6};
    auto e = std::partition_point(all_even.begin(), all_even.end(),
                                  [](int x){ return x % 2 == 0; });
    std::cout << "all-even pp == end: "
              << std::boolalpha << (e == all_even.end()) << '\n';

    // --- 範例 4: 全部不滿足 → 回傳 first ---
    std::vector<int> all_odd{1, 3, 5};
    auto f = std::partition_point(all_odd.begin(), all_odd.end(),
                                  [](int x){ return x % 2 == 0; });
    std::cout << "all-odd pp == begin: "
              << (f == all_odd.begin()) << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_278_first_bad_version();
    void leetcode_35_search_insert_position();
    void practical_find_state_transition();
    void leetcode_852_peak_mountain_array();
    void practical_first_overflow_in_log();
    leetcode_278_first_bad_version();
    leetcode_35_search_insert_position();
    practical_find_state_transition();
    leetcode_852_peak_mountain_array();
    practical_first_overflow_in_log();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 278: 第一個錯誤的版本 (First Bad Version)
// ----------------------------------------------------------------
// 題目:版本 1..n,某版本後皆為 bad。給 isBadVersion(v),
//      找第一個 bad 的版本。資料形如:
//      [good, good, ..., good, bad, bad, ..., bad]
//      ← 完全是「已分區」結構!
//
// 為什麼用 std::partition_point:
//   述詞 = 「is good」,partition_point 回傳「第一個 not good」 = 第一個 bad。
//   底層為二分搜尋,O(log n) — 與題目期望的最小 API 呼叫次數一致。
//
// 解法步驟:
//   1. 用 vector<int> {1..n} 模擬版本號。
//   2. 述詞 lambda: v < first_bad (good 的條件)。
//   3. partition_point 回傳的迭代器即為第一個 bad。
//
// 複雜度:時間 O(log n),空間 O(1)。
void leetcode_278_first_bad_version() {
    int n = 10, first_bad = 7;
    std::vector<int> versions(n);
    for (int i = 0; i < n; ++i) versions[i] = i + 1;
    auto it = std::partition_point(versions.begin(), versions.end(),
                                   [first_bad](int v){ return v < first_bad; });
    std::cout << "LC278 first bad version = " << *it << '\n';
}

// ----------------------------------------------------------------
// LeetCode 35: 搜尋插入位置 (Search Insert Position)
// ----------------------------------------------------------------
// 題目:給已排序陣列 nums 與 target,回傳 target 應該插入的位置。
//
// 為什麼用 std::partition_point:
//   排序資料就是「自然分區」 — 用 [x < target] 當述詞,
//   partition_point 給的就是「第一個 >= target」的位置 = 答案。
//   等同於 std::lower_bound,但這裡用 partition_point 凸顯
//   它在「條件型二分」的通用性。
//
// 複雜度:時間 O(log n),空間 O(1)。
void leetcode_35_search_insert_position() {
    std::vector<int> nums{1, 3, 5, 6};
    int target = 5;
    auto it = std::partition_point(nums.begin(), nums.end(),
                                   [target](int x){ return x < target; });
    std::cout << "LC35 insert position for " << target
              << " = " << (it - nums.begin()) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:找系統「狀態轉換點」
// ----------------------------------------------------------------
// 場景:監控紀錄按時序排列,系統前段 OK,後段 FAIL。
//      要找出「第一次故障」的時間點以便寫入告警事件。
//      資料天然「已分區」(按時序),所以可以二分搜尋。
//
// 為什麼用 std::partition_point:
//   * O(log n) 比 O(n) 線性掃描快得多。
//   * 可以套用在任何「狀態突變一次」的時序資料上。
void practical_find_state_transition() {
    enum class S { OK, FAIL };
    struct Event { int t; S s; };
    std::vector<Event> log{
        {1, S::OK}, {2, S::OK}, {3, S::OK}, {4, S::OK},
        {5, S::FAIL}, {6, S::FAIL}, {7, S::FAIL}
    };
    auto it = std::partition_point(log.begin(), log.end(),
                                   [](const Event& e){ return e.s == S::OK; });
    std::cout << "Practical first FAIL at t = " << it->t << '\n';
}

// ----------------------------------------------------------------
// LeetCode 852: 山脈陣列的峰頂索引 (Peak Index in a Mountain Array)
// 難度: medium
// ----------------------------------------------------------------
// 題目:山脈陣列嚴格遞增到峰頂,然後嚴格遞減。找峰頂索引。
//
// 為什麼用 std::partition_point:
//   述詞 = 「目前位置仍在上升段」(arr[i] < arr[i+1]) — 滿足分區性質;
//   partition_point 回傳第一個「不滿足」的位置 = 峰頂。
//
// 複雜度:時間 O(log n);空間 O(1)。
void leetcode_852_peak_mountain_array() {
    std::vector<int> arr{0, 2, 4, 6, 8, 5, 3, 1};
    // 把 arr 的 indices 視為被 partitioned 的序列 (述詞依賴外部值)
    std::vector<int> idx(arr.size() - 1);
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    auto it = std::partition_point(idx.begin(), idx.end(),
                                   [&](int i){ return arr[i] < arr[i + 1]; });
    int peak = (it == idx.end()) ? (int)arr.size() - 1 : *it;
    std::cout << "LC852: " << peak << '\n';
}

// ----------------------------------------------------------------
// 實務範例:找排序好的 log entry 中第一個「超過閾值的時戳」
// ----------------------------------------------------------------
// 場景:log 已按時戳排序,要找「第一個 > deadline」的 entry,
//      之後的 entry 全是「過期未處理」。用 partition_point 二分定位。
void practical_first_overflow_in_log() {
    std::vector<long long> timestamps{100, 200, 300, 400, 500, 600};
    long long deadline = 350;
    auto it = std::partition_point(timestamps.begin(), timestamps.end(),
                                   [deadline](long long t){ return t <= deadline; });
    int pos = (it == timestamps.end()) ? -1 : static_cast<int>(it - timestamps.begin());
    std::cout << "first > deadline at idx: " << pos << '\n';
}

// === 預期輸出 (Expected output) ===
// boundary at index 4, first odd = 1
// first elem >= 6 is 7 (at index 3)
// all-even pp == end: true
// all-odd pp == begin: true
// LC278 first bad version = 7
// LC35 insert position for 5 = 2
// Practical first FAIL at t = 5
// LC852: 4
// first > deadline at idx: 3
