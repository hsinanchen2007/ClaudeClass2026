// =============================================================================
//  第四課：迭代器的核心概念 12  —  本課教科書：自訂類別支援迭代器與範圍 for
// =============================================================================
//
// 【主題資訊 Information】
//   本檔是第四課的完整講義（下方 // 註解區為全文），
//   最後附一個「自己寫一個能被範圍 for 走訪的類別」的完整範例。
//   讓自訂類別支援範圍 for 的最低門檻：
//     類別本身   提供 begin() 與 end()（成員函式，或 ADL 找得到的自由函式）
//     迭代器類別 提供 operator*、operator++（前置）、operator!=
//   若還要讓 STL 演算法能用，額外需要 iterator_traits 的五個 typedef：
//     value_type、difference_type、pointer、reference、iterator_category
//   標準版本：範圍 for 為 C++11；C++17 放寬 begin/end 可為不同型別（sentinel）；
//             C++20 的 ranges 與 concepts 讓「合格迭代器」有了正式的編譯期檢查。
//   複雜度：本範例的 IntRange 走訪為 O(N)，且不配置任何記憶體。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「不需要繼承任何基底類別」是關鍵設計】
//   Java 的 for-each 要求物件實作 Iterable 介面；C# 要求 IEnumerable。
//   C++ 完全不需要 —— 編譯器只是在展開範圍 for 時去「找找看有沒有 begin/end」，
//   找到就用。這種「以行為而非繼承來定義相容性」的做法稱為 duck typing 的
//   編譯期版本，在 C++ 中叫做「概念（concept）」。
//   好處：
//     (a) 零成本 —— 沒有虛擬函式、沒有 vtable、可完全 inline
//     (b) 非侵入式 —— 你甚至可以為「別人寫的、不能改的類別」
//         在外部提供自由函式 begin(obj)/end(obj)，讓它突然支援範圍 for
//   代價：不滿足要求時的錯誤訊息很難讀（C++20 concepts 就是為了修這件事）。
//
// 【2. IntRange 示範的是「惰性序列」——不佔記憶體的容器】
//   IntRange(1, 1000000) 並沒有真的產生一百萬個 int，
//   它只存了 start_ 與 end_ 兩個整數；元素是在走訪時「算出來」的。
//   這正是 Python 的 range()、C++20 的 std::views::iota 背後的想法：
//   **迭代器不必指向真實存在的記憶體**，它只要能回答「現在的值是什麼」
//   與「怎麼走到下一個」即可。
//   這也解釋了為什麼 operator* 回傳 int 而非 int& —— 沒有元素可以被參考。
//   （這種迭代器不滿足 Forward Iterator 的要求，因為 Forward 要求
//     operator* 回傳真正的參考；它只是一個 Input Iterator。）
//
// 【3. 為什麼最少只要 *、++、!= 三個運算子】
//   直接看範圍 for 展開後的樣子就明白：
//       for (; __begin != __end; ++__begin) { int n = *__begin; ... }
//   全部用到的就是這三個。至於 ==、--、+n、<，範圍 for 一個都不需要。
//   （但若你想讓 std::find 等演算法也能用，就得補上 iterator_traits 的
//     五個 typedef，否則樣板實例化會失敗。）
//
// 【4. begin() 與 end() 為什麼要是 const 成員】
//   本範例中兩者都宣告成 const，這讓 `const IntRange r(1,6);`
//   也能被範圍 for 走訪。如果忘了加 const，
//   對 const 物件的範圍 for 就會編譯失敗 —— 這是自訂容器最常見的疏漏之一。
//   標準容器則同時提供 const 與非 const 兩組多載。
//
// 【概念補充 Concept Deep Dive】
//   如何判斷你的自訂迭代器「夠不夠格」給 STL 演算法用？
//   關鍵在 std::iterator_traits<It> 能不能取得五個型別。
//   本檔的 IntRange::Iterator **不能**直接用在 std::find 上，
//   因為它沒有提供那些 typedef。要讓它合格，需在類別內加上：
//       using iterator_category = std::input_iterator_tag;
//       using value_type        = int;
//       using difference_type   = std::ptrdiff_t;
//       using pointer           = const int*;
//       using reference         = int;          // 惰性序列回傳值而非參考
//   （C++17 以前的舊教材會教你繼承 std::iterator<...>，
//     但那個基底類別在 C++17 已被 deprecated，請改用直接宣告 typedef。）
//   加上之後 std::find(r.begin(), r.end(), 3) 才能編譯。
//   本範例刻意不加，是為了凸顯「範圍 for 的門檻」比「STL 演算法的門檻」低很多。
//
// 【注意事項 Pay Attention】
//   1. begin()/end() 必須宣告成 const 成員，否則 const 物件無法被範圍 for 走訪。
//   2. 只實作 *、++、!= 僅足以支援範圍 for；要餵給 STL 演算法還需要
//      iterator_traits 的五個 typedef。
//   3. 惰性序列的 operator* 回傳「值」而非「參考」，因此它只是 Input Iterator，
//      不能用在要求 Forward Iterator 以上的演算法（如 std::unique）。
//   4. 自訂迭代器要小心「半開區間」語意：end() 必須是一個不會被解參考、
//      且從 begin() 一定走得到的值，否則會無窮迴圈。
//   5. std::iterator<...> 基底類別在 C++17 已 deprecated，不要再繼承它。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自訂類別支援迭代器
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 要讓自訂類別能被範圍 for 走訪，最少需要實作什麼？
//     答：類別要提供 begin() 與 end()（成員函式，或 ADL 找得到的自由函式）；
//         它們回傳的迭代器物件最少要支援 operator*、前置 operator++
//         與 operator!=。就這三個，因為範圍 for 展開後只用到它們。
//         **不需要**繼承任何基底類別或實作任何介面。
//     追問：那要讓 std::find 也能用呢？
//           → 還要提供 iterator_traits 需要的五個 typedef：
//             iterator_category、value_type、difference_type、pointer、reference。
//             注意舊式的「繼承 std::iterator<>」寫法在 C++17 已 deprecated。
//
// 🔥 Q2. 為什麼 IntRange(1, 1000000) 不會佔用一百萬個 int 的記憶體？
//     答：因為它是「惰性序列」—— 物件本身只存 start_ 與 end_ 兩個整數，
//         元素在走訪時才被計算出來。迭代器不必指向真實存在的記憶體，
//         它只要能回答「目前的值」與「如何前進」即可。
//         Python 的 range()、C++20 的 std::views::iota 都是同樣的想法。
//     追問：這種迭代器的 operator* 應該回傳 int 還是 int&？
//           → 只能回傳 int（值）。因為沒有真實元素可供參考。
//             這也決定了它只能是 Input Iterator ——
//             Forward Iterator 以上要求 operator* 回傳真正的參考。
//
// ⚠️ 陷阱. 自訂容器寫好 begin()/end() 後，`const MyRange r(1,6); for (int n : r)`
//          卻編譯失敗，為什麼？
//     答：因為 begin()/end() 忘了宣告成 const 成員函式。
//         對 const 物件只能呼叫 const 成員函式，
//         非 const 的 begin() 無法被選中 → 找不到可用的 begin → 編譯錯誤。
//         標準容器之所以沒這個問題，是因為它們同時提供 const 與非 const 兩組多載。
//     為什麼會錯：測試時用的都是非 const 物件，所以一路都沒問題；
//         直到某天有人把它放進 const 參考參數（例如
//         void print(const MyRange& r)）才爆出來。
//         寫自訂容器時請養成習慣：begin/end 一律先寫 const 版本。
// ═══════════════════════════════════════════════════════════════════════════

