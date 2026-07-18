/*
================================================================================
【C++_Iterator/summary.cpp】

本目錄主題：Iterator（迭代器）— STL 的「膠水」

你需要掌握的 3 個層次：
  1) iterator 是什麼：像指標一樣能 *it / ++it
  2) iterator category：input/output/forward/bidirectional/random_access
  3) iterator adaptor：reverse_iterator、insert_iterator、move_iterator...

為什麼重要？
  - STL 演算法（<algorithm>）不認得容器，只認得 iterator
  - 容器的 iterator 能力（category）決定你能用哪些演算法、以及複雜度

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

/*
補充筆記：C++_Iterator/C++_Iterator summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Iterator/C++_Iterator summary 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Iterator/C++_Iterator summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <algorithm>
#include <iostream>
#include <iterator>
#include <list>
#include <numeric>
#include <string>
#include <vector>

template <class C>
static void print_range(const C& c, const std::string& label) {
    std::cout << label << ":";
    for (const auto& x : c) std::cout << ' ' << x;
    std::cout << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 1】iterator category 與能做的事
// -----------------------------------------------------------------------------
static void demo_categories() {
    std::cout << "\n[demo_categories]\n";

    // vector 的 iterator 是 random_access：支援 it+n、it2-it1、< 等
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 1);
    auto it = v.begin();
    std::cout << "  vector begin=" << *it << ", begin+3=" << *(it + 3) << "\n";

    // list 的 iterator 是 bidirectional：能 ++ / --，但不能 it+3
    std::list<int> ls{1, 2, 3, 4, 5};
    auto it2 = ls.begin();
    std::advance(it2, 3); // 用 advance 走 3 步（對 list 會做 3 次 ++）
    std::cout << "  list advance 3 => " << *it2 << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 2】reverse_iterator：反向走訪（容器多半提供 rbegin/rend）
// -----------------------------------------------------------------------------
static void demo_reverse_iterator() {
    std::cout << "\n[demo_reverse_iterator]\n";
    std::vector<int> v{1, 2, 3, 4};
    std::cout << "  reverse:";
    for (auto rit = v.rbegin(); rit != v.rend(); ++rit) std::cout << ' ' << *rit;
    std::cout << "\n";

    // 注意：reverse_iterator 的 base() 指向「反向 iterator 所對應元素的下一個」
    auto rit = v.rbegin();        // 指到 4
    auto base = rit.base();       // 指到 end()
    std::cout << "  rbegin.base() == end()? " << (base == v.end()) << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】insert iterators：把演算法輸出「插入」到容器
// -----------------------------------------------------------------------------
static void demo_insert_iterators() {
    std::cout << "\n[demo_insert_iterators]\n";

    std::vector<int> src{1, 2, 3};
    std::vector<int> dst;

    // back_inserter：把 assign/transform 的輸出用 push_back 塞進去
    std::copy(src.begin(), src.end(), std::back_inserter(dst));
    print_range(dst, "  after copy(back_inserter)");

    // inserter：指定插入位置（對 list 這種容器常用）
    std::list<int> ls{10, 20, 30};
    auto pos = std::next(ls.begin()); // 指到 20
    std::copy(src.begin(), src.end(), std::inserter(ls, pos));
    print_range(ls, "  list after inserter(before 20)");
}

// -----------------------------------------------------------------------------
// 【重點 4】iterator invalidation（失效）：容器修改後 iterator 可能變成野指標
// -----------------------------------------------------------------------------
static void demo_invalidation_note() {
    std::cout << "\n[demo_invalidation_note]\n";
    std::cout << "  vector 擴容會讓所有 iterator/reference/pointer 失效。\n";
    std::cout << "  list 插入不會讓既有 iterator 失效（除非你刪掉那個元素）。\n";
    std::cout << "  這是容器選型非常重要的考量。\n";
}

// -----------------------------------------------------------------------------
// 【重點 5】自訂 iterator（概念示範：最小可用的 forward iterator）
// -----------------------------------------------------------------------------
// 這裡用一個極簡「計數 iterator」示範：像 Python range 一樣產生 [start, end)。
// 目的：讓你理解 iterator 其實就是「能被解參考 + 能前進 + 能比較終止」的型別。
struct CounterIt {
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int*;
    using reference = int;

    int cur{};
    int operator*() const { return cur; }
    CounterIt& operator++() {
        ++cur;
        return *this;
    }
    bool operator==(const CounterIt& other) const { return cur == other.cur; }
    bool operator!=(const CounterIt& other) const { return !(*this == other); }
};

static void demo_custom_iterator() {
    std::cout << "\n[demo_custom_iterator]\n";

    CounterIt b{1};
    CounterIt e{6}; // 走到 6 停

    int sum = std::accumulate(b, e, 0);
    std::cout << "  accumulate(1..5) sum=" << sum << "\n";
}

int main() {
    demo_categories();
    demo_reverse_iterator();
    demo_insert_iterators();
    demo_invalidation_note();
    demo_custom_iterator();

    std::cout << "\n[done]\n";
    return 0;
}

