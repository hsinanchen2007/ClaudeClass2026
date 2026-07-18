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

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！五種分類：Input, Output, Forward, Bidirectional, RandomAccess" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
