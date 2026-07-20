// =============================================================================
//  06_reverse_iterator.cpp  —  std::reverse_iterator / make_reverse_iterator
// =============================================================================
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼這個 iterator 存在?                              │
//  └────────────────────────────────────────────────────────────┘
//
//  你想「從尾巴往頭」走訪一個容器,看似簡單,但用一般 iterator 寫起來很煩:
//
//      for (auto it = v.end(); it != v.begin(); ) {
//          --it;                  // 注意要先 --,因為 end() 不能解參考
//          std::cout << *it;
//      }
//
//  這個迴圈條件難寫:用 it >= v.begin() 會 UB (begin 之前不允許出現 iterator),
//  while-true + break 又難看。更糟的是:寫泛型演算法時,你想統一
//  「正向 / 反向」的介面,讓同一個 std::find / std::copy 可以反過來跑。
//
//  std::reverse_iterator<Iter> 就是「Iterator Adaptor」,把底層 iterator
//  的 ++/-- 行為對調 — 對它做 ++ 等於底層做 --。這樣:
//
//      // 從尾找 7,寫法跟正向 find 一模一樣
//      auto it = std::find(v.rbegin(), v.rend(), 7);
//
//  泛型程式碼一次寫好,反向掃 / 正向掃 通吃。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、底層機制 — 為什麼存的是「下一個位置」?                │
//  └────────────────────────────────────────────────────────────┘
//
//  reverse_iterator 內部存的是「對應到正向位置」的 iterator,
//  但解參考時會「先回退一步再讀」。為什麼這樣設計?
//
//  關鍵:STL 規範「容器的有效範圍」是 [begin, end),
//        end() 是「過尾哨兵」(past-the-end),不可解參考。
//
//  反向走訪時要對應的「過頭哨兵」(past-the-beginning) 是什麼?
//  → 只能是「begin() 前一格」 — 但這位置不存在、解參考會 UB。
//
//  解法:reverse_iterator 「永遠存正向位置 +1」:
//
//      rbegin() ≡ reverse_iterator(end())   ← 存 end,但 *rb 解參考時拿 end-1 = 最後元素
//      rend()   ≡ reverse_iterator(begin()) ← 存 begin,*re 不能解參考 (begin-1 不存在)
//
//  圖解 (序列 [1,2,3]):
//
//      正向: begin              end
//             ↓                  ↓
//             1   2   3
//             ↑                  ↑
//             rend             rbegin
//
//  這就是為什麼 rit.base() 「比 *rit 大一格」 — 它存的是「下一個正向位置」。
//
//  概念上的解參考實作:
//
//      class reverse_iterator {
//          Iter base_;           // 存「下一個正向位置」
//      public:
//          reference operator*() const {
//              auto tmp = base_;
//              --tmp;
//              return *tmp;       // 真實讀的是 base_ - 1
//          }
//          reverse_iterator& operator++() { --base_; return *this; }
//          reverse_iterator& operator--() { ++base_; return *this; }
//          Iter base() const { return base_; }
//      };
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、何時使用                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * 「反向走訪」最簡潔的寫法 — 比手寫 --it 安全。
//   * 把現成演算法「反向用」:
//       std::find(c.rbegin(), c.rend(), x)   ── 從尾找 (找最後一個)
//       std::copy(c.rbegin(), c.rend(), dst) ── 反向拷貝
//       std::lower_bound(c.rbegin(), ..., comp) ── 對降序資料二分搜
//   * 容器 .rbegin() / .rend() 提供得很完整;C++14 起也有非成員 std::rbegin / rend,
//     讓 C-array 也能反向走。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、Iterator 等級需求                                      │
//  └────────────────────────────────────────────────────────────┘
//
//   * reverse_iterator 至少需要 BidirectionalIterator (因為要 -- 才能反向 ++)。
//   * 對 forward_list / unordered_set / istream_iterator 不能包 reverse_iterator —
//     它們只支援 ++,沒有 --。
//   * 包出來的 reverse_iterator 等級 = 底層等級 (RandomAccess → RandomAccess)。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 五、Pitfalls (陷阱)                                        │
//  └────────────────────────────────────────────────────────────┘
//
//   1. rit.base() 指向的位置「比 *rit 大一格」。erase reverse_iterator 要轉換:
//        v.erase(std::next(rit).base());   // C++11 起常見作法
//      或更直觀:
//        v.erase(--(rit.base()));          // 拿 base 再退一格 (但要小心副作用)
//   2. *rend() 是 UB — rend 對應 begin-1,不存在。
//   3. 兩個 reverse_iterator 的「方向」相反:rit1 < rit2 對應 base1 > base2。
//      所以 rit1 - rit2 = base2 - base1,正負號要小心。
//   4. C++11 / C++14 對 base() 的「value category」有變化:
//      std::next(rit).base() 在 C++14 不會做不必要的拷貝。
//   5. make_reverse_iterator (C++14) 是工廠;舊 code 寫
//      std::reverse_iterator<It>(it) 也行,只是要打字型別。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 六、簽章 (Signatures)                                      │
//  └────────────────────────────────────────────────────────────┘
//
//      template <class Iter> class reverse_iterator;        // C++98
//
//      template <class Iter>
//      reverse_iterator<Iter> make_reverse_iterator(Iter);  // C++14 工廠
//
//      // 容器層級的 rbegin / rend (成員) — C++98
//      // 非成員 std::rbegin / std::rend — C++14 (對 C-array 也能用)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 七、參考連結 (References)                                  │
//  └────────────────────────────────────────────────────────────┘
//
//    https://en.cppreference.com/w/cpp/iterator/reverse_iterator       — class
//    https://en.cppreference.com/w/cpp/iterator/make_reverse_iterator  — C++14
//    https://en.cppreference.com/w/cpp/iterator/rbegin                 — rbegin / std::rbegin
//    https://en.cppreference.com/w/cpp/iterator/rend                   — rend  / std::rend
//    https://cplusplus.com/reference/iterator/reverse_iterator/        — 簡明
// =============================================================================

