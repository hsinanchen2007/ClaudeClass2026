// =============================================================================
//  第 4 課 總複習  —  迭代器（Iterator）：STL 的中樞神經
// =============================================================================
//
// 【主題資訊 Information】
//   核心操作（Input Iterator 的最小集合）：
//     *it        解參考，取得元素
//     ++it       前進（前置版；後置版 it++ 要多複製一份）
//     it1 != it2 比較，用來判斷是否走到終點
//   四種變體（以 vector 為例）：
//     begin()/end()     iterator                （可讀可寫、正向）
//     cbegin()/cend()   const_iterator          （唯讀、正向）        C++11
//     rbegin()/rend()   reverse_iterator        （可讀可寫、反向）
//     crbegin()/crend() const_reverse_iterator  （唯讀、反向）        C++11
//   標準版本：迭代器概念自 C++98；auto 型別推導、範圍 for、cbegin/cend、
//             std::next/std::prev、vector::data() 皆為 C++11；
//             C++20 加入 ranges 與 contiguous_iterator_tag。
//   複雜度：核心操作全部 O(1)；走訪 N 個元素為 O(N)。
//   標頭檔：容器自帶 begin/end；<iterator> 提供 advance/distance/next/prev 與配接器。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要迭代器：把「怎麼走」與「走什麼」分開】
//   用索引 v[i] 走訪只對「連續記憶體 + 有 operator[]」的容器成立。
//   list 是鏈結串列、map 是紅黑樹，索引在它們身上毫無意義。
//   迭代器把「如何前進到下一個元素」封裝進型別本身：
//       vector::iterator 的 ++ → 指標 +1
//       list::iterator   的 ++ → node = node->next
//       map::iterator    的 ++ → 中序走訪紅黑樹找後繼節點
//   對呼叫端而言介面完全相同（*、++、!=），因此同一個 std::find
//   能服務所有容器。這是整個 STL 得以成立的支點。
//
// 【2. 半開區間 [begin, end) 為什麼是對的設計】
//   end() 指向「最後一個元素之後」的位置，不是最後一個元素。好處有四：
//     (a) 空容器天然表示為 begin() == end()，不必特判
//     (b) 迴圈條件統一寫 it != end()（list 迭代器根本沒有 <）
//     (c) 元素個數 = end() - begin()（Random Access）
//     (d) 子區間可以自然巢狀組合：[b+1, b+4) 本身又是一個合法範圍
//   代價：*end() 是未定義行為，絕不能解參考。
//   附帶一提，若改成閉區間 [begin, last]，空容器就必須讓 last 指向
//   begin 的前一個位置 —— 而那在 C++ 中不是合法的指標值。
//
// 【3. 迭代器失效：這是選容器的決定性因素】
//     vector  push_back 觸發重新配置 → **全部**失效；未觸發則只有 end() 失效
//             erase → 被刪位置及其後全部失效
//     deque   兩端插入 → 迭代器失效但**參考與指標仍有效**（很特別）
//     list    插入 → 完全不失效；erase → 只有被刪的失效
//     set/map 規則同 list（節點式容器，元素不搬動）
//   所以「需要長期持有元素位置」的設計（LRU cache、書籤、觀察者註冊）
//   幾乎一定選 list 或 map，而不是 vector。
//
// 【4. 安全刪除的三種模式】
//     通用      for (auto it = c.begin(); it != c.end(); )
//                   if (cond) it = c.erase(it); else ++it;
//     vector    c.erase(std::remove(c.begin(), c.end(), v), c.end());   // O(N)
//     list      lst.remove(v) / lst.remove_if(pred);                    // 不搬移元素
//     C++20     std::erase(c, v) / std::erase_if(c, pred);              // 一行搞定
//   關鍵原則：**刪了就不要再 ++**，讓 erase 的回傳值推進迴圈。
//
// 【5. 自訂類別支援範圍 for 的最低門檻】
//   只要提供 begin() 與 end()（成員或 ADL 找得到的自由函式），
//   且回傳的物件支援 operator*、operator++（前置）、operator!= 即可。
//   不需要繼承任何基底類別 —— 這正是 STL 用 concept 而非繼承的體現。
//   若還想讓 STL 演算法能用，才需要補上 iterator_traits 所需的五個 typedef
//   （value_type、difference_type、pointer、reference、iterator_category）。
//
// 【概念補充 Concept Deep Dive】
//   const_iterator 與 「const iterator」是完全不同的兩件事，這常被搞混：
//       std::vector<int>::const_iterator it;   // 指向 const 元素的迭代器
//                                              // → it 可以 ++，但 *it 不能改
//       const std::vector<int>::iterator it2;  // 迭代器本身是 const
//                                              // → *it2 可以改，但 it2 不能 ++
//   類比指標就很清楚：前者是 const int*，後者是 int* const。
//   實務上你幾乎永遠要的是前者。
//   C++11 加入 cbegin()/cend() 就是為了讓「我要唯讀」這個意圖能被明確寫出來 ——
//   在此之前只能靠「把容器綁到 const 參考」間接取得 const_iterator，
//   而 auto it = v.begin() 在非 const 容器上永遠推導出可寫的 iterator。
//
// 【注意事項 Pay Attention】
//   1. *end() 是未定義行為，任何情況下都不可解參考。
//   2. erase 之後原迭代器立即失效，必須改用它的回傳值。
//   3. 範圍 for 不比較安全 —— 它只是把迭代器藏起來，容器一改照樣失效。
//   4. for (auto x : v) 每輪複製元素；唯讀請一律寫 const auto&。
//   5. auto it = v.begin() 推導出的是 iterator 而非 const_iterator；
//      要唯讀請明確寫 cbegin()。
//   6. std::distance 對 Random Access 是 O(1)，對 list 是 O(N) ——
//      在迴圈裡呼叫它是常見的意外 O(N²)。
//   7. 空 vector 不可寫 &v[0]（UB）；請改用 v.data()。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】迭代器核心概念總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 STL 的區間是半開的 [begin, end)？閉區間有什麼問題？
//     答：半開區間讓空容器天然表示為 begin() == end()、迴圈條件統一為
//         it != end()、元素個數等於 end() - begin()，且子區間可自然巢狀。
//         若用閉區間 [begin, last]，空容器就得讓 last 指向 begin 的前一個位置，
//         而「第一個元素之前的位址」在 C++ 中不是合法指標值。
//     追問：那 rend() 呢？它不就指向 begin() 的前面嗎？
//           → 不是。reverse_iterator 內部存的是正向迭代器 base()，
//             解參考時回傳 *(base()-1)。rend().base() 正好等於 begin()，
//             所以它從未真的指向 begin() 之前 —— 這個「差一位」設計
//             正是為了避開該未定義行為。
//
// 🔥 Q2. const_iterator 和 const iterator 差在哪？
//     答：完全不同。const_iterator 是「指向 const 元素的迭代器」——
//         it 可以 ++，但 *it 不能改，類比 const int*。
//         const iterator 是「迭代器本身是 const」—— *it 可以改，
//         但 it 不能 ++，類比 int* const。實務上你要的幾乎永遠是前者。
//     追問：那為什麼 C++11 要加 cbegin()/cend()？
//           → 因為 auto it = v.begin() 在非 const 容器上永遠推導出可寫的 iterator。
//             在 cbegin() 出現之前，想在非 const 容器上取得 const_iterator
//             只能寫出完整型別名或繞道 const 參考，非常囉嗦。
//
// ⚠️ 陷阱. 「我改用範圍 for 就不會有迭代器失效問題了」——為什麼這是錯的？
//     答：範圍 for 只是語法糖，展開後仍然是 __begin / __end 兩個迭代器，
//         而且它們在迴圈**開始前**就取好了。
//         迴圈內 push_back 觸發重新配置，兩者立刻失效 → 未定義行為；
//         迴圈內 erase 同樣會讓 __end 錯位。
//         範圍 for 不但沒有比較安全，反而因為看不到迭代器而更容易忘記這件事。
//     為什麼會錯：把「看不到指標/迭代器」誤當成「沒有指標/迭代器」。
//         這與「用了智慧指標就不會有懸空問題」是同一類誤解 ——
//         抽象隱藏了細節，但沒有消除底層約束。
//         要邊走邊刪，就得用明確的迭代器迴圈搭配 erase 的回傳值。
// ═══════════════════════════════════════════════════════════════════════════

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

