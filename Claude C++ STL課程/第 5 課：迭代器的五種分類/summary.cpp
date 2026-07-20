// =============================================================================
//  第 5 課 總複習  —  迭代器的五種分類：能力的偏序與演算法的最低要求
// =============================================================================
//
// 【主題資訊 Information】
//   五種分類與各自的能力（上層包含下層）：
//     Input          *it（讀）、++it、==/!=                       單次走訪
//     Output         *it = v（寫）、++it                          單次走訪
//     Forward        Input + 可寫 + **多次走訪保證**
//     Bidirectional  Forward + --it
//     Random Access  Bidirectional + it±n、it1-it2、it[n]、it1<it2
//   階層圖（開頭是**分叉**，不是一條直線）：
//     Input ──┐
//             ├─→ Forward ─→ Bidirectional ─→ Random Access ─→ (C++20) Contiguous
//     Output ─┘
//   各容器對應：
//     Random Access  vector / deque / array / string / 原生指標
//     Bidirectional  list / set / multiset / map / multimap
//     Forward        forward_list / unordered_set / unordered_map
//     Input          istream_iterator
//     Output         ostream_iterator / back_inserter / front_inserter / inserter
//   標準版本：分類自 C++98；contiguous_iterator_tag 是 C++20；
//             next/prev 是 C++11；if constexpr 是 C++17。
//   標頭檔：<iterator>（iterator_traits、tag、advance/distance/next/prev）
//
// 【詳細解釋 Explanation】
//
// 【1. 分類的意義：把「文件約定」變成「編譯期保證」】
//   每個演算法都有它真正需要的最低能力：
//       find / count / accumulate          → Input
//       copy / transform 的**目的地**      → Output
//       replace / remove / unique / rotate → Forward
//       reverse / copy_backward            → Bidirectional
//       sort / nth_element / binary_search → Random Access
//   把這件事寫進型別系統之後，用能力不足的迭代器就是**編譯錯誤**，
//   而不是執行期出錯、或效能悄悄從 O(1) 退化成 O(N)。
//   這也是為什麼 cppreference 上的樣板參數名稱（InputIt / ForwardIt / RandomIt）
//   不是隨便取的 —— 那是規格的一部分。
//
// 【2. Forward 的分水嶺：multi-pass 保證】
//   Input 與 Forward 最關鍵的差別不是「能不能寫」，而是**多次走訪保證**：
//       Forward：複製一份迭代器、各自前進，兩者都仍有效且產生相同序列
//       Input  ：++it 之後，先前的副本即失效（single-pass）
//   為什麼這件事這麼重要？因為 std::unique / std::remove 這類演算法
//   需要「同時持有兩個位置」（一個讀、一個寫），
//   而那要求兩個迭代器能獨立存在 —— 正是 multi-pass 的定義。
//   istream_iterator 做不到，因為串流讀過就沒了。
//
// 【3. 泛型程式碼該要求「最低」而非「最高」】
//   寫泛型函式時的實務準則是：**只要求你真正用到的最低等級**。
//   如果你的函式只是走訪一遍，就寫成接受 Input Iterator ——
//   這樣它同時能服務 vector、list、forward_list 甚至 std::cin。
//   隨手要求 Random Access 會把大半容器排除在外，而那個要求根本用不到。
//   std::find 就是最好的示範：它只要求 Input，所以無所不在。
//
// 【4. advance / distance / next / prev：讓程式碼對分類保持中立】
//   直接寫 it + n 會把程式碼綁死在 Random Access 上。改用：
//       std::advance(it, n)      就地移動，回傳 void
//       std::next(it, n)         回傳新迭代器，原 it 不動（C++11）
//       std::prev(it, n)         往回 n 步（需 Bidirectional 以上）
//       std::distance(a, b)      計算距離
//   它們用 iterator_traits 在**編譯期**分派到最佳實作：
//   Random Access 走 `+= n`（O(1)），其餘走迴圈（O(N)）。
//   注意：這只讓介面統一，**複雜度不會變** —— advance 對 list 仍是 O(N)。
//
// 【概念補充 Concept Deep Dive】
//   五個 tag 之間的**繼承關係**才是整個機制的核心，而不是那五個空類別本身：
//       forward_iterator_tag       : input_iterator_tag
//       bidirectional_iterator_tag : forward_iterator_tag
//       random_access_iterator_tag : bidirectional_iterator_tag
//   有了繼承，「標籤分派」就能自動實現「能力足夠即可」的語意：
//   傳一個 forward_iterator_tag 給只有 input 版與 random_access 版的多載組，
//   會因為 forward 繼承自 input 而正確選中 input 版（保守但正確）。
//   若五個 tag 彼此無關，每個演算法就得為五種分類各寫一份實作。
//   實務推論：要檢查「至少是某等級」請用 std::is_base_of_v 而非 is_same_v ——
//   後者會拒絕所有「能力更強」的分類（例如 C++20 的 contiguous）。
//   注意 output_iterator_tag **不在**這條繼承鏈上，它與 input 並列。
//
// 【注意事項 Pay Attention】
//   1. 五種分類的階層開頭是**分叉**：Input 與 Output 並列，能力互不包含。
//   2. Input Iterator 只保證單次走訪，++ 之後副本即失效。
//   3. std::distance 對 Input Iterator 會**消耗掉串流**；對 list 是 O(N)。
//   4. advance / next 只統一介面，不改變複雜度。
//   5. 檢查「至少是某等級」用 is_base_of_v，不要用 is_same_v。
//   6. C++20 的 contiguous_iterator_tag 是新增的最強分類；
//      但 libstdc++ 為 ABI 相容，vector 迭代器的 category 仍回報 random_access。
//   7. 自訂迭代器要能用於 STL 演算法，必須提供 iterator_traits 需要的五個 typedef；
//      舊式的「繼承 std::iterator<>」在 C++17 已 deprecated。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器五種分類總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 五種迭代器分類是什麼？它們的階層關係是一條直線嗎？
//     答：Input、Output、Forward、Bidirectional、Random Access。
//         **不是**一條直線 —— 開頭是分叉的：Input（只讀）與 Output（只寫）
//         能力互不包含，兩者之上才匯流成 Forward（能讀能寫且保證多次走訪），
//         再往上是 Bidirectional（可 --）、Random Access（可 ±n、[]、比較大小）。
//         C++20 另在最上層加了 Contiguous（保證記憶體連續）。
//     追問：Input 與 Forward 的關鍵差別是什麼？
//           → 多次走訪（multi-pass）保證。Forward 允許你複製迭代器、
//             各自前進而兩者都仍有效；Input 在 ++ 之後副本即失效。
//             std::unique / std::remove 需要同時持有讀寫兩個位置，
//             所以最低要求是 Forward。
//
// 🔥 Q2. std::advance(it, n) 對 vector 是 O(1)、對 list 是 O(N)，它怎麼辦到的？
//     答：標籤分派。advance 內部把
//         `typename iterator_traits<It>::iterator_category()` 這個空 tag 物件
//         當額外參數傳給內部多載，編譯器在**編譯期**選中對應實作：
//         random_access 版用 `it += n`，bidirectional/input 版用迴圈。
//         tag 是空類別、決策全在編譯期，最佳化後執行期零額外成本。
//     追問：那用 advance 是不是就讓 list 也變成 O(1) 了？
//           → 不是。advance 只統一了**介面**，讓同一份泛型程式碼能編過所有容器；
//             複雜度誠實反映底層 —— 對 list 仍然是 O(N) 的逐步走訪。
//
// ⚠️ 陷阱. 「std::sort 對 list 只是比較慢，不是不能用」——為什麼這是錯的？
//     答：是**編譯錯誤**，不是慢。std::sort 的實作裡有 it + n、it1 - it2 這類運算，
//         而 list::iterator（Bidirectional）根本沒有這些運算子，
//         樣板實例化時就失敗了。錯誤訊息通常很長（指向 <bits/stl_algo.h> 內部），
//         但根因就是這個。list 要排序請用成員函式 lst.sort()。
//     為什麼會錯：把 C++ 樣板當成 Python 的 duck typing —— 以為「跑起來再說」。
//         樣板是編譯期展開的，缺少任何一個用到的運算子都是編譯錯誤。
//         而這正是分類系統的價值：把「這個組合不合法」這件事
//         從執行期（或根本不會被發現）提前到編譯期。
//         C++20 的 concepts 進一步把錯誤訊息變回人話。
// ═══════════════════════════════════════════════════════════════════════════

