// =============================================================================
//  第六課 14 — std::priority_queue：建在 vector 上的二元堆積(binary heap)
// =============================================================================
//
// 【主題資訊 Information】
//   template<
//       class T,
//       class Container = std::vector<T>,
//       class Compare   = std::less<typename Container::value_type>
//   > class priority_queue;                                   // <queue>,C++98 起
//
//   完整介面(一樣很少):
//     bool             empty() const;           // O(1)
//     size_type        size()  const;           // O(1)
//     const_reference  top();                   // O(1),**回傳 const 參考**;空的時候是 UB
//     void             push(const T&);          // O(log n)
//     void             push(T&&);               // O(log n),C++11
//     template<class... Args> void emplace(Args&&...);  // O(log n),C++11
//     void             pop();                   // O(log n),回傳 void;空的時候是 UB
//     void             swap(priority_queue&);   // O(1),C++11
//
//   建構:
//     priority_queue(first, last);              // **O(n)** —— 一次 heapify,不是 O(n log n)
//
//   標準版本：priority_queue 本身 C++98;emplace / push(T&&) / swap 是 C++11;
//             CTAD 是 C++17 —— 皆以 g++ -pedantic-errors 在本機逐一編譯驗證。
//   標頭檔：#include <queue>(Compare 若用 std::greater 需 <functional>)
//
// 【詳細解釋 Explanation】
//
// 【1. 它是配接器,但這個配接器多做了一件事:跑演算法】
// std::stack 和 std::queue 是**純轉發**:push 就是 c.push_back,pop 就是
// c.pop_back/pop_front,配接器自己不做任何運算。
//
// priority_queue 不同。它在轉發之外還呼叫了 <algorithm> 的堆積演算法:
//
//     pq.push(x)  →  c.push_back(x);
//                    std::push_heap(c.begin(), c.end(), comp);   // 把新元素上浮到正確位置
//     pq.pop()    →  std::pop_heap(c.begin(), c.end(), comp);    // 把最大值換到尾端
//                    c.pop_back();                                // 再砍掉尾端
//     pq.top()    →  c.front();                                   // 堆積的根就是最大值
//
// 所以它是「容器 + 演算法」的組合包:vector 負責存,heap 演算法負責維持秩序。
// 這也解釋了為什麼 push/pop 是 O(log n) 而不是 O(1) —— 那個 log n 就是
// 元素在完全二元樹裡上浮或下沉的高度。
//
// 【2. 為什麼預設底層是 vector,而不是 queue 用的 deque?—— 需要隨機存取】
// 這題和上一個檔案的答案剛好互補,值得並排記:
//
//   * std::queue        需要 pop_front  → **vector 不能用**(沒有 pop_front)
//   * std::priority_queue 需要隨機存取 → **list 不能用**(沒有 operator[])
//
// 為什麼需要隨機存取?因為二元堆積是用**陣列**表示一棵完全二元樹的,
// 父子關係靠索引算術直接算出來,不靠指標:
//
//     節點 i 的左子 = 2i + 1
//     節點 i 的右子 = 2i + 2
//     節點 i 的父   = (i - 1) / 2
//
// 堆積演算法每一步都在做「跳到 2i+1」這種跳躍存取,所以底層必須支援
// O(1) 的隨機存取(random access iterator)。vector 是最直接的選擇:
// 記憶體連續、索引即位址算術、cache 友善。
//
// deque 也有 operator[],所以 std::priority_queue<int, std::deque<int>> 合法,
// 只是每次索引多一層 chunk 表的間接存取,通常比 vector 慢。
// 而 std::list 只有 bidirectional iterator,**連編譯都過不了** ——
// 錯誤訊息會出現在 <bits/stl_heap.h> 裡,說 std::_List_iterator 找不到
// operator-。那正是堆積演算法在做 iterator 算術時撞牆的地方。
//
// 【3. 預設是 MAX-heap ——「Compare = std::less 卻是大的先出」的反直覺】
// 這是本主題最多人栽跟頭的地方,幾乎每個人第一次都會猜錯。
//
//     std::priority_queue<int> pq;   // 預設 Compare = std::less<int>
//     // 直覺:less…小的優先?
//     // 實際:**最大的**先出來。
//
// 關鍵在於 comp(a, b) 這個呼叫的語意不是「誰先出」,而是:
//
//     comp(a, b) == true  代表「a 的優先權**低於** b」
//
// std::less 就是 a < b。所以「數值小 = 優先權低」→ 數值大的優先權高
// → 最大值待在堆頂 → 這是 max-heap。
//
// 要反過來拿 min-heap,就傳一個「數值大 = 優先權低」的比較器:
//
//     std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
//
// 記憶法:**Compare 描述的是「排在後面」的條件,不是「先出來」的條件。**
// 想成 std::sort ——  sort 用 less 會排成遞增,而堆頂等於「排序後的最後一個」,
// 也就是最大值。這樣就自洽了。
//
// 【4. 二元堆積的兩個動作:sift-up 與 sift-down】
// 堆積只維持一條不變式(heap invariant):**每個父節點都不低於它的子節點**。
//
//   push(x):把 x 放到陣列尾端(= 完全二元樹最後一個位置),然後不斷和父節點
//     比較,只要比父親「優先」就交換往上走 —— 這叫 sift-up。
//     最多走到根,樹高是 log n,所以 O(log n)。
//
//   pop():根就是要移除的那個。把**尾端元素搬到根**,砍掉尾端,然後讓這個
//     新根不斷和「較優先的那個子節點」交換往下沉 —— 這叫 sift-down。
//     同樣最多 log n 步。
//
// 為什麼不是直接把根拿掉?因為那會在陣列中間挖個洞,破壞「完全二元樹」的
// 緊密結構。用尾端元素補位再下沉,是唯一能維持陣列連續的做法。
//
// 【5. 底層那個 vector 是「堆積」,不是「排好序的陣列」—— 最大的誤解】
// 這點務必看清楚。堆積只保證父子關係,**兄弟之間毫無約束**。
//
// 本機實測:依序 push 30, 10, 50, 20, 40 之後,底層 vector 的實際內容是:
//
//     [ 50, 40, 30, 10, 20 ]
//        ↑ 這是 top()
//
// 注意 10 排在 20 前面 —— 完全沒有排序。它只是滿足:
//     50 ≥ 40, 50 ≥ 30(50 的兩個子);40 ≥ 10, 40 ≥ 20(40 的兩個子)。
//
// 由此推出三個常被誤會的結論:
//   (a) **沒有排序走訪**。想要排序結果只能一直 pop,那是 O(n log n)。
//   (b) **看不到第二名**。top() 只給你第一名;第二名一定是根的某個子節點,
//       但 priority_queue 沒有介面讓你看。要看只能先 pop 掉第一名。
//   (c) **沒有 iterator**,不能 range-based for、不能用 STL 演算法 ——
//       和 stack/queue 一樣,而且理由更強:遍歷一個堆積得到的是無序的內部
//       表示,對使用者毫無意義。
//
// 【6. 從既有範圍建構是 O(n),不是 O(n log n) —— 該用就要用】
// 兩種把 n 個元素放進堆積的寫法,複雜度**不一樣**:
//
//     // 寫法 A:一個一個 push  →  O(n log n)
//     std::priority_queue<int> a;
//     for (int x : v) a.push(x);
//
//     // 寫法 B:一次從範圍建構  →  O(n)
//     std::priority_queue<int> b(v.begin(), v.end());
//
// 寫法 B 內部呼叫 std::make_heap,用的是 Floyd 的 heapify 演算法:
// 從最後一個非葉節點開始往前,對每個節點做 sift-down。
// 直覺上像是 n 次 O(log n) = O(n log n),但實際上樹的**下半層佔了一半節點,
// 而它們只需要下沉 0~1 層** —— 把每層節點數乘上該層可能的下沉距離再加總,
// 級數收斂到 O(n)。這是演算法課的經典結論,面試也常問。
//
// 已經有一整批資料時請用寫法 B。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:比 stack/queue 多一個 Compare 成員
//     libstdc++ 的 priority_queue 簡化後:
//         template<class T, class Container = vector<T>, class Compare = less<T>>
//         class priority_queue {
//         protected:
//             Container c;      // 存資料
//             Compare   comp;   // 比較器
//         };
//     本機實測(GCC 15.2.0 / x86-64,**實作定義**):
//         sizeof(std::priority_queue<int>) == 32
//         sizeof(std::vector<int>)         == 24
//     多出來的 8 是 comp 這個成員 —— std::less<int> 是空類別(沒有資料成員),
//     但它是**具名成員**而不是基底類別,無法套用空基底最佳化(EBO),
//     所以至少佔 1 byte,再因對齊補到 8。這是 stack/queue 的 sizeof 剛好等於
//     底層容器、而 priority_queue 不等於的原因。
//     (若比較器是有狀態的 lambda 或函式物件,這裡就會更大。)
//
// (B) 為什麼陣列表示的二元樹不需要指標?
//     完全二元樹(complete binary tree)的定義是:除最後一層外都填滿,
//     且最後一層靠左緊密排列。正因為「沒有空洞」,節點在層序走訪中的序號
//     就能唯一決定它的位置,父子關係退化成純算術(2i+1 / 2i+2 / (i-1)/2)。
//     於是一棵樹可以用一段連續記憶體表示,零指標、零額外配置、cache 友善。
//     這是資料結構課裡「用不變式換空間」最漂亮的例子之一。
//
// (C) top() 回傳的是 const_reference —— 你不能改它
//     本機以 type traits 實測:decltype(pq.top()) 帶 const。
//     這不是保守,而是必要:如果允許你就地修改堆頂的值,heap invariant
//     可能當場被破壞(改小之後它就不該待在根了),而 priority_queue 無從得知、
//     也不會替你重新調整。之後所有操作的結果都會是錯的。
//     需要「修改優先權」的場景(例如 Dijkstra 的 decrease-key),
//     標準的 priority_queue **做不到** —— 常見替代方案是「懶惰刪除」
//     (直接 push 新值,pop 出來時檢查是不是過期項目就丟掉),
//     或改用 std::set / 自己實作帶索引的堆積。
//
// (D) 想親眼看底層 vector?標準留了一道後門
//     底層成員 c 的存取權是 **protected**(和 stack 一樣),所以可以繼承後
//     開一個唯讀窗口出來 —— 本檔就是這樣印出「50 40 30 10 20」證明它不是排序的:
//         class HeapPeek : public std::priority_queue<int> {
//         public: const container_type& raw() const { return c; }
//         };
//     這只該用於除錯或教學。正式程式碼這麼做等於親手拆掉配接器的保護。
//
// (E) priority_queue **不是穩定的(not stable)**
//     優先權相同的兩個元素,誰先出來**沒有任何保證** —— 堆積的交換過程
//     會打亂原本的插入順序。若你的需求是「同優先權時先到先服務」,
//     必須自己在元素裡加一個遞增序號,並在比較器裡拿它當第二關鍵字。
//     本檔的實務範例就是這樣做的,這在真實排程系統裡是必備的一步。
//
// 【注意事項 Pay Attention】
//  1. 對空的 priority_queue 呼叫 top() 或 pop() 是 **undefined behavior**。
//     配接器不做任何檢查。行為不保證、不可預測:可能讀到殘留舊值、
//     可能讓 size() 變成錯誤的極大值、可能當場崩潰,也可能看起來正常跑完。
//     絕不能拿「測試時沒事」當作正確的證據 —— 正確寫法永遠是先
//     if (!pq.empty())。(本檔的可執行程式碼因此不會對空 pq 做任何存取。)
//  2. **預設是 max-heap**。Compare 預設 std::less,而 comp(a,b) 的語意是
//     「a 的優先權低於 b」,所以最大的在頂端。要 min-heap 請用 std::greater<T>。
//  3. 底層 vector 是**堆積,不是排序好的陣列**。沒有排序走訪、看不到第二名。
//     想要有序輸出只能反覆 pop,那是 O(n log n)。
//  4. **std::list 不能當底層**(堆積演算法需要隨機存取),這是編譯期錯誤。
//     可選的是 std::vector(預設)與 std::deque。
//  5. top() 回傳 const 參考,不能就地修改。標準的 priority_queue **不支援
//     decrease-key**,需要改優先權請用懶惰刪除或換資料結構。
//  6. **不穩定**:相同優先權的元素出場順序不保證。要 FIFO 平手規則,
//     請自己加遞增序號當第二關鍵字。
//  7. 自訂的 Compare 必須是 **strict weak ordering**(嚴格弱序)。
//     常見錯誤是寫成 <=(不嚴格),這會讓「a 不優於 a」這條不成立,
//     堆積演算法的行為就變成 undefined —— 而且症狀通常是偶發的錯誤順序
//     或越界,極難追查。比較器裡永遠用 < 或 >,不要用 <= 或 >=。
//  8. sizeof(std::priority_queue<int>) == 32 是本機實測值,屬**實作定義**。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::priority_queue
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::priority_queue 預設是最大堆還是最小堆?為什麼?怎麼改成另一種?
//     答：預設是**最大堆(max-heap)**,最大的元素在 top()。原因是 Compare
//         預設為 std::less,而比較器的語意是 comp(a,b)==true 代表
//         「**a 的優先權低於 b**」;less 即 a<b,也就是「數值小 = 優先權低」,
//         於是數值最大的優先權最高、待在堆頂。
//         要最小堆就換一個把大數視為低優先的比較器:
//           std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
//         注意換 Compare 必須連中間的 Container 一起寫出來,因為它是第二個
//         模板參數,不能跳過。
//     追問：那為什麼底層預設是 vector,不是 queue 用的 deque?
//         → 因為二元堆積是用陣列表示完全二元樹,父子關係靠索引算術
//           (2i+1 / 2i+2 / (i-1)/2),每一步都要 O(1) 隨機存取。
//           vector 連續記憶體最適合;deque 也可以但多一層間接;
//           **list 完全不行,會編譯失敗**。
//
// 🔥 Q2. push n 個元素 vs 用範圍建構子一次建好,複雜度差在哪?
//     答：逐個 push 是 n 次 O(log n) = **O(n log n)**;
//         用 priority_queue(first, last) 走的是 std::make_heap,是 **O(n)**。
//         make_heap 用 Floyd heapify:從最後一個非葉節點往前,逐個 sift-down。
//         看起來像 O(n log n),但完全二元樹有一半節點在最底層、只需下沉 0 層,
//         往上每層節點數減半而下沉距離才加一,加總的級數收斂到 O(n)。
//         所以資料已經在手上時,一定要用範圍建構。
//     追問：那 pop 全部出來排序,總共是多少?
//         → O(n log n)。這就是 heapsort:O(n) 建堆 + n 次 O(log n) 的 pop。
//           和 std::sort 同級,但 heapsort 是原地且最壞情況也保證 O(n log n)。
//
// ⚠️ 陷阱. 想看看 priority_queue 裡「第二大」的元素,或把它從大到小印出來,
//         直接遍歷底層那個 vector 就好了吧?
//     答：不行。底層 vector 是**堆積,不是排序好的陣列**。本機實測依序 push
//         30,10,50,20,40 後,底層實際是 [50, 40, 30, 10, 20] —— 注意 10 排在
//         20 前面,根本沒排序。堆積只保證「父不低於子」,兄弟之間毫無關係。
//         而且 priority_queue 也沒有 iterator,連遍歷的介面都沒開放。
//         要有序輸出只能反覆 top()+pop(),O(n log n)。
//     為什麼會錯：把「有優先權」直接想成「排好序」。實際上堆積是一種
//         **偏序(partial order)**結構 —— 它刻意只維持剛好夠用的秩序,
//         因為完整排序太貴。O(log n) 的插入正是靠「不做全排序」換來的。
//
// ⚠️ 陷阱. 兩個任務優先權一樣,先 push 的會先出來吧?
//     答：**沒有任何保證**。std::priority_queue 不是穩定的(not stable) ——
//         sift-up / sift-down 的交換會打亂相同優先權元素的相對順序,
//         標準也完全沒承諾。實務上這會變成「同樣是普通優先權的任務,
//         有時候後送的先跑」這種偶發、難重現的 bug。
//     為什麼會錯：把 priority_queue 想成「一個會自動插隊的 queue」,
//         以為它保留了 queue 的 FIFO 底色。它們的內部結構毫無關係。
//         正解是自己在元素裡放一個遞增序號,比較器裡當第二關鍵字:
//         優先權相同時,序號小的(先送出的)優先。本檔實務範例就是這麼做。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <queue>
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【概念示範用】繼承以取得 protected 的底層容器,證明它不是排序好的陣列
//   標準規定 priority_queue 的成員 c 是 protected,所以子類別可以讀到它。
//   這是標準留的後門,只該用於除錯或教學 —— 正式程式碼這麼做等於拆掉保護。
// -----------------------------------------------------------------------------
class HeapPeek : public std::priority_queue<int> {
public:
    const container_type& raw() const { return c; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 215. Kth Largest Element in an Array
//   題目：找出陣列中**第 k 大**的元素(是排序後的第 k 個,不是第 k 個相異值)。
//   為什麼用到本主題：這是 priority_queue 最經典的應用,也是「為什麼不直接
//     排序」的最佳教材。全排序是 O(n log n);而我們只要第 k 大,
//     用一個**容量固定為 k 的最小堆**就好:
//       - 掃過每個元素,push 進堆;
//       - 堆的大小超過 k 就 pop 掉最小的那個。
//     掃完之後,堆裡剩下的正好是「最大的 k 個」,而堆頂(最小堆的最小值)
//     就是這 k 個之中最小的 —— 也就是全體第 k 大。
//   關鍵：這裡要用 **min-heap**(std::greater),不是預設的 max-heap ——
//     因為我們要能隨時丟掉「目前這 k 個裡最小的那個」。這個反直覺處
//     正是本題的考點。
//   複雜度：時間 O(n log k),空間 O(k)。當 k 遠小於 n 時明顯優於排序,
//     而且資料是串流(無法一次全載入)時,這是唯一可行的做法。
// -----------------------------------------------------------------------------
int findKthLargest(const std::vector<int>& nums, int k) {
    // min-heap:堆頂是目前這批裡最小的,方便淘汰
    std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;

    for (int x : nums) {
        minHeap.push(x);
        if (minHeap.size() > static_cast<size_t>(k)) {
            minHeap.pop();          // 淘汰最小的,只保留最大的 k 個
        }
    }
    // 迴圈保證了 size()==k(呼叫端須確保 1 <= k <= nums.size()),
    // 此處不會對空堆呼叫 top()。
    return minHeap.top();           // k 個之中最小的 = 全體第 k 大
}

// -----------------------------------------------------------------------------
// 【日常實務範例】背景任務排程器(task scheduler)—— 急件優先,同級先到先做
//   情境：接續上一個檔案的印表機 spooler。純 FIFO 對「主管的急件」和
//     「某人的 300 頁講義」一視同仁,實務上不可接受。真實的排程器
//     (作業系統、CI runner、訊息佇列、印表機驅動)都會分優先權:
//     急件插隊,但**同一優先權內仍必須先到先服務**,否則使用者會覺得
//     系統在隨機挑人。
//   為什麼用到本主題：「每次取出目前最該做的那一個」正是 priority_queue
//     的定義。O(log n) 的插入與取出,遠優於「每次線性掃一遍找最急的」O(n)。
//   關鍵細節(真實系統必備)：priority_queue **不是穩定的** —— 優先權相同時
//     出場順序沒有保證。所以每筆任務都帶一個遞增的 seq_,比較器把它當
//     第二關鍵字:優先權相同時,序號小的(先送出的)先做。少了這一步,
//     就會出現「同樣是普通任務,有時候後送的先跑」這種難以重現的抱怨。
// -----------------------------------------------------------------------------
struct Task {
    int         priority;    // 數字越小越急:0=緊急, 1=高, 2=普通
    unsigned    seq;         // 送出序號,用來打破平手 → 讓同級維持 FIFO
    std::string name;
};

struct TaskCompare {
    // 注意比較器的語意:回傳 true 代表「a 的優先權**低於** b」(a 會比較晚出來)
    bool operator()(const Task& a, const Task& b) const {
        if (a.priority != b.priority) {
            return a.priority > b.priority;   // priority 數字大 = 比較不急 = 低優先
        }
        return a.seq > b.seq;                 // 同級:序號大(後送的) = 低優先
    }
    // 全程只用 > ,沒有用 >= —— 必須維持 strict weak ordering。
};

class TaskScheduler {
public:
    void submit(const std::string& name, int priority) {
        tasks_.push(Task{priority, nextSeq_++, name});
    }

    // 取出並執行下一個最該做的任務。沒有任務時回傳 false。
    // 這個 empty() 檢查是必要的,不是防禦性冗餘 ——
    // 少了它,對空的 priority_queue 呼叫 top() 就是 undefined behavior。
    bool runNext(std::string& logLine) {
        if (tasks_.empty()) return false;

        const Task& t = tasks_.top();         // top() 回傳 const 參考,不能就地改
        logLine = "  執行: " + t.name
                + "  (優先權=" + levelName(t.priority)
                + ", 送出序=" + std::to_string(t.seq) + ")";
        tasks_.pop();                         // 先讀再移除(pop 回傳 void)
        return true;
    }

    size_t pending() const { return tasks_.size(); }

private:
    static const char* levelName(int p) {
        switch (p) {
            case 0:  return "緊急";
            case 1:  return "高";
            default: return "普通";
        }
    }

    std::priority_queue<Task, std::vector<Task>, TaskCompare> tasks_;
    unsigned nextSeq_ = 0;
};

int main() {
    // ── 原始課堂示範:max-heap 與 min-heap ─────────────────────────────────
    std::cout << "=== std::priority_queue ===" << std::endl;

    // 預設是最大堆（最大的在頂端）
    std::priority_queue<int> max_pq;

    max_pq.push(30);
    max_pq.push(10);
    max_pq.push(50);
    max_pq.push(20);
    max_pq.push(40);

    std::cout << "最大堆依序取出: ";
    while (!max_pq.empty()) {
        std::cout << max_pq.top() << " ";
        max_pq.pop();
    }
    std::cout << std::endl;

    // 最小堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;

    min_pq.push(30);
    min_pq.push(10);
    min_pq.push(50);
    min_pq.push(20);
    min_pq.push(40);

    std::cout << "最小堆依序取出: ";
    while (!min_pq.empty()) {
        std::cout << min_pq.top() << " ";
        min_pq.pop();
    }
    std::cout << std::endl;

    // ── 配接器的組成:vector + Compare ─────────────────────────────────────
    std::cout << "\n=== 配接器組成實證 ===" << std::endl;
    std::cout << "sizeof(std::priority_queue<int>) = "
              << sizeof(std::priority_queue<int>) << std::endl;
    std::cout << "sizeof(std::vector<int>)         = "
              << sizeof(std::vector<int>) << std::endl;
    std::cout << "多出來的部分是 Compare 成員(std::less<int> 雖是空類別,"
              << "但身為具名成員無法套用 EBO,對齊後仍佔位;數值為實作定義)"
              << std::endl;
    // 預設底層容器是 std::vector<T>
    static_assert(
        std::is_same<std::priority_queue<int>::container_type, std::vector<int>>::value,
        "priority_queue 的預設底層容器是 std::vector");

    // ── 底層是「堆積」不是「排序好的陣列」 ────────────────────────────────
    std::cout << "\n=== 底層真相: 它是 heap,不是 sorted array ===" << std::endl;
    HeapPeek peek;
    for (int x : {30, 10, 50, 20, 40}) peek.push(x);
    std::cout << "依序 push 30,10,50,20,40 後,底層 vector 實際內容: ";
    for (int x : peek.raw()) std::cout << x << " ";
    std::cout << std::endl;
    std::cout << "top() = " << peek.top() << std::endl;
    std::cout << "注意 10 排在 20 前面 → 完全沒有排序,只保證『父 ≥ 子』"
              << std::endl;
    std::cout << "父子關係靠索引算術: 節點 i 的子在 2i+1 與 2i+2" << std::endl;
    const std::vector<int>& h = peek.raw();
    for (size_t i = 0; i * 2 + 1 < h.size(); ++i) {
        std::cout << "  h[" << i << "]=" << h[i] << " ≥ 子 h[" << (2 * i + 1)
                  << "]=" << h[2 * i + 1];
        if (2 * i + 2 < h.size()) {
            std::cout << " , h[" << (2 * i + 2) << "]=" << h[2 * i + 2];
        }
        std::cout << std::endl;
    }

    // ── O(n) 範圍建構 vs O(n log n) 逐個 push ─────────────────────────────
    std::cout << "\n=== 範圍建構: O(n) heapify ===" << std::endl;
    std::vector<int> data{3, 1, 4, 1, 5, 9, 2, 6};
    std::priority_queue<int> fromRange(data.begin(), data.end());   // 一次 make_heap → O(n)
    std::cout << "從 {3,1,4,1,5,9,2,6} 一次建堆,大小: " << fromRange.size()
              << ",top: " << fromRange.top() << std::endl;
    std::cout << "逐個 push 是 O(n log n);範圍建構走 make_heap,是 O(n)" << std::endl;

    // ── 更換底層容器:deque 可以,list 不行 ────────────────────────────────
    std::cout << "\n=== 更換底層容器 ===" << std::endl;
    std::priority_queue<int, std::deque<int>> dpq;   // deque 有 operator[] → 合法
    for (int x : {5, 1, 9}) dpq.push(x);
    std::cout << "std::priority_queue<int, std::deque<int>> top: " << dpq.top()
              << ",大小: " << dpq.size() << std::endl;
    std::cout << "但 std::priority_queue<int, std::list<int>> 無法編譯 ——" << std::endl;
    std::cout << "  堆積演算法需要隨機存取,list 只有 bidirectional iterator"
              << std::endl;
    std::cout << "  (錯誤會出現在 <bits/stl_heap.h>:找不到 operator-,故此處不示範)"
              << std::endl;

    // ── LeetCode 215 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 215. Kth Largest Element in an Array ===" << std::endl;
    std::vector<int> nums1{3, 2, 1, 5, 6, 4};
    std::cout << "nums={3,2,1,5,6,4}, k=2 → " << findKthLargest(nums1, 2)
              << "  (排序後為 1 2 3 4 5 6,第 2 大是 5)" << std::endl;
    std::vector<int> nums2{3, 2, 3, 1, 2, 4, 5, 5, 6};
    std::cout << "nums={3,2,3,1,2,4,5,5,6}, k=4 → " << findKthLargest(nums2, 4)
              << "  (重複值各自計數,第 4 大是 4)" << std::endl;
    std::cout << "用容量 k 的 min-heap: O(n log k),優於全排序的 O(n log n)"
              << std::endl;

    // ── 日常實務:任務排程器 ──────────────────────────────────────────────
    std::cout << "\n=== 日常實務: 背景任務排程器(急件優先, 同級 FIFO) ===" << std::endl;
    TaskScheduler sched;
    sched.submit("每日備份",       2);   // 普通
    sched.submit("寄送驗證信",     1);   // 高
    sched.submit("資料庫還原",     0);   // 緊急
    sched.submit("產生月報",       2);   // 普通(比「每日備份」晚送)
    sched.submit("清理暫存檔",     2);   // 普通(最晚送)
    sched.submit("重設密碼通知",   1);   // 高(比「寄送驗證信」晚送)

    std::cout << "待處理任務數: " << sched.pending() << std::endl;
    std::cout << "執行順序:" << std::endl;
    std::string logLine;
    while (sched.runNext(logLine)) {
        std::cout << logLine << std::endl;
    }
    std::cout << "全部完成,剩餘: " << sched.pending() << std::endl;
    std::cout << "→ 急件最先;同優先權內嚴格照送出序號,"
              << "這是靠比較器的第二關鍵字換來的(priority_queue 本身不穩定)"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類14.cpp" -o pq_demo

// === 預期輸出 ===
// === std::priority_queue ===
// 最大堆依序取出: 50 40 30 20 10
// 最小堆依序取出: 10 20 30 40 50
//
// === 配接器組成實證 ===
// sizeof(std::priority_queue<int>) = 32
// sizeof(std::vector<int>)         = 24
// 多出來的部分是 Compare 成員(std::less<int> 雖是空類別,但身為具名成員無法套用 EBO,對齊後仍佔位;數值為實作定義)
//
// === 底層真相: 它是 heap,不是 sorted array ===
// 依序 push 30,10,50,20,40 後,底層 vector 實際內容: 50 40 30 10 20
// top() = 50
// 注意 10 排在 20 前面 → 完全沒有排序,只保證『父 ≥ 子』
// 父子關係靠索引算術: 節點 i 的子在 2i+1 與 2i+2
//   h[0]=50 ≥ 子 h[1]=40 , h[2]=30
//   h[1]=40 ≥ 子 h[3]=10 , h[4]=20
//
// === 範圍建構: O(n) heapify ===
// 從 {3,1,4,1,5,9,2,6} 一次建堆,大小: 8,top: 9
// 逐個 push 是 O(n log n);範圍建構走 make_heap,是 O(n)
//
// === 更換底層容器 ===
// std::priority_queue<int, std::deque<int>> top: 9,大小: 3
// 但 std::priority_queue<int, std::list<int>> 無法編譯 ——
//   堆積演算法需要隨機存取,list 只有 bidirectional iterator
//   (錯誤會出現在 <bits/stl_heap.h>:找不到 operator-,故此處不示範)
//
// === LeetCode 215. Kth Largest Element in an Array ===
// nums={3,2,1,5,6,4}, k=2 → 5  (排序後為 1 2 3 4 5 6,第 2 大是 5)
// nums={3,2,3,1,2,4,5,5,6}, k=4 → 4  (重複值各自計數,第 4 大是 4)
// 用容量 k 的 min-heap: O(n log k),優於全排序的 O(n log n)
//
// === 日常實務: 背景任務排程器(急件優先, 同級 FIFO) ===
// 待處理任務數: 6
// 執行順序:
//   執行: 資料庫還原  (優先權=緊急, 送出序=2)
//   執行: 寄送驗證信  (優先權=高, 送出序=1)
//   執行: 重設密碼通知  (優先權=高, 送出序=5)
//   執行: 每日備份  (優先權=普通, 送出序=0)
//   執行: 產生月報  (優先權=普通, 送出序=3)
//   執行: 清理暫存檔  (優先權=普通, 送出序=4)
// 全部完成,剩餘: 0
// → 急件最先;同優先權內嚴格照送出序號,這是靠比較器的第二關鍵字換來的(priority_queue 本身不穩定)