// ===== LeetCode 實戰 =====
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 118. Pascal's Triangle
//   題目：給定列數 numRows，回傳巴斯卡三角形的前 numRows 列。
//         每個內部元素等於上一列相鄰兩數之和。
//   為什麼用到本主題：這題需要「同時走訪上一列的相鄰兩個元素」，
//         正是迭代器（而非索引）最能表達意圖的場合：
//         prev.begin() 與 std::next(prev.begin()) 兩個游標並行前進。
//         同時也用到 back_inserter 與 const_iterator，
//         把本課的幾個重點串在一起。
//   複雜度：時間 O(numRows²)、空間 O(numRows²)（即輸出本身）。
// -----------------------------------------------------------------------------
std::vector<std::vector<int>> generatePascalTriangle(int numRows) {
    std::vector<std::vector<int>> tri;
    if (numRows <= 0) return tri;

    tri.push_back({1});                       // 第 1 列
    for (int r = 1; r < numRows; ++r) {
        const std::vector<int>& prev = tri.back();
        std::vector<int> row;
        row.reserve(static_cast<std::size_t>(r) + 1);

        row.push_back(1);                     // 每列首元素恆為 1
        // 用兩個並行的 const_iterator 取「相鄰兩數」——不需要任何索引運算
        for (auto it = prev.cbegin(); std::next(it) != prev.cend(); ++it) {
            row.push_back(*it + *std::next(it));
        }
        row.push_back(1);                     // 每列末元素恆為 1

        tri.push_back(std::move(row));
    }
    return tri;
}

