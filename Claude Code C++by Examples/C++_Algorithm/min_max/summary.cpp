/*
================================================================================
【C++_Algorithm/min_max/summary.cpp】
本章末總整理 — min/max 家族
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/algorithm#Minimum.2Fmaximum_operations
  - https://cplusplus.com/reference/algorithm/

--------------------------------------------------------------------------------
一、本章涵蓋的 API — 「值」對「位置」兩條主線
--------------------------------------------------------------------------------
  值 (拿值):                          位置 (拿 iterator):
  ────────────────────────             ──────────────────────────
  std::min(a, b)                       std::min_element(first, last)
  std::max(a, b)                       std::max_element(first, last)
  std::minmax(a, b)                    std::minmax_element(first, last)
  std::clamp(v, lo, hi)                —

  比較器版本都有對應的 [, comp] overload。
  std::min / max / minmax 也提供 initializer_list 版本:
      std::min({3, 7, 2})

--------------------------------------------------------------------------------
二、回傳值型別細節 (容易踩坑)
--------------------------------------------------------------------------------
  std::min(a, b)       → const T&       (注意:是 reference)
  std::max(a, b)       → const T&
  std::minmax(a, b)    → std::pair<const T&, const T&>   ★ 注意是「reference pair」
  std::min({...})      → T              (initializer_list 版本,值型別)
  std::clamp(v, lo, hi)→ const T&

  ⚠ 危險範例 (dangling reference):
      auto p = std::minmax(1, 2);   // p.first / p.second 是「對暫時量的 reference」
                                    // 暫時量在語句結束後就消滅 → UB
      std::cout << p.first;         // 可能讀到垃圾

  ❌ 常見誤解:以為 structured binding 會延長壽命——不會。
      auto [lo, hi] = std::minmax(1, 2);  // 仍然 dangling（ASan: stack-use-after-scope 實測）
                                          // 兩參數版回傳 pair<const T&, const T&>,
                                          // 拆開來的 lo/hi 仍綁在已消滅的暫存值上。

  ✅ 正確做法:用值接住,或改用 initializer_list 版:
      int lo = std::min(1, 2);            // 直接拷貝
      auto p = std::minmax({1, 2});       // initializer_list 版回傳 pair<T, T>,是值不是參考
      int a = 1, b = 2;                   // 或先存成 lvalue 再用兩參數版
      auto q = std::minmax(a, b);

--------------------------------------------------------------------------------
三、複雜度 (Complexity)
--------------------------------------------------------------------------------
  API                  比較次數              迭代器需求
  ──────────────────  ──────────────────    ──────────────────
  min(a,b) / max(a,b)  1                     —
  minmax(a,b)          1                     —
  min({...}) 等         N - 1                 InputIterator
  min_element          N - 1                 ForwardIterator
  max_element          N - 1                 ForwardIterator
  minmax_element       至多 3*(N-1)/2        ForwardIterator   ★ 比分兩次呼叫省
  clamp                最多 2                 —

  ★ minmax_element 的設計亮點:成對掃描,把比較次數從 2N 降到 1.5N。

--------------------------------------------------------------------------------
四、空範圍 (Empty Range) 行為
--------------------------------------------------------------------------------
  std::min_element(empty range) → 回 last
  std::max_element(empty range) → 回 last
  std::minmax_element(empty range) → { last, last }

  ★ 呼叫端必須先檢查 it != last 才能解參考。
  ★ std::min / max 的 initializer_list 版本不可傳空串列 → UB。

--------------------------------------------------------------------------------
五、clamp 重點 (C++17)
--------------------------------------------------------------------------------
  std::clamp(v, lo, hi)
    - 若 v < lo  → 回 lo
    - 若 hi < v  → 回 hi
    - 否則回 v 本身

  限制:
    1. 前置條件:!(hi < lo),否則 UB (lo 必須 <= hi)。
    2. 回傳是 const T&;若 v 是暫時量,小心 dangling。
    3. 浮點 NaN:NaN < lo 與 hi < NaN 皆為 false → 結果回 NaN。
       如果想把 NaN 也夾到範圍內,要自己先處理。

--------------------------------------------------------------------------------
六、比較器 (Comparator)
--------------------------------------------------------------------------------
  - 預設 std::less<T>。
  - 自訂時要符合 strict weak ordering。
  - 寫成 [](const T& a, const T& b){ return a < b; },不能寫 <=。

  典型「拿最大」:
      auto it = std::max_element(v.begin(), v.end(),
                  [](const Person& a, const Person& b){
                      return a.score < b.score;     // 比 score
                  });

--------------------------------------------------------------------------------
七、min/max 與 std::min_element 在「相等元素」上的選擇
--------------------------------------------------------------------------------
  - std::max(a, b):若 a == b,回 a (左)。
  - std::max_element:回「最左邊」的最大元素 iterator。
    (因為內部用嚴格 < 比較,遇到等值不更新最佳解)
  - std::min_element:同理回最左邊的最小元素。

  若需要「最右邊」的極值,要反向掃 (用 std::rbegin/rend) 或自訂比較。

--------------------------------------------------------------------------------
八、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. minmax + auto = dangling reference:
       auto p = std::minmax(a, b);  // a, b 是 prvalue → p 內含對亡者的參考
     對策:用 std::minmax({a, b}) (initializer_list 版,傳值);或先存。

  2. min/max 與型別不同:
       std::max(1u, -1)   // 編譯錯,unsigned 與 int 推不出共同 T
     對策:std::max<int>(...) 顯式指定型別。

  3. 對空容器呼叫 max_element 後解參考:
       *std::max_element(v.begin(), v.end())  // v 空 → UB

  4. clamp 上下界倒置:
       std::clamp(5, 10, 0)   // hi < lo → UB

  5. NaN 在 min/max:
       std::min(1.0, std::nan(""))  // 結果為 nan? 還是 1.0? 依比較方向而異
     對策:浮點極值要先過濾 NaN。

  6. minmax_element 與 minmax 的「位置 vs 值」混淆:
       minmax_element 回 pair<iterator, iterator>
       minmax        回 pair<const T&, const T&>

  7. 對 stream / input range 求 min_element:
       InputIterator 不允許多次走訪;標準要求 ForwardIterator,所以不行。

--------------------------------------------------------------------------------
九、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 取得樣本中最大值/最小值 (基本統計、boundary 分析)。
  - 限制使用者輸入範圍 (UI HP / 音量) → clamp。
  - 同時拿到「最高分」與「最低分」 → minmax_element 一次掃。
  - 找最近的時間點:對排序好的 vector<time_t> 用 min_element 配自訂距離函式。
  - 比較兩個欄位的安全大小 → std::min(buf_size, request_size)。
================================================================================
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】min/max 家族總覽
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請把 min/max 家族依「回傳值 vs 回傳位置」分類，並說明各自的回傳型別。
//     答：兩條主線。**值**：`min(a,b)`/`max(a,b)` → `const T&`；
//         `minmax(a,b)` → `pair<const T&, const T&>`；`clamp(v,lo,hi)` → `const T&`；
//         這些的 initializer_list 版本則回傳「值」(`T` 或 `pair<T,T>`)。
//         **位置**：`min_element`/`max_element` → 迭代器；
//         `minmax_element` → `pair<迭代器, 迭代器>`。
//         判準：要拿去算索引、取成員、或 erase 掉它，就得用 *_element。
//     追問：clamp 為什麼沒有「位置」版？(它是對單一值做邊界裁剪，本來就不涉及區間)
//
// 🔥 Q2. 這一家的比較次數保證各是多少？
//     答：`min(a,b)`/`max(a,b)`/`minmax(a,b)` 各 1 次；`clamp` 至多 2 次；
//         `min_element`/`max_element` 各 **N-1** 次；
//         `minmax_element` 至多 **⌊3(N-1)/2⌋** 次（pairwise：兩兩配對，每 2 個元素
//         花 3 次比較），比分開呼叫的 2(N-1) 次省約四分之一。
//         這些都是標準明文的保證，不是實作細節。
//     追問：迭代器需求呢？(*_element 系列要求 ForwardIterator——因為要記住並回傳位置；
//           InputIterator 只能單次走訪，不能用)
//
// ⚠️ 陷阱. `auto [lo, hi] = std::minmax(1, 2);` 結構化繫結能救 dangling 嗎？
//     答：**不能**。minmax 兩值版回傳 `pair<const T&, const T&>`，pair 成員本身是
//         reference；結構化繫結只是給那兩個 reference 取名字，1 和 2 這兩個暫存物
//         仍在全表達式結束時銷毀，lo/hi 隨即 dangling。
//         正解：`auto p = std::minmax({1, 2});`（ilist 版回傳 pair<T,T>，是值），
//         或先把兩個運算元存成具名變數再傳進去。
//     為什麼會錯：一般認知是「用 auto 接、或用結構化繫結，就會複製一份」——複製確實
//         發生了，但複製的是「裝著 reference 的 pair」，裡面那層 reference 原封不動。
//         對比之下 `int lo = std::min(1, 2);` 是安全的，因為那是從 const T& 真的
//         拷貝出一個 int。差別就在 pair 多包了一層。
//
// 🔥 Q4. 空範圍時這一家各自怎麼表現？
//     答：`min_element`/`max_element` 回傳 `last`、`minmax_element` 回傳 `{last, last}`
//         ——都合法，但**不可解參考**，呼叫端必須先檢查 `it != last`。
//         相對地，`std::min({})`/`std::max({})` 傳空的 initializer_list 是 **UB**
//         ——它得回傳一個 T，而空集合根本沒有最小值可回。
//     追問：為什麼設計上不讓 *_element 也是 UB？(回傳 last 是迭代器天生就有的
//           「查無此物」哨兵，不必額外發明；值版本則沒有這種哨兵可用)
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <iostream>
#include <vector>

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_minmax_values() {
    std::cout << "\n[demo_minmax_values]\n";
    std::cout << "  min(3,5)=" << std::min(3, 5) << "\n";
    std::cout << "  max(3,5)=" << std::max(3, 5) << "\n";

    // 用值接住,避免 dangling reference
    std::pair<int, int> mm = std::minmax(3, 5);
    std::cout << "  minmax(3,5)=(" << mm.first << "," << mm.second << ")\n";

    // initializer_list 版本(傳值,安全)
    std::cout << "  min{3,7,2}=" << std::min({3, 7, 2}) << "\n";
    std::cout << "  max{3,7,2}=" << std::max({3, 7, 2}) << "\n";
}

static void demo_minmax_elements() {
    std::cout << "\n[demo_minmax_elements]\n";
    std::vector<int> v{5, 1, 9, 2, 7};

    auto it_min = std::min_element(v.begin(), v.end());
    auto it_max = std::max_element(v.begin(), v.end());
    auto it_mm  = std::minmax_element(v.begin(), v.end());  // 一次拿兩個

    std::cout << "  min=" << *it_min << ", max=" << *it_max << "\n";
    std::cout << "  minmax_element=(" << *it_mm.first << ","
              << *it_mm.second << ")\n";

    // 空 range 安全示範
    std::vector<int> empty;
    auto e_it = std::min_element(empty.begin(), empty.end());
    std::cout << "  empty min_element == end? " << (e_it == empty.end()) << "\n";
}

static void demo_clamp() {
    std::cout << "\n[demo_clamp]\n";
#if __cplusplus >= 201703L
    std::cout << "  clamp(5,  0, 10)=" << std::clamp(5,  0, 10) << "\n";  // 5
    std::cout << "  clamp(-1, 0, 10)=" << std::clamp(-1, 0, 10) << "\n";  // 0
    std::cout << "  clamp(99, 0, 10)=" << std::clamp(99, 0, 10) << "\n";  // 10
#else
    std::cout << "  (std::clamp 需要 C++17)\n";
#endif
}

int main() {
    demo_minmax_values();
    demo_minmax_elements();
    demo_clamp();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary

// === 預期輸出 ===
//
// [demo_minmax_values]
//   min(3,5)=3
//   max(3,5)=5
//   minmax(3,5)=(3,5)
//   min{3,7,2}=2
//   max{3,7,2}=7
//
// [demo_minmax_elements]
//   min=1, max=9
//   minmax_element=(1,9)
//   empty min_element == end? 1
//
// [demo_clamp]
//   clamp(5,  0, 10)=5
//   clamp(-1, 0, 10)=0
//   clamp(99, 0, 10)=10
//
// [done]
