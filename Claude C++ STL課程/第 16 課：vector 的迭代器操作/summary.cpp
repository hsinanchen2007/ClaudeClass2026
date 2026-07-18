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
#include <iterator>  // std::distance, std::advance, std::next, std::prev

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
