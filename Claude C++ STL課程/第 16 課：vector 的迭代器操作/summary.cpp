// =============================================================================
//  第 16 課 總複習：vector 的迭代器操作 —— STL 把「容器」與「演算法」接起來的那層
// =============================================================================
//
// 【主題資訊 Information】
//   八個取得迭代器的成員函式（四組，全部 O(1)、全部 noexcept）：
//     iterator                begin()   / end()          // 正向可讀寫
//     reverse_iterator        rbegin()  / rend()         // 反向可讀寫
//     const_iterator          cbegin()  / cend()         // 正向唯讀   （C++11）
//     const_reverse_iterator  crbegin() / crend()        // 反向唯讀   （C++11）
//   輔助工具（<iterator>）：
//     distance(a, b)  advance(it, n)  next(it, n=1)  prev(it, n=1)
//     其中 next/prev 是 C++11 加入、C++17 起為 constexpr
//   vector 的迭代器類別：random access（C++20 起更精確地說是 contiguous）
//   標頭檔：<vector>、<iterator>
//
// 【詳細解釋 Explanation】
//
// 【1. 迭代器要解決的是什麼問題】
//   如果沒有迭代器，一個排序函式必須為每種容器各寫一份：
//   sort_vector、sort_list、sort_deque……N 種容器 × M 種演算法 = N×M 份程式碼。
//   迭代器把「怎麼走訪」這件事從演算法裡抽出來，變成一組統一的介面
//   （*it、++it、it != end）。於是演算法只要寫一份，
//   任何提供了這組介面的容器都能套用——N×M 降成 N+M。
//   這是 STL 最核心的設計，也是為什麼 <algorithm> 裡的函式
//   全部接受「一對迭代器」而不是「一個容器」。
//
// 【2. 迭代器不只是指標的美化】
//   對 vector 而言，迭代器實質上就是包著 T* 的薄封裝（甚至直接是 T*）。
//   但對 list 它是指向節點的指標、對 map 是紅黑樹節點、
//   對 istream_iterator 它甚至沒有對應的記憶體——每次 ++ 就從串流讀下一筆。
//   共通點只有「行為」，不是「實作」。
//   所以任何「迭代器就是指標」的直覺，換到 list 或 map 上就會失準。
//
// 【3. 五種迭代器類別決定了你能寫什麼】
//     input          單次讀取、只能前進        （istream_iterator）
//     forward        可重複走訪                （forward_list、unordered_map）
//     bidirectional  可 --                     （list、map、set）
//     random access  可 +n、-n、[]、比大小     （vector、deque、array）
//     contiguous     額外保證記憶體連續（C++20）（vector、array、string）
//   vector 落在最強的那一級，所以 it + 3、it[2]、it1 < it2 全都能寫。
//   但這也養成了一個壞習慣：一旦把程式碼搬到 list 上就整片編譯失敗。
//   泛型程式碼要用 advance/next/prev/distance 才能跨類別運作。
//
// 【4. 半開區間 [begin, end) 是整個設計的樞紐】
//   end() 指向「最後一個元素之後」，不是最後一個元素。這帶來三件事：
//     (a) 空容器天然安全：begin() == end()，迴圈零次執行，不需特例；
//     (b) 長度就是 end() - begin()，不必 +1；
//     (c) 區間可無縫接合：[a,b) + [b,c) = [a,c)，不重不漏。
//   代價只有一個：end() 絕不可解參考。
//
// 【5. const_iterator 表達的是意圖，不只是限制】
//   在 const vector 上 begin() 本來就回傳 const_iterator，
//   所以 cbegin() 的價值在「非 const 容器上仍明確表達唯讀」：
//     for (auto it = v.cbegin(); it != v.cend(); ++it)   // 讀者一眼看出不會改
//   搭配 auto 時尤其重要——寫 auto it = v.begin() 看不出意圖，
//   寫 auto it = v.cbegin() 就把「我只讀」寫進了型別裡，讓編譯器幫你把關。
//
// 【6. 反向迭代器的 ++ 為什麼是往回走】
//   reverse_iterator 是個配接器（adapter），內部包著一個正向迭代器 current，
//   它的 operator++ 實際上執行 --current，而 operator* 回傳的是 *(current - 1)。
//   這個「差一格」的設計是為了讓 rbegin() 能由 end() 直接建構、
//   rend() 由 begin() 直接建構，維持半開區間的一致性。
//   副作用是：rit.base() 取回的正向迭代器會比 rit 指的元素「往後一格」。
//
// 【概念補充 Concept Deep Dive】
//   ▸ vector 的內部只有三根指標
//       T* begin_;  T* end_;  T* cap_end_;
//     於是 size() = end_ - begin_、capacity() = cap_end_ - begin_、
//     empty() = (begin_ == end_)，全部 O(1)。
//     本機（x86-64 / GCC 15.2 / libstdc++）實測 sizeof(std::vector<int>) = 24，
//     正好是三根 8-byte 指標——這是實作定義值，非標準保證。
//     預設建構的 vector 尚未配置記憶體，本機實測 data() 回傳 nullptr，
//     同樣屬實作細節，標準只保證 begin() == end()。
//   ▸ 範圍 for 的展開結果
//       auto&& __range = v;
//       auto __begin = __range.begin();
//       auto __end   = __range.end();
//       for (; __begin != __end; ++__begin) { T x = *__begin; ... }
//     看懂這個展開就同時明白兩件事：為何範圍 for 對空容器安全（begin==end），
//     以及為何在範圍 for 內 push_back 會炸（__end 早就被存下來且已失效）。
//   ▸ distance 的複雜度陷阱
//     distance(a, b) 對 random access 是一次減法 O(1)，
//     對 list 卻要一路 ++ 數過去 O(n)。
//     在 list 的迴圈裡呼叫 distance 是很經典的意外 O(n²)。
//
// 【注意事項 Pay Attention】
//   1. *v.end() 是 UB；空容器時 *v.begin() 同樣是 UB。
//   2. 任何可能觸發重新配置的操作（push_back/insert/resize/reserve）
//      會使「所有」迭代器、指標、引用失效；erase 則使刪除點之後的失效。
//   3. 不要在範圍 for 內修改容器大小——__end 已被快取，行為未定義。
//   4. 反向迭代器的 base() 會差一格，rit.base() - 1 才是 rit 指的元素。
//   5. it1 < it2 只有 random access 迭代器能寫；泛型程式碼一律用 !=。
//   6. auto it = v.begin() 在 const 容器上會自動得到 const_iterator，
//      不必也不能再寫 cbegin()（那是給非 const 容器表達意圖用的）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】vector 的迭代器操作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 迭代器存在的意義是什麼？直接用索引不行嗎？
//     答：索引只對「支援隨機存取的容器」有意義，list、map、set 都沒有索引。
//         迭代器把「走訪」抽象成一組統一介面（*it、++it、it != end），
//         讓 <algorithm> 的每個演算法只需寫一份，就能套用在所有容器上，
//         把 N 種容器 × M 種演算法的組合爆炸降成 N + M。
//     追問：那為什麼演算法接受的是「一對迭代器」而不是容器本身？
//         → 因為這樣才能對「子區間」操作，例如只排序前一半、
//           或對原生陣列的一段操作。傳容器就失去了這個彈性。
//
// 🔥 Q2. begin() 和 cbegin() 差在哪？在 const vector 上呼叫 begin() 會得到什麼？
//     答：cbegin() 一定回傳 const_iterator；begin() 則依容器的 const 性而定。
//         在 const vector 上，begin() 本來就回傳 const_iterator，
//         所以 cbegin() 的真正價值是在「非 const 容器」上明確表達唯讀意圖，
//         尤其搭配 auto 時能讓編譯器替你把關。
//     追問：const_iterator 和 const iterator 一樣嗎？
//         → 完全不同。const_iterator 是「指向常數的迭代器」（不能改元素，
//           但迭代器本身可以 ++）；const iterator 是「常數迭代器」
//           （不能 ++，但可以改元素）。對應到指標就是
//           const T* 與 T* const 的差別。
//
// 🔥 Q3. 反向迭代器的 ++ 為什麼會往索引變小的方向走？
//     答：reverse_iterator 是配接器，內部持有一個正向迭代器 current，
//         它的 operator++ 實際執行 --current，operator* 回傳 *(current-1)。
//         這種「刻意差一格」的設計讓 rbegin() 能直接由 end() 建構、
//         rend() 由 begin() 建構，維持半開區間的一致性。
//     追問：rit.base() 回傳什麼？
//         → 回傳內部的正向迭代器，它比 rit 所指的元素「往後一格」。
//           想取得 rit 指向的正向位置要寫 rit.base() - 1。
//           這個差一格在用 erase(rit.base()) 時最常出錯。
//
// ⚠️ 陷阱. for (auto x : v) { if (x == 3) v.push_back(99); }
//          這段程式看起來只是「邊走邊加」，為什麼是 undefined behavior？
//     答：範圍 for 在迴圈開始前就把 v.end() 存進了隱藏變數 __end。
//         push_back 一旦觸發重新配置，整塊緩衝區搬到新位址，
//         __begin 和 __end 全部變成懸空迭代器，
//         之後的 ++__begin 與 __begin != __end 都是 UB。
//     為什麼會錯：以為範圍 for 每一輪都會重新問容器「結束了沒」。
//         它不會——begin/end 只在進入迴圈時各取一次。
//         這也解釋了為什麼即使 push_back 沒觸發重新配置
//         （capacity 還夠），迴圈也不會多跑那一格：__end 是舊值。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第16課：vector 的迭代器操作】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 迭代器（iterator）的概念：行為像指標，指向容器中某個元素
 * 2. vector 提供的八個迭代器函數（四組）
 * 3. 半開區間 [begin, end) 設計原則
 * 4. 隨機存取迭代器（Random Access Iterator）的完整運算
 * 5. auto 與迭代器的搭配
 * 6. 範圍 for 迴圈是迭代器的語法糖
 * 7. 實用輔助函數：distance、advance、next、prev
 * 8. 空 vector 的迭代器行為
 * 9. 迭代器失效（Iterator Invalidation）預告
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <iterator>   // std::distance, std::advance, std::next, std::prev
#include <algorithm>  // std::find_if, std::swap