void demo_leetcode_118() {
    std::cout << "\n===== LeetCode 118. Pascal's Triangle =====" << std::endl;
    for (const auto& row : generatePascalTriangle(5)) {
        std::cout << "  ";
        for (int n : row) std::cout << n << " ";
        std::cout << std::endl;
    }
    std::cout << "  numRows=0 時回傳空: "
              << generatePascalTriangle(0).size() << " 列" << std::endl;
}

// ===== 日常實務 =====
// -----------------------------------------------------------------------------
// 【日常實務範例】感測器資料清洗：找出跳變點並剔除離群值
//   情境：IoT 感測器每秒回報一次讀數，偶爾會因為干擾產生離群值。
//         需求是 (1) 找出「與前一筆差距超過門檻」的跳變點供告警，
//         (2) 把離群值從序列中剔除。
//   為什麼用到本主題：
//     - 「與前一筆比較」= 兩個並行迭代器（it 與 std::prev(it)），
//       這是 std::adjacent_find 的手寫版本，索引寫法反而不自然
//     - 剔除離群值 = erase-remove 慣用法，正是本課「安全刪除」的重點
//     - 全程用 const_iterator 唯讀掃描，不會意外改到資料
// -----------------------------------------------------------------------------
struct Reading {
    int  timestamp;
    double celsius;
};

// (1) 找出跳變點：回傳每個跳變的 timestamp
std::vector<int> findSpikes(const std::vector<Reading>& data, double threshold) {
    std::vector<int> spikes;
    if (data.size() < 2) return spikes;

    // 從第二筆開始，與前一筆比較（兩個並行游標）
    for (auto it = std::next(data.cbegin()); it != data.cend(); ++it) {
        double delta = it->celsius - std::prev(it)->celsius;
        if (delta < 0) delta = -delta;
        if (delta > threshold) spikes.push_back(it->timestamp);
    }
    return spikes;
}

// (2) 剔除超出合理範圍的離群值（erase-remove 慣用法）
int dropOutOfRange(std::vector<Reading>& data, double lo, double hi) {
    std::size_t before = data.size();
    data.erase(std::remove_if(data.begin(), data.end(),
                              [lo, hi](const Reading& r) {
                                  return r.celsius < lo || r.celsius > hi;
                              }),
               data.end());
    return static_cast<int>(before - data.size());
}

