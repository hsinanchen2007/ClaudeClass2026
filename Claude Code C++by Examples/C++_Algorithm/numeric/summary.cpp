/*
================================================================================
【C++_Algorithm/numeric/summary.cpp】
本章末總整理 — <numeric> 數值演算法
================================================================================

本檔僅為章末教科書式總整理。依規則 8,本檔不包含任何 LeetCode 題目。

參考:
  - https://en.cppreference.com/w/cpp/header/numeric
  - https://cplusplus.com/reference/numeric/

--------------------------------------------------------------------------------
一、本章涵蓋的 API
--------------------------------------------------------------------------------
  累加 / 摺疊 (Folding):
    std::accumulate(first, last, init [, op])         → init + 全部元素
    std::reduce    (first, last [, init] [, op])      → 同上,但可平行 (C++17)
    std::inner_product(...)                            → 內積 / 自訂雙運算的累積
    std::transform_reduce(...)                         → 邊轉換邊累積 (C++17)

  前綴和 / 掃描 (Scans):
    std::partial_sum(first, last, d_first [, op])     → 前綴和 (inclusive)
    std::inclusive_scan(first, last, d_first ...)     → 同 partial_sum 但可平行 (C++17)
    std::exclusive_scan(first, last, d_first, init ...)  → 不含當前元素的前綴和
    std::transform_inclusive_scan(...)                 → 邊轉換邊掃描 (C++17)
    std::transform_exclusive_scan(...)                 → 同上,exclusive 版

  差分:
    std::adjacent_difference(first, last, d_first [, op])  → 相鄰差

  填充:
    std::iota(first, last, init)                       → 連續遞增填值

  整數工具 (在 <numeric>):
    std::gcd(a, b)     std::lcm(a, b)                  (C++17)
    std::midpoint(a, b)                                (C++20)

--------------------------------------------------------------------------------
二、accumulate vs reduce — 最容易混淆的兩兄弟
--------------------------------------------------------------------------------
  std::accumulate(first, last, init, op)
    - 嚴格從左到右順序計算:((init op a[0]) op a[1]) op ...
    - 不可平行 (因為左結合的順序很重要)。
    - 可用於非可交換、非結合的運算 (例如字串串接、矩陣乘法)。
    - 不需要 init 與元素型別相同,但 init 型別會「決定累加結果型別」(常見 bug)。

  std::reduce(first, last [, init] [, op])    (C++17)
    - 不保證計算順序;假設運算「可交換 + 可結合」。
    - 可加 std::execution::par 平行執行。
    - 若 init 省略,使用元素型別的 value-initialized 預設值 (int: 0)。
    - 浮點數的浮點誤差順序敏感 → 不同執行可能略有不同。

  典型陷阱:
      std::vector<double> v = ...;
      double sum = std::accumulate(v.begin(), v.end(), 0);   // ★ 0 是 int → 整數累加!
                                                              //   小數會被截掉!
      double sum2 = std::accumulate(v.begin(), v.end(), 0.0); // ✓ 0.0 才是 double

  ★ init 一定要寫成「想要的累加型別」對應字面值。

--------------------------------------------------------------------------------
三、複雜度與迭代器
--------------------------------------------------------------------------------
  API                          時間              迭代器           備註
  ───────────────────────────  ──────────────    ──────────       ──────────
  accumulate                    O(N)              InputIterator     順序確定
  reduce                        O(N)              ForwardIterator   可平行
  inner_product                 O(N)              InputIterator     需兩條 range
  transform_reduce              O(N)              ForwardIterator   可平行
  partial_sum                   O(N)              InputIterator     順序確定
  inclusive_scan                O(N)              ForwardIterator   可平行
  exclusive_scan                O(N)              ForwardIterator   可平行
  adjacent_difference           O(N)              InputIterator     順序確定
  iota                          O(N)              ForwardIterator   寫入型
  gcd / lcm / midpoint          O(log min(a,b))   —                 編譯期可 constexpr

--------------------------------------------------------------------------------
四、前綴和 (Prefix Sum) 與 inclusive vs exclusive
--------------------------------------------------------------------------------
  input:           1, 2, 3, 4
  inclusive_scan:  1, 3, 6, 10     (含當位)
  exclusive_scan:  0, 1, 3, 6      (不含當位,需 init = 0)

  ★ partial_sum 等於 inclusive_scan,但 partial_sum 只能順序執行。
  ★ 用 inclusive_scan / exclusive_scan 可帶 execution policy 做平行掃描。

  典型應用:
    - 區間查詢 [L, R] 之和:O(1) 用 prefix[R+1] - prefix[L]
    - 平行寫入時計算每個執行緒應有的 offset
    - 跑路記錄/累計營收

--------------------------------------------------------------------------------
五、inner_product 與 transform_reduce
--------------------------------------------------------------------------------
  std::inner_product(a1, a2, b1, init [, op1, op2])
    - 預設 op1 = +,op2 = *,即計算 dot product:Σ a[i] * b[i]。
    - 可改寫成自訂兩個運算的「成對組合 + 摺疊」,例如比對相同數。

  std::transform_reduce(...) (C++17)
    - 概念上是 inner_product 的現代版,且可平行。
    - 兩種形式:
       (a) 兩 range:Σ binop(a[i], b[i]) 經 op 摺疊
       (b) 一 range + 一元 transform:Σ transform(a[i])

  ★ 對「先映射再加總」(map-reduce 模式),用 transform_reduce 比寫
    transform + accumulate 更清晰且可平行。

--------------------------------------------------------------------------------
六、adjacent_difference 與 partial_sum 是逆運算
--------------------------------------------------------------------------------
  input:                1, 3, 6, 10
  adjacent_difference:  1, 2, 3, 4     (第一個元素照抄;之後是相鄰差)
  partial_sum 結果:    1, 3, 6, 10    (恢復原序列)

  典型應用:
    - 從累積病例 → 每日新增 (adjacent_difference)。
    - 從每日新增 → 累積病例 (partial_sum)。

--------------------------------------------------------------------------------
七、iota (連續遞增填值)
--------------------------------------------------------------------------------
  std::iota(first, last, init)
    - 把 init, init+1, init+2, ... 寫入 [first, last)。
    - 名稱來自 APL 語言的 ι 運算子。
    - 常用於建立索引陣列 (argsort 模式):
         std::vector<int> idx(n);
         std::iota(idx.begin(), idx.end(), 0);
         std::sort(idx.begin(), idx.end(),
                   [&](int a, int b){ return values[a] < values[b]; });

--------------------------------------------------------------------------------
八、整數工具 (C++17 / C++20)
--------------------------------------------------------------------------------
  std::gcd(a, b)         最大公因數                  C++17
  std::lcm(a, b)         最小公倍數                  C++17,內部 = a/gcd * b
  std::midpoint(a, b)    安全的中點 (整數防溢位)      C++20

  ★ midpoint 解決經典的 binary search bug:
       int mid = (lo + hi) / 2;       // lo + hi 可能 overflow
       int mid = std::midpoint(lo, hi); // 不會 overflow

--------------------------------------------------------------------------------
九、常見陷阱 (Common Pitfalls)
--------------------------------------------------------------------------------
  1. accumulate 的 init 型別錯 → 截斷 / 溢位:
     - 整數 vector 求和但元素總和超過 int → 用 long long init。
     - 浮點 vector 求和但 init 用整數 → 小數被丟掉。

  2. reduce 對浮點數可能與 accumulate 結果略不同:
     - 浮點不嚴格結合,平行化會改變捨入順序 → 數值微差。

  3. partial_sum / scan 的 in-place 用法:
     - d_first 可以等於 first,但兩端不可重疊「錯位」否則 UB。

  4. inner_product 沒檢查 b 的長度:
     - 預設只用 (b1) 一個起點,假設 b 至少和 a 一樣長 → 越界 UB。

  5. iota 寫入時要先 resize:
     - std::iota 不會擴大 container,只是寫到既有空間。

  6. transform_reduce 兩 range 長度不一致 → 結果未定義 (短的那邊決定上限)。

  7. gcd / lcm 對負數:
     - cppreference 規定參數為「無號值」會比較單純;若傳負數,行為依
       實作而定 (通常先取絕對值)。為避免歧義,先呼叫 std::abs。

  8. accumulate / reduce 不抛例外保證:
     - 若 op 拋例外,範圍部分被走過,部分未走;通常無法回復狀態。

--------------------------------------------------------------------------------
十、選擇準則速查表
--------------------------------------------------------------------------------
  需求                                          選擇
  ─────────────────────────────────────────  ─────────────────────────
  整段求和 (順序固定、可用於字串等非結合)         std::accumulate
  整段求和 (可平行、純加法)                       std::reduce
  內積                                          std::inner_product 或 transform_reduce
  前綴和 (順序固定)                              std::partial_sum
  前綴和 (可平行)                                std::inclusive_scan
  不含當前的前綴和                                std::exclusive_scan
  邊轉換邊摺疊                                    std::transform_reduce
  相鄰差 (累積轉變化)                             std::adjacent_difference
  建立 0..N-1 索引                                std::iota
  最大公因數 / 最小公倍數                          std::gcd / std::lcm
  安全中點 (避免溢位)                              std::midpoint

--------------------------------------------------------------------------------
十一、工作場景 (Daily Work Use Cases)
--------------------------------------------------------------------------------
  - 計算訂單總金額 → accumulate。
  - 計算移動平均的前綴和 → partial_sum。
  - 平行加總大資料 → reduce(par)。
  - 加權平均 (學分 * 分數) → inner_product 或 transform_reduce。
  - 每日新增 → 累積總數轉換 → adjacent_difference / partial_sum。
  - 螢幕比例化簡 → gcd 約分。
  - 二分搜尋 mid 計算 → midpoint 防溢位。
================================================================================
*/
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// 示範區 (Demonstration)
// -----------------------------------------------------------------------------
static void demo_iota_accumulate() {
    std::cout << "\n[demo_iota_accumulate]\n";
    std::vector<int> v(5);
    std::iota(v.begin(), v.end(), 1);  // 1..5

    int sum = std::accumulate(v.begin(), v.end(), 0);
    std::cout << "  sum(1..5)=" << sum << "\n";

    // 自訂 op:把元素串成 CSV 字串 (展示 accumulate 的非數值用法)
    std::string s = std::accumulate(v.begin(), v.end(), std::string{},
                                    [](std::string acc, int x) {
                                        if (!acc.empty()) acc += ",";
                                        acc += std::to_string(x);
                                        return acc;
                                    });
    std::cout << "  join=" << s << "\n";
}

