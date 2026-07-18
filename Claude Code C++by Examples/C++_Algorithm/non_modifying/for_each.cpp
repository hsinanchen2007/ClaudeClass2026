// ============================================================
// std::for_each
// 分類 (Category): Non-modifying sequence operations (非修改型,但 f 可改元素)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/for_each
//   * https://cplusplus.com/reference/algorithm/for_each/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::for_each 是最基礎的「對範圍中每個元素套用一個動作」的 STL。
// 它的語意完全等價於以下 for 迴圈:
//
//     for (auto it = first; it != last; ++it) {
//         f(*it);
//     }
//
// 但用 STL 寫的好處在於:
//
//   1. 「意圖」很明確 — 看到 std::for_each 立刻知道是「對每個元素做事」,
//      不會跟「找元素」「過濾元素」「累加元素」混在一起讀。
//   2. 配合執行策略 (C++17) 可以一行切換為平行執行。
//   3. 配合 functor 可以「一次處理 + 同時收集統計」(見回傳值說明)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、與 range-based for / std::ranges::for_each 的差異      │
// └────────────────────────────────────────────────────────────┘
//
//   * range-based for (`for (auto& x : v)`)
//     - 寫起來最短,但只能用「整個容器」的範圍。
//   * std::for_each(first, last, f)
//     - 可以指定子範圍 (例如 v.begin() + 1, v.end() - 1)。
//     - 可加執行策略平行化。
//     - 可回傳「最終的 functor」做累計。
//   * std::ranges::for_each(rng, f)  (C++20)
//     - 直接傳容器,可帶投影 (projection)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、為什麼 for_each「歸類在非修改型」?                    │
// └────────────────────────────────────────────────────────────┘
//
// 這是命名上的歷史包袱。標準把它分在 non-modifying,但只是因為
// 「演算法本身」不會改任何東西 — 它把元素傳給 f,如果 f 把元素
// 透過參考改了,那是 f 的事。
//
// 結論:技術上 f 可以修改元素 (例如把每個元素平方),
//      但若想清楚表達「修改」意圖,用 std::transform 更恰當。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   // C++98 起 (C++20 起為 constexpr)
//   template <class InputIt, class UnaryFunc>
//   UnaryFunc for_each(InputIt first, InputIt last, UnaryFunc f);
//
//   // C++17 起 (平行版本)
//   template <class ExecutionPolicy, class FwdIt, class UnaryFunc>
//   void for_each(ExecutionPolicy&& policy,
//                 FwdIt first, FwdIt last, UnaryFunc f);
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、回傳值 (Return value) — 一個不太為人知的特性           │
// └────────────────────────────────────────────────────────────┘
//
// 非平行版回傳 std::move(f) — 也就是「最終的 functor」!
// 這代表:如果 f 是有狀態的 functor (例如累加器),你可以
// 把回傳值存起來,從中取出累積的狀態。
//
//   struct Sum { int total = 0; void operator()(int x){ total += x; } };
//   Sum s = std::for_each(v.begin(), v.end(), Sum{});
//   //  ^^^^^ 這裡的 s.total 才是真正的累計,不是傳進去那個 Sum{}
//
// 為什麼要這樣設計?因為 functor 是「按值傳入」的,你傳進去的
// 那個 Sum{} 在函式內被複製/移動了,內部狀態的改動不會反映回呼叫端。
// 所以 STL 把「最終狀態」當成回傳值給你。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 恰好 (last - first) 次 — O(n)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. f 是「按值傳入」 — 有狀態的 functor 必須用回傳值取最終狀態。
//   2. f 不可使範圍/迭代器失效 (不要在 f 裡 push_back 同一個 vector)。
//   3. 平行版回傳 void,且 f 必須是「無資料相依」的純函式。
//   4. 想做「過濾」「映射」請用 copy_if / transform,for_each 是純動作。
//
// ============================================================

