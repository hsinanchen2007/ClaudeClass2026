// =============================================================================
//  第三課：STL 的六大組件概覽 4  —  演算法組件（sort / find / count / accumulate / minmax）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名（節錄）：
//     void      sort(RandomIt first, RandomIt last);                       // <algorithm>
//     InputIt   find(InputIt first, InputIt last, const T& value);         // <algorithm>
//     ptrdiff_t count(InputIt first, InputIt last, const T& value);        // <algorithm>
//     T         accumulate(InputIt first, InputIt last, T init);           // <numeric>
//     pair<It,It> minmax_element(ForwardIt first, ForwardIt last);         // <algorithm>
//   標準版本：以上皆 C++98/03 即有；minmax_element 是 C++11 新增；
//             C++17 增加平行版多載 sort(std::execution::par, ...)（需連結 TBB）。
//   複雜度：sort O(N log N)（標準保證，C++11 起是最壞情況保證）；
//           find / count / accumulate / minmax_element 皆 O(N)。
//           minmax_element 保證最多約 1.5N 次比較，優於分別呼叫 min+max 的 2N。
//   標頭檔：<algorithm>（sort/find/count/minmax_element）、<numeric>（accumulate）
//
// 【詳細解釋 Explanation】
//
// 【1. 演算法為什麼「不知道」容器是誰】
//   STL 演算法一律只接收「一對迭代器」而非容器本身：
//       std::sort(v.begin(), v.end());     // 不是 std::sort(v)
//   這件事看似囉嗦，卻是整個 STL 能維持 M+N 而非 M×N 規模的關鍵：
//   有 M 個容器、N 個演算法時，只要各自對接迭代器介面，就不需要寫 M×N 份實作。
//   附帶好處是可以自然地對「子區間」操作：
//       std::sort(v.begin() + 10, v.begin() + 20);   // 只排中間十個
//   如果介面是 sort(v)，這件事就得另外開一組 API。
//   （C++20 的 ranges::sort(v) 補上了便利語法，但底層仍是同一套迭代器模型。）
//
// 【2. std::sort 到底是什麼排序法】
//   標準只規定「平均 O(N log N)」，C++11 起更強化為「最壞情況 O(N log N)」。
//   libstdc++ 的實作是 introsort（内省排序）：
//     - 主體用 quicksort（快，cache 友善）
//     - 遞迴深度超過 2*log2(N) 時切換 heapsort（避免 quicksort 最壞 O(N²)）
//     - 剩下不到 16 個元素時改用 insertion sort（小陣列上常數最小）
//   注意 std::sort **不保證穩定**（相等元素的相對順序可能改變）。
//   要穩定請用 std::stable_sort（O(N log N)，但可能配置額外記憶體；
//   記憶體不足時退化為 O(N log²N) 的原地版本）。
//
// 【3. accumulate 的第三個參數決定回傳型別 —— 最常見的隱形 bug】
//   accumulate 的回傳型別就是 init 的型別，不是元素型別：
//       std::vector<double> v = {1.5, 2.5};
//       auto s = std::accumulate(v.begin(), v.end(), 0);      // init 是 int！
//       // 每次累加都被截斷成 int → 結果是 3 而不是 4.0
//   正確寫法是 0.0（double）或 0LL（long long，避免大量 int 相加溢位）。
//   同理，對 vector<int> 求總和時若可能超過 2^31，一定要寫 0LL。
//
// 【4. find 的回傳值與「沒找到」的表示】
//   find 回傳「指向該元素的迭代器」，沒找到則回傳 last（也就是 end()）。
//   所以判斷必須拿 end() 比對，且要注意：比對的必須是同一個容器的 end()。
//   若要拿到索引，對 Random Access 迭代器可用 it - v.begin()（O(1)）；
//   泛型寫法是 std::distance(v.begin(), it)。
//
// 【概念補充 Concept Deep Dive】
//   為什麼 minmax_element 只要約 1.5N 次比較，而不是 2N？
//   關鍵是「成對處理」：每次取兩個元素 a、b，先讓它們互比一次（1 次比較），
//   較小的那個才去挑戰目前最小值、較大的那個才去挑戰目前最大值（各 1 次）。
//   於是每處理 2 個元素只用 3 次比較，平均每元素 1.5 次，
//   相較於「每個元素都同時挑戰 min 和 max」的 2 次，省下 25%。
//   對 int 這種比較極廉價的型別看不出差別，但當元素是長字串、或比較器是
//   使用者自訂的昂貴函式時，這 25% 是實打實的。
//
// 【注意事項 Pay Attention】
//   1. std::sort 需要 Random Access Iterator：list / forward_list 用不了，
//      請改用容器自己的成員函式 lst.sort()。
//   2. std::sort 不穩定；需要穩定請用 std::stable_sort。
//   3. accumulate 的 init 型別決定累加型別與溢位行為 —— 用 0.0 / 0LL 而非 0。
//   4. 傳給 sort 的比較器必須是「嚴格弱序」（strict weak ordering）。
//      寫成 <=（把相等視為 true）會讓 sort 在越界位置比較 → 未定義行為，
//      可能崩潰也可能安靜地產生錯誤結果，症狀不固定。
//   5. minmax_element 回傳的是 pair<iterator, iterator>；空範圍時兩者都等於 last，
//      解參考前必須先確認容器非空。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 演算法組件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 演算法為什麼接收「一對迭代器」而不是容器本身？
//     答：為了把 M 個容器與 N 個演算法解耦成 M+N 份程式碼而非 M×N。
//         迭代器是兩者之間的共同介面，演算法完全不需要知道容器型別。
//         附帶好處是可以直接對子區間操作，例如 sort(v.begin()+10, v.begin()+20)。
//     追問：那 C++20 的 std::ranges::sort(v) 是不是推翻了這個設計？
//           → 沒有。ranges 只是在上面加了一層便利介面（並支援 projection 與
//             sentinel），底層仍然是迭代器；且它同時解決了「不小心把兩個不同
//             容器的迭代器配對」這個舊介面的安全漏洞。
//
// 🔥 Q2. std::sort 用的是什麼演算法？複雜度保證是什麼？
//     答：libstdc++ 用 introsort：quicksort 為主，遞迴過深（超過 2*log2 N）
//         時切 heapsort 避免最壞 O(N²)，小區間（<16）改 insertion sort。
//         標準自 C++11 起要求最壞情況也是 O(N log N)。std::sort 不保證穩定。
//     追問：那 std::stable_sort 的複雜度是多少？
//           → 有足夠額外記憶體時 O(N log N)；配置不到時退化為 O(N log²N)
//             的原地歸併。這是標準明文允許的兩段式保證。
//
// ⚠️ 陷阱. std::accumulate(v.begin(), v.end(), 0) 對 vector<double> 求和，為什麼結果是錯的？
//     答：accumulate 的累加器型別由第三個參數 init 決定，這裡 0 是 int，
//         所以內部累加器是 int，每一步 double 相加後都被截斷成整數。
//         {1.5, 2.5} 求和會得到 3 而不是 4。正確寫法是傳 0.0。
//     為什麼會錯：直覺認為「函式會看容器裝什麼就用什麼型別」，
//         但 accumulate 是 template<class It, class T>，T 只從 init 推導，
//         與元素型別無關。同樣的坑也發生在 vector<int> 大量求和時傳 0 導致
//         int 溢位 —— 該寫 0LL。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 217. Contains Duplicate
//   題目：判斷陣列中是否存在重複元素。
//   為什麼用到本主題：最直接的解法就是「sort 後檢查相鄰是否相等」，
//                     一次 std::sort + 一次 std::adjacent_find 就結束，
//                     正好示範「演算法互相組合」這個 STL 的核心用法。
//   複雜度：時間 O(N log N)（排序主導）、空間 O(1)（就地排序副本）。
//   註：用 unordered_set 可做到平均 O(N)，但排序解不需額外容器且常數更小。
// -----------------------------------------------------------------------------
bool containsDuplicate(std::vector<int> nums) {   // 傳值：不動到呼叫端資料
    std::sort(nums.begin(), nums.end());
    return std::adjacent_find(nums.begin(), nums.end()) != nums.end();
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩個陣列的交集（結果中每個元素只出現一次，順序不限）。
//   為什麼用到本主題：sort + set_intersection + unique 是三個 STL 演算法的接力，
//                     示範「先把資料整理成演算法要求的前置條件（已排序），
//                     再套用需要該前置條件的演算法」這個非常常見的流程。
//   複雜度：時間 O(N log N + M log M)、空間 O(min(N, M))。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    // set_intersection 要求兩邊都已排序
    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(out));
    // set_intersection 對重複元素會保留 min(count_a, count_b) 份，故再去重
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】考試成績統計報表
//   情境：教務系統拿到一個班級的分數，要產出：平均分、最高/最低分、
//         及格人數、以及排名後的分數表。
//   為什麼用到本主題：這是 accumulate / minmax_element / count_if / sort
//                     四個演算法各司其職的教科書級組合，
//                     且能示範 accumulate 用 0.0 與整數除法的實際差異。
// -----------------------------------------------------------------------------
struct ScoreReport {
    double average = 0.0;
    int    lowest  = 0;
    int    highest = 0;
    int    passed  = 0;
};

