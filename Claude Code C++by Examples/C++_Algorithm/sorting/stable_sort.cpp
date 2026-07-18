// ============================================================
// std::stable_sort
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/stable_sort
//   * https://cplusplus.com/reference/algorithm/stable_sort/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 — 「穩定」二字是關鍵                           │
// └────────────────────────────────────────────────────────────┘
//
// std::stable_sort 跟 std::sort 做的事一樣:依比較器排序整個範圍。
// 唯一差別:
//
//   「『相等元素』的相對順序與輸入時相同。」
//
// 想像情境:你有一份學生資料 {name, score}:
//
//   原:   [Amy/85, Ben/85, Cody/85]
//   sort:        可能變成 [Cody/85, Amy/85, Ben/85]   ← 順序被洗掉
//   stable_sort: 一定是 [Amy/85, Ben/85, Cody/85]    ← 原順序保留
//
// 對「分數相同則保持入榜順序」這類需求,stable_sort 是必要工具。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼穩定排序很重要? (多重排序鍵)                    │
// └────────────────────────────────────────────────────────────┘
//
// 「多重排序」是 stable_sort 的殺手級應用:
//
//   * 「主排班級,次排分數,再依時間順序」 — 怎麼做?
//   * 答:由「最次要 key」往「最主要 key」依序 stable_sort。
//
// 這個技巧的原理:
//   stable_sort 不會破壞「之前已建立的順序」,
//   所以多次 stable_sort 後,前一次的順序會留作「次要 key」。
//
// 範例:依 (klass↑, score↓, 進場順序↑) 排:
//   1. (進場順序↑) 已是天然存在 — 不必操作。
//   2. stable_sort by score↓
//   3. stable_sort by klass↑
//   結果:klass 相同則 score 高在前;score 也相同則進場順序在前。
//
// 這比寫一個「複合比較器」(同時比 klass 與 score) 更彈性 — 你可以
// 各層獨立思考、獨立 sort。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、空間代價 (與 stable_partition 同理)                    │
// └────────────────────────────────────────────────────────────┘
//
// stable_sort 通常實作為「合併排序 (merge sort)」 — 需要 O(N) 暫存。
// 若記憶體不足,實作會 fall back 到 O(N log² N) 的「無暫存版」。
//
//   * 有暫存: O(N log N)
//   * 無暫存: O(N log² N)
//
// 因此「sort 換 stable_sort」是「以空間 + 一些時間 換 順序保證」。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、何時用 stable_sort,何時用 sort?                       │
// └────────────────────────────────────────────────────────────┘
//
//   選 stable_sort 的時機:
//     1. 多重排序鍵 (multi-key) 場景
//     2. 排序後同 key 內的「進場順序」要保留
//     3. 對應 stable_partition 的概念
//
//   不需要穩定 → 直接用 sort,效率好。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void stable_sort(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void stable_sort(RandomIt first, RandomIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、複雜度與需求                                           │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N log N) 有暫存;退化版 O(N log² N)
//   空間: O(N) (暫存) 或 O(1) (退化版)
//   需求: RandomAccessIterator (與 sort 相同)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 只在「需要穩定」時才用 — 否則 sort 更省。
//   2. comp 必須嚴格弱序,與 sort 同。
//   3. 多重排序的順序:從「最次要 key」往「最主要 key」做。
//   4. 對 list 不能用 (要 RandomAccess);改用 list::sort (它本身就是穩定的)。
//
// ============================================================

