// =============================================================================
//  第三課：STL 的六大組件概覽 7  —  <functional> 內建函數物件與透明比較器
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<functional>
//   算術類：std::plus<T> / minus<T> / multiplies<T> / divides<T> / modulus<T> / negate<T>
//   比較類：std::equal_to<T> / not_equal_to<T> / greater<T> / less<T>
//           / greater_equal<T> / less_equal<T>
//   邏輯類：std::logical_and<T> / logical_or<T> / logical_not<T>
//   位元類：std::bit_and<T> / bit_or<T> / bit_xor<T>（C++11）/ bit_not<T>（C++14）
//   標準版本：基本型自 C++98；**透明版（std::less<> 即 std::less<void>）是 C++14**；
//             bit_not 是 C++14。C++17 移除了 unary_function / binary_function。
//   複雜度：這些 functor 本身都是 O(1)、可完全 inline，不引入任何額外成本。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼標準要為 a > b 這種小事準備一個類別】
//   因為演算法接收的是「物件」不是「運算子」。你不能寫 std::sort(v.begin(), v.end(), >)。
//   要把「用 > 比較」這個行為傳進去，就得包成一個有 operator() 的型別：
//       template <class T> struct greater {
//           constexpr bool operator()(const T& a, const T& b) const { return a > b; }
//       };
//   於是 std::sort(v.begin(), v.end(), std::greater<int>()) 就成立了。
//   這是 C++98 沒有 lambda 時代唯一乾淨的作法，至今仍是最能表達意圖的寫法。
//
// 【2. std::greater<int>() 那對括號是什麼】
//   std::greater<int> 是**型別**，std::greater<int>() 才是**物件**（呼叫預設建構子）。
//   sort 的第三個參數要的是物件，所以括號不能省。
//   C++17 起有 CTAD，但對這種無參數建構的空類別沒有幫助 —— 括號還是要寫。
//
// 【3. 透明比較器 std::greater<>（C++14）解決了什麼】
//   舊寫法必須指定型別：std::less<std::string>。這帶來兩個問題：
//     (a) 型別寫錯或改了容器就要跟著改。
//     (b) 對關聯容器查詢會產生**多餘的暫時物件**：
//         std::set<std::string> s;
//         s.find("hello");        // 舊：先把 const char* 轉成暫時 std::string（可能配置堆積）
//   C++14 的 std::less<void>（可簡寫 std::less<>）改用樣板化的 operator()：
//       template <class T, class U>
//       constexpr auto operator()(T&& t, U&& u) const
//           -> decltype(std::forward<T>(t) < std::forward<U>(u));
//   宣告成 std::set<std::string, std::less<>> 之後，find("hello") 可以直接拿
//   const char* 與 std::string 比較（heterogeneous lookup），省下那次配置。
//   「transparent（透明）」這個詞的由來就是：它對參數型別是透明的、不強制轉換。
//
// 【4. 這些 functor 在哪裡真的常用】
//     - std::sort(..., std::greater<int>())        降序排序
//     - std::priority_queue<int, vector<int>, std::greater<int>>
//                                                  把預設的大頂堆變成小頂堆
//     - std::map<K, V, std::greater<K>>            鍵由大到小排列
//     - std::accumulate(b, e, 1, std::multiplies<int>())   求連乘積
//     - std::transform(a.begin(), a.end(), b.begin(), out.begin(), std::plus<int>())
//                                                  兩個序列逐元素相加
//   現代 C++ 中簡單情況常用 lambda 取代，但 priority_queue 的樣板參數位置
//   要的是**型別**而非物件，那裡 std::greater<int> 依然是最自然的選擇。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 std::sort 的比較器必須是「嚴格弱序（strict weak ordering）」？
//   嚴格弱序要求：
//     (1) 反自反性：comp(a, a) 必須為 false
//     (2) 反對稱性：comp(a,b) 為真則 comp(b,a) 必為假
//     (3) 遞移性：comp(a,b) 且 comp(b,c) → comp(a,c)
//     (4) 等價的遞移性：由 !comp(a,b) && !comp(b,a) 定義的「等價」也要遞移
//   std::greater 用 >，滿足 (1)（a > a 為 false）。
//   但如果誤寫成 greater_equal（>=），(1) 就破了：comp(a, a) 變成 true。
//   後果不是「排序結果順序怪怪的」而是**未定義行為**：
//   introsort 的內層迴圈靠「一定會遇到不滿足比較的元素」來停止掃描，
//   自反的比較器會讓它衝出陣列邊界 → 讀寫越界，可能崩潰、也可能安靜地損毀資料。
//   這也是為什麼自訂比較器一律用 < 或 >，絕不要用 <= 或 >=。
//
// 【注意事項 Pay Attention】
//   1. std::greater<int> 是型別，std::greater<int>() 才是物件；sort 要物件。
//   2. **絕不要**把比較器寫成 >= 或 <=（非嚴格弱序）→ 未定義行為，可能越界。
//   3. std::greater<>（透明版）是 C++14；C++11 只能寫 std::greater<T>。
//   4. priority_queue 預設是**大頂堆**（用 std::less）；要小頂堆得明確傳 std::greater<T>
//      —— 這個「less 卻是大頂堆」的反直覺設計是常見混淆點。
//   5. std::divides / modulus 除以零仍然是未定義行為，functor 不會幫你檢查。
//   6. logical_and<bool> 沒有 && 的短路（short-circuit）語意：兩個參數都會被求值。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】內建函數物件與比較器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::priority_queue 預設是大頂堆還是小頂堆？怎麼改成另一種？
//     答：預設用 std::less<T>，結果是**大頂堆**（top() 是最大值）。
//         要小頂堆得寫完整三個樣板參數：
//           std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
//         注意第三個參數要的是**型別**（std::greater<int>），不是物件。
//     追問：為什麼 less 反而給出大頂堆，這不是很反直覺嗎？
//           → 因為底層是 std::make_heap/push_heap 建的 max-heap，
//             而那組演算法的慣例是「comp(a,b) 為真代表 a 的優先度較低」。
//             用 less 時「較小者優先度較低」→ 最大值浮到 top。
//
// 🔥 Q2. std::less<> 和 std::less<int> 差在哪？為什麼 C++14 要加前者？
//     答：std::less<int> 的 operator() 固定收兩個 const int&；
//         std::less<>（= std::less<void>，C++14）的 operator() 是樣板，
//         可以接受任意兩種可比較的型別。主要價值是關聯容器的
//         heterogeneous lookup：宣告 std::set<std::string, std::less<>> 之後，
//         s.find("hello") 不必先建構一個暫時的 std::string。
//     追問：那 std::map 為什麼不預設就用 std::less<>？
//           → 為了 ABI 與回溯相容。改預設會改變既有型別的 mangled name，
//             因此標準選擇讓使用者顯式選用。
//
// ⚠️ 陷阱. 自訂比較器寫成 `return a >= b;` 會發生什麼事？
//     答：不是「排序結果稍微不同」，而是**未定義行為**。
//         >= 不滿足嚴格弱序的反自反性（comp(a,a) 應為 false，但 a>=a 是 true）。
//         libstdc++ 的 introsort 分割迴圈靠「總會碰到停止條件」來收斂，
//         自反比較器會讓指標衝出陣列 → 越界讀寫。可能立刻 segfault、
//         也可能安靜地寫壞相鄰記憶體，症狀完全不固定。
//     為什麼會錯：把比較器想成「回答誰比較大」，覺得含不含等號只是小差別。
//         實際上 sort 要的是「嚴格小於」語意，等號必須回傳 false。
//         排錯線索：加 -D_GLIBCXX_DEBUG 重編，libstdc++ 會直接報
//         "comparison doesn't meet irreflexive requirements"。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <queue>
#include <numeric>
#include <algorithm>
#include <functional>  // 內建函數物件

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：內建 functor 在 LeetCode 上幾乎總是以「priority_queue 的第三個樣板參數」
//         或「sort 的第三個參數」形式出現，是解法的一個零件而非題目本身。
//         同一課的第 6 個範例已用 LeetCode 179. Largest Number 完整示範
//         「自訂比較器」這件事；在這裡再塞一題只會重複。
//         下面改以「Top-K 熱門關鍵字」實務範例展示 std::greater 在
//         priority_queue 上不可取代的用法（該處樣板參數只能填型別，lambda 不便用）。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例 1】搜尋服務的 Top-K 熱門關鍵字（min-heap 維持前 K 名）
//   情境：搜尋引擎每秒收到大量查詢，要即時維護「目前點擊數最高的 K 個關鍵字」。
//         全部排序是 O(N log N) 且要存下全部資料；用大小為 K 的小頂堆
//         可以做到 O(N log K) 且只佔 O(K) 記憶體。
//   為什麼用到本主題：小頂堆的關鍵就是把 priority_queue 的第三個樣板參數
//                     從預設的 std::less 換成 std::greater —— 這裡填的是**型別**，
//                     是內建 functor 至今無可取代的位置。
// -----------------------------------------------------------------------------
using Hit = std::pair<int, std::string>;   // (點擊數, 關鍵字)