// ===== 重點一：迭代器的基本概念 =====
// 迭代器就像一個「指標」，指向容器中的某個元素。
// 透過迭代器可以：
//   *it       → 解參考，取得元素值（讀/寫）
//   ++it      → 前進一步到下一個元素
//   --it      → 後退一步到前一個元素
// 型別寫法：std::vector<T>::iterator
// 使用 auto 可以讓編譯器自動推導型別，使程式碼更簡潔。

// ===== 重點二：vector 的八個迭代器函數（四組）=====
//
// 組別 1：正向可讀寫
//   begin()  → iterator，指向第一個元素
//   end()    → iterator，指向最後元素「之後」的哨兵位置（不可解參考！）
//
// 組別 2：反向可讀寫
//   rbegin() → reverse_iterator，指向最後一個元素
//   rend()   → reverse_iterator，指向第一元素「之前」的哨兵位置（不可解參考！）
//   反向迭代器的 ++ 實際上往索引減小的方向移動。
//
// 組別 3：正向唯讀（C++11）
//   cbegin() → const_iterator，指向第一個元素，不可修改元素值
//   cend()   → const_iterator，指向最後元素之後的哨兵
//   用途：搭配 auto 時明確表達「只讀」意圖
//
// 組別 4：反向唯讀（C++11）
//   crbegin() → const_reverse_iterator，指向最後一個元素，不可修改
//   crend()   → const_reverse_iterator，指向第一元素之前的哨兵

