// =============================================================================
//  第六課 9 — std::multimap：一個鍵對應多個值的有序關聯容器
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key, class T,
//            class Compare = std::less<Key>,
//            class Allocator = std::allocator<std::pair<const Key, T>>>
//   class multimap;                                   // <map>,C++98 起
//
//   常用介面：
//     iterator insert(const value_type& v);           // O(log n),恆成功,不會失敗
//     iterator emplace(Args&&...);                    // O(log n),C++11
//     size_type count(const Key& k) const;            // O(log n + k),k = 命中筆數
//     std::pair<iterator,iterator> equal_range(const Key& k);   // O(log n) ★主要存取方式
//     iterator lower_bound(const Key& k);             // O(log n)
//     iterator upper_bound(const Key& k);             // O(log n)
//     size_type erase(const Key& k);                  // O(log n + k),刪除**全部**同鍵元素
//     iterator  erase(iterator pos);                  // 攤提 O(1),只刪這一個
//     bool contains(const Key& k) const;              // C++20
//
//   **沒有** operator[]、**沒有** at()、**沒有** insert_or_assign / try_emplace。
//   原因見【2】—— 這不是遺漏,是邏輯上根本無法定義。
//
//   複雜度：查找/插入/刪除單筆皆 O(log n);走訪已排序序列 O(n)。
//   標頭檔：#include <map>（multimap 與 map 同一個標頭檔,不是 <multimap>）
//
// 【詳細解釋 Explanation】
//
// 【1. multimap 解決什麼問題:「鍵不唯一」是真實世界的常態】
// std::map 有一條硬性約束:一個 key 只能有一個 value。這在很多情境下是對的
// (使用者 ID → 使用者資料),但真實世界裡「一對多」其實更常見:
//
//     一個人有多支電話          (本檔的示範)
//     一個 HTTP 回應有多個 Set-Cookie header
//     一個事件有多個訂閱者 (event → handlers)
//     一個作者寫過多本書
//     一個時間戳有多筆同時發生的 log
//
// 遇到這種需求,你有兩條路:
//   (a) std::map<Key, std::vector<Value>>  —— 值端自己裝一個容器
//   (b) std::multimap<Key, Value>          —— 容器本身允許重複鍵
//
// 兩者不是誰取代誰。差別在於「你要不要把同一個 key 的那組值當成一個整體來操作」:
//   * 若你常整組取出、整組覆寫、或要知道「這個 key 有沒有出現過」(即使值是空的),
//     用 map<K, vector<V>> 比較自然 —— 它有 operator[],空 vector 也是合法狀態。
//   * 若你只是不斷「再加一筆」、查詢時想拿到扁平的一串 (key, value) pair、
//     而且希望所有元素躺在同一棵樹上按 key 全域排序,用 multimap 更直接 ——
//     不必先找到 vector 再 push_back,也不會有「vector 存在但為空」的中間狀態。
//
// multimap 的插入還有一個 map 沒有的性質:**insert 永遠成功**。map 的 insert
// 回傳 pair<iterator,bool>,那個 bool 是用來告訴你「鍵已存在,我沒有插入」;
// multimap 沒有這種情況,所以它的 insert 直接回傳 iterator,連 bool 都不需要。
//
// 【2. 為什麼 multimap 沒有 operator[]:這是邏輯問題,不是實作偷懶】
// 這是本主題最常見的面試題。答案很單純:**沒有一個合理的語意可以定義它**。
//
// map 的 m[k] 語意是「回傳 k 對應的那個值的 reference,不存在就預設建構一個」。
// 這個定義能成立,完全建立在「k 最多只有一個值」這個前提上。到了 multimap:
//
//     phonebook["Alice"]   // Alice 有三支電話 —— 要回傳哪一支的 reference?
//
// 第一支?最後一支?任選一支?每個選擇都是武斷的,而且無論選哪個都會讓
// 「賦值」變得荒謬:phonebook["Alice"] = "0900-000-000" 到底是**改掉其中一支**,
// 還是**再新增一支**,還是**把三支全部覆蓋成一支**?三種解讀都說得通,
// 這正是標準乾脆不提供它的原因 —— 一個無法給出唯一正確解釋的介面,不如不給。
//
// 同樣的道理,at()、insert_or_assign()、try_emplace() 這些「以鍵唯一為前提」
// 的介面在 multimap 全部缺席。取而代之的主要存取方式就是下一節的 equal_range。
//
// 【3. equal_range:multimap 的正字標記存取方式】
// 既然一個 key 對應「一段」元素,存取的單位自然就從「一個位置」變成「一段區間」。
// equal_range(k) 回傳一個 pair<iterator,iterator>,標出所有 key 等價於 k 的
// 半開區間 [first, second)：
//
//     auto range = phonebook.equal_range("Alice");
//     for (auto it = range.first; it != range.second; ++it)
//         use(it->second);
//
// 為什麼這樣可行?因為 multimap 底層是**依 key 排序的平衡二元搜尋樹**(紅黑樹),
// 相同 key 的元素在中序走訪下必然**連續相鄰**。所以「所有等於 k 的元素」一定
// 是一段連續區間,可以用兩個 iterator 表示,不需要回傳一個 vector。
//
// equal_range 本質上等價於 { lower_bound(k), upper_bound(k) },但只走一次樹,
// 所以比分別呼叫兩次省一半。三者複雜度都是 O(log n)。
//
// 若 key 不存在,equal_range 回傳的兩個 iterator **相等**(空區間),迴圈自然
// 一次都不會執行 —— 不需要另外寫 if (count(k) > 0) 先判斷,那是多餘的一次查找。
//
// 【4. count() 的複雜度是 O(log n + k),不是 O(log n)】
// 很多人以為 count() 跟 map 一樣是 O(log n)。在 map 上是的(結果只可能是 0 或 1);
// 但在 multimap 上,它必須**實際數過**那段區間有幾個元素,所以是 O(log n + k),
// 其中 k 是命中的筆數。
//
// 實務上這代表:如果你只是想知道「有沒有」,不要寫 count(k) > 0 ——
// 那會白白走完整段區間。正確寫法是:
//     if (mm.find(k) != mm.end())     // O(log n),找到一個就停
//     if (mm.contains(k))             // C++20,語意最清楚
//
// 【5. 等價(equivalence)不是相等(equality) —— 關聯容器的核心概念】
// 這是所有 STL 有序關聯容器(set/map/multiset/multimap)共通、卻最常被忽略的一點。
//
// multimap 從頭到尾**只用 Compare(預設 std::less<Key>)**,它從不呼叫 operator==。
// 兩個 key 被視為「同一個 key」的判準是:
//
//     !comp(a, b) && !comp(b, a)      ← 這叫 equivalence(等價)
//
// 而不是:
//
//     a == b                          ← 這叫 equality(相等)
//
// 對 int、std::string 這種「排序與相等一致」的型別,兩者結果相同,所以你感覺不到
// 差別。但只要你換一個自訂比較器,差異立刻現形。本檔的 HTTP header 實務範例
// 就刻意示範了這點:用一個**忽略大小寫**的比較器之後,
//
//     "Set-Cookie" == "set-cookie"    → false (兩個字串並不相等)
//     等價嗎?                          → true  (比較器認為誰都不小於誰)
//
// 於是 equal_range("set-cookie") 會**找到**當初以 "Set-Cookie" 插入的那些元素。
// 這正是 HTTP header 名稱大小寫不敏感(RFC 7230)的正確實作方式,也是理解
// 「等價 vs 相等」最實際的例子。
//
// 【6. 同鍵元素的順序:C++11 起才有保證】
// 「Alice 的三支電話,取出來的順序是我插入的順序嗎?」
//
//   * C++98/03：**不保證**。標準沒有規定同鍵元素的相對順序。
//   * C++11 起：**保證**。標準明訂等價元素維持**插入順序**(insertion order),
//     亦即新插入的元素會放在既有等價元素的**後面**(upper_bound 的位置)。
//
// 所以在 C++11 之後,multimap 可以安全地當作「保序的一對多表格」使用 ——
// 本檔的電話簿示範中,Alice 的三支號碼就是照插入順序印出來的。
// 注意這個保證只涵蓋 insert/emplace;若你用 emplace_hint 給了錯誤的 hint,
// 元素仍會被放到正確的等價區間內,但順序以標準規定為準。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:和 map 是同一棵樹,一個 byte 都不差
//     libstdc++ 的 multimap 內部就是一個 _Rb_tree(紅黑樹),與 map 用的是
//     **同一份模板實作**,差別只在傳給它的插入函式不同:
//         map      → _M_insert_unique()      (鍵重複就拒絕)
//         multimap → _M_insert_equal()       (鍵重複照插)
//     所以本機實測 sizeof(std::multimap<int,int>) == 48 == sizeof(std::map<int,int>)
//     (**實作定義**;libstdc++ 的樹頭含 comparator、header node 的三個指標與
//     節點計數)。注意這個 48 與 Key/T 的實際型別無關 —— 元素都在堆積(heap)上的
//     節點裡,容器本體只存樹的骨架,所以 multimap<string,string> 也是 48。
//
//     每個元素是一個獨立配置的節點,內含:
//         [顏色][父指標][左指標][右指標][ std::pair<const Key, T> ]
//     三個指標 + 顏色標記 = 每個元素約 32 bytes 的固定額外開銷(實作定義)。
//     這就是關聯容器記憶體效率遠不如 vector 的原因。
//
// (B) 為什麼同鍵元素必然連續:中序走訪的直接推論
//     紅黑樹的中序走訪(in-order traversal)產生的是**依 Compare 排序的序列**。
//     在排序序列中,所有等價的元素當然會擠在一起 —— 不可能出現
//     "Alice", "Bob", "Alice" 這種被隔開的排列,否則序列就不是排序的了。
//     equal_range 能只用兩個 iterator 表達「全部命中結果」,靠的就是這個性質。
//     (對照組:unordered_multimap 沒有排序,但它靠「同一個 bucket 內把等價元素
//      串在一起」達成同樣的連續性,所以 equal_range 同樣可用。)
//
// (C) erase(key) 與 erase(iterator) 是兩個完全不同的操作
//     這是 multimap 最容易釀成資料損毀的地方,值得單獨拿出來講:
//         mm.erase("Alice");        // 刪掉 Alice 的**全部** 3 筆,回傳 3
//         mm.erase(it);             // 只刪掉 it 指的那**一**筆,回傳下一個 iterator
//     在 map 上這兩者的效果剛好一樣(因為最多只有一筆),所以從 map 換到 multimap
//     的人特別容易踩到:原本「刪掉這個 key」的程式碼,語意在不知不覺間從
//     「刪一筆」變成「刪一整組」。
//     要刪特定的某一筆,標準做法是先 equal_range 找到它再 erase(iterator)。
//
// (D) 刪除時的 iterator 失效規則:比 vector 寬鬆得多
//     紅黑樹的節點不會因為別處的插入/刪除而搬家,所以:
//         * insert 從不使任何 iterator/reference 失效。
//         * erase **只**使被刪除元素自己的 iterator/reference 失效,其餘全部有效。
//     這讓「邊走訪邊刪除」成為安全操作,但必須用正確寫法(C++11 起 erase 回傳
//     下一個 iterator):
//         for (auto it = mm.begin(); it != mm.end(); )
//             if (pred(*it)) it = mm.erase(it);   // ← 用回傳值續走
//             else           ++it;
//     絕不能寫 mm.erase(it); ++it; —— it 在 erase 後已失效,再對它 ++ 是
//     undefined behavior。
//
// 【注意事項 Pay Attention】
//  1. multimap **沒有** operator[] 與 at()。這是刻意的設計(見【2】),不要試圖
//     用 mm[k] 存取 —— 那是編譯錯誤,不是執行期問題。
//  2. erase(key) 刪除**所有**等價元素並回傳刪除筆數;erase(iterator) 只刪一筆。
//     從 map 移植過來的程式碼務必逐一檢查每個 erase 的語意。
//  3. count(k) 是 O(log n + k),不是 O(log n)。只想判斷存在性請用 find()
//     或 C++20 的 contains(),不要用 count(k) > 0。
//  4. 「同一個 key」的判準是**等價**(!(a<b) && !(b<a)),不是 operator==。
//     自訂比較器時,若比較器與 operator== 不一致(如本檔的大小寫不敏感比較器),
//     兩者結果會不同 —— 容器一律以比較器為準。
//  5. 比較器必須是 **strict weak ordering**。若你寫了一個 comp(a,a) 回傳 true
//     的比較器(例如誤用 <=),行為是 undefined —— 不保證是崩潰,也可能是靜默的
//     資料錯亂或無窮迴圈,而且通常在資料量變大後才顯現。
//  6. 同鍵元素維持插入順序的保證是 **C++11 起**才有的。若你在維護必須相容
//     C++03 的老程式碼,不能依賴這個順序。
//  7. 不要對 equal_range 回傳的區間做 range-based for —— 那需要一個具備
//     begin()/end() 的物件,而 std::pair 沒有。請用傳統三段式 for 迴圈,
//     或 C++20 的 std::ranges::subrange 包一層。
//  8. 走訪順序由 **key 的排序**決定,不是插入順序。只有「同一個 key 內部」
//     才依插入順序。想要全域維持插入順序,multimap 不是正確的工具。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::multimap
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::multimap 為什麼沒有 operator[]?
//     答：因為它沒有唯一合理的語意。map 的 m[k] 建立在「一個 key 最多一個值」
//         之上,才能回傳「那個值」的 reference。multimap 的一個 key 可能對應
//         多筆,m[k] 要回傳第一筆、最後一筆、還是任選一筆?更麻煩的是賦值:
//         m[k] = v 到底是改掉其中一筆、新增一筆、還是把整組覆蓋成一筆?
//         每種解讀都說得通 —— 沒有唯一正確答案的介面,標準選擇不提供。
//         取而代之的存取方式是 equal_range(k),回傳整段命中區間。
//     追問：那 at() 和 insert_or_assign 呢?
//         → 同樣缺席,理由完全一樣:它們都以「鍵唯一」為前提。
//           multimap 的插入一律用 insert/emplace,而且**永遠成功**,
//           所以 insert 直接回傳 iterator,不像 map 要回傳 pair<iterator,bool>。
//
// 🔥 Q2. 要取出 multimap 中某個 key 的所有值,正確做法是什麼?為什麼可行?
//     答：用 equal_range(k),它回傳 pair<iterator,iterator> 表示半開區間
//         [first, second),走訪這段區間即可。之所以能用「一段區間」表示全部
//         命中結果,是因為 multimap 底層是依 key 排序的紅黑樹,中序走訪下
//         等價的 key 必然**連續相鄰**。equal_range 等價於一次同時算出
//         lower_bound 與 upper_bound,只走一次樹,O(log n)。
//         key 不存在時兩個 iterator 相等,迴圈自然不執行,不必先判斷存在性。
//     追問：那 count(k) 的複雜度是多少?
//         → O(log n + k),k 是命中筆數 —— 它必須真的數過那段區間。
//           所以只想判斷「有沒有」時應該用 find() 或 C++20 的 contains(),
//           寫 count(k) > 0 會白白走完整段區間。
//
// ⚠️ 陷阱. phonebook.erase("Alice") 之後,Alice 還剩幾筆資料?
//     答：0 筆。erase(key) 會刪除**所有**等價於該 key 的元素,並回傳刪除的
//         筆數(此例為 3)。若只想刪掉其中一筆,必須先用 equal_range 或 find
//         定位到那一筆,再呼叫 erase(iterator) —— 那個多載只刪一個元素。
//     為什麼會錯：多數人是從 std::map 遷移過來的,而在 map 上
//         erase(key) 和 erase(iterator) 的效果剛好一樣(因為最多就一筆),
//         腦中因此建立了「erase 就是刪掉那一筆」的模型。換到 multimap 後,
//         同一行程式碼的語意在不知不覺間從「刪一筆」變成「刪一整組」,
//         而且編譯完全不會報錯 —— 這類 bug 通常要到資料異常才被發現。
//
// ⚠️ 陷阱. 自訂一個大小寫不敏感的比較器後,插入 "Set-Cookie",
//         再用 equal_range("set-cookie") 查詢,會找到嗎?
//     答：會找到。關聯容器判斷「是不是同一個 key」用的是**等價**
//         (!comp(a,b) && !comp(b,a)),不是 operator==。在大小寫不敏感的
//         比較器下,"Set-Cookie" 與 "set-cookie" 誰都不小於誰 → 等價 → 命中。
//         儘管這兩個 std::string 用 == 比較是 false。
//     為什麼會錯：多數人心中的模型是「容器會拿 key 做 == 比對」。
//         但有序關聯容器**從頭到尾只呼叫 Compare,一次都不呼叫 operator==**。
//         沒認清這點,不只會誤判查詢結果,還可能寫出與 operator== 不一致的
//         比較器而不自知 —— 那會讓「相等的東西被當成兩個 key」或反過來。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>   // std::lexicographical_compare
#include <cctype>      // std::tolower

