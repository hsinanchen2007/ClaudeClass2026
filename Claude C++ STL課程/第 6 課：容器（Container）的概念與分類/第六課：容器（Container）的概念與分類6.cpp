// =============================================================================
//  第六課 6 — std::set：有序、不重複、迭代器永不失效的關聯容器
// =============================================================================
//
// 【主題資訊 Information】
//   template<class Key,
//            class Compare   = std::less<Key>,
//            class Allocator = std::allocator<Key>> class set;   // <set>,C++98 起
//
//   常用介面：
//     std::pair<iterator,bool> insert(const Key& k);    // O(log n),bool 表示是否真的插入
//     iterator  find(const Key& k);                     // O(log n),找不到回 end()
//     size_type count(const Key& k) const;              // O(log n),set 只會是 0 或 1
//     size_type erase(const Key& k);                    // O(log n),回傳刪掉幾個(0 或 1)
//     iterator  erase(iterator pos);                    // 攤銷 O(1)
//     iterator  lower_bound(const Key& k);              // O(log n),第一個 >= k
//     iterator  upper_bound(const Key& k);              // O(log n),第一個 >  k
//     std::pair<iterator,iterator> equal_range(k);      // O(log n)
//     node_type extract(const Key& k);                  // C++17,把節點抽出來
//     void      merge(set& other);                      // C++17,節點搬家不重配置
//     bool      contains(const Key& k) const;           // C++20
//
//   複雜度：插入／刪除／查找皆 O(log n);走訪 O(n) 且**保證由小到大**
//   標頭檔：#include <set>（multiset 也在這個標頭檔裡）
//
// 【詳細解釋 Explanation】
//
// 【1. set 同時要滿足三件事,所以它只能是平衡樹】
// 如果只要「不重複」,雜湊表（unordered_set）就夠了,而且是 O(1)。
// std::set 之所以選擇比較慢的 O(log n),是因為它要同時滿足**三個**保證,
// 而雜湊表只能滿足第一個:
//
//   (a) 元素唯一（去重）
//   (b) **走訪時保證由小到大排序**      ← 雜湊表做不到
//   (c) **迭代器/指標/參考長期穩定**    ← 雜湊表 rehash 時會失效
//
// 能同時做到這三件事的資料結構,就是**平衡二元搜尋樹**。
// libstdc++ 的實作是**紅黑樹（red-black tree）**（此為**實作定義**;
// C++ 標準只規定 O(log n) 的複雜度與有序走訪,並沒有指定必須用哪種樹）。
//
// 這也直接給出了「什麼時候該用 set、什麼時候該用 unordered_set」的判準:
//   * 只需要查「在不在」、不在乎順序 → unordered_set,O(1) 更快
//   * 需要排序輸出、需要「找出所有介於 A 和 B 之間的元素」、
//     或需要迭代器在增刪之間保持有效 → 只能用 set
//
// 【2. 為什麼 set 的元素是 const:改了 key 就毀了整棵樹】
// set 的 iterator 解參考出來的是 **const Key&**。也就是說:
//
//     std::set<int> s{10, 20, 30};
//     auto it = s.find(20);
//     *it = 5;                     // ✗ 編譯錯誤:元素是 const
//
// 這不是保守,而是**必要**。元素在紅黑樹中的位置,是依照它的值排出來的:
// 20 之所以放在那個節點,是因為它比左邊大、比右邊小。如果允許你就地把 20
// 改成 5,節點位置卻不會跟著移動,整棵樹的排序不變式（invariant）就被破壞了 ——
// 之後所有的 find/lower_bound 都會走錯分支,而且**不會有任何錯誤訊息**,
// 只會安靜地給你錯誤答案。這是最難除錯的一種 bug,所以標準乾脆用型別擋死。
//
// 傳統做法是「erase 掉舊的、insert 新的」,但那會**銷毀節點再重新配置一個**,
// 對大型元素（例如內含 std::string 或 vector 的物件）是不必要的浪費。
// C++17 為此加入了 **node handle（節點把手）**:
//
//     auto nh = s.extract(20);     // 把整個節點從樹上摘下來,元素不複製、不銷毀
//     nh.value() = 5;              // 節點已離開樹,此時改 key 是安全的
//     s.insert(std::move(nh));     // 把同一個節點掛回正確位置
//
// 全程**沒有任何記憶體配置、沒有任何元素複製**,只是把節點從樹上摘下再掛回去。
// 這就是為什麼 extract 被稱為「唯一正確的改 key 方式」。
//
// 【3. insert 回傳 pair<iterator,bool>:一次呼叫給你兩個答案】
// set::insert 的回傳型別常讓初學者困惑,但它其實非常實用:
//
//     auto [it, inserted] = s.insert(30);   // C++17 structured binding
//     //   it        → 指向「set 裡那個值為 30 的元素」（無論是新插入的還是本來就有的）
//     //   inserted  → true 表示真的插入了;false 表示已經存在,這次沒動
//
// 為什麼這樣設計?因為「插入」和「檢查是否已存在」是同一次樹走訪就能同時得到的
// 資訊。如果分成兩步寫 `if (s.count(x) == 0) s.insert(x);`,你會**走訪兩次樹**,
// 白白多付一次 O(log n)。直接看 insert 回傳的 bool 才是正確寫法 ——
// 這也是很多「去重並統計新增了幾個」的程式碼的標準骨架。
//
// 【4. lower_bound / upper_bound / equal_range:選 set 的真正理由】
// 這三個成員函式是 set 相對於 unordered_set 最大的價值,因為它們**需要順序**,
// 雜湊表在原理上就不可能提供:
//
//     lower_bound(k)  → 第一個 **>= k** 的元素
//     upper_bound(k)  → 第一個 **>  k** 的元素
//     equal_range(k)  → 上面兩者組成的 pair
//
// 有了它們就能做**區間查詢（range query）**:
//     「給我所有介於 400 和 499 之間的狀態碼」
//     「給我這個時間戳之後最近的一筆事件」
//     「給我比 x 小的最大元素」（前驅查詢）
// 這些在 unordered_set 上只能退化成 O(n) 全掃。
//
// 注意：要用成員版的 s.lower_bound(k),**不要**用 std::lower_bound(s.begin(), …)。
// 後者是泛型演算法,對非隨機存取迭代器只能一步一步走,會退化成 O(n);
// 成員版才會真正沿著樹往下走,是 O(log n)。
//
// 【5. 比較器必須是 strict weak ordering:寫成 <= 是經典 bug】
// set 判斷「兩個元素是不是同一個」,用的不是 operator==,而是比較器:
//
//     兩元素 a、b 等價（equivalent） ⟺ !(a < b) && !(b < a)
//
// 這叫做**等價（equivalence）**,而不是**相等（equality）**。因此比較器必須
// 滿足 strict weak ordering,其中最容易被違反的一條是**非自反性**:
// 對任何 a,comp(a, a) 必須為 **false**。
//
// 所以如果你自訂比較器時寫成 `return a <= b;`,就違反了這條 ——
// comp(a,a) 會回傳 true。後果不是編譯錯誤,而是**未定義行為**:
// 樹的平衡邏輯會依賴這個性質,違反後結果不保證、也不可預測,
// 可能是元素找不到、可能是重複元素跑進 set、也可能走訪時越界。
// 記住:**自訂比較器一律用嚴格小於 `<`,絕不用 `<=`。**
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局：容器本體很小,成本都在節點
//     本機實測（GCC 15.2.0 / x86-64,屬**實作定義**）:
//         sizeof(std::set<int>)      == 48
//         sizeof(std::map<int,int>)  == 48     ← 和 set 一樣大
//     這 48 bytes 裝的是紅黑樹的管理結構:比較器物件、節點計數,
//     以及一個 header 節點（含 parent/left/right 三個指標與顏色欄位）。
//     set 和 map 一樣大不是巧合 —— libstdc++ 的 set 和 map 底層是**同一棵**
//     _Rb_tree 模板,差別只在「節點裡存的是 Key」還是「存 pair<const Key, T>」。
//
//     每個節點的實際內容大約是:
//         parent 指標(8) + left 指標(8) + right 指標(8) + 顏色(4,含 padding) + 元素
//     所以一個 set<int> 節點要花 40 bytes 左右來裝 4 bytes 的 int。
//     這就是節點式容器的代價:管理成本遠大於資料本身。
//
// (B) 為什麼是紅黑樹而不是 AVL 樹
//     兩者都是平衡 BST,查找都是 O(log n)。差別在平衡的嚴格程度:
//     AVL 樹平衡得更嚴格,查找略快,但**插入/刪除時需要更多次旋轉**;
//     紅黑樹容忍較鬆的平衡（最長路徑不超過最短路徑的兩倍）,
//     插入/刪除最多常數次旋轉,增刪成本較低。
//     STL 的關聯容器讀寫都很頻繁,紅黑樹的整體取捨較好,因此成為主流實作選擇
//     （再次強調:這是**實作定義**,標準沒有規定）。
//
// (C) 迭代器穩定性：set 相對於 vector 的決定性優勢
//     set 是節點式容器,元素一旦配置就不會搬家。標準保證:
//       * insert 之後,**所有既有的迭代器、指標、參考全部保持有效**
//         （即使插入導致樹旋轉也一樣 —— 旋轉只改指標,節點本身不動）
//       * erase 之後,**只有被刪掉的那個元素**的迭代器失效,其餘全部有效
//     對照 vector:一次 push_back 觸發 reallocation,所有迭代器與指標全部失效。
//     所以「需要長期持有指向元素的指標/迭代器」時,set/map 是正確選擇。
//
// (D) count() 為什麼存在（set 明明只會回 0 或 1）
//     因為 set 和 multiset 共用同一套介面。在 set 上 count() 只會是 0 或 1,
//     語意上等同於「在不在」;C++20 加入的 contains() 才是意圖更清楚的寫法。
//     在 C++17 以前的慣例寫法是 `s.find(k) != s.end()`,
//     比 `s.count(k) > 0` 更能表達「我只是要查存在性」。
//
// 【注意事項 Pay Attention】
//  1. set 的元素是 **const**,不能就地修改。要改 key 請用 C++17 的
//     extract() + 修改 + insert(node handle),或退而求其次 erase + insert。
//  2. 自訂比較器必須是 **strict weak ordering**。寫成 `a <= b` 會違反非自反性,
//     結果是 undefined behavior —— 行為不保證、也不可預測,不要假設它會怎麼壞。
//  3. 用**成員版** s.lower_bound(k)（O(log n)）,不要用 std::lower_bound
//     配 set 的迭代器（會退化成 O(n)）。同理適用 find:s.find 是 O(log n),
//     std::find 是 O(n)。
//  4. erase(iterator) 之後,該 iterator 就失效了,不能再用。正確寫法是
//     `it = s.erase(it);`（回傳下一個位置,C++11 起）。
//  5. 對 end() 解參考是 undefined behavior。lower_bound 找不到時會回 end(),
//     務必先檢查 `it != s.end()` 再解參考。
//  6. sizeof(std::set<int>) == 48、max_size() == 230584300921369395 都是本機
//     （GCC 15.2.0 / libstdc++ / x86-64）實測值,屬**實作定義**,換平台/實作會不同。
//  7. 「紅黑樹」是 libstdc++ 的實作選擇,不是標準規定。面試時說
//     「標準只要求 O(log n),主流實作用紅黑樹」比直接說「set 就是紅黑樹」精準。
//  8. 版本差異（已於本機以 -pedantic-errors 驗證）:
//     extract / merge 是 **C++17**;contains 是 **C++20**;
//     std::erase_if(set) 是 **C++20**;structured binding 是 **C++17**。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::set
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::set 和 std::unordered_set 該怎麼選?
//     答：unordered_set 是雜湊表,平均 O(1),但走訪順序無意義、rehash 時
//         迭代器會失效。set 是平衡二元搜尋樹（libstdc++ 用紅黑樹,屬實作定義）,
//         O(log n),但換來三件雜湊表給不了的事:走訪保證由小到大、
//         支援 lower_bound/upper_bound 這種區間查詢、
//         以及迭代器在插入與刪除其他元素時都保持有效。
//         只查存在性就用 unordered_set;需要順序或區間查詢就用 set。
//     追問：set 的迭代器在 insert 之後真的不會失效嗎?樹不是會旋轉?
//         → 真的不會。旋轉改的是節點之間的 parent/left/right 指標,
//           節點本身在記憶體中的位置沒有移動,所以指向元素的迭代器、
//           指標、參考全部仍然有效。這正是節點式容器的核心優勢。
//
// 🔥 Q2. 為什麼 set 的元素是 const?那要修改元素該怎麼辦?
//     答：因為元素在樹中的位置是依它的值決定的。若允許就地修改 key,
//         節點不會跟著移動,排序不變式就被破壞,之後所有 find/lower_bound
//         都會安靜地走錯分支給出錯誤答案 —— 沒有任何錯誤訊息。標準用型別擋死。
//         C++17 起的正解是 node handle:extract(k) 把節點摘下來、
//         改 nh.value()、再 insert(std::move(nh)) 掛回去,
//         全程不配置記憶體、不複製元素。
//     追問：那 std::map 呢?也不能改嗎?
//         → map 的元素型別是 pair<const Key, T>:**key 是 const 不能改,
//           但 value（T）可以自由修改**,因為 value 不參與排序。
//           要改 map 的 key 一樣得用 extract。
//
// ⚠️ 陷阱. 我想把值 20 改成 5,寫 `s.erase(20); s.insert(5);` 有什麼問題?
//        比起 extract 差在哪?
//     答：結果是對的,但成本不同。erase 會**銷毀節點並釋放記憶體**,
//         insert 再**重新配置一個新節點**,元素本身也要重新建構一次。
//         元素若是 std::string 或大型物件,這是完全不必要的一次配置＋一次複製。
//         extract 只是把同一個節點從樹上摘下再掛回去,零配置、零元素複製。
//     為什麼會錯：直覺上「反正結果一樣」,忽略了 set 是**節點式**容器 ——
//         節點的配置與銷毀本身就是成本。C++17 引入 node handle 就是為了
//         讓「移動元素」和「重建元素」這兩件事在語意上分開。
//
// ⚠️ 陷阱. 自訂比較器時我寫 `return a.score <= b.score;`,編譯過了,
//        為什麼行為怪怪的?
//     答：因為 set 要求比較器是 strict weak ordering,其中一條是**非自反性**:
//         comp(a, a) 必須為 false。寫 `<=` 會讓 comp(a,a) 回傳 true,違反要求。
//         這不是編譯錯誤,而是 **undefined behavior** —— 結果不保證也不可預測,
//         可能查不到明明存在的元素、可能出現「重複」元素、也可能走訪時越界。
//         比較器一律用嚴格小於 `<`。
//     為什麼會錯：腦中把比較器想成「排序用的大小關係」,覺得 <= 和 < 只差在
//         相等時的處理,無傷大雅。但 set 是**用比較器來判斷相等**的:
//         a 和 b 等價的定義是 !(a<b) && !(b<a)。比較器一旦不滿足嚴格性,
//         「相等」的判斷本身就壞了,而不只是排序順序的問題。
//
// ⚠️ 陷阱. 「先檢查再插入」的寫法 `if (s.count(x) == 0) s.insert(x);` 有問題嗎?
//     答：邏輯正確,但**走訪了兩次樹**,付了兩次 O(log n)。
//         insert 本來就會回傳 pair<iterator,bool>,bool 直接告訴你這次是否真的
//         插入了。正確寫法是 `if (s.insert(x).second) { /* 這是新元素 */ }`,
//         一次走訪就同時完成插入與判斷。
//     為什麼會錯：沿用了「查表再寫入」的通用思路,沒注意到 STL 的關聯容器
//         早就把這個資訊包在回傳值裡了 —— 這也是 insert 為什麼要回傳
//         那個看起來很囉唆的 pair 的原因。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <utility>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 349. Intersection of Two Arrays
//   題目：給兩個整數陣列,回傳它們的交集,結果中每個元素只能出現一次。
//   為什麼用到本主題：這題同時需要 set 的兩個特性 ——「自動去重」讓結果天然
//     滿足「每個元素只出現一次」,而「有序」讓輸出直接是排序好的,不必再 sort。
//     把第一個陣列丟進 set,再拿第二個陣列去查,就是標準解。
//   複雜度：時間 O((n+m) log n),空間 O(n)。
// -----------------------------------------------------------------------------
std::vector<int> intersection(const std::vector<int>& nums1,
                              const std::vector<int>& nums2) {
    std::set<int> pool(nums1.begin(), nums1.end());   // 建 set 時自動去重 + 排序
    std::set<int> result;
    for (int v : nums2) {
        if (pool.count(v) != 0) {      // O(log n) 查找
            result.insert(v);          // set 保證結果不重複
        }
    }
    return std::vector<int>(result.begin(), result.end());  // 走訪即已排序
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 220. Contains Duplicate III
//   題目：給定陣列 nums 與兩個數 indexDiff(k)、valueDiff(t),判斷是否存在
//     一組 i != j,使得 |i - j| <= k 且 |nums[i] - nums[j]| <= t。
//   為什麼用到本主題：這題是「非用 std::set 不可」的典型 —— 因為它要問的是
//     **區間查詢**:「目前視窗裡有沒有任何值落在 [nums[i]-t, nums[i]+t] 之間?」
//     unordered_set 只能回答「某個特定值在不在」,無法回答「某個範圍內有沒有值」,
//     在這題上只能 O(n) 全掃。set 的 lower_bound 一次 O(log k) 就給出答案。
//   作法：維護一個大小不超過 k 的滑動視窗 set,對每個新元素用 lower_bound
//     找出「第一個 >= nums[i]-t 的值」,再檢查它是否 <= nums[i]+t。
//   複雜度：時間 O(n log k),空間 O(k)。
//   註：用 long long 避免 nums[i] ± t 在 int 邊界溢位（LeetCode 的測資會踩這個）。
// -----------------------------------------------------------------------------
bool containsNearbyAlmostDuplicate(const std::vector<int>& nums, int k, int t) {
    if (k <= 0 || t < 0) return false;

    std::set<long long> window;
    for (std::size_t i = 0; i < nums.size(); ++i) {
        const long long cur = static_cast<long long>(nums[i]);

        // 區間查詢:視窗中第一個 >= cur - t 的元素
        auto it = window.lower_bound(cur - t);
        if (it != window.end() && *it <= cur + t) {
            return true;                   // 找到一個落在 [cur-t, cur+t] 的值
        }

        window.insert(cur);

        // 維持視窗大小 <= k：把離開視窗的最舊元素移除
        if (i >= static_cast<std::size_t>(k)) {
            window.erase(static_cast<long long>(nums[i - static_cast<std::size_t>(k)]));
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從 access log 收斂出現過的 HTTP 狀態碼,並做區間查詢
//   情境：分析 nginx access log 時,常要先知道「這批流量出現過哪些狀態碼」,
//     再進一步追問「其中屬於 4xx（用戶端錯誤）的有哪些」。
//   為什麼用到本主題：
//     (1) 狀態碼會大量重複 → set 自動去重,不必自己判斷。
//     (2) 報表要由小到大列出 → set 走訪天然有序,不必另外 sort。
//     (3) 「所有 4xx」是一個**區間查詢** → lower_bound(400) 到 lower_bound(500),
//         這正是 set 相對於 unordered_set 唯一無法被取代的能力。
// -----------------------------------------------------------------------------
std::set<int> collectStatusCodes(const std::vector<std::string>& logLines) {
    std::set<int> codes;
    for (const std::string& line : logLines) {
        // 簡化的解析：取行尾以空白分隔的最後一個欄位當狀態碼
        std::size_t pos = line.find_last_of(' ');
        if (pos == std::string::npos) continue;
        const std::string field = line.substr(pos + 1);
        try {
            codes.insert(std::stoi(field));
        } catch (const std::exception&) {
            continue;                       // 欄位不是數字就跳過,不讓壞資料中斷分析
        }
    }
    return codes;
}

// 區間查詢：取出 [low, high) 之間的所有狀態碼
std::vector<int> codesInRange(const std::set<int>& codes, int low, int high) {
    std::vector<int> out;
    // 成員版 lower_bound 是 O(log n)；std::lower_bound 配 set 迭代器會退化成 O(n)
    for (auto it = codes.lower_bound(low); it != codes.lower_bound(high); ++it) {
        out.push_back(*it);
    }
    return out;
}

int main() {
    // ── 原始課堂示範:set 的基本操作 ──────────────────────────────────────
    std::cout << "=== std::set ===" << std::endl;

    // 元素自動排序，不允許重複
    std::set<int> s;

    s.insert(30);
    s.insert(10);
    s.insert(50);
    s.insert(20);
    s.insert(40);
    s.insert(30);  // 重複，會被忽略

    std::cout << "元素（自動排序）: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "大小: " << s.size() << std::endl;

    // 查找
    auto it = s.find(30);
    if (it != s.end()) {
        std::cout << "找到 30" << std::endl;
    }

    // count：因為不重複，只會回傳 0 或 1
    std::cout << "30 的數量: " << s.count(30) << std::endl;
    std::cout << "100 的數量: " << s.count(100) << std::endl;

    // 刪除
    s.erase(30);
    std::cout << "刪除 30 後: ";
    for (int n : s) std::cout << n << " ";
    std::cout << std::endl;

    // ── insert 的回傳值：一次走訪拿到兩個答案 ─────────────────────────────
    std::cout << "\n=== insert 回傳 pair<iterator,bool> ===" << std::endl;
    std::set<int> t = {1, 2, 3};
    {
        auto [pos, inserted] = t.insert(4);       // C++17 structured binding
        std::cout << "insert(4): 值=" << *pos
                  << ", 這次真的插入了嗎? " << std::boolalpha << inserted << std::endl;
    }
    {
        auto [pos, inserted] = t.insert(2);       // 已存在
        std::cout << "insert(2): 值=" << *pos
                  << ", 這次真的插入了嗎? " << inserted
                  << "  ← 已存在,沒有動作" << std::endl;
    }
    std::cout << "→ 不需要先 count 再 insert,那會白走兩次樹" << std::endl;

    // ── 區間查詢：lower_bound / upper_bound / equal_range ─────────────────
    std::cout << "\n=== 區間查詢: 選 set 而非 unordered_set 的真正理由 ===" << std::endl;
    std::set<int> nums = {10, 20, 30, 40, 50, 60};
    std::cout << "集合: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << std::endl;

    auto lb = nums.lower_bound(30);   // 第一個 >= 30
    auto ub = nums.upper_bound(30);   // 第一個 >  30
    std::cout << "lower_bound(30) = " << *lb << "  (第一個 >= 30)" << std::endl;
    std::cout << "upper_bound(30) = " << *ub << "  (第一個 >  30)" << std::endl;

    // 找不存在的值：lower_bound 給出「應該插在哪」
    auto lb35 = nums.lower_bound(35);
    std::cout << "lower_bound(35) = " << *lb35
              << "  (35 不存在,回傳第一個比它大的)" << std::endl;

    std::cout << "所有介於 [20, 50) 的元素: ";
    for (auto p = nums.lower_bound(20); p != nums.lower_bound(50); ++p) {
        std::cout << *p << " ";
    }
    std::cout << std::endl;

    // 邊界：找不到就回 end()，解參考 end() 是 UB
    auto lb99 = nums.lower_bound(99);
    std::cout << "lower_bound(99) 是否等於 end()? "
              << (lb99 == nums.end() ? "是" : "否")
              << "  ← 必須先檢查再解參考" << std::endl;

    // ── 元素是 const：C++17 用 extract 改 key ─────────────────────────────
    std::cout << "\n=== 元素是 const,改 key 要用 extract (C++17) ===" << std::endl;
    std::set<std::string> names = {"alice", "bob", "carol"};
    std::cout << "原始: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << std::endl;
    // 註：*names.find("bob") = "zoe"; 無法編譯 —— 元素型別是 const std::string

    auto nh = names.extract("bob");     // 把節點從樹上摘下來（不銷毀、不重新配置）
    if (!nh.empty()) {
        nh.value() = "zoe";             // 節點已離開樹,此時改 key 是安全的
        names.insert(std::move(nh));    // 掛回樹上的正確位置
    }
    std::cout << "extract 改成 zoe 後: ";
    for (const auto& n : names) std::cout << n << " ";
    std::cout << "  ← 全程零配置、零元素複製" << std::endl;

    // ── 迭代器穩定性 ──────────────────────────────────────────────────────
    std::cout << "\n=== 迭代器穩定性: 插入/刪除其他元素不會失效 ===" << std::endl;
    std::set<int> stable = {100, 200, 300};
    auto keep = stable.find(200);          // 長期持有這個迭代器
    stable.insert(150);
    stable.insert(250);
    stable.erase(100);                     // 刪掉「別的」元素
    std::cout << "插入 150、250 並刪除 100 之後,原本指向 200 的迭代器仍然有效: "
              << *keep << std::endl;
    std::cout << "→ 節點式容器的核心優勢;vector 一旦 reallocation 就全部失效" << std::endl;

    // ── 容器規格 ──────────────────────────────────────────────────────────
    std::cout << "\n=== 容器規格 (本機實測,屬實作定義) ===" << std::endl;
    std::cout << "sizeof(std::set<int>)     = " << sizeof(std::set<int>) << std::endl;
    std::cout << "sizeof(std::map<int,int>) = 48  (與 set 同大：底層是同一棵 _Rb_tree)"
              << std::endl;
    std::cout << "set<int>::max_size()      = " << std::set<int>().max_size() << std::endl;

    // ── LeetCode 349 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    std::vector<int> a1 = {4, 9, 5};
    std::vector<int> a2 = {9, 4, 9, 8, 4};
    std::cout << "nums1 = [4,9,5], nums2 = [9,4,9,8,4]" << std::endl;
    std::cout << "交集（去重 + 已排序）= [";
    std::vector<int> inter = intersection(a1, a2);
    for (std::size_t i = 0; i < inter.size(); ++i) {
        std::cout << inter[i] << (i + 1 < inter.size() ? "," : "");
    }
    std::cout << "]" << std::endl;

    // ── LeetCode 220 ──────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 220. Contains Duplicate III ===" << std::endl;
    std::vector<int> d1 = {1, 2, 3, 1};
    std::cout << "nums=[1,2,3,1], k=3, t=0 → " << std::boolalpha
              << containsNearbyAlmostDuplicate(d1, 3, 0) << std::endl;
    std::vector<int> d2 = {1, 5, 9, 1, 5, 9};
    std::cout << "nums=[1,5,9,1,5,9], k=2, t=3 → "
              << containsNearbyAlmostDuplicate(d2, 2, 3) << std::endl;
    std::vector<int> d3 = {1, 0, 1, 1};
    std::cout << "nums=[1,0,1,1], k=1, t=2 → "
              << containsNearbyAlmostDuplicate(d3, 1, 2) << std::endl;

    // ── 日常實務：access log 狀態碼分析 ───────────────────────────────────
    std::cout << "\n=== 日常實務: access log 狀態碼收斂與區間查詢 ===" << std::endl;
    std::vector<std::string> accessLog = {
        "10.0.0.7 GET /api/users 200",
        "10.0.0.9 GET /api/users 200",
        "10.0.0.7 POST /api/login 401",
        "10.0.0.3 GET /static/app.js 304",
        "10.0.0.7 GET /api/orders 500",
        "10.0.0.4 GET /missing 404",
        "10.0.0.9 POST /api/login 401",
        "10.0.0.2 GET /api/users 200",
        "10.0.0.8 GET /api/report 503"
    };

    std::set<int> codes = collectStatusCodes(accessLog);
    std::cout << "出現過的狀態碼（去重 + 排序）: ";
    for (int c : codes) std::cout << c << " ";
    std::cout << std::endl;
    std::cout << "共 " << codes.size() << " 種（原始 " << accessLog.size() << " 行）"
              << std::endl;

    std::vector<int> clientErrors = codesInRange(codes, 400, 500);
    std::cout << "4xx 用戶端錯誤: ";
    for (int c : clientErrors) std::cout << c << " ";
    std::cout << std::endl;

    std::vector<int> serverErrors = codesInRange(codes, 500, 600);
    std::cout << "5xx 伺服器錯誤: ";
    for (int c : serverErrors) std::cout << c << " ";
    std::cout << std::endl;
    std::cout << "→ 區間查詢在 unordered_set 上只能 O(n) 全掃" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類6.cpp" -o set_demo

// === 預期輸出 ===
// === std::set ===
// 元素（自動排序）: 10 20 30 40 50
// 大小: 5
// 找到 30
// 30 的數量: 1
// 100 的數量: 0
// 刪除 30 後: 10 20 40 50
//
// === insert 回傳 pair<iterator,bool> ===
// insert(4): 值=4, 這次真的插入了嗎? true
// insert(2): 值=2, 這次真的插入了嗎? false  ← 已存在,沒有動作
// → 不需要先 count 再 insert,那會白走兩次樹
//
// === 區間查詢: 選 set 而非 unordered_set 的真正理由 ===
// 集合: 10 20 30 40 50 60
// lower_bound(30) = 30  (第一個 >= 30)
// upper_bound(30) = 40  (第一個 >  30)
// lower_bound(35) = 40  (35 不存在,回傳第一個比它大的)
// 所有介於 [20, 50) 的元素: 20 30 40
// lower_bound(99) 是否等於 end()? 是  ← 必須先檢查再解參考
//
// === 元素是 const,改 key 要用 extract (C++17) ===
// 原始: alice bob carol
// extract 改成 zoe 後: alice carol zoe   ← 全程零配置、零元素複製
//
// === 迭代器穩定性: 插入/刪除其他元素不會失效 ===
// 插入 150、250 並刪除 100 之後,原本指向 200 的迭代器仍然有效: 200
// → 節點式容器的核心優勢;vector 一旦 reallocation 就全部失效
//
// === 容器規格 (本機實測,屬實作定義) ===
// sizeof(std::set<int>)     = 48
// sizeof(std::map<int,int>) = 48  (與 set 同大：底層是同一棵 _Rb_tree)
// set<int>::max_size()      = 230584300921369395
//
// === LeetCode 349. Intersection of Two Arrays ===
// nums1 = [4,9,5], nums2 = [9,4,9,8,4]
// 交集（去重 + 已排序）= [4,9]
//
// === LeetCode 220. Contains Duplicate III ===
// nums=[1,2,3,1], k=3, t=0 → true
// nums=[1,5,9,1,5,9], k=2, t=3 → false
// nums=[1,0,1,1], k=1, t=2 → true
//
// === 日常實務: access log 狀態碼收斂與區間查詢 ===
// 出現過的狀態碼（去重 + 排序）: 200 304 401 404 500 503
// 共 6 種（原始 9 行）
// 4xx 用戶端錯誤: 401 404
// 5xx 伺服器錯誤: 500 503
// → 區間查詢在 unordered_set 上只能 O(n) 全掃
