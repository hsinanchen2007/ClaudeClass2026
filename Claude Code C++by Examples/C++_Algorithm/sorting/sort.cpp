// ============================================================
// std::sort
// 分類 (Category): Sorting operations (排序演算法)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/sort
//   * https://cplusplus.com/reference/algorithm/sort/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::sort 是 STL 中最常用的排序函式 — 對 [first, last) 進行
// 「就地、不穩定」的排序。
//
// 核心特性:
//   * 預設用 operator< 比較,可改傳自訂比較器 comp
//   * 標準保證「平均/最壞」皆 O(N log N) (C++11 起的最壞保證)
//   * 一般實作為 introsort:quicksort + heapsort + insertion sort 混合
//     - 起初用 quicksort (常數最快)
//     - 遞迴深度過深時轉 heapsort (避免 worst case 退化)
//     - 子範圍小時轉 insertion sort (常數最佳)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼 sort 是「不穩定」的?                            │
// └────────────────────────────────────────────────────────────┘
//
// 「穩定 (stable)」指「相等元素的相對順序保持不變」。
// std::sort 不保證這件事 — 如:
//
//   原:   {2a, 1, 2b}     (2a 與 2b 內容相等,但「位置」不同)
//   sort: 可能變成 {1, 2b, 2a} 或 {1, 2a, 2b}
//
// 為什麼不穩定?因為 quicksort 的 partition 步驟必定要 swap,
// 容易破壞同值元素的相對順序。換來的好處是:
//   * 不需要 O(N) 暫存空間
//   * 平均常數很小,通常比 stable_sort 快
//
// 需要穩定請用 std::stable_sort (代價是 O(N) 空間)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、嚴格弱序 (Strict Weak Ordering) — 比較器的鐵律          │
// └────────────────────────────────────────────────────────────┘
//
// 自訂比較器 comp 必須滿足「嚴格弱序」 — 否則行為未定義 (UB)。
// 條件:
//
//   1. 反對稱:   !(a < a)       (任何元素不小於自身)
//   2. 反對稱性: a < b 與 b < a 不可同時為 true
//   3. 傳遞性:   a < b 且 b < c → a < c
//   4. 等價傳遞: a !< b 且 b !< a 表示「等價」,等價需傳遞
//
// 最常見的錯誤:
//
//   comp = [](int a, int b){ return a <= b; }   // 錯!違反反對稱
//
// 因為 a == b 時,a <= b 為 true 且 b <= a 也為 true,違反規則 2 — UB!
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class RandomIt>
//   void sort(RandomIt first, RandomIt last);
//
//   template <class RandomIt, class Compare>
//   void sort(RandomIt first, RandomIt last, Compare comp);
//
//   * C++20 起為 constexpr。
//   * C++17 起有執行策略多載 (可平行)。
//   * C++20 引入 std::ranges::sort,可帶投影 (projection)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: O(N log N) — 平均 / 最壞 都保證
//   空間: O(log N) (遞迴堆疊)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、需求 (Requirements)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   * RandomAccessIterator — std::list 不能用 std::sort,要用 list::sort
//   * 元素可 MoveConstructible 與 MoveAssignable
//   * comp 需為嚴格弱序
//
// ┌────────────────────────────────────────────────────────────┐
// │ 七、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 不穩定 — 需要穩定請用 stable_sort。
//   2. comp 必須是嚴格弱序 — 不能用 <=、>=。
//   3. 對 list 用 list::sort 成員函式 (它支援 BidirIt)。
//   4. 「只要前 K 名」可改用 partial_sort / nth_element 加速。
//   5. 對小範圍 (< 16),手動寫 insertion sort 有時更快;
//      但通常標準庫已最佳化,別過度造輪子。
//
// ============================================================