// -----------------------------------------------------------------------------
// 【日常實務範例 1】解析 HTTP 回應標頭(header)—— 合法重複的 header
//   情境：HTTP 規格允許同一個 header 名稱出現多次,最典型的就是 Set-Cookie ——
//     伺服器一次要種三個 cookie,就送三行 Set-Cookie。用 std::map 存會把前兩個
//     蓋掉,直接造成登入失敗這類真實 bug;這正是 multimap 的標準用途。
//   為什麼用到本主題：
//     (1) 一個 key(header 名)對應多個值 → multimap。
//     (2) HTTP header 名稱**大小寫不敏感**(RFC 7230 §3.2),所以比較器要忽略
//         大小寫 —— 順便示範「等價 vs 相等」:"Set-Cookie" 與 "set-cookie"
//         用 == 比是 false,但在這個比較器下**等價**,查詢照樣命中。
// -----------------------------------------------------------------------------
struct CaseInsensitiveLess {
    bool operator()(const std::string& a, const std::string& b) const {
        // 逐字元轉小寫後做字典序比較。注意 std::tolower 的參數必須先轉成
        // unsigned char,否則傳入負值的 char 是 undefined behavior。
        return std::lexicographical_compare(
            a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y) {
                return std::tolower(x) < std::tolower(y);
            });
    }
};

