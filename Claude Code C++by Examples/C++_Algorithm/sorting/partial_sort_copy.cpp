// ============================================================
// std::partial_sort_copy
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/partial_sort_copy
//   * https://cplusplus.com/reference/algorithm/partial_sort_copy/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::partial_sort_copy 解的問題:
//
//   「我要從來源裡找出『前 K 名,且排序好』,
//    把它們寫到另一個容器,原來源不要動。」
//
// 它是 partial_sort 的「不變動原資料」版本。最終輸出大小
// 為 min(N, M) — 來源元素數 N 與目的容器大小 M 取小者。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼有時候比 partial_sort 更實用?                    │
// └────────────────────────────────────────────────────────────┘
//
// 三大優勢:
//
//   1. 「來源只需 InputIterator」 — 適合 stream / forward_list /
//      其他單向走訪容器,partial_sort 因要 RandomAccess 沒辦法用。
//   2. 「來源是 const」 — 不能就地修改的情況,partial_sort_copy 是唯一選擇。
//   3. 「想保留原資料給其他用途」 — 例如稽核需要原始順序。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、與 partial_sort 的對照                                 │
// └────────────────────────────────────────────────────────────┘
//
//   ┌──────────────────────┬─────────────────────────────────┐
//   │ partial_sort         │ 就地排序前 K (修改原資料)        │
//   │ partial_sort_copy    │ 不動原資料,輸出寫到目的端       │
//   └──────────────────────┴─────────────────────────────────┘
//
// 兩者都是 O(N log K),選擇取決於「能否修改來源」與「來源迭代器類別」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class InputIt, class RandomIt>
//   RandomIt partial_sort_copy(InputIt first, InputIt last,
//                              RandomIt d_first, RandomIt d_last);
//
//   template <class InputIt, class RandomIt, class Compare>
//   RandomIt partial_sort_copy(InputIt first, InputIt last,
//                              RandomIt d_first, RandomIt d_last,
//                              Compare comp);
//
//   * C++20 起為 constexpr。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值與複雜度                                         │
// └────────────────────────────────────────────────────────────┘
//
//   回傳: d_first + min(N, M),指向「目的範圍中最後寫入位置的下一個」
//   時間: O(N log min(N, M)) — heap-based
//   空間: O(1) (除了輸出空間)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 來源只需 InputIterator,目的需 RandomAccessIterator (內部用 heap)。
//   2. 目的容器若比來源大,實際只填入 N 個 (來源元素數);多餘位置不動。
//   3. 想就地處理 → 改用 partial_sort。
//
// ============================================================