/*
補充筆記：std::sort
  - std::sort 需要 random access iterator，因此可用在 vector、array、deque，但不能直接用在 list。
  - 比較器必須形成 strict weak ordering；使用 <= 或會變動結果的比較器是典型錯誤。
  - sort 不保證相等元素的相對順序，需要穩定性時改用 stable_sort。
  - std::sort 屬於排序與選取工具；先分清楚你需要完整排序、前 K 小、還是只要第 N 個元素就定位。
  - sort 不穩定，stable_sort 保留等價元素的原相對順序；資料有次排序鍵時穩定性很重要。
  - nth_element 只保證第 n 個元素就位，左邊不大於它、右邊不小於它，但左右兩邊各自不排序。
  - partial_sort 適合需要前 K 個已排序結果；若 K 遠小於 N，通常比完整 sort 更合適。
  - 比較器必須是 strict weak ordering，不能用 <= 當 less；這是排序初學者常犯的錯。
  - 排序會移動元素，保存的 iterator、pointer、reference 是否有效取決於容器和操作；排序 vector 時元素值位置會改變。
  - std::sort 的比較器若把相等元素也判成 true，例如使用 <=，會破壞排序所需的 strict weak ordering。
  - std::sort 呼叫後元素位置可能改變；若其他資料結構保存索引或指向元素的 iterator，要重新檢查關聯是否仍正確。
  - std::sort 的選擇要看需求：完整排序用 sort/stable_sort，只要第 N 名用 nth_element，只要前 K 名用 partial_sort。
*/
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    // --- 範例 1: 預設遞增排序 ---
    std::vector<int> v{5, 2, 8, 1, 9, 3};
    std::sort(v.begin(), v.end());
    std::cout << "asc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 遞減 (用 std::greater<>) ---
    std::sort(v.begin(), v.end(), std::greater<int>{});
    std::cout << "desc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 3: 自訂 lambda — 依字串長度 ---
    std::vector<std::string> words{"longest", "mid", "a", "longer"};
    std::sort(words.begin(), words.end(),
              [](const std::string& a, const std::string& b){
                  return a.size() < b.size();
              });
    std::cout << "by length: ";
    for (auto& s : words) std::cout << s << ' ';
    std::cout << '\n';

    // --- 範例 4: 對結構體排序 (依 score 降序;score 同則依 name 升序) ---
    struct Player { std::string name; int score; };
    std::vector<Player> p{{"Alice", 80}, {"Bob", 90},
                          {"Carol", 80}, {"Dave", 70}};
    std::sort(p.begin(), p.end(),
              [](const Player& a, const Player& b){
                  if (a.score != b.score) return a.score > b.score;
                  return a.name < b.name;
              });
    std::cout << "leaderboard:";
    for (auto& x : p) std::cout << " " << x.name << "(" << x.score << ")";
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_56_merge_intervals();
    void leetcode_252_meeting_rooms();
    void practical_event_timeline();
    void leetcode_179_largest_number();
    void practical_sort_by_multiple_criteria();
    leetcode_56_merge_intervals();
    leetcode_252_meeting_rooms();
    practical_event_timeline();
    leetcode_179_largest_number();
    practical_sort_by_multiple_criteria();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 56: 合併區間 (Merge Intervals)
// ----------------------------------------------------------------
// 題目:給一組區間 intervals[i] = [start_i, end_i],合併重疊的區間。
//
// 為什麼用 std::sort:
//   區間合併的標準做法 — 先依「起點」排序,讓所有可能重疊的相鄰列在一起,
//   接著線性掃描合併。排序是這個演算法的關鍵第一步。
//
// 解法步驟:
//   1. std::sort 依「a.first < b.first」排序。
//   2. 線性掃描,若當前起點 <= 結果末端的終點 → 合併 (取較大終點);
//      否則加入新區間。
//
// 複雜度:時間 O(N log N) — 排序主導;空間 O(N) (回傳)。
void leetcode_56_merge_intervals() {
    std::vector<std::pair<int,int>> intervals{{1,3},{2,6},{8,10},{15,18}};
    std::sort(intervals.begin(), intervals.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });
    std::vector<std::pair<int,int>> merged;
    for (auto& iv : intervals) {
        if (!merged.empty() && iv.first <= merged.back().second) {
            merged.back().second = std::max(merged.back().second, iv.second);
        } else {
            merged.push_back(iv);
        }
    }
    std::cout << "LC56:";
    for (auto& iv : merged) std::cout << " [" << iv.first << "," << iv.second << "]";
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 252: 會議室 (Meeting Rooms)
// ----------------------------------------------------------------
// 題目:給一組會議時間區間,判斷一個人是否能參加全部 (即區間互不重疊)。
//
// 為什麼用 std::sort:
//   依「起始時間」排序後,只要看相鄰兩場是否衝突就好。
//   排序把「找衝突」從 O(N²) 降到 O(N log N)。
//
// 複雜度:時間 O(N log N);空間 O(1)。
void leetcode_252_meeting_rooms() {
    std::vector<std::pair<int,int>> meetings{{0,30},{5,10},{15,20}};
    std::sort(meetings.begin(), meetings.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });
    bool can_attend = true;
    for (size_t i = 1; i < meetings.size(); ++i) {
        if (meetings[i-1].second > meetings[i].first) { can_attend = false; break; }
    }
    std::cout << "LC252: " << (can_attend ? "true" : "false") << '\n';
}