/*
 * ================================================================
 * 【第5課：迭代器的五種分類】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 為何要分類：容器結構決定迭代器能力
 * 2. Input Iterator：只讀、只能前進、單次遍歷（istream_iterator）
 * 3. Output Iterator：只寫、只能前進、單次遍歷（ostream_iterator, back_inserter）
 * 4. Forward Iterator：可多次遍歷、只能前進（forward_list, unordered_set）
 * 5. Bidirectional Iterator：可雙向移動（list, set, map）
 * 6. Random Access Iterator：完整功能，支援跳躍和比較（vector, deque, 指標）
 * 7. 演算法對迭代器的要求：find >= Input, reverse >= Bidirectional, sort >= Random Access
 * 8. 通用工具：std::advance, std::distance, std::next, std::prev
 * 9. 迭代器標籤（iterator_traits）：編譯期類別識別
 * ================================================================
 * 目錄：
 *   重點一：五種迭代器的階層與能力比較
 *   重點二：Input Iterator（istream_iterator）
 *   重點三：Output Iterator（ostream_iterator, back_inserter）
 *   重點四：Forward Iterator（forward_list）
 *   重點五：Bidirectional Iterator（list, set）
 *   重點六：Random Access Iterator（vector）完整功能
 *   重點七：演算法與迭代器要求（find/reverse/sort）
 *   重點八：advance/distance/next/prev 工具函數
 *   重點九：迭代器標籤與 iterator_traits
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <list>
#include <forward_list>
#include <deque>
#include <set>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <string>
#include <numeric>
#include <type_traits>

// ===== 重點一：五種迭代器的階層 =====
// 五種迭代器由弱到強形成階層：
//
//   Random Access Iterator    ← 最強，支援所有操作
//        ↑ 繼承
//   Bidirectional Iterator    ← 可雙向移動（++ 和 --）
//        ↑ 繼承
//   Forward Iterator          ← 只能前進，但可多次遍歷
//       ↗↖ 繼承（分叉）
//   Input         Output      ← 最基本，單次遍歷
//
// 上層包含下層所有能力：能用 Forward 的地方也能用 Random Access。
//
// 容器的迭代器類別：
//   Random Access：vector, deque, array, string, 原生指標
//   Bidirectional：list, set, multiset, map, multimap
//   Forward：forward_list, unordered_set/map
//   Input：istream_iterator
//   Output：ostream_iterator, back_inserter

void demo_iterator_hierarchy() {
    std::cout << "===== 重點一：迭代器階層 =====" << std::endl;
    std::cout << "五種迭代器（由弱到強）：" << std::endl;
    std::cout << "  1. Input Iterator   - 只讀、只能前進、單次遍歷" << std::endl;
    std::cout << "  2. Output Iterator  - 只寫、只能前進、單次遍歷" << std::endl;
    std::cout << "  3. Forward Iterator - 可讀寫、只能前進、可多次遍歷" << std::endl;
    std::cout << "  4. Bidirectional    - 可讀寫、可雙向、可多次遍歷" << std::endl;
    std::cout << "  5. Random Access    - 完整功能：+n, -n, [], <, >, 距離計算" << std::endl;
    std::cout << std::endl;

    // 展示各容器使用不同迭代器類型
    std::vector<int>       vec  = {3, 1, 4};
    std::list<int>         lst  = {3, 1, 4};
    std::forward_list<int> flst = {3, 1, 4};

    // vector：Random Access，可以 +n
    auto v_it = vec.begin();
    std::cout << "vector[+2] = " << *(v_it + 2) << "  (Random Access)" << std::endl;

    // list：Bidirectional，可以 -- 但不能 +n
    auto l_it = lst.begin();
    ++l_it; ++l_it;
    --l_it;  // 可以後退
    std::cout << "list --后: " << *l_it << "  (Bidirectional)" << std::endl;

    // forward_list：Forward，不能 -- 也不能 +n
    auto f_it = flst.begin();
    ++f_it;  // 只能前進
    std::cout << "forward_list ++: " << *f_it << "  (Forward)" << std::endl;
}

// ===== 重點二：Input Iterator =====
// 特性：只能讀取（不能寫入）、只能往前（不能後退或跳躍）、單次遍歷。
// 代表：istream_iterator（從串流讀取資料）
// 限制：一旦讀過，無法重新遍歷同一串流。

void demo_input_iterator() {
    std::cout << "\n===== 重點二：Input Iterator =====" << std::endl;

    // 使用 istringstream 模擬輸入串流（避免真實 stdin 互動）
    std::istringstream iss("10 20 30 40 50");

    // istream_iterator<int> 是 Input Iterator
    std::istream_iterator<int> it(iss);    // 指向第一個元素
    std::istream_iterator<int> end_it;    // 預設建構 = 結束標記

    std::cout << "讀取: " << *it << std::endl;  // 讀取
    ++it;
    std::cout << "前進後讀取: " << *it << std::endl;

    // 支援的操作：*it（讀）、++it（前進）、it == end（比較）
    // 不支援：--it（後退）、it + n（跳躍）、多次遍歷

    // 使用迭代器範圍建構 vector（istream_iterator 很常這樣用）
    std::istringstream iss2("1 2 3 4 5");
    std::istream_iterator<int> begin2(iss2);
    std::istream_iterator<int> end2;
    std::vector<int> nums(begin2, end2);

    std::cout << "從串流讀入 vector: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "（Input Iterator：只讀、只前進、單次遍歷）" << std::endl;
}

// ===== 重點三：Output Iterator =====
// 特性：只能寫入（讀取無意義）、只能往前、單次遍歷。
// 代表：ostream_iterator、back_inserter、front_inserter、inserter

void demo_output_iterator() {
    std::cout << "\n===== 重點三：Output Iterator =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // ostream_iterator：將元素「寫入」到輸出串流
    std::cout << "ostream_iterator 輸出: ";
    std::copy(vec.begin(), vec.end(),
              std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    // 手動操作 ostream_iterator（展示其介面）
    std::ostream_iterator<int> out_it(std::cout, " ");
    std::cout << "手動寫入: ";
    *out_it = 100; ++out_it;  // 寫入後前進
    *out_it = 200; ++out_it;
    *out_it = 300;
    std::cout << std::endl;

    // back_inserter：寫入到容器尾端（自動 push_back）
    std::vector<int> src = {1, 2, 3};
    std::vector<int> dest;
    std::copy(src.begin(), src.end(), std::back_inserter(dest));
    std::cout << "back_inserter 結果: ";
    for (int n : dest) std::cout << n << " ";
    std::cout << std::endl;

    std::cout << "（Output Iterator：只寫、只前進、單次遍歷）" << std::endl;
}

// ===== 重點四：Forward Iterator =====
// 特性：可讀可寫、只能往前、可多次遍歷（可保存迭代器）。
// 代表：forward_list、unordered_set、unordered_map
// 相比 Input Iterator 的進步：保存的迭代器還有效、可以重複遍歷

void demo_forward_iterator() {
    std::cout << "\n===== 重點四：Forward Iterator =====" << std::endl;

    std::forward_list<int> flist = {10, 20, 30, 40, 50};

    // 保存迭代器（Input Iterator 不保證這是安全的）
    auto saved = flist.begin();
    ++saved;  // 指向 20

    // 可以多次遍歷同一個容器
    std::cout << "第一次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    std::cout << "第二次遍歷: ";
    for (auto it = flist.begin(); it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 之前保存的迭代器依然有效
    std::cout << "保存的迭代器指向: " << *saved << std::endl;

    // 從保存的位置繼續遍歷
    std::cout << "從保存位置繼續: ";
    for (auto it = saved; it != flist.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 不支援：-- 後退（只能前進）
    // --saved;  // 編譯錯誤！
    std::cout << "（Forward Iterator：可讀寫、只前進、可多次遍歷）" << std::endl;
}

// ===== 重點五：Bidirectional Iterator =====
// 特性：可讀可寫、可雙向移動（++ 和 --）、可多次遍歷。
// 代表：list、set、multiset、map、multimap
// 不支援：跳躍（it + n）和大小比較（it1 < it2）

void demo_bidirectional_iterator() {
    std::cout << "\n===== 重點五：Bidirectional Iterator =====" << std::endl;

    // list：雙向鏈結串列，所以可以往前往後
    std::list<int> lst = {10, 20, 30, 40, 50};

    auto it = lst.begin();
    std::cout << "起始: " << *it << std::endl;
    ++it;
    std::cout << "++it: " << *it << std::endl;
    ++it;
    std::cout << "++it: " << *it << std::endl;
    --it;  // 可以後退！Bidirectional 的關鍵特性
    std::cout << "--it（後退）: " << *it << std::endl;

    // set 也是 Bidirectional Iterator（有序，樹狀結構）
    std::set<int> s = {50, 10, 30, 20, 40};  // 自動排序
    std::cout << "set 正向: ";
    for (auto sit = s.begin(); sit != s.end(); ++sit) {
        std::cout << *sit << " ";
    }
    std::cout << std::endl;

    std::cout << "set 反向: ";
    for (auto sit = s.rbegin(); sit != s.rend(); ++sit) {
        std::cout << *sit << " ";
    }
    std::cout << std::endl;

    // 不支援：跳躍（it + 2）
    // it + 2;  // 編譯錯誤！
    std::cout << "（Bidirectional Iterator：可讀寫、可雙向、不能跳躍）" << std::endl;
}

// ===== 重點六：Random Access Iterator =====
// 特性：最強大，支援所有迭代器操作：
//   ++ / -- / it+n / it-n / it1-it2（距離）/ it[n] / it1<it2（比較）
// 代表：vector、deque、array、string、原生指標

void demo_random_access_iterator() {
    std::cout << "\n===== 重點六：Random Access Iterator =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto it = vec.begin();

    // 解參考
    std::cout << "*it = " << *it << std::endl;

    // 前進/後退（同 Bidirectional）
    ++it;
    std::cout << "++it: " << *it << std::endl;
    --it;
    std::cout << "--it: " << *it << std::endl;

    // 跳躍！（Random Access 獨有）
    it = it + 3;
    std::cout << "it + 3: " << *it << std::endl;
    it = it - 2;
    std::cout << "it - 2: " << *it << std::endl;

    // 下標存取（Random Access 獨有）
    it = vec.begin();
    std::cout << "it[2] = " << it[2] << std::endl;
    std::cout << "it[4] = " << it[4] << std::endl;

    // 計算距離（Random Access 獨有，O(1)）
    auto beg = vec.begin();
    auto end = vec.end();
    std::cout << "end - begin = " << (end - beg) << std::endl;

    // 比較大小（Random Access 獨有）
    auto mid = vec.begin() + 2;
    std::cout << "begin < mid? " << (beg < mid ? "是" : "否") << std::endl;
    std::cout << "end > mid?   " << (end > mid ? "是" : "否") << std::endl;

    // 原生指標也是 Random Access Iterator！
    int arr[] = {100, 200, 300, 400, 500};
    std::sort(arr, arr + 5);  // 直接用指標作為迭代器
    auto found = std::find(arr, arr + 5, 300);
    std::cout << "陣列 find(300) 索引 = " << (found - arr) << std::endl;

    std::cout << "（Random Access：完整功能，+n, -n, [], 距離, 比較）" << std::endl;
}

// ===== 重點七：演算法對迭代器類別的要求 =====
// 演算法文件會說明需要哪種迭代器。使用能力不足的迭代器 = 編譯錯誤。
//
//   find/count/accumulate      → Input Iterator（最寬鬆）
//   copy/transform（目的地）   → Output Iterator
//   replace/remove/unique      → Forward Iterator
//   reverse/copy_backward      → Bidirectional Iterator
//   sort/binary_search/nth_element → Random Access Iterator（最嚴格）

void demo_algorithm_iterator_requirements() {
    std::cout << "\n===== 重點七：演算法對迭代器的要求 =====" << std::endl;

    std::vector<int>       vec  = {5, 2, 8, 1, 9};
    std::list<int>         lst  = {5, 2, 8, 1, 9};
    std::forward_list<int> flst = {5, 2, 8, 1, 9};

    // find：只需要 Input Iterator → 三種容器都能用
    std::cout << "[find 需要 Input Iterator]" << std::endl;
    auto vi = std::find(vec.begin(),  vec.end(),  8);
    auto li = std::find(lst.begin(),  lst.end(),  8);
    auto fi = std::find(flst.begin(), flst.end(), 8);
    std::cout << "vector/list/forward_list find(8): "
              << (vi != vec.end()  ? "找到" : "沒找到") << " / "
              << (li != lst.end()  ? "找到" : "沒找到") << " / "
              << (fi != flst.end() ? "找到" : "沒找到") << std::endl;

    // reverse：需要 Bidirectional → forward_list 不能用
    std::cout << "[reverse 需要 Bidirectional Iterator]" << std::endl;
    std::reverse(vec.begin(), vec.end());
    std::reverse(lst.begin(), lst.end());
    // std::reverse(flst.begin(), flst.end());  // 編譯錯誤！
    std::cout << "vector 反轉: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "list 反轉: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;

    // sort：需要 Random Access → 只有 vector 能用
    std::cout << "[sort 需要 Random Access Iterator]" << std::endl;
    std::sort(vec.begin(), vec.end());
    // std::sort(lst.begin(), lst.end());   // 編譯錯誤！
    // std::sort(flst.begin(), flst.end()); // 編譯錯誤！
    lst.sort();    // list 有自己的 sort 成員函數
    flst.sort();   // forward_list 也有
    std::cout << "vector 排序: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "list 排序: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "forward_list 排序: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
}

// ===== 重點八：advance / distance / next / prev =====
// 這些工具函數讓程式碼對迭代器類別保持通用：
//   std::advance(it, n)     - 前進 n 步（Random Access: O(1)，其他: O(n)）
//   std::distance(it1, it2) - 計算距離（Random Access: O(1)，其他: O(n)）
//   std::next(it, n)        - 不修改 it，回傳前進 n 步後的迭代器（C++11）
//   std::prev(it, n)        - 不修改 it，回傳後退 n 步後的迭代器（C++11）

void demo_advance_distance() {
    std::cout << "\n===== 重點八：advance/distance/next/prev =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int>   lst = {10, 20, 30, 40, 50};

    // advance：移動迭代器
    auto vit = vec.begin();
    auto lit = lst.begin();
    std::advance(vit, 3);  // vector: 內部用 it + 3，O(1)
    std::advance(lit, 3);  // list: 內部用 ++it 三次，O(n)
    std::cout << "advance(it, 3) in vector: " << *vit << std::endl;
    std::cout << "advance(it, 3) in list:   " << *lit << std::endl;

    // distance：計算距離
    std::cout << "distance(begin, end) in vector: "
              << std::distance(vec.begin(), vec.end()) << std::endl;
    std::cout << "distance(begin, end) in list:   "
              << std::distance(lst.begin(), lst.end()) << std::endl;

    // next / prev（C++11）：不修改原迭代器，回傳新迭代器
    auto it = vec.begin();
    auto next2  = std::next(it, 2);      // 前進 2 步，it 不變
    auto prev1  = std::prev(vec.end(), 1); // 從 end 後退 1 步

    std::cout << "*it（原不變）= " << *it << std::endl;
    std::cout << "*next(it, 2) = " << *next2 << std::endl;
    std::cout << "*prev(end, 1) = " << *prev1 << std::endl;
}

// ===== 重點九：迭代器標籤（Iterator Tags） =====
// STL 用 iterator_category 標籤在編譯期識別迭代器類別。
// 演算法可以根據標籤選擇最佳實作（template 特化）。
// 標籤類型（繼承層次）：
//   std::input_iterator_tag
//   std::output_iterator_tag
//   std::forward_iterator_tag         繼承自 input
//   std::bidirectional_iterator_tag   繼承自 forward
//   std::random_access_iterator_tag   繼承自 bidirectional

template <typename Iterator>
std::string get_iterator_category_name() {
    using cat = typename std::iterator_traits<Iterator>::iterator_category;

    if constexpr (std::is_same_v<cat, std::input_iterator_tag>)
        return "Input Iterator";
    else if constexpr (std::is_same_v<cat, std::output_iterator_tag>)
        return "Output Iterator";
    else if constexpr (std::is_same_v<cat, std::forward_iterator_tag>)
        return "Forward Iterator";
    else if constexpr (std::is_same_v<cat, std::bidirectional_iterator_tag>)
        return "Bidirectional Iterator";
    else if constexpr (std::is_same_v<cat, std::random_access_iterator_tag>)
        return "Random Access Iterator";
    else
        return "Unknown";
}

// 利用標籤實作通用 advance（簡化版展示原理）
template <typename It>
void my_advance(It& it, int n, std::random_access_iterator_tag) {
    it += n;  // O(1)
    std::cout << "  （使用 += 跳躍，O(1)）" << std::endl;
}

template <typename It>
void my_advance(It& it, int n, std::bidirectional_iterator_tag) {
    if (n > 0) while (n--) ++it;
    else       while (n++) --it;
    std::cout << "  （使用 ++/-- 逐步移動，O(n)）" << std::endl;
}

template <typename It>
void my_advance(It& it, int n, std::forward_iterator_tag) {
    while (n-- > 0) ++it;
    std::cout << "  （使用 ++ 逐步前進，O(n)）" << std::endl;
}

template <typename It>
void my_advance(It& it, int n) {
    my_advance(it, n, typename std::iterator_traits<It>::iterator_category{});
}

void demo_iterator_tags() {
    std::cout << "\n===== 重點九：迭代器標籤 =====" << std::endl;

    // 查詢各容器的迭代器類別
    std::cout << "各容器迭代器類別：" << std::endl;
    std::cout << "  vector:       "
              << get_iterator_category_name<std::vector<int>::iterator>() << std::endl;
    std::cout << "  list:         "
              << get_iterator_category_name<std::list<int>::iterator>() << std::endl;
    std::cout << "  forward_list: "
              << get_iterator_category_name<std::forward_list<int>::iterator>() << std::endl;
    std::cout << "  原生指標 int*: "
              << get_iterator_category_name<int*>() << std::endl;
    std::cout << "  istream_iterator: "
              << get_iterator_category_name<std::istream_iterator<int>>() << std::endl;
    std::cout << "  ostream_iterator: "
              << get_iterator_category_name<std::ostream_iterator<int>>() << std::endl;

    // 展示 my_advance 根據標籤選擇不同實作
    std::cout << "\nmy_advance 根據標籤選擇實作：" << std::endl;
    std::vector<int> vec = {10, 20, 30, 40, 50};
    std::list<int>   lst = {10, 20, 30, 40, 50};
    std::forward_list<int> flst = {10, 20, 30, 40, 50};

    auto vit = vec.begin();
    std::cout << "vector my_advance(it, 3):";
    my_advance(vit, 3);
    std::cout << "  *it = " << *vit << std::endl;

    auto lit = lst.begin();
    std::cout << "list my_advance(it, 3):";
    my_advance(lit, 3);
    std::cout << "  *it = " << *lit << std::endl;

    auto fit = flst.begin();
    std::cout << "forward_list my_advance(it, 3):";
    my_advance(fit, 3);
    std::cout << "  *it = " << *fit << std::endl;
}

// ===== LeetCode 實戰 =====
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 26. Remove Duplicates from Sorted Array
//   題目：對已排序陣列就地去重，回傳新長度 k，前 k 個位置為去重結果。
//   為什麼用到本主題：這題的標準解法是 std::unique + erase，
//         而 **std::unique 的最低要求正好是 Forward Iterator** ——
//         因為它必須同時持有「讀」與「寫」兩個位置並各自前進，
//         那需要多次走訪（multi-pass）保證，Input Iterator 給不了。
//         下面刻意寫出三個版本，讓「分類決定你能用什麼」變成可執行的證據：
//           (a) vector（Random Access）→ std::unique 可用，且能用 - 算長度
//           (b) list（Bidirectional）  → std::unique 也可用（Forward 就夠），
//                                        但算長度只能用 std::distance（O(N)）
//           (c) forward_list（Forward）→ std::unique 仍可用，
//                                        但它有更好的成員函式 unique()
//   複雜度：unique 本身 O(N)；(a) 算 k 是 O(1)，(b)(c) 是 O(N)。
// -----------------------------------------------------------------------------
int removeDuplicates(std::vector<int>& nums) {
    auto new_end = std::unique(nums.begin(), nums.end());   // 只需 Forward
    int k = static_cast<int>(new_end - nums.begin());       // 這行需要 Random Access
    nums.erase(new_end, nums.end());
    return k;
}

void demo_leetcode_26() {
    std::cout << "\n===== LeetCode 26. Remove Duplicates from Sorted Array =====" << std::endl;

    std::vector<int> v = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    int k = removeDuplicates(v);
    std::cout << "  (a) vector [0,0,1,1,1,2,2,3,3,4] → k=" << k << "，內容 = ";
    for (int n : v) std::cout << n << " ";
    std::cout << std::endl;

    // list 是 Bidirectional：unique 仍可用（只需 Forward），
    // 但長度只能用 distance（O(N)），不能寫 new_end - begin()
    std::list<int> l = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    auto l_end = std::unique(l.begin(), l.end());
    auto lk = std::distance(l.begin(), l_end);              // O(N)：沒有 operator-
    l.erase(l_end, l.end());
    std::cout << "  (b) list   同樣資料          → k=" << lk << "，內容 = ";
    for (int n : l) std::cout << n << " ";
    std::cout << "  （長度只能用 distance，O(N)）" << std::endl;

    // forward_list 是 Forward：std::unique 仍成立，但成員版更好
    std::forward_list<int> f = {0, 0, 1, 1, 1, 2, 2, 3, 3, 4};
    f.unique();                                             // 成員版：不搬移元素
    std::cout << "  (c) forward_list 用成員 unique() → 內容 = ";
    for (int n : f) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  → std::unique 只要求 Forward，所以三種容器都能用；" << std::endl;
    std::cout << "    但「算長度」需要 Random Access 才有 O(1) 的減法" << std::endl;
}

// ===== 日常實務 =====
// -----------------------------------------------------------------------------
// 【日常實務範例】寫一個對所有容器都成立的「取樣」工具
//   情境：監控系統收到大量資料（可能存在 vector、也可能是 list），
//         要每隔 N 筆取一筆做抽樣，避免把全部資料都送進分析管線。
//   為什麼用到本主題：這個工具要能吃**任何**容器，
//         所以只能要求最低等級（Input Iterator）——
//         用 ++ 逐步前進而非 it += n。
//         這正是「泛型程式碼該要求最低而非最高」的實踐。
//         下面同時給出一個「只接受 Random Access」的最佳化版本，
//         示範兩者的取捨。
// -----------------------------------------------------------------------------

// 通用版：只要求 Input Iterator → vector / list / forward_list / 串流都能用
template <typename InIt, typename OutIt>
OutIt sampleEveryN(InIt first, InIt last, std::size_t n, OutIt out) {
    std::size_t i = 0;
    for (; first != last; ++first, ++i) {       // 只用 ++，最低要求
        if (i % n == 0) { *out = *first; ++out; }
    }
    return out;
}

// 最佳化版：要求 Random Access → 可以直接跳，少走很多步
template <typename RandIt, typename OutIt>
OutIt sampleEveryNFast(RandIt first, RandIt last, std::size_t n, OutIt out) {
    static_assert(
        std::is_base_of_v<std::random_access_iterator_tag,
                          typename std::iterator_traits<RandIt>::iterator_category>,
        "sampleEveryNFast 需要 Random Access Iterator");
    for (auto it = first; it < last; it += static_cast<std::ptrdiff_t>(n)) {
        *out = *it; ++out;                       // 直接跳，不必逐一走過
    }
    return out;
}

void demo_practical_sampling() {
    std::cout << "\n===== 日常實務：跨容器的抽樣工具 =====" << std::endl;

    std::vector<int>       v(20);
    std::iota(v.begin(), v.end(), 1);            // 1..20
    std::list<int>         l(v.begin(), v.end());
    std::forward_list<int> f(v.begin(), v.end());

    std::cout << "  原始資料 1..20，每 5 筆取一筆" << std::endl;

    std::vector<int> out_v, out_l, out_f;
    sampleEveryN(v.begin(), v.end(), 5, std::back_inserter(out_v));
    sampleEveryN(l.begin(), l.end(), 5, std::back_inserter(out_l));
    sampleEveryN(f.begin(), f.end(), 5, std::back_inserter(out_f));

    std::cout << "    vector       → ";
    for (int n : out_v) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "    list         → ";
    for (int n : out_l) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "    forward_list → ";
    for (int n : out_f) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  → 同一份程式碼服務三種容器，因為它只要求 Input Iterator"
              << std::endl;

    // 最佳化版只能用在 Random Access 容器上
    std::vector<int> out_fast;
    sampleEveryNFast(v.begin(), v.end(), 5, std::back_inserter(out_fast));
    std::cout << "  Random Access 專用版（直接跳，走訪步數少很多）→ ";
    for (int n : out_fast) std::cout << n << " ";
    std::cout << std::endl;
    std::cout << "  （對 list 呼叫這個版本會觸發 static_assert，"
                 "在編譯期就被擋下）" << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  第5課：迭代器的五種分類  總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_iterator_hierarchy();
    demo_input_iterator();
    demo_output_iterator();
    demo_forward_iterator();
    demo_bidirectional_iterator();
    demo_random_access_iterator();
    demo_algorithm_iterator_requirements();
    demo_advance_distance();
    demo_iterator_tags();
    demo_leetcode_26();
    demo_practical_sampling();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！五種分類：Input, Output, Forward, Bidirectional, RandomAccess" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ================================================================
//   第5課：迭代器的五種分類  總複習
// ================================================================
// ===== 重點一：迭代器階層 =====
// 五種迭代器（由弱到強）：
//   1. Input Iterator   - 只讀、只能前進、單次遍歷
//   2. Output Iterator  - 只寫、只能前進、單次遍歷
//   3. Forward Iterator - 可讀寫、只能前進、可多次遍歷
//   4. Bidirectional    - 可讀寫、可雙向、可多次遍歷
//   5. Random Access    - 完整功能：+n, -n, [], <, >, 距離計算
//
// vector[+2] = 4  (Random Access)
// list --后: 1  (Bidirectional)
// forward_list ++: 1  (Forward)
//
// ===== 重點二：Input Iterator =====
// 讀取: 10
// 前進後讀取: 20
// 從串流讀入 vector: 1 2 3 4 5
// （Input Iterator：只讀、只前進、單次遍歷）
//
// ===== 重點三：Output Iterator =====
// ostream_iterator 輸出: 10 20 30 40 50
// 手動寫入: 100 200 300
// back_inserter 結果: 1 2 3
// （Output Iterator：只寫、只前進、單次遍歷）
//
// ===== 重點四：Forward Iterator =====
// 第一次遍歷: 10 20 30 40 50
// 第二次遍歷: 10 20 30 40 50
// 保存的迭代器指向: 20
// 從保存位置繼續: 20 30 40 50
// （Forward Iterator：可讀寫、只前進、可多次遍歷）
//
// ===== 重點五：Bidirectional Iterator =====
// 起始: 10
// ++it: 20
// ++it: 30
// --it（後退）: 20
// set 正向: 10 20 30 40 50
// set 反向: 50 40 30 20 10
// （Bidirectional Iterator：可讀寫、可雙向、不能跳躍）
//
// ===== 重點六：Random Access Iterator =====
// *it = 10
// ++it: 20
// --it: 10
// it + 3: 40
// it - 2: 20
// it[2] = 30
// it[4] = 50
// end - begin = 5
// begin < mid? 是
// end > mid?   是
// 陣列 find(300) 索引 = 2
// （Random Access：完整功能，+n, -n, [], 距離, 比較）
//
// ===== 重點七：演算法對迭代器的要求 =====
// [find 需要 Input Iterator]
// vector/list/forward_list find(8): 找到 / 找到 / 找到
// [reverse 需要 Bidirectional Iterator]
// vector 反轉: 9 1 8 2 5
// list 反轉: 9 1 8 2 5
// [sort 需要 Random Access Iterator]
// vector 排序: 1 2 5 8 9
// list 排序: 1 2 5 8 9
// forward_list 排序: 1 2 5 8 9
//
// ===== 重點八：advance/distance/next/prev =====
// advance(it, 3) in vector: 40
// advance(it, 3) in list:   40
// distance(begin, end) in vector: 5
// distance(begin, end) in list:   5
// *it（原不變）= 10
// *next(it, 2) = 30
// *prev(end, 1) = 50
//
// ===== 重點九：迭代器標籤 =====
// 各容器迭代器類別：
//   vector:       Random Access Iterator
//   list:         Bidirectional Iterator
//   forward_list: Forward Iterator
//   原生指標 int*: Random Access Iterator
//   istream_iterator: Input Iterator
//   ostream_iterator: Output Iterator
//
// my_advance 根據標籤選擇實作：
// vector my_advance(it, 3):  （使用 += 跳躍，O(1)）
//   *it = 40
// list my_advance(it, 3):  （使用 ++/-- 逐步移動，O(n)）
//   *it = 40
// forward_list my_advance(it, 3):  （使用 ++ 逐步前進，O(n)）
//   *it = 40
//
// ===== LeetCode 26. Remove Duplicates from Sorted Array =====
//   (a) vector [0,0,1,1,1,2,2,3,3,4] → k=5，內容 = 0 1 2 3 4
//   (b) list   同樣資料          → k=5，內容 = 0 1 2 3 4   （長度只能用 distance，O(N)）
//   (c) forward_list 用成員 unique() → 內容 = 0 1 2 3 4
//   → std::unique 只要求 Forward，所以三種容器都能用；
//     但「算長度」需要 Random Access 才有 O(1) 的減法
//
// ===== 日常實務：跨容器的抽樣工具 =====
//   原始資料 1..20，每 5 筆取一筆
//     vector       → 1 6 11 16
//     list         → 1 6 11 16
//     forward_list → 1 6 11 16
//   → 同一份程式碼服務三種容器，因為它只要求 Input Iterator
//   Random Access 專用版（直接跳，走訪步數少很多）→ 1 6 11 16
//   （對 list 呼叫這個版本會觸發 static_assert，在編譯期就被擋下）
//
// ================================================================
//   複習完畢！五種分類：Input, Output, Forward, Bidirectional, RandomAccess
// ================================================================