///*
//# 第四課：迭代器（Iterator）的核心概念
//
//---
//
//## 4.1 什麼是迭代器？
//
//**迭代器**（Iterator）是 STL 中最核心的概念之一。簡單來說：
//
//> **迭代器是一種「泛化的指標」，提供統一的方式來遍歷任何容器中的元素。**
//
//### 從指標說起
//
//你在 C 語言中已經很熟悉指標了：
//
//```cpp
//#include <iostream>
//
//int main() {
//    int arr[] = {10, 20, 30, 40, 50};
//    
//    // 用指標遍歷陣列
//    for (int* ptr = arr; ptr < arr + 5; ++ptr) {
//        std::cout << *ptr << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//10 20 30 40 50 
//```
//
//指標的操作：
//- `*ptr`：取得指向的值（解參考）
//- `++ptr`：移動到下一個元素
//- `ptr < end`：比較位置
//
//**迭代器就是把這些操作「抽象化」，讓它能用在任何容器上。**
//
//---
//
//## 4.2 為什麼需要迭代器？
//
//### 問題：不同容器，不同遍歷方式
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                    沒有迭代器的世界                              │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   陣列：                                                        │
//│   for (int i = 0; i < size; ++i)                               │
//│       process(arr[i]);                                          │
//│                                                                 │
//│   鏈結串列：                                                    │
//│   for (Node* p = head; p != nullptr; p = p->next)              │
//│       process(p->data);                                         │
//│                                                                 │
//│   二元樹：                                                      │
//│   void traverse(Node* n) {                                      │
//│       if (n == nullptr) return;                                │
//│       traverse(n->left);                                        │
//│       process(n->data);                                         │
//│       traverse(n->right);                                       │
//│   }                                                             │
//│                                                                 │
//│   問題：每種資料結構都要寫不同的遍歷邏輯！                      │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//### 解決方案：迭代器統一介面
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                    有迭代器的世界                                │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   不管什麼容器，遍歷方式都一樣：                                │
//│                                                                 │
//│   for (auto it = container.begin(); it != container.end(); ++it)│
//│       process(*it);                                             │
//│                                                                 │
//│   或更簡潔的：                                                  │
//│                                                                 │
//│   for (auto& element : container)                               │
//│       process(element);                                         │
//│                                                                 │
//│   底層的複雜性被迭代器封裝了！                                  │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//---
//
//## 4.3 迭代器的基本操作
//
//每個迭代器都支援以下基本操作：
//
//| 操作 | 說明 | 類似的指標操作 |
//|------|------|----------------|
//| `*it` | 取得迭代器指向的元素 | `*ptr` |
//| `++it` | 移動到下一個元素 | `++ptr` |
//| `it++` | 移動到下一個元素（後置） | `ptr++` |
//| `it1 == it2` | 比較兩個迭代器是否相等 | `ptr1 == ptr2` |
//| `it1 != it2` | 比較兩個迭代器是否不相等 | `ptr1 != ptr2` |
//
//### 基本操作示範
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    std::cout << "=== 迭代器基本操作 ===" << std::endl;
//    
//    // 取得起始迭代器
//    std::vector<int>::iterator it = vec.begin();
//    
//    // *it：解參考
//    std::cout << "*it = " << *it << std::endl;
//    
//    // ++it：前進
//    ++it;
//    std::cout << "++it 後，*it = " << *it << std::endl;
//    
//    // it++：後置前進
//    std::cout << "*it++ = " << *it++ << std::endl;  // 先取值，再前進
//    std::cout << "現在 *it = " << *it << std::endl;
//    
//    // 比較
//    std::cout << "\n=== 迭代器比較 ===" << std::endl;
//    auto begin = vec.begin();
//    auto end = vec.end();
//    
//    std::cout << "begin == end? " << (begin == end ? "是" : "否") << std::endl;
//    std::cout << "begin != end? " << (begin != end ? "是" : "否") << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== 迭代器基本操作 ===
//*it = 10
//++it 後，*it = 20
//*it++ = 20
//現在 *it = 30
//
//=== 迭代器比較 ===
//begin == end? 否
//begin != end? 是
//```
//
//---
//
//## 4.4 begin() 與 end()
//
//每個 STL 容器都提供 `begin()` 和 `end()` 成員函數：
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                    begin() 與 end() 圖解                        │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   容器內容：  [ 10 | 20 | 30 | 40 | 50 ]                        │
//│                 ↑                         ↑                     │
//│              begin()                    end()                   │
//│                                                                 │
//│   重點：                                                        │
//│   • begin() 指向第一個元素                                      │
//│   • end() 指向「最後一個元素的下一個位置」（past-the-end）      │
//│   • end() 不指向任何有效元素，不能解參考                        │
//│                                                                 │
//│   這是「半開區間」[begin, end)                                  │
//│   • 包含 begin                                                  │
//│   • 不包含 end                                                  │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//### 為什麼用半開區間？
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    // 半開區間的好處：
//    
//    // 1. 空容器的判斷很簡單
//    std::vector<int> empty_vec;
//    if (empty_vec.begin() == empty_vec.end()) {
//        std::cout << "空容器：begin() == end()" << std::endl;
//    }
//    
//    // 2. 遍歷邏輯統一
//    std::cout << "\n遍歷非空容器: ";
//    for (auto it = vec.begin(); it != vec.end(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    // 3. 計算元素個數
//    std::cout << "\n元素個數: " << (vec.end() - vec.begin()) << std::endl;
//    
//    // 4. 子區間表示
//    auto start = vec.begin() + 1;  // 指向 20
//    auto finish = vec.begin() + 4;  // 指向 50（不包含）
//    
//    std::cout << "子區間 [1, 4): ";
//    for (auto it = start; it != finish; ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//空容器：begin() == end()
//
//遍歷非空容器: 10 20 30 40 50 
//
//元素個數: 5
//子區間 [1, 4): 20 30 40 
//```
//
//---
//
//## 4.5 使用 auto 簡化迭代器宣告
//
//迭代器的完整型別很長，使用 `auto` 可以大幅簡化：
//
//```cpp
//#include <iostream>
//#include <vector>
//#include <list>
//#include <map>
//#include <string>
//
//int main() {
//    // 完整型別宣告（冗長）
//    std::vector<int> vec = {1, 2, 3};
//    std::vector<int>::iterator it1 = vec.begin();
//    
//    // 使用 auto（簡潔）
//    auto it2 = vec.begin();
//    
//    std::cout << "兩者等價：*it1 = " << *it1 << ", *it2 = " << *it2 << std::endl;
//    
//    // 對於複雜型別，auto 更加重要
//    std::map<std::string, int> scores;
//    scores["Alice"] = 95;
//    scores["Bob"] = 87;
//    
//    // 不用 auto，型別非常長
//    std::map<std::string, int>::iterator map_it1 = scores.begin();
//    
//    // 用 auto，清爽多了
//    auto map_it2 = scores.begin();
//    
//    std::cout << "map 第一個元素：" << map_it2->first 
//              << " = " << map_it2->second << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//兩者等價：*it1 = 1, *it2 = 1
//map 第一個元素：Alice = 95
//```
//
//---
//
//## 4.6 const_iterator：唯讀迭代器
//
//當你只想讀取元素，不想修改時，使用 `const_iterator`：
//
//```cpp
//#include <iostream>
//#include <vector>
//
//void print_vector(const std::vector<int>& vec) {
//    // const 容器只能用 const_iterator
//    // 使用 cbegin() 和 cend() 明確取得 const_iterator
//    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
//        std::cout << *it << " ";
//        // *it = 100;  // 編譯錯誤！不能透過 const_iterator 修改
//    }
//    std::cout << std::endl;
//}
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    // 一般迭代器：可讀可寫
//    std::cout << "=== 一般迭代器 ===" << std::endl;
//    for (auto it = vec.begin(); it != vec.end(); ++it) {
//        *it *= 2;  // 可以修改
//    }
//    print_vector(vec);
//    
//    // const_iterator：只能讀
//    std::cout << "\n=== const_iterator ===" << std::endl;
//    
//    // 方法一：從 const 容器取得
//    const std::vector<int>& const_ref = vec;
//    for (auto it = const_ref.begin(); it != const_ref.end(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    // 方法二：使用 cbegin() / cend()
//    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== 一般迭代器 ===
//20 40 60 80 100 
//
//=== const_iterator ===
//20 40 60 80 100 
//20 40 60 80 100 
//```
//
//### 迭代器型別總覽
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                    迭代器型別總覽                                │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   成員函數              回傳型別              用途              │
//│   ─────────────────────────────────────────────────────────     │
//│   begin()              iterator              可讀寫，正向      │
//│   end()                iterator                                 │
//│                                                                 │
//│   cbegin()             const_iterator        唯讀，正向        │
//│   cend()               const_iterator                           │
//│                                                                 │
//│   rbegin()             reverse_iterator      可讀寫，反向      │
//│   rend()               reverse_iterator                         │
//│                                                                 │
//│   crbegin()            const_reverse_iterator 唯讀，反向       │
//│   crend()              const_reverse_iterator                   │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//---
//
//## 4.7 reverse_iterator：反向迭代器
//
//反向迭代器讓你從後往前遍歷容器：
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    // 正向遍歷
//    std::cout << "正向: ";
//    for (auto it = vec.begin(); it != vec.end(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    // 反向遍歷
//    std::cout << "反向: ";
//    for (auto rit = vec.rbegin(); rit != vec.rend(); ++rit) {
//        std::cout << *rit << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//正向: 10 20 30 40 50 
//反向: 50 40 30 20 10 
//```
//
//### 反向迭代器圖解
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                   反向迭代器圖解                                 │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   容器內容：  [ 10 | 20 | 30 | 40 | 50 ]                        │
//│                                                                 │
//│   正向：       ↑                         ↑                      │
//│             begin()                    end()                    │
//│             (指向 10)              (past-the-end)               │
//│                                                                 │
//│   反向：                           ↑                         ↑  │
//│                                 rbegin()                  rend()│
//│                                (指向 50)              (past-the-│
//│                                                        beginning)│
//│                                                                 │
//│   rbegin() 的 ++ 會往左移動（邏輯上的「下一個」是前一個元素）    │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//---
//
//## 4.8 迭代器與演算法
//
//迭代器的真正威力在於它讓演算法與容器解耦：
//
//```cpp
//#include <iostream>
//#include <vector>
//#include <list>
//#include <algorithm>
//
//int main() {
//    std::vector<int> vec = {5, 2, 8, 1, 9};
//    std::list<int> lst = {5, 2, 8, 1, 9};
//    
//    // 同一個 find 演算法，用在不同容器
//    std::cout << "=== std::find ===" << std::endl;
//    
//    auto vec_it = std::find(vec.begin(), vec.end(), 8);
//    if (vec_it != vec.end()) {
//        std::cout << "在 vector 中找到 8" << std::endl;
//    }
//    
//    auto lst_it = std::find(lst.begin(), lst.end(), 8);
//    if (lst_it != lst.end()) {
//        std::cout << "在 list 中找到 8" << std::endl;
//    }
//    
//    // 同一個 count 演算法
//    std::cout << "\n=== std::count ===" << std::endl;
//    
//    std::vector<int> data = {1, 2, 2, 3, 2, 4, 2};
//    int count = std::count(data.begin(), data.end(), 2);
//    std::cout << "2 出現了 " << count << " 次" << std::endl;
//    
//    // 在子區間上操作
//    std::cout << "\n=== 子區間操作 ===" << std::endl;
//    
//    std::vector<int> nums = {10, 20, 30, 40, 50, 60, 70};
//    
//    // 只在 [begin+1, begin+5) 範圍內查找，即 {20, 30, 40, 50}
//    auto sub_it = std::find(nums.begin() + 1, nums.begin() + 5, 40);
//    if (sub_it != nums.begin() + 5) {
//        std::cout << "在子區間中找到 40" << std::endl;
//    }
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== std::find ===
//在 vector 中找到 8
//在 list 中找到 8
//
//=== std::count ===
//2 出現了 4 次
//
//=== 子區間操作 ===
//在子區間中找到 40
//```
//
//---
//
//## 4.9 迭代器失效問題
//
//這是使用迭代器時最容易出錯的地方：**當容器被修改時，迭代器可能失效**。
//
//### 問題示範
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {1, 2, 3, 4, 5};
//    
//    // 危險！在遍歷時修改容器
//    /*
//    for (auto it = vec.begin(); it != vec.end(); ++it) {
//        if (*it == 3) {
//            vec.erase(it);  // 刪除後，it 失效！
//            // 繼續使用 it 是未定義行為
//        }
//    }
//    */
//    
//    // 正確做法：使用 erase 的回傳值
//    std::cout << "原始: ";
//    for (int n : vec) std::cout << n << " ";
//    std::cout << std::endl;
//    
//    for (auto it = vec.begin(); it != vec.end(); /* 不在這裡 ++ */) {
//        if (*it == 3) {
//            it = vec.erase(it);  // erase 回傳下一個有效迭代器
//        } else {
//            ++it;
//        }
//    }
//    
//    std::cout << "刪除 3 後: ";
//    for (int n : vec) std::cout << n << " ";
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//原始: 1 2 3 4 5 
//刪除 3 後: 1 2 4 5 
//```
//
//### 迭代器失效規則
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                    迭代器失效規則                                │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│  vector：                                                       │
//│  ├── 插入：可能導致所有迭代器失效（若發生重新配置）             │
//│  ├── 刪除：被刪元素之後的所有迭代器失效                         │
//│  └── push_back：可能導致所有迭代器失效（若發生重新配置）        │
//│                                                                 │
//│  deque：                                                        │
//│  ├── 頭尾插入：所有迭代器失效（但指標和參考不失效）             │
//│  └── 中間插入/刪除：所有迭代器失效                              │
//│                                                                 │
//│  list / forward_list：                                          │
//│  ├── 插入：不影響現有迭代器                                     │
//│  └── 刪除：只有被刪元素的迭代器失效                             │
//│                                                                 │
//│  set / map（樹狀結構）：                                        │
//│  ├── 插入：不影響現有迭代器                                     │
//│  └── 刪除：只有被刪元素的迭代器失效                             │
//│                                                                 │
//│  unordered_set / unordered_map：                                │
//│  ├── 插入：可能導致所有迭代器失效（若發生 rehash）              │
//│  └── 刪除：只有被刪元素的迭代器失效                             │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//### 安全的刪除模式
//
//```cpp
//#include <iostream>
//#include <vector>
//#include <list>
//#include <algorithm>
//
//int main() {
//    // 方法一：使用 erase 的回傳值（適用於所有容器）
//    std::cout << "=== 方法一：erase 回傳值 ===" << std::endl;
//    std::vector<int> vec1 = {1, 2, 3, 2, 4, 2, 5};
//    
//    for (auto it = vec1.begin(); it != vec1.end(); ) {
//        if (*it == 2) {
//            it = vec1.erase(it);
//        } else {
//            ++it;
//        }
//    }
//    
//    for (int n : vec1) std::cout << n << " ";
//    std::cout << std::endl;
//    
//    // 方法二：使用 erase-remove 慣用法（適用於 vector）
//    std::cout << "\n=== 方法二：erase-remove 慣用法 ===" << std::endl;
//    std::vector<int> vec2 = {1, 2, 3, 2, 4, 2, 5};
//    
//    vec2.erase(
//        std::remove(vec2.begin(), vec2.end(), 2),
//        vec2.end()
//    );
//    
//    for (int n : vec2) std::cout << n << " ";
//    std::cout << std::endl;
//    
//    // 方法三：對於 list，使用成員函數 remove（更高效）
//    std::cout << "\n=== 方法三：list::remove ===" << std::endl;
//    std::list<int> lst = {1, 2, 3, 2, 4, 2, 5};
//    
//    lst.remove(2);  // list 有自己的 remove 成員函數
//    
//    for (int n : lst) std::cout << n << " ";
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== 方法一：erase 回傳值 ===
//1 3 4 5 
//
//=== 方法二：erase-remove 慣用法 ===
//1 3 4 5 
//
//=== 方法三：list::remove ===
//1 3 4 5 
//```
//
//---
//
//## 4.10 迭代器與指標的關係
//
//迭代器被設計成「像指標一樣操作」，但它是更抽象的概念：
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    // 取得底層指標
//    int* ptr = vec.data();  // C++11
//    // 或 int* ptr = &vec[0];
//    
//    // 取得迭代器
//    auto it = vec.begin();
//    
//    std::cout << "=== 相似的操作 ===" << std::endl;
//    std::cout << "指標解參考: *ptr = " << *ptr << std::endl;
//    std::cout << "迭代器解參考: *it = " << *it << std::endl;
//    
//    std::cout << "\n指標算術: *(ptr + 2) = " << *(ptr + 2) << std::endl;
//    std::cout << "迭代器算術: *(it + 2) = " << *(it + 2) << std::endl;
//    
//    // 迭代器可以轉換成指標（對於連續記憶體的容器）
//    std::cout << "\n=== 迭代器轉指標 ===" << std::endl;
//    int* from_iterator = &(*it);  // 取得迭代器指向的元素的位址
//    std::cout << "*from_iterator = " << *from_iterator << std::endl;
//    
//    // 但指標不能直接當迭代器用在演算法中（需要配合 size）
//    // 好消息是，原始指標本身就是一種迭代器！
//    std::cout << "\n=== 指標作為迭代器 ===" << std::endl;
//    int arr[] = {100, 200, 300};
//    
//    // 指標可以直接用在 STL 演算法
//    auto found = std::find(arr, arr + 3, 200);
//    if (found != arr + 3) {
//        std::cout << "在陣列中找到 200" << std::endl;
//    }
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== 相似的操作 ===
//指標解參考: *ptr = 10
//迭代器解參考: *it = 10
//
//指標算術: *(ptr + 2) = 30
//迭代器算術: *(it + 2) = 30
//
//=== 迭代器轉指標 ===
//*from_iterator = 10
//
//=== 指標作為迭代器 ===
//在陣列中找到 200
//```
//
//### 指標就是 Random Access Iterator
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                指標與迭代器的關係                                │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│   原始指標（T*）滿足 Random Access Iterator 的所有要求：        │
//│                                                                 │
//│   ✓ *ptr         解參考                                        │
//│   ✓ ++ptr        前進                                          │
//│   ✓ --ptr        後退                                          │
//│   ✓ ptr + n      隨機存取                                      │
//│   ✓ ptr - n      隨機存取                                      │
//│   ✓ ptr[n]       下標存取                                      │
//│   ✓ ptr1 - ptr2  計算距離                                      │
//│   ✓ ptr1 < ptr2  比較                                          │
//│                                                                 │
//│   所以 STL 演算法可以直接用在 C 風格陣列上！                    │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//---
//
//## 4.11 範圍 for 與迭代器
//
//C++11 的範圍 for 迴圈底層就是使用迭代器：
//
//```cpp
//#include <iostream>
//#include <vector>
//
//int main() {
//    std::vector<int> vec = {10, 20, 30, 40, 50};
//    
//    // 範圍 for（語法糖）
//    std::cout << "範圍 for: ";
//    for (int n : vec) {
//        std::cout << n << " ";
//    }
//    std::cout << std::endl;
//    
//    // 等價的迭代器寫法
//    std::cout << "迭代器: ";
//    for (auto it = vec.begin(); it != vec.end(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    // 編譯器實際上把範圍 for 轉換成類似這樣：
//    /*
//    {
//        auto&& __range = vec;
//        auto __begin = __range.begin();
//        auto __end = __range.end();
//        for (; __begin != __end; ++__begin) {
//            int n = *__begin;
//            // 迴圈主體
//        }
//    }
//    */
//    
//    // 如果要修改元素，使用參考
//    std::cout << "\n修改元素（使用參考）: ";
//    for (int& n : vec) {
//        n *= 2;
//    }
//    for (int n : vec) {
//        std::cout << n << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//範圍 for: 10 20 30 40 50 
//迭代器: 10 20 30 40 50 
//
//修改元素（使用參考）: 20 40 60 80 100 
//```
//
//---
//
//## 4.12 完整範例：自訂類別使用迭代器
//
//讓我們看看如何讓自訂類別支援迭代器和範圍 for：
//
//```cpp
//#include <iostream>
//#include <vector>
//#include <algorithm>
//
//class IntRange {
//private:
//    int start_;
//    int end_;
//    
//public:
//    // 內部迭代器類別
//    class Iterator {
//    private:
//        int current_;
//        
//    public:
//        Iterator(int value) : current_(value) {}
//        
//        int operator*() const { return current_; }
//        
//        Iterator& operator++() {
//            ++current_;
//            return *this;
//        }
//        
//        bool operator!=(const Iterator& other) const {
//            return current_ != other.current_;
//        }
//    };
//    
//    IntRange(int start, int end) : start_(start), end_(end) {}
//    
//    Iterator begin() const { return Iterator(start_); }
//    Iterator end() const { return Iterator(end_); }
//};
//
//int main() {
//    std::cout << "=== 自訂 IntRange 類別 ===" << std::endl;
//    
//    IntRange range(1, 6);  // 代表 1, 2, 3, 4, 5
//    
//    // 可以使用範圍 for！
//    std::cout << "範圍 for: ";
//    for (int n : range) {
//        std::cout << n << " ";
//    }
//    std::cout << std::endl;
//    
//    // 也可以使用迭代器
//    std::cout << "迭代器: ";
//    for (auto it = range.begin(); it != range.end(); ++it) {
//        std::cout << *it << " ";
//    }
//    std::cout << std::endl;
//    
//    return 0;
//}
//```
//
//**輸出：**
//```
//=== 自訂 IntRange 類別 ===
//範圍 for: 1 2 3 4 5 
//迭代器: 1 2 3 4 5 
//```
//
//---
//
//## 4.13 本課重點整理
//
//```
//┌─────────────────────────────────────────────────────────────────┐
//│                      第四課 重點整理                             │
//├─────────────────────────────────────────────────────────────────┤
//│                                                                 │
//│  1. 迭代器是什麼                                                 │
//│     → 泛化的指標，提供統一的容器遍歷方式                        │
//│     → 連接容器與演算法的橋樑                                    │
//│                                                                 │
//│  2. 基本操作                                                     │
//│     → *it（解參考）、++it（前進）、it1 != it2（比較）           │
//│                                                                 │
//│  3. begin() 與 end()                                            │
//│     → 半開區間 [begin, end)                                     │
//│     → end() 指向「最後元素的下一個位置」                        │
//│                                                                 │
//│  4. 迭代器種類                                                   │
//│     → iterator：可讀寫                                          │
//│     → const_iterator：唯讀（cbegin/cend）                       │
//│     → reverse_iterator：反向（rbegin/rend）                     │
//│                                                                 │
//│  5. 迭代器失效                                                   │
//│     → 容器修改時，迭代器可能失效                                │
//│     → 使用 erase 的回傳值來安全刪除                             │
//│     → 不同容器有不同的失效規則                                  │
//│                                                                 │
//│  6. 迭代器與指標                                                 │
//│     → 指標是一種 Random Access Iterator                         │
//│     → STL 演算法可以直接用在 C 風格陣列                         │
//│                                                                 │
//│  7. 範圍 for                                                    │
//│     → 語法糖，底層使用迭代器                                    │
//│     → 需要 begin() 和 end()                                     │
//│                                                                 │
//└─────────────────────────────────────────────────────────────────┘
//```
//
//---
//
//## 4.14 課後練習
//
//1. **思考題**：為什麼 `end()` 不指向最後一個元素，而是指向「最後一個元素的下一個位置」？這種設計有什麼好處？
//
//2. **實作題**：寫一個函數，接受一個 vector，使用迭代器將所有奇數刪除（注意迭代器失效問題）。
//
//提示：
//```cpp
//void remove_odds(std::vector<int>& vec) {
//    // 你的實作
//}
//```
//
//---
//
//準備好進入**第五課：迭代器的五種分類**了嗎？下一課我們會深入探討 Input、Output、Forward、Bidirectional、Random Access 這五種迭代器的能力差異。
//*/
//



