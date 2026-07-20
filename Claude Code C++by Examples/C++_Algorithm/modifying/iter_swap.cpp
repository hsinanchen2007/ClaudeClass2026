// ============================================================
// std::iter_swap
// 分類 (Category): Modifying sequence operations (修改型)
// 標頭檔 (Header):  <algorithm>
// 參考 (References):
//   * https://en.cppreference.com/w/cpp/algorithm/iter_swap
//   * https://cplusplus.com/reference/algorithm/iter_swap/
// ============================================================
//
// ┌────────────────────────────────────────────────────────────┐
// │ 一、課題介紹 (Topic Introduction)                          │
// └────────────────────────────────────────────────────────────┘
//
// std::iter_swap 解的問題:
//
//   「給我兩個迭代器 a, b,把它們指向的元素交換。」
//
// 概念上等同於:
//
//   std::swap(*a, *b);
//
// 多數情況下兩者結果一致,但 iter_swap 在「proxy 迭代器」
// (像 std::vector<bool>::iterator) 與「跨容器交換」的場景中更可靠。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 二、為什麼不直接用 std::swap(*a, *b)?                     │
// └────────────────────────────────────────────────────────────┘
//
// 對於 std::vector<bool> 這類「特化容器」,*a 不是一個 bool& 而是
// 「proxy reference」 (BitReference)。寫 std::swap(*a, *b) 會試圖
// 對 proxy 物件做 swap,實作上有時不會真的交換到底層 bit。
//
// std::iter_swap 的標準語意是「交換兩個迭代器指向的東西」,
// 標準庫實作會走 ADL 找對應的 swap (例如 vector<bool>::swap),
// 因此對 proxy 容器更安全。
//
// 結論:寫泛型 (generic) 程式碼,要交換「兩個迭代器解參考」的內容,
//      用 iter_swap 比 swap(*a, *b) 更穩。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 三、跨容器與跨型別的交換                                   │
// └────────────────────────────────────────────────────────────┘
//
// iter_swap 的兩個迭代器型別「可以不同」 — 只要 *a 與 *b
// 兩個物件可以互相 swap。常見情境:
//
//   * vector<int> 的某元素 與 list<int> 的某元素交換
//   * 兩個不同的 vector 中各自的某筆資料交換
//   * 在 partition / sort 內部用迭代器對換,不必先解參考
//
// ┌────────────────────────────────────────────────────────────┐
// │ 四、函式簽章 (Signatures)                                  │
// └────────────────────────────────────────────────────────────┘
//
//   template <class FwdIt1, class FwdIt2>
//   void iter_swap(FwdIt1 a, FwdIt2 b);
//
//   * C++20 起為 constexpr。
//   * C++20 引入 std::ranges::iter_swap (有更強的客製化點)。
//
// ┌────────────────────────────────────────────────────────────┐
// │ 五、複雜度 (Complexity)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   時間: 常數時間 — O(1)
//   空間: O(1)
//
// ┌────────────────────────────────────────────────────────────┐
// │ 六、注意事項 (Pitfalls)                                    │
// └────────────────────────────────────────────────────────────┘
//
//   1. 對普通容器,iter_swap(a, b) ≡ std::swap(*a, *b);沒差。
//   2. 對 proxy 容器 (vector<bool>),iter_swap 才是安全選項。
//   3. C++20 之後,std::ranges::iter_swap 是更通用的客製化點,
//      自訂迭代器若想支援交換,應該特化它。
//   4. 兩個迭代器若指向同一位置,效果就是「自我交換」 — 通常無害但無意義。
//
// ============================================================

/*
補充筆記：std::iter_swap
  - iter_swap 交換兩個 iterator 指向的元素，而不是交換 iterator 變數本身。
  - 它會使用 ADL 找到適合元素型別的 swap，對自訂型別比手寫暫存值更可靠。
  - 兩個 iterator 都必須可解參考且可交換。
  - std::iter_swap 會改變目標範圍內容或元素順序；呼叫前要確認輸出範圍空間足夠、iterator 有效。
  - copy/transform/generate/fill 寫入目的地時不會自動擴容，除非你使用 back_inserter 這類 insert iterator。
  - remove/unique 不會真的縮短容器，只把保留元素移到前面並回傳新邏輯結尾；還要搭配 erase 才會改變 size。
  - reverse/rotate/swap_ranges 會改變元素位置；若外部保存索引或 iterator，要重新確認是否仍指向預期元素。
  - move algorithm 會把元素搬到目的地，來源仍有效但值可能改變；後續只能重新指定或安全銷毀。
  - shuffle/sample 需要亂數引擎；不要每次呼叫都用同一個固定種子，除非你刻意要可重現測試結果。
*/