std::vector<Hit> topKKeywords(const std::vector<Hit>& stream, std::size_t k) {
    // 小頂堆：堆頂是目前前 K 名中最小的那個，方便被更大的取代
    std::priority_queue<Hit, std::vector<Hit>, std::greater<Hit>> min_heap;

    for (const Hit& h : stream) {
        min_heap.push(h);
        if (min_heap.size() > k) min_heap.pop();   // 踢掉最小的，維持 K 個
    }

    std::vector<Hit> result;
    while (!min_heap.empty()) {
        result.push_back(min_heap.top());
        min_heap.pop();
    }
    std::reverse(result.begin(), result.end());     // 由高到低
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】設定檔鍵值查詢：透明比較器省下暫時字串
//   情境：服務啟動時把設定讀進 std::set<std::string>，之後用字面值反覆查詢。
//   為什麼用到本主題：預設的 std::less<std::string> 會讓 s.count("timeout")
//                     先建構一個暫時 std::string；改用 std::less<> 之後
//                     const char* 可直接與 std::string 比較，省下配置。
// -----------------------------------------------------------------------------
bool hasKeyTransparent(const std::set<std::string, std::less<>>& keys,
                       const char* name) {
    return keys.find(name) != keys.end();   // 不建構暫時 std::string
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};

    // 預設排序（升序）
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 使用 greater<int> 降序排序
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 其他內建函數物件
    std::cout << "\n內建函數物件示範：" << std::endl;
    std::cout << "plus<int>()(3, 4) = " << std::plus<int>()(3, 4) << std::endl;
    std::cout << "minus<int>()(10, 3) = " << std::minus<int>()(10, 3) << std::endl;
    std::cout << "multiplies<int>()(5, 6) = " << std::multiplies<int>()(5, 6) << std::endl;
    std::cout << "logical_and<bool>()(true, false) = "
              << std::logical_and<bool>()(true, false) << std::endl;

    // 搭配演算法：內建 functor 最實用的場合
    std::cout << "\n=== 搭配演算法 ===" << std::endl;
    std::vector<int> a = {1, 2, 3, 4, 5};
    std::cout << "連乘積 accumulate(...,1,multiplies) = "
              << std::accumulate(a.begin(), a.end(), 1, std::multiplies<int>()) << std::endl;

    std::vector<int> b = {10, 20, 30, 40, 50};
    std::vector<int> sum(a.size());
    std::transform(a.begin(), a.end(), b.begin(), sum.begin(), std::plus<int>());
    std::cout << "逐元素相加 transform(...,plus) = ";
    for (int n : sum) std::cout << n << " ";
    std::cout << std::endl;

    // 大頂堆 vs 小頂堆
    std::cout << "\n=== priority_queue：less 是大頂堆，greater 才是小頂堆 ===" << std::endl;
    std::priority_queue<int> max_heap;                                    // 預設 less
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_heap;
    for (int n : {30, 10, 50, 20}) { max_heap.push(n); min_heap.push(n); }
    std::cout << "  預設(less) 的 top()    = " << max_heap.top() << "  ← 最大值" << std::endl;
    std::cout << "  greater<int> 的 top()  = " << min_heap.top() << "  ← 最小值" << std::endl;

    // 透明比較器（C++14）
    std::cout << "\n=== 透明比較器 std::less<>（C++14）===" << std::endl;
    std::set<std::string, std::less<>> config = {"timeout", "retries", "endpoint"};
    std::cout << "  find(\"timeout\") 存在? " << (hasKeyTransparent(config, "timeout") ? "是" : "否")
              << "  （不建構暫時 std::string）" << std::endl;
    std::cout << "  find(\"missing\") 存在? " << (hasKeyTransparent(config, "missing") ? "是" : "否")
              << std::endl;

    // map 用 greater：鍵由大到小
    std::cout << "\n=== 用 greater 反轉關聯容器的排序 ===" << std::endl;
    std::set<int, std::greater<int>> desc_set = {3, 1, 4, 1, 5, 9, 2, 6};
    std::cout << "  set<int, greater<int>>: ";
    for (int n : desc_set) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務 1：Top-3 熱門關鍵字（小頂堆）===" << std::endl;
    std::vector<Hit> stream = {
        {120, "cpp iterator"}, {45,  "stl allocator"}, {300, "vector vs list"},
        {87,  "lambda capture"}, {210, "move semantics"}, {15,  "raii"},
    };
    for (const Hit& h : topKKeywords(stream, 3)) {
        std::cout << "  " << h.first << " 次  " << h.second << std::endl;
    }

    std::cout << "\n=== 日常實務 2：設定檔鍵查詢 ===" << std::endl;
    for (const char* key : {"endpoint", "retries", "unknown_key"}) {
        std::cout << "  " << key << " → "
                  << (hasKeyTransparent(config, key) ? "已設定" : "未設定") << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽7.cpp -o demo7

// === 預期輸出 ===
// 升序: 1 2 5 8 9
// 降序: 9 8 5 2 1
//
// 內建函數物件示範：
// plus<int>()(3, 4) = 7
// minus<int>()(10, 3) = 7
// multiplies<int>()(5, 6) = 30
// logical_and<bool>()(true, false) = 0
//
// === 搭配演算法 ===
// 連乘積 accumulate(...,1,multiplies) = 120
// 逐元素相加 transform(...,plus) = 11 22 33 44 55
//
// === priority_queue：less 是大頂堆，greater 才是小頂堆 ===
//   預設(less) 的 top()    = 50  ← 最大值
//   greater<int> 的 top()  = 10  ← 最小值
//
// === 透明比較器 std::less<>（C++14）===
//   find("timeout") 存在? 是  （不建構暫時 std::string）
//   find("missing") 存在? 否
//
// === 用 greater 反轉關聯容器的排序 ===
//   set<int, greater<int>>: 9 6 5 4 3 2 1
//
// === 日常實務 1：Top-3 熱門關鍵字（小頂堆）===
//   300 次  vector vs list
//   210 次  move semantics
//   120 次  cpp iterator
//
// === 日常實務 2：設定檔鍵查詢 ===
//   endpoint → 已設定
//   retries → 已設定
//   unknown_key → 未設定
