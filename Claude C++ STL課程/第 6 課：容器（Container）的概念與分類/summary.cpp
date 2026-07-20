/*
 * ================================================================
 * 【第 6 課：容器（Container）的概念與分類】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -Wall -Wextra summary.cpp -o summary
 *
 * 本課重點：
 * 1. 容器的概念：管理一組物件的資料結構
 * 2. 序列容器（Sequence Containers）：array, vector, deque, list, forward_list
 * 3. 關聯容器（Associative Containers）：set, map, multiset, multimap
 * 4. 無序關聯容器（Unordered）：unordered_set, unordered_map
 * 5. 容器配接器（Adapters）：stack, queue, priority_queue
 * 6. 如何選擇正確的容器
 * ================================================================
 */

// =============================================================================
//  summary.cpp — STL 容器全景：四大分類、記憶體模型、與選容器的決策方法
// =============================================================================
//
// 【主題資訊 Information】
//
//   四大分類與標頭檔（C++17 為準）：
//
//   ┌──────────────┬─────────────────────────────┬──────────────┬──────────┐
//   │ 分類          │ 容器                        │ 標頭檔        │ 標準版本  │
//   ├──────────────┼─────────────────────────────┼──────────────┼──────────┤
//   │ 序列容器      │ array                       │ <array>       │ C++11    │
//   │              │ vector                      │ <vector>      │ C++98    │
//   │              │ deque                       │ <deque>       │ C++98    │
//   │              │ list                        │ <list>        │ C++98    │
//   │              │ forward_list                │ <forward_list>│ C++11    │
//   ├──────────────┼─────────────────────────────┼──────────────┼──────────┤
//   │ 關聯容器      │ set / multiset              │ <set>         │ C++98    │
//   │（有序）       │ map / multimap              │ <map>         │ C++98    │
//   ├──────────────┼─────────────────────────────┼──────────────┼──────────┤
//   │ 無序關聯容器  │ unordered_set / _multiset   │<unordered_set>│ C++11    │
//   │（雜湊）       │ unordered_map / _multimap   │<unordered_map>│ C++11    │
//   ├──────────────┼─────────────────────────────┼──────────────┼──────────┤
//   │ 容器配接器    │ stack / queue               │ <stack><queue>│ C++98    │
//   │              │ priority_queue              │ <queue>       │ C++98    │
//   └──────────────┴─────────────────────────────┴──────────────┴──────────┘
//
//   核心操作複雜度總表（標準保證的上界，非實測常數）：
//
//   容器             隨機存取   頭部插入   尾部插入   中間插入   查找
//   array            O(1)      ✗         ✗         ✗         O(n)
//   vector           O(1)      O(n)      攤銷 O(1)  O(n)      O(n)
//   deque            O(1)      O(1)      O(1)      O(n)      O(n)
//   list             ✗         O(1)      O(1)      O(1)*     O(n)
//   forward_list     ✗         O(1)      ✗         O(1)*     O(n)
//   set / map        ✗         —         —         O(log n)  O(log n)
//   unordered_set/map ✗        —         —         平均 O(1)  平均 O(1)
//                                                            最壞 O(n)
//   （* 表示「已持有該位置的 iterator」的前提下。）
//
// 【詳細解釋 Explanation】
//
// 【1. 容器到底是什麼:所有權與 value semantics】
// STL 容器不只是「裝東西的盒子」。它的核心承諾是**所有權(ownership)**:
// 容器擁有它所儲存的元素,負責它們的建構、複製、搬移與解構,並在自己被解構時
// 自動釋放全部資源。這正是 RAII 在資料結構層次的體現 —— 你不必寫任何
// delete,離開 scope 就乾淨。
//
// 由此推出兩個常被忽略的結論:
//   (a) 容器儲存的是元素的**副本**,不是參考。把物件 push_back 進 vector,
//       vector 裡的是另一個物件。想要共享而非複製,要存指標或 smart pointer。
//   (b) 元素型別必須滿足容器要求的操作。以前(C++03)要求 CopyConstructible;
//       C++11 之後大幅放寬,只需要你**實際用到的操作**可行 —— 例如只
//       emplace_back 且從不複製,元素甚至可以是 move-only(如 unique_ptr)。
//
// 【2. 為什麼剛好分成這四類:兩條設計軸線】
// 分類不是隨意的,它由兩個正交的問題決定:
//
//   軸線一:元素的位置由誰決定?
//     * 由「你插入的順序」決定 → 序列容器。你說放哪就放哪。
//     * 由「元素的值」決定     → 關聯容器 / 無序容器。容器自己安排位置,
//                                 因為它要靠位置來加速查找。
//
//   軸線二(僅對「值決定位置」者):靠比較還是靠雜湊?
//     * 靠比較(operator< 或自訂 Compare) → 有序關聯容器,得到「順序」這個
//       額外能力(可以走訪出排序結果、可以做範圍查詢 lower_bound)。
//     * 靠雜湊(std::hash)                → 無序容器,放棄順序,換取
//       平均 O(1) 的查找。
//
// 配接器則是完全不同層次的東西 —— 它不是第五種資料結構,而是「對既有容器
// 的介面再包裝」,見第 6 點。
//
// 【3. 序列容器的根本二分:連續記憶體 vs 節點串接】
// 五種序列容器可以先切成兩個世界,其他差異都是這個二分的推論:
//
//   ● 連續記憶體(array、vector,deque 是「分段連續」的特例)
//     元素一個挨一個排列 → 可以用位址算術直接跳到第 i 個 → **隨機存取 O(1)**。
//     代價:中間插入必須把後面全部往後搬 → O(n);vector 容量不足時還要
//     重新配置整塊記憶體並搬移全部元素 → **所有 iterator、指標、reference 全部失效**。
//
//   ● 節點串接(list、forward_list)
//     每個元素獨立配置一個節點,用指標串起來 → 沒有位址算術可用 →
//     **不支援隨機存取**。但插入/刪除只是改幾個指標 → **O(1),且不影響其他節點**
//     → 其他 iterator 全部保持有效。
//
// 所以「vector vs list」的選擇,本質上是在問:
//     「我要的是快速走訪與隨機存取,還是穩定的 iterator 與 O(1) 拼接?」
//
// 【4. 有序關聯容器:為什麼是平衡樹】
// set/map 需要同時提供三件事:有序走訪、O(log n) 查找、以及插入刪除時
// 其他元素的 iterator 不失效。平衡二元搜尋樹(libstdc++ 用紅黑樹,屬**實作定義**;
// 標準只規定複雜度上界,沒有規定樹的種類)是同時滿足這三者的結構。
//
// 「有序」這個能力不是免費的,但它換來一個雜湊表**做不到**的功能:**範圍查詢**。
//     lower_bound(k) / upper_bound(k) / equal_range(k)
// 「找出所有大於等於 k 的鍵」「找出 [a,b) 區間內的所有項目」這種需求,
// unordered_map 完全無能為力(它的元素在記憶體中根本沒有順序關係)。
// 這才是選 map 而不是 unordered_map 的真正理由 —— 不是效能,是能力。
//
// 【5. 無序容器:平均 O(1) 的代價是什麼】
// unordered_* 用雜湊表實作。libstdc++ 採 separate chaining(每個 bucket 掛一條
// 鏈結串列),bucket 數量走質數序列 —— 本機實測 1 → 13 → 29 → 59 → 127 → 257,
// max_load_factor 預設為 1(以上皆**實作定義**;MSVC 用 2 的冪次策略)。
//
// 代價有三個,而且都很實際:
//   (a) 最壞情況是 O(n)。所有 key 雜湊到同一個 bucket 時,查找退化成走訪一條
//       長鏈。這不只是理論 —— 惡意構造的輸入可以刻意製造碰撞,這是真實存在的
//       攻擊手法(hash flooding)。
//   (b) rehash 會讓**所有 iterator 失效**。但要特別注意一個對比:指向**元素本身**
//       的指標與 reference **仍然有效**,因為節點沒有被搬動,只是重新掛到別的
//       bucket。這和 vector 重新配置時「指標、reference、iterator 全滅」正好相反,
//       是面試極愛考的細節。
//   (c) 沒有順序。走訪順序未由標準規定,不同實作、甚至不同執行都可能不同,
//       絕對不可以依賴。
//
// 【6. 容器配接器:限制本身就是價值】
// stack、queue、priority_queue 不是容器,它們**包住**一個真正的容器,然後
// 刻意把介面砍掉:
//     template<class T, class Container = deque<T>>  class stack;
//     template<class T, class Container = deque<T>>  class queue;
//     template<class T, class Container = vector<T>,
//              class Compare = less<T>>              class priority_queue;
//
// 為什麼要故意讓功能變少?因為**不可能做錯的事,就不會做錯**。用 stack 表達
// 「這是後進先出的資料」時,任何人都無法對它做索引存取或從中間刪除 ——
// 這個限制把設計意圖直接寫進型別系統,是自我文件化的程式碼。代價是它們
// **沒有 iterator**,因此不能用 range-based for、不能餵給 STL 演算法。
// 這是設計取捨,不是缺陷。
//
// 注意預設底層容器**不一樣**:stack/queue 用 deque,priority_queue 用 vector。
// 原因在於 priority_queue 內部是 binary heap,heap 演算法需要**隨機存取
// iterator** 才能用位址算術找到 parent/child(索引 i 的子節點是 2i+1、2i+2),
// 所以底層必須是 vector 或 deque —— **list 無法支撐 priority_queue**。
//
// 【7. 選容器不要背表,要問四個問題】
// 死記「什麼情況用什麼」很快就會忘。實務上照下面順序問,答案自己會浮現:
//
//   Q1. 我需要用「鍵」查找嗎?
//        否 → 序列容器,跳 Q2。
//        是 → 需要有序走訪或範圍查詢嗎?
//               要 → map / set
//               不要 → unordered_map / unordered_set(較快)
//   Q2. 大小在編譯期就固定嗎?
//        是 → array(零開銷,資料在 stack)
//        否 → 跳 Q3
//   Q3. 我主要在哪裡增刪?
//        只在尾端            → vector
//        頭尾都要            → deque
//        任意位置且已有 iterator,或需要 iterator 永遠不失效 → list
//   Q4. 存取模式受限嗎?
//        LIFO → stack;FIFO → queue;每次取最大/最小 → priority_queue
//
// 真正的預設答案是:**不確定就用 vector**。它的 cache locality 極佳,即使在
// 理論上不利的場景(例如中間插入)也常常因為記憶體連續而勝過 list。
//
// 【8. iterator 失效:唯一必須跨容器記牢的表】
// 這是容器主題中最常在程式碼裡真正咬人的一點,值得單獨記住:
//
//   容器           插入時                              刪除時
//   vector        重新配置 → 全部失效;               被刪點之後全部失效
//                 未重新配置 → 插入點之後失效
//   deque         兩端插入 → iterator 全失效,
//                 但 reference/指標仍有效;
//                 中間插入 → 全部失效                 兩端刪 → 只有被刪的失效
//   list /        永不失效                            只有被刪的失效
//   forward_list
//   set / map     永不失效                            只有被刪的失效
//   unordered_*   rehash 時 iterator 全失效,          只有被刪的失效
//                 reference/指標仍有效
//
// 從這張表可以看出一條規律:**節點式容器的穩定性遠高於連續式容器**。
// 這正是「明明 vector 比較快,為什麼還需要 list/map」的關鍵答案之一。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局全景
//     vector:   [ptr_begin | ptr_end | ptr_cap] ──→ heap 上一整塊連續元素
//     deque:    [map 指標陣列] ──→ 多個固定大小的 chunk(本機 deque<int> 每
//               chunk 128 個元素 = 512 bytes,**實作定義**)
//     list:     每個節點 [prev | next | value],彼此以指標串接,散落 heap
//     forward_list: 每個節點 [next | value] —— 只有一個指標
//     set/map:  紅黑樹節點 [parent | left | right | colour | value]
//     unordered_*: bucket 陣列 ──→ 每格掛一條節點鏈
//
// (B) 容器物件本身的大小(本機實測,GCC 15.2.0 / x86-64,皆**實作定義**)
//       sizeof(std::array<int,5>)        = 20   ← 就是 5 個 int,零額外開銷
//       sizeof(std::vector<int>)         = 24   ← 三個指標
//       sizeof(std::deque<int>)          = 80   ← map 指標 + 兩組完整 iterator
//       sizeof(std::list<int>)           = 24
//       sizeof(std::forward_list<int>)   = 8    ← 只有一個 head 指標!
//       sizeof(std::set<int>)            = 48
//       sizeof(std::map<int,int>)        = 48
//       sizeof(std::unordered_set<int>)  = 56
//     注意這是**容器物件本身**的大小,不含它在 heap 上管理的元素。
//     forward_list 只有 8 bytes 正是它存在的理由:與手寫 C 單向鏈結串列
//     完全等價的開銷,連 size 計數都不存(所以它沒有 size())。
//
// (C) 為什麼 list 常常打不過 vector
//     理論複雜度說 list 中間插入 O(1)、vector O(n),但實測中 vector 經常獲勝,
//     原因是理論模型忽略了 cache:
//       * vector 元素連續 → 一次 cache line 載入就帶進多個元素,
//         硬體 prefetcher 也能正確預測 → 走訪幾乎全部命中 cache。
//       * list 節點散落 heap → 每次 ++it 都可能是一次 cache miss,
//         而一次 miss 的代價可達數十到上百個 CPU cycle。
//       * 而且 vector 的 O(n) 搬移是 memmove,是 CPU 最擅長的連續複製。
//     所以「中間插入很多 → 用 list」這個教科書建議,在元素小、資料量不大時
//     常常是錯的。這是實務上反覆被量測驗證的現象;確切的交叉點取決於元素大小、
//     資料量與硬體,請以你自己的量測為準,不要照抄任何特定數字。
//
// (D) 為什麼複雜度表不是全部
//     O(log n) 的 map 查找,實際上每一步都是一次指標跳躍 = 潛在 cache miss;
//     O(1) 的 unordered_map 查找,要先算 hash(對字串是走訪整個字串)、
//     再找 bucket、再走鏈。所以在 n 很小(幾十個元素)時,一個線性掃描的
//     vector 往往比兩者都快。複雜度描述的是**成長趨勢**,不是**絕對速度**。
//
// (E) 關於本檔開頭的 using namespace std;
//     本檔沿用原始教材寫法以保持一致,但要說清楚:在標頭檔或大型專案中
//     using namespace std; 是不建議的做法。它把整個 std 命名空間拉進當前
//     作用域,容易與自訂名稱衝突(例如自己寫的 count、distance、size),
//     且衝突發生時的錯誤訊息極難閱讀。慣例是在 .cpp 的局部作用域使用,
//     或明確寫出 using std::vector; 這類個別宣告。
//
// 【注意事項 Pay Attention】
//  1. 複雜度表是標準保證的**上界**,不代表實際速度。常數因子與 cache 行為
//     經常主導小資料量下的結果。
//  2. 所有標示為「實作定義」的數值(vector 成長倍率、deque chunk 大小、
//     bucket 數列、sizeof、max_size)都是本機 libstdc++ 的實測值,
//     換編譯器或標準庫就可能不同,不可寫進程式邏輯。
//  3. 對空容器呼叫 front()、back()、top()、pop() 是 undefined behavior。
//     行為**不保證、不可預測** —— 可能崩潰,也可能安靜地讀到垃圾值繼續執行。
//     絕不能用「測起來沒事」當作正確性的證據。
//  4. unordered_* 的走訪順序未由標準規定,任何依賴它的程式都是錯的。
//     需要穩定輸出請先複製到 vector 排序,或直接改用 map/set。
//  5. map 的 operator[] 在鍵不存在時會**預設建構並插入**該鍵。它是寫入介面,
//     不是查詢介面;純查詢請用 find()、at() 或 C++20 的 contains()。
//     也因為會修改容器,operator[] 無法用在 const map 上。
//  6. iterator 失效規則必須連同「是否重新配置」一起判斷。vector 的
//     push_back 只有在觸發 reallocation 時才讓全部 iterator 失效 ——
//     但你通常無法預知何時觸發,所以安全的寫法是「插入後一律視為已失效」。
//  7. 容器儲存的是副本。存 raw pointer 時容器不管理生命週期,元素被刪除後
//     指標就懸空;需要共享所有權請用 shared_ptr,獨佔則用 unique_ptr。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】STL 容器分類與選擇
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. STL 容器分成哪幾類?分類的依據是什麼?
//     答：四類 —— 序列容器(array/vector/deque/list/forward_list)、
//         有序關聯容器(set/map/multiset/multimap)、無序關聯容器
//         (unordered_ 系列)、容器配接器(stack/queue/priority_queue)。
//         依據是兩條軸線:元素位置由「插入順序」還是「元素的值」決定;
//         若由值決定,再看是靠比較(有序)還是靠雜湊(無序)。配接器不是
//         獨立資料結構,而是包裝既有容器並限制其介面。
//     追問：那 std::string 算不算容器?
//         → 算。它滿足容器需求(有 begin/end/size/iterator),可以餵給 STL
//           演算法,實質上是 vector<char> 加上大量字串專用介面;但它通常
//           被歸在字串主題而非容器主題討論。
//
// 🔥 Q2. map 和 unordered_map 該怎麼選?只看效能就選 unordered_map 嗎?
//     答：不是。unordered_map 平均 O(1)、map 是 O(log n),但選擇的首要
//         依據應該是**能力**而不是速度:需要有序走訪、lower_bound/upper_bound
//         範圍查詢、或最壞情況保證時,只能用 map(unordered 根本做不到範圍查詢,
//         最壞還會退化成 O(n))。其次才考慮效能,而且 key 是長字串時
//         unordered_map 每次查找都要走訪整個字串算 hash,優勢會縮小。
//     追問：unordered_map 的最壞情況為什麼是 O(n)?
//         → 所有 key 雜湊到同一個 bucket 時,查找退化成走訪一條鏈。
//           惡意輸入可刻意製造這種碰撞,即 hash flooding 攻擊。
//
// 🔥 Q3. vector 的 push_back 為什麼是「攤銷 O(1)」而不是 O(1)?
//     答：容量不足時要配置新記憶體並搬移全部既有元素,那一次是 O(n)。
//         但因為容量是**倍數成長**(本機 libstdc++ 為 2 倍,屬實作定義),
//         n 次 push_back 的總搬移量是 1+2+4+…+n < 2n,平均分攤到每次
//         就是常數 —— 這就是攤銷分析。若改成「每次只加 1」的線性成長,
//         總成本會變成 O(n²)。
//     追問：既然如此,為什麼 MSVC 用 1.5 倍而不是 2 倍?
//         → 1.5 倍成長時,先前釋放的區塊總和有機會滿足後續的配置需求
//           (2 倍成長則永遠不夠),對記憶體重用與碎片較友善。
//           兩者都是合法選擇,標準未規定倍率。
//
// ⚠️ 陷阱. 「中間會頻繁插入,所以應該用 list」—— 這個推論哪裡有問題?
//     答：只有在「已經持有該位置的 iterator」時,list 插入才是 O(1)。
//         如果每次都要先找到插入點,尋找本身就是 O(n) 的走訪,總成本
//         和 vector 一樣是 O(n) —— 而且 list 走訪的每一步都可能 cache miss,
//         vector 的搬移卻是 memmove。實測中元素小、資料量中等時 vector
//         經常勝出。
//     為什麼會錯：把複雜度表當成效能排行榜,忽略了 (1) O(1) 附帶的前提條件、
//         (2) 大 O 隱藏的常數因子、(3) 現代 CPU 上 cache 才是主導因素。
//
// ⚠️ 陷阱. vector 重新配置時 iterator 會失效;那 unordered_map 在 rehash 時,
//         指向元素的**指標和 reference** 也會失效嗎?
//     答：不會。rehash 只是把節點重新掛到不同的 bucket,節點本身沒有被搬動,
//         所以指標與 reference 仍然有效 —— 失效的只有 iterator。
//         這和 vector 重新配置時「iterator、指標、reference 全部失效」
//         正好相反。
//     為什麼會錯：把「iterator 失效」直接等同於「元素被搬走」。對連續式容器
//         這個直覺成立,但對節點式容器(list/set/map/unordered_*)不成立 ——
//         元素一旦配置在某個節點裡,除非被刪除,否則位址終生不變。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <string>
#include <algorithm>
#include <utility>
using namespace std;

