// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計16.cpp
//    —  同名時該用誰：容器成員函式 vs 通用演算法
// =============================================================================
//
// 【主題資訊 Information】
//   同名但不同東西的對照（左為成員函式，右為通用演算法）：
//     list::sort()      vs  std::sort()      → std::sort 對 list **編譯失敗**
//     list::remove(v)   vs  std::remove()    → 成員是真刪除；演算法只搬移
//     list::unique()    vs  std::unique()    → 成員是真刪除；演算法只搬移
//     list::reverse()   vs  std::reverse()   → 成員只改指標，不搬移元素值
//     list::merge()     vs  std::merge()     → 成員是接合節點，不配置記憶體
//     set::find(v)      vs  std::find()      → O(log N)  vs  O(N)
//     set::count(v)     vs  std::count()     → O(log N)  vs  O(N)
//     set::lower_bound  vs  std::lower_bound → O(log N)  vs  對 set 為 O(N)
//     map::find(k)      vs  std::find()      → O(log N)；且演算法比的是 pair
//
//   標準版本：以上成員函式皆為 C++98
//   通則：**同名時一律優先用成員函式**
//   標頭檔：<list>、<set>、<map>；通用演算法在 <algorithm>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼會出現「同名但不同東西」】
// 這是分離設計必然的產物。通用演算法為了能適用所有容器，
// **只能透過 iterator 這個最小介面工作**，因此它看不到：
//   * set 內部是排序好的樹（所以 std::find 只能一個一個比）
//   * list 是節點串起來的（所以 std::remove 只能搬移元素值，不能解開指標）
// 容器自己則完全知道內部結構，能做到演算法做不到的事。
// 於是標準在容器上另外提供了同名的成員函式——
// **名字一樣是為了好記，但它們是兩種不同的東西。**
//
// 【2. 三種不同層級的差異（不是只有「比較快」）】
//   (a) **複雜度差異**：set::find 是 O(log N)，std::find 是 O(N)。
//       兩者結果相同，只是快慢不同。
//   (b) **語意差異**：list::remove 是**真的刪除**（size 會變）；
//       std::remove **只搬移**（size 不變，需搭配 erase）。
//       這不是效能問題，是行為根本不同——用錯會得到錯誤的結果。
//   (c) **可用性差異**：std::sort 對 list **根本編譯不過**（需要 Random Access）。
// 所以「優先用成員函式」不只是效能建議，很多時候是正確性要求。
//
// 【3. list 的成員函式為什麼能做到演算法做不到的事】
// 關鍵在於**鏈結串列可以「重新接線」而不搬移元素本身**：
//   * list::sort()   → merge sort，只重新串接節點指標
//   * list::remove() → 解開節點指標並釋放該節點
//   * list::reverse()→ 把每個節點的 prev/next 對調
//   * list::splice() → 把另一個 list 的節點整段接過來，**O(1)**，完全不複製
// 這帶來一個非常重要的性質：
//   **這些操作之後，指向未被刪除元素的 iterator、reference、pointer 全部仍然有效。**
// std::sort 是搬移「元素的值」，排序後同一個 iterator 指向的東西就換人了。
// 若你的程式持有指向串列元素的長期指標（例如 LRU 快取的節點指標），
// 這個差別是決定性的——這正是 LRU Cache 用 list + splice 實作的原因。
//
// 【4. set / map：不只是快，還有「不能用」的問題】
// 對 set 使用會**寫入元素**的演算法（sort、replace、remove、unique）
// 全部**編譯失敗**，因為 set 的元素透過 iterator 取得時是 const 的
// （允許修改會破壞紅黑樹的排序不變式）。
// 而唯讀類的演算法（find、count、lower_bound）雖然能編譯，
// 卻是 O(N) 線性掃描——因為它們不知道自己面對的是一棵排序好的樹。
// 特別注意 map：std::find 對 map 比的是整個 pair<const K, V>，
// 你通常想找的是 key，這時應該用 m.find(key)。
//
// 【5. 唯一的例外：std::find_if 這類「演算法才有」的功能】
// 成員函式只提供「依值查找」，沒有「依條件查找」。
// 所以要在 set 中找「第一個大於 100 的元素」，仍然要用 std::find_if
// （或更好的 s.lower_bound(101)，若條件剛好與排序準則一致）。
// 通則的精確版本是：
//   **功能相同且同名時用成員函式；成員函式做不到的事才用通用演算法。**
//
// 【概念補充 Concept Deep Dive】
//
// (A) list::splice —— 分離設計完全做不到的操作
//   lst1.splice(pos, lst2) 把 lst2 的全部節點搬到 lst1 的 pos 位置，
//   **複雜度 O(1)**（只改幾個指標），且不配置也不釋放任何記憶體。
//   通用演算法**永遠做不到這件事**，因為它只有 iterator，
//   無法改變兩個容器的節點歸屬。這是成員函式價值的極致展現。
//
// (B) 為什麼 std::sort 不能對 list 自動改用 list::sort
//   因為 std::sort 收到的只是兩個 iterator，它無從得知「這段範圍屬於哪個 list」，
//   更無法呼叫該物件的成員函式——這與 std::remove 不能真的刪除是同一個原因。
//   iterator 是刻意設計成「不回指容器」的，這正是解耦的代價。
//
// (C) unordered_set / unordered_map 也有同樣的對照
//   它們的成員 find 是**平均 O(1)**（雜湊），std::find 依然是 O(N)。
//   而且它們只提供 Forward Iterator，
//   所以連 std::reverse 都不能用（也沒有意義——雜湊容器本來就無序）。
//
// 【注意事項 Pay Attention】
// 1. **同名時一律優先用成員函式**——這是本課最實用的一條準則。
// 2. 差異有三種層級：複雜度不同（set::find）、**語意不同**（list::remove
//    是真刪除、std::remove 不是）、**可用性不同**（std::sort 對 list 編譯失敗）。
// 3. **list 的成員操作不會使 iterator 失效**（被刪除的元素除外），
//    std::sort 則會讓 iterator 指向不同的元素。持有長期指標時這是關鍵差別。
// 4. 對 set / map 使用會寫入元素的演算法一律編譯失敗（元素是 const）。
// 5. map 用 std::find 是拿整個 pair 比較；要找 key 請用 m.find(key)。
// 6. **成員函式做不到的事才用通用演算法**（例如 find_if 這類依條件查找）。
// 7. list::sort 是 merge sort，**保證穩定**；std::sort 不保證穩定。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】成員函式 vs 通用演算法
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 std::set 有自己的 find，而不直接用 std::find？
//     答：因為兩者複雜度差很多。std::find 只透過 iterator 逐一用 operator==
//         比較，是 O(N)；set::find 知道自己是排序好的紅黑樹，
//         能走樹狀搜尋路徑，是 O(log N)。
//         這是分離設計的代價：通用演算法拿不到容器的內部結構知識。
//     追問：為什麼標準不讓 std::find 對 set 自動特化？→ 因為兩者判斷「相等」
//         的依據不同：std::find 用 operator==，set 用比較器導出的等價關係
//         （!(a<b) && !(b<a)）。自動替換會改變語意。
//
// 🔥 Q2. list::remove(2) 和 std::remove(lst.begin(), lst.end(), 2)
//        差別在哪？這是效能問題還是正確性問題？
//     答：**兩者都是**。效能上，list::remove 直接解開節點指標並釋放節點，
//         std::remove 則要逐一 move assignment 搬移元素值。
//         但更關鍵的是**語意不同**：list::remove 是真的刪除，size 會變小；
//         std::remove 只是把要保留的往前搬、回傳新邏輯結尾，
//         **size 完全不變**，必須另外呼叫 erase 才會真的刪除。
//         用錯不只是慢，是結果就錯了。
//     追問：為什麼 std::remove 不能真的刪除？→ 它只有兩個 iterator，
//         拿不到容器本體，沒有能力呼叫 erase 改變 size。
//
// 🔥 Q3. list::sort() 和 std::sort() 除了「能不能用」之外，還有什麼本質差別？
//     答：**對 iterator 有效性的影響完全不同**。list::sort 用 merge sort
//         重新串接節點指標，**元素本身完全不移動**，所以排序後原有的
//         iterator、reference、pointer 全部仍指向同一個元素、依然有效。
//         std::sort 則是搬移元素的值，排序後同一個 iterator 指向的東西就換人了。
//         若程式持有指向元素的長期指標（如 LRU 快取），這是決定性的差別。
//         另外 list::sort 是 merge sort **保證穩定**，std::sort 不保證。
//     追問：還有什麼是通用演算法完全做不到的？→ list::splice，
//         把另一個 list 的節點整段接過來，**O(1)**、不複製也不配置記憶體。
//         通用演算法永遠做不到，因為它無法改變節點的容器歸屬。
//
// ⚠️ 陷阱. 想從 std::set 中移除所有偶數，寫成
//        s.erase(std::remove_if(s.begin(), s.end(), isEven), s.end());
//        為什麼不行？
//     答：**編譯就會失敗**。remove_if 需要把要保留的元素 move assignment
//         往前搬，但 set 的元素透過 iterator 取得時是 const 的
//         （能修改就會破壞紅黑樹的排序不變式），無法被賦值。
//         正確做法是走訪並用 s.erase(it) 逐一刪除（注意先取得下一個 iterator），
//         C++20 之後可以直接用 std::erase_if(s, isEven)。
//     為什麼會錯：erase-remove idiom 被當成「刪除元素的萬用句型」背了下來，
//         卻沒注意到它**只適用於序列容器**（vector / deque / string）。
//         關聯容器有自己的 erase，而且本來就不需要搬移。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】LRU 快取的核心：為什麼一定要用 list 的成員函式
//   情境：實作一個 LRU（Least Recently Used）快取。需求是「每次存取某個 key，
//         就把它移到最近使用的一端」，而且要 O(1)。
//   為什麼用到本主題：這是 list::splice 的殺手級應用——
//         把節點從串列中間搬到尾端，**O(1) 且不複製資料**，
//         更關鍵的是 **map 中存的 iterator 在 splice 之後仍然有效**。
//         若改用 vector + std::rotate，每次搬移都是 O(N)，
//         而且所有既存的 iterator 全部失效，整個設計就垮了。
//         這是「成員函式做得到、通用演算法做不到」最有說服力的例子。
// -----------------------------------------------------------------------------
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

    void access(const std::string& key) {
        auto found = index_.find(key);          // map::find → O(log N)
        if (found != index_.end()) {
            // 命中：把該節點移到尾端（最近使用）
            // ★ splice 是 O(1)，且 found->second 這個 iterator 之後仍然有效
            order_.splice(order_.end(), order_, found->second);
            return;
        }
        // 未命中：容量滿了就淘汰最舊的（串列前端）
        if (order_.size() >= capacity_) {
            index_.erase(order_.front());
            order_.pop_front();
        }
        order_.push_back(key);
        index_[key] = std::prev(order_.end());  // 存下節點位置
    }

    void print(const std::string& label) const {
        std::cout << "  " << label << " (舊→新): ";
        for (const auto& k : order_) std::cout << k << " ";
        std::cout << std::endl;
    }