using HttpHeaders = std::multimap<std::string, std::string, CaseInsensitiveLess>;

HttpHeaders parseHeaders(const std::vector<std::string>& rawLines) {
    HttpHeaders headers;
    for (const std::string& line : rawLines) {
        std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;          // 格式不合就跳過

        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // 去掉 value 前導空白(HTTP 允許 "Name: value" 的那個空格)
        std::size_t start = value.find_first_not_of(" \t");
        value = (start == std::string::npos) ? std::string() : value.substr(start);

        headers.emplace(std::move(name), std::move(value));
    }
    return headers;
}

// 取出某個 header 的所有值(key 不存在就回傳空 vector)
std::vector<std::string> getAll(const HttpHeaders& h, const std::string& name) {
    std::vector<std::string> out;
    auto range = h.equal_range(name);                       // O(log n)
    for (auto it = range.first; it != range.second; ++it)   // key 不存在 → 空區間
        out.push_back(it->second);
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】事件訂閱表(event → handlers)—— 示範 erase 的兩種語意
//   情境：GUI、遊戲引擎、訊息匯流排都需要「一個事件名稱對應多個處理函式」。
//     訂閱時不斷 emplace,發事件時 equal_range 取出全部處理函式依序呼叫。
//   為什麼用到本主題：
//     (1) 天然的一對多,而且同一事件的 handler 要**依註冊順序**觸發 ——
//         C++11 起 multimap 保證等價元素維持插入順序,剛好滿足這個需求。
//     (2) 退訂剛好對應 multimap 兩種 erase:
//         「這個事件全部退訂」→ erase(key);「只退掉某一個 handler」→ erase(iterator)。
// -----------------------------------------------------------------------------
using Handler   = std::function<void(const std::string&)>;
using EventBus  = std::multimap<std::string, std::pair<std::string, Handler>>;
//                                            ^^^^^^^^^ handler 名稱,方便示範與除錯

void emit(const EventBus& bus, const std::string& event, const std::string& payload) {
    auto range = bus.equal_range(event);
    if (range.first == range.second) {                      // 空區間 = 沒人訂閱
        std::cout << "  [" << event << "] 沒有訂閱者" << std::endl;
        return;
    }
    for (auto it = range.first; it != range.second; ++it)
        it->second.second(payload);                         // 依註冊順序呼叫
}

// 只退訂「某事件的某一個具名 handler」→ 必須用 erase(iterator)
bool unsubscribeOne(EventBus& bus, const std::string& event, const std::string& handlerName) {
    auto range = bus.equal_range(event);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.first == handlerName) {
            bus.erase(it);          // ← 只刪這一個。若寫成 bus.erase(event) 會全滅
            return true;
        }
    }
    return false;
}