/*
補充筆記：std::for_each
  - for_each 把 callable 套用到每個元素，常用在需要副作用的場合。
  - 若只是轉換資料，transform 通常比 for_each 更能表達輸入與輸出。
  - callable 可回傳值但會被忽略；重點在副作用或內部累積狀態。
  - std::for_each 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// 一個有狀態的 functor,示範「回傳值」的用途
struct Sum {
    int total = 0;
    void operator()(int x) { total += x; }
};

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // --- 範例 1: 列印每個元素 (lambda 不修改元素) ---
    std::cout << "elements: ";
    std::for_each(v.begin(), v.end(),
                  [](int x){ std::cout << x << ' '; });
    std::cout << '\n';

    // --- 範例 2: 透過參考修改元素 (將每個元素平方) ---
    // 注意:雖然能這樣寫,但若意圖是「映射」,改用 std::transform 更恰當。
    std::for_each(v.begin(), v.end(), [](int& x){ x = x * x; });
    std::cout << "squared: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 取用回傳的 functor 收集累積狀態 ---
    Sum s = std::for_each(v.begin(), v.end(), Sum{});
    std::cout << "sum of squares = " << s.total << '\n';

    // --- 範例 4: 對空範圍呼叫,f 不會被呼叫 ---
    std::vector<int> empty;
    int call_count = 0;
    std::for_each(empty.begin(), empty.end(),
                  [&](int){ ++call_count; });
    std::cout << "empty range call_count = " << call_count << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_1672_richest_customer_wealth();
    void practical_log_processing();
    void leetcode_1450_busy_students();
    void practical_apply_discount();
    leetcode_1672_richest_customer_wealth();
    practical_log_processing();
    leetcode_1450_busy_students();
    practical_apply_discount();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 1672: 最富有客戶的資產總量 (Richest Customer Wealth)
// ----------------------------------------------------------------
// 題目:給 m × n 的二維陣列 accounts,accounts[i][j] 為第 i 客戶在
//      第 j 銀行的存款。回傳「最富有客戶」的總資產。
//
// 為什麼用 std::for_each:
//   外層走訪每個客戶 (每一列),內層對該列做動作 (累加)。
//   for_each 把「對每一個元素做事」這個意圖直接呈現。
//
// 解法步驟:
//   1. 對每一列 row: 用內層 for_each 把所有銀行存款加到 wealth。
//   2. 維護全域 richest = max(richest, wealth)。
//
// 複雜度:時間 O(m × n),空間 O(1)。
void leetcode_1672_richest_customer_wealth() {
    std::vector<std::vector<int>> accounts{
        {1, 2, 3},
        {3, 2, 1},
        {2, 5, 1}
    };
    int richest = 0;
    std::for_each(accounts.begin(), accounts.end(),
                  [&](const std::vector<int>& row) {
                      int wealth = 0;
                      std::for_each(row.begin(), row.end(),
                                    [&](int v) { wealth += v; });
                      if (wealth > richest) richest = wealth;
                  });
    std::cout << "LC1672: richest = " << richest << '\n';
}

// ----------------------------------------------------------------
// 實務範例:log 處理 — 一次走訪,同時累積多項統計
// ----------------------------------------------------------------
// 場景:處理一批伺服器 log,要同時統計總行數、ERROR 數、WARN 數。
//      這正是「有狀態 functor」+「回傳值取最終狀態」的最佳示範。
//
// 為什麼用 std::for_each + 有狀態 functor:
//   * 一次走訪同時累積多個指標,效率最高。
//   * 回傳的 functor 直接攜帶結果,不必額外宣告全域變數或修改外部捕獲。
struct LogStat {
    int errors = 0;
    int warns  = 0;
    int idx    = 0;
    void operator()(const std::string& line) {
        ++idx;
        if (line.find("ERROR") != std::string::npos) ++errors;
        else if (line.find("WARN") != std::string::npos) ++warns;
    }
};
void practical_log_processing() {
    std::vector<std::string> logs{
        "INFO: server started",
        "WARN: slow query 1.2s",
        "ERROR: db connection refused",
        "INFO: retry success",
        "ERROR: timeout"
    };
    LogStat stat = std::for_each(logs.begin(), logs.end(), LogStat{});
    std::cout << "log: lines=" << stat.idx
              << " errors=" << stat.errors
              << " warns="  << stat.warns << '\n';
}

// ----------------------------------------------------------------
// LeetCode 1450: 在既定時間做作業的學生數量
// ----------------------------------------------------------------
// 題目:給 startTime[]、endTime[],查詢時間 queryTime,
//      回傳在 queryTime 時正在做作業的學生數量 (即 start <= queryTime <= end)。
//
// 為什麼用 std::for_each:
//   經典「對每對 (start, end) 做檢查並累加計數」 — 用 for_each + lambda 即可,
//   也可用 count_if 更直接,這裡示範 for_each 做帶外部狀態的累積。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_1450_busy_students() {
    std::vector<int> startTime{1, 2, 3};
    std::vector<int> endTime{3, 2, 7};
    int queryTime = 4;
    int count = 0;
    for (size_t i = 0; i < startTime.size(); ++i) {
        if (startTime[i] <= queryTime && queryTime <= endTime[i]) ++count;
    }
    std::cout << "LC1450: " << count << '\n';
}

// ----------------------------------------------------------------
// 實務範例:對每筆訂單套用折扣 (printer-style 副作用)
// ----------------------------------------------------------------
// 場景:結帳時對 vector 中每個 Order 套用折扣 (修改價格欄位)。
//      for_each 直接表達「對每個元素做副作用」,語意比 transform 更明確。
void practical_apply_discount() {
    struct Order { std::string name; double price; };
    std::vector<Order> orders{{"A", 100}, {"B", 200}, {"C", 50}};
    double discount = 0.9;
    std::for_each(orders.begin(), orders.end(),
                  [discount](Order& o){ o.price *= discount; });
    std::cout << "after discount:";
    for (auto& o : orders) std::cout << " " << o.name << "=" << o.price;
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// elements: 1 2 3 4 5
// squared: 1 4 9 16 25
// sum of squares = 55
// empty range call_count = 0
// LC1672: richest = 8
// log: lines=5 errors=2 warns=1
// LC1450: 1
// after discount: A=90 B=180 C=45