/*
補充筆記：reverse_iterator
  - reverse_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - reverse_iterator 的 base() 指向反向 iterator 所代表元素的下一個正向位置，這點最容易 off-by-one。
  - rbegin 對應最後一個元素，rend 對應第一個元素之前的停止位置。
  - 用 reverse_iterator erase 時通常要把 base() 調整到真正要刪的位置。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】reverse_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. reverse_iterator 的 base() 為什麼會差一個位置？
//     答：為了維持左閉右開區間的一致性。reverse_iterator 內部持有一個 base iterator，
//         它的 operator* 是「先退一格再解參考」，所以恆有 &*rit == &*(rit.base() - 1)。
//         於是 rbegin().base() == end()、rend().base() == begin()，區間 [rbegin, rend)
//         正好對應 [begin, end) 的反轉。
//     追問：rbegin() 指向哪個元素？（最後一個元素，但它的 base() 是 end()）
//
// 🔥 Q2. 拿到一個 reverse_iterator，要怎麼用它呼叫 erase？
//     答：不能直接寫 v.erase(rit.base())——那會刪到右邊一格。正確寫法是
//         v.erase(std::next(rit).base()) 或 v.erase((rit + 1).base())，先退一格再取 base。
//     追問：reverse_iterator 最低需要底層是哪一級 iterator？（bidirectional，因為它要靠
//         -- 才能往回走；unordered_* 是 forward，所以沒有 rbegin()）
//
// ⚠️ 陷阱. 「rbegin() 就是 end() - 1」這個理解錯在哪？
//     答：錯在把兩者當成同一個東西。rbegin() 確實「指到」最後一個元素，但它是
//         reverse_iterator 型別，其 base() 等於 end() 而不是 end() - 1；型別與 base 位置
//         兩件事都不同，混用會在 erase / insert 時整整偏一格。
//     為什麼會錯：腦中只記住「指向哪個元素」，忽略了 reverse_iterator 是一層 adaptor，
//         base 位置與解參考位置刻意錯開一格。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5};

    // -----------------------------------------------------------------------
    // 範例 1:反向印出
    //   rbegin() 是「指向最後一個元素」、rend() 是「指向第一個元素的前一格」。
    //   迴圈條件 it != rend() 跟正向用法一致 — 這就是 adaptor 的價值。
    // -----------------------------------------------------------------------
    std::cout << "rbegin/rend: ";
    for (auto it = v.rbegin(); it != v.rend(); ++it) std::cout << *it << ' ';
    std::cout << '\n';   // 5 4 3 2 1

    // -----------------------------------------------------------------------
    // 範例 2:base() 之間的關係 — *(rit.base() - 1) == *rit
    // -----------------------------------------------------------------------
    auto rit = v.rbegin();      // 指向 5
    std::cout << "*rit              = " << *rit << '\n';
    std::cout << "*(rit.base() - 1) = " << *(rit.base() - 1) << " (一樣是 5)\n";

    // -----------------------------------------------------------------------
    // 範例 3:用 reverse iterator 找「最後一個」符合條件的元素
    //   std::find 從 rbegin/rend 跑,等於從尾巴往頭找。
    // -----------------------------------------------------------------------
    std::vector<int> w = {1, 7, 3, 7, 5};
    auto last_seven = std::find(w.rbegin(), w.rend(), 7);
    if (last_seven != w.rend()) {
        // 算正向索引 = (size - 1) - (rit - rbegin)
        auto idx = std::distance(last_seven, w.rend()) - 1;
        std::cout << "last 7 在索引 " << idx << " (期望 3)\n";
    }

    // -----------------------------------------------------------------------
    // 範例 4:把字串反轉印出 (不修改原字串)
    //   string ctor 接受 (InputIt first, InputIt last),這裡傳 rbegin/rend
    //   就建出反向的 string。
    // -----------------------------------------------------------------------
    std::string s = "Hello";
    std::string r(s.rbegin(), s.rend());
    std::cout << "reverse(\"Hello\") = " << r << " (期望 olleH)\n";

    // -----------------------------------------------------------------------
    // 範例 5:std::make_reverse_iterator (C++14)
    //   * 工具函式 — 把一個 iterator 包成它對應的 reverse_iterator,自動推導型別。
    //   * 真正用途:當你「手上只有一個普通 iterator (不是容器)」、需要建反向 iterator 時。
    //     例如用 std::find 找到位置後,想從該位置「往回掃」。
    // -----------------------------------------------------------------------
    std::vector<int> data = {10, 20, 30, 40, 50, 60, 70};

    // 找到 40 → 從 40 往「前」找第一個 < 25 的元素
    auto pivot = std::find(data.begin(), data.end(), 40);
    if (pivot != data.end()) {
        // 注意關鍵:把 pivot 包成 reverse_iterator 後,++ 會「往容器頭部」走。
        auto rit2 = std::make_reverse_iterator(pivot);
        auto found = std::find_if(rit2, data.rend(),
                                  [](int x){ return x < 25; });
        if (found != data.rend()) {
            std::cout << "從 40 往左第一個 <25 = " << *found
                      << " (期望 20)\n";
        }
    }

    // 直接驗證:make_reverse_iterator(end()) == rbegin()
    auto rb = std::make_reverse_iterator(data.end());
    std::cout << "*make_reverse_iterator(end()) = " << *rb
              << " (= *rbegin() = 70)\n";

    // === LeetCode / 實務範例 ===
    void leetcode_344_reverse_string();
    void leetcode_7_reverse_integer();
    void practical_show_recent_logs_first();
    void leetcode_541_reverse_string_ii();
    void practical_stack_trace_top_down();
    leetcode_344_reverse_string();
    leetcode_7_reverse_integer();
    practical_show_recent_logs_first();
    leetcode_541_reverse_string_ii();
    practical_stack_trace_top_down();

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 rit.base() 會「比 *rit 多一格」(偏移 +1)?
    //    A:因為 STL 規範有效範圍是 [begin, end),end() 是「過尾哨兵」不可解參考。
    //      反向走訪需要對應的「過頭哨兵」,但 begin() 之前的位置沒有合法 iterator。
    //      解法是讓 reverse_iterator 內部存「正向位置 +1」(也就是「下一個正向位置」),
    //      解參考時先 --tmp 再讀。這樣 rbegin() = reverse_iterator(end()),
    //      rend() = reverse_iterator(begin()),全部落在合法範圍內。
    //
    //  Q2:reverse_iterator 至少需要哪一級 iterator?
    //    A:至少需要 BidirectionalIterator,因為它的 operator++ 會呼叫底層的 --。
    //      因此 forward_list / unordered_set / istream_iterator 不能包成 reverse_iterator。
    //      包出來的等級「沿用底層」 — 底層是 RandomAccess,reverse_iterator 也是
    //      RandomAccess,可以用 std::sort 對反向序列排序 (但意義通常不大)。
    //
    //  Q3:在 vector 上拿到 reverse_iterator 後想 erase 該元素,怎麼寫?
    //    A:erase 只接受正向 iterator,所以要先轉。常見慣用語:
    //          v.erase(std::next(rit).base());   // C++11 起,不會多一次拷貝
    //      原理是 next(rit) 等於底層位置往前一格,再取 base() 拿到正向 iterator,
    //      正好指到 *rit 對應的元素。直接寫 rit.base() 會指錯地方 (差一格)。
    //

    return 0;
}

// ============================================================
//                LeetCode / 實務範例 (Practical Examples)
// ============================================================

// ----------------------------------------------------------------
// LeetCode 344: Reverse String
// ----------------------------------------------------------------
// 題目:給字元陣列 s,把它「就地 (in-place)」反轉。
//      限制:不可額外開 O(n) 陣列。
//
// 為什麼這題對 reverse_iterator / 雙指針適合:
//   * 雙指針從兩端 swap → 經典 in-place 反轉手法,O(n/2)。
//   * 也可以一行完成:std::reverse(s.begin(), s.end());
//   * 或用 std::copy(s.rbegin(), s.rend(), tmp.begin()) 寫到另一個容器
//     (但會違反 in-place 限制)。
//
// 解法核心:
//   * l 從頭、r 從尾,iter_swap 兩端後 l++, --r,直到相遇。
//
// 複雜度:時間 O(n);空間 O(1)。
void leetcode_344_reverse_string() {
    auto reverse_string = [](std::vector<char>& s) {
        auto l = s.begin();
        auto r = s.end();
        while (l != r && l != --r) {
            std::iter_swap(l, r);
            ++l;
        }
    };
    std::vector<char> chars = {'h','e','l','l','o'};
    reverse_string(chars);
    std::cout << "LC344 reversed chars: ";
    for (char c : chars) std::cout << c;       // olleh
    std::cout << '\n';
}

// ----------------------------------------------------------------
// LeetCode 7: Reverse Integer
// ----------------------------------------------------------------
// 題目:給 32-bit 整數 x,回傳「位數反轉」後的整數。例:
//      x = 123  →  321
//      x = -120 → -21
//      若反轉後溢出 32-bit 範圍,回傳 0 (本範例略過此檢查,以 long long 暫存)。
//
// 為什麼這題對 reverse_iterator 適合示範:
//   * 把整數先轉字串,再用 string ctor + rbegin/rend「直接造一個反向字串」,
//     把「字串反轉」這件事寫成單行,展示 reverse_iterator 的最 idiomatic 用法。
//   * 注意處理負號 — 把 abs 後反轉,最後再加負號回去。
//
// 解法核心 (字串版,最直觀):
//   1. 轉成字串 (取絕對值)。
//   2. 用 rbegin/rend 造反向字串。
//   3. 轉回整數,根據原號加負。
//
// 複雜度:時間 O(d) (d = 位數);空間 O(d)。
void leetcode_7_reverse_integer() {
    auto reverse_int = [](int x) -> long long {
        std::string str = std::to_string(std::abs((long long)x));
        std::string rev(str.rbegin(), str.rend());     // ← reverse_iterator!
        long long val = std::stoll(rev);
        return x < 0 ? -val : val;
    };
    std::cout << "LC7 reverse 12345 = " << reverse_int(12345) << '\n';
    std::cout << "LC7 reverse -120  = " << reverse_int(-120) << '\n';
}

// ----------------------------------------------------------------
// 實務範例:顯示「最近 N 筆 log」(時間倒序)
// ----------------------------------------------------------------
// 場景:log 容器裡是按時間遞增儲存的;UI 卻要顯示「最新的在最上面」。
//      最簡單的做法 — 用 rbegin / rend 直接反向走訪,不必額外複製或 sort。
//
// 重點:
//   * 不需要任何額外記憶體,O(1) 空間。
//   * 不影響原容器內的順序。
//   * std::for_each 接 rbegin/rend 也通,因為 reverse_iterator 是
//     BidirectionalIterator (vector 上更是 RandomAccess)。
void practical_show_recent_logs_first() {
    std::vector<std::string> logs = {
        "10:00:01 server start",
        "10:00:05 user login: alice",
        "10:00:09 query: SELECT *",
        "10:00:14 user logout: alice",
    };

    std::cout << "最近 log 倒序顯示:\n";
    int n = 0;
    for (auto it = logs.rbegin(); it != logs.rend() && n < 3; ++it, ++n) {
        std::cout << "  - " << *it << '\n';
    }
}

// ----------------------------------------------------------------
// LeetCode 541: Reverse String II (區段反轉)
// ----------------------------------------------------------------
// 題目:給字串 s 與整數 k,每 2k 字元就把前 k 個反轉,剩下的 (不滿 k 的尾巴) 全反轉。
//      例 s="abcdefg", k=2 → "bacdfeg"
//
// 為什麼這題適合用 std::reverse + iterator:
//   * 每段反轉的範圍是 [i, i+k),用 begin() + i 與 begin() + min(i+k, len) 算出。
//   * std::reverse 內部就是雙指針 swap,需要 BidirectionalIterator;string 的 iterator
//     是 RandomAccess,自然滿足。
//
// 解法核心:迴圈每次跳 2k 步,反轉前 k 個。
// 複雜度:時間 O(n)、空間 O(1)。
void leetcode_541_reverse_string_ii() {
    auto reverse_str_ii = [](std::string s, int k) {
        for (size_t i = 0; i < s.size(); i += 2 * k) {
            auto first = s.begin() + i;
            auto last  = s.begin() + std::min(i + k, s.size());
            std::reverse(first, last);
        }
        return s;
    };
    std::cout << "LC541 \"abcdefg\" k=2 = " << reverse_str_ii("abcdefg", 2)
              << " (期望 bacdfeg)\n";
}

// ----------------------------------------------------------------
// 實務範例:Stack Trace 由內而外印出 (Top-Down)
// ----------------------------------------------------------------
// 場景:存 stack trace 用 vector<string>,新呼叫 push_back 到尾端 (最內層),
// 但顯示時習慣「最外層 main 在最上面」 — 這就用 rbegin/rend 反向走訪。
// 不需要拷貝,O(1) 額外空間。
void practical_stack_trace_top_down() {
    std::vector<std::string> trace{
        "main",            // 最外層
        "parse_args",
        "load_config",
        "read_file"        // 最內層
    };
    std::cout << "Stack Trace (top-down):\n";
    for (auto it = trace.rbegin(); it != trace.rend(); ++it)
        std::cout << "  at " << *it << '\n';
    // 預期輸出: read_file -> load_config -> parse_args -> main
}

// === 預期輸出 (Expected output) ===
// rbegin/rend: 5 4 3 2 1
// *rit              = 5
// *(rit.base() - 1) = 5 (一樣是 5)
// last 7 在索引 3 (期望 3)
// reverse("Hello") = olleH (期望 olleH)
// 從 40 往左第一個 <25 = 20 (期望 20)
// *make_reverse_iterator(end()) = 70 (= *rbegin() = 70)
// LC344 reversed chars: olleh
// LC7 reverse 12345 = 54321
// LC7 reverse -120  = -21
// 最近 log 倒序顯示:
//   - 10:00:14 user logout: alice
//   - 10:00:09 query: SELECT *
//   - 10:00:05 user login: alice