int main() {
    // ── 原始課堂示範:multimap 的基本操作 ──────────────────────────────────
    std::cout << "=== std::multimap ===" << std::endl;

    // 一個鍵可以對應多個值
    std::multimap<std::string, std::string> phonebook;

    phonebook.insert({"Alice", "0912-345-678"});
    phonebook.insert({"Alice", "02-1234-5678"});  // Alice 有兩個號碼
    phonebook.insert({"Bob", "0923-456-789"});
    phonebook.insert({"Alice", "03-9876-5432"});  // Alice 第三個號碼

    std::cout << "電話簿:" << std::endl;
    for (const auto& entry : phonebook) {
        std::cout << "  " << entry.first << ": " << entry.second << std::endl;
    }

    // 查找 Alice 的所有號碼
    // equal_range：取得所有等於某值的元素範圍
    // 這裡會回傳一個 pair，first 是第一個 Alice 的位置，second 是最後一個 Alice 的下一個位置
    // 注意：multimap 的元素是按照鍵排序的，所以所有 Alice 的號碼會連在一起
    std::cout << "\nAlice 的所有號碼:" << std::endl;
    auto range = phonebook.equal_range("Alice");
    for (auto it = range.first; it != range.second; ++it) {
        std::cout << "  " << it->second << std::endl;
    }

    std::cout << "\nAlice 有 " << phonebook.count("Alice") << " 個號碼" << std::endl;

    // ── 觀察 1:走訪順序 = key 排序,同 key 內才是插入順序 ──────────────────
    std::cout << "\n=== 走訪順序的兩層規則 ===" << std::endl;
    std::cout << "插入順序為 Alice / Alice / Bob / Alice,但走訪時 Bob 排在最後,"
              << std::endl;
    std::cout << "因為外層依 key 排序(Alice < Bob);Alice 的三支號碼則維持插入順序"
              << std::endl;
    std::cout << "(C++11 起標準保證等價元素維持插入順序)。" << std::endl;

    // ── 觀察 2:key 不存在時 equal_range 回傳空區間 ────────────────────────
    std::cout << "\n=== key 不存在時的 equal_range ===" << std::endl;
    auto none = phonebook.equal_range("Zoe");
    std::cout << "equal_range(\"Zoe\") 的兩個 iterator 相等嗎? "
              << std::boolalpha << (none.first == none.second) << std::endl;
    std::cout << "→ 空區間,迴圈一次都不會執行,不必先寫 if (count > 0)" << std::endl;

    // ── 觀察 3:count() 是 O(log n + k);判斷存在性該用 find() ──────────────
    std::cout << "\n=== count() vs find() ===" << std::endl;
    std::cout << "count(\"Alice\") = " << phonebook.count("Alice")
              << "  (O(log n + k),要真的數過整段區間)" << std::endl;
    std::cout << "find(\"Alice\") != end() = "
              << (phonebook.find("Alice") != phonebook.end())
              << "  (O(log n),找到一個就停 → 判斷存在性用這個)" << std::endl;

    // ── 觀察 4:erase(key) 與 erase(iterator) 的天壤之別 ───────────────────
    std::cout << "\n=== erase(key) vs erase(iterator) ===" << std::endl;
    {
        std::multimap<std::string, std::string> copy = phonebook;

        // (a) erase(iterator)：只刪一筆
        auto r = copy.equal_range("Alice");
        copy.erase(r.first);                       // 刪掉 Alice 的第一支
        std::cout << "erase(iterator) 後,Alice 剩 " << copy.count("Alice")
                  << " 筆 (原本 3 筆,只刪掉一筆)" << std::endl;

        // (b) erase(key)：全部刪光,回傳刪除筆數
        std::size_t removed = copy.erase("Alice");
        std::cout << "erase(\"Alice\")  回傳 " << removed
                  << ",Alice 剩 " << copy.count("Alice")
                  << " 筆 (整組刪光 —— 這是最經典的意外)" << std::endl;
        std::cout << "刪除後容器總筆數: " << copy.size()
                  << " (只剩 Bob)" << std::endl;
    }

    // ── 觀察 5:multimap 與 map 的記憶體骨架完全一樣 ───────────────────────
    std::cout << "\n=== 記憶體佈局(實作定義) ===" << std::endl;
    std::cout << "sizeof(std::map<int,int>)      = "
              << sizeof(std::map<int, int>) << std::endl;
    std::cout << "sizeof(std::multimap<int,int>) = "
              << sizeof(std::multimap<int, int>) << std::endl;
    std::cout << "→ 兩者共用同一份紅黑樹實作,差別只在插入時用 unique 還是 equal 版本"
              << std::endl;
    std::cout << "sizeof(std::multimap<string,string>) = "
              << sizeof(std::multimap<std::string, std::string>)
              << " (元素在 heap 節點裡,容器本體只存樹骨架,故大小與型別無關)"
              << std::endl;

    // ── 日常實務 1:HTTP header 解析 ───────────────────────────────────────
    std::cout << "\n=== 日常實務 1: HTTP 回應標頭解析 ===" << std::endl;
    std::vector<std::string> raw = {
        "Content-Type: text/html; charset=utf-8",
        "Set-Cookie: session=abc123; Path=/; HttpOnly",
        "Set-Cookie: theme=dark; Path=/; Max-Age=31536000",
        "Cache-Control: no-cache",
        "set-cookie: tracking=xyz789; Path=/",      // 故意用小寫,示範等價
        "Content-Length: 5120"
    };
    HttpHeaders headers = parseHeaders(raw);

    std::cout << "解析出 " << headers.size() << " 個 header 欄位" << std::endl;
    std::cout << "Set-Cookie 共 " << headers.count("Set-Cookie") << " 筆:" << std::endl;
    for (const std::string& v : getAll(headers, "Set-Cookie"))
        std::cout << "  - " << v << std::endl;

    std::cout << "以全小寫 \"set-cookie\" 查詢也命中 "
              << headers.count("set-cookie") << " 筆" << std::endl;
    std::cout << "但字串本身相等嗎? std::string(\"Set-Cookie\") == \"set-cookie\" → "
              << (std::string("Set-Cookie") == "set-cookie") << std::endl;
    std::cout << "→ 這就是「等價(equivalence)」與「相等(equality)」的差別:"
              << std::endl;
    std::cout << "   容器只用比較器判斷等價,從不呼叫 operator==" << std::endl;

    std::cout << "單值 header 照常取用: Content-Type = "
              << getAll(headers, "content-type").front() << std::endl;

    // ── 日常實務 2:事件訂閱表 ─────────────────────────────────────────────
    std::cout << "\n=== 日常實務 2: 事件訂閱表(event → handlers) ===" << std::endl;
    EventBus bus;
    bus.emplace("user.login", std::make_pair("audit", [](const std::string& u) {
        std::cout << "  [audit]   記錄登入: " << u << std::endl;
    }));
    bus.emplace("user.login", std::make_pair("metrics", [](const std::string& u) {
        std::cout << "  [metrics] 累加登入次數: " << u << std::endl;
    }));
    bus.emplace("user.login", std::make_pair("welcome", [](const std::string& u) {
        std::cout << "  [welcome] 寄送歡迎信給: " << u << std::endl;
    }));
    bus.emplace("user.logout", std::make_pair("audit", [](const std::string& u) {
        std::cout << "  [audit]   記錄登出: " << u << std::endl;
    }));

    std::cout << "emit(\"user.login\") — 依註冊順序觸發 "
              << bus.count("user.login") << " 個 handler:" << std::endl;
    emit(bus, "user.login", "alice@example.com");

    std::cout << "退訂 user.login 的 metrics(erase(iterator),只刪一個):" << std::endl;
    bool ok = unsubscribeOne(bus, "user.login", "metrics");
    std::cout << "  退訂成功? " << ok << ",剩下 "
              << bus.count("user.login") << " 個 handler" << std::endl;
    emit(bus, "user.login", "bob@example.com");

    std::cout << "整個事件全部退訂(erase(key),整組刪光):" << std::endl;
    std::size_t n = bus.erase("user.login");
    std::cout << "  移除了 " << n << " 個 handler" << std::endl;
    emit(bus, "user.login", "carol@example.com");
    std::cout << "user.logout 不受影響:" << std::endl;
    emit(bus, "user.logout", "alice@example.com");

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類9.cpp" -o multimap_demo

// === 預期輸出 ===
// === std::multimap ===
// 電話簿:
//   Alice: 0912-345-678
//   Alice: 02-1234-5678
//   Alice: 03-9876-5432
//   Bob: 0923-456-789
//
// Alice 的所有號碼:
//   0912-345-678
//   02-1234-5678
//   03-9876-5432
//
// Alice 有 3 個號碼
//
// === 走訪順序的兩層規則 ===
// 插入順序為 Alice / Alice / Bob / Alice,但走訪時 Bob 排在最後,
// 因為外層依 key 排序(Alice < Bob);Alice 的三支號碼則維持插入順序
// (C++11 起標準保證等價元素維持插入順序)。
//
// === key 不存在時的 equal_range ===
// equal_range("Zoe") 的兩個 iterator 相等嗎? true
// → 空區間,迴圈一次都不會執行,不必先寫 if (count > 0)
//
// === count() vs find() ===
// count("Alice") = 3  (O(log n + k),要真的數過整段區間)
// find("Alice") != end() = true  (O(log n),找到一個就停 → 判斷存在性用這個)
//
// === erase(key) vs erase(iterator) ===
// erase(iterator) 後,Alice 剩 2 筆 (原本 3 筆,只刪掉一筆)
// erase("Alice")  回傳 2,Alice 剩 0 筆 (整組刪光 —— 這是最經典的意外)
// 刪除後容器總筆數: 1 (只剩 Bob)
//
// === 記憶體佈局(實作定義) ===
// sizeof(std::map<int,int>)      = 48
// sizeof(std::multimap<int,int>) = 48
// → 兩者共用同一份紅黑樹實作,差別只在插入時用 unique 還是 equal 版本
// sizeof(std::multimap<string,string>) = 48 (元素在 heap 節點裡,容器本體只存樹骨架,故大小與型別無關)
//
// === 日常實務 1: HTTP 回應標頭解析 ===
// 解析出 6 個 header 欄位
// Set-Cookie 共 3 筆:
//   - session=abc123; Path=/; HttpOnly
//   - theme=dark; Path=/; Max-Age=31536000
//   - tracking=xyz789; Path=/
// 以全小寫 "set-cookie" 查詢也命中 3 筆
// 但字串本身相等嗎? std::string("Set-Cookie") == "set-cookie" → false
// → 這就是「等價(equivalence)」與「相等(equality)」的差別:
//    容器只用比較器判斷等價,從不呼叫 operator==
// 單值 header 照常取用: Content-Type = text/html; charset=utf-8
//
// === 日常實務 2: 事件訂閱表(event → handlers) ===
// emit("user.login") — 依註冊順序觸發 3 個 handler:
//   [audit]   記錄登入: alice@example.com
//   [metrics] 累加登入次數: alice@example.com
//   [welcome] 寄送歡迎信給: alice@example.com
// 退訂 user.login 的 metrics(erase(iterator),只刪一個):
//   退訂成功? true,剩下 2 個 handler
//   [audit]   記錄登入: bob@example.com
//   [welcome] 寄送歡迎信給: bob@example.com
// 整個事件全部退訂(erase(key),整組刪光):
//   移除了 2 個 handler
//   [user.login] 沒有訂閱者
// user.logout 不受影響:
//   [audit]   記錄登出: alice@example.com
