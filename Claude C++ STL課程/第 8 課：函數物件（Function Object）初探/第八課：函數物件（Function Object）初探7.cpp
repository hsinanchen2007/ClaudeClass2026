// =============================================================================
//  第八課：函數物件 7  —  <functional> 的比較類函數物件
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<functional>
//   六個比較函數物件（全部是 C++98）：
//       std::equal_to<T>       a == b
//       std::not_equal_to<T>   a != b
//       std::greater<T>        a >  b
//       std::less<T>           a <  b        ← 整個標準函式庫的預設排序語意
//       std::greater_equal<T>  a >= b
//       std::less_equal<T>     a <= b
//   透明版本 std::less<> 等：**C++14**（本機以 -pedantic-errors 實測確認）。
//   C++14 起 operator() 是 constexpr。
//   ★ 只有 less 與 greater 適合當**排序比較器**；
//     _equal 版本違反嚴格弱序，絕不可傳給 sort / set / map（見下方陷阱）。
//
// 【詳細解釋 Explanation】
//
// 【1. std::less 是整個標準函式庫的「預設語意」】
//   凡是需要排序或有序性的地方，預設都是 std::less：
//       std::sort(first, last)                    → less
//       std::set<T> / std::map<K,V>               → less<K>
//       std::priority_queue<T>                    → less（但語意是大根堆，見 Q2）
//       std::lower_bound / std::binary_search     → less
//   這個統一約定的好處是：自訂型別只要提供一個 operator<，
//   整個標準函式庫馬上全部可用。
//
// 【2. 這六個裡面只有兩個能當排序比較器】
//   這是本檔最關鍵、也最容易出事的一點。
//   排序比較器必須滿足**嚴格弱序**，第一條是 comp(a, a) == false。
//       std::less<int>{}(5, 5)          → false  ✓ 合格
//       std::greater<int>{}(5, 5)       → false  ✓ 合格
//       std::less_equal<int>{}(5, 5)    → true   ✗ 違反！
//       std::greater_equal<int>{}(5, 5) → true   ✗ 違反！
//   把 std::less_equal 傳給 std::sort 或 std::set 是**未定義行為**，
//   可能崩潰、可能默默寫壞記憶體、也可能小資料下看起來正常。
//   equal_to / not_equal_to 則根本不是序關係，用途是
//   std::find_if、std::search、unordered 容器的 KeyEqual 參數。
//
// 【3. 為什麼要有這些「看起來很廢」的包裝】
//   同第 6 檔：運算子不是物件，傳不進演算法。
//   但比較類的實用性遠高於算術類，因為它們最常出現在**型別參數**的位置：
//       std::set<int, std::greater<int>>                   遞減的 set
//       std::priority_queue<int, std::vector<int>, std::greater<int>>  小根堆
//       std::map<std::string, int, std::less<>>            支援異質查找
//   在這些地方 lambda 不好用（C++20 前無法預設建構），
//   所以這批 functor 至今仍是主流寫法。
//
// 【4. std::less<T*> 的一個冷門但重要的保證】
//   對指標，內建的 < 只在「同一個陣列/物件內」才有定義的順序，
//   比較兩個不相關的指標是未指定的（unspecified）。
//   但 std::less<T*> 被標準**特別要求**提供一個全序（total order）——
//   所以 std::set<T*> 是合法可用的，而直接寫 p1 < p2 嚴格說不保證。
//   這是「為什麼要用 std::less 而不是直接寫 <」的一個真實理由。
//
// 【概念補充 Concept Deep Dive】
//
// (A) priority_queue 用 less 為什麼是「大根堆」
//     這是最反直覺的地方。priority_queue 的定義是
//     「top() 回傳依比較器**最大**的元素」。
//     用 std::less（升序語意）時，「最大」就是數值最大 → 大根堆。
//     要小根堆得反過來傳 std::greater：
//         std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;
//     記法：sort 的比較器決定「誰排前面」，
//           priority_queue 的比較器決定「誰排最後 = 誰先出來」。
//
// (B) equal_to 與 unordered 容器
//     std::unordered_map<K, V, Hash, KeyEqual> 的第四個參數預設是
//     std::equal_to<K>。自訂型別當鍵時要同時提供 Hash 與 KeyEqual，
//     而且必須保證「相等的物件雜湊值一定相同」，否則查找會失敗。
//
// (C) 浮點數與 NaN 會破壞所有比較的性質
//     NaN 與任何值（包括自己）比較都回傳 false。
//     所以含 NaN 的序列傳給 sort 會違反嚴格弱序 → 未定義行為。
//     C++20 的 std::ranges 版本一樣不救；要安全排序浮點必須先濾掉 NaN，
//     或改用 std::strong_order（C++20）。
//
// (D) 透明版本 std::less<> 的異質查找
//     std::set<std::string, std::less<>> s;
//     s.find("hello");        // 直接用 const char*，不建立臨時 std::string
//     沒有 std::less<> 的話，find 的參數型別固定是 std::string，
//     每次查找都要配置一次記憶體。對高頻查找這是實質的效能差異。
//
// 【注意事項 Pay Attention】
//   1. **絕不可**把 less_equal / greater_equal 當排序比較器——
//      它們違反嚴格弱序，是未定義行為。
//   2. equal_to / not_equal_to 不是序關係，不能給 sort / set 用。
//   3. priority_queue 的 less 是**大根堆**，要小根堆用 greater。
//   4. 浮點含 NaN 時所有比較都失效，排序前必須先處理。
//   5. std::less<T*> 保證全序；直接寫 p1 < p2 對不相關指標則不保證。
//   6. 透明版本 std::less<> 是 C++14。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】比較類函數物件
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. <functional> 的六個比較 functor，哪些可以當 std::sort 的比較器？
//     答：只有 std::less 和 std::greater。
//         排序比較器必須滿足嚴格弱序，第一條就是 comp(a, a) 必須為 false。
//         less_equal 和 greater_equal 對相等的元素回傳 true，直接違反，
//         傳給 sort / set / map 是**未定義行為**。
//         equal_to / not_equal_to 根本不是序關係，
//         它們的用途是 find_if、search、以及 unordered 容器的 KeyEqual。
//     追問：那為什麼標準還要提供 less_equal？
//         → 它們是給「單純需要一個比較動作」的場合用的，
//           例如當成一般述詞傳給 find_if、或用在 partition 的條件。
//           標準提供的是完整的運算子包裝，不代表每個都適合當排序比較器。
//
// 🔥 Q2. std::priority_queue<int> 預設用 std::less，為什麼它是**大**根堆？
//        sort 用 less 明明是升序。
//     答：因為兩者的語意層級不同。sort 的比較器定義「誰排在前面」，
//         comp(a,b)==true 表示 a 在 b 前面 → less 得到升序。
//         priority_queue 的定義則是「top() 回傳依比較器**最大**的元素」，
//         用 less 時「最大」就是數值最大 → 大根堆、每次彈出最大值。
//         要小根堆必須反過來：
//             std::priority_queue<int, std::vector<int>, std::greater<int>>
//     追問：這在解題時怎麼記？
//         → 「Top K 大的元素」要維護一個 **小**根堆（greater），
//           因為要能快速丟掉目前最小的那個。方向常常和直覺相反，
//           寫完務必用小例子驗證一次。
//
// ⚠️ 陷阱. 「我想讓排序把相等的元素也算進去，
//         所以用 std::less_equal<int>() 當比較器，這樣更完整吧？」
//     答：這是未定義行為。std::less_equal{}(5, 5) 回傳 true，
//         違反嚴格弱序的第一條（非自反性）。
//         後果不是「排序結果略有不同」，而是 quicksort 的分割迴圈
//             while (comp(*i, pivot)) ++i;
//         失去終止保證——它沒有邊界檢查，靠「一定有元素讓 comp 回傳 false」
//         停下來。指標可能一路衝出陣列，變成越界存取。
//         同樣的比較器給 std::set 會讓紅黑樹的不變式被破壞。
//     為什麼會錯：把比較器理解成「要不要把這兩個算成有序關係」，
//         想讓它「包含等於的情況」。
//         但比較器回答的問題是固定的：「a 是否**嚴格**排在 b 前面？」
//         相等的情況答案就該是 false——這不是遺漏，是定義。
//         排序需要知道的「相等」是靠 comp(a,b) 與 comp(b,a) **都為 false**
//         推導出來的，不需要也不可以由比較器直接回報。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include <string>
#include <algorithm>
#include <functional>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔刻意不放
//   理由：本檔的內容是「<functional> 提供了哪六個比較 functor、
//   哪些能當排序比較器」，屬於標準函式庫的**清單與合約**知識。
//   自訂比較器真正發揮價值的解題場景，本課第 2 檔已經用
//   LeetCode 179. Largest Number 完整示範過了（那題的難點全在比較器設計）。
//   在這裡再掛一題，會變成「同一個觀念換一組數字再講一次」，
//   對讀者沒有新增資訊。
//   本檔改用「排行榜 Top-N」的實務範例，因為它同時用到
//   std::greater 的兩種身分（sort 的引數 / priority_queue 的型別參數），
//   那才是這批 functor 不可取代的地方。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】遊戲排行榜：取分數最高的 Top-N
//   情境：線上遊戲有數萬名玩家，首頁要顯示分數最高的前 10 名。
//         完整排序全部玩家是浪費——只需要前 N 名。
//   為什麼用到本主題：這個場景一次用到 std::greater 的**兩種身分**：
//     (1) 當**引數**傳給 std::partial_sort —— 只排出前 N 名，O(n log N)，
//         比完整 sort 的 O(n log n) 省很多
//     (2) 當**型別參數**給 std::priority_queue 做成小根堆 —— 串流資料時
//         只需 O(N) 記憶體就能維護 Top-N，資料再多也不必全部載入
//   第 (2) 點的方向最反直覺：要「最大的 N 個」，維護的是**小**根堆，
//   因為堆頂是目前 N 個裡最小的，新資料只要比它大就換掉它。
// -----------------------------------------------------------------------------
struct Player {
    std::string name;
    int         score;
};

