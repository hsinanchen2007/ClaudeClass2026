// ============================================================
// std::inplace_merge
// 分類 (Category): Set operations on sorted ranges (有序範圍合併)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/inplace_merge
//   * https://cplusplus.com/reference/algorithm/inplace_merge/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::inplace_merge 解的問題:
//
//   「同一個容器中,『前段已排序』(first..middle) + 『後段已排序』(middle..last),
//    把整段合併成單一排序範圍 — 就地完成,不必另外配置容器。」
//
// 它是 std::merge 的「就地版」 — 兩段在「同一塊記憶體」內。
// 對「Merge Sort 的合併步驟」「append 新資料後重新合併」這類場景非常實用。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼有時候比 merge 好用?                            │
// └────────────────────────────────────────────────────────────┘
//
//   * std::merge 需要「另外的目的容器」,要先準備 buffer。
//   * std::inplace_merge 直接對原地操作,不必額外管理另一個 vector。
//
// 想像情境:
//   「我已經把資料放在一個 vector 裡;前半段 sort 過,後半段也 sort 過;
//    我希望就地合併,不想多配一塊記憶體。」 → 用 inplace_merge。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、空間代價                                               │
// └────────────────────────────────────────────────────────────┘
//
// inplace_merge 一般會嘗試配置 O(N) 暫存以達到 O(N) 線性合併;
// 若記憶體不足,fall back 到 O(N log N) 的「無暫存版」。
//
//   * 有暫存: O(N) 移動
//   * 無暫存: O(N log N) 移動 (退化版)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、典型應用                                               │
// └────────────────────────────────────────────────────────────┘
//
//   * Merge Sort 的合併步驟 (見範例)
//   * 「append 新資料後重排」 — 比整體 sort 快,當 N >> k 時尤其
//   * LC 88 變體 (用 inplace_merge 解,把 nums2 放到尾段再合併)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class BidirIt>
//   void inplace_merge(BidirIt first, BidirIt middle, BidirIt last);
//
//   template <class BidirIt, class Compare>
//   void inplace_merge(BidirIt first, BidirIt middle, BidirIt last,
//                      Compare comp);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、需求與穩定性                                           │
// └────────────────────────────────────────────────────────────┘
//
//   需求:BidirectionalIterator;兩段子範圍各自已依 comp 排序。
//   穩定性:相等元素中,前段拷貝排在後段拷貝之前。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. middle 必須在 [first, last] 內;兩段都要已排序。
//   2. 兩段排序方向必須一致。
//   3. 想避免就地、想保留原資料 → 改用 std::merge。
//
// ============================================================

