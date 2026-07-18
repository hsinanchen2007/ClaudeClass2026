/*
 * ================================================================
 * 【第4課：迭代器（Iterator）的核心概念】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -o summary summary.cpp
 * 本課重點：
 * 1. 迭代器的本質：泛化的指標，提供統一的容器遍歷介面
 * 2. 基本操作：*it（解參考）、++it（前進）、it1 != it2（比較）
 * 3. begin() 與 end()：半開區間 [begin, end) 的設計
 * 4. auto 簡化迭代器宣告
 * 5. const_iterator（唯讀）與 reverse_iterator（反向）
 * 6. 迭代器失效問題與安全刪除模式
 * 7. 迭代器與指標的關係
 * 8. 範圍 for 的底層原理
 * 9. 自訂類別支援迭代器（begin/end）
 * ================================================================
 * 目錄：
 *   重點一：迭代器的概念與基本操作
 *   重點二：begin() / end() 與半開區間設計
 *   重點三：auto 簡化、const_iterator、reverse_iterator
 *   重點四：迭代器與演算法協作
 *   重點五：迭代器失效與安全刪除
 *   重點六：迭代器與指標的關係
 *   重點七：範圍 for 底層原理
 *   重點八：自訂類別支援迭代器
 * ================================================================
 */

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>

// ===== 重點一：迭代器的概念與基本操作 =====
// 迭代器是「泛化的指標」：
//   - 抽象了不同容器的遍歷細節
//   - 提供統一介面：*it（讀值）、++it（前進）、it1 != it2（比較）
// 相比原始指標：指標只能遍歷連續記憶體（陣列）；
//   迭代器可以遍歷任何容器，包括鏈結串列、樹、雜湊表。

void demo_basic_operations() {
    std::cout << "===== 重點一：迭代器基本操作 =====" << std::endl;

    // 指標遍歷 C 陣列（原始形式）
    int arr[] = {10, 20, 30, 40, 50};
    std::cout << "指標遍歷陣列: ";
    for (int* ptr = arr; ptr < arr + 5; ++ptr) {
        std::cout << *ptr << " ";
    }
    std::cout << std::endl;

    // 迭代器遍歷 vector（相同概念，更通用）
    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 完整型別宣告
    std::vector<int>::iterator it = vec.begin();

    std::cout << "\n迭代器基本操作：" << std::endl;
    std::cout << "*it = " << *it << std::endl;          // 解參考：取得值

    ++it;                                                // 前置遞增（推薦，效率較高）
    std::cout << "++it 後 *it = " << *it << std::endl;

    std::cout << "*it++ = " << *it++ << std::endl;      // 後置遞增：先取值，再前進
    std::cout << "現在 *it = " << *it << std::endl;

    // 比較操作
    auto begin_it = vec.begin();
    auto end_it = vec.end();
    std::cout << "begin == end? " << (begin_it == end_it ? "是" : "否") << std::endl;
    std::cout << "begin != end? " << (begin_it != end_it ? "是" : "否") << std::endl;
}

// ===== 重點二：begin() / end() 與半開區間 =====
// STL 採用半開區間 [begin, end)：
//   - begin() 指向第一個元素
//   - end() 指向「最後一個元素的下一個位置」（past-the-end）
//   - end() 絕對不能被解參考！
//
// 半開區間的好處：
//   1. 空容器：begin() == end()，邏輯清晰
//   2. 遍歷終止條件統一：it != end()
//   3. 子區間表示簡潔：[begin+1, begin+4)
//   4. 計算元素個數：end() - begin()（隨機存取迭代器）

