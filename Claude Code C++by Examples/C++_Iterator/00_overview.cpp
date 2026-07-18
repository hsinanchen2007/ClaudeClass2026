// =============================================================================
//  00_overview.cpp  —  C++ STL Iterator 總覽 + 速查表
// =============================================================================
//  Iterator 是「指標的泛化」。STL 容器和演算法之間靠 iterator 解耦：
//      容器 ──提供 iterator──→ 演算法
//
//  五大類別 (Category)，從弱到強，後者繼承前者所有能力：
//
//      input_iterator_tag           只能讀、單次走訪
//      output_iterator_tag          只能寫、單次走訪
//      forward_iterator_tag         可讀寫、可多次走訪、單向
//      bidirectional_iterator_tag   可讀寫、雙向 (++ 與 --)
//      random_access_iterator_tag   可讀寫、O(1) 跳躍 (it + n, it[n], it1 - it2)
//
//  (C++20 之後還新增了 contiguous_iterator_tag，本專案略過)
//
//  std::iterator_traits<It> 提供了 5 個 typedef，讓泛型演算法可以萃取資訊：
//      ::value_type        指向元素的型別
//      ::difference_type   兩個 iterator 相減的型別 (通常 ptrdiff_t)
//      ::pointer           value_type*
//      ::reference         value_type&
//      ::iterator_category 上面五個 tag 之一
//
//  小提醒：
//   * std::iterator (基底類別) 自 C++17 已 deprecated，本專案不使用。
//   * 演算法常根據 iterator_category 選擇最佳實作 (tag dispatch)。
//
// =============================================================================
//                       === 速查表 1：操作 vs Iterator 類別 ===
// =============================================================================
//
//   操作                  | Input | Output | Forward | Bidir | RandomAcc
//   ──────────────────────┼───────┼────────┼─────────┼───────┼──────────
//   *it (讀)              |  ✓    |   ✗    |   ✓     |  ✓    |   ✓
//   *it = x (寫)          |  ✗    |   ✓    |  ✓(*)   | ✓(*)  |  ✓(*)
//   ++it / it++           |  ✓    |   ✓    |   ✓     |  ✓    |   ✓
//   --it / it--           |  ✗    |   ✗    |   ✗     |  ✓    |   ✓
//   it == it2 / !=        |  ✓    |   ✗    |   ✓     |  ✓    |   ✓
//   it +  n / it -  n     |  ✗    |   ✗    |   ✗     |  ✗    |   ✓
//   it += n / it -= n     |  ✗    |   ✗    |   ✗     |  ✗    |   ✓
//   it[n]                 |  ✗    |   ✗    |   ✗     |  ✗    |   ✓
//   it1 -  it2            |  ✗    |   ✗    |   ✗     |  ✗    |   ✓
//   it1 <  it2 / <=/>/>=  |  ✗    |   ✗    |   ✗     |  ✗    |   ✓
//   多次走訪 (multi-pass) |  ✗    |   ✗    |   ✓     |  ✓    |   ✓
//
//   (*) Forward/Bidir/RandomAccess 「也能寫」的前提是它指向非 const 元素，
//       例如 vector<int> 的 iterator 可寫，vector<int>::const_iterator 不行.
//
// =============================================================================
//                === 速查表 2：常見演算法需要的 Iterator 等級 ===
// =============================================================================
//
//   演算法                            | 最低需求 Iterator
//   ──────────────────────────────────┼─────────────────────
//   for_each / find / find_if / count |  Input
//   accumulate / inner_product        |  Input
//   copy (來源端)                     |  Input
//   copy (目的端)                     |  Output
//   transform / fill / generate       |  Input + Output
//   adjacent_find / search / unique   |  Forward
//   replace / remove (in-place)       |  Forward
//   rotate                            |  Forward
//   reverse                           |  Bidirectional
//   inplace_merge                    |  Bidirectional
//   stable_partition                  |  Bidirectional
//   sort / stable_sort / partial_sort |  RandomAccess
//   nth_element                       |  RandomAccess
//   binary_search / lower_bound       |  Forward 即可，但只有 RandomAccess
//                                     |  能達到 O(log n) 「跳躍」
//   shuffle / random_shuffle          |  RandomAccess
//
//   小結：寫泛型程式時「需求越低越好」— 這樣可接受的容器越多.
//
// =============================================================================
//                === 速查表 3：容器 → Iterator 等級 ===
// =============================================================================
//
//   容器                          | Iterator 等級     | 連續記憶體?
//   ──────────────────────────────┼──────────────────┼─────────────
//   array<T,N>                    | RandomAccess     | 是
//   vector<T> (T != bool)         | RandomAccess     | 是
//   vector<bool>                  | RandomAccess     | 否 (proxy)
//   string                        | RandomAccess     | 是
//   deque<T>                      | RandomAccess     | 否 (chunked)
//   list<T>                       | Bidirectional    | 否
//   set / multiset                | Bidirectional    | 否
//   map / multimap                | Bidirectional    | 否
//   forward_list<T>               | Forward          | 否
//   unordered_set / map / 變體     | Forward          | 否
//   istream_iterator              | Input            | -
//   ostream_iterator / inserter   | Output           | -
//
// =============================================================================
//                  === 速查表 4：Iterator Adaptor 速查 ===
// =============================================================================
//
//   Adaptor                  | 出自 | 用途                    | 工廠函式
//   ─────────────────────────┼──────┼─────────────────────────┼────────────────────
//   reverse_iterator         | C++98| 反向走訪                | make_reverse_iterator (C++14)
//   back_insert_iterator     | C++98| 在尾巴 push_back        | back_inserter
//   front_insert_iterator    | C++98| 在頭部 push_front       | front_inserter
//   insert_iterator          | C++98| 在指定位置 insert       | inserter
//   move_iterator            | C++11| 把 *it 變成 rvalue ref  | make_move_iterator
//   istream_iterator         | C++98| 讀格式化資料            | -
//   ostream_iterator         | C++98| 寫格式化資料            | -
//   istreambuf_iterator      | C++98| 讀字元 (不跳空白)       | -
//   ostreambuf_iterator      | C++98| 寫字元                  | -
//
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/iterator                — Iterator 總覽
//    https://en.cppreference.com/w/cpp/iterator/iterator_traits — iterator_traits
//    https://en.cppreference.com/w/cpp/iterator/iterator_tags   — 五個 tag 類別
//    https://cplusplus.com/reference/iterator/                  — 簡明對照
// =============================================================================