#include <iostream>
#include <vector>
#include <string>
#include <iterator>
#include <algorithm>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】不強加。
//   理由：本檔的主題是「自己寫一個迭代器讓類別支援範圍 for」。
//         LeetCode 一律直接給你 vector/string 作為輸入，
//         沒有任何一題會要求你實作自訂迭代器 —— 真那樣寫還會被視為過度設計。
//         這個技能的價值在函式庫與框架設計（讓自己的型別融入 STL 生態），
//         正是下面兩個實務範例展示的場景。
//         同一課的 summary.cpp 已用 LeetCode 118. Pascal's Triangle
//         示範「用迭代器解題」，此處不重複。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例 1】讓自家的環形緩衝區（ring buffer）支援範圍 for
//   情境：嵌入式／低延遲系統常用固定大小的環形緩衝區保留「最近 N 筆」資料
//         （感測器讀數、log、封包）。寫滿後會繞回頭覆蓋最舊的那筆。
//   為什麼用到本主題：使用者當然希望能直接寫 for (int v : buffer)，
//         但「從最舊的一筆開始、走到陣列尾端要繞回開頭」這個順序
//         沒辦法用普通指標表達 —— 必須自訂迭代器把繞行邏輯封裝進 operator++。
//         這正是自訂迭代器最有價值的用途：**把複雜的走訪順序藏起來**，
//         讓呼叫端用起來就像在走訪一般容器。
// -----------------------------------------------------------------------------
class RingBuffer {
    static const std::size_t CAP = 5;
    int         data_[CAP] = {};
    std::size_t head_  = 0;    // 最舊元素的索引
    std::size_t count_ = 0;    // 目前有幾筆

public:
    void push(int v) {
        if (count_ < CAP) {
            data_[(head_ + count_) % CAP] = v;
            ++count_;
        } else {
            data_[head_] = v;              // 覆蓋最舊的那筆
            head_ = (head_ + 1) % CAP;     // 頭往前移一格
        }
    }

