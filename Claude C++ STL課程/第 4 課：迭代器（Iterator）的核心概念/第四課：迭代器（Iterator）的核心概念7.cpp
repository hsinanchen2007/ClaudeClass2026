// =============================================================================
//  第四課：迭代器的核心概念 7  —  迭代器作為演算法與容器之間的橋樑（含子區間）
// =============================================================================
//
// 【主題資訊 Information】
//   簽名：
//     InputIt   find (InputIt first, InputIt last, const T& value);   // <algorithm>
//     ptrdiff_t count(InputIt first, InputIt last, const T& value);   // <algorithm>
//   標準版本：兩者皆 C++98；C++20 另提供 std::ranges::find(rng, value)
//             與 projection 版本 std::ranges::find(rng, value, &S::member)。
//   複雜度：find / count 皆為 O(N)，最多做 N 次比較（find 找到即停）。
//   關鍵性質：兩者只要求 **Input Iterator**（能 *it、++it、比較），
//             因此 vector / list / deque / forward_list / set / istream 全部適用。
//
// 【詳細解釋 Explanation】
//
// 【1. 「橋樑」這個比喻的精確意思】
//   演算法的簽名裡完全沒有容器的影子：
//       template <class InputIt, class T>
//       InputIt find(InputIt first, InputIt last, const T& value);
//   InputIt 是什麼？編譯器在實例化時才知道。它可能是 vector 的指標包裝、
//   可能是 list 的節點指標、也可能是 istream_iterator。
//   find 唯一做的事是：
//       while (first != last) { if (*first == value) return first; ++first; }
//   它只用到 !=、*、++ 三個操作 —— 這正是 Input Iterator 的全部要求。
//   於是「同一份原始碼」可以服務所有提供這三個操作的型別。
//
// 【2. 為什麼傳「兩個」迭代器而不是容器】
//   除了解耦之外，最實際的好處是**子區間**：
//       std::find(v.begin() + 1, v.begin() + 5, 40);   // 只在 [1,5) 找
//   如果介面是 find(container, value)，這件事就得另外設計 API
//   （傳 offset？傳 length？各種容器的表示法還不一樣）。
//   一對迭代器把「範圍」變成一個統一、可組合的概念，
//   而且天然支援「範圍的範圍」（例如對排序後的區間再做 binary search）。
//
// 【3. 子區間的正確性責任在呼叫端】
//   v.begin() + 5 是否合法完全由你負責 —— 演算法不會檢查。
//   若 v 只有 3 個元素，v.begin() + 5 已經是未定義行為（連比較都不能做）。
//   而且判斷「找到了沒」要跟**你傳進去的 last** 比，不是跟 v.end() 比：
//       auto it = std::find(v.begin() + 1, v.begin() + 5, 40);
//       if (it != v.begin() + 5) { ... }     // 正確
//       if (it != v.end())       { ... }     // 錯！沒找到時 it 是 begin()+5，
//                                            //     它 != v.end()，會誤判成找到
//   這是子區間搜尋最常見的 bug，而且很安靜。
//
// 【4. find 的回傳值語意】
//   find 回傳「指向命中元素的迭代器」，沒找到則回傳 last。
//   注意這與 std::string::find 回傳 npos（一個整數）是兩套不同的慣例 ——
//   容器演算法用迭代器，字串成員函式用索引。混淆這兩者是常見錯誤。
//   要把迭代器轉成索引：
//       auto idx = std::distance(v.begin(), it);   // 泛型，list 是 O(n)
//       auto idx = it - v.begin();                 // 只有 Random Access 能用，O(1)
//
// 【概念補充 Concept Deep Dive】
//   「同一份程式碼」其實會被編譯成**多份**機器碼 —— 這是樣板的編譯期多型：
//     std::find(vec.begin(), vec.end(), 8)   → 實例化 find<vector<int>::iterator, int>
//     std::find(lst.begin(), lst.end(), 8)   → 實例化 find<list<int>::iterator, int>
//   兩份各自針對該迭代器最佳化：vector 版的 ++it 是指標 +4（且可能被向量化），
//   list 版的 ++it 是 node = node->next（無法向量化，且每步都可能 cache miss）。
//   所以雖然兩者都是 O(N)，實測上 vector 版通常快數倍。
//   這也解釋了 STL 的核心取捨：**用二進位膨脹與編譯時間，換取零抽象成本**。
//   若改用 Java 式的執行期多型（統一介面 + 虛擬函式），
//   就只會有一份程式碼，但每次 ++ 都要付一次虛擬呼叫。
//
// 【注意事項 Pay Attention】
//   1. 子區間搜尋的結果要跟**你傳的 last** 比較，不是容器的 end()。
//   2. v.begin() + n 只有 Random Access 迭代器能用；list 要用 std::next(it, n)。
//   3. 傳給演算法的兩個迭代器必須來自**同一個容器**且 first 能走到 last，
//      否則是未定義行為（且通常編得過，不會有任何警告）。
//   4. std::find 用 operator== 比較；自訂型別必須提供它，
//      或改用 std::find_if 搭配述詞。
//   5. 對已排序的資料用 std::find 是 O(N) 的浪費 ——
//      該用 std::binary_search / lower_bound（O(log N)，但需 Random Access
//      才能達到 O(log N)；對 list 仍是 O(N) 的走訪）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器與演算法的協作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼同一個 std::find 能同時用在 vector 和 list 上？
//     答：因為 find 的樣板參數只要求 Input Iterator 的三個操作：
//         *it（讀值）、++it（前進）、it1 != it2（比較）。
//         vector 與 list 的迭代器都提供這三個，只是內部實作不同
//         （指標 +1 vs node->next）。編譯器會為兩者各實例化一份專屬機器碼。
//     追問：那為什麼 std::sort 就不行？
//           → sort 需要 Random Access（it+n、it1-it2、it[n]）來取樞紐與分割，
//             list 的迭代器沒有這些運算子，樣板實例化會失敗 → 編譯錯誤。
//
// 🔥 Q2. std::find 沒找到時回傳什麼？跟 std::string::find 一樣嗎？
//     答：不一樣，這是兩套慣例。std::find（演算法）回傳你傳進去的 last；
//         std::string::find（成員函式）回傳 std::string::npos（一個 size_type）。
//         前者是迭代器語意，後者是索引語意。
//     追問：那子區間搜尋 find(v.begin()+1, v.begin()+5, x) 沒找到時回傳什麼？
//           → 回傳 v.begin()+5，也就是你傳的 last，**不是** v.end()。
//             所以判斷式必須寫 it != v.begin()+5。
//
// ⚠️ 陷阱. auto it = std::find(v.begin()+1, v.begin()+5, 99);
//          if (it != v.end()) { std::cout << *it; }   —— 這段錯在哪？
//     答：搜尋範圍是 [1,5)，沒找到時 find 回傳的是 v.begin()+5 而非 v.end()。
//         若 v 有 7 個元素，v.begin()+5 != v.end() 成立 → 判斷式誤判為「找到了」，
//         接著解參考 *it 讀到的是索引 5 的元素 —— 一個根本不在搜尋範圍內、
//         也不等於 99 的值。程式不會崩潰，只會安靜地給出錯誤答案。
//     為什麼會錯：把「沒找到」記成「回傳 end()」。
//         正確的規則是「回傳你傳進去的那個 last」——
//         當你傳的是完整範圍時 last 剛好等於 end()，所以平常看起來沒問題，
//         一旦改成子區間就爆了。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <iterator>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 349. Intersection of Two Arrays
//   題目：回傳兩個陣列的交集（每個元素只出現一次，順序不限）。
//   為什麼用到本主題：set_intersection 是「吃兩對迭代器區間、寫進第三個
//         輸出迭代器」的典型演算法 —— 它完全不知道來源是 vector 還是別的容器，
//         只要求兩個區間都已排序。這正是本檔「迭代器是演算法與容器之橋樑」
//         的直接應用，也順帶示範**多個區間同時參與運算**的介面設計。
//   複雜度：時間 O(N log N + M log M)（排序主導）、空間 O(min(N, M))。
// -----------------------------------------------------------------------------
std::vector<int> intersection(std::vector<int> a, std::vector<int> b) {
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    std::vector<int> out;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::back_inserter(out));
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】分頁查詢：只在「本頁區間」內做統計
//   情境：後台列表一次載入全部資料，但畫面一次只顯示一頁（例如每頁 10 筆）。
//         要在「目前這一頁」內統計符合條件的筆數，而不是整個資料集。
//   為什麼用到本主題：分頁本質就是一個子區間 [page*size, (page+1)*size)，
//         而 STL 演算法天生吃區間 —— 不需要先把該頁複製出來，
//         直接把兩個迭代器傳進去即可，零額外配置。
//   注意：邊界必須自己夾好（最後一頁可能不滿），否則 begin()+n 會越界。
// -----------------------------------------------------------------------------
struct Order {
    std::string id;
    int         amount;
    bool        paid;
};