struct ByScoreDesc {
    bool operator()(const Player& a, const Player& b) const {
        if (a.score != b.score) return a.score > b.score;
        return a.name < b.name;          // 同分時用名字打破平手，結果才穩定
    }
};

// 方法一：全部資料在手 → partial_sort 只排前 N 名
std::vector<Player> topNByPartialSort(std::vector<Player> players, std::size_t n) {
    n = std::min(n, players.size());
    std::partial_sort(players.begin(), players.begin() + static_cast<long>(n),
                      players.end(), ByScoreDesc{});
    players.resize(n);
    return players;
}

// 方法二：串流資料 → 維護一個大小為 N 的「小根堆」
//   堆頂是目前 Top-N 裡分數最低的；新資料比它高就替換掉。
//   記憶體只需 O(N)，不必把全部玩家載入。
struct ByScoreAsc {                       // 小根堆需要「反向」的比較器
    bool operator()(const Player& a, const Player& b) const {
        if (a.score != b.score) return a.score > b.score;   // 注意方向
        return a.name < b.name;
    }
};

std::vector<Player> topNByHeap(const std::vector<Player>& stream, std::size_t n) {
    // priority_queue 的 top() 是「依比較器最大」的元素。
    // 傳 ByScoreAsc（分數小的算「大」）→ top() 就是分數最低的 → 小根堆。
    std::priority_queue<Player, std::vector<Player>, ByScoreAsc> heap;
    for (const auto& p : stream) {
        heap.push(p);
        if (heap.size() > n) heap.pop();      // 踢掉目前最低分的
    }
    std::vector<Player> out;
    while (!heap.empty()) { out.push_back(heap.top()); heap.pop(); }
    std::reverse(out.begin(), out.end());     // 堆是由低到高彈出，反過來
    return out;
}