// ===== 重點三：半開區間 [begin, end) =====
// STL 的核心設計原則：end() 指向的位置「不屬於」容器的有效範圍。
// 好處：
//   - 空容器：begin() == end()，for 迴圈一次都不執行，天然安全
//   - 長度計算：end() - begin() 就是元素個數
//   - 區間 [first, last) 的元素個數 = last - first
//
// 圖示（5個元素）：
//   元素：  [10] [20] [30] [40] [50]
//            ↑                        ↑
//          begin()                  end()  （end 指向此處，不可解參考）

// ===== 重點四：隨機存取迭代器（Random Access Iterator）=====
// vector 的迭代器是最強大的類型，支援所有指標能做的運算：
//   *it           → 解參考
//   it->member    → 成員存取（等同 (*it).member）
//   ++it / it++   → 前進一步
//   --it / it--   → 後退一步
//   it + n        → 前進 n 步，回傳新迭代器（原迭代器不變）
//   it - n        → 後退 n 步，回傳新迭代器
//   it += n       → 前進 n 步（原地修改）
//   it -= n       → 後退 n 步（原地修改）
//   it[n]         → 下標，等同 *(it + n)
//   it1 - it2     → 兩個迭代器之間的距離（回傳 ptrdiff_t）
//   it1 == it2    → 相等比較
//   it1 != it2    → 不等比較
//   it1 < it2     → 小於（以及 >、<=、>=）
//
// 對比：list 的迭代器是「雙向迭代器」，不支援 +n、-n、[]、< 等運算。

