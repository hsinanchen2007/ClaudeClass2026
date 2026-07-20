// ============================================================
// std::for_each_n   (C++17 起)
// 分類 (Category): Non-modifying sequence operations (非修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/for_each_n
//   * https://cplusplus.com/reference/algorithm/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::for_each_n 是 C++17 引入的小工具,本質就是 for_each 的
// 「用個數 n 而不是用尾端迭代器 last」版本。
//
// 它等價於以下 for 迴圈:
//
//     for (Size i = 0; i < n; ++i, ++first) {
//         f(*first);
//     }
//
// 適用情境:
//   * 你「知道個數」但不一定「拿得到 end()」 (例如串流、單向鏈表的子段)。
//   * 你「明確只想處理前 N 個元素」 (例如分頁顯示前 10 筆)。
//   * 與 distance/std::next 比起來,直接用 n 更直白。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、for_each vs for_each_n 該選哪個?                      │
// └────────────────────────────────────────────────────────────┘
//
//   * 範圍是「[first, last)」這種「兩端迭代器」自然描述 → for_each
//   * 範圍是「從某個位置開始,共 n 個」自然描述         → for_each_n
//
// 兩者都會對每個元素呼叫一次 f,語意上是孿生兄弟。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // C++17 起 (C++20 起為 constexpr)
//   template <class InputIt, class Size, class UnaryFunc>
//   InputIt for_each_n(InputIt first, Size n, UnaryFunc f);
//
//   // C++17 起 (平行版本)
//   template <class ExecutionPolicy, class FwdIt, class Size, class UnaryFunc>
//   FwdIt for_each_n(ExecutionPolicy&& policy,
//                    FwdIt first, Size n, UnaryFunc f);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、回傳值 (Return value)                                  │
// └────────────────────────────────────────────────────────────┘
//
// 回傳「最後一個被處理元素的下一個位置」的迭代器,
// 即 std::next(first, n) — 可方便串接後續操作。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 n 次套用 f
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. n 必須 ≤ 範圍實際剩餘長度!超出範圍是未定義行為 (UB)。
//      想安全地「最多 n 個」用 std::min(n, distance) 自己截斷。
//   2. n 為 0 → 完全不做事,直接回 first。
//   3. 與 for_each 不同,for_each_n 不傳回 functor;若需累積狀態
//      要靠 lambda 捕獲外部變數。
//   4. C++17 才有 — 需要 -std=c++17 (本專案預設即是)。
//
// ============================================================