void demo_practical_sensor() {
    std::cout << "\n===== 日常實務：感測器資料清洗 =====" << std::endl;
    std::vector<Reading> data = {
        {1000, 21.5}, {1001, 21.7}, {1002, 21.6}, {1003, 88.4},
        {1004, 21.8}, {1005, 22.0}, {1006, -40.0}, {1007, 22.1},
    };
    std::cout << "  原始 " << data.size() << " 筆讀數" << std::endl;

    std::vector<int> spikes = findSpikes(data, 5.0);
    std::cout << "  跳變超過 5.0°C 的時間點: ";
    for (int ts : spikes) std::cout << ts << " ";
    std::cout << "（共 " << spikes.size() << " 處）" << std::endl;

    int dropped = dropOutOfRange(data, -20.0, 60.0);
    std::cout << "  剔除超出 [-20, 60] 的離群值: " << dropped << " 筆" << std::endl;
    std::cout << "  清洗後 " << data.size() << " 筆: ";
    for (const auto& r : data) std::cout << r.celsius << " ";
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
    demo_leetcode_118();
    demo_practical_sensor();

    std::cout << "\n================================================================" << std::endl;
    std::cout << "  複習完畢！迭代器：泛化指標、半開區間、失效問題、自訂支援" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 ===
// ================================================================
//   第4課：迭代器（Iterator）的核心概念  總複習
// ================================================================
// ===== 重點一：迭代器基本操作 =====
// 指標遍歷陣列: 10 20 30 40 50
//
// 迭代器基本操作：
// *it = 10
// ++it 後 *it = 20
// *it++ = 20
// 現在 *it = 30
// begin == end? 否
// begin != end? 是
//
// ===== 重點二：begin/end 與半開區間 =====
// 空容器：begin() == end()
// 遍歷容器: 10 20 30 40 50
// 元素個數: 5
// 子區間 [1, 4): 20 30 40
//
// ===== 重點三：迭代器型別 =====
// auto 迭代器 *it = 10
// map 首項: Alice=95
// 修改前: 10 20 30 40 50
// 修改後(x2): 20 40 60 80 100
// const_iterator: 20 40 60 80 100
// reverse_iterator: 100 80 60 40 20
//
// ===== 重點四：迭代器與演算法 =====
// find(8) in vector: 找到
// find(8) in list:   找到
// count(2) = 4
// 在子區間 [1,5) 找到 40，索引 = 3
//
// ===== 重點五：迭代器失效與安全刪除 =====
// [方法一：erase 回傳值]
// 原始: 1 2 3 4 5
// 刪除3後: 1 2 4 5
// [方法二：erase-remove 慣用法]
// 刪除所有2後: 1 3 4 5
// [方法三：list::remove]
// 刪除所有2後: 1 3 4 5
//
// ===== 重點六：迭代器與指標 =====
// 指標解參考: *ptr = 10
// 迭代器解參考: *it = 10
// 指標算術: *(ptr+2) = 30
// 迭代器算術: *(it+2) = 30
// *from_it = 10
// 在陣列中找到 300，索引=2
// 陣列排序: 1 2 3 4 5
//
// ===== 重點七：範圍 for 的底層 =====
// 範圍 for（唯讀）: 10 20 30 40 50
// 等價迭代器: 10 20 30 40 50
// 範圍 for（const 參考）: 10 20 30 40 50
// 修改後（x2）: 20 40 60 80 100
//
// ===== 重點八：自訂類別支援迭代器 =====
// 範圍 for: 1 2 3 4 5
// 迭代器: 1 2 3 4 5
//
// ===== LeetCode 118. Pascal's Triangle =====
//   1
//   1 1
//   1 2 1
//   1 3 3 1
//   1 4 6 4 1
//   numRows=0 時回傳空: 0 列
//
// ===== 日常實務：感測器資料清洗 =====
//   原始 8 筆讀數
//   跳變超過 5.0°C 的時間點: 1003 1004 1006 1007 （共 4 處）
//   剔除超出 [-20, 60] 的離群值: 2 筆
//   清洗後 6 筆: 21.5 21.7 21.6 21.8 22 22.1
//
// ================================================================
//   複習完畢！迭代器：泛化指標、半開區間、失效問題、自訂支援
// ================================================================