    std::size_t size() const { return count_; }

    // 自訂迭代器：把「繞回頭」的取模運算封裝在裡面
    class Iterator {
        const RingBuffer* buf_;
        std::size_t       step_;   // 已走了幾步（0 .. count_）
    public:
        Iterator(const RingBuffer* b, std::size_t s) : buf_(b), step_(s) {}

        int operator*() const {
            return buf_->data_[(buf_->head_ + step_) % CAP];   // 繞行邏輯在這
        }
        Iterator& operator++() { ++step_; return *this; }
        bool operator!=(const Iterator& o) const { return step_ != o.step_; }
    };

    // 注意：兩者都宣告成 const 成員，const 物件才能被範圍 for 走訪
    Iterator begin() const { return Iterator(this, 0); }
    Iterator end()   const { return Iterator(this, count_); }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】惰性序列 vs 先建容器：記憶體差異
//   情境：對「1 到 N」做統計時，常見的寫法是先建一個裝滿數字的 vector 再走訪。
//         N 很大時等於白白配置 N * sizeof(int) 的記憶體。
//   為什麼用到本主題：IntRange 這種惰性序列只存兩個整數，
//         元素在走訪時才計算 —— 完全不配置任何堆積記憶體。
//         這正是 Python range() 與 C++20 std::views::iota 背後的原理。
// -----------------------------------------------------------------------------
long long sumRangeEager(int n) {          // 先建容器：需要 n * 4 bytes
    std::vector<int> tmp;
    tmp.reserve(static_cast<std::size_t>(n));
    for (int i = 1; i <= n; ++i) tmp.push_back(i);

    long long total = 0;
    for (int v : tmp) total += v;
    return total;
}

class IntRange {
private:
    int start_;
    int end_;
    
public:
    // 內部迭代器類別
    class Iterator {
    private:
        int current_;
        