int countPaidInPage(const std::vector<Order>& orders,
                    std::size_t page, std::size_t page_size) {
    std::size_t from = page * page_size;
    if (from >= orders.size()) return 0;                    // 頁碼超出範圍
    std::size_t to = std::min(from + page_size, orders.size());  // 夾住尾端

    auto first = orders.begin() + static_cast<std::ptrdiff_t>(from);
    auto last  = orders.begin() + static_cast<std::ptrdiff_t>(to);

    return static_cast<int>(
        std::count_if(first, last, [](const Order& o) { return o.paid; }));
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};

    // 同一個 find 演算法，用在不同容器
    std::cout << "=== std::find ===" << std::endl;

    auto vec_it = std::find(vec.begin(), vec.end(), 8);
    if (vec_it != vec.end()) {
        std::cout << "在 vector 中找到 8" << std::endl;
    }

    auto lst_it = std::find(lst.begin(), lst.end(), 8);
    if (lst_it != lst.end()) {
        std::cout << "在 list 中找到 8" << std::endl;
    }

    // 同一個 count 演算法
    std::cout << "\n=== std::count ===" << std::endl;

    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2};
    int count = static_cast<int>(std::count(data.begin(), data.end(), 2));
    std::cout << "2 出現了 " << count << " 次" << std::endl;

    // 在子區間上操作
    std::cout << "\n=== 子區間操作 ===" << std::endl;

    std::vector<int> nums = {10, 20, 30, 40, 50, 60, 70};

    // 只在 [begin+1, begin+5) 範圍內查找，即 {20, 30, 40, 50}
    auto sub_it = std::find(nums.begin() + 1, nums.begin() + 5, 40);
    if (sub_it != nums.begin() + 5) {
        std::cout << "在子區間中找到 40" << std::endl;
    }

    // 子區間搜尋最常見的 bug：拿 end() 當判斷基準
    std::cout << "\n=== 子區間判斷基準的陷阱（實際演示）===" << std::endl;
    auto miss = std::find(nums.begin() + 1, nums.begin() + 5, 99);   // 99 不在裡面
    std::cout << "  搜尋 [1,5) 找 99（不存在）" << std::endl;
    std::cout << "  正確判斷 it != begin()+5 : "
              << (miss != nums.begin() + 5 ? "誤判為找到" : "正確判定沒找到") << std::endl;
    std::cout << "  錯誤判斷 it != end()     : "
              << (miss != nums.end() ? "誤判為找到 ← BUG！" : "正確") << std::endl;
    std::cout << "  （此時 *it = " << *miss
              << "，是索引 5 的元素，根本不在搜尋範圍內）" << std::endl;

    // 迭代器 → 索引：兩種寫法與各自的複雜度
    std::cout << "\n=== 把迭代器換算成索引 ===" << std::endl;
    auto found = std::find(nums.begin(), nums.end(), 50);
    std::cout << "  it - begin()            = " << (found - nums.begin())
              << "   (Random Access, O(1))" << std::endl;
    std::cout << "  std::distance(begin,it) = " << std::distance(nums.begin(), found)
              << "   (泛型，list 上是 O(n))" << std::endl;

    // 泛型子區間：list 沒有 +n，要用 std::next
    std::cout << "\n=== list 的子區間要用 std::next ===" << std::endl;
    auto l_first = std::next(lst.begin(), 1);
    auto l_last  = std::next(lst.begin(), 4);
    std::cout << "  list [1,4) 內容: ";
    for (auto it = l_first; it != l_last; ++it) std::cout << *it << " ";
    std::cout << std::endl;
    std::cout << "  在該子區間找 8: "
              << (std::find(l_first, l_last, 8) != l_last ? "找到" : "沒找到") << std::endl;

    std::cout << "\n=== LeetCode 349. Intersection of Two Arrays ===" << std::endl;
    std::cout << "  [1,2,2,1] ∩ [2,2]     = ";
    for (int n : intersection({1, 2, 2, 1}, {2, 2})) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  [4,9,5] ∩ [9,4,9,8,4] = ";
    for (int n : intersection({4, 9, 5}, {9, 4, 9, 8, 4})) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "\n=== 日常實務：分頁區間內統計已付款訂單 ===" << std::endl;
    std::vector<Order> orders = {
        {"A001", 1200, true },  {"A002",  340, false}, {"A003", 5600, true },
        {"A004",  890, true },  {"A005", 2300, false}, {"A006",  150, true },
        {"A007", 4400, false},  {"A008",  770, true },
    };
    const std::size_t page_size = 3;
    std::size_t pages = (orders.size() + page_size - 1) / page_size;
    std::cout << "  共 " << orders.size() << " 筆，每頁 " << page_size
              << " 筆，" << pages << " 頁" << std::endl;
    for (std::size_t p = 0; p < pages; ++p) {
        std::cout << "    第 " << (p + 1) << " 頁已付款: "
                  << countPaidInPage(orders, p, page_size) << " 筆" << std::endl;
    }
    std::cout << "    超出範圍的第 99 頁: " << countPaidInPage(orders, 99, page_size)
              << " 筆（安全回傳 0，不會越界）" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念7.cpp -o demo7

// === 預期輸出 ===
// === std::find ===
// 在 vector 中找到 8
// 在 list 中找到 8
//
// === std::count ===
// 2 出現了 4 次
//
// === 子區間操作 ===
// 在子區間中找到 40
//
// === 子區間判斷基準的陷阱（實際演示）===
//   搜尋 [1,5) 找 99（不存在）
//   正確判斷 it != begin()+5 : 正確判定沒找到
//   錯誤判斷 it != end()     : 誤判為找到 ← BUG！
//   （此時 *it = 60，是索引 5 的元素，根本不在搜尋範圍內）
//
// === 把迭代器換算成索引 ===
//   it - begin()            = 4   (Random Access, O(1))
//   std::distance(begin,it) = 4   (泛型，list 上是 O(n))
//
// === list 的子區間要用 std::next ===
//   list [1,4) 內容: 2 8 1
//   在該子區間找 8: 找到
//
// === LeetCode 349. Intersection of Two Arrays ===
//   [1,2,2,1] ∩ [2,2]     = 2
//   [4,9,5] ∩ [9,4,9,8,4] = 4 9
//
// === 日常實務：分頁區間內統計已付款訂單 ===
//   共 8 筆，每頁 3 筆，3 頁
//     第 1 頁已付款: 2 筆
//     第 2 頁已付款: 2 筆
//     第 3 頁已付款: 1 筆
//     超出範圍的第 99 頁: 0 筆（安全回傳 0，不會越界）
