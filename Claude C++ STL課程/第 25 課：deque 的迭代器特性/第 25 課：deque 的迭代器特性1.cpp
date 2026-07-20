// =============================================================================
//  第 25 課：deque 的迭代器特性1.cpp
//    —  random access 的完整能力，以及「iterator 失效但 reference 有效」
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔: #include <deque>
//   迭代器類別: LegacyRandomAccessIterator（與 vector 同級）
//     ++it / --it / it±n / it±=n / it2-it1 / it[n] / <,>,<=,>=,==,!=   皆 O(1)
//   ★ 實作上是「四個指標」的胖迭代器:cur / first / last / node
//
// 【詳細解釋 Explanation】
//
// 【1. 介面與 vector 相同，實作完全不同】
// vector 的元素連續，迭代器可以就是一根裸指標；++ 是位址加 sizeof(T)。
// deque 的元素分散在多個 buffer，裸指標走到段尾就沒有下一步 —— 它不知道
// 下一段在哪。所以 deque 的迭代器必須額外記住:
//     cur   目前元素
//     first / last  本段（buffer）的頭尾邊界
//     node  自己在 map（中控器指標陣列）的哪一格
// 本機實測 sizeof(deque<int>::iterator)=32 bytes、vector 的是 8 bytes。
// ★ 這是 libstdc++ 的實作細節，標準只規定行為與複雜度。
//
// 【2. 因此 ++ 與 += 的成本不同】
//   ++it → 多半只是 ++cur，外加一次 `cur == last` 邊界比較；
//          每 buffer_size 次才需要 set_node 跳下一段。
//   it += n → 可能跨越任意多段，需要**除法/取模**算出跳幾個 node。
//   → 兩者都是 O(1)，但常數差很多。遍歷請用 range-for / iterator，
//     不要用 `for (i…) dq[i]`（每次都重算定址）。
//
// 【概念補充 Concept Deep Dive】
// random access 這個「類別」不只是速度標籤，它決定**你能用哪些演算法**:
//   std::sort(dq.begin(), dq.end())    // deque 可以（需要 it+n、it2-it1）
//   std::sort(lst.begin(), lst.end())  // list 編譯失敗（只有 bidirectional）
// list 因此才需要自備成員函式 lst.sort()。
//
// 【注意事項 Pay Attention】
//  1. **頭尾插入 → 所有 iterator 失效，但 reference/pointer 仍有效**
//     （元素沒被搬移，死的是迭代器內部快取的 node）。
//  2. 中間 insert/erase → iterator 與 reference **全部**失效。
//  3. end() 不可解參考；迭代器的 < 與 - 只在同一容器內有意義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】deque 的迭代器特性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. deque 迭代器跟 vector 迭代器，介面與實作各差在哪?
//     答：介面完全相同，都是 random access。實作上 vector 的通常就是裸指標
//         （本機 8 bytes）；deque 的內含 cur/first/last/node 四個指標
//         （本機 32 bytes），因為它得知道本段邊界與自己在 map 的位置，
//         才能在越過 buffer 時跳到下一段。
//     追問：那 it += n 為什麼比 vector 貴?→ vector 是一次加法；
//         deque 要用除法/取模算出跨幾個 node，再重設段內偏移。
//
// ⚠️ 陷阱. 以下程式錯在哪?
//        auto it = dq.begin();
//        dq.push_back(42);      // 只在尾端加，前面的元素沒被動到
//        std::cout << *it;      // ← UB
//     答：deque 頭尾插入會使**所有 iterator 失效**（含 it）。雖然元素確實
//         沒被搬移，但 it 內部快取的 node 指向舊 map，而 map 可能已被重配。
//     為什麼會錯：直覺是「尾端插入不影響前面」——這對 **reference/pointer
//         成立**（標準明文保證仍有效），但對 **iterator 不成立**。
//         這正是 deque 把兩者拆開的地方；要跨越插入操作，請存 pointer 或索引。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <deque>
#include <vector>
#include <algorithm>
#include <iterator>
#include <type_traits>

