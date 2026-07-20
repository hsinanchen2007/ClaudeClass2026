// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計11.cpp
//    —  std::sort / std::stable_sort 與嚴格弱序（strict weak ordering）
// =============================================================================
//
// 【主題資訊 Information】
//   void sort       (RandomIt f, RandomIt l);                 // C++98
//   void sort       (RandomIt f, RandomIt l, Compare comp);   // C++98
//   void stable_sort(RandomIt f, RandomIt l);                 // C++98
//   void stable_sort(RandomIt f, RandomIt l, Compare comp);   // C++98
//
//   標準版本：C++98 起（C++17 加執行策略、C++20 加 constexpr）
//   迭代器需求：**Random Access Iterator**（所以 list / forward_list 不能用）
//   複雜度：sort 為 O(N log N)（**C++11 起是最壞情況保證**，C++03 只保證平均）；
//           stable_sort 為 O(N log N)（有額外記憶體時）或 O(N log²N)（無額外記憶體時）
//   穩定性：sort **不保證穩定**；stable_sort 保證相等元素維持原相對順序
//   標頭檔：<algorithm>；std::greater 等函式物件在 <functional>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼 std::sort 需要 Random Access Iterator】
// std::sort 的主流實作是 introsort（內省排序）：quicksort 為主，
// 遞迴太深時切換 heapsort，小區間改用 insertion sort。
// 這三種演算法都需要「隨機跳到任意位置」的能力：
//   * quicksort 要選 pivot（常取頭、中、尾的中位數）並從兩端往中間夾
//   * heapsort 要用索引算父子節點（2i+1、2i+2）
//   * 分割時要計算中點 first + (last - first) / 2
// 這些都需要 O(1) 的隨機存取，也就是 Random Access Iterator。
// std::list 只提供 Bidirectional Iterator，**所以 std::sort(lst.begin(), lst.end())
// 根本編譯不過**——這不是效能問題，是編譯期就擋下來的型別錯誤。
// list 有自己的成員函式 lst.sort()，用的是 merge sort（只改指標、不搬移元素）。
//
// 【2. 嚴格弱序（strict weak ordering）：最容易踩到 UB 的地方】
// 傳給 sort 的比較器**必須**滿足嚴格弱序，最關鍵的一條是：
//     comp(a, a) 必須為 false（非自反性，irreflexive）
// 也就是說，比較器要表達的是 **「<」而不是「<=」**。
// 寫成 <= 會發生什麼？
//     std::sort(v.begin(), v.end(), [](int a, int b){ return a <= b; });  // ✗ UB
// 當兩個元素相等時，comp(a,b) 和 comp(b,a) 都回傳 true，排序演算法會認為
// 「a 在 b 前面」且「b 在 a 前面」，內部的邊界判斷因此失效，
// 指標可能跑出容器範圍 → **未定義行為，實務上常見的後果是記憶體越界**。
// 這個 bug 的可怕之處在於：元素少的時候可能完全正常（小區間走 insertion sort），
// 資料量一大才爆炸，而且**不一定每次都爆**。
// 嚴格弱序的完整要求：
//   (a) 非自反：comp(a, a) == false
//   (b) 反對稱：comp(a, b) 為 true 則 comp(b, a) 必為 false
//   (c) 遞移性：comp(a,b) 且 comp(b,c) → comp(a,c)
//   (d) 等價的遞移性：a 與 b 等價、b 與 c 等價 → a 與 c 等價
//       （等價定義為 !comp(a,b) && !comp(b,a)）
//
// 【3. sort 為什麼不保證穩定，而 stable_sort 要另外提供】
// 「穩定」指的是：比較起來相等的元素，排序後仍維持原本的相對順序。
// std::sort 用的 quicksort 會做長距離交換，天生就會打亂相等元素的順序。
// 那為什麼不乾脆都用穩定的演算法？因為**穩定是有代價的**：
//   * stable_sort 通常用 merge sort，需要 O(N) 的額外記憶體
//   * 若配置不到額外記憶體，會退化成 in-place merge，複雜度變 O(N log²N)
//   * std::sort 則是 O(1) 額外空間、O(N log N) 最壞保證，常數也更小
// 所以標準把選擇權交給使用者：**不需要穩定就用 sort（更快、不吃記憶體），
// 需要穩定才用 stable_sort。**
//
// 【4. 什麼時候「穩定」是必要的：多鍵排序】
// 最典型的場景是「先按 A 排，A 相同再按 B 排」。有兩種做法：
//   (a) 一次排序，比較器內處理多個鍵（推薦，用 std::tie 很簡潔）：
//       return std::tie(a.dept, a.name) < std::tie(b.dept, b.name);
//   (b) 多趟 stable_sort，**由次要鍵排到主要鍵**：
//       stable_sort(..., by name);      // 先排次要鍵
//       stable_sort(..., by dept);      // 再排主要鍵，穩定性保住 name 的順序
//   做法 (b) 若用 std::sort 就會失效，因為第二趟會打亂第一趟的結果。
//   注意順序是**反的**：先排次要鍵，最後排主要鍵。
//
// 【概念補充 Concept Deep Dive】
//
// (A) introsort：C++11 把「最壞 O(N log N)」寫進標準的原因
//   純 quicksort 的最壞情況是 O(N²)（例如對已排序資料每次都選到最差 pivot），
//   而且這可以被惡意構造的輸入觸發（quicksort 演算法複雜度攻擊）。
//   introsort 的解法是監控遞迴深度，超過 2*log(N) 就切換到 heapsort
//   （保證 O(N log N)）。所以 C++11 起標準要求 sort 的**最壞情況**
//   也是 O(N log N)，而不只是平均。
//
// (B) 為什麼小區間改用 insertion sort
//   對 16 個元素以下的區間，insertion sort 的常數比 quicksort 小很多
//   （沒有遞迴開銷、cache 友善、幾乎沒有分支預測失敗）。
//   libstdc++ 的門檻值 _S_threshold 是 16——**這是實作細節，非標準規定**。
//
// (C) 比較器該傳 const& 還是傳值
//   對 int 這類小型別傳值即可；對 std::string、自訂結構應傳 const&，
//   否則每次比較都會複製一次物件。排序過程中比較次數是 O(N log N)，
//   複製成本會被放大得很明顯。本檔的 Person 比較器就是用 const&。
//
// 【注意事項 Pay Attention】
// 1. **std::sort 需要 Random Access Iterator**：list / forward_list / set
//    都不能用，會編譯失敗。list 要用成員函式 lst.sort()。
// 2. **比較器必須是嚴格弱序**：用 < 不要用 <=。寫成 <= 是未定義行為，
//    小資料可能沒事、大資料才崩潰，極難除錯。
// 3. std::sort **不保證穩定**；需要保持相等元素原順序必須用 stable_sort。
// 4. 多趟 stable_sort 做多鍵排序時，順序是**由次要鍵排到主要鍵**（反過來）。
// 5. stable_sort 需要 O(N) 額外記憶體；配置失敗會退化成 O(N log²N)。
// 6. 對 std::set / std::map 不需要也不能排序——它們本來就依比較器維持有序，
//    且元素是 const 的，無法被 sort 搬移。
// 7. sort 之後原有的 iterator 仍然有效（vector 沒有重新配置），
//    但它們**指向的元素已經換人了**，語意上通常已無意義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】sort 與 stable_sort
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::sort(lst.begin(), lst.end()) 對 std::list 會編譯失敗？
//     答：std::sort 要求 **Random Access Iterator**，因為它的實作（introsort）
//         需要選 pivot、計算中點、用索引算 heap 父子節點，這些都要 O(1)
//         隨機存取。list 只提供 Bidirectional Iterator，型別檢查在編譯期就擋下來。
//         list 有自己的成員函式 lst.sort()，用 merge sort **只改節點指標、
//         不搬移元素值**，對鏈結串列反而更有效率。
//     追問：那 list::sort 的複雜度是多少？→ 一樣是 O(N log N)，
//         但它不需要隨機存取，且元素本身完全不移動，
//         所以指向元素的 iterator 與 reference 在排序後仍然有效。
//
// 🔥 Q2. 比較器寫成 [](int a, int b){ return a <= b; } 會發生什麼？
//     答：**未定義行為**。sort 要求嚴格弱序，其中一條是非自反性：
//         comp(a, a) 必須為 false。用 <= 時 comp(a, a) 回傳 true，
//         相等的元素會被判定成「互相小於對方」，排序內部的邊界判斷失效，
//         指標可能跑出容器範圍造成記憶體越界。
//     追問：為什麼測試時常常沒事？→ 因為 libstdc++ 對 16 個元素以下的區間
//         走 insertion sort，不會觸發越界；資料量變大走到 quicksort 分割
//         才會爆，而且不保證每次都爆。這正是它難除錯的原因。
//
// 🔥 Q3. 什麼情況下一定要用 stable_sort 而不能用 sort？
//     答：當「相等元素的原始相對順序帶有意義」時。最典型的是多趟排序做多鍵：
//         先 stable_sort 按次要鍵、再 stable_sort 按主要鍵，
//         穩定性保證第二趟不會打亂第一趟的結果。若用 std::sort，
//         第二趟會把次要鍵的順序完全打亂。
//         另一個場景是「按分數排名，同分者維持報名先後」這類業務規則。
//     追問：那為什麼不乾脆都用 stable_sort？→ 因為穩定有代價：
//         stable_sort 需要 O(N) 額外記憶體，配置失敗還會退化成 O(N log²N)；
//         std::sort 是 O(1) 額外空間、常數更小。
//
// ⚠️ 陷阱. 想「先按部門排，部門相同再按姓名排」，寫成這樣對嗎？
//        std::sort(v.begin(), v.end(), byName);
//        std::sort(v.begin(), v.end(), byDept);
//     答：錯。std::sort 不保證穩定，第二次排序會把第一次排好的姓名順序打亂，
//         同部門內的姓名會是任意順序。正確做法有兩種：
//         (a) 改用 stable_sort 跑這兩趟（順序不變，仍是先次要鍵再主要鍵）；
//         (b) 更好的做法是一次排序、比較器內處理兩個鍵：
//             return std::tie(a.dept, a.name) < std::tie(b.dept, b.name);
//     為什麼會錯：多數人知道「多鍵排序要排兩次」，卻沒注意到這個技巧
//         **完全建立在穩定性之上**。用錯演算法，技巧就失效了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>   // std::greater
#include <string>
#include <tuple>        // std::tie