void demo_begin_end() {
    std::cout << "\n===== 重點二：begin/end 與半開區間 =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 1. 空容器判斷
    std::vector<int> empty_vec;
    if (empty_vec.begin() == empty_vec.end()) {
        std::cout << "空容器：begin() == end()" << std::endl;
    }

    // 2. 標準遍歷
    std::cout << "遍歷容器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 3. 計算元素個數（隨機存取迭代器可用 - 運算）
    std::cout << "元素個數: " << (vec.end() - vec.begin()) << std::endl;

    // 4. 子區間操作 [1, 4) 即元素 20, 30, 40
    auto start  = vec.begin() + 1;
    auto finish = vec.begin() + 4;
    std::cout << "子區間 [1, 4): ";
    for (auto it = start; it != finish; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

// ===== 重點三：auto 簡化、const_iterator、reverse_iterator =====
// 迭代器型別常常很長，auto 可以大幅簡化。
// 迭代器有四種變體（以 vector 為例）：
//   begin()/end()   → iterator（可讀可寫，正向）
//   cbegin()/cend() → const_iterator（唯讀，正向）
//   rbegin()/rend() → reverse_iterator（可讀可寫，反向）
//   crbegin()/crend() → const_reverse_iterator（唯讀，反向）

void print_vec(const std::vector<int>& vec) {
    // const 容器參數只能使用 const_iterator（或 cbegin/cend）
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        // *it = 99;  // 編譯錯誤！const_iterator 不允許修改
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

void demo_iterator_types() {
    std::cout << "\n===== 重點三：迭代器型別 =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // auto 大幅簡化迭代器宣告
    auto it = vec.begin();  // 等同 std::vector<int>::iterator it = vec.begin();
    std::cout << "auto 迭代器 *it = " << *it << std::endl;

    // 對 map 這類複雜型別，auto 更重要
    std::map<std::string, int> scores = {{"Alice", 95}, {"Bob", 87}};
    auto map_it = scores.begin();  // 否則型別是 std::map<std::string,int>::iterator
    std::cout << "map 首項: " << map_it->first << "=" << map_it->second << std::endl;

    // 一般迭代器：可讀可寫
    std::cout << "修改前: ";
    print_vec(vec);
    for (auto it2 = vec.begin(); it2 != vec.end(); ++it2) {
        *it2 *= 2;  // 透過迭代器修改值
    }
    std::cout << "修改後(x2): ";
    print_vec(vec);

    // const_iterator：唯讀（透過 cbegin/cend 明確取得）
    std::cout << "const_iterator: ";
    for (auto cit = vec.cbegin(); cit != vec.cend(); ++cit) {
        std::cout << *cit << " ";
    }
    std::cout << std::endl;

    // reverse_iterator：從後往前（透過 rbegin/rend）
    std::cout << "reverse_iterator: ";
    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
        // 注意：reverse_iterator 的 ++ 邏輯上往「左」移動
        std::cout << *rit << " ";
    }
    std::cout << std::endl;
}

// ===== 重點四：迭代器與演算法協作 =====
// 迭代器是演算法與容器之間的橋樑：
//   - 同一演算法可以在不同容器上運作
//   - 可以指定「子區間」讓演算法只處理部分元素

void demo_iterator_algorithm() {
    std::cout << "\n===== 重點四：迭代器與演算法 =====" << std::endl;

    // 同一個 find 演算法，用在 vector 和 list
    std::vector<int> vec = {5, 2, 8, 1, 9};
    std::list<int>   lst = {5, 2, 8, 1, 9};

    auto vec_it = std::find(vec.begin(), vec.end(), 8);
    auto lst_it = std::find(lst.begin(), lst.end(), 8);

    std::cout << "find(8) in vector: " << (vec_it != vec.end() ? "找到" : "沒找到") << std::endl;
    std::cout << "find(8) in list:   " << (lst_it != lst.end() ? "找到" : "沒找到") << std::endl;

    // count 也是如此
    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2};
    int cnt = std::count(data.begin(), data.end(), 2);
    std::cout << "count(2) = " << cnt << std::endl;

    // 子區間操作：只在 [begin+1, begin+5) 的範圍內搜尋
    std::vector<int> nums = {10, 20, 30, 40, 50, 60, 70};
    auto sub_it = std::find(nums.begin() + 1, nums.begin() + 5, 40);
    if (sub_it != nums.begin() + 5) {
        std::cout << "在子區間 [1,5) 找到 40，索引 = "
                  << (sub_it - nums.begin()) << std::endl;
    }
}

// ===== 重點五：迭代器失效與安全刪除 =====
// 當容器被修改（插入/刪除）時，現有迭代器可能「失效」（指向無效位置）。
// 失效規則（重要！）：
//   vector：插入可能導致全部失效；刪除導致被刪元素之後的迭代器失效
//   list：插入不影響；刪除只讓被刪元素的迭代器失效
//   set/map：插入不影響；刪除只讓被刪元素的迭代器失效
//
// 安全刪除方式：
//   方法一：使用 erase() 的回傳值（回傳下一個有效迭代器）
//   方法二：erase-remove 慣用法（vector 最常用）
//   方法三：list 的成員函數 remove()

void demo_iterator_invalidation() {
    std::cout << "\n===== 重點五：迭代器失效與安全刪除 =====" << std::endl;

    // 方法一：使用 erase 的回傳值（適用於所有容器）
    // 錯誤做法（不要這樣做）：
    // for (auto it = vec.begin(); it != vec.end(); ++it)
    //     if (*it == 3) vec.erase(it);  // 刪除後 it 失效！UB！

    // 正確做法：erase 回傳下一個有效迭代器
    std::cout << "[方法一：erase 回傳值]" << std::endl;
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::cout << "原始: ";
    for (int n : vec1) std::cout << n << " ";
    std::cout << std::endl;

    for (auto it = vec1.begin(); it != vec1.end(); /* 不在這裡 ++ */ ) {
        if (*it == 3) {
            it = vec1.erase(it);  // erase 回傳下一個有效迭代器
        } else {
            ++it;
        }
    }
    std::cout << "刪除3後: ";
    for (int n : vec1) std::cout << n << " ";
    std::cout << std::endl;

    // 方法二：erase-remove 慣用法（最簡潔，適合 vector）
    // std::remove 把不要的元素移到尾端，回傳新的邏輯尾端迭代器
    // 再用 erase 刪除邏輯尾端到真正尾端的部分
    std::cout << "[方法二：erase-remove 慣用法]" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 2, 4, 2, 5};
    vec2.erase(std::remove(vec2.begin(), vec2.end(), 2), vec2.end());
    std::cout << "刪除所有2後: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;

    // 方法三：list 的成員函數 remove（最適合 list）
    std::cout << "[方法三：list::remove]" << std::endl;
    std::list<int> lst = {1, 2, 3, 2, 4, 2, 5};
    lst.remove(2);
    std::cout << "刪除所有2後: ";
    for (int n : lst) std::cout << n << " ";
    std::cout << std::endl;
}