// ===== 重點五：範圍 for 迴圈的本質 =====
// for (declaration : expression) { statement; }
// 等同於（概念上）：
//   auto __begin = expression.begin();
//   auto __end   = expression.end();
//   for (; __begin != __end; ++__begin) {
//       declaration = *__begin;
//       statement;
//   }
//
// 三種接收方式：
//   for (T x : v)           → 值拷貝（修改 x 不影響 v）
//   for (T& x : v)          → 引用（修改 x 直接修改 v 中的元素）
//   for (const T& x : v)    → 常數引用（唯讀，不拷貝，效能最佳）

// ===== 重點六：實用輔助函數 =====
// std::distance(it1, it2)
//   → 計算兩個迭代器之間的距離（回傳 it2 - it1 的值）
//   → 對隨機存取迭代器：O(1)；對其他迭代器：O(n)
//
// std::advance(it, n)
//   → 原地移動 it（n 可正可負），沒有回傳值
//   → 修改的是原迭代器本身
//
// std::next(it, n = 1)  （C++11）
//   → 回傳「it 前進 n 步」的新迭代器，原迭代器不變
//
// std::prev(it, n = 1)  （C++11）
//   → 回傳「it 後退 n 步」的新迭代器，原迭代器不變
//
// advance vs next/prev 比較：
//   advance → 原地修改，void 回傳
//   next/prev → 回傳新迭代器，原迭代器不受影響

// ===== 重點七：迭代器失效（Iterator Invalidation）=====
// 任何可能導致 vector 記憶體重新配置的操作（push_back、insert、resize 等），
// 都會使「所有」現有的迭代器、指標、引用「失效」。
// 失效後繼續使用 = 未定義行為（像使用懸空指標一樣危險）。
// 安全做法：在可能觸發重新配置的操作之後，重新從 begin() 取得迭代器。
// 詳細原因見第 17 課。

void demo_basic_iterator() {
    std::cout << "\n--- 一、基本迭代器操作 ---" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 完整型別寫法（冗長）
    std::vector<int>::iterator it_full = v.begin();
    std::cout << "第一個元素（完整型別）：" << *it_full << std::endl;

    // auto 簡化（推薦）
    auto it = v.begin();
    std::cout << "解參考 *it = " << *it << std::endl;

    ++it;  // 前進一步
    std::cout << "++it 後 *it = " << *it << std::endl;  // 20

    --it;  // 後退一步
    std::cout << "--it 後 *it = " << *it << std::endl;  // 10

    it = it + 3;  // 前進 3 步（隨機跳躍，隨機存取迭代器特有）
    std::cout << "it + 3 後 *it = " << *it << std::endl;  // 40

    it = it - 2;  // 後退 2 步
    std::cout << "it - 2 後 *it = " << *it << std::endl;  // 20

    std::cout << "下標 it[2] = " << it[2] << std::endl;   // 40，等同 *(it+2)

    auto it1 = v.begin();
    auto it2 = v.end();
    std::cout << "end - begin 距離 = " << (it2 - it1) << std::endl;  // 5

    std::cout << std::boolalpha;
    std::cout << "begin() < end() = " << (v.begin() < v.end()) << std::endl;  // true
}

void demo_eight_iterators() {
    std::cout << "\n--- 二、八個迭代器函數展示 ---" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};

    // 1) 正向迭代器：begin() / end()
    std::cout << "正向遍歷（begin/end）：";
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 2) 反向迭代器：rbegin() / rend()
    // 反向迭代器的 ++ 實際上是往前（索引遞減）移動
    std::cout << "反向遍歷（rbegin/rend）：";
    for (auto rit = v.rbegin(); rit != v.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;

    // 3) 常數迭代器：cbegin() / cend()（唯讀）
    std::cout << "正向唯讀（cbegin/cend）：";
    for (auto it = v.cbegin(); it != v.cend(); ++it) {
        std::cout << *it << " ";
        // *it = 999;  // 編譯錯誤！const_iterator 不可修改元素
    }
    std::cout << std::endl;

    // 4) 常數反向迭代器：crbegin() / crend()（唯讀 + 反向）
    std::cout << "反向唯讀（crbegin/crend）：";
    for (auto rit = v.crbegin(); rit != v.crend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << std::endl;
}

void demo_range_for() {
    std::cout << "\n--- 三、範圍 for 迴圈的三種接收方式 ---" << std::endl;
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};

    // 值拷貝：修改的是副本，原容器不受影響
    std::cout << "值拷貝（修改不影響原容器）：" << std::endl;
    for (std::string name : names) {
        name += "!";  // 修改的是拷貝
        std::cout << "  " << name;
    }
    std::cout << std::endl;
    std::cout << "  原容器 names[0] = " << names[0] << "（未被修改）" << std::endl;

    // 引用：直接修改原容器中的元素
    std::cout << "引用（直接修改原容器）：" << std::endl;
    for (std::string& name : names) {
        name += "!";
    }
    std::cout << "  names[0] 現在 = " << names[0] << "（已被修改）" << std::endl;

    // const 引用：唯讀遍歷，不拷貝（效能最佳的唯讀方式）
    std::cout << "const 引用（唯讀遍歷）：";
    for (const std::string& name : names) {
        std::cout << name << " ";
        // name += "?";  // 編譯錯誤！
    }
    std::cout << std::endl;
}