struct Person {
    std::string name;
    int age;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 56. Merge Intervals
//   題目：給定一組區間 intervals，合併所有重疊的區間並回傳結果。
//   為什麼用到本主題：這題的關鍵第一步就是**依區間起點排序**——
//         排序之後，任何可以合併的區間必定相鄰，只要線性掃一次即可。
//         這是「排序讓問題結構浮現」的經典案例，也是 sort 最常見的用途：
//         排序本身不是目的，而是為了讓後續的演算法能用更簡單的方式完成工作。
//   複雜度：O(N log N)，由排序主導。
//   註：這裡不需要穩定性（區間相等時誰先誰後不影響結果），所以用 sort 即可。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> mergeIntervals(std::vector<std::vector<int>> intervals) {
    if (intervals.empty()) return {};

    // 依起點排序（嚴格弱序：用 < 不用 <=）
    std::sort(intervals.begin(), intervals.end(),
              [](const std::vector<int>& a, const std::vector<int>& b) {
                  return a[0] < b[0];
              });

    std::vector<std::vector<int>> merged;
    merged.push_back(intervals[0]);
    for (std::size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i][0] <= merged.back()[1]) {
            // 有重疊 → 延伸目前這段的右界
            merged.back()[1] = std::max(merged.back()[1], intervals[i][1]);
        } else {
            merged.push_back(intervals[i]);
        }
    }
    return merged;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】伺服器 log 依「等級 + 時間」多鍵排序
