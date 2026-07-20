// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計13.cpp
//    —  二分搜尋家族：binary_search / lower_bound / upper_bound / equal_range
// =============================================================================
//
// 【主題資訊 Information】
//   bool  binary_search(FwdIt f, FwdIt l, const T& v);              // C++98
//   FwdIt lower_bound  (FwdIt f, FwdIt l, const T& v);              // C++98
//   FwdIt upper_bound  (FwdIt f, FwdIt l, const T& v);              // C++98
//   pair<FwdIt,FwdIt> equal_range(FwdIt f, FwdIt l, const T& v);    // C++98
//   （四者皆有額外的 Compare comp 多載）
//
//   標準版本：C++98 起（C++20 加 constexpr）
//   **前提條件：範圍必須已依同一個比較準則「分區」（partitioned）**
//   迭代器需求：Forward Iterator 即可，但**只有 Random Access Iterator
//               才有 O(log N) 的意義**（見下方詳解）
//   複雜度：比較次數 O(log N)；但 iterator 前進次數對非隨機存取容器是 O(N)
//   標頭檔：<algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 四個函式的精確定義（一次記清楚）】
// 對已排序範圍與目標值 v：
//     lower_bound → 第一個 **>= v** 的位置（v 該插入的最前位置）
//     upper_bound → 第一個 **>  v** 的位置（v 該插入的最後位置）
//     equal_range → pair{lower_bound, upper_bound}，即所有等於 v 的區間
//     binary_search → bool，等價於「lower_bound 找到的位置確實等於 v」
// 三個關鍵推論：
//   * [lower_bound, upper_bound) 就是所有等於 v 的元素，
//     **它的長度就是 v 出現的次數**（這比 std::count 的 O(N) 快得多）
//   * v 不存在時，lower_bound == upper_bound，兩者都指向「v 該插入的位置」
//   * 因此 lower_bound 同時解決了「找元素」與「找插入點」兩個問題
//
// 【2. 為什麼有了 binary_search 還幾乎總是該用 lower_bound】
// binary_search 只回傳 bool——它**丟掉了位置資訊**。
// 但實務上你幾乎總是需要位置：找到了要取值、要修改、要算索引；
// 沒找到要知道該插在哪裡。而 lower_bound 的成本與 binary_search 完全相同。
//     // 想知道有沒有，順便想拿到位置：
//     auto it = std::lower_bound(v.begin(), v.end(), x);
//     bool found = (it != v.end() && *it == x);      // ← binary_search 的等價寫法
// 所以：**binary_search 只在「純粹只要 yes/no」時才用**，其餘一律 lower_bound。
//
// 【3. 前提條件比「已排序」更寬鬆，也更危險】
// 標準的正式要求不是「已排序」，而是**已依判準分區（partitioned）**：
// 存在某個分界點，使得所有滿足 elem < v 的元素都在不滿足的元素之前。
// 完全排序的範圍必然滿足這個條件，所以「先排序」是最簡單的保證方式。
// **未滿足前提時的行為是未定義的**——不會報錯、不會丟例外，
// 只會安靜地回傳一個沒有意義的位置。這是本家族最危險的地方：
// 錯誤不會當場顯現，而是變成難以追查的邏輯錯誤。
// 尤其注意：用了自訂比較器時，**排序與搜尋必須用同一個比較器**。
// 用 greater<int> 排成降序後，再用預設的 lower_bound 搜尋就是未定義行為。
//
// 【4. 「O(log N) 次比較」不等於「O(log N) 時間」——這是最精妙的一點】
// 這四個函式只要求 **Forward Iterator**，所以 std::list 也能傳進去，
// 而且**比較次數確實是 O(log N)**。但是——
// 二分搜尋每一步都要「跳到中點」，對 Forward Iterator 而言，
// 跳 k 步需要真的走 k 次 ++（std::advance 對非隨機存取是 O(k)）。
// 累加起來，iterator 前進的總成本是 **O(N)**。
// 結論：
//   * 對 vector / array / deque（Random Access）→ 真正的 O(log N)
//   * 對 list（Bidirectional）→ 比較 O(log N) 次，但總時間 O(N)，
//     **比直接用 std::find 線性掃還慢**（因為多了計算中點的開銷）
//   * 對 set / map → **不要用**，它們有自己的 O(log N) 成員函式
//     s.lower_bound(v)（走紅黑樹路徑，不需要計算中點）
// 這是本課「iterator category 決定演算法效能」最細緻的一個例子：
// 它不像 std::sort 那樣直接編譯失敗，而是**默默地變慢**。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 equal_range 不是單純呼叫兩次
//   標準規定 equal_range 的比較次數上界與呼叫 lower_bound + upper_bound
//   相同（都是 O(log N)），但實作可以更聰明：先二分找到任一個等於 v 的位置，
//   再從該點向左右各做一次較小範圍的二分。libstdc++ 就是這樣做的，
//   實際比較次數少於分別呼叫兩次。
//
// (B) 計算中點為什麼寫成 first + (last - first) / 2
//   而不是 (first + last) / 2——後者對指標根本沒有定義（指標不能相加），
//   對整數索引則有溢位風險（low + high 可能超過 int 上限）。
//   這個寫法是二分搜尋的標準防溢位慣例，在手寫二分時尤其重要。
//
// (C) 與 std::partition_point 的關係
//   C++11 的 std::partition_point(f, l, pred) 回傳「第一個不滿足 pred 的位置」，
//   而 lower_bound(f, l, v) 恰好等價於 partition_point(f, l, [&](x){ return x < v; })。
//   這正說明了為什麼前提條件是「分區」而非「排序」——
//   二分搜尋家族本質上都是 partition_point 的特例。
//
// 【注意事項 Pay Attention】
// 1. **範圍必須已依同一準則排序／分區**，否則是未定義行為
//    （不會報錯，只會安靜地給出錯誤答案）。
// 2. 用自訂比較器排序時，**搜尋必須傳入同一個比較器**。
// 3. **回傳的迭代器可能是 end()**，解參考前一定要檢查：
//    if (it != v.end() && *it == x)
// 4. 對 **std::list 雖然能編譯但總時間是 O(N)**，比 std::find 還慢，不要用。
// 5. 對 **set / map 一律用成員函式** s.lower_bound(v)，不要用 std::lower_bound。
// 6. 想同時知道「有沒有」和「在哪裡」時用 lower_bound；
//    binary_search 丟掉位置資訊，只在純粹要 bool 時使用。
// 7. 計算某值出現次數，用 equal_range 的區間長度（O(log N)），
//    不要用 std::count（O(N)）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】二分搜尋家族
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. lower_bound 和 upper_bound 的差別是什麼？兩者相減代表什麼？
//     答：lower_bound 回傳第一個 **>= v** 的位置，upper_bound 回傳第一個
//         **> v** 的位置。兩者之間的區間 [lower, upper) 就是所有等於 v 的元素，
//         **區間長度即 v 出現的次數**——這是 O(log N) 的計數方式，
//         比 std::count 的 O(N) 快得多。若 v 不存在，兩者相等，
//         且都指向「v 應該插入的位置」。
//     追問：equal_range 呢？→ 它一次回傳這兩個迭代器組成的 pair，
//         而且實作上比分別呼叫兩次更省比較次數。
//
// 🔥 Q2. std::lower_bound 可以用在 std::list 上嗎？效能如何？
//     答：可以編譯（它只要求 Forward Iterator），**比較次數也確實是 O(log N)**，
//         但**總時間是 O(N)**。因為二分搜尋每步要跳到中點，
//         而 list 的 iterator 不支援隨機存取，跳 k 步就要走 k 次 ++。
//         實際上比直接用 std::find 線性掃還慢（多了計算中點的開銷）。
//     追問：那 set / map 呢？→ 更不該用。它們有成員函式 s.lower_bound(v)，
//         走紅黑樹路徑真正達到 O(log N)。**同名時一律優先用成員函式。**
//
// 🔥 Q3. 為什麼實務上幾乎總是用 lower_bound 而不是 binary_search？
//     答：因為兩者成本相同，但 binary_search **丟掉了位置資訊**，只回傳 bool。
//         而實務需求幾乎總是需要位置：找到要取值或修改，沒找到要知道插入點。
//         用 lower_bound 可以一併得到：
//             auto it = std::lower_bound(v.begin(), v.end(), x);
//             bool found = (it != v.end() && *it == x);
//         這行就等價於 binary_search，還多拿到了 it。
//     追問：那 binary_search 什麼時候有用？→ 純粹只要 yes/no、
//         完全不需要位置時，它的意圖表達得更清楚。
//
// ⚠️ 陷阱. 用 std::sort(v.begin(), v.end(), std::greater<int>()) 排成降序後，
//        再呼叫 std::lower_bound(v.begin(), v.end(), 5) 會怎樣？
//     答：**未定義行為**。lower_bound 預設用 operator< 判斷，
//         但範圍是依 greater 排的（降序），不滿足「依 < 分區」的前提。
//         它不會報錯、不會丟例外，只會安靜地回傳一個毫無意義的位置。
//         正確寫法是把同一個比較器也傳給搜尋：
//         std::lower_bound(v.begin(), v.end(), 5, std::greater<int>())
//     為什麼會錯：多數人記得「二分搜尋要先排序」，卻沒意識到
//         「**排序與搜尋必須用同一個比較準則**」。而且因為不會當場出錯，
//         這個 bug 常常一路潛伏到生產環境。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <functional>   // std::greater

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 704. Binary Search
//   題目：在已排序陣列中找 target，回傳其索引；不存在回傳 -1。
//   為什麼用到本主題：這題就是二分搜尋的定義。用 lower_bound 找到位置後，
//         再確認該位置確實等於 target（因為 lower_bound 找不到時會回傳插入點）。
//   複雜度：O(log N)。
// -----------------------------------------------------------------------------
int binarySearchLC(const std::vector<int>& nums, int target) {
    auto it = std::lower_bound(nums.begin(), nums.end(), target);
    // ★ 必須同時檢查 != end() 與 *it == target
    if (it != nums.end() && *it == target) {
        return static_cast<int>(it - nums.begin());
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 35. Search Insert Position
//   題目：在已排序陣列中找 target 的索引；若不存在，回傳它該被插入的位置。
//   為什麼用到本主題：這題**正是 lower_bound 的定義本身**——
//         「第一個 >= target 的位置」既是元素所在處，也是它該插入的位置。
//         一行解，而且完全不需要判斷有沒有找到。
//   複雜度：O(log N)。
// -----------------------------------------------------------------------------
int searchInsert(const std::vector<int>& nums, int target) {
    return static_cast<int>(
        std::lower_bound(nums.begin(), nums.end(), target) - nums.begin());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 34. Find First and Last Position of Element in Sorted Array
//                        Element in Sorted Array
//   題目：在已排序陣列中找 target 的起訖索引；不存在回傳 [-1, -1]。
//   為什麼用到本主題：這題**就是 equal_range**——它同時回傳
//         lower_bound 與 upper_bound，正好對應起點與終點。
//         起點 = lower_bound，終點 = upper_bound - 1。
//   複雜度：O(log N)。
// -----------------------------------------------------------------------------
std::vector<int> searchRange(const std::vector<int>& nums, int target) {
    auto range = std::equal_range(nums.begin(), nums.end(), target);
    if (range.first == range.second) return {-1, -1};   // 空區間 = 不存在
    return {static_cast<int>(range.first - nums.begin()),
            static_cast<int>(range.second - nums.begin() - 1)};
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】時間序列查詢：找出指定時間區間內的監控樣本
//   情境：監控系統把樣本依時間戳（秒）遞增存在 vector 中。
//         儀表板要查「09:00 到 09:05 之間的所有樣本」。
//   為什麼用到本主題：時間序列天生已排序，正是二分搜尋的理想場景。
//         用 lower_bound 找區間起點、upper_bound 找區間終點，
//         兩次 O(log N) 就取出整個區間，遠優於線性掃描。
//         注意起點用 lower_bound（含起始時間）、終點用 upper_bound（含結束時間），
//         這個搭配才能得到閉區間 [from, to] 的語意。
// -----------------------------------------------------------------------------
struct Sample {
    int epochSec;
    int latencyMs;
};

void queryTimeRange(const std::vector<Sample>& series, int from, int to) {
    // 依 epochSec 二分搜尋：比較器只看時間欄位
    auto begin = std::lower_bound(series.begin(), series.end(), from,
                                  [](const Sample& s, int t) { return s.epochSec < t; });
    auto end = std::upper_bound(series.begin(), series.end(), to,
                                [](int t, const Sample& s) { return t < s.epochSec; });

    std::cout << "  查詢 [" << from << ", " << to << "] 共 "
              << (end - begin) << " 筆:" << std::endl;
    for (auto it = begin; it != end; ++it) {
        std::cout << "    t=" << it->epochSec << "  " << it->latencyMs << "ms" << std::endl;
    }
}

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // 二分搜尋要求已排序！

    // binary_search：是否存在, 這裡檢查 vec 中是否存在元素 5 和 11
    std::cout << "=== binary_search ===" << std::endl;
    std::cout << "存在 5: " << (std::binary_search(vec.begin(), vec.end(), 5) ? "是" : "否") << std::endl;
    std::cout << "存在 11: " << (std::binary_search(vec.begin(), vec.end(), 11) ? "是" : "否") << std::endl;

    // lower_bound：第一個 >= 值的位置, 這裡找到 vec 中第一個大於或等於 5 的元素的位置和數值
    std::cout << "\n=== lower_bound ===" << std::endl;
    auto lb = std::lower_bound(vec.begin(), vec.end(), 5);
    std::cout << "lower_bound(5): 位置 " << (lb - vec.begin()) << ", 值 " << *lb << std::endl;

    // upper_bound：第一個 > 值的位置, 這裡找到 vec 中第一個大於 5 的元素的位置和數值
    std::cout << "\n=== upper_bound ===" << std::endl;
    auto ub = std::upper_bound(vec.begin(), vec.end(), 5);
    std::cout << "upper_bound(5): 位置 " << (ub - vec.begin()) << ", 值 " << *ub << std::endl;

    // ★ 回傳值可能是 end()，解參考前一定要檢查
    std::cout << "\n=== 回傳 end() 的情況（必須先檢查再解參考）===" << std::endl;
    auto ub2 = std::upper_bound(vec.begin(), vec.end(), 10);   // 10 是最大值
    std::cout << "upper_bound(10): 位置 " << (ub2 - vec.begin());
    if (ub2 == vec.end()) {
        std::cout << " → 就是 end()，*ub2 是未定義行為，不可解參考" << std::endl;
    }
    auto lb2 = std::lower_bound(vec.begin(), vec.end(), 99);   // 不存在且大於全部
    std::cout << "lower_bound(99): 位置 " << (lb2 - vec.begin())
              << " → 找不到，這是 99 該插入的位置" << std::endl;

    // equal_range：lower_bound 和 upper_bound 的組合, 這裡找到 vec 中等於 2 的元素的範圍（位置和數量）
    std::cout << "\n=== equal_range ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 2, 2, 3, 4, 5};
    auto range = std::equal_range(vec2.begin(), vec2.end(), 2);
    std::cout << "equal_range(2): [" << (range.first - vec2.begin())
              << ", " << (range.second - vec2.begin()) << ")" << std::endl;
    std::cout << "2 的數量: " << (range.second - range.first)
              << "  ← O(log N)，比 std::count 的 O(N) 快" << std::endl;
    std::cout << "對照 std::count(2): " << std::count(vec2.begin(), vec2.end(), 2)
              << "  ← 結果相同但需掃描全部元素" << std::endl;

    // ★ 自訂比較器：排序與搜尋必須用同一個
    std::cout << "\n=== 降序資料必須把比較器一起傳給搜尋 ===" << std::endl;
    std::vector<int> desc = {10, 8, 6, 4, 2};
    auto correct = std::lower_bound(desc.begin(), desc.end(), 6, std::greater<int>());
    std::cout << "降序資料 lower_bound(6, greater): 位置 " << (correct - desc.begin())
              << ", 值 " << *correct << "  ← 正確" << std::endl;
    std::cout << "  (若省略 greater 而用預設 <，前提條件不成立，屬未定義行為，"
                 "不會報錯但結果無意義)" << std::endl;

    // ★ set 要用成員函式，不要用 std::lower_bound
    std::cout << "\n=== set 請用成員函式 lower_bound ===" << std::endl;
    std::set<int> s = {10, 20, 30, 40, 50};
    auto sit = s.lower_bound(25);      // O(log N)：走樹狀路徑
    std::cout << "set::lower_bound(25) = " << *sit
              << "  ← 成員函式走樹狀結構，真正的 O(log N)" << std::endl;
    std::cout << "  (std::lower_bound 對 set 雖可編譯，但 iterator 前進是 O(N)，不該用)"
              << std::endl;

    std::cout << "\n=== LeetCode 704. Binary Search ===" << std::endl;
    std::cout << "[-1,0,3,5,9,12] 找 9  -> " << binarySearchLC({-1, 0, 3, 5, 9, 12}, 9) << std::endl;
    std::cout << "[-1,0,3,5,9,12] 找 2  -> " << binarySearchLC({-1, 0, 3, 5, 9, 12}, 2) << std::endl;

    std::cout << "\n=== LeetCode 35. Search Insert Position ===" << std::endl;
    std::cout << "[1,3,5,6] 找 5 -> " << searchInsert({1, 3, 5, 6}, 5) << std::endl;
    std::cout << "[1,3,5,6] 找 2 -> " << searchInsert({1, 3, 5, 6}, 2) << std::endl;
    std::cout << "[1,3,5,6] 找 7 -> " << searchInsert({1, 3, 5, 6}, 7) << std::endl;

    std::cout << "\n=== LeetCode 34. Find First and Last Position ===" << std::endl;
    auto r1 = searchRange({5, 7, 7, 8, 8, 10}, 8);
    std::cout << "[5,7,7,8,8,10] 找 8 -> [" << r1[0] << "," << r1[1] << "]" << std::endl;
    auto r2 = searchRange({5, 7, 7, 8, 8, 10}, 6);
    std::cout << "[5,7,7,8,8,10] 找 6 -> [" << r2[0] << "," << r2[1] << "]" << std::endl;

    std::cout << "\n=== 日常實務：時間序列區間查詢 ===" << std::endl;
    std::vector<Sample> series = {
        {1000, 45}, {1060, 52}, {1120, 300}, {1180, 48},
        {1240, 61}, {1300, 55}, {1360, 900}
    };
    queryTimeRange(series, 1120, 1300);
    queryTimeRange(series, 1400, 1500);

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計13.cpp -o demo13

// === 預期輸出 ===
// === binary_search ===
// 存在 5: 是
// 存在 11: 否
//
// === lower_bound ===
// lower_bound(5): 位置 4, 值 5
//
// === upper_bound ===
// upper_bound(5): 位置 5, 值 6
//
// === 回傳 end() 的情況（必須先檢查再解參考）===
// upper_bound(10): 位置 10 → 就是 end()，*ub2 是未定義行為，不可解參考
// lower_bound(99): 位置 10 → 找不到，這是 99 該插入的位置
//
// === equal_range ===
// equal_range(2): [1, 4)
// 2 的數量: 3  ← O(log N)，比 std::count 的 O(N) 快
// 對照 std::count(2): 3  ← 結果相同但需掃描全部元素
//
// === 降序資料必須把比較器一起傳給搜尋 ===
// 降序資料 lower_bound(6, greater): 位置 2, 值 6  ← 正確
//   (若省略 greater 而用預設 <，前提條件不成立，屬未定義行為，不會報錯但結果無意義)
//
// === set 請用成員函式 lower_bound ===
// set::lower_bound(25) = 30  ← 成員函式走樹狀結構，真正的 O(log N)
//   (std::lower_bound 對 set 雖可編譯，但 iterator 前進是 O(N)，不該用)
//
// === LeetCode 704. Binary Search ===
// [-1,0,3,5,9,12] 找 9  -> 4
// [-1,0,3,5,9,12] 找 2  -> -1
//
// === LeetCode 35. Search Insert Position ===
// [1,3,5,6] 找 5 -> 2
// [1,3,5,6] 找 2 -> 1
// [1,3,5,6] 找 7 -> 4
//
// === LeetCode 34. Find First and Last Position ===
// [5,7,7,8,8,10] 找 8 -> [3,4]
// [5,7,7,8,8,10] 找 6 -> [-1,-1]
//
// === 日常實務：時間序列區間查詢 ===
//   查詢 [1120, 1300] 共 4 筆:
//     t=1120  300ms
//     t=1180  48ms
//     t=1240  61ms
//     t=1300  55ms
//   查詢 [1400, 1500] 共 0 筆:
