// =============================================================================
//  第七課：演算法（Algorithm）與容器的分離設計15.cpp
//    —  迭代器分類（Iterator Category）決定演算法可不可用
// =============================================================================
//
// 【主題資訊 Information】
//   五個迭代器分類標籤（定義在 <iterator>，C++98 起）：
//     std::input_iterator_tag          → 單次通過、唯讀
//     std::forward_iterator_tag        → 可多次通過、可讀寫
//     std::bidirectional_iterator_tag  → 再加上 --（可往回走）
//     std::random_access_iterator_tag  → 再加上 +n、-n、[]、<（O(1) 跳躍）
//     std::contiguous_iterator_tag     → **C++20 新增**，再保證記憶體連續
//
//   取得方式：std::iterator_traits<It>::iterator_category
//   標頭檔：<iterator>
//
//   各容器提供的分類：
//     vector / array / deque   → Random Access（vector/array 在 C++20 起為 Contiguous）
//     list / set / map         → Bidirectional
//     forward_list / unordered_* → Forward
//     istream_iterator         → Input
//
// 【詳細解釋 Explanation】
//
// 【1. 這是整個第 7 課的收束點】
// 前面 14 個檔案反覆出現同一句話：「這個演算法需要 XX Iterator」。
// 本檔要把它講清楚：**迭代器分類是一份「能力清單」，
// 演算法宣告自己需要哪些能力，容器宣告自己提供哪些能力，
// 兩者在編譯期對帳。**
// 這就是分離設計能成立的關鍵——演算法不需要認識容器，
// 只需要知道「你給我的 iterator 有沒有我要的能力」。
//
// 【2. 五個分類是「能力遞增」的階梯】
//     Input          : *it（讀）、++it、!=        —— 只能往前走一次
//     Forward        : Input + 可重複走訪、可寫入
//     Bidirectional  : Forward + --it             —— 可以往回走
//     Random Access  : Bidirectional + it+n、it-n、it[n]、it1<it2  —— O(1) 跳躍
//     Contiguous     : Random Access + 保證元素在記憶體中連續（C++20）
// 每一層都**包含**前一層的所有能力。所以要求 Input Iterator 的演算法
// （如 std::find）人人可用；要求 Random Access 的（如 std::sort）
// 就只有 vector / deque / array 能用。
//
// 【3. 為什麼是「編譯期錯誤」而不是「執行期變慢」】
//     std::sort(lst.begin(), lst.end());   // list → 編譯失敗，不是變慢
// std::sort 內部會寫出 first + (last - first) / 2 這樣的程式碼。
// list 的 iterator 根本沒有定義 operator+ 與 operator-，
// 所以編譯器在實例化 template 時就找不到對應的運算子 → 編譯錯誤。
// 這正是「靜態多型」的好處：**不合法的組合在編譯期就被擋下來**，
// 不會等到執行期才出問題。
// （代價是錯誤訊息可能很長，因為它來自 template 實例化的深處。
//   C++20 的 concepts 就是為了改善這種錯誤訊息而設計的。）
//
// 【4. 但有一種情況更危險：能編譯，只是默默變慢】
// 並非所有不匹配都會編譯失敗。有些演算法**只要求較低的分類**，
// 但在較高分類上效率好得多：
//     std::lower_bound  → 只要 Forward Iterator，但對 list 是 O(N) 而非 O(log N)
//     std::distance     → 只要 Input Iterator，但對 list 是 O(N) 而非 O(1)
//     std::advance      → 同上
// 這些對 list 都能編譯、都能跑出正確答案，**只是效能完全不同**。
// 這比編譯失敗更難發現，因為程式「看起來是對的」。
// 判斷準則：**演算法需要「跳躍」時，非隨機存取的容器就會退化。**
//
// 【5. 標籤分派（tag dispatch）：STL 如何依能力自動選最佳實作】
// std::advance 的實作大致是：
//     advance(it, n, input_iterator_tag)         { while (n--) ++it; }        // O(n)
//     advance(it, n, random_access_iterator_tag) { it += n; }                 // O(1)
//     // 對外統一入口，靠 iterator_category 選擇上面哪一個
// 這叫**標籤分派**：用型別（tag）在編譯期選擇實作，零執行期成本。
// std::rotate、std::distance、std::reverse 都用了同樣的手法
// （見第 10 個檔案 rotate 的三種實作策略）。
// C++20 之後可以改用 concepts 與 if constexpr 寫得更清楚，但機制相同。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 std::list 提供 Bidirectional 而不是 Random Access
//   雙向鏈結串列的每個節點只知道前後鄰居，要跳到第 n 個元素只能一步步走，
//   本質上是 O(n)。若硬要提供 operator+ 而內部用迴圈實作，
//   會讓「看起來 O(1) 的語法」實際上是 O(n)，
//   使用者寫出 for (i...) lst.begin() + i 就會變成 O(N²) 而不自知。
//   **STL 的態度是：做不到 O(1) 就不要提供那個語法。**
//   這是一個重要的 API 設計原則：語法的複雜度應該誠實反映實際成本。
//
// (B) C++20 的 contiguous_iterator_tag 解決什麼問題
//   Random Access 保證 O(1) 跳躍，但**不保證記憶體連續**（deque 就是反例：
//   它由多個分段緩衝區組成，能 O(1) 跳躍但不連續）。
//   有些操作需要真正的連續記憶體——例如把資料傳給 C API（memcpy、write()）
//   或建立 std::span。C++20 因此新增這一層，讓 vector/array/string
//   能與 deque 區分開來。
//
// (C) 這些「需求」在 C++20 之前只是文件約定
//   C++17 以前，iterator category 只是 traits 上的標籤，
//   演算法並沒有正式檢查它——不合法時的錯誤來自「某個運算子找不到」，
//   訊息冗長難懂。C++20 的 concepts（std::random_access_iterator 等）
//   把需求正式寫進簽名，錯誤訊息才變成「不滿足 concept」這種可讀的形式。
//
// 【注意事項 Pay Attention】
// 1. **std::sort / nth_element / partial_sort 需要 Random Access**：
//    list / forward_list / set 一律編譯失敗，要用成員函式。
// 2. **std::reverse 需要 Bidirectional**：forward_list 編譯失敗，用 flst.reverse()。
// 3. **能編譯不代表該用**：lower_bound / distance / advance 對 list
//    能跑但退化成 O(N)，這是「默默變慢」型的陷阱。
// 4. **set / map 的元素透過 iterator 取得時是 const 的**，
//    所以即使分類足夠，任何「會寫入元素」的演算法（sort、replace、remove）
//    也不能用——這是另一個獨立的限制。
// 5. 通用程式碼要算距離請用 std::distance，不要用 it2 - it1
//    （後者只有 Random Access 能編譯）。
// 6. C++20 起可用 concepts（std::random_access_iterator）取得更好的錯誤訊息。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器分類
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 五種迭代器分類分別是什麼？各自多了什麼能力？
//     答：Input（單次通過、唯讀：*、++、!=）→ Forward（可重複走訪、可寫入）
//         → Bidirectional（多了 --，可往回走）→ Random Access（多了 +n、-n、[]、<，
//         O(1) 跳躍）→ Contiguous（C++20，再保證記憶體連續）。
//         每層包含前一層的全部能力，是一個遞增的階梯。
//     追問：deque 是 Random Access 但不是 Contiguous，為什麼？
//         → deque 由多段緩衝區組成，能靠索引換算 O(1) 跳到任意位置，
//         但這些段在記憶體中並不相鄰，所以不能安全地當成一塊連續緩衝區
//         傳給 memcpy 或 C API。
//
// 🔥 Q2. std::sort(lst.begin(), lst.end()) 對 std::list 是「編譯錯誤」
//        還是「執行變慢」？為什麼？
//     答：**編譯錯誤**。std::sort 內部要計算中點（first + (last-first)/2）、
//         要做隨機跳躍，而 list 的 iterator 根本沒有定義 operator+ / operator-，
//         template 實例化時就找不到運算子而失敗。
//         這是靜態多型的優點：不合法的組合在編譯期被擋下，不會拖到執行期。
//     追問：那 list 要怎麼排序？→ 用成員函式 lst.sort()，它用 merge sort
//         **只改節點指標、不搬移元素值**，所以指向元素的 iterator 與 reference
//         在排序後依然有效——這是 std::sort 做不到的。
//
// 🔥 Q3. 什麼是標籤分派（tag dispatch）？舉一個 STL 中的例子。
//     答：用 iterator_category 這個**型別標籤**在編譯期選擇不同實作，零執行期成本。
//         例如 std::advance：對 Input Iterator 用 while(n--) ++it（O(n)），
//         對 Random Access Iterator 用 it += n（O(1)）；
//         對外只有一個入口，由 traits 上的標籤決定走哪一版。
//         std::distance、std::rotate、std::reverse 都用同樣手法。
//     追問：C++20 有更好的寫法嗎？→ 有，改用 concepts 加 if constexpr，
//         意圖更明確、錯誤訊息也好得多，但底層機制（編譯期依能力選實作）相同。
//
// ⚠️ 陷阱. std::lower_bound 對 std::list「能編譯」，所以可以放心用嗎？
//     答：不行。它只要求 Forward Iterator，所以確實能編譯，
//         **比較次數也真的是 O(log N)**——但二分搜尋每步要跳到中點，
//         而 list 跳 k 步就要走 k 次 ++，**iterator 前進的總成本是 O(N)**。
//         實際上比直接用 std::find 線性掃還慢（多了計算中點的開銷）。
//     為什麼會錯：大家把「能編譯」當成「設計上支援」。
//         但 iterator category 只保證**語法可用**，不保證**複雜度符合預期**。
//         這類「默默變慢」的陷阱比編譯錯誤危險得多——
//         程式跑得出正確答案，效能問題要到資料量變大才會浮現。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <forward_list>
#include <deque>
#include <set>
#include <algorithm>
#include <iterator>
#include <type_traits>