//   情境：事故調查時要把當天的 log 依嚴重程度分組呈現，
//         同一等級內部必須維持原始的時間先後（也就是原始檔案順序）。
//   為什麼用到本主題：這是**穩定性真的有意義**的場景。
//         示範兩種正確做法，並對照用 std::sort 會怎麼壞掉。
// -----------------------------------------------------------------------------
struct LogEntry {
    std::string time;
    std::string level;   // FATAL / ERROR / WARN / INFO
    std::string message;
};

// 等級的嚴重度排序權重（數字越小越嚴重）
int severityRank(const std::string& level) {
    if (level == "FATAL") return 0;
    if (level == "ERROR") return 1;
    if (level == "WARN")  return 2;
    return 3;                        // INFO 及其他
}

void printLogs(const std::string& title, const std::vector<LogEntry>& logs) {
    std::cout << "  " << title << std::endl;
    for (const auto& e : logs) {
        std::cout << "    " << e.time << " [" << e.level << "] " << e.message << std::endl;
    }
}

int main() {
    // 基本排序, 這裡將 vec 中的元素從小到大排序
    std::cout << "=== sort ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3, 7};
    std::sort(vec.begin(), vec.end());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 自訂比較函數, 這裡將 vec 中的元素從大到小排序
    std::cout << "\n=== sort 自訂比較 ===" << std::endl;
    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // stable_sort：保持相等元素的原始順序, 這裡將 people 按年齡排序，相同年齡的保持原順序
    std::cout << "\n=== stable_sort ===" << std::endl;
    std::vector<Person> people = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 25},
        {"Diana", 30}
    };

    // 按年齡排序，相同年齡的保持原順序, 這裡使用 lambda 表達式作為比較函數
    // ★ 比較器用 <，不能用 <=（<= 違反嚴格弱序，是未定義行為）
    std::stable_sort(people.begin(), people.end(),
        [](const Person& a, const Person& b) {
            return a.age < b.age;
        });

    std::cout << "按年齡 stable_sort:" << std::endl;
    for (const auto& p : people) {
        std::cout << "  " << p.name << ": " << p.age << std::endl;
    }
    std::cout << "  ↑ 25 歲的 Alice 仍排在 Charlie 前面（原始順序被保住）" << std::endl;

    // ★ 一次排序處理多個鍵：用 std::tie 最簡潔
    std::cout << "\n=== 多鍵排序：用 std::tie 一次搞定 ===" << std::endl;
    std::vector<Person> staff = {
        {"Zoe", 30}, {"Adam", 25}, {"Mia", 30}, {"Ben", 25}
    };
    std::sort(staff.begin(), staff.end(),
        [](const Person& a, const Person& b) {
            return std::tie(a.age, a.name) < std::tie(b.age, b.name);
        });
    std::cout << "先依年齡、年齡相同再依姓名:" << std::endl;
    for (const auto& p : staff) {
        std::cout << "  " << p.age << " " << p.name << std::endl;
    }

    std::cout << "\n=== LeetCode 56. Merge Intervals ===" << std::endl;
    auto merged1 = mergeIntervals({{1, 3}, {2, 6}, {8, 10}, {15, 18}});
    std::cout << "[[1,3],[2,6],[8,10],[15,18]] -> ";
    for (const auto& iv : merged1) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
    std::cout << std::endl;

    auto merged2 = mergeIntervals({{1, 4}, {4, 5}});
    std::cout << "[[1,4],[4,5]] -> ";
    for (const auto& iv : merged2) std::cout << "[" << iv[0] << "," << iv[1] << "] ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：log 依嚴重度分組、組內保持時間順序 ===" << std::endl;
    std::vector<LogEntry> logs = {
        {"09:00:01", "INFO",  "service started"},
        {"09:00:05", "ERROR", "db connection refused"},
        {"09:00:07", "WARN",  "cache miss rate high"},
        {"09:00:09", "ERROR", "retry failed"},
        {"09:00:12", "FATAL", "out of memory"},
        {"09:00:15", "INFO",  "shutdown initiated"}
    };
    printLogs("原始順序（依時間）:", logs);

    // 做法 (a)：stable_sort 依等級排序，同等級內自動維持原本的時間順序
    auto byLevel = logs;
    std::stable_sort(byLevel.begin(), byLevel.end(),
        [](const LogEntry& a, const LogEntry& b) {
            return severityRank(a.level) < severityRank(b.level);
        });
    printLogs("stable_sort 依嚴重度（同級維持時間順序）:", byLevel);

    // 做法 (b)：一次排序、比較器處理兩個鍵，結果相同且不依賴穩定性
    // ★ 注意：std::tie 綁的是「參考」，只能吃 lvalue。
    //   寫成 std::tie(severityRank(a.level), a.time) 會編譯失敗，
    //   因為 severityRank(...) 回傳的是暫時值（prvalue），無法綁到 int&。
    //   要把函式回傳值納入多鍵比較，必須先存進區域變數，或改用 std::make_tuple（會複製）。
    auto byTie = logs;
    std::sort(byTie.begin(), byTie.end(),
        [](const LogEntry& a, const LogEntry& b) {
            const int ra = severityRank(a.level);
            const int rb = severityRank(b.level);
            return std::tie(ra, a.time) < std::tie(rb, b.time);
        });
    std::cout << "  用 std::tie 一次排序的結果是否與 stable_sort 相同: "
              << (std::equal(byLevel.begin(), byLevel.end(), byTie.begin(), byTie.end(),
                             [](const LogEntry& a, const LogEntry& b) {
                                 return a.time == b.time && a.level == b.level;
                             }) ? "是" : "否")
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計11.cpp -o demo11