/*
補充筆記：overview
  - overview 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - iterator 是 STL 演算法和容器之間的橋樑；演算法只需要知道如何走訪，不需要知道容器具體型別。
  - begin/end 形成半開區間，end 是停止標記，不是最後元素。
  - 理解 iterator category 後，才知道某演算法為什麼能不能用在 list、vector 或 stream 上。
*/
#include <iostream>
#include <iterator>
#include <vector>
#include <list>
#include <forward_list>
#include <set>
#include <deque>
#include <typeinfo>
#include <numeric>     // std::partial_sum (LC1480)
#include <string>

// 把 tag 轉成可讀的字串，方便 demo
template <class Tag>
const char* tag_name() {
    if constexpr (std::is_same_v<Tag, std::input_iterator_tag>)         return "input";
    else if constexpr (std::is_same_v<Tag, std::output_iterator_tag>)   return "output";
    else if constexpr (std::is_same_v<Tag, std::forward_iterator_tag>)  return "forward";
    else if constexpr (std::is_same_v<Tag, std::bidirectional_iterator_tag>) return "bidirectional";
    else if constexpr (std::is_same_v<Tag, std::random_access_iterator_tag>) return "random_access";
    else return "unknown";
}

template <class It>
void print_category(const char* name) {
    using cat = typename std::iterator_traits<It>::iterator_category;
    std::cout << "  " << name << " → " << tag_name<cat>() << "_iterator\n";
}