private:
    std::size_t capacity_;
    std::list<std::string> order_;                                    // 使用順序
    std::map<std::string, std::list<std::string>::iterator> index_;   // key → 節點位置
};

int main() {
    // list 的成員函數 vs STL 演算法
    std::cout << "=== list 的成員函數 ===" << std::endl;

    std::list<int> lst = {5, 2, 8, 1, 9, 2, 7, 2};

    // list::sort() 比 std::sort() 更適合 list
    // ★ 不是「更適合」而已 —— std::sort 對 list 根本編譯不過（需要 Random Access）
    // ★ list::sort 是 merge sort，只重新串接節點指標，元素本身完全不移動
    lst.sort();
    std::cout << "sort 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // list::remove() 比 erase-remove 慣用法更高效
    // ★ 而且語意不同：這是「真的刪除」，size 會變小
    lst.remove(2);
    std::cout << "remove(2) 後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << "(size=" << lst.size() << ")" << std::endl;

    // list::unique() 移除連續重複, 但不會移除非連續的重複元素
    std::list<int> lst2 = {1, 1, 2, 2, 2, 3, 3};
    lst2.unique();
    std::cout << "unique 後: ";
    for (int n : lst2) std::cout << n << " ";
    std::cout << "(size=" << lst2.size() << ")  ← 成員版是真刪除，不必再 erase" << std::endl;

    // ★ 對照：std::remove 對 list 雖然能編譯，但只搬移、size 不變
    std::cout << "\n=== 語意差異：成員 remove vs std::remove ===" << std::endl;
    std::list<int> memberVer = {1, 2, 3, 2, 4};
    std::list<int> algoVer   = {1, 2, 3, 2, 4};

    memberVer.remove(2);                                          // 真刪除
    (void)std::remove(algoVer.begin(), algoVer.end(), 2);         // 只搬移，回傳值刻意丟棄

    std::cout << "  list::remove(2)  後 size = " << memberVer.size()
              << "  ← 真的刪除了" << std::endl;
    std::cout << "  std::remove(2)   後 size = " << algoVer.size()
              << "  ← size 完全沒變（還需要 erase）" << std::endl;

    // ★ iterator 有效性：list 成員操作不會讓既存 iterator 指向別的元素
    std::cout << "\n=== iterator 有效性差異 ===" << std::endl;
    std::list<int> stable = {30, 10, 20};
    auto keep = std::find(stable.begin(), stable.end(), 10);   // 記住指向 10 的節點
    stable.sort();                                             // 只改指標，不搬移元素值
    std::cout << "  list 排序後，原本指向 10 的 iterator 仍指向: " << *keep
              << "  ← 依然有效" << std::endl;

    std::vector<int> unstable = {30, 10, 20};
    auto vpos = unstable.begin() + 1;                          // 原本指向 10
    std::sort(unstable.begin(), unstable.end());               // 搬移元素的值
    std::cout << "  vector 排序後，同一個位置的 iterator 現在指向: " << *vpos
              << "  ← 位置還在，但元素換人了" << std::endl;

    // set 的成員函數
    std::cout << "\n=== set 的成員函數 ===" << std::endl;
    std::set<int> s = {5, 2, 8, 1, 9};

    // set::find() 是 O(log N)（紅黑樹）, std::find() 是 O(N)（線性掃描）
    auto it = s.find(8);                                  // O(log N)
    auto it2 = std::find(s.begin(), s.end(), 8);          // O(N)，能用但不該用

    if (it != s.end()) {
        std::cout << "set::find(8): 找到，值 = " << *it << "  [O(log N) 樹狀搜尋]" << std::endl;
    }
    if (it2 != s.end()) {
        std::cout << "std::find(8): 找到，值 = " << *it2 << "  [O(N) 線性掃描，結果相同但較慢]"
                  << std::endl;
    }

    std::cout << "set::count(8) = " << s.count(8) << "  [O(log N)]" << std::endl;
    std::cout << "std::count(8) = " << std::count(s.begin(), s.end(), 8)
              << "  [O(N)，同樣結果]" << std::endl;

    // ★ set 不能用會寫入元素的演算法（元素是 const）
    std::cout << "\n=== set 不能用「會寫入元素」的演算法 ===" << std::endl;
    std::cout << "  std::sort(s.begin(), s.end())      → 編譯失敗（元素是 const，且非隨機存取）"
              << std::endl;
    std::cout << "  std::remove_if(s.begin(), s.end()) → 編譯失敗（無法對 const 元素賦值）"
              << std::endl;
    std::cout << "  正確做法：走訪並用 s.erase(it) 逐一刪除" << std::endl;

    // 正確地從 set 移除所有偶數（C++17 寫法；C++20 可直接用 std::erase_if）
    std::set<int> nums = {1, 2, 3, 4, 5, 6};
    for (auto i = nums.begin(); i != nums.end(); ) {
        if (*i % 2 == 0) i = nums.erase(i);   // erase 回傳下一個位置
        else ++i;
    }
    std::cout << "  移除偶數後的 set: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << std::endl;

    // ★ map：std::find 比的是整個 pair，要找 key 必須用成員函式
    std::cout << "\n=== map 要找 key 必須用成員函式 ===" << std::endl;
    std::map<std::string, int> stock = {{"apple", 12}, {"banana", 5}, {"cherry", 30}};
    auto mit = stock.find("banana");          // O(log N)，直接用 key 找
    if (mit != stock.end()) {
        std::cout << "  map::find(\"banana\") → 數量 " << mit->second << std::endl;
    }
    std::cout << "  (std::find 對 map 是拿整個 pair<const K,V> 比較，"
                 "不能只給 key)" << std::endl;

    // ★ 成員函式做不到的事，才用通用演算法
    std::cout << "\n=== 成員函式做不到的：依「條件」查找 ===" << std::endl;
    auto big = std::find_if(s.begin(), s.end(), [](int n) { return n > 4; });
    if (big != s.end()) {
        std::cout << "  set 中第一個大於 4 的元素: " << *big
                  << "  [成員函式只能依值查找，這裡必須用 find_if]" << std::endl;
    }
    std::cout << "  (但若條件與排序準則一致，s.lower_bound(5) 更快：O(log N))"
              << std::endl;
    std::cout << "  s.lower_bound(5) = " << *s.lower_bound(5) << std::endl;

    std::cout << "\n=== 日常實務：LRU 快取（list::splice 的殺手級應用）===" << std::endl;
    LruCache cache(3);
    cache.access("/api/users");
    cache.access("/api/orders");
    cache.access("/api/stats");
    cache.print("初始狀態");

    cache.access("/api/users");        // 命中 → splice 到尾端，O(1)
    cache.print("再次存取 users 後");

    cache.access("/api/report");       // 未命中且已滿 → 淘汰最舊的 orders
    cache.print("加入 report 後");
    std::cout << "  (splice 是 O(1) 且 map 中存的 iterator 仍然有效；"
                 "換成 vector 就會全部失效)" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計16.cpp -o demo16

// === 預期輸出 ===
// === list 的成員函數 ===
// sort 後: 1 2 2 2 5 7 8 9 
// remove(2) 後: 1 5 7 8 9 (size=5)
// unique 後: 1 2 3 (size=3)  ← 成員版是真刪除，不必再 erase
//
// === 語意差異：成員 remove vs std::remove ===
//   list::remove(2)  後 size = 3  ← 真的刪除了
//   std::remove(2)   後 size = 5  ← size 完全沒變（還需要 erase）
//
// === iterator 有效性差異 ===
//   list 排序後，原本指向 10 的 iterator 仍指向: 10  ← 依然有效
//   vector 排序後，同一個位置的 iterator 現在指向: 20  ← 位置還在，但元素換人了
//
// === set 的成員函數 ===
// set::find(8): 找到，值 = 8  [O(log N) 樹狀搜尋]
// std::find(8): 找到，值 = 8  [O(N) 線性掃描，結果相同但較慢]
// set::count(8) = 1  [O(log N)]
// std::count(8) = 1  [O(N)，同樣結果]
//
// === set 不能用「會寫入元素」的演算法 ===
//   std::sort(s.begin(), s.end())      → 編譯失敗（元素是 const，且非隨機存取）
//   std::remove_if(s.begin(), s.end()) → 編譯失敗（無法對 const 元素賦值）
//   正確做法：走訪並用 s.erase(it) 逐一刪除
//   移除偶數後的 set: 1 3 5 
//
// === map 要找 key 必須用成員函式 ===
//   map::find("banana") → 數量 5
//   (std::find 對 map 是拿整個 pair<const K,V> 比較，不能只給 key)
//
// === 成員函式做不到的：依「條件」查找 ===
//   set 中第一個大於 4 的元素: 5  [成員函式只能依值查找，這裡必須用 find_if]
//   (但若條件與排序準則一致，s.lower_bound(5) 更快：O(log N))
//   s.lower_bound(5) = 5
//
// === 日常實務：LRU 快取（list::splice 的殺手級應用）===
//   初始狀態 (舊→新): /api/users /api/orders /api/stats 
//   再次存取 users 後 (舊→新): /api/orders /api/stats /api/users 
//   加入 report 後 (舊→新): /api/stats /api/users /api/report 
//   (splice 是 O(1) 且 map 中存的 iterator 仍然有效；換成 vector 就會全部失效)
