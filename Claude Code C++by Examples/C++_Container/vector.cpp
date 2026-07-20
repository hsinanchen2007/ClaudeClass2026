// ============================================================================
//  vector.cpp — std::vector 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra vector.cpp -o vector && ./vector
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/vector
//  參考 (cplusplus.com): https://cplusplus.com/reference/vector/vector/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::vector 是一個「動態陣列」(dynamic array),元素在記憶體中是連續存放的
//  (contiguous storage)。可以視為 C-style array 的增強版,具備自動擴容能力。
//
//  ▌ 底層資料結構
//  一塊連續的 heap 記憶體 + 三個指標 (begin / end / capacity_end):
//
//      ┌───────────────────────── capacity ─────────────────────────┐
//      ┌─────────────── size ───────────────┐
//      │  e0  │  e1  │  e2  │  e3  │  e4  │ /// 未使用的預留空間 ///│
//      └──────┴──────┴──────┴──────┴──────┴────────────────────────┘
//      ↑                                   ↑                        ↑
//     begin()                             end()              capacity_end
//
//  ▌ 所屬類別
//  Sequence container (序列容器)
//
//  ▌ 時間複雜度
//      隨機存取 (operator[], at)         O(1)
//      末端 push_back / pop_back         平均 O(1) (攤銷,amortized)
//      中間 / 前端 insert / erase        O(n)
//      搜尋 (linear search)              O(n)
//      size / empty / capacity           O(1)
//
//  ▌ 與其他 container 的比較
//      vector  vs  array   : array 大小固定 (compile-time),不可成長
//      vector  vs  deque   : deque 兩端皆 O(1),但記憶體不連續、cache 較差
//      vector  vs  list    : list 任意位置 insert/erase 為 O(1) 但無隨機存取
//      vector  vs  C array : vector 自動管理記憶體,可動態成長
//
//  ▌ 適用情境
//      ✅ 元素數量會變動,但主要操作集中在尾端
//      ✅ 需要隨機存取 (operator[])
//      ✅ 需要連續記憶體 (e.g. 傳給 C API、SIMD、memcpy)
//      ❌ 頻繁在「中間」或「前端」插入/刪除 → 改用 list / deque
//      ❌ 大小編譯期已知且不變 → 改用 array
//
//  ▌ Iterator 失效規則 (★ 非常重要)
//      • 任何「擴容」(reallocation) 都會使「所有」iterator/reference/pointer 失效
//      • push_back / emplace_back: 只有當 size() == capacity() 才會擴容並失效
//      • insert / erase:該位置「之後」(含) 的 iterator 全部失效
//      • clear:end() 以外的 iterator 失效
//      • reserve:若 new_cap > capacity() 才擴容並失效
//      • shrink_to_fit:可能擴容並失效 (實作可選)
//
// ============================================================================