int main() {
    // -----------------------------------------------------------------------
    // Demo 1：列出常見容器的 iterator 類別 — 對照速查表 3
    // -----------------------------------------------------------------------
    std::cout << "[各容器的 iterator 類別]\n";
    print_category<std::vector<int>::iterator>("vector<int>");          // random_access
    print_category<std::deque<int>::iterator>("deque<int>");            // random_access
    print_category<std::list<int>::iterator>("list<int>");              // bidirectional
    print_category<std::set<int>::iterator>("set<int>");                // bidirectional
    print_category<std::forward_list<int>::iterator>("forward_list<int>"); // forward
    print_category<int*>("int* (raw pointer)");                         // random_access

    // -----------------------------------------------------------------------
    // Demo 2：iterator_traits 五個 typedef 全部抽出來看
    // -----------------------------------------------------------------------
    std::cout << "\n[iterator_traits<vector<double>::iterator>]\n";
    using It = std::vector<double>::iterator;
    using T  = std::iterator_traits<It>;
    std::cout << "  value_type        = " << typeid(T::value_type).name()
              << " (期望 d = double)\n";
    std::cout << "  difference_type   = " << typeid(T::difference_type).name()
              << " (通常 l = long = ptrdiff_t)\n";
    std::cout << "  pointer           = " << typeid(T::pointer).name() << '\n';
    std::cout << "  reference         = " << typeid(T::reference).name() << '\n';
    std::cout << "  iterator_category = "
              << tag_name<T::iterator_category>() << "_iterator_tag\n";

    // -----------------------------------------------------------------------
    // Demo 3：std::distance 的多型 — 不同類別走不同的內部實作
    //   * RandomAccess: O(1) 直接相減
    //   * 其他:         O(n) 邊走邊數
    //   * 你只看到一個介面，但底層 tag dispatch 會選最佳路徑.
    // -----------------------------------------------------------------------
    std::cout << "\n[std::distance 對不同類別都通用]\n";
    std::vector<int>       v{1,2,3,4,5};
    std::list<int>         l{1,2,3,4,5};
    std::forward_list<int> f{1,2,3,4,5};
    std::cout << "  vector       distance = " << std::distance(v.begin(), v.end()) << '\n';
    std::cout << "  list         distance = " << std::distance(l.begin(), l.end()) << '\n';
    std::cout << "  forward_list distance = " << std::distance(f.begin(), f.end()) << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::sort 為什麼最低需求是 RandomAccessIterator,而 std::find 卻只要 InputIterator?
    //    A:sort 內部是 introsort,需要做 pivot 選擇、二分切割、it+n 跳躍與 it[n] 索引,
    //      這些操作必須 O(1) 才能維持整體 O(n log n)。find 只是逐個比對到匹配為止,
    //      單向走訪一次即可,給最弱的 InputIterator 也能跑,因此可吃 stream。
    //
    //  Q2：std::binary_search 的最低需求是 ForwardIterator 還是 RandomAccessIterator?
    //    A:標準規範「最低需求」是 ForwardIterator(因為它用 lower_bound 實作,
    //      只要能 ++ 與比較),但若 iterator 不是 RandomAccess,std::advance 變 O(n),
    //      整體就退化成 O(n) 而非 O(log n)。所以「能跑」與「能跑得快」是兩回事。
    //
    //  Q3：iterator_traits 為什麼是「型別萃取」而不是直接呼叫 it 的成員?
    //    A:因為原生指標 (T*) 不是 class,它沒有內建 typedef。iterator_traits 對
    //      T* 提供了特化版,讓泛型演算法 (例如 std::distance) 可以用統一寫法
    //      typename iterator_traits<It>::iterator_category 萃取資訊,
    //      不論 It 是自製 class 還是 raw pointer 都通用。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 1480 — Running Sum of 1D Array (前綴和)
    // -----------------------------------------------------------------------
    // 給整數陣列 nums,回傳 running sum:result[i] = nums[0]+...+nums[i]。
    // 用 std::partial_sum 一行解決 — 這個演算法的最低需求是 InputIterator (來源)
    // + OutputIterator (目的),展示「iterator 類別決定演算法可吃哪些容器」的核心思想。
    {
        std::vector<int> nums{1, 2, 3, 4};
        std::vector<int> running(nums.size());
        std::partial_sum(nums.begin(), nums.end(), running.begin());
        std::cout << "\n[LC1480 RunningSum] = [ ";
        for (int x : running) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 1 3 6 10 ]
    }

    // -----------------------------------------------------------------------
    // 實戰範例:Logger 寫入抽象 — iterator-style sink
    // -----------------------------------------------------------------------
    // 真實場景:一個 log 函式不該綁定 vector / list / file 哪種「sink」,只要對方
    // 提供 OutputIterator 就能寫。下面寫一個 log_all() 接受任意 OutputIterator,
    // 同樣的程式碼可寫到 vector、寫到 stdout (ostream_iterator)、或寫到 list。
    // 這正是 iterator 設計哲學的精髓 — 介面與容器解耦。
    {
        std::vector<std::string> events{"start", "load_config", "ready"};
        auto log_all = [](auto src_begin, auto src_end, auto out) {
            for (auto it = src_begin; it != src_end; ++it, ++out) {
                *out = "[INFO] " + *it;
            }
        };
        // sink 1: vector
        std::vector<std::string> file_buf(events.size());
        log_all(events.begin(), events.end(), file_buf.begin());
        std::cout << "[Logger -> vector]\n";
        for (auto& s : file_buf) std::cout << "  " << s << '\n';
        // 預期輸出: 三行 [INFO] xxx
    }

    return 0;
}