/*
補充筆記：std::stable_sort
  - stable_sort 的價值是保留等價元素原本相對順序，常用在多欄位排序。
  - 典型做法是先依次要 key 排，再對主要 key stable_sort，或直接寫完整比較器。
  - 它通常比 sort 需要更多資源；穩定性沒有語意需求時不必預設使用。
  - std::stable_sort 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::stable_sort 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::stable_sort 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::stable_sort 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    struct Item { std::string name; int priority; };
    std::vector<Item> v{
        {"a", 2}, {"b", 1}, {"c", 2},
        {"d", 1}, {"e", 3}, {"f", 2}
    };

    // --- 範例 1: 依 priority 升冪;同 priority 保留進場順序 ---
    std::stable_sort(v.begin(), v.end(),
                     [](const Item& x, const Item& y){
                         return x.priority < y.priority;
                     });
    std::cout << "stable sort by priority:";
    for (auto& it : v)
        std::cout << " {" << it.name << "," << it.priority << "}";
    std::cout << '\n';

    // --- 範例 2: 多重排序鍵 — 主依 priority,次依 name 字母 ---
    // 從「最次要 key」(name) 往「最主要 key」(priority) 依序 stable_sort
    std::vector<Item> w{
        {"banana", 2}, {"apple", 2}, {"cherry", 1},
        {"date", 2},   {"berry", 1}
    };
    std::stable_sort(w.begin(), w.end(),
                     [](const Item& a, const Item& b){
                         return a.name < b.name;
                     });
    std::stable_sort(w.begin(), w.end(),
                     [](const Item& a, const Item& b){
                         return a.priority < b.priority;
                     });
    std::cout << "multi-key sort:";
    for (auto& it : w)
        std::cout << " {" << it.name << "," << it.priority << "}";
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_922_sort_array_by_parity_stable();
    void leetcode_multikey_stable();
    void practical_report_multikey();
    void leetcode_2418_sort_people_stable();
    void practical_stable_sort_orders();
    leetcode_922_sort_array_by_parity_stable();
    leetcode_multikey_stable();
    practical_report_multikey();
    leetcode_2418_sort_people_stable();
    practical_stable_sort_orders();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 922 變體:依奇偶分組,保留組內原順序
// ----------------------------------------------------------------
// 題目簡化:把所有偶數排在前、奇數排在後,但同類別內須保持原始輸入順序。
//
// 為什麼用 std::stable_sort:
//   * 把「偶數」當 0、「奇數」當 1,依該值升冪排序。
//   * 穩定性確保「同奇偶」內順序不變。
//
// 複雜度:時間 O(N log N);空間 O(N) (stable_sort 暫存)。
void leetcode_922_sort_array_by_parity_stable() {
    std::vector<int> nums{4, 2, 5, 7, 6, 3, 8, 1};
    std::stable_sort(nums.begin(), nums.end(),
                     [](int a, int b){ return (a % 2) < (b % 2); });
    std::cout << "LC922-stable:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 多重排序示範:學生 (班級, 分數) + 入榜順序
// ----------------------------------------------------------------
// 題目:依「班級升冪」、班內「分數降冪」、分數相同保持原入榜順序。
//
// 為什麼用 std::stable_sort (兩次):
//   1. 先 stable_sort 次要 key (score 降冪)。
//   2. 再 stable_sort 主要 key (klass 升冪)。
//   結果就會是三層排序,且最次要 key (進場順序) 自動保留。
void leetcode_multikey_stable() {
    struct Student { std::string name; int klass; int score; };
    std::vector<Student> v{
        {"Amy",   2, 85},
        {"Ben",   1, 90},
        {"Cody",  2, 90},
        {"Dora",  1, 90},
        {"Eli",   2, 90},
        {"Fred",  1, 85},
    };
    std::stable_sort(v.begin(), v.end(),
                     [](const Student& a, const Student& b){ return a.score > b.score; });
    std::stable_sort(v.begin(), v.end(),
                     [](const Student& a, const Student& b){ return a.klass < b.klass; });
    std::cout << "multikey:";
    for (auto& s : v) std::cout << " {" << s.name << "," << s.klass << "," << s.score << "}";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:銷售報表多重排序
// ----------------------------------------------------------------
// 場景:報表要求 (region 升冪, revenue 降冪, emp_id 升冪)。
//
// 為什麼用 std::stable_sort (依次三次):
//   多重排序鍵的標準解 — 從最次要 key 往最主要 key 做 stable_sort。
//   優點:每層獨立、可任意調換主次順序、可加減層數。
void practical_report_multikey() {
    struct Sale { std::string region; int revenue; int emp_id; };
    std::vector<Sale> data{
        {"East", 500, 7},
        {"West", 800, 3},
        {"East", 800, 5},
        {"West", 800, 2},
        {"East", 800, 1},
    };
    std::stable_sort(data.begin(), data.end(),
                     [](const Sale& a, const Sale& b){ return a.emp_id < b.emp_id; });
    std::stable_sort(data.begin(), data.end(),
                     [](const Sale& a, const Sale& b){ return a.revenue > b.revenue; });
    std::stable_sort(data.begin(), data.end(),
                     [](const Sale& a, const Sale& b){ return a.region < b.region; });
    std::cout << "report:";
    for (auto& s : data)
        std::cout << " {" << s.region << "," << s.revenue << ",#" << s.emp_id << "}";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 2418: 依身高排序人員 (Sort the People)
// ----------------------------------------------------------------
// 題目:給名字陣列 names 與身高陣列 heights,依身高「降冪」排序回傳名字。
//      同身高保留原順序 (stable)。
//
// 為什麼用 std::stable_sort:
//   題意要求穩定 — 同身高的人保留原始位置順序。
//   把 (name, height) 配對後,以 height 降冪 stable_sort。
//
// 複雜度:時間 O(N log N);空間 O(N)。
void leetcode_2418_sort_people_stable() {
    std::vector<std::string> names{"Mary", "John", "Emma"};
    std::vector<int> heights{180, 165, 170};
    std::vector<std::pair<std::string, int>> people;
    for (size_t i = 0; i < names.size(); ++i)
        people.push_back({names[i], heights[i]});
    std::stable_sort(people.begin(), people.end(),
                     [](const auto& a, const auto& b){ return a.second > b.second; });
    std::cout << "LC2418:";
    for (auto& p : people) std::cout << " " << p.first;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:訂單依「金額分群」展示,組內保留下單順序
// ----------------------------------------------------------------
// 場景:UI 把訂單按金額分檔顯示 (大檔/中檔/小檔),
//      但組內要保留「下單時序」 — 穩定排序剛好對應。
void practical_stable_sort_orders() {
    struct Order { int seq; double amount; };
    std::vector<Order> orders{
        {1, 50}, {2, 200}, {3, 800}, {4, 50}, {5, 200}, {6, 50}
    };
    auto tier = [](double a){ return a < 100 ? 0 : a < 500 ? 1 : 2; };
    std::stable_sort(orders.begin(), orders.end(),
                     [&](const Order& a, const Order& b){
                         return tier(a.amount) < tier(b.amount);
                     });
    std::cout << "tiered orders:";
    for (auto& o : orders) std::cout << " #" << o.seq << "(" << o.amount << ")";
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// stable sort by priority: {b,1} {d,1} {a,2} {c,2} {f,2} {e,3}
// multi-key sort: {berry,1} {cherry,1} {apple,2} {banana,2} {date,2}
// LC922-stable: 4 2 6 8 5 7 3 1
// multikey: {Ben,1,90} {Dora,1,90} {Fred,1,85} {Cody,2,90} {Eli,2,90} {Amy,2,85}
// report: {East,800,#1} {East,800,#5} {East,500,#7} {West,800,#2} {West,800,#3}
// LC2418: Mary Emma John
// tiered orders: #1(50) #4(50) #6(50) #2(200) #5(200) #3(800)