// -----------------------------------------------------------------------------
// 【日常實務範例 1】寫一個「依容器能力自動選最佳策略」的通用函式
//   情境：工具函式庫要提供一個「取第 N 個元素」的 API，呼叫端可能傳任何容器。
//         對 vector 應該直接跳（O(1)），對 list 只能一步步走（O(N)），
//         而且應該讓呼叫端知道這次付出的成本。
//   為什麼用到本主題：這正是 STL 內部標籤分派的縮小版。
//         用 if constexpr（C++17）在**編譯期**依 iterator_category 選擇實作，
//         沒有任何執行期判斷成本——這就是 std::advance 的原理。
// -----------------------------------------------------------------------------
template <typename Iter>
std::string categoryName() {
    using Cat = typename std::iterator_traits<Iter>::iterator_category;
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Cat>) {
        return "Random Access";
    } else if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag, Cat>) {
        return "Bidirectional";
    } else if constexpr (std::is_base_of_v<std::forward_iterator_tag, Cat>) {
        return "Forward";
    } else {
        return "Input";
    }
}

// 依 iterator 能力選最佳實作（標籤分派的 if constexpr 版本）
template <typename Iter>
void describeNthAccess(const std::string& containerName, Iter first, Iter last,
                       std::size_t n) {
    using Cat = typename std::iterator_traits<Iter>::iterator_category;

    std::cout << "  " << containerName << " (" << categoryName<Iter>() << "): ";
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag, Cat>) {
        // O(1)：直接跳
        if (n < static_cast<std::size_t>(std::distance(first, last))) {
            std::cout << "第 " << n << " 個 = " << *(first + static_cast<std::ptrdiff_t>(n))
                      << "  [直接跳躍，O(1)]" << std::endl;
        }
    } else {
        // O(N)：只能一步步走
        auto it = first;
        std::size_t steps = 0;
        while (it != last && steps < n) { ++it; ++steps; }
        if (it != last) {
            std::cout << "第 " << n << " 個 = " << *it
                      << "  [逐步前進 " << steps << " 次，O(N)]" << std::endl;
        }
    }
}