// ================================================================
// 重點一：序列容器 —— array（固定大小陣列）
// ================================================================
// std::array：大小在編譯期固定，無法動態增減
// 優點：零額外開銷，與 C 陣列效能相同，但更安全（有 at()、size()）
// 適用：大小固定、效能敏感的場景

void demoArray() {
    cout << "\n--- std::array ---" << endl;

    array<int, 5> arr = {10, 20, 30, 40, 50};

    cout << "大小: " << arr.size() << endl;
    cout << "第一個: " << arr.front() << endl;
    cout << "最後一個: " << arr.back() << endl;
    cout << "arr[2]: " << arr[2] << endl;
    // arr.at(10);  // 會拋出 out_of_range 例外
    cout << "元素: ";
    for (int n : arr) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點二：序列容器 —— vector（動態陣列）
// ================================================================
// std::vector：最常用的容器，動態大小，連續記憶體
// 隨機存取 O(1)；尾端新增 O(1)；中間插入/刪除 O(n)

void demoVector() {
    cout << "\n--- std::vector ---" << endl;

    vector<int> v = {1, 2, 3};
    v.push_back(4);          // 尾端新增
    v.push_back(5);

    cout << "size=" << v.size() << ", capacity=" << v.capacity() << endl;
    cout << "元素: ";
    for (int n : v) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點三：序列容器 —— deque（雙端佇列）
// ================================================================
// std::deque：頭尾都能 O(1) 新增/刪除，不保證連續記憶體
// push_front() 是 vector 沒有的功能

void demoDeque() {
    cout << "\n--- std::deque ---" << endl;

    deque<int> dq = {3, 4, 5};
    dq.push_front(2);   // 頭部插入 O(1)
    dq.push_front(1);
    dq.push_back(6);    // 尾部插入 O(1)

    cout << "元素: ";
    for (int n : dq) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點四：序列容器 —— list（雙向鏈結串列）
// ================================================================
// std::list：任意位置插入/刪除 O(1)，但隨機存取 O(n)
// 沒有 operator[]，只能用迭代器遍歷

void demoList() {
    cout << "\n--- std::list ---" << endl;

    list<int> lst = {1, 3, 5, 7};
    lst.push_front(0);     // 頭部 O(1)
    lst.push_back(9);      // 尾部 O(1)

    // 在迭代器位置插入
    auto it = lst.begin();
    advance(it, 2);        // 移動到第 3 個
    lst.insert(it, 100);   // 任意位置插入 O(1)

    cout << "元素: ";
    for (int n : lst) cout << n << " ";
    cout << endl;

    lst.sort();            // list 有自己的 sort（不能用 std::sort）
    cout << "排序後: ";
    for (int n : lst) cout << n << " ";
    cout << endl;
}

// ================================================================
// 重點五：關聯容器 —— set（有序不重複集合）
// ================================================================
// std::set：自動排序，不允許重複元素，用紅黑樹實作（實作定義）
// 查找/插入/刪除：O(log n)

void demoSet() {
    cout << "\n--- std::set ---" << endl;

    set<int> s = {5, 3, 8, 1, 3, 5};  // 重複的 3、5 只保留一個
    cout << "set（自動排序，去重）: ";
    for (int n : s) cout << n << " ";
    cout << endl;

    // 查找
    auto it = s.find(3);
    cout << "找到 3: " << (it != s.end() ? "是" : "否") << endl;

    // count：在 set 中只有 0 或 1
    cout << "8 的數量: " << s.count(8) << endl;
    cout << "9 的數量: " << s.count(9) << endl;
}

// ================================================================
// 重點六：關聯容器 —— map（有序鍵值對）
// ================================================================
// std::map：key-value 對，按 key 自動排序，key 唯一
// 存取：m[key]（可能自動插入！）；安全存取用 m.at(key)

void demoMap() {
    cout << "\n--- std::map ---" << endl;

    map<string, int> scores;
    scores["Alice"] = 95;
    scores["Bob"] = 87;
    scores["Carol"] = 92;

    // 有序輸出（按 key 字母順序）
    cout << "成績（有序）:" << endl;
    for (const auto& [name, score] : scores) {  // C++17 結構化綁定
        cout << "  " << name << ": " << score << endl;
    }

    // find 比 [] 安全（不會自動插入不存在的 key）
    auto it = scores.find("Dave");
    cout << "找到 Dave: " << (it != scores.end() ? "是" : "否") << endl;
}

// ================================================================
// 重點七：無序關聯容器 —— unordered_map
// ================================================================
// std::unordered_map：用雜湊表實作，平均 O(1) 存取
// 不保證順序，但速度比 map 快（平均）

void demoUnorderedMap() {
    cout << "\n--- std::unordered_map ---" << endl;

    unordered_map<string, int> freq;
    vector<string> words = {"apple", "banana", "apple", "cherry", "banana", "apple"};

    for (const string& w : words) {
        freq[w]++;  // 統計詞頻
    }

    // 注意：unordered_map 的走訪順序未由標準規定。為了讓教學輸出穩定，
    // 這裡先複製到 vector 排序後再輸出——實務上也應該這樣做。
    vector<pair<string, int>> sorted(freq.begin(), freq.end());
    sort(sorted.begin(), sorted.end());

    cout << "詞頻（已排序輸出，原容器無序）:" << endl;
    for (const auto& [word, count] : sorted) {
        cout << "  " << word << ": " << count << endl;
    }
}

// ================================================================
// 重點八：容器配接器 —— stack, queue, priority_queue
// ================================================================
// 配接器不是獨立容器，而是對現有容器的包裝，限制存取方式
// stack    = LIFO（後進先出）；預設用 deque 實作
// queue    = FIFO（先進先出）；預設用 deque 實作
// priority_queue = 最大堆；預設用 vector 實作（heap 演算法需要隨機存取）

void demoAdapters() {
    cout << "\n--- 容器配接器 ---" << endl;

    // stack（LIFO）
    stack<int> stk;
    stk.push(1); stk.push(2); stk.push(3);
    cout << "stack top: " << stk.top() << endl;  // 3
    stk.pop();
    cout << "pop 後 top: " << stk.top() << endl; // 2

    // queue（FIFO）
    queue<int> q;
    q.push(10); q.push(20); q.push(30);
    cout << "queue front: " << q.front() << endl; // 10
    q.pop();
    cout << "pop 後 front: " << q.front() << endl; // 20

    // priority_queue（最大堆）
    priority_queue<int> pq;
    pq.push(5); pq.push(1); pq.push(8); pq.push(3);
    cout << "priority_queue top: " << pq.top() << endl; // 8（最大）
}

// ================================================================
// 重點九：容器物件本身的大小（本機實測，皆為實作定義）
// ================================================================
// 這組數字說明「容器物件」和「它管理的元素」是兩件事。
// forward_list 只有一個指標大小，正是它放棄 size() 換來的。

void demoSizeof() {
    cout << "\n--- 容器物件本身的大小（實作定義）---" << endl;
    cout << "  array<int,5>        : " << sizeof(array<int, 5>) << endl;
    cout << "  vector<int>         : " << sizeof(vector<int>) << endl;
    cout << "  deque<int>          : " << sizeof(deque<int>) << endl;
    cout << "  list<int>           : " << sizeof(list<int>) << endl;
    cout << "  forward_list<int>   : " << sizeof(forward_list<int>) << endl;
    cout << "  set<int>            : " << sizeof(set<int>) << endl;
    cout << "  map<int,int>        : " << sizeof(map<int, int>) << endl;
    cout << "  unordered_set<int>  : " << sizeof(unordered_set<int>) << endl;
    cout << "  （不含 heap 上的元素；換編譯器/標準庫可能不同）" << endl;
}

// ================================================================
// 重點十：vector 的容量成長 —— 攤銷 O(1) 的實證
// ================================================================
// 本機 libstdc++ 的成長倍率是 2（實作定義；MSVC 約 1.5）。
// 觀察 capacity 只在 2 的冪次跳躍，正是「倍數成長 → 攤銷常數」的來源。

void demoGrowth() {
    cout << "\n--- vector 容量成長（成長倍率為實作定義）---" << endl;
    vector<int> v;
    size_t cap = v.capacity();
    cout << "  初始 capacity = " << cap << endl;
    for (int i = 0; i < 40; ++i) {
        v.push_back(i);
        if (v.capacity() != cap) {
            cap = v.capacity();
            cout << "  size=" << v.size() << " 時 capacity 跳到 " << cap << endl;
        }
    }
}

// ================================================================
// 重點十一：iterator 失效 —— 節點式 vs 連續式的關鍵差異
// ================================================================
// 用「元素位址是否改變」來直接觀察，比背表更有感。
// 這裡只印布林值（是否相同），不印位址本身，以確保輸出可重現。

void demoIteratorStability() {
    cout << "\n--- iterator / reference 穩定性 ---" << endl;

    // vector：重新配置後元素被整批搬移，舊位址失效
    vector<int> v;
    v.reserve(2);
    v.push_back(1);
    const int* before = &v[0];
    for (int i = 0; i < 100; ++i) v.push_back(i);   // 必定觸發重新配置
    const int* after = &v[0];
    cout << "  vector 重新配置後，首元素位址是否不變: "
         << (before == after ? "是" : "否（元素被搬移了）") << endl;

    // list：節點配置後就不再移動
    list<int> l;
    l.push_back(1);
    const int* lbefore = &l.front();
    for (int i = 0; i < 100; ++i) l.push_back(i);
    const int* lafter = &l.front();
    cout << "  list 大量插入後，首元素位址是否不變: "
         << (lbefore == lafter ? "是（節點不搬移）" : "否") << endl;

    // unordered_set：rehash 讓 iterator 失效，但元素位址不變
    unordered_set<int> us;
    us.insert(1);
    const int* ubefore = &(*us.find(1));
    for (int i = 2; i < 500; ++i) us.insert(i);      // 必定觸發多次 rehash
    const int* uafter = &(*us.find(1));
    cout << "  unordered_set rehash 後，元素位址是否不變: "
         << (ubefore == uafter ? "是（只有 iterator 失效）" : "否") << endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 347. Top K Frequent Elements
//   題目：給一個整數陣列，回傳出現次數前 k 高的元素。
//   為什麼用到本主題：這題是「選容器」的最佳示範——一題之內要用到兩種不同
//     分類的容器，而且各自的理由都很明確：
//       * 統計次數 → unordered_map（只需要查找與累加，不需要順序 → 平均 O(1)）
//       * 取前 k 名 → priority_queue（只要最大的 k 個，不需要全排序）
//     用 min-heap 並維持大小為 k，可把複雜度壓到 O(n log k) 而非 O(n log n)。
//   複雜度：時間 O(n log k)，空間 O(n)。
// -----------------------------------------------------------------------------
vector<int> topKFrequent(const vector<int>& nums, int k) {
    // 步驟一：用 unordered_map 統計頻率（不需要順序，選無序容器）
    unordered_map<int, int> freq;
    for (int n : nums) ++freq[n];

    // 步驟二：min-heap 維持「目前最強的 k 個」。堆頂是這 k 個裡最弱的，
    //         一旦超過 k 就把堆頂丟掉——這樣堆永遠只有 k 個元素。
    using Item = pair<int, int>;                    // (次數, 數值)
    priority_queue<Item, vector<Item>, greater<Item>> minHeap;

    for (const auto& [value, count] : freq) {
        minHeap.emplace(count, value);
        if (static_cast<int>(minHeap.size()) > k) minHeap.pop();
    }

    // 步驟三：倒出來。堆是由小到大彈出，所以反轉後才是「次數由高到低」。
    vector<int> result;
    while (!minHeap.empty()) {
        result.push_back(minHeap.top().second);
        minHeap.pop();
    }
    reverse(result.begin(), result.end());
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】伺服器存取日誌（access log）分析
//   情境：讀一批 nginx 風格的存取日誌，要回答四個典型的維運問題：
//     1. 總共有哪些不重複的來源 IP？（且希望輸出是排序好的）
//     2. 各個 HTTP 狀態碼各出現幾次？（且希望按狀態碼由小到大呈現）
//     3. 流量最高的前 N 個路徑是哪些？
//     4. 最近 K 筆請求是什麼？（滑動視窗）
//   為什麼用到本主題：這四個問題剛好各自指向一種容器分類，是「依需求選容器」
//     最真實的展示：
//       1 → set          （要去重，而且要有序 → 有序關聯容器）
//       2 → map          （key-value，且要按 key 有序輸出）
//       3 → unordered_map 統計 + priority_queue 取 top-N
//       4 → deque        （固定長度滑動視窗，頭尾都要 O(1) 增刪）
// -----------------------------------------------------------------------------
struct LogEntry {
    string ip;
    string path;
    int    status;
};

void analyzeAccessLog(const vector<LogEntry>& entries, size_t windowSize) {
    // 問題 1：不重複來源 IP —— 需要去重 + 有序 → set
    set<string> uniqueIps;
    for (const auto& e : entries) uniqueIps.insert(e.ip);

    cout << "  不重複來源 IP（" << uniqueIps.size() << " 個，已排序）:" << endl;
    for (const auto& ip : uniqueIps) cout << "    " << ip << endl;

    // 問題 2：狀態碼分布 —— key-value 且要按 key 有序 → map
    map<int, int> statusCount;
    for (const auto& e : entries) ++statusCount[e.status];

    cout << "  狀態碼分布（按狀態碼排序）:" << endl;
    for (const auto& [status, count] : statusCount) {
        cout << "    " << status << " : " << count << " 次" << endl;
    }

    // 問題 3：熱門路徑 top 2 —— 統計不需順序(unordered_map)，取前 N 用 heap
    unordered_map<string, int> pathHits;
    for (const auto& e : entries) ++pathHits[e.path];

    using Hit = pair<int, string>;
    priority_queue<Hit> maxHeap;                     // 預設 max-heap
    for (const auto& [path, hits] : pathHits) maxHeap.emplace(hits, path);

    cout << "  熱門路徑 top 2:" << endl;
    for (int i = 0; i < 2 && !maxHeap.empty(); ++i) {
        cout << "    " << maxHeap.top().second
             << " (" << maxHeap.top().first << " 次)" << endl;
        maxHeap.pop();
    }

    // 問題 4：最近 K 筆請求 —— 固定長度滑動視窗，頭尾都要 O(1) → deque
    deque<string> recent;
    for (const auto& e : entries) {
        recent.push_back(e.path);
        if (recent.size() > windowSize) recent.pop_front();   // 尾進頭出
    }

    cout << "  最近 " << windowSize << " 筆請求路徑:" << endl;
    for (const auto& p : recent) cout << "    " << p << endl;
}

// ================================================================
// 重點十二：如何選擇容器
// ================================================================
//
// ┌─────────────────┬──────────────────────────────────────────┐
// │ 容器             │ 最佳使用場景                              │
// ├─────────────────┼──────────────────────────────────────────┤
// │ array           │ 大小固定，零開銷                          │
// │ vector          │ 一般用途，尾端增刪頻繁，需要隨機存取      │
// │ deque           │ 頭尾都需要頻繁增刪                        │
// │ list            │ 中間頻繁插入/刪除，且已持有迭代器        │
// │ set             │ 需要自動排序且不重複的集合               │
// │ map             │ 需要 key-value 對應，且有序               │
// │ unordered_map   │ key-value 對應，追求最快查找速度         │
// │ stack           │ 需要 LIFO 行為                           │
// │ queue           │ 需要 FIFO 行為                           │
// │ priority_queue  │ 需要每次取最大（或最小）元素             │
// └─────────────────┴──────────────────────────────────────────┘

int main() {
    cout << "=========================================" << endl;
    cout << "   第 6 課：STL 容器概念與分類展示" << endl;
    cout << "=========================================" << endl;

    demoArray();
    demoVector();
    demoDeque();
    demoList();
    demoSet();
    demoMap();
    demoUnorderedMap();
    demoAdapters();

    // ── 進階觀察：記憶體、成長、穩定性 ────────────────────────────────────
    demoSizeof();
    demoGrowth();
    demoIteratorStability();

    // ── LeetCode 347 ──────────────────────────────────────────────────────
    cout << "\n=== LeetCode 347. Top K Frequent Elements ===" << endl;
    {
        vector<int> nums = {1, 1, 1, 2, 2, 3};
        auto top = topKFrequent(nums, 2);
        cout << "  nums = {1,1,1,2,2,3}, k = 2 → ";
        for (int n : top) cout << n << " ";
        cout << endl;

        vector<int> nums2 = {4, 4, 4, 5, 5, 6, 6, 6, 6};
        auto top2 = topKFrequent(nums2, 2);
        cout << "  nums = {4,4,4,5,5,6,6,6,6}, k = 2 → ";
        for (int n : top2) cout << n << " ";
        cout << endl;
    }

    // ── 日常實務：存取日誌分析 ────────────────────────────────────────────
    cout << "\n=== 日常實務: 伺服器存取日誌分析 ===" << endl;
    {
        vector<LogEntry> log = {
            {"10.0.0.7", "/index.html", 200},
            {"10.0.0.3", "/api/users",  200},
            {"10.0.0.7", "/api/users",  500},
            {"10.0.0.9", "/index.html", 200},
            {"10.0.0.3", "/api/users",  404},
            {"10.0.0.7", "/index.html", 200},
        };
        analyzeAccessLog(log, 3);
    }

    cout << "\n=========================================" << endl;
    cout << " 選容器口訣：" << endl;
    cout << " 一般用 vector；有序唯一用 set/map；" << endl;
    cout << " 快速查找用 unordered；" << endl;
    cout << " LIFO 用 stack；FIFO 用 queue" << endl;
    cout << " —— 但真正該問的是：要不要用鍵查找？要不要有序？在哪裡增刪？" << endl;
    cout << "=========================================" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// 注意：本檔輸出中，vector 的 capacity 成長序列與各容器的 sizeof 屬「實作定義」，
//       換編譯器或標準庫版本可能不同；unordered_map 的原始走訪順序未由標準規定，
//       故程式中已先排序後再輸出，以確保教學輸出穩定可重現。

// === 預期輸出 ===
// =========================================
//    第 6 課：STL 容器概念與分類展示
// =========================================
//
// --- std::array ---
// 大小: 5
// 第一個: 10
// 最後一個: 50
// arr[2]: 30
// 元素: 10 20 30 40 50
//
// --- std::vector ---
// size=5, capacity=6
// 元素: 1 2 3 4 5
//
// --- std::deque ---
// 元素: 1 2 3 4 5 6
//
// --- std::list ---
// 元素: 0 1 100 3 5 7 9
// 排序後: 0 1 3 5 7 9 100
//
// --- std::set ---
// set（自動排序，去重）: 1 3 5 8
// 找到 3: 是
// 8 的數量: 1
// 9 的數量: 0
//
// --- std::map ---
// 成績（有序）:
//   Alice: 95
//   Bob: 87
//   Carol: 92
// 找到 Dave: 否
//
// --- std::unordered_map ---
// 詞頻（已排序輸出，原容器無序）:
//   apple: 3
//   banana: 2
//   cherry: 1
//
// --- 容器配接器 ---
// stack top: 3
// pop 後 top: 2
// queue front: 10
// pop 後 front: 20
// priority_queue top: 8
//
// --- 容器物件本身的大小（實作定義）---
//   array<int,5>        : 20
//   vector<int>         : 24
//   deque<int>          : 80
//   list<int>           : 24
//   forward_list<int>   : 8
//   set<int>            : 48
//   map<int,int>        : 48
//   unordered_set<int>  : 56
//   （不含 heap 上的元素；換編譯器/標準庫可能不同）
//
// --- vector 容量成長（成長倍率為實作定義）---
//   初始 capacity = 0
//   size=1 時 capacity 跳到 1
//   size=2 時 capacity 跳到 2
//   size=3 時 capacity 跳到 4
//   size=5 時 capacity 跳到 8
//   size=9 時 capacity 跳到 16
//   size=17 時 capacity 跳到 32
//   size=33 時 capacity 跳到 64
//
// --- iterator / reference 穩定性 ---
//   vector 重新配置後，首元素位址是否不變: 否（元素被搬移了）
//   list 大量插入後，首元素位址是否不變: 是（節點不搬移）
//   unordered_set rehash 後，元素位址是否不變: 是（只有 iterator 失效）
//
// === LeetCode 347. Top K Frequent Elements ===
//   nums = {1,1,1,2,2,3}, k = 2 → 1 2
//   nums = {4,4,4,5,5,6,6,6,6}, k = 2 → 6 4
//
// === 日常實務: 伺服器存取日誌分析 ===
//   不重複來源 IP（3 個，已排序）:
//     10.0.0.3
//     10.0.0.7
//     10.0.0.9
//   狀態碼分布（按狀態碼排序）:
//     200 : 4 次
//     404 : 1 次
//     500 : 1 次
//   熱門路徑 top 2:
//     /index.html (3 次)
//     /api/users (3 次)
//   最近 3 筆請求路徑:
//     /index.html
//     /api/users
//     /index.html
//
// =========================================
//  選容器口訣：
//  一般用 vector；有序唯一用 set/map；
//  快速查找用 unordered；
//  LIFO 用 stack；FIFO 用 queue
//  —— 但真正該問的是：要不要用鍵查找？要不要有序？在哪裡增刪？
// =========================================