    public:
        Iterator(int value) : current_(value) {}
        
        int operator*() const { return current_; }
        
        Iterator& operator++() {
            ++current_;
            return *this;
        }
        
        bool operator!=(const Iterator& other) const {
            return current_ != other.current_;
        }
    };
    
    IntRange(int start, int end) : start_(start), end_(end) {}
    
    Iterator begin() const { return Iterator(start_); }
    Iterator end() const { return Iterator(end_); }
};

int main() {
    std::cout << "=== 自訂 IntRange 類別 ===" << std::endl;
    
    IntRange range(1, 6);  // 代表 1, 2, 3, 4, 5
    
    // 可以使用範圍 for！
    std::cout << "範圍 for: ";
    for (int n : range) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    // 也可以使用迭代器
    std::cout << "迭代器: ";
    for (auto it = range.begin(); it != range.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // begin/end 是 const 成員 → const 物件也能走訪
    std::cout << "\n=== begin()/end() 為什麼要是 const 成員 ===" << std::endl;
    const IntRange const_range(10, 15);      // 注意這裡是 const 物件
    std::cout << "const 物件的範圍 for: ";
    for (int n : const_range) std::cout << n << " ";
    std::cout << "  ← 因為 begin()/end() 宣告成 const 才編得過" << std::endl;

    // 惰性序列不佔記憶體
    std::cout << "\n=== 惰性序列 vs 先建容器 ===" << std::endl;
    const int N = 100000;
    std::cout << "  IntRange(1, " << N << ") 物件大小 = "
              << sizeof(IntRange) << " bytes（只存兩個 int）" << std::endl;
    std::cout << "  先建 vector<int> 需要配置   = "
              << (static_cast<long long>(N) * static_cast<long long>(sizeof(int)) / 1024)
              << " KB 的堆積記憶體" << std::endl;

    long long lazy_sum = 0;
    for (int n : IntRange(1, N + 1)) lazy_sum += n;      // 零配置
    long long eager_sum = sumRangeEager(N);              // 要配置 N*4 bytes

    std::cout << "  惰性走訪求和 1.." << N << " = " << lazy_sum << std::endl;
    std::cout << "  先建容器求和 1.." << N << " = " << eager_sum << std::endl;
    std::cout << "  兩者結果相同，但惰性版本沒有配置任何堆積記憶體" << std::endl;

    std::cout << "\n=== 日常實務：環形緩衝區支援範圍 for ===" << std::endl;
    RingBuffer rb;   // 容量 5
    std::cout << "  推入 1..3（未滿）: ";
    for (int v : {1, 2, 3}) rb.push(v);
    for (int v : rb) std::cout << v << " ";
    std::cout << " (size=" << rb.size() << ")" << std::endl;

    std::cout << "  推入 4..5（剛好滿）: ";
    for (int v : {4, 5}) rb.push(v);
    for (int v : rb) std::cout << v << " ";
    std::cout << " (size=" << rb.size() << ")" << std::endl;

    std::cout << "  再推入 6、7（開始覆蓋最舊的）: ";
    for (int v : {6, 7}) rb.push(v);
    for (int v : rb) std::cout << v << " ";
    std::cout << " (size=" << rb.size() << ")" << std::endl;
    std::cout << "  → 走訪順序永遠是「最舊到最新」，"
                 "繞行邏輯完全藏在 operator++ 裡面" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra 第四課：迭代器（Iterator）的核心概念12.cpp -o demo12

// === 預期輸出 ===
// === 自訂 IntRange 類別 ===
// 範圍 for: 1 2 3 4 5
// 迭代器: 1 2 3 4 5
//
// === begin()/end() 為什麼要是 const 成員 ===
// const 物件的範圍 for: 10 11 12 13 14   ← 因為 begin()/end() 宣告成 const 才編得過
//
// === 惰性序列 vs 先建容器 ===
//   IntRange(1, 100000) 物件大小 = 8 bytes（只存兩個 int）
//   先建 vector<int> 需要配置   = 390 KB 的堆積記憶體
//   惰性走訪求和 1..100000 = 5000050000
//   先建容器求和 1..100000 = 5000050000
//   兩者結果相同，但惰性版本沒有配置任何堆積記憶體
//
// === 日常實務：環形緩衝區支援範圍 for ===
//   推入 1..3（未滿）: 1 2 3  (size=3)
//   推入 4..5（剛好滿）: 1 2 3 4 5  (size=5)
//   再推入 6、7（開始覆蓋最舊的）: 3 4 5 6 7  (size=5)
//   → 走訪順序永遠是「最舊到最新」，繞行邏輯完全藏在 operator++ 裡面