void demo_utility_functions() {
    std::cout << "\n--- 四、實用輔助函數 ---" << std::endl;
    std::vector<int> v = {10, 20, 30, 40, 50};

    // std::distance：計算兩迭代器之間的距離
    auto it1 = v.begin();
    auto it2 = v.begin() + 3;
    std::cout << "distance(begin, begin+3) = "
              << std::distance(it1, it2) << std::endl;  // 3

    // std::advance：原地移動迭代器（修改 it 本身）
    auto it_adv = v.begin();
    std::advance(it_adv, 3);   // 前進 3 步，it_adv 指向第 4 個元素
    std::cout << "advance 前進 3 步後 *it = " << *it_adv << std::endl;  // 40
    std::advance(it_adv, -1);  // 後退 1 步
    std::cout << "advance 後退 1 步後 *it = " << *it_adv << std::endl;  // 30

    // std::next：回傳新迭代器，原迭代器不變
    auto it_base = v.begin();
    auto it_next = std::next(it_base, 3);
    std::cout << "next 後原迭代器 *it_base = " << *it_base << "（不變）" << std::endl;  // 10
    std::cout << "next(begin, 3) 指向 = " << *it_next << std::endl;  // 40

    // std::prev：回傳前退若干步的新迭代器
    auto it_last = std::prev(v.end());       // 指向最後一個元素
    auto it_2nd  = std::prev(v.end(), 2);    // 指向倒數第二個元素
    std::cout << "*prev(end) = " << *it_last << std::endl;  // 50
    std::cout << "*prev(end, 2) = " << *it_2nd << std::endl;  // 40
}

void demo_empty_vector_iterator() {
    std::cout << "\n--- 五、空 vector 的迭代器 ---" << std::endl;
    std::vector<int> v;  // 空 vector

    std::cout << std::boolalpha;
    std::cout << "空 vector：begin() == end() = "
              << (v.begin() == v.end()) << std::endl;  // true

    // for 迴圈條件一開始就不成立，安全地不執行
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it;  // 不會執行
    }
    std::cout << "（迴圈未執行，安全）" << std::endl;
}