/*
補充筆記：std::inplace_merge
  - std::inplace_merge 是排序範圍上的集合運算；輸入必須已依同一比較器排序，否則交集、聯集、差集結果會錯。
  - 這些演算法處理的是 sorted range，不是 std::set 專用；vector 排序後也能使用。
  - set_union/set_intersection/set_difference 對重複元素有明確規則，結果數量不是單純去重後集合概念。
  - 輸出範圍要有足夠空間，或使用 back_inserter 讓容器自動追加。
  - merge/inplace_merge 合併的是兩段已排序資料；它們不會替未排序資料做完整排序。
  - 比較器必須與排序時一致；若輸入用小寫排序、運算時用大小寫不敏感比較，結果可能不符合前置條件。
  - std::inplace_merge 的輸入是 sorted range；這裡的 set operation 不是只能用 std::set，而是依排序結果做線性合併。
  - std::inplace_merge 對重複元素有 multiset 語意，例如交集會取兩邊出現次數的較小值，聯集會取較大值。
  - std::inplace_merge 的輸出 iterator 代表寫入結果的結尾；若使用預先配置容器，結束後通常要 erase 多餘尾端。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::inplace_merge
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. inplace_merge 做什麼?和 std::merge 的分工是什麼?
//     答：把「同一個範圍內、以 middle 為界的兩段各自已排序的子範圍」就地合併成
//         一段有序資料,不需要另外準備目的容器。std::merge 則是讀兩個(可以來自
//         不同容器的)有序範圍、寫到第三個不重疊的目的端。前置條件同樣是兩段都
//         已依同一個 comp 排序,而且方向一致。
//     追問：middle 的合法範圍?(答：必須落在 [first, last] 內,兩段各自已排序)
//
// 🔥 Q2. 為什麼說它是 merge sort 的核心步驟?
//     答：merge sort = 遞迴排序左右兩半 + 合併。左右兩半排好後,整個範圍剛好就是
//         「兩段相鄰的有序子範圍」— 正是 inplace_merge 的輸入形式,所以自製
//         merge sort 只要遞迴兩次再呼叫它一次即可(本檔 my_merge_sort 就是)。
//         同樣的模式也適用於「已排序主清單 append 一小段新資料後重新合併」,
//         O(N + k) 比整體重排 O((N+k) log (N+k)) 划算。
//
// Q3. 「in place」是不是代表完全不配置記憶體?
//     答：不是。實作通常會嘗試配置一塊暫存以達到線性次數的合併;若配置不到,
//         會退化成 O(N log N) 次比較的無暫存版本 — 結果仍然正確,只是變慢。
//         所以它是「不需要你自己準備目的容器」,不是「保證零額外配置」。
//     追問：迭代器需求?(答：BidirectionalIterator 即可,比 std::sort 要求的
//           random access 寬鬆;而且它是穩定的 — 等價元素中前段的拷貝排在前面)
//
// ⚠️ 陷阱. 對一個完全未排序的 vector 呼叫 inplace_merge,會幫我排好嗎?
//     答：不會。它不是排序演算法,只保證「把兩段『已經有序』的資料歸併」。
//         餵未排序資料是違反前置條件的 UB,不會報錯,輸出只是一團無意義的順序。
//     為什麼會錯：名字裡有 merge、又會「就地重排元素」,容易被當成某種排序;
//         實際上兩段子範圍是否有序,得由呼叫端自己保證(所以範例才先各 sort 一次)。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 兩段合併 ---
    std::vector<int> v{1, 3, 5, 7,    2, 4, 6, 8};
    std::inplace_merge(v.begin(), v.begin() + 4, v.end());
    std::cout << "inplace_merge: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 自己 sort 兩段然後合併 ---
    std::vector<int> w{8, 1, 5, 3, 9, 2, 7, 4, 6};
    auto mid = w.begin() + w.size() / 2;
    std::sort(w.begin(), mid);
    std::sort(mid, w.end());
    std::cout << "two halves sorted: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';
    std::inplace_merge(w.begin(), mid, w.end());
    std::cout << "after inplace_merge: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 自訂 comp (降序) ---
    std::vector<int> d{9, 7, 5,  8, 6, 1};
    std::inplace_merge(d.begin(), d.begin() + 3, d.end(),
                       std::greater<int>{});
    std::cout << "desc: ";
    for (int x : d) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_88_merge_sorted_array_inplace();
    void merge_sort_concept_demo();
    void practical_append_then_inplace_merge();
    void leetcode_315_count_smaller_concept();
    void practical_streaming_sorted_batches();
    leetcode_88_merge_sorted_array_inplace();
    merge_sort_concept_demo();
    practical_append_then_inplace_merge();
    leetcode_315_count_smaller_concept();
    practical_streaming_sorted_batches();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 88: 合併兩個有序陣列 (inplace_merge 版)
// ----------------------------------------------------------------
// 題目:nums1 (大小 m+n,前 m 為有效) + nums2 (大小 n),合併進 nums1 並排序。
//
// 為什麼用 std::inplace_merge:
//   先把 nums2 直接放到 nums1 的尾段 (覆蓋原本佔位的 0),此時 nums1 內形成
//   「[0, m) 已排序、[m, m+n) 已排序」 — 標準的 inplace_merge 輸入。
//
// 複雜度:時間 O(m+n);空間 O(m+n) (STL 內部緩衝)。
void leetcode_88_merge_sorted_array_inplace() {
    std::vector<int> nums1{1, 2, 3, 0, 0, 0};
    int m = 3;
    std::vector<int> nums2{2, 5, 6};
    int n = 3;
    std::copy(nums2.begin(), nums2.end(), nums1.begin() + m);
    std::inplace_merge(nums1.begin(), nums1.begin() + m, nums1.begin() + m + n);
    std::cout << "LC88 inplace: ";
    for (int x : nums1) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// Merge Sort 的合併步驟概念示範
// ----------------------------------------------------------------
// 自製 merge sort 時,核心是「分而治之 + 合併」。合併步驟用 inplace_merge
// 可省掉手寫雙指標合併的麻煩,程式碼簡短易讀。
//
// 複雜度:整體 O(N log N)。
static void my_merge_sort(std::vector<int>& v, int lo, int hi) {
    if (hi - lo <= 1) return;
    int mid = lo + (hi - lo) / 2;
    my_merge_sort(v, lo, mid);
    my_merge_sort(v, mid, hi);
    std::inplace_merge(v.begin() + lo, v.begin() + mid, v.begin() + hi);
}
void merge_sort_concept_demo() {
    std::vector<int> v{5, 2, 8, 1, 9, 3, 7, 4, 6};
    my_merge_sort(v, 0, static_cast<int>(v.size()));
    std::cout << "merge_sort: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:append 新資料後 inplace_merge
// ----------------------------------------------------------------
// 場景:系統維護一個排序好的歷史資料 vector。批次新進一小段資料時,
//      先對「新進那段」單獨排序,append 到後面,再 inplace_merge —
//      比整體 sort O((N+k) log (N+k)) 更省 (尤其 N >> k 時)。
//
// 複雜度:排新段 O(k log k) + 合併 O(N+k)。
void practical_append_then_inplace_merge() {
    std::vector<int> data{1, 4, 6, 9, 12, 15};            // 已排序
    std::vector<int> incoming{8, 2, 13, 5};               // 本次新進
    auto old_end = data.size();
    data.insert(data.end(), incoming.begin(), incoming.end());
    std::sort(data.begin() + old_end, data.end());
    std::inplace_merge(data.begin(), data.begin() + old_end, data.end());
    std::cout << "append+merge: ";
    for (int x : data) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 315 概念:右側比自己小的數量 — merge_sort 思路示範
// 難度: hard
// ----------------------------------------------------------------
// 題目:LC 315 是用 merge sort 計算「右側比自己小的元素個數」。
//      最佳解需在 merge step 累計貢獻;這裡示範 inplace_merge 在
//      自製 merge sort 中的角色 (作為合併的工具)。
//
// 為什麼用 std::inplace_merge:
//   把 merge sort 寫得很短 — 兩半遞迴排序 + inplace_merge 合併。
//
// 複雜度:時間 O(n log n);空間 O(n)。
void leetcode_315_count_smaller_concept() {
    std::vector<int> arr{5, 2, 6, 1};
    // 簡化版:只 demo inplace_merge 在 merge_sort 中的角色
    std::vector<int> v = arr;
    // 對前半排序、後半排序、再 inplace_merge
    int mid = v.size() / 2;
    std::sort(v.begin(), v.begin() + mid);
    std::sort(v.begin() + mid, v.end());
    std::inplace_merge(v.begin(), v.begin() + mid, v.end());
    std::cout << "LC315-demo (sorted):";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:串流資料 — 多批次到達後合併到主清單
// ----------------------------------------------------------------
// 場景:資料流每批次到達都已自行排序,主清單也排序;
//      每次只需 inplace_merge 一段即可,O(N+k) 比整體 sort 更省。
void practical_streaming_sorted_batches() {
    std::vector<int> master{1, 3, 5, 7, 9};
    std::vector<int> batch1{2, 4, 6};
    std::vector<int> batch2{0, 8, 10};
    for (auto& batch : {batch1, batch2}) {
        auto old_end = master.size();
        master.insert(master.end(), batch.begin(), batch.end());
        std::inplace_merge(master.begin(), master.begin() + old_end, master.end());
    }
    std::cout << "master:";
    for (int x : master) std::cout << ' ' << x;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// inplace_merge: 1 2 3 4 5 6 7 8
// two halves sorted: 1 3 5 8 2 4 6 7 9
// after inplace_merge: 1 2 3 4 5 6 7 8 9
// desc: 9 8 7 6 5 1
// LC88 inplace: 1 2 2 3 5 6
// merge_sort: 1 2 3 4 5 6 7 8 9
// append+merge: 1 2 4 5 6 8 9 12 13 15
// LC315-demo (sorted): 1 2 5 6
// master: 0 1 2 3 4 5 6 7 8 9 10