int main() {
    std::cout << "=== 比較類函數物件 ===" << std::endl;

    // equal_to<T>：相等
    std::equal_to<int> eq;
    std::cout << "equal_to: 5 == 5? " << (eq(5, 5) ? "true" : "false") << std::endl;

    // not_equal_to<T>：不相等
    std::not_equal_to<int> neq;
    std::cout << "not_equal_to: 5 != 3? " << (neq(5, 3) ? "true" : "false") << std::endl;

    // greater<T>：大於
    std::greater<int> gt;
    std::cout << "greater: 5 > 3? " << (gt(5, 3) ? "true" : "false") << std::endl;

    // less<T>：小於
    std::less<int> lt;
    std::cout << "less: 3 < 5? " << (lt(3, 5) ? "true" : "false") << std::endl;

    // greater_equal<T>：大於等於
    std::greater_equal<int> gte;
    std::cout << "greater_equal: 5 >= 5? " << (gte(5, 5) ? "true" : "false") << std::endl;

    // less_equal<T>：小於等於
    std::less_equal<int> lte;
    std::cout << "less_equal: 3 <= 5? " << (lte(3, 5) ? "true" : "false") << std::endl;

    // 實際應用：降序排序
    std::cout << "\n=== 實際應用：排序 ===" << std::endl;
    std::vector<int> vec = {5, 2, 8, 1, 9, 3};

    std::sort(vec.begin(), vec.end(), std::greater<int>());
    std::cout << "降序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    std::sort(vec.begin(), vec.end(), std::less<int>());
    std::cout << "升序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // -------------------------------------------------------------------------
    std::cout << "\n=== 六個裡面只有兩個能當排序比較器 ===" << std::endl;
    {
        std::cout << "嚴格弱序的第一條：comp(a, a) 必須是 false" << std::endl;
        std::cout << std::boolalpha;
        std::cout << "  less<int>{}(5,5)          = " << std::less<int>{}(5, 5)
                  << "   ✓ 可當排序比較器" << std::endl;
        std::cout << "  greater<int>{}(5,5)       = " << std::greater<int>{}(5, 5)
                  << "   ✓ 可當排序比較器" << std::endl;
        std::cout << "  less_equal<int>{}(5,5)    = " << std::less_equal<int>{}(5, 5)
                  << "    ✗ 違反！給 sort/set 是未定義行為" << std::endl;
        std::cout << "  greater_equal<int>{}(5,5) = " << std::greater_equal<int>{}(5, 5)
                  << "    ✗ 違反！" << std::endl;
        std::cout << "  equal_to / not_equal_to 根本不是序關係，" << std::endl;
        std::cout << "    用途是 find_if、search、unordered 容器的 KeyEqual。"
                  << std::endl;
        std::cout << "→ 這裡只做合約檢查，不實際把違規比較器傳給 sort（那是 UB）。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== priority_queue 的 less 為什麼是「大根堆」 ===" << std::endl;
    {
        // 預設（less）：top() 是「依比較器最大」的元素 → 數值最大 → 大根堆
        std::priority_queue<int> maxHeap;
        // greater：把「小」當成「大」→ top() 是數值最小 → 小根堆
        std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;

        for (int x : {5, 2, 8, 1, 9, 3}) { maxHeap.push(x); minHeap.push(x); }

        std::cout << "資料 {5,2,8,1,9,3}" << std::endl;
        std::cout << "  預設（less）  top() = " << maxHeap.top()
                  << "  ← 大根堆，每次彈出最大值" << std::endl;
        std::cout << "  用 greater    top() = " << minHeap.top()
                  << "  ← 小根堆，每次彈出最小值" << std::endl;

        std::cout << "  大根堆彈出順序: ";
        while (!maxHeap.empty()) { std::cout << maxHeap.top() << " "; maxHeap.pop(); }
        std::cout << std::endl;
        std::cout << "  小根堆彈出順序: ";
        while (!minHeap.empty()) { std::cout << minHeap.top() << " "; minHeap.pop(); }
        std::cout << std::endl;
        std::cout << "→ sort 的比較器決定「誰排前面」；" << std::endl;
        std::cout << "  priority_queue 的比較器決定「誰是最大 = 誰先出來」。"
                  << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 當「型別參數」用：這是 lambda 難取代的地方 ===" << std::endl;
    {
        std::set<int, std::greater<int>> desc = {5, 2, 8, 1, 9, 3};
        std::cout << "std::set<int, std::greater<int>>: ";
        for (int n : desc) std::cout << n << " ";
        std::cout << std::endl;

        // C++14 透明版本：異質查找，不建立臨時 std::string
        std::set<std::string, std::less<>> names = {"alice", "bob", "carol"};
        auto it = names.find("bob");        // 直接用 const char*
        std::cout << "std::set<string, std::less<>>.find(\"bob\") 找到了嗎? "
                  << std::boolalpha << (it != names.end()) << std::endl;
        std::cout << "  ← 透明比較器（C++14）讓 find 不必為了查一次"
                  << "而建立臨時 std::string" << std::endl;
    }

    // -------------------------------------------------------------------------
    std::cout << "\n=== 日常實務：遊戲排行榜 Top-3 ===" << std::endl;
    {
        const std::vector<Player> players = {
            {"nova",     8420},
            {"kestrel",  9310},
            {"draco",    7650},
            {"lyra",     9310},
            {"orion",    9905},
            {"vega",     6180},
            {"atlas",    8420},
            {"phoenix",  5040},
        };

        std::cout << "全部玩家 " << players.size() << " 人" << std::endl;

        auto top3a = topNByPartialSort(players, 3);
        std::cout << "方法一 partial_sort（O(n log N)，只排前 N 名）:" << std::endl;
        int rank = 1;
        for (const auto& p : top3a)
            std::cout << "    " << rank++ << ". " << p.name << "  " << p.score << std::endl;

        auto top3b = topNByHeap(players, 3);
        std::cout << "方法二 小根堆（O(N) 記憶體，適合串流資料）:" << std::endl;
        rank = 1;
        for (const auto& p : top3b)
            std::cout << "    " << rank++ << ". " << p.name << "  " << p.score << std::endl;

        bool same = std::equal(top3a.begin(), top3a.end(), top3b.begin(),
            [](const Player& a, const Player& b) {
                return a.name == b.name && a.score == b.score;
            });
        std::cout << "  兩種方法結果一致? " << std::boolalpha << same << std::endl;
        std::cout << "→ 要「最大的 N 個」卻維護**小**根堆，方向和直覺相反：" << std::endl;
        std::cout << "  堆頂是目前 N 個裡最小的，新資料比它大才值得換掉它。"
                  << std::endl;
        std::cout << "  同分時用名字打破平手，結果才穩定可重現。" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第八課：函數物件（Function Object）初探7.cpp -o demo7
//
// ⚠️ 本檔用到透明比較器 std::less<>（C++14），最低需要 -std=c++14。

// === 預期輸出 ===
// === 比較類函數物件 ===
// equal_to: 5 == 5? true
// not_equal_to: 5 != 3? true
// greater: 5 > 3? true
// less: 3 < 5? true
// greater_equal: 5 >= 5? true
// less_equal: 3 <= 5? true
//
// === 實際應用：排序 ===
// 降序: 9 8 5 3 2 1
// 升序: 1 2 3 5 8 9
//
// === 六個裡面只有兩個能當排序比較器 ===
// 嚴格弱序的第一條：comp(a, a) 必須是 false
//   less<int>{}(5,5)          = false   ✓ 可當排序比較器
//   greater<int>{}(5,5)       = false   ✓ 可當排序比較器
//   less_equal<int>{}(5,5)    = true    ✗ 違反！給 sort/set 是未定義行為
//   greater_equal<int>{}(5,5) = true    ✗ 違反！
//   equal_to / not_equal_to 根本不是序關係，
//     用途是 find_if、search、unordered 容器的 KeyEqual。
// → 這裡只做合約檢查，不實際把違規比較器傳給 sort（那是 UB）。
//
// === priority_queue 的 less 為什麼是「大根堆」 ===
// 資料 {5,2,8,1,9,3}
//   預設（less）  top() = 9  ← 大根堆，每次彈出最大值
//   用 greater    top() = 1  ← 小根堆，每次彈出最小值
//   大根堆彈出順序: 9 8 5 3 2 1
//   小根堆彈出順序: 1 2 3 5 8 9
// → sort 的比較器決定「誰排前面」；
//   priority_queue 的比較器決定「誰是最大 = 誰先出來」。
//
// === 當「型別參數」用：這是 lambda 難取代的地方 ===
// std::set<int, std::greater<int>>: 9 8 5 3 2 1
// std::set<string, std::less<>>.find("bob") 找到了嗎? true
//   ← 透明比較器（C++14）讓 find 不必為了查一次而建立臨時 std::string
//
// === 日常實務：遊戲排行榜 Top-3 ===
// 全部玩家 8 人
// 方法一 partial_sort（O(n log N)，只排前 N 名）:
//     1. orion  9905
//     2. kestrel  9310
//     3. lyra  9310
// 方法二 小根堆（O(N) 記憶體，適合串流資料）:
//     1. orion  9905
//     2. kestrel  9310
//     3. lyra  9310
//   兩種方法結果一致? true
// → 要「最大的 N 個」卻維護**小**根堆，方向和直覺相反：
//   堆頂是目前 N 個裡最小的，新資料比它大才值得換掉它。
//   同分時用名字打破平手，結果才穩定可重現。