/*
補充筆記：std::partial_sort_copy
  - std::partial_sort_copy 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::partial_sort_copy 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::partial_sort_copy 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::partial_sort_copy 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::partial_sort_copy
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 和 partial_sort 差在哪?什麼場合只能用它?
//     答:它把來源排序後的前 k 個寫到另一個區間,完全不修改來源。兩個場合
//         partial_sort 做不到而它可以:(1) 來源是 const / 唯讀,不能就地重排;
//         (2) 來源只提供 input iterator (例如串流、單向走訪的容器),而
//         partial_sort 要求 random access iterator。目的端則仍需 random access。
//     追問:回傳值是什麼?
//           (指向目的區間中實際寫入內容的尾端,即 d_first + k)
//
// 🔥 Q2. 會輸出幾個元素?k 由誰決定?
//     答:k = min(來源元素數 N, 目的區間大小 M),由兩者取小者決定,
//         複雜度約 O(N log k)。換句話說「目的端的大小就是你要的 K」:
//         想取前 10 名,就把目的容器先弄成 10 個元素。
//
// ⚠️ 陷阱. 目的端只做了 reserve(k) 就呼叫,會發生什麼事?
//     答:什麼都不會寫入,而且不會有任何錯誤訊息。reserve 只增加 capacity,
//         size 仍是 0,於是 d_first == d_last 是空區間 → k = min(N, 0) = 0。
//         必須用 resize(k) (或建立足夠大小的容器) 才有可寫入的元素。
//     為什麼會錯:把 reserve 誤當成「空間準備好了就能直接寫」;演算法只透過
//         iterator 寫入「已存在的元素」,不會、也不能幫你 resize 容器。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> src{7, 2, 9, 1, 5, 8, 3, 6, 4};

    // --- 範例 1: 取前 4 名 (升冪) ---
    std::vector<int> top4(4);
    std::partial_sort_copy(src.begin(), src.end(), top4.begin(), top4.end());
    std::cout << "top4 ascending: ";
    for (int x : top4) std::cout << x << ' ';
    std::cout << '\n';
    std::cout << "src unchanged: ";
    for (int x : src) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 取前 3 名 (降冪) ---
    std::vector<int> top3(3);
    std::partial_sort_copy(src.begin(), src.end(),
                           top3.begin(), top3.end(),
                           std::greater<int>{});
    std::cout << "top3 descending: ";
    for (int x : top3) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 目的容器比來源大 → 只填來源個數 ---
    std::vector<int> small{3, 1, 2};
    std::vector<int> big(5, 0);
    auto e = std::partial_sort_copy(small.begin(), small.end(),
                                    big.begin(), big.end());
    std::cout << "fill big: ";
    for (auto it = big.begin(); it != e; ++it) std::cout << *it << ' ';
    std::cout << "(填入 " << (e - big.begin()) << " 個元素)\n";

    // === LeetCode / 實務範例 ===
    void leetcode_215_kth_largest_copy();
    void practical_topk_report();
    void leetcode_347_top_k_freq_copy();
    void practical_immutable_snapshot_top();
    leetcode_215_kth_largest_copy();
    practical_topk_report();
    leetcode_347_top_k_freq_copy();
    practical_immutable_snapshot_top();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 215: 第 K 大元素 (不破壞原資料版)
// ----------------------------------------------------------------
// 題目:給陣列 nums 與 k,回傳第 k 大的元素。
//
// 為什麼用 std::partial_sort_copy:
//   * 適用「nums 是 const」、「需要保留原始順序」的場景。
//   * 同時得到「前 k 大且排序好」 — topK[k-1] 即為第 k 大。
//
// 複雜度:時間 O(N log K);空間 O(K)。
void leetcode_215_kth_largest_copy() {
    const std::vector<int> nums{3, 2, 3, 1, 2, 4, 5, 5, 6};
    int k = 4;
    std::vector<int> topK(k);
    std::partial_sort_copy(nums.begin(), nums.end(),
                           topK.begin(), topK.end(),
                           std::greater<int>{});
    std::cout << "LC215-copy kth=" << topK[k-1] << " topK:";
    for (int x : topK) std::cout << ' ' << x;
    std::cout << '\n';
    std::cout << "nums unchanged:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:Top-K 報表輸出 (從串流挑出前 K)
// ----------------------------------------------------------------
// 場景:從大型銷售紀錄中挑出「銷售額前 K 名」並依降冪寫入報表;
//      原始資料保留供其他用途。
//
// 為什麼用 std::partial_sort_copy:
//   * 來源單向迭代即可,適合串流/forward_list。
//   * 不變動原資料。
//   * 一行完成「找出 + 排序 + 輸出」。
void practical_topk_report() {
    struct Sale { std::string product; int amount; };
    std::vector<Sale> all_sales{
        {"A", 120}, {"B", 80}, {"C", 200}, {"D", 50},
        {"E", 175}, {"F", 95}, {"G", 230}, {"H", 60},
    };
    int k = 3;
    std::vector<Sale> report(k);
    std::partial_sort_copy(all_sales.begin(), all_sales.end(),
                           report.begin(), report.end(),
                           [](const Sale& a, const Sale& b){ return a.amount > b.amount; });
    std::cout << "report top" << k << ":";
    for (auto& s : report) std::cout << " " << s.product << "(" << s.amount << ")";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 347 變體:從 const 資料中找 top K 高頻 (partial_sort_copy)
// ----------------------------------------------------------------
// 題目:來源資料不可變動,要取「前 K 高頻」並有序輸出。
//
// 為什麼用 std::partial_sort_copy:
//   不破壞原始資料 (例如記憶體中的 cache 物件),
//   一次取出排序好的 top K。
//
// 複雜度:時間 O(N log K);空間 O(K)。
void leetcode_347_top_k_freq_copy() {
    const std::vector<int> nums{1, 1, 1, 2, 2, 3, 3, 3, 3, 4};
    std::unordered_map<int, int> freq;
    for (int x : nums) ++freq[x];
    std::vector<std::pair<int, int>> arr(freq.begin(), freq.end());
    int k = 2;
    std::vector<std::pair<int, int>> top(k);
    std::partial_sort_copy(arr.begin(), arr.end(),
                           top.begin(), top.end(),
                           [](const auto& a, const auto& b){ return a.second > b.second; });
    std::cout << "LC347-copy top" << k << ":";
    for (auto& p : top) std::cout << " " << p.first << "(" << p.second << ")";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:對「不可變 snapshot」取前 N 名
// ----------------------------------------------------------------
// 場景:資料庫返回一份不可變的 read-only snapshot,要從中找「前 5 名」
//      但不能修改 snapshot 本身 — partial_sort_copy 直接適用。
void practical_immutable_snapshot_top() {
    const std::vector<int> snapshot{72, 85, 90, 65, 95, 88, 78, 92, 80};
    int n = 3;
    std::vector<int> top(n);
    std::partial_sort_copy(snapshot.begin(), snapshot.end(),
                           top.begin(), top.end(),
                           std::greater<int>{});
    std::cout << "top " << n << " scores:";
    for (int s : top) std::cout << ' ' << s;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra partial_sort_copy.cpp -o partial_sort_copy

// === 預期輸出 ===
// top4 ascending: 1 2 3 4
// src unchanged: 7 2 9 1 5 8 3 6 4
// top3 descending: 9 8 7
// fill big: 1 2 3 (填入 3 個元素)
// LC215-copy kth=4 topK: 6 5 5 4
// nums unchanged: 3 2 3 1 2 4 5 5 6
// report top3: G(230) C(200) E(175)
// LC347-copy top2: 3(4) 1(3)
// top 3 scores: 95 92 90