// === 預期輸出 ===
// === sort ===
// 升序: 1 2 3 5 7 8 9 
//
// === sort 自訂比較 ===
// 降序: 9 8 7 5 3 2 1 
//
// === stable_sort ===
// 按年齡 stable_sort:
//   Alice: 25
//   Charlie: 25
//   Bob: 30
//   Diana: 30
//   ↑ 25 歲的 Alice 仍排在 Charlie 前面（原始順序被保住）
//
// === 多鍵排序：用 std::tie 一次搞定 ===
// 先依年齡、年齡相同再依姓名:
//   25 Adam
//   25 Ben
//   30 Mia
//   30 Zoe
//
// === LeetCode 56. Merge Intervals ===
// [[1,3],[2,6],[8,10],[15,18]] -> [1,6] [8,10] [15,18] 
// [[1,4],[4,5]] -> [1,5] 
//
// === 日常實務：log 依嚴重度分組、組內保持時間順序 ===
//   原始順序（依時間）:
//     09:00:01 [INFO] service started
//     09:00:05 [ERROR] db connection refused
//     09:00:07 [WARN] cache miss rate high
//     09:00:09 [ERROR] retry failed
//     09:00:12 [FATAL] out of memory
//     09:00:15 [INFO] shutdown initiated
//   stable_sort 依嚴重度（同級維持時間順序）:
//     09:00:12 [FATAL] out of memory
//     09:00:05 [ERROR] db connection refused
//     09:00:09 [ERROR] retry failed
//     09:00:07 [WARN] cache miss rate high
//     09:00:01 [INFO] service started
//     09:00:15 [INFO] shutdown initiated
//   用 std::tie 一次排序的結果是否與 stable_sort 相同: 是