// ===========================================================================
// 【面試題】std::iter_swap
// ---------------------------------------------------------------------------
// 🔥 Q1. iter_swap(a, b) 和 std::swap(a, b) 差在哪?
//     答:iter_swap 交換的是「兩個 iterator 指向的元素」(語意等同 swap(*a, *b));
//         std::swap(a, b) 交換的是「兩個 iterator 變數本身」,元素完全沒動。
//         名字很像但作用的層次不同,是很常見的口誤來源。複雜度 O(1)。
//
// ⚠️ 陷阱. 那我自己寫 auto tmp = *a; *a = *b; *b = tmp; 不是一樣嗎?
//     答:對一般容器一樣,但對 proxy 容器(最典型的是 std::vector<bool>)會壞掉。
//         vector<bool> 的 operator* 回傳的是 proxy reference 而不是 bool&,
//         auto tmp = *a; 推導出來的是那個 proxy(仍指著原位置),不是值的複本,
//         於是 tmp 會跟著被改寫,交換失敗。iter_swap 則透過 ADL 找到專為該 proxy
//         提供的 swap,是安全的做法。
//     為什麼會錯:大家假設 *it 一定是真正的 reference。vector<bool> 是標準裡著名的
//         特例,它把多個 bool 壓進位元,沒有真正的 bool& 可以回傳。
//
// Q2. C++20 之後寫自訂 iterator,該支援什麼?
//     答:C++20 引入 std::ranges::iter_swap 這個 customization point,比 C++98 的
//         std::iter_swap 更通用(能處理 proxy iterator 與跨型別交換)。
//         自訂 iterator 若要支援交換,應該讓 ranges::iter_swap 找得到你的實作。
//
// Q3. 兩個 iterator 指向同一個位置時會怎樣?
//     答:就是自我交換,通常無害但也沒有意義。不過這提醒了一件事:寫演算法時最好自己
//         避開這種呼叫,別假設每個型別的 swap 都對自我交換做過驗證。
// ===========================================================================

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <vector>