int main() {
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int> lst = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};

    // find：只需要 Input Iterator（所有容器都能用）, 但效率可能不同
    std::cout << "=== find（Input Iterator）===" << std::endl;
    std::cout << "vector: " << (*std::find(vec.begin(), vec.end(), 8)) << std::endl;
    std::cout << "list: " << (*std::find(lst.begin(), lst.end(), 8)) << std::endl;
    std::cout << "forward_list: " << (*std::find(flst.begin(), flst.end(), 8)) << std::endl;

    // reverse：需要 Bidirectional Iterator, list 和 vector 支援，但 forward_list 不支援
    std::cout << "\n=== reverse（Bidirectional Iterator）===" << std::endl;
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 錯誤！forward_list 只有 Forward Iterator

    std::cout << "vector reversed: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    flst.reverse();     // ✓ forward_list 用自己的成員函式（改指標方向，不需往回走）
    std::cout << "forward_list reversed(成員函式): ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;

    // sort：需要 Random Access Iterator, vector 支援，但 list 和 forward_list 不支援
    std::cout << "\n=== sort（Random Access Iterator）===" << std::endl;
    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());   // 編譯錯誤！list 的 iterator 沒有 operator+ / -

    // list 和 forward_list 有自己的 sort（merge sort，只改節點指標、不搬移元素值）
    lst.sort();
    flst.sort();

    std::cout << "All sorted successfully!" << std::endl;
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "list:   ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // ★ 用 iterator_traits 印出各容器實際提供的分類
    std::cout << "\n=== 各容器的迭代器分類（編譯期查得）===" << std::endl;
    std::cout << "  vector<int>       : " << categoryName<std::vector<int>::iterator>() << std::endl;
    std::cout << "  deque<int>        : " << categoryName<std::deque<int>::iterator>() << std::endl;
    std::cout << "  list<int>         : " << categoryName<std::list<int>::iterator>() << std::endl;
    std::cout << "  set<int>          : " << categoryName<std::set<int>::iterator>() << std::endl;
    std::cout << "  forward_list<int> : " << categoryName<std::forward_list<int>::iterator>() << std::endl;
    std::cout << "  int*  (原生指標)   : " << categoryName<int*>() << std::endl;

    // ★ 能力對照表：哪些演算法可用
    std::cout << "\n=== 演算法需求 vs 容器能力 ===" << std::endl;
    std::cout << "  演算法          需求分類         vector  list  forward_list" << std::endl;
    std::cout << "  std::find       Input             可      可      可" << std::endl;
    std::cout << "  std::reverse    Bidirectional     可      可    編譯失敗" << std::endl;
    std::cout << "  std::sort       Random Access     可    編譯失敗 編譯失敗" << std::endl;
    std::cout << "  std::lower_bound Forward          可   可但O(N)  可但O(N)" << std::endl;
    std::cout << "  (最後一列是「能編譯但會默默變慢」的陷阱)" << std::endl;

    // ★ 標籤分派實作：同一個函式對不同容器自動選最佳策略
    std::cout << "\n=== 依 iterator 能力自動選實作（標籤分派）===" << std::endl;
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    std::list<int> l2 = {10, 20, 30, 40, 50};
    std::forward_list<int> f2 = {10, 20, 30, 40, 50};
    describeNthAccess("vector      ", v2.begin(), v2.end(), 3);
    describeNthAccess("list        ", l2.begin(), l2.end(), 3);
    describeNthAccess("forward_list", f2.begin(), f2.end(), 3);

    // ★ 通用程式碼算距離要用 std::distance，不能用 it2 - it1
    std::cout << "\n=== 通用程式碼請用 std::distance ===" << std::endl;
    auto vit = std::find(v2.begin(), v2.end(), 30);
    auto lit = std::find(l2.begin(), l2.end(), 30);
    std::cout << "  vector 中 30 的位置: " << std::distance(v2.begin(), vit)
              << "  [Random Access → O(1)]" << std::endl;
    std::cout << "  list   中 30 的位置: " << std::distance(l2.begin(), lit)
              << "  [Bidirectional → O(N)]" << std::endl;
    std::cout << "  (vit - v2.begin() 只有 vector 能編譯；std::distance 兩者皆可)"
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第七課：演算法（Algorithm）與容器的分離設計15.cpp -o demo15