int main() {
    std::deque<int> dq = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    std::cout << "=== 1. 迭代器類別與大小 ===\n";
    using DIt = std::deque<int>::iterator;
    constexpr bool isRandom =
        std::is_same_v<typename std::iterator_traits<DIt>::iterator_category,
                       std::random_access_iterator_tag>;
    std::cout << "  是 random_access_iterator? " << isRandom << "\n";
    std::cout << "  sizeof(deque<int>::iterator)  = " << sizeof(DIt)
              << " bytes（cur/first/last/node 四指標）\n";
    std::cout << "  sizeof(vector<int>::iterator) = "
              << sizeof(std::vector<int>::iterator) << " bytes（裸指標）\n";
    std::cout << "  ★ libstdc++ 實測值，標準未規定內部佈局\n";

    std::cout << "\n=== 2. 正向 / 反向遍歷 ===\n";
    std::cout << "  正向:";
    for (auto it = dq.begin(); it != dq.end(); ++it) std::cout << " " << *it;
    std::cout << "\n  反向:";
    for (auto it = dq.rbegin(); it != dq.rend(); ++it) std::cout << " " << *it;
    std::cout << "\n";

    std::cout << "\n=== 3. 隨機跳躍 / 距離 / 比較 / 下標 ===\n";
    auto it = dq.begin();
    it += 3;  std::cout << "  begin()+3 → " << *it << "\n";
    it += 4;  std::cout << "  再 +4     → " << *it << "\n";
    it -= 5;  std::cout << "  再 -5     → " << *it << "\n";
    std::cout << "  end()-begin() = " << (dq.end() - dq.begin()) << "\n";
    auto a = dq.begin() + 2, b = dq.begin() + 7;
    std::cout << "  a→" << *a << " b→" << *b
              << "  a<b? " << (a < b) << "  b-a=" << (b - a) << "\n";
    std::cout << "  begin()[0]=" << dq.begin()[0]
              << " begin()[5]=" << dq.begin()[5] << "（it[n] 等同 *(it+n)）\n";

    std::cout << "\n=== 4. random access → std::sort 可直接套用 ===\n";
    std::deque<int> messy = {42, 7, 19, 3, 25, 11};
    std::cout << "  排序前:";
    for (int v : messy) std::cout << " " << v;
    std::sort(messy.begin(), messy.end());   // 換成 list 這行無法編譯
    std::cout << "\n  排序後:";
    for (int v : messy) std::cout << " " << v;
    std::cout << "\n  ★ list 只有 bidirectional iterator，須改用成員 lst.sort()\n";

    std::cout << "\n=== 5. 失效規則:iterator 死、reference 活 ===\n";
    std::deque<int> d2 = {100, 200, 300};
    int* p = &d2[1];                       // 指向值 200 的元素
    std::cout << "  push_front 前 *p=" << *p << "\n";
    for (int i = 0; i < 1000; ++i) d2.push_front(i);
    std::cout << "  push_front 1000 次後 *p=" << *p
              << " → pointer 仍有效（元素未被搬移）\n";
    std::cout << "  同一元素索引已變成 1001，d2[1001]=" << d2[1001] << "\n";
    std::cout << "  ★ 但先前的所有 iterator 都已失效，解參考是 UB\n";
    std::cout << "  ★ 中間 insert/erase 則連 reference/pointer 也失效\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 25 課：deque 的迭代器特性1.cpp" -o deque_iter

//   ★ libstdc++ 實測值，標準未規定內部佈局

// === 預期輸出 ===
// === 1. 迭代器類別與大小 ===
//   是 random_access_iterator? 1
//   sizeof(deque<int>::iterator)  = 32 bytes（cur/first/last/node 四指標）
//   sizeof(vector<int>::iterator) = 8 bytes（裸指標）
//
// === 2. 正向 / 反向遍歷 ===
//   正向: 10 20 30 40 50 60 70 80 90 100
//   反向: 100 90 80 70 60 50 40 30 20 10
//
// === 3. 隨機跳躍 / 距離 / 比較 / 下標 ===
//   begin()+3 → 40
//   再 +4     → 80
//   再 -5     → 30
//   end()-begin() = 10
//   a→30 b→80  a<b? 1  b-a=5
//   begin()[0]=10 begin()[5]=60（it[n] 等同 *(it+n)）
//
// === 4. random access → std::sort 可直接套用 ===
//   排序前: 42 7 19 3 25 11
//   排序後: 3 7 11 19 25 42
//   ★ list 只有 bidirectional iterator，須改用成員 lst.sort()
//
// === 5. 失效規則:iterator 死、reference 活 ===
//   push_front 前 *p=200
//   push_front 1000 次後 *p=200 → pointer 仍有效（元素未被搬移）
//   同一元素索引已變成 1001，d2[1001]=200
//   ★ 但先前的所有 iterator 都已失效，解參考是 UB
//   ★ 中間 insert/erase 則連 reference/pointer 也失效