// ============================================================
//                          基本範例
// ============================================================
int main() {
    std::vector<int> v{1, 2, 3, 4, 5};

    // --- 範例 1: 交換首尾元素 ---
    std::iter_swap(v.begin(), v.end() - 1);
    std::cout << "swap first-last: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';

    // --- 範例 2: 跨容器交換元素 (vector vs list) ---
    std::list<int> l{100, 200, 300};
    std::iter_swap(v.begin(), l.begin());
    std::cout << "v[0] now=" << v.front()
              << ", l.front()=" << l.front() << '\n';

    // --- 範例 3: vector<bool> proxy iterator (示範安全性) ---
    std::vector<bool> bits{true, false, true};
    std::iter_swap(bits.begin(), bits.begin() + 2);
    std::cout << "bits: " << bits[0] << ' ' << bits[1] << ' ' << bits[2] << '\n';

    // --- 範例 4: 用 iter_swap 手寫「反轉」 (示範它在 partition/reverse 內部的角色) ---
    std::vector<int> w{5, 2, 8, 1, 9, 3};
    auto lo = w.begin(), hi = w.end() - 1;
    while (lo < hi) {
        std::iter_swap(lo, hi);
        ++lo; --hi;
    }
    std::cout << "reversed manually: ";
    for (int x : w) std::cout << x << ' ';
    std::cout << '\n';

    // === LeetCode / 實務範例 ===
    void leetcode_344_reverse_string();
    void practical_cross_container_swap();
    void leetcode_905_sort_array_by_parity();
    void practical_swap_top_with_min();
    leetcode_344_reverse_string();
    practical_cross_container_swap();
    leetcode_905_sort_array_by_parity();
    practical_swap_top_with_min();
    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 344: 反轉字串 (Reverse String)
// ----------------------------------------------------------------
// 題目:給字元陣列 s,原地反轉之 (空間需 O(1))。
//
// 為什麼用 std::iter_swap:
//   反轉的雙指針作法核心動作就是「對換頭尾」 — 用迭代器版本最直白,
//   不必去想 *lo 是不是有 proxy 等細節。
//
// 解法步驟:
//   1. lo 指向 begin,hi 指向最後一個元素。
//   2. while (lo < hi): iter_swap(lo, hi);  ++lo; --hi;
//
// 複雜度:時間 O(n);空間 O(1)。
//        (你也可以直接用 std::reverse,這裡示範底層動作。)
void leetcode_344_reverse_string() {
    std::vector<char> s{'h','e','l','l','o'};
    auto lo = s.begin();
    auto hi = s.end();
    if (hi != lo) --hi;
    while (lo < hi) {
        std::iter_swap(lo, hi);
        ++lo; --hi;
    }
    std::cout << "LC344: ";
    for (char c : s) std::cout << c;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:跨容器交換元素 (任務佇列負載平衡)
// ----------------------------------------------------------------
// 場景:兩個 worker 的任務佇列 A、B,監控發現 B 的某個任務太重,
//      想跟 A 的某個輕任務對換,以平衡兩端負擔。
//
// 為什麼用 std::iter_swap:
//   兩個容器是不同的 vector,直接 swap(*itA, *itB) 也行,
//   但用 iter_swap 程式語意更清楚:「我在交換兩個迭代器指向的元素」。
void practical_cross_container_swap() {
    std::vector<std::string> a{"taskA1", "taskA2", "taskA3"};
    std::vector<std::string> b{"taskB1", "taskB2", "taskB3"};
    std::iter_swap(a.begin() + 1, b.begin() + 2);   // a[1] <-> b[2]
    std::cout << "A: ";
    for (auto& s : a) std::cout << s << ' ';
    std::cout << "\nB: ";
    for (auto& s : b) std::cout << s << ' ';
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 905: 按奇偶排序陣列 (Sort Array By Parity)
// ----------------------------------------------------------------
// 題目:把陣列中的偶數放在前面,奇數放在後面 (不要求個別內部排序)。
//
// 為什麼用 std::iter_swap:
//   經典「雙指針 partition」核心動作:左找到奇,右找到偶,iter_swap。
//   這是 std::partition 的內部寫法之一。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_905_sort_array_by_parity() {
    std::vector<int> nums{3, 1, 2, 4};
    auto lo = nums.begin(), hi = nums.end() - 1;
    while (lo < hi) {
        if (*lo % 2 == 0) ++lo;
        else if (*hi % 2 != 0) --hi;
        else { std::iter_swap(lo, hi); ++lo; --hi; }
    }
    std::cout << "LC905:";
    for (int x : nums) std::cout << ' ' << x;
    std::cout << '\n';
}

// ----------------------------------------------------------------
// 實務範例:把容器中「最小值」交換到開頭 (selection-sort 第一步)
// ----------------------------------------------------------------
// 場景:某些情境只需要「最小元素」放到開頭 (例如取最小報價作為基準),
//      不必整個排序。min_element + iter_swap 一行完成。
void practical_swap_top_with_min() {
    std::vector<int> v{40, 25, 60, 10, 35};
    auto mn = std::min_element(v.begin(), v.end());
    std::iter_swap(v.begin(), mn);
    std::cout << "min at front:";
    for (int x : v) std::cout << ' ' << x;
    std::cout << '\n';
}

// 編譯: g++ -std=c++20 -Wall -Wextra iter_swap.cpp -o iter_swap

// === 預期輸出 ===
// swap first-last: 5 2 3 4 1
// v[0] now=100, l.front()=5
// bits: 1 0 1
// reversed manually: 3 9 1 8 2 5
// LC344: olleh
// A: taskA1 taskB3 taskA3
// B: taskB1 taskB2 taskA2
// LC905: 4 2 1 3
// min at front: 10 25 60 40 35