ScoreReport analyzeScores(const std::vector<int>& scores, int pass_line) {
    ScoreReport r;
    if (scores.empty()) return r;

    // 注意：init 寫 0.0 讓累加器是 double；若寫 0 則整段用 int 累加
    double total = std::accumulate(scores.begin(), scores.end(), 0.0);
    r.average = total / static_cast<double>(scores.size());

    auto mm = std::minmax_element(scores.begin(), scores.end());
    r.lowest  = *mm.first;
    r.highest = *mm.second;

    r.passed = static_cast<int>(
        std::count_if(scores.begin(), scores.end(),
                      [pass_line](int s) { return s >= pass_line; }));
    return r;
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7, 4, 6};

    std::cout << "=== 基本演算法示範 ===" << std::endl;

    // 排序
    std::sort(vec.begin(), vec.end());
    std::cout << "排序後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 查找
    auto it = std::find(vec.begin(), vec.end(), 7);
    if (it != vec.end()) {
        std::cout << "找到 7，位置: " << (it - vec.begin()) << std::endl;
    }

    // 計數
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2, 5};
    int count = static_cast<int>(std::count(data.begin(), data.end(), 2));
    std::cout << "2 出現次數: " << count << std::endl;

    // 累加
    int sum = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "總和: " << sum << std::endl;

    // 最大最小
    auto minmax = std::minmax_element(vec.begin(), vec.end());
    std::cout << "最小值: " << *minmax.first << std::endl;
    std::cout << "最大值: " << *minmax.second << std::endl;

    // accumulate 的 init 型別陷阱（實際演示）
    std::cout << "\n=== accumulate 的 init 型別陷阱 ===" << std::endl;
    std::vector<double> prices = {1.5, 2.5, 3.25};
    std::cout << "prices = 1.5 2.5 3.25，正確總和應為 7.25" << std::endl;
    std::cout << "  accumulate(..., 0)   = "
              << std::accumulate(prices.begin(), prices.end(), 0)
              << "   ← init 是 int，每步都被截斷" << std::endl;
    std::cout << "  accumulate(..., 0.0) = "
              << std::accumulate(prices.begin(), prices.end(), 0.0)
              << " ← 正確" << std::endl;

    std::cout << "\n=== LeetCode 217. Contains Duplicate ===" << std::endl;
    std::cout << "[1,2,3,1] → " << (containsDuplicate({1, 2, 3, 1}) ? "true" : "false") << std::endl;
    std::cout << "[1,2,3,4] → " << (containsDuplicate({1, 2, 3, 4}) ? "true" : "false") << std::endl;

    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    std::cout << "[1,2,2,1] ∩ [2,2] = ";
    for (int n : intersection({1, 2, 2, 1}, {2, 2})) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "[4,9,5] ∩ [9,4,9,8,4] = ";
    for (int n : intersection({4, 9, 5}, {9, 4, 9, 8, 4})) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：班級成績統計 ===" << std::endl;
    std::vector<int> scores = {88, 45, 92, 73, 60, 55, 100, 38};
    ScoreReport r = analyzeScores(scores, 60);
    std::cout << "人數     : " << scores.size() << std::endl;
    std::cout << "平均分   : " << r.average << std::endl;
    std::cout << "最低/最高: " << r.lowest << " / " << r.highest << std::endl;
    std::cout << "及格人數 : " << r.passed << " （及格線 60）" << std::endl;

    std::sort(scores.begin(), scores.end(), std::greater<int>());
    std::cout << "分數排名 : ";
    for (int s : scores) std::cout << s << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第三課：STL 的六大組件概覽4.cpp -o demo4

// === 預期輸出 ===
// === 基本演算法示範 ===
// 排序後: 1 2 3 4 5 6 7 8 9
// 找到 7，位置: 6
// 2 出現次數: 4
// 總和: 45
// 最小值: 1
// 最大值: 9
//
// === accumulate 的 init 型別陷阱 ===
// prices = 1.5 2.5 3.25，正確總和應為 7.25
//   accumulate(..., 0)   = 6   ← init 是 int，每步都被截斷
//   accumulate(..., 0.0) = 7.25 ← 正確
//
// === LeetCode 217. Contains Duplicate ===
// [1,2,3,1] → true
// [1,2,3,4] → false
//
// === LeetCode 349. Intersection of Two Arrays ===
// [1,2,2,1] ∩ [2,2] = 2
// [4,9,5] ∩ [9,4,9,8,4] = 4 9
//
// === 日常實務：班級成績統計 ===
// 人數     : 8
// 平均分   : 68.875
// 最低/最高: 38 / 100
// 及格人數 : 5 （及格線 60）
// 分數排名 : 100 92 88 73 60 55 45 38