static void demo_inner_product_partial_sum() {
    std::cout << "\n[demo_inner_product_partial_sum]\n";
    std::vector<int> a{1, 2, 3};
    std::vector<int> b{4, 5, 6};

    int dot = std::inner_product(a.begin(), a.end(), b.begin(), 0);
    std::cout << "  dot=" << dot << "\n";  // 1*4 + 2*5 + 3*6 = 32

    std::vector<int> ps(a.size());
    std::partial_sum(a.begin(), a.end(), ps.begin());
    std::cout << "  partial_sum:";
    for (int x : ps) std::cout << ' ' << x;
    std::cout << "\n";  // 1 3 6
}

static void demo_reduce_scan_note() {
    std::cout << "\n[demo_reduce_scan_note]\n";
#if __cplusplus >= 201703L
    std::vector<int> v{1, 2, 3, 4};
    std::cout << "  reduce=" << std::reduce(v.begin(), v.end(), 0) << "\n";

    std::vector<int> inc(v.size());
    std::inclusive_scan(v.begin(), v.end(), inc.begin());
    std::cout << "  inclusive_scan:";
    for (int x : inc) std::cout << ' ' << x;
    std::cout << "\n";  // 1 3 6 10

    // gcd / lcm
    std::cout << "  gcd(12, 18) = " << std::gcd(12, 18) << "\n";  // 6
    std::cout << "  lcm(4, 6)   = " << std::lcm(4, 6)   << "\n";  // 12
#else
    std::cout << "  (reduce / scan / gcd / lcm 需要 C++17)\n";
#endif
}

int main() {
    demo_iota_accumulate();
    demo_inner_product_partial_sum();
    demo_reduce_scan_note();
    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