// === 預期輸出 ===
// === find（Input Iterator）===
// vector: 8
// list: 8
// forward_list: 8
//
// === reverse（Bidirectional Iterator）===
// vector reversed: 9 1 8 2 5 
// forward_list reversed(成員函式): 9 1 8 2 5 
//
// === sort（Random Access Iterator）===
// All sorted successfully!
// vector: 1 2 5 8 9 
// list:   1 2 5 8 9 
//
// === 各容器的迭代器分類（編譯期查得）===
//   vector<int>       : Random Access
//   deque<int>        : Random Access
//   list<int>         : Bidirectional
//   set<int>          : Bidirectional
//   forward_list<int> : Forward
//   int*  (原生指標)   : Random Access
//
// === 演算法需求 vs 容器能力 ===
//   演算法          需求分類         vector  list  forward_list
//   std::find       Input             可      可      可
//   std::reverse    Bidirectional     可      可    編譯失敗
//   std::sort       Random Access     可    編譯失敗 編譯失敗
//   std::lower_bound Forward          可   可但O(N)  可但O(N)
//   (最後一列是「能編譯但會默默變慢」的陷阱)
//
// === 依 iterator 能力自動選實作（標籤分派）===
//   vector       (Random Access): 第 3 個 = 40  [直接跳躍，O(1)]
//   list         (Bidirectional): 第 3 個 = 40  [逐步前進 3 次，O(N)]
//   forward_list (Forward): 第 3 個 = 40  [逐步前進 3 次，O(N)]
//
// === 通用程式碼請用 std::distance ===
//   vector 中 30 的位置: 2  [Random Access → O(1)]
//   list   中 30 的位置: 2  [Bidirectional → O(N)]
//   (vit - v2.begin() 只有 vector 能編譯；std::distance 兩者皆可)