/*
補充筆記：std::for_each_n
  - for_each_n 從起點開始處理 n 個元素，不需要 end iterator。
  - 呼叫者必須保證起點後至少有 n 個可用元素。
  - 它常見於 iterator/sentinel 不方便同時提供時的固定長度處理。
  - std::for_each_n 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::for_each_n
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. for_each_n 是哪個標準加入的?回傳值和 for_each 一樣嗎?
//     答:C++17 加入。回傳值「不一樣」—
//         for_each 回傳 functor(by value),for_each_n 回傳的是 iterator,
//         即最後一個被處理元素的下一個位置(等於 std::next(first, n)),方便串接。
//         所以「用有狀態 functor 蒐集結果」的慣用法在這裡行不通,
//         要靠 lambda 捕獲外部變數。
//     追問:什麼時候該選 for_each_n?
//           (當範圍自然是「從某位置起共 n 個」而不是兩端 iterator 時)
//
// ⚠️ 陷阱. n 大於區間實際剩餘長度會怎樣?
//     答:未定義行為(UB)。for_each_n 只拿到起點和個數,沒有 last 可以擋,
//         它不會、也無法幫你檢查邊界,就這樣一路走出界。
//         要安全地「最多 n 個」必須自己先取 std::min(n, distance(first, last))。
//     為什麼會錯:很多人把它想成「像 substr 那樣會自動截斷到結尾」,
//         但 STL 演算法的前置條件一律由呼叫端負責,不做防禦性檢查。
//
// Q3. C++17 的執行策略多載有什麼不同?
//     答:平行版要求 forward iterator(不能只是 input iterator),
//         回傳型別同樣是 iterator,但不保證套用順序,
//         因此 f 必須無資料相依、不可有相互競爭的副作用。
//     追問:n 為 0 時呢?(完全不套用 f,直接回 first)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{10, 20, 30, 40, 50};

    // --- 範例 1: 處理前 3 個元素 ---
    std::cout << "first 3: ";
    std::for_each_n(v.begin(), 3, [](int x){ std::cout << x << ' '; });
    std::cout << '\n';

    // --- 範例 2: 修改前 3 個元素 (各加 100) ---
    std::for_each_n(v.begin(), 3, [](int& x){ x += 100; });
    std::cout << "after add 100 to first 3: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 利用回傳值串接 (處理前 2 個,接著處理剩下) ---
    auto it = std::for_each_n(v.begin(), 2, [](int x){ std::cout << "[A]" << x << ' '; });
    std::for_each(it, v.end(), [](int x){ std::cout << "[B]" << x << ' '; });
    std::cout << '\n';

    // --- 範例 4: n = 0,什麼都不做 ---
    int call_count = 0;
    std::for_each_n(v.begin(), 0, [&](int){ ++call_count; });
    std::cout << "n=0 call_count = " << call_count << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1480_running_sum();
    void practical_top_n_orders();
    void leetcode_2overlap_first_k_sum();
    void practical_preview_first_lines();
    leetcode_1480_running_sum();
    practical_top_n_orders();
    leetcode_2overlap_first_k_sum();
    practical_preview_first_lines();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1480: 一維陣列的動態和 (Running Sum of 1d Array)
// ----------------------------------------------------------------
// 題目:給陣列 nums,回傳「runningSum[i] = nums[0] + nums[1] + ... + nums[i]」。
//
// 為什麼用 std::for_each_n:
//   要處理「從 index 1 開始的 n-1 個元素」 — 用 n 來描述比用 end iterator 直白。
//   每個元素只需要把「自身 + 前一個」即可,最終就會變成累加和。
//
// 解法步驟:
//   1. 從 index 1 開始,要處理 size-1 個元素。
//   2. 每個元素 += 前一個 (用 lambda 捕獲一個指標追蹤「前一個」的位置)。
//
// 複雜度:時間 O(n),空間 O(1) (原地修改)。
void leetcode_1480_running_sum() {
    std::vector<int> nums{1, 2, 3, 4};
    if (!nums.empty()) {
        std::for_each_n(nums.begin() + 1, nums.size() - 1,
                        [prev = &nums[0]](int& x) mutable {
                            x += *prev;
                            prev = &x;
                        });
    }
    std::cout << "LC1480: ";
    for (int x : nums) std::cout << x << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:取前 N 筆訂單做摘要顯示 (分頁第一頁)
// ----------------------------------------------------------------
// 場景:資料庫拉回大量訂單,前端只想顯示「前 N 筆」。
//      for_each_n 比 for_each + 計數器或 begin..begin+N 都更直觀。
struct Order {
    int id;
    std::string customer;
    double amount;
};
void practical_top_n_orders() {
    std::vector<Order> orders{
        {1001, "Alice",   120.5},
        {1002, "Bob",      45.0},
        {1003, "Charlie", 999.9},
        {1004, "Dora",     33.3},
        {1005, "Eve",     200.0}
    };
    const std::size_t topN = 3;
    std::cout << "top" << topN << " orders:\n";
    std::for_each_n(orders.begin(), topN, [](const Order& o) {
        std::cout << "  #" << o.id << ' ' << o.customer
                  << " $" << o.amount << '\n';
    });
}

// ----------------------------------------------------------------
// LeetCode 概念題:前 K 筆得分總和 (Sum of First K Scores)
// ----------------------------------------------------------------
// 題目:給排好序的分數,計算「前 k 高分」總和 (k 可能比 size 小)。
//
// 為什麼用 std::for_each_n:
//   題意「處理前 N 筆」直接對應 for_each_n,比 std::accumulate 更明確
//   (accumulate 想表達「總和」時更乾淨,for_each_n 適合在「副作用累加」情境)。
//
// 複雜度:時間 O(k);空間 O(1)。
void leetcode_2overlap_first_k_sum() {
    std::vector<int> scores{99, 95, 90, 80, 75, 60};
    int k = 3, sum = 0;
    std::for_each_n(scores.begin(), k, [&sum](int x){ sum += x; });
    std::cout << "first " << k << " sum: " << sum << '\n';
}

// ----------------------------------------------------------------
// 實務範例:預覽檔案前 N 行 (head -N 行為)
// ----------------------------------------------------------------
// 場景:CLI 工具 `head -10` 行為 — 列出前 N 行供使用者預覽。
//      for_each_n 直接表達「處理前 N 個元素」。
void practical_preview_first_lines() {
    std::vector<std::string> lines{
        "line1", "line2", "line3", "line4", "line5", "line6"
    };
    std::cout << "preview:";
    std::for_each_n(lines.begin(), 3,
                    [](const std::string& s){ std::cout << " [" << s << "]"; });
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// first 3: 10 20 30
// after add 100 to first 3: 110 120 130 40 50
// [A]110 [A]120 [B]130 [B]40 [B]50
// n=0 call_count = 0
// LC1480: 1 3 6 10
// top3 orders:
//   #1001 Alice $120.5
//   #1002 Bob $45
//   #1003 Charlie $999.9
// first 3 sum: 284
// preview: [line1] [line2] [line3]
