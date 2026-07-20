// =============================================================================
//  第六課 10 — std::unordered_set：以雜湊表換取平均 O(1) 的無序集合
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key,
//            class Hash     = std::hash<Key>,
//            class KeyEqual = std::equal_to<Key>,
//            class Allocator = std::allocator<Key>>
//   class unordered_set;                              // <unordered_set>,C++11 起
//
//   常用介面：
//     std::pair<iterator,bool> insert(const Key& k);  // 平均 O(1),最壞 O(n)
//     iterator find(const Key& k);                    // 平均 O(1),最壞 O(n)
//     size_type count(const Key& k) const;            // 只可能是 0 或 1
//     size_type erase(const Key& k);                  // 平均 O(1)
//     bool      contains(const Key& k) const;         // C++20
//
//   雜湊表狀態查詢：
//     size_type bucket_count() const noexcept;        // 目前的桶數
//     float     load_factor() const noexcept;         // size() / bucket_count()
//     float     max_load_factor() const noexcept;     // 預設 1.0,超過就 rehash
//     void      reserve(size_type n);                 // 預先備妥容納 n 個元素的桶
//     void      rehash(size_type n);                  // 直接指定至少 n 個桶
//
//   **沒有** lower_bound / upper_bound / 有序走訪 —— 見【3】。
//
//   複雜度：查找、插入、刪除平均 O(1),**最壞 O(n)**(全部碰撞時退化成一條鏈)。
//   標頭檔：#include <unordered_set>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要 unordered_set:用「排序能力」換「查找速度」】
// std::set 底層是紅黑樹,每次查找要從樹根往下比較約 log2(n) 次:
// 100 萬筆資料要比對約 20 次,每一次都是一個可能 cache miss 的指標跳躍。
//
// unordered_set 的思路完全不同:與其「比較著找」,不如「算出來」。
// 把 key 丟進雜湊函式 h(key),得到一個整數,再對桶數取模,就直接算出它該待在
// 哪一個桶(bucket)裡 —— 不需要任何比較就定位完成。這是常數時間的操作,
// 與元素個數無關。
//
//     位置 = h(key) % bucket_count()
//
// 代價是:桶的順序是雜湊值決定的,和 key 的大小毫無關係。所以 unordered_set
// **失去了排序**。這正是命名的由來 —— unordered 不是「隨機」,而是
// 「不保證任何順序」。
//
// 一句話總結這個取捨:
//     std::set           → 有序、穩定 O(log n)、比較次數多
//     std::unordered_set → 無序、平均 O(1)、最壞 O(n)
//
// 【2. 碰撞與 separate chaining:平均 O(1) 的「平均」二字很重要】
// 不同的 key 算出同一個桶,叫做**碰撞(collision)**。因為桶數有限而 key 的
// 可能值無限,碰撞是必然會發生的,雜湊表的設計重點不是「避免碰撞」,而是
// 「碰撞後怎麼辦」。libstdc++ 採用的策略是 **separate chaining(分離鏈結)**:
// 同一個桶裡的元素用單向鏈結串起來。
//
//     bucket[0] → nullptr
//     bucket[1] → [10] → [23] → [36] → nullptr     ← 三個 key 碰撞在同一桶
//     bucket[2] → [7]  → nullptr
//
// 查找流程於是變成兩步:
//     (1) 算 h(key) % bucket_count() 找到桶            → O(1)
//     (2) 沿著該桶的鏈走,用 KeyEqual 逐一比對         → O(該桶長度)
//
// 若元素平均分散,每桶長度接近 load_factor(預設上限 1.0),第 (2) 步就是常數
// 時間 → 整體平均 O(1)。但若**所有 key 都碰撞到同一個桶**,那條鏈長 n,
// 查找就退化成 O(n) —— 和一個沒排序的鏈結串列一樣慢。
//
// 這不只是理論。**hash flooding** 是真實存在的阻斷服務攻擊:攻擊者刻意送出
// 一大批雜湊值相同的 key(例如 HTTP 參數名、JSON 欄位名),讓伺服器的雜湊表
// 全部擠進同一個桶,把原本 O(1) 的查表變成 O(n),CPU 瞬間被打滿。
// 這也是為什麼 std::unordered_set 在「最壞情況必須有保證」的場合(即時系統、
// 直接吃外部輸入當 key 的服務)不一定是正確選擇 —— std::set 雖然平均較慢,
// 但它的 O(log n) 是**最壞情況保證**,不會被輸入資料誘導退化。
//
// 【3. 為什麼沒有 lower_bound:它在無序容器裡沒有意義】
// std::set 有 lower_bound/upper_bound,可以做「找出所有 >= 50 的元素」這種
// 範圍查詢,因為樹的中序走訪本來就是排序的。
//
// unordered_set 的元素位置由雜湊值決定,相鄰的桶之間毫無大小關係 —— 10 和 11
// 可能一個在桶 3、一個在桶 47。要找「所有 >= 50 的元素」只能把整個表掃一遍
// O(n),那和用 vector 沒有差別。既然無法比線性掃描更快,標準就乾脆不提供
// lower_bound —— 一個看似能用、實則暗藏 O(n) 的介面比沒有更危險。
//
// 選容器時的正確順序因此是:**先看能力,再看速度**。
//     需要有序走訪?需要範圍查詢?需要最壞情況保證? → 只能用 set
//     以上皆否,只是要判斷「在不在」?             → unordered_set 比較快
//
// 【4. load_factor 與 rehash:雜湊表如何自動長大】
// 桶太少會讓每桶的鏈變長,查找變慢;桶太多則浪費記憶體。雜湊表用
// **load factor(負載因子)** 來管理這個平衡:
//
//     load_factor() = size() / bucket_count()
//
// 容器有一個 max_load_factor(),**預設是 1.0**。每次插入後若 load_factor 會
// 超過這個上限,容器就會 **rehash**:配置一組更大的桶陣列,把**每一個現有
// 元素重新計算桶位置**再放進去。這是一次 O(n) 的操作。
//
// 所以「平均 O(1)」其實是**攤提(amortized)**的結果:絕大多數插入是 O(1),
// 偶爾一次是 O(n),平均下來仍是 O(1) —— 和 vector 擴容的道理完全一樣。
//
// 如果你事先知道大概會放幾筆,呼叫 reserve(n) 可以一次配置到位,完全避開
// 中途的多次 rehash。這是雜湊容器最容易拿到、也最常被忽略的效能改善。
//
// 【5. rehash 時的失效規則:和 vector 恰好相反(本主題最反直覺的一點)】
// vector 擴容時,元素本身被搬到新的記憶體位置,所以 iterator、reference、
// pointer **全部失效**。
//
// unordered_set 的 rehash 則完全不同:
//
//     * **所有 iterator 失效**   ← 桶陣列重建,走訪順序整個變了
//     * **references / pointers 仍然有效**  ← 元素節點本身沒有搬家!
//
// 為什麼?因為 separate chaining 下,每個元素都住在自己獨立配置的節點裡,
// 桶陣列存的只是「指向節點的指標」。rehash 重建的是**桶陣列**與節點之間的
// 鏈結關係,節點本身連動都沒動過 —— 它在 heap 上的位址從頭到尾不變。
//
// 這條規則在實務上很有用:你可以安全地持有指向某個元素的指標,即使之後又
// 插入了大量資料觸發 rehash,那個指標依然指向同一個元素。這是 vector 絕對
// 做不到的事。本檔的 main() 用實際位址印出來驗證了這一點。
//
// 【6. std::hash 支援哪些型別:自訂型別要自己動手】
// 標準為所有內建型別(int、double、指標…)、std::string、std::string_view
// 等提供了 std::hash 特化。但**沒有**為 std::pair、std::tuple、std::vector
// 或你自己的 struct 提供 —— 所以下面這行是編譯錯誤:
//
//     std::unordered_set<std::pair<int,int>> s;   // ✗ 找不到 std::hash<pair>
//
// (對照組:std::set<std::pair<int,int>> 完全沒問題,因為 std::pair 有 operator<。)
//
// 要讓自訂型別能當 key,必須提供**兩樣東西**:
//     (1) 雜湊函式  —— 算出桶位置
//     (2) 相等比較  —— 碰撞後在鏈上逐一比對時要用(預設是 operator==)
// 兩者缺一不可,而且必須一致:**a == b 就必須 h(a) == h(b)**。若違反這條,
// 兩個相等的 key 會被算到不同的桶,容器裡就會同時存在兩份「相等」的元素,
// 集合的唯一性保證從此瓦解。本檔的實務範例示範了正確做法。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:容器本體很小,資料全在節點
//     本機實測 sizeof(std::unordered_set<int>) == 56(**實作定義**)。
//     libstdc++ 的 _Hashtable 本體大致含:
//         [桶陣列指標][桶數][鏈結串列頭節點][元素個數][max_load_factor]
//         [單桶最佳化用的內建緩衝]
//     元素本身完全不在這 56 bytes 裡 —— 每個元素是一個獨立 heap 節點:
//         [next 指標][(可選)快取的雜湊值][ Key 本體 ]
//     所以每個元素至少多付一個指標(8 bytes)的開銷,和 std::set 的三指標
//     節點相比省一些,但仍遠不如 vector 緊湊(實作定義)。
//
// (B) libstdc++ 用質數桶策略,MSVC 用 2 的冪 —— 兩者都合法
//     本機實測(GCC 15.2.0 / libstdc++,**實作定義**):
//         空容器             bucket_count = 1     (還沒配置桶陣列的特例)
//         size 1   → 13 桶
//         size 14  → 29 桶
//         size 30  → 59 桶
//         size 60  → 127 桶
//         size 128 → 257 桶
//     13, 29, 59, 127, 257 全是**質數**。用質數當模數可以讓 h(key) % n 的
//     結果分佈更均勻,對品質較差的雜湊函式特別重要。
//     MSVC 的 STL 則採 2 的冪,因為 % 2^k 可以最佳化成位元 AND(快很多),
//     但它會另外對雜湊值再做一次混洗來補償分佈。
//     **兩種策略都符合標準**,所以 bucket_count() 的具體數值**絕對不可以**
//     寫進測試斷言或跨平台的邏輯判斷裡。
//
// (C) 走訪順序未由標準規定 —— 不只是「和插入順序不同」
//     很多人以為 unordered 的意思是「順序 = 雜湊順序,固定但看不懂」。
//     更準確的說法是:標準**完全沒有規定**走訪順序。它取決於雜湊函式、
//     桶數、插入歷史(是否發生過 rehash),因此:
//         * 換一個標準庫實作 → 順序不同
//         * 同一支程式插入順序不同 → 順序可能不同
//         * 未來版本的 libstdc++ 改了雜湊策略 → 順序不同
//     所以任何「靠走訪順序」的邏輯或單元測試都是脆弱的。需要穩定輸出時,
//     正確做法是**複製到 vector 再排序**(本檔新增的示範都採用這個做法)。
//
// (D) 為什麼 count() 只可能回傳 0 或 1
//     unordered_set 保證元素唯一,所以 count() 的結果只有兩種。真正需要
//     計數的是 unordered_multiset。C++20 之後,判斷存在性請優先用
//     contains(),語意最直接。
//
// 【注意事項 Pay Attention】
//  1. **走訪順序未由標準規定**,不同實作、不同插入歷史、甚至不同版本都可能
//     不同。絕對不要依賴它,也不要把它寫進測試斷言。
//  2. 平均 O(1) 是**平均**。全部碰撞時退化為 O(n),而且這可以被惡意輸入誘導
//     (hash flooding)。最壞情況必須有保證時,請用 std::set。
//  3. rehash 後**所有 iterator 失效**,但 **references / pointers 仍有效**
//     (節點不搬家)。這與 vector 恰好相反,別套用 vector 的直覺。
//  4. bucket_count() 的具體數值是**實作定義**。libstdc++ 用質數(本機實測
//     1 → 13 → 29 → 59 → 127 → 257),MSVC 用 2 的冪。不可跨平台假設。
//  5. std::hash **沒有**為 std::pair、std::tuple、std::vector 或自訂 struct
//     提供特化。自訂 key 必須自備 hash functor **與** operator==(或 KeyEqual),
//     且必須滿足「a == b ⇒ h(a) == h(b)」。違反這條會讓集合出現重複元素。
//  6. 別用 unordered_set 找「最小/最大值」或做範圍查詢 —— 那是 O(n) 全掃。
//     需要這些能力就該用 std::set,這是能力問題,不是效能問題。
//  7. 已知資料量時請呼叫 reserve(n),可避開中途多次 O(n) 的 rehash。
//     這是雜湊容器最廉價的效能改善。
//  8. max_load_factor 預設為 1.0。調低可減少碰撞但更耗記憶體、rehash 更頻繁;
//     沒有實測數據支持前不要亂動。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::unordered_set
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::set 和 std::unordered_set 該怎麼選?
//     答：**先看能力,再看速度**。set 是紅黑樹,有序、支援 lower_bound /
//         範圍查詢 / 取最小最大值,且 O(log n) 是**最壞情況保證**。
//         unordered_set 是雜湊表,平均 O(1) 但最壞 O(n),且完全沒有順序可言。
//         如果你需要有序走訪或範圍查詢,unordered_set 根本做不到(不是慢,是
//         沒有這個能力);如果只是要判斷「在不在」,unordered_set 通常明顯較快。
//     追問：什麼情況下你會刻意選比較慢的 std::set?
//         → (1) 需要有序輸出或範圍查詢;(2) 需要**最壞情況**的時間保證,
//           例如即時系統或直接拿外部輸入當 key 的服務 —— 雜湊表可能被
//           hash flooding 攻擊誘導退化成 O(n),紅黑樹不會;
//           (3) key 型別沒有現成的 std::hash,但有 operator<。
//
// 🔥 Q2. unordered_set 的平均複雜度是 O(1),那最壞情況呢?為什麼?
//     答：最壞 O(n)。libstdc++ 用 separate chaining:同一個桶的元素串成鏈。
//         查找是「算雜湊定位到桶」O(1) 加上「沿鏈逐一用 == 比對」O(鏈長)。
//         元素分佈均勻時鏈很短(load_factor 預設上限 1.0),所以平均 O(1);
//         但若所有 key 都雜湊到同一個桶,鏈長 n,查找就退化成線性掃描。
//     追問：這在實務上會發生嗎?
//         → 會,而且是已知的攻擊手法:hash flooding。攻擊者刻意送出一批
//           雜湊值相同的 key(如 HTTP 參數名),讓伺服器的查表從 O(1) 變 O(n),
//           用很小的流量把 CPU 打滿。防禦手段是加隨機種子的雜湊,
//           或在這類路徑改用有最壞保證的 std::set。
//
// ⚠️ 陷阱. unordered_set 插入大量元素觸發 rehash 後,先前取得的
//         iterator 和 reference 還能用嗎?
//     答：**iterator 全部失效,但 reference 和 pointer 仍然有效**。
//         因為 separate chaining 下每個元素住在自己獨立配置的節點裡,
//         桶陣列存的只是指標。rehash 重建的是桶陣列與鏈結關係,
//         **節點本身沒有搬家**,位址不變,所以指向元素的 reference/pointer
//         依然有效;但走訪順序整個重來,iterator 因此失效。
//     為什麼會錯：多數人把 vector 的直覺直接套過來 —— vector 擴容會搬移
//         元素,所以 iterator、reference、pointer 一起失效。
//         unordered 容器的答案**恰好相反**,而且方向是反的這點特別容易記錯:
//         關鍵在於「元素是否被搬動」,不是「容器是否重新配置」。
//
// ⚠️ 陷阱. std::unordered_set<std::pair<int,int>> 可以直接用嗎?
//     答：不行,**編譯錯誤**。標準沒有為 std::pair 提供 std::hash 特化
//         (只有內建型別、std::string 這類才有)。必須自己寫一個 hash functor
//         傳進第二個模板參數。反過來 std::set<std::pair<int,int>> 完全沒問題,
//         因為 std::pair 有現成的 operator<。
//     為什麼會錯：大家習慣「STL 型別之間應該都能互相組合」,而 set 又確實
//         可以直接吃 pair,於是預設 unordered_set 也行。實際上兩者的需求不同:
//         有序容器只要能**比較大小**,雜湊容器則需要**雜湊函式 + 相等比較**,
//         而後者標準無法替使用者的組合型別給出通用且合理的預設實作。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 217. Contains Duplicate
//   題目：判斷陣列中是否存在重複元素。
//   為什麼用到本主題：這是 unordered_set 最教科書式的用途 —— 只需要
//     「這個值我看過沒有」這一種能力,不需要順序、不需要範圍查詢。
//     邊掃描邊插入,insert 回傳的 pair<iterator,bool> 的 bool 直接告訴你
//     「這次是不是新元素」,一次走訪就能得到答案,連第二次查找都省了。
//   複雜度：時間平均 O(n)(最壞 O(n^2),全部碰撞時),空間 O(n)。
//     對照組:先 sort 再比鄰居是 O(n log n) 時間、O(1) 額外空間 ——
//     這是典型的以空間換時間。
// -----------------------------------------------------------------------------
bool containsDuplicate(const std::vector<int>& nums) {
    std::unordered_set<int> seen;
    seen.reserve(nums.size());          // 預先配置,避開過程中的多次 rehash
    for (int n : nums) {
        // insert 回傳 pair<iterator, bool>;bool 為 false 代表元素已存在
        if (!seen.insert(n).second) return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 128. Longest Consecutive Sequence
//   題目：給一個未排序的整數陣列,找出最長「連續數字」序列的長度,要求 O(n)。
//   為什麼用到本主題：題目明確禁止 O(n log n),所以不能排序。關鍵技巧是
//     把所有數字丟進 unordered_set,取得 O(1) 的「某個數字在不在」查詢能力;
//     然後只從「序列的起點」(即 n-1 不存在的那個 n)開始往上數。
//     這個「只從起點起算」的條件,保證每個元素最多被走訪兩次 → 整體 O(n)。
//     若少了這個條件,退化成對每個元素都往上數,就變成 O(n^2)。
//   複雜度：時間平均 O(n),空間 O(n)。
// -----------------------------------------------------------------------------
int longestConsecutive(const std::vector<int>& nums) {
    std::unordered_set<int> pool(nums.begin(), nums.end());
    int best = 0;
    for (int n : pool) {
        // 只有當 n-1 不存在時,n 才是某段連續序列的起點 —— 這是 O(n) 的關鍵
        if (pool.count(n - 1) != 0) continue;

        int cur = n;
        int len = 1;
        while (pool.count(cur + 1) != 0) { ++cur; ++len; }
        best = std::max(best, len);
    }
    return best;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】存取日誌(access log)的不重複訪客(unique visitor)統計
//   情境：每天的 access log 動輒數百萬行,要算出「今天有幾個不重複 IP」、
//     以及「哪些 IP 出現在封鎖名單裡」。這是資料處理最常見的去重需求。
//   為什麼用到本主題：
//     (1) 去重只需要「看過沒有」,正是 unordered_set 的主場,且不在乎順序。
//     (2) 封鎖名單比對是純粹的成員查詢,平均 O(1);若用 vector 逐一比對是
//         O(n*m),資料量一大就完全不能用。
//     (3) 示範 reserve() 的實際用法:日誌行數大致可估,先配置好桶可省下
//         中途所有 rehash。
//   注意輸出穩定性：unordered_set 走訪順序未由標準規定,所以要輸出給人看的
//     結果,一律先複製到 vector 再 sort —— 這樣教學輸出才可重現。
// -----------------------------------------------------------------------------
struct AccessLogStats {
    std::size_t              totalRequests = 0;
    std::size_t              uniqueVisitors = 0;
    std::vector<std::string> blockedHits;      // 已排序,確保輸出穩定
};

AccessLogStats analyzeAccessLog(const std::vector<std::string>& logLines,
                                const std::unordered_set<std::string>& blocklist) {
    AccessLogStats stats;
    std::unordered_set<std::string> uniqueIps;
    uniqueIps.reserve(logLines.size());        // 預估上界,避開多次 rehash

    std::unordered_set<std::string> blockedSeen;

    for (const std::string& line : logLines) {
        // 典型 combined log 格式:IP 在第一個空白之前
        std::size_t sp = line.find(' ');
        if (sp == std::string::npos) continue;
        std::string ip = line.substr(0, sp);

        ++stats.totalRequests;
        uniqueIps.insert(ip);

        // 封鎖名單比對:平均 O(1)
        if (blocklist.count(ip) != 0) blockedSeen.insert(ip);
    }

    stats.uniqueVisitors = uniqueIps.size();
    stats.blockedHits.assign(blockedSeen.begin(), blockedSeen.end());
    std::sort(stats.blockedHits.begin(), stats.blockedHits.end());   // 穩定輸出
    return stats;
}

// -----------------------------------------------------------------------------
// 【自訂型別當 key】示範「hash functor + operator==」缺一不可
//   情境：把 (x, y) 座標當成 key 存進 unordered_set(遊戲地圖已探索格子、
//     棋盤已落子位置)。std::hash **沒有** std::pair 的特化,所以必須自備。
// -----------------------------------------------------------------------------
struct Point {
    int x;
    int y;
};

// (1) 相等比較 —— 碰撞後沿鏈比對時使用
bool operator==(const Point& a, const Point& b) {
    return a.x == b.x && a.y == b.y;
}

// (2) 雜湊函式 —— 必須滿足「a == b ⇒ h(a) == h(b)」
struct PointHash {
    std::size_t operator()(const Point& p) const noexcept {
        // 把兩個 int 混成一個 size_t。0x9e3779b9 是黃金比例常數,
        // 這是 boost::hash_combine 的經典寫法,目的是讓位元充分擴散。
        std::size_t h1 = std::hash<int>{}(p.x);
        std::size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
    }
};

int main() {
    // ── 原始課堂示範:unordered_set 的基本操作 ────────────────────────────
    std::cout << "=== std::unordered_set ===" << std::endl;

    std::unordered_set<int> us;

    us.insert(30);
    us.insert(10);
    us.insert(50);
    us.insert(20);
    us.insert(40);

    // 順序不固定（取決於雜湊）
    std::cout << "元素（順序不固定）: ";
    for (int n : us) std::cout << n << " ";
    std::cout << std::endl;

    // 查找：平均 O(1)
    if (us.find(30) != us.end()) {
        std::cout << "找到 30" << std::endl;
    }

    // 雜湊表資訊
    // bucket_count：桶的數量
    // load_factor：負載因子 = 元素數量 / 桶的數量
    // 負載因子越高，碰撞越多，性能可能下降
    // 注意：unordered_set 的元素是無序的，因為它們是根據雜湊值分佈在不同的桶中
    // bucket_count 和 load_factor 是 unordered_set 的內部資訊，可以用來分析性能
    // 注意：不同的實現可能有不同的 bucket_count 和 load_factor，這取決於元素的數量和雜湊函數的效率
    std::cout << "桶的數量: " << us.bucket_count() << std::endl;
    std::cout << "負載因子: " << us.load_factor() << std::endl;

    // ── 穩定輸出的正確做法:複製到 vector 再排序 ──────────────────────────
    std::cout << "\n=== 要穩定輸出就先排序 ===" << std::endl;
    std::vector<int> sorted(us.begin(), us.end());
    std::sort(sorted.begin(), sorted.end());
    std::cout << "排序後(可重現): ";
    for (int n : sorted) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "→ 上面那行「順序不固定」的輸出不可寫進測試斷言" << std::endl;

    // ── 觀察 1:每個元素落在哪個桶 ────────────────────────────────────────
    std::cout << "\n=== 元素與桶的對應(實作定義) ===" << std::endl;
    for (int n : sorted) {
        std::cout << "  元素 " << n << " → bucket " << us.bucket(n)
                  << " (該桶內有 " << us.bucket_size(us.bucket(n)) << " 個元素)"
                  << std::endl;
    }

    // ── 觀察 2:桶數的成長過程(libstdc++ 的質數策略) ─────────────────────
    std::cout << "\n=== bucket_count 的成長(libstdc++,實作定義) ===" << std::endl;
    {
        std::unordered_set<int> grow;
        std::cout << "  空容器            bucket_count = " << grow.bucket_count()
                  << ", max_load_factor = " << grow.max_load_factor() << std::endl;
        for (int i = 1; i <= 300; ++i) {
            std::size_t before = grow.bucket_count();
            grow.insert(i);
            if (grow.bucket_count() != before) {
                std::cout << "  size = " << grow.size()
                          << " 時 rehash → bucket_count = " << grow.bucket_count()
                          << std::endl;
            }
        }
        std::cout << "  → 13, 29, 59, 127, 257, 541 全是質數;MSVC 改用 2 的冪。"
                  << std::endl;
        std::cout << "  → 這些數值是實作定義,不可寫進跨平台的邏輯或測試。"
                  << std::endl;
    }

    // ── 觀察 3:rehash 後 iterator 失效,但 reference/pointer 仍有效 ───────
    std::cout << "\n=== rehash 的失效規則(與 vector 相反) ===" << std::endl;
    {
        std::unordered_set<int> s;
        s.insert(42);
        const int* addrBefore = &(*s.find(42));       // 指向元素本體的指標
        std::size_t bucketsBefore = s.bucket_count();

        // 插入大量元素,必然觸發多次 rehash
        for (int i = 0; i < 500; ++i) s.insert(i + 1000);

        const int* addrAfter = &(*s.find(42));
        std::cout << "  rehash 前 bucket_count = " << bucketsBefore
                  << ",rehash 後 = " << s.bucket_count() << std::endl;
        std::cout << "  元素 42 的位址在 rehash 前後相同嗎? "
                  << std::boolalpha << (addrBefore == addrAfter) << std::endl;
        std::cout << "  → 節點沒有搬家,所以 reference/pointer 仍然有效;"
                  << std::endl;
        std::cout << "    但所有 iterator 已失效(走訪順序整個重建)。" << std::endl;
        std::cout << "    vector 擴容則是三者全部失效 —— 方向剛好相反。" << std::endl;
    }

    // ── 觀察 4:reserve() 可以完全避開中途 rehash ─────────────────────────
    std::cout << "\n=== reserve() 的效果 ===" << std::endl;
    {
        std::unordered_set<int> noReserve;
        int rehashCount = 0;
        for (int i = 0; i < 1000; ++i) {
            std::size_t before = noReserve.bucket_count();
            noReserve.insert(i);
            if (noReserve.bucket_count() != before) ++rehashCount;
        }

        std::unordered_set<int> withReserve;
        withReserve.reserve(1000);
        int rehashCount2 = 0;
        for (int i = 0; i < 1000; ++i) {
            std::size_t before = withReserve.bucket_count();
            withReserve.insert(i);
            if (withReserve.bucket_count() != before) ++rehashCount2;
        }

        std::cout << "  插入 1000 筆,未 reserve → rehash " << rehashCount << " 次"
                  << std::endl;
        std::cout << "  插入 1000 筆,先 reserve → rehash " << rehashCount2 << " 次"
                  << std::endl;
        std::cout << "  → 每次 rehash 都是 O(n),已知資料量時 reserve 幾乎是免費的優化"
                  << std::endl;
    }

    // ── 觀察 5:記憶體佈局 ────────────────────────────────────────────────
    std::cout << "\n=== 記憶體佈局(實作定義) ===" << std::endl;
    std::cout << "sizeof(std::unordered_set<int>) = "
              << sizeof(std::unordered_set<int>) << std::endl;
    std::cout << "→ 容器本體只存桶陣列指標/桶數/元素數等骨架,"
              << "元素在各自的 heap 節點裡" << std::endl;

    // ── LeetCode 217 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 217. Contains Duplicate ===" << std::endl;
    std::vector<int> a = {1, 2, 3, 1};
    std::vector<int> b = {1, 2, 3, 4};
    std::cout << "containsDuplicate({1,2,3,1}) = "
              << containsDuplicate(a) << std::endl;
    std::cout << "containsDuplicate({1,2,3,4}) = "
              << containsDuplicate(b) << std::endl;

    // ── LeetCode 128 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 128. Longest Consecutive Sequence ===" << std::endl;
    std::vector<int> c = {100, 4, 200, 1, 3, 2};
    std::cout << "longestConsecutive({100,4,200,1,3,2}) = "
              << longestConsecutive(c) << "  (連續序列 1,2,3,4)" << std::endl;
    std::vector<int> d = {0, 3, 7, 2, 5, 8, 4, 6, 0, 1};
    std::cout << "longestConsecutive({0,3,7,2,5,8,4,6,0,1}) = "
              << longestConsecutive(d) << "  (連續序列 0~8)" << std::endl;

    // ── 日常實務:access log 不重複訪客統計 ───────────────────────────────
    std::cout << "\n=== 日常實務: access log 不重複訪客統計 ===" << std::endl;
    std::vector<std::string> logLines = {
        "203.0.113.7 - - [20/Jul/2026:09:15:01] \"GET /index.html\" 200 5120",
        "198.51.100.23 - - [20/Jul/2026:09:15:04] \"GET /api/v1/users\" 200 812",
        "203.0.113.7 - - [20/Jul/2026:09:15:09] \"GET /style.css\" 200 2048",
        "192.0.2.155 - - [20/Jul/2026:09:15:12] \"POST /login\" 401 96",
        "198.51.100.23 - - [20/Jul/2026:09:15:15] \"GET /api/v1/orders\" 200 4096",
        "192.0.2.155 - - [20/Jul/2026:09:15:18] \"POST /login\" 401 96",
        "203.0.113.7 - - [20/Jul/2026:09:15:22] \"GET /logo.png\" 200 15360",
        "198.51.100.99 - - [20/Jul/2026:09:15:30] \"GET /admin\" 403 128"
    };
    std::unordered_set<std::string> blocklist = {"192.0.2.155", "198.51.100.99"};

    AccessLogStats stats = analyzeAccessLog(logLines, blocklist);
    std::cout << "總請求數:     " << stats.totalRequests << std::endl;
    std::cout << "不重複訪客:   " << stats.uniqueVisitors << std::endl;
    std::cout << "命中封鎖名單: " << stats.blockedHits.size() << " 個 IP" << std::endl;
    for (const std::string& ip : stats.blockedHits)
        std::cout << "  - " << ip << std::endl;

    // ── 自訂型別當 key ────────────────────────────────────────────────────
    std::cout << "\n=== 自訂型別當 key(hash functor + operator==) ===" << std::endl;
    std::unordered_set<Point, PointHash> explored;
    explored.insert({0, 0});
    explored.insert({1, 2});
    explored.insert({3, 4});
    explored.insert({1, 2});      // 重複,不會被插入

    std::cout << "已探索格子數: " << explored.size()
              << " (插入 4 次,其中 {1,2} 重複)" << std::endl;
    std::cout << "{1,2} 探索過了嗎? "
              << (explored.count({1, 2}) != 0) << std::endl;
    std::cout << "{9,9} 探索過了嗎? "
              << (explored.count({9, 9}) != 0) << std::endl;
    std::cout << "→ std::hash 沒有 std::pair/自訂 struct 的特化,"
              << "所以 hash 與 == 都必須自備" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類10.cpp" -o unordered_set_demo

// 注意：unordered 容器的走訪順序未由標準規定,不同實作／不同執行可能不同。
//       下方「元素（順序不固定）」與各項 bucket 數值皆為本機(GCC 15.2.0 /
//       libstdc++ / x86-64)的實測結果,屬實作定義,不可作為跨平台的保證。

// === 預期輸出 ===
// === std::unordered_set ===
// 元素（順序不固定）: 40 20 50 10 30
// 找到 30
// 桶的數量: 13
// 負載因子: 0.384615
//
// === 要穩定輸出就先排序 ===
// 排序後(可重現): 10 20 30 40 50
// → 上面那行「順序不固定」的輸出不可寫進測試斷言
//
// === 元素與桶的對應(實作定義) ===
//   元素 10 → bucket 10 (該桶內有 1 個元素)
//   元素 20 → bucket 7 (該桶內有 1 個元素)
//   元素 30 → bucket 4 (該桶內有 1 個元素)
//   元素 40 → bucket 1 (該桶內有 1 個元素)
//   元素 50 → bucket 11 (該桶內有 1 個元素)
//
// === bucket_count 的成長(libstdc++,實作定義) ===
//   空容器            bucket_count = 1, max_load_factor = 1
//   size = 1 時 rehash → bucket_count = 13
//   size = 14 時 rehash → bucket_count = 29
//   size = 30 時 rehash → bucket_count = 59
//   size = 60 時 rehash → bucket_count = 127
//   size = 128 時 rehash → bucket_count = 257
//   size = 258 時 rehash → bucket_count = 541
//   → 13, 29, 59, 127, 257, 541 全是質數;MSVC 改用 2 的冪。
//   → 這些數值是實作定義,不可寫進跨平台的邏輯或測試。
//
// === rehash 的失效規則(與 vector 相反) ===
//   rehash 前 bucket_count = 13,rehash 後 = 541
//   元素 42 的位址在 rehash 前後相同嗎? true
//   → 節點沒有搬家,所以 reference/pointer 仍然有效;
//     但所有 iterator 已失效(走訪順序整個重建)。
//     vector 擴容則是三者全部失效 —— 方向剛好相反。
//
// === reserve() 的效果 ===
//   插入 1000 筆,未 reserve → rehash 7 次
//   插入 1000 筆,先 reserve → rehash 0 次
//   → 每次 rehash 都是 O(n),已知資料量時 reserve 幾乎是免費的優化
//
// === 記憶體佈局(實作定義) ===
// sizeof(std::unordered_set<int>) = 56
// → 容器本體只存桶陣列指標/桶數/元素數等骨架,元素在各自的 heap 節點裡
//
// === LeetCode 217. Contains Duplicate ===
// containsDuplicate({1,2,3,1}) = true
// containsDuplicate({1,2,3,4}) = false
//
// === LeetCode 128. Longest Consecutive Sequence ===
// longestConsecutive({100,4,200,1,3,2}) = 4  (連續序列 1,2,3,4)
// longestConsecutive({0,3,7,2,5,8,4,6,0,1}) = 9  (連續序列 0~8)
//
// === 日常實務: access log 不重複訪客統計 ===
// 總請求數:     8
// 不重複訪客:   4
// 命中封鎖名單: 2 個 IP
//   - 192.0.2.155
//   - 198.51.100.99
//
// === 自訂型別當 key(hash functor + operator==) ===
// 已探索格子數: 3 (插入 4 次,其中 {1,2} 重複)
// {1,2} 探索過了嗎? true
// {9,9} 探索過了嗎? false
// → std::hash 沒有 std::pair/自訂 struct 的特化,所以 hash 與 == 都必須自備
