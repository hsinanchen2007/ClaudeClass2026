// =============================================================================
//  第六課 7 — std::multiset：允許重複的有序集合，與「等價」而非「相等」
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key,
//            class Compare   = std::less<Key>,
//            class Allocator = std::allocator<Key>>
//   class multiset;                                   // <set>,C++98 起
//
//   常用介面與複雜度：
//     iterator insert(const Key&);          // O(log n)；回傳 iterator（不是 pair！）
//     size_type count(const Key&) const;    // O(log n + k)，k = 等價元素個數
//     iterator find(const Key&);            // O(log n)，回傳「某一個」等價元素
//     pair<iterator,iterator> equal_range(const Key&) const;   // O(log n)
//     iterator lower_bound(const Key&);     // O(log n)，第一個「不小於」的位置
//     iterator upper_bound(const Key&);     // O(log n)，第一個「大於」的位置
//     size_type erase(const Key&);          // O(log n + k)，刪除【全部】等價元素
//     iterator  erase(const_iterator);      // 攤銷 O(1)，只刪這一個
//
//   走訪順序：依 Compare 由小到大（決定性，不像 unordered_* 那樣未指定）
//   標頭檔：#include <set>（multiset 和 set 同一個標頭檔）
//
// 【詳細解釋 Explanation】
//
// 【1. multiset 與 set 只差一條不變式】
// 兩者用的是**同一棵平衡二元搜尋樹**(libstdc++ 為紅黑樹,屬**實作定義**),
// 同樣是 O(log n)、同樣有序、同樣的迭代器穩定性。唯一的差別是一條不變式:
//     set      :樹中不存在兩個等價的元素(insert 遇到等價者直接放棄)
//     multiset :允許任意多個等價元素共存(insert 永遠成功)
//
// 這條差異直接改變了介面的形狀:
//   * set::insert 回傳 pair<iterator, bool> —— bool 告訴你「是否真的插入了」。
//   * multiset::insert 回傳單純的 iterator —— 因為它**永遠會插入成功**,
//     沒有「插不進去」這種情況,回傳 bool 就沒有意義了。
// 這是「介面反映不變式」的好例子:型別簽名本身就在說明語意。
//
// 【2. 最關鍵的概念:等價(equivalence)不是相等(equality)】
// 這是 set/multiset 家族最容易被誤解、也最常在實務上咬人的一點。
//
// multiset **從來不呼叫 operator==**。它判斷兩個元素「算不算同一類」,靠的是
// 比較器 Compare(預設 std::less<Key>),定義為:
//     a 與 b 等價  ⟺  !(a < b) && !(b < a)
//
// 對 int 這種簡單型別,等價和相等剛好一致,所以看不出差別。但只要比較器
// 只看物件的一部分,兩者就會分道揚鑣。例如:
//     struct Task { int priority; std::string name; };
//     比較器只比 priority
// 那麼 {3, "備份"} 與 {3, "寄信"} 就是**等價**的(儘管它們並不相等)。
// 於是:
//   * 在 set 裡,第二個插不進去 —— 你會神祕地掉資料。
//   * 在 multiset 裡兩個都留著,count(Task{3,...}) 回傳 2,
//     equal_range 會同時撈出這兩筆。
//
// 換句話說:**比較器定義了「同一類」的界線**。設計比較器時要想清楚你要的是
// 「完整識別」還是「只依某個鍵排序」,兩者的後果完全不同。
//
// 【3. 為什麼 count() 不是 O(1),而是 O(log n + k)】
// 等價的元素在樹中必然形成一段**連續區間**(因為樹是依比較器排序的)。
// count() 的做法是:先 O(log n) 找到區間左端(lower_bound)、再 O(log n) 找到
// 右端(upper_bound),然後**實際走過**這段區間去數個數 —— 走訪 k 個元素
// 就是 O(k)。
//
// 這解釋了兩件事:
//   * 若你只想知道「有沒有」,用 find() != end()(O(log n))即可,
//     不要用 count() > 0 —— 後者在重複元素極多時會白白走訪一整段。
//   * 若你要的是那些元素本身,直接用 equal_range 拿到區間,
//     不必先 count 再走一次(那等於走了兩趟)。
//
// 【4. erase 的兩種語意:最常見的意外】
// multiset 的 erase 有兩個語意完全不同的多載,混淆它們是實務上真實會發生的 bug:
//     ms.erase(30);        // 依【值】刪除 → 刪掉【全部】等價於 30 的元素
//                          //   回傳被刪除的個數
//     ms.erase(it);        // 依【iterator】刪除 → 只刪掉那一個
//                          //   回傳下一個元素的 iterator
//
// 「我只想刪掉一個 30」的正確寫法是:
//     auto it = ms.find(30);
//     if (it != ms.end()) ms.erase(it);      // 只刪一個
// 直接寫 ms.erase(30) 會把三個 30 全部刪光。這個差異在 set 上看不出來
// (set 本來就只有一個),所以從 set 換到 multiset 時特別容易踩到。
//
// 【5. C++11 起保證等價元素維持插入順序】
// C++11 明文規定:等價的元素在容器中會依**插入的先後順序**排列
// (即新插入的等價元素會排在既有等價元素之後)。C++03 並沒有這個保證。
//
// 這讓 multiset 可以安全地當成「穩定的優先佇列」使用 —— 相同優先權的工作
// 會依進入順序被處理,不會莫名其妙被重排。若沒有這條保證,任何依賴
// FIFO 公平性的排程邏輯都是不可靠的。
//
// 【概念補充 Concept Deep Dive】
//
// (A) lower_bound / upper_bound 是怎麼運作的
//     兩者都是在紅黑樹上做二分下降,各 O(log n):
//       lower_bound(v) → 第一個「不小於 v」的位置,即等價區間的**起點**
//       upper_bound(v) → 第一個「大於 v」的位置,  即等價區間的**終點(不含)**
//     所以 equal_range(v) 就等於 {lower_bound(v), upper_bound(v)},
//     而 count(v) == distance(lower_bound(v), upper_bound(v))。
//     這組介面是有序容器獨有的能力 —— unordered_multiset 完全做不到範圍查詢,
//     因為它的元素在記憶體中根本沒有順序關係。
//
// (B) 元素是 const 的
//     multiset 的 iterator 解參照得到的是 const Key&,你不能就地修改元素。
//     原因和 set 相同:元素的值決定了它在樹中的位置,就地修改會立刻破壞排序
//     不變式,之後所有查找都會失準。C++17 之後若真要改值又不想重新配置節點,
//     正確做法是 extract() 取出 node handle → 修改 → 再 insert。
//
// (C) 迭代器與 reference 的穩定性
//     和 set/list 一樣,multiset 是節點式容器:節點一旦配置就不再移動。
//     插入完全不影響既有 iterator;刪除只讓**被刪的那一個**失效。
//     這點與 vector(重新配置就全滅)形成強烈對比。
//
// (D) multiset 與 priority_queue 的取捨
//     兩者都能「反覆取出極值」,但能力不同:
//       priority_queue:只能看堆頂、只能從一端取;但常數小、記憶體連續。
//       multiset      :可以走訪全部元素、可以刪除任意元素、可以做範圍查詢、
//                       可以同時取最小(begin())與最大(rbegin());代價是
//                       節點配置與 cache 不友善。
//     需要「取極值 + 中途刪除任意元素」時(例如滑動視窗),只有 multiset 做得到。
//
// 【注意事項 Pay Attention】
//  1. **erase(value) 會刪掉全部等價元素**,不是一個。只想刪一個請先 find()
//     再 erase(iterator)。這是從 set 換到 multiset 時最常見的意外。
//  2. count() 是 O(log n + k) 不是 O(1)。只想知道「存不存在」請用
//     find() != end(),或 C++20 的 contains()。
//  3. 比較器必須是**嚴格弱序**(strict weak ordering)。用 <= 取代 < 會破壞
//     不變式,導致行為錯誤(可能查不到明明存在的元素,甚至更難預期的結果)。
//  4. multiset 用的是**等價**(!(a<b) && !(b<a))而非 operator==。
//     比較器只看部分欄位時,兩個不相等的物件仍可能被視為等價。
//  5. 元素是 const,不可就地修改;需要改值請用 C++17 的 extract() + insert。
//  6. multiset 的迭代器是**雙向**的,不能隨機存取。std::next(it, k) 是 O(k)
//     的逐步走訪,在迴圈裡這樣做會變成 O(n·k)。
//  7. 底層是紅黑樹屬**實作定義**;標準只保證 O(log n) 與有序,未規定樹的種類。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::multiset
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. multiset 和 set 的差別是什麼?為什麼兩者的 insert 回傳型別不同?
//     答：底層是同一棵平衡二元搜尋樹,差別只有一條不變式 —— set 不允許等價元素,
//         multiset 允許。因此 set::insert 回傳 pair<iterator,bool>,那個 bool
//         用來告訴你「是否真的插入了」;而 multiset::insert **永遠會成功**,
//         不存在插不進去的情況,所以只回傳 iterator。介面形狀直接反映了不變式。
//     追問：那 multiset 的 count() 有可能回傳大於 1 嗎?set 呢?
//         → multiset 可以是任意正整數;set 只可能是 0 或 1
//           (C++20 起兩者都可改用 contains() 表達「存不存在」更清楚)。
//
// 🔥 Q2. multiset 判斷兩個元素「相同」時,用的是 operator== 還是 operator<?
//     答：用 **operator<**(或你提供的 Compare),而且判斷的是**等價**而非相等:
//         a 與 b 等價 ⟺ !(a < b) && !(b < a)。multiset 從來不呼叫 operator==。
//         這代表:若比較器只比較物件的部分欄位(例如只比 priority),
//         兩個並不相等的物件仍會被視為等價,count 和 equal_range 都會把它們算在一起。
//     追問：這在 set 上會造成什麼後果?
//         → 更嚴重:第二個等價元素會被靜默丟棄,你會莫名其妙掉資料。
//           所以設計比較器時必須先想清楚要的是「完整識別」還是「只依某鍵排序」。
//
// 🔥 Q3. 為什麼 multiset::count() 不是 O(1)?
//     答：因為等價元素在樹中形成一段連續區間。count 要先用 lower_bound 找到
//         區間起點、upper_bound 找到終點(各 O(log n)),再**實際走過**這段區間
//         數個數,所以是 O(log n + k),k 是等價元素個數。
//         若只想知道存不存在,用 find() != end() 即可,那是純 O(log n)。
//     追問：那要一次取出所有等價元素,最有效率的寫法是什麼?
//         → 直接用 equal_range(v) 拿到 {first, last} 區間走訪一次即可;
//           不要先 count 再走一遍,那等於白走兩趟。
//
// ⚠️ 陷阱. 有三個 30,我想刪掉「一個」30,寫 ms.erase(30) 對嗎?
//     答：不對。erase(value) 是依**值**刪除,會把**全部三個** 30 都刪光,
//         並回傳被刪除的個數(3)。只刪一個必須先取得 iterator:
//             auto it = ms.find(30);
//             if (it != ms.end()) ms.erase(it);   // 只刪這一個
//     為什麼會錯：在 set 上 erase(value) 剛好只會刪一個(因為本來就只有一個),
//         於是這個習慣被原封不動帶到 multiset,語意卻已經完全不同。
//         從 set 改成 multiset 時,所有 erase(value) 呼叫都必須重新檢視。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 480. Sliding Window Median
//   題目：給定陣列 nums 與視窗大小 k，回傳每個滑動視窗的中位數。
//   為什麼用到本主題：這題是 multiset 的代表作 —— 它同時要求四件事，
//     而**只有有序且允許重複的關聯容器**能一次滿足：
//     1. 視窗內的值**會重複** → 不能用 set（重複值會被吞掉，中位數就錯了）
//     2. 必須隨時有序才能取中位數 → 不能用 unordered_multiset
//     3. 視窗滑出時要精準刪掉「那一個」舊值
//        → window.erase(window.lower_bound(old))：先取得指向某一個等價元素的
//          iterator 再刪。若寫成 window.erase(old) 會把視窗內**所有**等於
//          old 的值全部刪光，視窗大小立刻崩壞 —— 這正是本課 erase 兩種語意的
//          真實後果。
//     4. 中位數位置要能隨插入/刪除便宜地維護
//        → 靠 iterator 穩定性：插入與刪除不會使 mid 以外的 iterator 失效，
//          所以我們可以只用 ++/-- 微調 mid，而不必每次重新 std::next。
//   實作要點：mid 永遠指向「第 k/2 個元素」。插入比 mid 小的值會把 mid 往後推
//     一格（故 --mid 抵銷）；移除比 mid 小或等於 mid 的值會把 mid 往前拉一格
//     （故 ++mid 抵銷）。順序很重要：必須在真正 erase 之前調整完 mid。
//   複雜度：時間 O(n log k)，空間 O(k)。
//   注意：偶數視窗取平均前先轉 double，避免兩個大 int 相加溢位。
// -----------------------------------------------------------------------------
std::vector<double> medianSlidingWindow(const std::vector<int>& nums, int k) {
    if (k <= 0 || nums.size() < static_cast<size_t>(k)) return {};

    std::multiset<int> window(nums.begin(), nums.begin() + k);
    auto mid = std::next(window.begin(), k / 2);      // 指向第 k/2 個（0-based）
    std::vector<double> result;

    for (size_t i = static_cast<size_t>(k); ; ++i) {
        // 取當前視窗中位數：奇數取 mid，偶數取 mid 與前一個的平均
        result.push_back(k % 2 == 1
            ? static_cast<double>(*mid)
            : (static_cast<double>(*mid) + static_cast<double>(*std::prev(mid))) / 2.0);

        if (i == nums.size()) break;                  // 已產生全部視窗

        int incoming = nums[i];
        int outgoing = nums[i - static_cast<size_t>(k)];

        window.insert(incoming);
        if (incoming < *mid) --mid;                   // 新值落在 mid 左邊 → mid 被推後
        if (outgoing <= *mid) ++mid;                  // 舊值在 mid 左邊或就是 mid → mid 會被拉前
        window.erase(window.lower_bound(outgoing));   // 只刪一個，不是全部
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】服務延遲的滑動視窗百分位監控（p50 / p95）
//   情境：SRE 監控一個 API 的回應時間，需要隨時回報「最近 N 次請求」的
//     中位數(p50)與 p95 延遲。延遲值大量重複（很多請求都是 12ms），
//     而且視窗滑動時要精準移除最舊的那一筆。
//   為什麼用到本主題：這個需求同時要求三件事，只有 multiset 能一次滿足 ——
//     1. 允許重複值（延遲重複是常態）→ 不能用 set
//     2. 隨時保持有序，才能取任意百分位 → 不能用 unordered_multiset
//     3. 能刪除「特定的那一筆」舊值 → find() + erase(iterator)
//     若用 vector 每次重新排序是 O(n log n)；multiset 的插入與刪除都是 O(log n)。
//   誠實的限制：multiset 的迭代器是雙向的，std::next(it, k) 是 O(k) 走訪，
//     所以取百分位本身是 O(n)。視窗很大又要極高頻取值時，正式做法會改用
//     「兩個 heap」或專門的順序統計樹；本例的視窗小，這樣寫最清楚。
// -----------------------------------------------------------------------------
class LatencyWindow {
public:
    explicit LatencyWindow(size_t windowSize) : cap_(windowSize) {}

    void record(int latencyMs) {
        samples_.insert(latencyMs);
        order_.push_back(latencyMs);
        if (order_.size() > cap_) {
            int oldest = order_.front();
            order_.erase(order_.begin());
            auto it = samples_.find(oldest);        // 只刪一筆，不是全部
            if (it != samples_.end()) samples_.erase(it);
        }
    }

    // percentile(50) → 中位數；percentile(95) → p95
    int percentile(int p) const {
        if (samples_.empty()) return -1;
        size_t idx = (samples_.size() * static_cast<size_t>(p)) / 100;
        if (idx >= samples_.size()) idx = samples_.size() - 1;
        return *std::next(samples_.begin(), static_cast<std::ptrdiff_t>(idx));
    }

    int  min()  const { return samples_.empty() ? -1 : *samples_.begin(); }
    int  max()  const { return samples_.empty() ? -1 : *samples_.rbegin(); }
    size_t size() const { return samples_.size(); }

private:
    size_t                 cap_;
    std::multiset<int>     samples_;   // 有序 + 允許重複
    std::vector<int>       order_;     // 記錄進入順序，用來知道該淘汰誰
};

int main() {
    // ── 原始課堂示範：multiset 基本操作 ───────────────────────────────────
    std::cout << "=== std::multiset ===" << std::endl;

    // 允許重複元素
    std::multiset<int> ms;

    ms.insert(30);
    ms.insert(10);
    ms.insert(30);  // 重複，會被保留
    ms.insert(20);
    ms.insert(30);  // 第三個 30

    std::cout << "元素: ";
    for (int n : ms) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "30 的數量: " << ms.count(30) << std::endl;

    // equal_range：取得所有等於某值的元素範圍
    // 這裡會回傳一個 pair，first 是第一個 30 的位置，second 是最後一個 30 的下一個位置
    auto range = ms.equal_range(30);
    std::cout << "所有 30：";
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // ── erase 的兩種語意（最常見的意外）──────────────────────────────────
    std::cout << "\n=== erase(value) vs erase(iterator) ===" << std::endl;
    {
        std::multiset<int> a = {30, 10, 30, 20, 30};
        auto it = a.find(30);                 // 找到「某一個」30
        a.erase(it);                          // 只刪這一個
        std::cout << "  erase(iterator) 後: ";
        for (int n : a) std::cout << n << " ";
        std::cout << " → 還剩 " << a.count(30) << " 個 30" << std::endl;

        std::multiset<int> b = {30, 10, 30, 20, 30};
        size_t removed = b.erase(30);         // 依值刪除 → 全部刪光
        std::cout << "  erase(30) 後:        ";
        for (int n : b) std::cout << n << " ";
        std::cout << " → 一次刪掉 " << removed << " 個，剩 "
                  << b.count(30) << " 個 30" << std::endl;
    }

    // ── 等價 vs 相等：比較器只看部分欄位時 ────────────────────────────────
    std::cout << "\n=== 等價(equivalence) 不等於 相等(equality) ===" << std::endl;
    {
        struct Task { int priority; std::string name; };
        // 比較器只看 priority → 只要 priority 相同就「等價」
        auto byPriority = [](const Task& a, const Task& b) {
            return a.priority < b.priority;
        };
        std::multiset<Task, decltype(byPriority)> tasks(byPriority);

        tasks.insert({3, "備份資料庫"});
        tasks.insert({1, "寄送報表"});
        tasks.insert({3, "清理暫存檔"});     // 與「備份資料庫」等價，但並不相等
        tasks.insert({2, "重建索引"});

        std::cout << "  依優先權排序（等價者維持插入順序，C++11 保證）:" << std::endl;
        for (const auto& t : tasks) {
            std::cout << "    p" << t.priority << " " << t.name << std::endl;
        }

        // 查詢所有 priority == 3 的工作。注意 name 隨便填都找得到，
        // 因為比較器根本不看 name。
        auto r = tasks.equal_range(Task{3, "（這個欄位不參與比較）"});
        std::cout << "  priority == 3 的工作有 "
                  << std::distance(r.first, r.second) << " 件:" << std::endl;
        for (auto it = r.first; it != r.second; ++it) {
            std::cout << "    " << it->name << std::endl;
        }
        std::cout << "  → 兩個 Task 並不相等，卻被視為等價（比較器只看 priority）"
                  << std::endl;
    }

    // ── lower_bound / upper_bound：有序容器獨有的範圍查詢 ─────────────────
    std::cout << "\n=== 範圍查詢（unordered 容器做不到）===" << std::endl;
    {
        std::multiset<int> scores = {55, 60, 60, 72, 72, 72, 88, 95};
        auto lo = scores.lower_bound(60);
        auto hi = scores.upper_bound(72);
        std::cout << "  分數落在 [60, 72] 區間的有: ";
        for (auto it = lo; it != hi; ++it) std::cout << *it << " ";
        std::cout << "（共 " << std::distance(lo, hi) << " 筆）" << std::endl;
        std::cout << "  最小: " << *scores.begin()
                  << "，最大: " << *scores.rbegin() << std::endl;
    }

    // ── LeetCode 480 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 480. Sliding Window Median ===" << std::endl;
    {
        auto show = [](const std::vector<int>& nums, int k, const char* expect) {
            std::cout << "  nums={";
            for (size_t i = 0; i < nums.size(); ++i)
                std::cout << nums[i] << (i + 1 < nums.size() ? "," : "");
            std::cout << "}, k=" << k << " → ";
            for (double d : medianSlidingWindow(nums, k)) std::cout << d << " ";
            std::cout << " (期望 " << expect << ")" << std::endl;
        };
        show({1, 3, -1, -3, 5, 3, 6, 7}, 3, "1 -1 -1 3 5 6");
        show({1, 2, 3, 4, 2, 3, 1, 4, 2}, 3, "2 3 3 3 2 3 2");
        show({1, 4, 2, 3}, 4, "2.5");   // 偶數視窗 → 取中間兩數平均
    }

    // ── 日常實務：延遲百分位監控 ──────────────────────────────────────────
    std::cout << "\n=== 日常實務: API 延遲滑動視窗百分位 ===" << std::endl;
    {
        LatencyWindow win(8);                 // 只看最近 8 次請求
        // 一段真實感的延遲序列（單位 ms），注意大量重複值
        std::vector<int> latencies = {12, 15, 12, 11, 350, 13, 12, 14,
                                      12, 11, 12, 900, 13, 12};

        for (size_t i = 0; i < latencies.size(); ++i) {
            win.record(latencies[i]);
            if (i == 7 || i == latencies.size() - 1) {
                std::cout << "  第 " << (i + 1) << " 次請求後（視窗 "
                          << win.size() << " 筆）:" << std::endl;
                std::cout << "    min=" << win.min()
                          << "ms  p50=" << win.percentile(50)
                          << "ms  p95=" << win.percentile(95)
                          << "ms  max=" << win.max() << "ms" << std::endl;
            }
        }
        std::cout << "  → 900ms 的尖峰滑出視窗後，p95 立刻回落，"
                     "這正是滑動視窗監控要的效果" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類7.cpp" -o multiset_demo

// === 預期輸出 ===
// === std::multiset ===
// 元素: 10 20 30 30 30
// 30 的數量: 3
// 所有 30：30 30 30
//
// === erase(value) vs erase(iterator) ===
//   erase(iterator) 後: 10 20 30 30  → 還剩 2 個 30
//   erase(30) 後:        10 20  → 一次刪掉 3 個，剩 0 個 30
//
// === 等價(equivalence) 不等於 相等(equality) ===
//   依優先權排序（等價者維持插入順序，C++11 保證）:
//     p1 寄送報表
//     p2 重建索引
//     p3 備份資料庫
//     p3 清理暫存檔
//   priority == 3 的工作有 2 件:
//     備份資料庫
//     清理暫存檔
//   → 兩個 Task 並不相等，卻被視為等價（比較器只看 priority）
//
// === 範圍查詢（unordered 容器做不到）===
//   分數落在 [60, 72] 區間的有: 60 60 72 72 72 （共 5 筆）
//   最小: 55，最大: 95
//
// === LeetCode 480. Sliding Window Median ===
//   nums={1,3,-1,-3,5,3,6,7}, k=3 → 1 -1 -1 3 5 6  (期望 1 -1 -1 3 5 6)
//   nums={1,2,3,4,2,3,1,4,2}, k=3 → 2 3 3 3 2 3 2  (期望 2 3 3 3 2 3 2)
//   nums={1,4,2,3}, k=4 → 2.5  (期望 2.5)
//
// === 日常實務: API 延遲滑動視窗百分位 ===
//   第 8 次請求後（視窗 8 筆）:
//     min=11ms  p50=13ms  p95=350ms  max=350ms
//   第 14 次請求後（視窗 8 筆）:
//     min=11ms  p50=12ms  p95=900ms  max=900ms
//   → 900ms 的尖峰滑出視窗後，p95 立刻回落，這正是滑動視窗監控要的效果