// ===== 重點六：迭代器與指標的關係 =====
// 迭代器的設計靈感來自指標，操作介面刻意設計成一致。
// 原生指標（T*）本身就滿足 Random Access Iterator 的所有要求，
// 因此 STL 演算法可以直接用在 C 風格陣列上。

void demo_iterator_vs_pointer() {
    std::cout << "\n===== 重點六：迭代器與指標 =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 取得底層連續記憶體的指標（C++11 的 data()）
    int* ptr = vec.data();
    auto it  = vec.begin();

    std::cout << "指標解參考: *ptr = " << *ptr << std::endl;
    std::cout << "迭代器解參考: *it = " << *it << std::endl;
    std::cout << "指標算術: *(ptr+2) = " << *(ptr + 2) << std::endl;
    std::cout << "迭代器算術: *(it+2) = " << *(it + 2) << std::endl;

    // 取得迭代器指向的元素的實際記憶體位址
    int* from_it = &(*it);
    std::cout << "*from_it = " << *from_it << std::endl;

    // 原始指標就是一種 Random Access Iterator！
    // 所以 STL 演算法可以直接用在 C 陣列
    int arr[] = {100, 200, 300, 400, 500};
    auto found = std::find(arr, arr + 5, 300);  // arr 和 arr+5 當作迭代器
    if (found != arr + 5) {
        std::cout << "在陣列中找到 300，索引=" << (found - arr) << std::endl;
    }

    // std::sort 也可以直接用在 C 陣列
    int arr2[] = {5, 3, 1, 4, 2};
    std::sort(arr2, arr2 + 5);
    std::cout << "陣列排序: ";
    for (int i = 0; i < 5; ++i) std::cout << arr2[i] << " ";
    std::cout << std::endl;
}

// ===== 重點七：範圍 for 底層原理 =====
// C++11 的範圍 for 只是迭代器的語法糖：
//
//   for (int n : vec) { ... }
//
// 等價於：
//
//   auto&& __range = vec;
//   auto __begin = __range.begin();
//   auto __end   = __range.end();
//   for (; __begin != __end; ++__begin) {
//       int n = *__begin;
//       ...
//   }
//
// 要修改元素，要用參考 for (int& n : vec)

void demo_range_for() {
    std::cout << "\n===== 重點七：範圍 for 的底層 =====" << std::endl;

    std::vector<int> vec = {10, 20, 30, 40, 50};

    // 範圍 for（語法糖）
    std::cout << "範圍 for（唯讀）: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 等價的迭代器寫法
    std::cout << "等價迭代器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // 使用 const 參考（高效唯讀，避免複製）
    std::cout << "範圍 for（const 參考）: ";
    for (const int& n : vec) std::cout << n << " ";
    std::cout << std::endl;

    // 使用參考修改元素
    for (int& n : vec) n *= 2;
    std::cout << "修改後（x2）: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
}

// ===== 重點八：自訂類別支援迭代器 =====
// 任何提供 begin() 和 end() 的類別都可以使用範圍 for。
// 迭代器類別需要實作：
//   - operator*()   （解參考）
//   - operator++()  （前置遞增）
//   - operator!=()  （不等比較）

class IntRange {
    int start_, end_;
public:
    // 內嵌迭代器類別
    class Iterator {
        int current_;
    public:
        explicit Iterator(int v) : current_(v) {}
        int operator*() const { return current_; }
        Iterator& operator++() { ++current_; return *this; }
        bool operator!=(const Iterator& other) const {
            return current_ != other.current_;
        }
    };

    IntRange(int start, int end) : start_(start), end_(end) {}
    Iterator begin() const { return Iterator(start_); }
    Iterator end()   const { return Iterator(end_); }
};

void demo_custom_iterator() {
    std::cout << "\n===== 重點八：自訂類別支援迭代器 =====" << std::endl;

    IntRange range(1, 6);  // 代表 1, 2, 3, 4, 5

    // 可以使用範圍 for
    std::cout << "範圍 for: ";
    for (int n : range) std::cout << n << " ";
    std::cout << std::endl;

    // 也可以使用迭代器
    std::cout << "迭代器: ";
    for (auto it = range.begin(); it != range.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "================================================================" << std::endl;
    std::cout << "  第4課：迭代器（Iterator）的核心概念  總複習" << std::endl;
    std::cout << "================================================================" << std::endl;

    demo_basic_operations();
    demo_begin_end();
    demo_iterator_types();
    demo_iterator_algorithm();
    demo_iterator_invalidation();
    demo_iterator_vs_pointer();
    demo_range_for();
    demo_custom_iterator();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！迭代器：泛化指標、半開區間、失效問題、自訂支援" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
