// =============================================================================
//  第四課：迭代器（Iterator）的核心概念 3  —  半開區間 [begin, end) 的設計
// =============================================================================
//
// 【主題資訊 Information】
//   語法：
//     auto first = c.begin();   // 指向第一個元素
//     auto last  = c.end();     // 指向「最後一個元素的下一個位置」past-the-end
//     for (auto it = first; it != last; ++it) { ... }
//   標準版本：begin()/end() 成員函式是 C++98；
//             自由函式 std::begin/std::end 是 C++11；
//             cbegin()/cend() 是 C++11；std::size() 是 C++17。
//   複雜度：begin()/end() 對所有標準容器都是 O(1)。
//           `end() - begin()` 只有 Random Access Iterator 是 O(1)，
//           其他要用 std::distance，對 list 是 O(N)。
//   標頭檔：<vector> 等容器自身；std::distance/std::size 在 <iterator>。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼是 [begin, end) 而不是 [begin, last]】
//   這是 STL 最重要也最容易被輕看的設計決策。用「尾後位置」而不是
//   「最後一個元素」表示終點，一次解決了四個問題：
//
//   (a) 空區間可以表示。若用閉區間 [first, last]，空集合要怎麼寫？
//       last 得比 first 小一格——但那個位置可能根本不存在（見第 1 檔的
//       arr-1 是 UB）。半開區間只要 begin == end 就是空，乾淨且合法。
//
//   (b) 元素個數 = end - begin，不必 +1。閉區間得寫 last - first + 1，
//       那個 +1 就是 off-by-one bug 的溫床。
//
//   (c) 相鄰子區間可以無縫接合。[a, b) 與 [b, c) 拼起來剛好是 [a, c)，
//       中間不重疊也不漏。閉區間要寫成 [a, b] 與 [b+1, c]，又出現 +1。
//
//   (d) 迴圈終止條件統一成 `it != end`。閉區間得寫 `it <= last`，
//       而 `<=` 需要 Random Access——對 list 根本寫不出來。
//
// 【2. end() 到底指向什麼？】
//   end() 指向一個**不存在元素的位置**。它是合法的迭代器值（可以複製、
//   可以比較、可以 --），但**不可解參考**。
//   對 vector 而言 end() 就是 data() + size() 的位置；對 std::list 而言，
//   libstdc++ 的實作是一個嵌在容器本體裡的「哨兵節點」（sentinel node），
//   環狀串接使 ++last 會回到 begin。不同容器實作方式不同，
//   但對使用者的契約完全一致：可比較、不可解參考。
//
// 【3. 為什麼 `end() - begin()` 不總是可用】
//   兩個迭代器相減需要 Random Access Iterator（vector、array、deque、
//   string、原生指標）。std::list 的迭代器只是 Bidirectional，沒有
//   operator-。通用寫法是：
//       auto n = std::distance(first, last);   // 回傳 difference_type
//   std::distance 內部用 tag dispatch：對 Random Access 直接 last - first
//   （O(1)），對其他類別退化成逐一 ++ 計數（O(N)）。
//   **同一行程式碼在 vector 上是常數時間、在 list 上是線性時間**——
//   這是 STL「複雜度可預期」原則的一個重要例外提醒。
//
// 【4. 子區間就是一對迭代器】
//   STL 沒有「子容器」的概念，只有「一對迭代器」。想對前半段排序就傳
//   [begin, begin + n/2)，想在中間找就傳 [begin+1, begin+5)。
//   演算法完全不知道也不在乎這對迭代器是不是覆蓋整個容器——
//   這就是為什麼同一個 std::find 能用在整個容器、子區間、甚至 C 陣列上。
//   （C++20 的 std::ranges 把「一對迭代器」正式打包成 range 物件，
//   讓寫法變成 std::ranges::find(vec, 40)，但底層模型完全沒變。）
//
// 【概念補充 Concept Deep Dive】
//   半開區間不是 C++ 獨有的偏好，而是一個廣泛正確的設計。
//   Dijkstra 在 EWD831 中論證過：表示整數區間時，
//   「下界含、上界不含」是唯一同時滿足下列條件的寫法——
//     * 長度直接等於 upper - lower
//     * 空區間可用 lower == upper 表示，且不需要越界的值
//     * 相鄰區間可以用同一個數字接合
//   Python 的 range(a, b)、切片 s[a:b]，Rust 的 a..b 都採同一慣例。
//   理解這一點之後，「end() 為什麼不能解參考」就不再是要背的規則，
//   而是設計的必然結果：end 從來就不是一個元素，它是一個「邊界」。
//
// 【注意事項 Pay Attention】
//   1. `*vec.end()` 是 UB，`*(vec.end() - 1)` 才是最後一個元素
//      （前提是容器非空，否則 end()-1 本身就無效）。取最後一個元素
//      建議直接用 `vec.back()`。
//   2. `vec.begin() + 4` 這種寫法只有 Random Access Iterator 支援。
//      通用寫法是 `std::next(vec.begin(), 4)`（C++11，<iterator>）。
//   3. 兩個迭代器必須來自**同一個容器**才能比較或相減；
//      拿 v1.begin() 跟 v2.end() 比較是 UB，而且通常不會有任何警告。
//   4. `end() - begin()` 的型別是 difference_type（有號的 ptrdiff_t），
//      不是 size_type。與 `vec.size()`（無號）混用比較會觸發
//      -Wsign-compare 警告。C++20 起可用 std::ssize() 取得有號長度。
//   5. 容器被修改後，先前存下的 end() 可能失效——vector push_back 之後
//      舊的 end() 幾乎必定失效。迴圈中不要在容器會變動時快取 end()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】半開區間與 end()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 選擇半開區間 [begin, end)？至少講三個理由。
//     答：(1) 空區間可用 begin == end 乾淨表示，不需要「begin 前一格」
//             這種語言層面就是 UB 的值；
//         (2) 元素個數直接是 end - begin，沒有 +1，消滅 off-by-one；
//         (3) 相鄰子區間 [a,b) 與 [b,c) 可無縫接合成 [a,c)；
//         (4) 終止條件統一為 it != end，只需要 == / !=，對只支援
//             Bidirectional 的 list 也成立（閉區間得用 <=，list 沒有）。
//     追問：那 end() 指向哪裡？可以解參考嗎？
//         → 指向「最後一個元素的下一個位置」，該處沒有物件，
//           解參考是 UB。它是合法的迭代器值，只是不可解參考。
//
// 🔥 Q2. 怎麼算一個區間有幾個元素？vector 跟 list 有差嗎？
//     答：通用寫法是 std::distance(first, last)。對 vector（Random Access）
//         它直接編成 last - first，O(1)；對 list（Bidirectional）它只能
//         逐一 ++ 計數，O(N)。std::distance 用 iterator_category 做
//         tag dispatch 在編譯期選擇實作，所以呼叫端寫法完全一樣。
//     追問：回傳型別是什麼？→ iterator_traits<It>::difference_type，
//         通常是 std::ptrdiff_t（有號）。跟 size() 的 size_type（無號）
//         比較會有 sign-compare 警告。
//
// ⚠️ 陷阱. 想走訪「除了最後一個以外」的元素，寫成
//         for (auto it = v.begin(); it != v.end() - 1; ++it)
//         哪裡有問題？
//     答：容器為空時 v.end() - 1 會產生 begin 之前的位置，那個迭代器值
//         本身就是無效的（UB），而且迴圈條件 `it != <無效值>` 幾乎必定
//         不成立，會一路走出容器。
//     為什麼會錯：腦中假設「容器至少有一個元素」。半開區間的正確用法是
//         先判空：`if (!v.empty()) for (auto it = v.begin(); it != std::prev(v.end()); ++it)`，
//         或者更安全地用索引/size 判斷。任何對 end() 做減法的程式碼，
//         都必須先確認容器非空。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <list>
#include <iterator>   // std::distance, std::next, std::size

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列原地去重，回傳新長度；前 k 個位置必須是去重後的結果。
//   為什麼用到本主題：這題的本質就是「維護一個半開區間 [begin, write)
//                     當作已完成區」。write 正是一個 past-the-end 迭代器——
//                     它指向下一個要寫入的位置，而 [begin, write) 恰好是
//                     目前的答案。回傳的長度就是 write - begin。
//                     這說明半開區間不只是遍歷慣例，也是雙指標演算法的
//                     自然表達方式。
//   複雜度：O(n) 時間、O(1) 額外空間。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    auto write = nums.begin() + 1;                 // 已完成區為 [begin, write)
    for (auto read = nums.begin() + 1; read != nums.end(); ++read) {
        if (*read != *(write - 1)) {               // 與已完成區最後一個比較
            *write = *read;
            ++write;
        }
    }
    return static_cast<int>(write - nums.begin()); // 半開區間長度，不必 +1
}

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 半開區間的好處：

    // 1. 空容器的判斷很簡單
    std::cout << "=== 1. 空容器判斷 ===" << std::endl;
    std::vector<int> empty_vec;
    if (empty_vec.begin() == empty_vec.end()) {
        std::cout << "空容器：begin() == end()" << std::endl;
    }

    // 2. 遍歷邏輯統一
    std::cout << "\n=== 2. 遍歷邏輯統一 ===" << std::endl;
    std::cout << "遍歷非空容器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 3. 計算元素個數（不必 +1）
    std::cout << "\n=== 3. 計算元素個數 ===" << std::endl;
    std::cout << "end() - begin()          = " << (vec.end() - vec.begin())
              << "（Random Access 才能相減，O(1)）" << std::endl;
    std::cout << "std::distance(b, e)      = " << std::distance(vec.begin(), vec.end())
              << "（通用寫法）" << std::endl;

    std::list<int> lst = {1, 2, 3, 4, 5, 6, 7};
    // lst.end() - lst.begin() 編譯失敗：list 的迭代器不是 Random Access
    std::cout << "list 用 std::distance    = " << std::distance(lst.begin(), lst.end())
              << "（同一行程式碼，但這裡是 O(N) 逐一計數）" << std::endl;

    // 4. 子區間表示
    std::cout << "\n=== 4. 子區間就是一對迭代器 ===" << std::endl;
    auto start = vec.begin() + 1;   // 指向 20
    auto finish = vec.begin() + 4;  // 指向 50（不包含）

    std::cout << "子區間 [1, 4): ";
    for (auto it = start; it != finish; ++it) {
        std::cout << *it << " ";
    }
    std::cout << "（共 " << (finish - start) << " 個）" << std::endl;

    // 5. 相鄰子區間可無縫接合：[0,2) + [2,5) = [0,5)
    std::cout << "\n=== 5. 相鄰區間無縫接合 ===" << std::endl;
    auto mid = vec.begin() + 2;
    std::cout << "[begin, mid) 有 " << (mid - vec.begin()) << " 個, "
              << "[mid, end) 有 " << (vec.end() - mid) << " 個, "
              << "合計 " << (vec.end() - vec.begin()) << " 個（不重不漏）" << std::endl;

    // 6. 通用寫法：std::next 取代 begin()+n
    std::cout << "\n=== 6. std::next（通用，不需 Random Access）===" << std::endl;
    std::cout << "*std::next(lst.begin(), 3) = " << *std::next(lst.begin(), 3) << std::endl;

    // 7. end() 不可解參考，取最後一個元素要退一格
    std::cout << "\n=== 7. 取最後一個元素 ===" << std::endl;
    std::cout << "*(vec.end() - 1) = " << *(vec.end() - 1)
              << "，vec.back() = " << vec.back()
              << "（*vec.end() 是 UB，不可寫）" << std::endl;

    std::cout << "\n=== LeetCode 26. Remove Duplicates from Sorted Array ===" << std::endl;
    std::vector<int> nums = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    std::cout << "原始: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << std::endl;
    int k = removeDuplicates(nums);
    std::cout << "去重後長度 k = " << k << "，前 k 個元素: ";
    for (int i = 0; i < k; ++i) std::cout << nums[i] << " ";
    std::cout << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第四課：迭代器（Iterator）的核心概念3.cpp" -o iter3

// === 預期輸出 ===
// === 1. 空容器判斷 ===
// 空容器：begin() == end()
//
// === 2. 遍歷邏輯統一 ===
// 遍歷非空容器: 10 20 30 40 50
//
// === 3. 計算元素個數 ===
// end() - begin()          = 5（Random Access 才能相減，O(1)）
// std::distance(b, e)      = 5（通用寫法）
// list 用 std::distance    = 7（同一行程式碼，但這裡是 O(N) 逐一計數）
//
// === 4. 子區間就是一對迭代器 ===
// 子區間 [1, 4): 20 30 40 （共 3 個）
//
// === 5. 相鄰區間無縫接合 ===
// [begin, mid) 有 2 個, [mid, end) 有 3 個, 合計 5 個（不重不漏）
//
// === 6. std::next（通用，不需 Random Access）===
// *std::next(lst.begin(), 3) = 4
//
// === 7. 取最後一個元素 ===
// *(vec.end() - 1) = 50，vec.back() = 50（*vec.end() 是 UB，不可寫）
//
// === LeetCode 26. Remove Duplicates from Sorted Array ===
// 原始: 0 0 1 1 1 2 2 3 3 4
// 去重後長度 k = 5，前 k 個元素: 0 1 2 3 4