/*
補充筆記：std::vector
  - vector 的核心是連續記憶體與動態擴容；push_back 可能搬移整個緩衝區。
  - 任何造成 reallocation 的操作都會讓舊 pointer、reference、iterator 失效。
  - reserve 是效能工具，不改 size；resize 才會改元素數量。
  - std::vector 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::vector
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. vector 的擴容機制是什麼？為什麼 push_back 是 amortized O(1)？
//     答：當 size() == capacity() 時再 push_back 會觸發 reallocation：配置一塊更大的新記憶體 →
//         搬移舊元素 → 解構舊元素 → 釋放舊記憶體。
//         成長倍率是 **implementation-defined**：libstdc++（GCC）約 2 倍、MSVC 約 1.5 倍，
//         **標準並未規定倍率**。以 2 倍為例，插入 n 個元素的總搬移次數為 1+2+4+...+n < 2n，
//         均攤到每次插入就是 O(1)。
//     追問：擴容之後舊的 iterator 還能用嗎？（不行，iterator / pointer / reference 全部失效；
//         未擴容時的 insert / erase 則是插入點或刪除點之後全部失效）
//
// 🔥 Q2. size() 和 capacity() 差在哪？reserve() 和 resize() 呢？
//     答：size() 是目前實際持有的元素個數，capacity() 是不觸發 reallocation 前可容納的個數，
//         恆有 size() <= capacity()。reserve(n) 只改 capacity，不改 size、不建構任何元素，也不能縮小；
//         resize(n) 改的是 size：變大時 value-initialize 補足新元素，變小時解構多餘元素。
//     追問：reserve(100) 之後能存取 v[0] 嗎？（不行，size 還是 0，是 UB）／
//         clear() 之後 capacity 會歸零嗎？（不會，要真正釋放請用 shrink_to_fit（non-binding request）
//         或 swap trick：`std::vector<T>(v).swap(v);`）
//
// 🔥 Q3. emplace_back 和 push_back 的差別？一定要用 emplace_back 嗎？
//     答：push_back 接受已建構好的物件（copy 或 move 進容器）；
//         emplace_back 接受建構子參數並以 perfect forwarding 在容器記憶體中 in-place 建構，可省一次 copy/move。
//         但「永遠用 emplace_back」是迷思：傳入已存在物件時兩者等價；
//         emplace_back 用 direct-initialization，會繞過 explicit 檢查，可能讓不該過的程式碼編譯過。
//     追問：emplace_back 的回傳值？（C++17 起回傳新元素的 reference，C++11/14 回傳 void）
//
// ⚠️ 陷阱 1. vector 擴容時，元素是用 move 還是 copy 搬移？
//     答：**取決於元素的 move constructor 是否標了 noexcept**。
//         push_back 必須提供 strong exception guarantee（失敗則容器不變）；
//         若 move constructor 沒標 noexcept，搬到一半拋例外就無法回復（元素已被掏空），
//         所以實作會保守地改用 copy。標準以 std::move_if_noexcept 表達此邏輯：
//         move ctor 為 noexcept（或型別不可 copy）才 move，否則 copy。
//         **這就是「為什麼 move constructor 一定要標 noexcept」最經典的答案。**
//     為什麼會錯：大家以為「只要型別有 move ctor，vector 擴容就一定用 move」，
//         實際上沒標 noexcept 時會默默退回 copy，效能差很多卻看不出來。
//
// ⚠️ 陷阱 2. std::vector<bool> 有什麼問題？
//     答：它是 template specialization，為省記憶體以 bit 儲存，因此不滿足 Container 的要求：
//         operator[] 回傳的不是 bool& 而是 proxy object（std::vector<bool>::reference），
//         因為 C++ 無法對 bit 取 reference。連帶問題：不能寫 `bool& b = v[0];`、
//         `&v[0]` 不是 bool*、`auto x = v[0]` 得到的是 proxy 而非 bool。
//         替代：std::vector<char>、std::deque<bool>、std::bitset（固定大小）。
//     為什麼會錯：大家預設「vector<T> 對任何 T 行為都一致」，
//         但 bool 是唯一被特化、行為不同的那個。
// ═══════════════════════════════════════════════════════════════════════════

#include <vector>       // std::vector
#include <iostream>
#include <algorithm>    // std::sort, std::find
#include <string>
#include <iterator>     // std::back_inserter
#include <stdexcept>    // std::out_of_range
#include <numeric>      // std::iota

// 為了印出方便,寫一個 helper
template <typename T>
void print(const std::vector<T>& v, const std::string& label = "") {
    if (!label.empty()) std::cout << label << ": ";
    std::cout << "[ ";
    for (const auto& e : v) std::cout << e << ' ';
    std::cout << "] (size=" << v.size()
              << ", capacity=" << v.capacity() << ")\n";
}

int main() {
    // ========================================================================
    //  1. 建構子 (Constructors)
    // ========================================================================
    // vector 提供多種建構方式,挑選正確的可避免不必要的拷貝。
    //
    // 注意:vector<int>(10, 5)   →  10 個 5
    //       vector<int>{10, 5}   →  list-initialization,得到 [10, 5]
    //       這是 C++ 最容易踩雷的「最讓人困擾的解析 (most vexing parse)」家族問題。

    std::vector<int> v1;                          // (1) 空 vector
    std::vector<int> v2(5);                       // (2) 5 個 default-init (int → 0)
    std::vector<int> v3(5, 42);                   // (3) 5 個 42
    std::vector<int> v4{1, 2, 3, 4, 5};           // (4) initializer_list
    std::vector<int> v5(v4.begin() + 1, v4.end());// (5) 由 iterator range 建構
    std::vector<int> v6(v4);                      // (6) copy
    std::vector<int> v7(std::move(v6));           // (7) move (v6 被掏空)

    print(v1, "v1 (default)        ");
    print(v2, "v2 (size=5)         ");
    print(v3, "v3 (5 個 42)        ");
    print(v4, "v4 (initializer)    ");
    print(v5, "v5 (range copy)     ");
    print(v6, "v6 (after move from)");  // 移動後通常為空,但「未明確指定」,別依賴內容
    print(v7, "v7 (move 來的)      ");

    // ========================================================================
    //  2. assign — 整批替換內容
    // ========================================================================
    // 三種多載: count + value / iterator range / initializer_list
    // 行為等同 clear() 後重新填入,但更有效率 (可重用既有 capacity)。

    std::vector<int> a;
    a.assign(3, 99);              // [99, 99, 99]
    print(a, "assign(3, 99)       ");
    a.assign({7, 8, 9, 10});      // [7, 8, 9, 10]
    print(a, "assign({...})       ");

    // ========================================================================
    //  3. 元素存取 (Element access)
    // ========================================================================
    //   operator[](i)  : 不檢查邊界 → 越界是 UB (undefined behavior)
    //   at(i)          : 會檢查邊界 → 越界丟 std::out_of_range
    //   front() / back(): 取頭/尾,空 vector 呼叫是 UB
    //   data()          : 取得指向第一個元素的原始指標,可傳給 C API

    std::vector<int> e{10, 20, 30, 40};
    std::cout << "\n[元素存取]\n";
    std::cout << "e[1]      = " << e[1] << '\n';        // 20
    std::cout << "e.at(2)   = " << e.at(2) << '\n';     // 30
    std::cout << "e.front() = " << e.front() << '\n';   // 10
    std::cout << "e.back()  = " << e.back() << '\n';    // 40

    try {
        std::cout << e.at(100) << '\n';                 // 會丟例外
    } catch (const std::out_of_range& ex) {
        std::cout << "at(100) 例外: " << ex.what() << '\n';
    }

    // data() 可以拿到底層連續記憶體,對 C API 很重要
    int* raw = e.data();
    std::cout << "raw[0] via data() = " << raw[0] << '\n';

    // ========================================================================
    //  4. Iterators
    // ========================================================================
    //   begin / end          (正向)
    //   rbegin / rend        (反向)
    //   cbegin / cend / crbegin / crend  (const,C++11)
    //
    // vector 的 iterator 屬於 random access iterator,可用 +、-、< 等運算。

    std::vector<int> it{1, 2, 3, 4, 5};
    std::cout << "\n[正向走訪]   ";
    for (auto it1 = it.begin(); it1 != it.end(); ++it1) std::cout << *it1 << ' ';
    std::cout << "\n[反向走訪]   ";
    for (auto it2 = it.rbegin(); it2 != it.rend(); ++it2) std::cout << *it2 << ' ';
    std::cout << "\n[const 走訪] ";
    for (auto it3 = it.cbegin(); it3 != it.cend(); ++it3) std::cout << *it3 << ' ';
    std::cout << '\n';

    // ========================================================================
    //  5. 容量 (Capacity)
    // ========================================================================
    //   empty()         : O(1),size == 0 嗎?
    //   size()          : O(1),目前元素數
    //   max_size()      : 理論上限 (取決於 allocator / 平台)
    //   capacity()      : 已配置的空間 (≥ size)
    //   reserve(n)      : 預留至少 n 的空間,n ≤ capacity 則無動作
    //   shrink_to_fit() : 釋放多餘 capacity (實作可選擇不縮小)
    //
    // ★ Tip:若你已知資料量,先 reserve() 可避免多次擴容,效能差異可達數倍。

    std::vector<int> cap;
    std::cout << "\n[Capacity 觀察]\n";
    for (int i = 0; i < 10; ++i) {
        cap.push_back(i);
        std::cout << "  push " << i
                  << " → size=" << cap.size()
                  << ", capacity=" << cap.capacity() << '\n';
    }
    cap.reserve(100);
    std::cout << "  reserve(100) → capacity=" << cap.capacity() << '\n';
    cap.shrink_to_fit();
    std::cout << "  shrink_to_fit → capacity=" << cap.capacity() << '\n';

    // max_size():理論上可容納的最大元素數,通常是 size_t 上限 / sizeof(T)
    // 實務上幾乎用不到,但屬於標準 API,提供給 generic code 查詢上限。
    std::cout << "  max_size() = " << cap.max_size() << '\n';

    // get_allocator():取得目前的 allocator 物件
    // 一般使用者很少直接用,主要給「想自己寫 allocator-aware 容器」的人。
    auto alloc = cap.get_allocator();
    int* p_alloc = alloc.allocate(3);
    alloc.deallocate(p_alloc, 3);
    std::cout << "  get_allocator() OK (alloc/deallocate test passed)\n";

    // ========================================================================
    //  6. 修改 (Modifiers)
    // ========================================================================

    // ──── clear ────
    // 清空所有元素,size → 0,但 capacity 通常不變
    std::vector<int> m1{1, 2, 3};
    m1.clear();
    print(m1, "after clear()       ");

    // ──── insert ────
    // 在指定位置「前」插入,回傳指向第一個被插入元素的 iterator
    // 注意:可能造成擴容 → 後續 iterator 全部失效
    std::vector<int> m2{1, 2, 5};
    m2.insert(m2.begin() + 2, 3);              // [1,2,3,5]
    m2.insert(m2.begin() + 3, 2, 4);           // [1,2,3,4,4,5]
    m2.insert(m2.end(), {6, 7});               // [1,2,3,4,4,5,6,7]
    print(m2, "after insert()      ");

    // ──── emplace ────
    // 直接在指定位置「就地建構」,避免一次拷貝/搬移
    std::vector<std::string> m3;
    m3.emplace(m3.end(), 5, 'A');              // 用 string(5,'A') 建構 → "AAAAA"
    print(m3, "after emplace()     ");

    // ──── erase ────
    // 刪除單一 / 區間。回傳「被刪除元素的下一個」iterator
    // 經典慣用法:erase-remove idiom
    std::vector<int> m4{1, 2, 3, 2, 4, 2, 5};
    m4.erase(std::remove(m4.begin(), m4.end(), 2), m4.end()); // 移除所有 2
    print(m4, "erase-remove all 2  ");
    // C++20 起可改用 std::erase(m4, 2);

    // ──── push_back / emplace_back ────
    // push_back: 拷貝 / 搬移既有物件
    // emplace_back: 直接傳建構參數,在容器內就地建構 (zero-copy)
    std::vector<std::pair<int,std::string>> m5;
    m5.push_back({1, "one"});                  // 會建構臨時 pair 再 move
    m5.emplace_back(2, "two");                 // 直接在 vector 內建構,較快
    std::cout << "emplace_back demo: ";
    for (auto& [k,v] : m5) std::cout << '(' << k << ',' << v << ") ";
    std::cout << '\n';

    // ──── pop_back ────
    // 刪除最後一個元素;空 vector 呼叫是 UB
    m4.pop_back();
    print(m4, "after pop_back()    ");

    // ──── resize ────
    // 改變 size:變大 → 用 default 或指定值補;變小 → 砍掉尾巴
    std::vector<int> m6{1, 2, 3};
    m6.resize(5);                              // [1,2,3,0,0]
    print(m6, "resize(5)           ");
    m6.resize(7, 9);                           // [1,2,3,0,0,9,9]
    print(m6, "resize(7, 9)        ");
    m6.resize(2);                              // [1,2]
    print(m6, "resize(2)           ");

    // ──── swap ────
    // O(1) 交換兩個 vector,只是換內部指標
    std::vector<int> s1{1,2,3}, s2{9,8};
    s1.swap(s2);
    print(s1, "s1 after swap       ");
    print(s2, "s2 after swap       ");

    // 非成員 std::swap(a, b) — 標準也提供獨立函式版本
    // (透過 ADL 找到 std::swap,效果與成員 swap 相同,O(1))
    std::swap(s1, s2);
    print(s1, "after std::swap     ");

    // ========================================================================
    //  7. 比較運算子 (==, !=, <, <=, >, >=, <=>)
    // ========================================================================
    // 字典序比較 (lexicographical),C++20 起有 spaceship operator <=>
    std::vector<int> c1{1,2,3}, c2{1,2,4};
    std::cout << "\n[比較] c1 < c2 ? " << std::boolalpha << (c1 < c2) << '\n';

    // ========================================================================
    //  8. C++20 / C++23 新增 free functions
    // ========================================================================
    // std::erase(v, value)            — 直接刪除所有等於 value 的元素
    // std::erase_if(v, predicate)     — 刪除符合條件的元素
    std::vector<int> n1{1,2,3,4,5,6};
    std::erase_if(n1, [](int x){ return x % 2 == 0; });
    print(n1, "erase_if(偶數)      ");

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls) ★必看
    // ========================================================================
    //
    //  (1) push_back 後 iterator 失效
    //      vector<int> v{1,2,3};
    //      auto it = v.begin();
    //      v.push_back(4);   // 若擴容,it 失效!不能再用
    //
    //  (2) 在 for 迴圈中 erase
    //      錯: for (auto it = v.begin(); it != v.end(); ++it)
    //              if (*it == x) v.erase(it);   // it 失效後 ++it 是 UB
    //      對: for (auto it = v.begin(); it != v.end(); )
    //              if (*it == x) it = v.erase(it); else ++it;
    //
    //  (3) vector<bool> 不是真正的 vector
    //      它是個 bit-packed 特化版,operator[] 回傳的是 proxy 物件,
    //      不能做 vector<bool>::reference& r = v[0]; 之類的操作。
    //      若需要 bool 陣列,改用 vector<char> 或 std::bitset / std::deque<bool>。
    //
    //  (4) size_t 為 unsigned,反向迴圈要小心
    //      錯: for (size_t i = v.size() - 1; i >= 0; --i)  // 永遠成立 → 無限迴圈
    //      對: for (size_t i = v.size(); i-- > 0; )
    //
    //  (5) reserve vs resize
    //      reserve(n) 只配置空間,size 不變;
    //      resize(n)  會真的把 size 變成 n,用 default 值或指定值填補。
    //
    //  (6) 不要對「迴圈中正在迭代的 vector」做 push_back
    //      可能擴容後 reference / iterator 全部失效。
    //
    //  (7) 預設 capacity 成長策略不是標準規定
    //      多數實作為 1.5 倍 (MSVC) 或 2 倍 (libstdc++ / libc++),
    //      所以「攤銷 O(1)」的常數因子在不同編譯器可能略有差異。

    // ========================================================================
    //  10. 最佳實踐 (Best Practice)
    // ========================================================================
    //
    //  • 已知大概大小 → 先 reserve() 避免多次 realloc
    //  • 建構複雜物件 → 優先用 emplace_back / emplace
    //  • 移除元素 → 用 erase-remove idiom 或 C++20 的 std::erase / std::erase_if
    //  • 與 C API 互動 → 用 v.data() 取得指標,而不是 &v[0] (空 vector 時 UB)
    //  • 大量資料移動 → 確保元素 type 有 noexcept move constructor,
    //    否則 vector 擴容時會選擇拷貝以維持 strong exception guarantee
    //  • 不需要修改 → 用 const vector& 傳遞,避免拷貝
    //  • 元素數量固定 → 改用 std::array,連 heap 配置都省了
    //  • 需要 stable 的 iterator → 別用 vector,改用 list 或 deque

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // vector 是面試題最常出現的 container。下面四題都是「雙指標 / 一次走訪」
    // 經典套路,理解這幾題後,絕大多數陣列題都能套用同樣的模板。

    // ──── LC 121: Best Time to Buy and Sell Stock (買賣股票最佳時機) ────
    // 給定每天股價,求一次買一次賣最大利潤 → 維護「目前最低買入價」與
    // 「最大利潤」一次走訪即可。經典 DP 縮減為 O(1) 空間。
    {
        std::vector<int> prices{7, 1, 5, 3, 6, 4};
        int min_price = prices[0], max_profit = 0;
        for (int p : prices) {
            min_price = std::min(min_price, p);                // 更新買點
            max_profit = std::max(max_profit, p - min_price);  // 試著賣出
        }
        std::cout << "\n[LC121 Stock] max profit = " << max_profit << '\n';
        // 預期輸出: 5  (價格 1 買入,價格 6 賣出)
    }

    // ──── LC 283: Move Zeroes (移動零) ────
    // 把所有 0 移到末端,非零元素相對順序保持不變,要求 in-place。
    // 經典「快慢指標」: write 只在遇到非零時前進,read 走完整個陣列。
    {
        std::vector<int> nums{0, 1, 0, 3, 12};
        size_t write = 0;
        for (size_t read = 0; read < nums.size(); ++read) {
            if (nums[read] != 0) std::swap(nums[write++], nums[read]);
        }
        print(nums, "[LC283 Move Zeroes]");
        // 預期輸出: [ 1 3 12 0 0 ]
    }

    // ──── LC 26: Remove Duplicates from Sorted Array (移除排序陣列重複項) ────
    // 在已排序 vector 上原地刪除重複,回傳新長度。雙指標模板題。
    // C++ 中 std::unique + erase 也能做,但這裡示範手刻雙指標,理解其本質。
    {
        std::vector<int> nums{1, 1, 2, 2, 3, 4, 4, 5};
        size_t k = 0;
        for (size_t i = 0; i < nums.size(); ++i) {
            if (k == 0 || nums[i] != nums[k - 1]) nums[k++] = nums[i];
        }
        nums.resize(k);
        print(nums, "[LC26 Unique]      ");
        // 預期輸出: [ 1 2 3 4 5 ]
        // 等價寫法 (善用 STL): nums.erase(std::unique(nums.begin(), nums.end()), nums.end());
    }

    // ──── LC 167: Two Sum II - Input Array Is Sorted (兩數之和 II) ────
    // 已排序的陣列中找兩數之和等於 target,回傳下標 (1-based)。
    // 經典「對撞指標」: 太小 → 左指標右移; 太大 → 右指標左移。O(n) 解法。
    {
        std::vector<int> nums{2, 7, 11, 15};
        int target = 18;
        int l = 0, r = (int)nums.size() - 1;
        while (l < r) {
            int s = nums[l] + nums[r];
            if (s == target) break;
            if (s < target) ++l; else --r;
        }
        std::cout << "[LC167 Two Sum II] 1-based indices = ("
                  << (l + 1) << ',' << (r + 1) << ")\n";
        // 預期輸出: (2, 3)  (因為 7 + 11 == 18)
    }

    // ──── LC 88: Merge Sorted Array (合併兩個已排序陣列) ────
    // 給 nums1 (長度 m + n,後 n 個是 0 佔位) 與 nums2 (長度 n),將兩者合併到 nums1。
    // 思路:由「後往前」雙指針填入,避免覆蓋 nums1 尚未處理的元素。O(m+n)。
    // 為何用 vector:題目以連續記憶體陣列為背景,vector 提供 O(1) 隨機存取剛好。
    {
        std::vector<int> nums1{1, 2, 3, 0, 0, 0};
        std::vector<int> nums2{2, 5, 6};
        int m = 3, n = 3;
        int i = m - 1, j = n - 1, k = m + n - 1;
        while (j >= 0) {
            if (i >= 0 && nums1[i] > nums2[j]) nums1[k--] = nums1[i--];
            else                               nums1[k--] = nums2[j--];
        }
        print(nums1, "[LC88 Merge]       ");
        // 預期輸出: [ 1 2 2 3 5 6 ]
    }

    // ========================================================================
    //  12. 實戰範例:事件時間序列分析 (Event Log 統計)
    // ========================================================================
    // 真實場景:後端服務每秒收到大量事件,以 vector<long long> 紀錄時間戳。
    // 需求:統計最近 N 秒內的事件數量、最小/最大時間差、平均間隔。
    // vector 是這類「append-only 時間軸」最常用的容器:
    //   • push_back 攤銷 O(1),寫入快
    //   • 連續記憶體 → cache 友善,適合線性掃描
    //   • 已排序 (按時間) → 可用 lower_bound 做時間範圍查詢 O(log n)
    {
        std::vector<long long> events{100, 103, 110, 115, 120, 130, 145};
        long long now = events.back();
        long long window = 30;  // 最近 30 秒

        // 用 lower_bound 找到「時間 >= now - window」的第一個位置
        auto it_begin = std::lower_bound(events.begin(), events.end(), now - window);
        auto recent_cnt = std::distance(it_begin, events.end());

        // 計算事件間平均間隔 (相鄰差)
        long long total_gap = 0;
        for (size_t i = 1; i < events.size(); ++i) total_gap += events[i] - events[i-1];
        double avg_gap = events.size() > 1 ? (double)total_gap / (events.size() - 1) : 0.0;

        std::cout << "[Event Log] 最近 " << window << " 秒事件數 = " << recent_cnt
                  << ",平均間隔 = " << avg_gap << " 秒\n";
        // 預期輸出: 最近 30 秒事件數 = 4,平均間隔 = 7.5 秒
    }

    std::cout << "\n=== vector demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：reserve(n) 與 resize(n) 差在哪?何時該用 reserve?
    //    A：reserve 只配置容量 (capacity),size 不變,沒有任何元素被建構;resize 會
    //       真的把 size 改成 n,缺的元素以 default 或指定值補。已知資料量時先 reserve
    //       可避免多次 realloc,效能通常差數倍。
    //
    //  Q2：為什麼 push_back 是「攤銷 O(1)」而不是嚴格 O(1)?
    //    A：當 size == capacity 時要 realloc 一塊更大的記憶體並搬移所有元素 (O(n))。
    //       但成長策略是幾何級數 (1.5 或 2 倍),平均下來每次 push_back 是 O(1)。
    //
    //  Q3：vector<bool> 為何是「臭名昭著」的特化?要怎麼避開?
    //    A：vector<bool> 是 bit-packed 特化版,operator[] 回傳 proxy 物件而非 bool&,
    //       無法取得真正的 reference / pointer,也無法用於 std::ref、SIMD、C API。
    //       若需要真正的 bool 陣列,改用 std::vector<char> 或 std::deque<bool>。
    //
    return 0;
}

/*
============================================================================
  附錄 A:vector 完整 member function 一覽 (cppreference 對照表)
============================================================================
  Constructors:
      vector()
      vector(const Allocator&)
      vector(size_type n, const T& value, const Allocator& = Allocator())
      vector(size_type n)                                    // C++11
      vector(InputIt first, InputIt last, ...)
      vector(const vector&)        / vector(const vector&, const Allocator&)
      vector(vector&&)             / vector(vector&&, const Allocator&)
      vector(std::initializer_list<T>, ...)
      vector(std::from_range_t, R&&, ...)                    // C++23

  Destructor: ~vector()

  operator=:
      operator=(const vector&)
      operator=(vector&&)          (C++11, noexcept on allocator傳遞性)
      operator=(std::initializer_list<T>)

  assign / assign_range (C++23)

  get_allocator()

  Element access:
      at, operator[], front, back, data

  Iterators:
      begin, end, cbegin, cend
      rbegin, rend, crbegin, crend

  Capacity:
      empty, size, max_size, reserve, capacity, shrink_to_fit

  Modifiers:
      clear
      insert, insert_range (C++23)
      emplace
      erase
      push_back
      emplace_back            → 回傳 reference (C++17 起)
      append_range (C++23)
      pop_back
      resize
      swap

  Non-member:
      operator==, !=, <, <=, >, >=, <=> (C++20)
      std::swap(vector, vector)
      std::erase(vector, value)        (C++20)
      std::erase_if(vector, predicate) (C++20)

  Deduction guides (C++17): 從 iterator pair 自動推導 T

============================================================================
  附錄 B:預期輸出片段 (供對照)
============================================================================
  v3 (5 個 42)        : [ 42 42 42 42 42 ] (size=5, capacity=5)
  v4 (initializer)    : [ 1 2 3 4 5 ] (size=5, capacity=5)
  ...
  at(100) 例外: vector::_M_range_check: __n (which is 100) >= this->size() ...
  ...
  push 0 → size=1, capacity=1
  push 1 → size=2, capacity=2
  push 2 → size=3, capacity=4
  push 3 → size=4, capacity=4
  push 4 → size=5, capacity=8
  ...
  erase_if(偶數)      : [ 1 3 5 ] (size=3, capacity=6)
============================================================================
*/