void demo_iterator_invalidation() {
    std::cout << "\n--- 六、迭代器失效示範 ---" << std::endl;
    std::vector<int> v = {10, 20, 30};
    auto it = v.begin();  // 指向 10

    std::cout << "操作前 *it = " << *it << std::endl;

    // push_back 可能觸發重新配置，使 it 失效
    v.push_back(40);
    v.push_back(50);
    v.push_back(60);

    // 此時若直接用 it，可能是未定義行為！
    // std::cout << *it;  // 危險！

    // 安全做法：重新取得迭代器
    it = v.begin();
    std::cout << "重新取得迭代器後 *it = " << *it << std::endl;  // 10

    std::cout << "目前所有元素：";
    for (auto x : v) std::cout << x << " ";
    std::cout << std::endl;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 344. Reverse String
//   題目：原地反轉字元陣列，要求 O(1) 額外空間。
//   為什麼用到本主題：頭尾雙指標往中間夾，正是 random access 迭代器
//         「可比大小、可雙向移動」兩項能力的直接應用。
//         同一段邏輯若改用 list 的 bidirectional 迭代器，
//         left < right 就寫不出來（只能比 !=），這正好凸顯迭代器類別的差異。
// -----------------------------------------------------------------------------
void reverseString(std::vector<char>& s) {
    if (s.empty()) return;                    // 空容器不可 prev(end())
    auto left  = s.begin();
    auto right = std::prev(s.end());
    while (left < right) {                    // < 只有 random access 能寫
        std::swap(*left, *right);
        ++left;
        --right;
    }
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：已排序陣列原地去重，回傳去重後的長度。
//   為什麼用到本主題：這題的本質是「一個讀迭代器、一個寫迭代器同時前進」，
//         也就是 std::unique 的內部做法。示範兩個迭代器在同一個容器上
//         以不同速度移動——這是迭代器抽象最實用的樣式之一。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    if (nums.empty()) return 0;
    auto write = nums.begin();                       // 慢指標：下一個要寫的位置
    for (auto read = std::next(nums.begin()); read != nums.end(); ++read) {
        if (*read != *write) {                       // 遇到新值才寫入
            ++write;
            *write = *read;
        }
    }
    nums.erase(std::next(write), nums.end());        // 砍掉尾巴
    return static_cast<int>(nums.size());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】存取日誌分析：找出最近一次錯誤，並回推它前面的前後文
//   情境：伺服器 log 依時間順序存在 vector 裡，出事時要找到
//         「最後一筆 ERROR」，並把它前面幾行一起印出來供除錯。
//   為什麼用本主題：「從後往前找」用 reverse_iterator 最自然，
//         而「找到之後回推 N 行」則需要把反向迭代器轉回正向位置——
//         這正是 rit.base() 差一格的實戰演練，也是最容易寫錯的地方。
// -----------------------------------------------------------------------------
void printErrorContext(const std::vector<std::string>& logs, std::size_t contextLines) {
    // 從尾端往前找第一筆含 "ERROR" 的紀錄
    auto rit = std::find_if(logs.crbegin(), logs.crend(),
                            [](const std::string& line) {
                                return line.find("ERROR") != std::string::npos;
                            });

    if (rit == logs.crend()) {
        std::cout << "  （日誌中沒有 ERROR）" << std::endl;
        return;
    }

    // 關鍵：rit.base() 指向「錯誤那行的下一行」，所以要 -1 才是錯誤行本身
    auto errIt = std::prev(rit.base());
    std::size_t errIndex = static_cast<std::size_t>(std::distance(logs.cbegin(), errIt));

    // 往前回推 contextLines 行，並小心不要跨過 begin()
    std::size_t start = (errIndex >= contextLines) ? errIndex - contextLines : 0;
    auto from = std::next(logs.cbegin(), static_cast<std::ptrdiff_t>(start));

    std::cout << "  最後一筆 ERROR 位於第 " << errIndex << " 行，前後文：" << std::endl;
    for (auto it = from; it != std::next(errIt); ++it) {
        std::cout << (it == errIt ? "  >> " : "     ") << *it << std::endl;
    }
}

void demo_leetcode_and_practice() {
    std::cout << "\n--- 七、LeetCode 344. Reverse String ---" << std::endl;
    std::vector<char> s = {'h', 'e', 'l', 'l', 'o'};
    reverseString(s);
    std::cout << "反轉 {'h','e','l','l','o'} → ";
    for (char c : s) std::cout << c;
    std::cout << std::endl;

    std::cout << "\n--- 八、LeetCode 26. Remove Duplicates from Sorted Array ---" << std::endl;
    std::vector<int> nums = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int len = removeDuplicates(nums);
    std::cout << "去重後長度 " << len << "，內容: ";
    for (int x : nums) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "\n--- 九、日常實務：用反向迭代器定位最後一筆 ERROR ---" << std::endl;
    std::vector<std::string> logs = {
        "10:00:01 INFO  service started",
        "10:00:05 INFO  connected to db",
        "10:00:09 WARN  slow query 1200ms",
        "10:00:11 ERROR failed to write cache",
        "10:00:12 INFO  retrying",
        "10:00:15 INFO  recovered"
    };
    printErrorContext(logs, 2);
    printErrorContext({"10:00:01 INFO ok", "10:00:02 INFO ok"}, 2);
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "       第 16 課：vector 的迭代器操作 總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_basic_iterator();
    demo_eight_iterators();
    demo_range_for();
    demo_utility_functions();
    demo_empty_vector_iterator();
    demo_iterator_invalidation();
    demo_leetcode_and_practice();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "課程重點回顧：" << std::endl;
    std::cout << "  1. 迭代器 = 指標概念的抽象，透過 begin()/end() 取得" << std::endl;
    std::cout << "  2. 八個迭代器函數：begin/end、rbegin/rend、cbegin/cend、crbegin/crend" << std::endl;
    std::cout << "  3. 半開區間 [begin, end)：end() 不可解參考，是哨兵" << std::endl;
    std::cout << "  4. vector 的迭代器是隨機存取迭代器：支援 +n、-n、[]、< 等" << std::endl;
    std::cout << "  5. auto 搭配 cbegin() 明確表達唯讀意圖" << std::endl;
    std::cout << "  6. 範圍 for 是 begin()/end() 的語法糖" << std::endl;
    std::cout << "  7. distance()、advance()、next()、prev() 為實用工具" << std::endl;
    std::cout << "  8. 可能觸發重新配置的操作後，迭代器全部失效！" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary16
//
// 【關於下方預期輸出的但書】
//   本檔輸出完全是確定性的，每次執行都應完全相同。
//   檔頭【概念補充】中提到的 sizeof(std::vector<int>) = 24
//   是本機（x86-64 / GCC 15.2 / libstdc++）的實測值，屬實作定義，
//   該數值並未出現在下方輸出中。

// === 預期輸出 ===
// ================================================================
//        第 16 課：vector 的迭代器操作 總複習
// ================================================================
//
// --- 一、基本迭代器操作 ---
// 第一個元素（完整型別）：10
// 解參考 *it = 10
// ++it 後 *it = 20
// --it 後 *it = 10
// it + 3 後 *it = 40
// it - 2 後 *it = 20
// 下標 it[2] = 40
// end - begin 距離 = 5
// begin() < end() = true
//
// --- 二、八個迭代器函數展示 ---
// 正向遍歷（begin/end）：10 20 30 40 50
// 反向遍歷（rbegin/rend）：50 40 30 20 10
// 正向唯讀（cbegin/cend）：10 20 30 40 50
// 反向唯讀（crbegin/crend）：50 40 30 20 10
//
// --- 三、範圍 for 迴圈的三種接收方式 ---
// 值拷貝（修改不影響原容器）：
//   Alice!  Bob!  Charlie!
//   原容器 names[0] = Alice（未被修改）
// 引用（直接修改原容器）：
//   names[0] 現在 = Alice!（已被修改）
// const 引用（唯讀遍歷）：Alice! Bob! Charlie!
//
// --- 四、實用輔助函數 ---
// distance(begin, begin+3) = 3
// advance 前進 3 步後 *it = 40
// advance 後退 1 步後 *it = 30
// next 後原迭代器 *it_base = 10（不變）
// next(begin, 3) 指向 = 40
// *prev(end) = 50
// *prev(end, 2) = 40
//
// --- 五、空 vector 的迭代器 ---
// 空 vector：begin() == end() = true
// （迴圈未執行，安全）
//
// --- 六、迭代器失效示範 ---
// 操作前 *it = 10
// 重新取得迭代器後 *it = 10
// 目前所有元素：10 20 30 40 50 60
//
// --- 七、LeetCode 344. Reverse String ---
// 反轉 {'h','e','l','l','o'} → olleh
//
// --- 八、LeetCode 26. Remove Duplicates from Sorted Array ---
// 去重後長度 5，內容: 0 1 2 3 4
//
// --- 九、日常實務：用反向迭代器定位最後一筆 ERROR ---
//   最後一筆 ERROR 位於第 3 行，前後文：
//      10:00:05 INFO  connected to db
//      10:00:09 WARN  slow query 1200ms
//   >> 10:00:11 ERROR failed to write cache
//   （日誌中沒有 ERROR）
//
// ================================================================
// 課程重點回顧：
//   1. 迭代器 = 指標概念的抽象，透過 begin()/end() 取得
//   2. 八個迭代器函數：begin/end、rbegin/rend、cbegin/cend、crbegin/crend
//   3. 半開區間 [begin, end)：end() 不可解參考，是哨兵
//   4. vector 的迭代器是隨機存取迭代器：支援 +n、-n、[]、< 等
//   5. auto 搭配 cbegin() 明確表達唯讀意圖
//   6. 範圍 for 是 begin()/end() 的語法糖
//   7. distance()、advance()、next()、prev() 為實用工具
//   8. 可能觸發重新配置的操作後，迭代器全部失效！
// ================================================================