// ----------------------------------------------------------------
// 實務範例:事件按時序排序 (timeline)
// ----------------------------------------------------------------
// 場景:系統產生一連串事件,各帶時戳;要按時間升冪列出 (穩定性不要求)。
//      常見於 log 分析、event sourcing 重播、稽核報表。
//
// 為什麼用 std::sort:
//   單一 key (timestamp) 的「不穩定排序」 — std::sort 是首選,O(N log N)。
void practical_event_timeline() {
    struct Event { std::string name; long long ts; };
    std::vector<Event> events{
        {"login", 1700000300},
        {"click", 1700000100},
        {"logout", 1700000500},
        {"view", 1700000200},
    };
    std::sort(events.begin(), events.end(),
              [](const Event& a, const Event& b){ return a.ts < b.ts; });
    std::cout << "timeline:";
    for (auto& e : events) std::cout << " " << e.name;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 179: 最大數 (Largest Number)
// 難度: medium
// ----------------------------------------------------------------
// 題目:給非負整數陣列,將它們重排後拼接成最大的整數 (字串形式回傳)。
//
// 為什麼用 std::sort + 自訂比較:
//   核心 trick:對字串 a, b,比較 a+b 與 b+a — 哪個拼接後大,哪個就排前面。
//
// 複雜度:時間 O(N log N × L);空間 O(N) (字串轉換)。
void leetcode_179_largest_number() {
    std::vector<int> nums{3, 30, 34, 5, 9};
    std::vector<std::string> strs;
    for (int x : nums) strs.push_back(std::to_string(x));
    std::sort(strs.begin(), strs.end(),
              [](const std::string& a, const std::string& b){
                  return a + b > b + a;
              });
    std::string ans;
    for (auto& s : strs) ans += s;
    if (ans.front() == '0') ans = "0";   // 全 0 特例
    std::cout << "LC179: " << ans << '\n';
}

// ----------------------------------------------------------------
// 實務範例:多重排序鍵 — 員工依「部門, 年齡」排序
// ----------------------------------------------------------------
// 場景:HR 系統員工列表,先依部門 (字典序),同部門內依年齡 (大到小) 排。
//      std::sort + 多鍵比較 lambda 一次完成。
void practical_sort_by_multiple_criteria() {
    struct Emp { std::string dept; std::string name; int age; };
    std::vector<Emp> emps{
        {"R&D",  "Alice", 35},
        {"HR",   "Bob",   28},
        {"R&D",  "Carol", 42},
        {"HR",   "Dora",  30},
        {"R&D",  "Eve",   25}
    };
    std::sort(emps.begin(), emps.end(),
              [](const Emp& a, const Emp& b){
                  if (a.dept != b.dept) return a.dept < b.dept;
                  return a.age > b.age;
              });
    std::cout << "sorted emps:";
    for (auto& e : emps)
        std::cout << " " << e.dept << "/" << e.name << "(" << e.age << ")";
    std::cout << '\n';
}

// === 預期輸出 (Expected output) ===
// asc: 1 2 3 5 8 9
// desc: 9 8 5 3 2 1
// by length: a mid longer longest
// leaderboard: Bob(90) Alice(80) Carol(80) Dave(70)
// LC56: [1,6] [8,10] [15,18]
// LC252: false
// timeline: click view login logout
// LC179: 9534330
// sorted emps: HR/Dora(30) HR/Bob(28) R&D/Carol(42) R&D/Alice(35) R&D/Eve(25)
