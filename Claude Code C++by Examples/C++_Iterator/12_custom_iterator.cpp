// =============================================================================
//  12_custom_iterator.cpp  —  自製 Iterator (理解 iterator 的型別契約)
// =============================================================================
//  iterator 不是「魔法」— 它就是個遵守特定 typedef 與 operator 介面的 class。
//  寫過一次，你才會真正看懂 STL 為什麼能那麼通用。
//
//  自製 iterator 必備五個 typedef (餵給 std::iterator_traits)：
//     iterator_category   — input/output/forward/bidirectional/random_access_tag
//     value_type          — operator* 解出來的型別
//     difference_type     — it1 - it2 的回傳型別 (通常 std::ptrdiff_t)
//     pointer             — operator-> 的回傳型別 (常 = value_type*)
//     reference           — operator* 的回傳型別 (常 = value_type&)
//
//  必備運算子：
//     至少：  *it     ++it    it1==it2    it1!=it2     (InputIterator)
//     再加：  it++                                       (post-increment)
//     再加：  --it / it--                                (Bidirectional)
//     再加：  it+n / it-n / it+=n / it-=n / it[n]
//             it1<it2 / it1-it2 等                      (RandomAccess)
//
//  這個範例：寫一個「只讀 + 跨步走訪 (stride)」的 RandomAccess iterator，
//  封裝成簡單的 view，可以無痛餵給 std::accumulate / std::find 等演算法。
//
//  歷史 (重要)：
//    在 C++11 / C++14 時代，自製 iterator 通常會「繼承」 std::iterator<Tag, T>
//    這個基底類別 — 它提供了上面那五個 typedef 的預設值，省去手寫。
//    但因為它的模板參數順序與多個預設值容易誤用、且與後來的概念體系格格不入，
//    C++17 起 std::iterator 已被「deprecated (將被淘汰)」。
//    現代寫法 = 直接在 class 內手寫五個 typedef (即本檔示範的方式)。
//    本檔最後也保留一個小範例，示範「歷史用法」是長什麼樣子，方便讀舊 code.
//
//  參考連結 (cppreference / cplusplus)：
//    https://en.cppreference.com/w/cpp/iterator                       — Iterator 總覽
//    https://en.cppreference.com/w/cpp/iterator/iterator_traits       — 五個 typedef
//    https://en.cppreference.com/w/cpp/iterator/iterator_tags         — 五個 tag
//    https://en.cppreference.com/w/cpp/iterator/iterator              — std::iterator (deprecated)
//    https://en.cppreference.com/w/cpp/named_req/RandomAccessIterator — 規格細節
//    https://cplusplus.com/reference/iterator/                        — 簡明
// =============================================================================

/*
補充筆記：custom_iterator
  - custom_iterator 的核心是「位置」與「走訪能力」；iterator 不擁有元素，只是通往容器元素的操作介面。
  - 不同 iterator category 能力不同：input 只能讀一次，forward 可多次走訪，bidirectional 可退後，random access 可做加減與索引。
  - end iterator 是哨兵位置，不能解參考；所有 [begin, end) 半開區間都依賴這個規則。
  - 容器修改可能讓 iterator 失效；vector reallocation、erase、insert 的規則和 list/map 不同，不能通用直覺。
  - insert iterator、stream iterator、move iterator 都是在改變演算法如何讀寫元素，而不是改變演算法本身。
  - 自訂 iterator 要讓 value_type、reference、difference_type、iterator_category 等 traits 能被演算法理解。
  - 自訂 iterator 要提供演算法需要的操作，例如解參考、遞增、相等比較和 traits。
  - iterator_category 或 iterator_concept 會影響標準演算法選擇哪些操作；標錯能力會造成錯誤假設。
  - reference 型別若不是 T&，例如 proxy reference，要特別小心和泛型演算法的相容性。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】自訂 iterator 與 iterator_traits
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 什麼是 std::iterator_traits？為什麼需要這一層？
//     答：它是一層 traits 間接層，統一取出 iterator 的五個關聯型別：value_type、
//         difference_type、pointer、reference、iterator_category。需要它的原因是泛型演算法
//         既要支援 class 型 iterator（可以自己寫 nested typedef），也要支援裸指標 T*
//         （不可能有 nested typedef）；iterator_traits 對 T* 提供 partial specialization，
//         讓兩者在演算法眼中長得一樣。
//     追問：iterator_category 拿來做什麼？（tag dispatch，例如 distance / advance 依等級
//         選 O(1) 或 O(n) 實作）／C++20 的 concepts 取代它了嗎？（沒有，iterator_traits
//         仍在，另外新增 iter_value_t 等別名）
//
// 🔥 Q2. 如何自己寫一個符合 STL 規範的 iterator？
//     答：先提供五個關聯型別，再依目標 category 實作對應的 operator。以 forward iterator
//         為例需要：operator*、operator->、pre 與 post 的 operator++、operator== / !=、
//         default constructor、copy constructor 與 copy assignment。Bidirectional 再加
//         operator--；random access 再加 +、-、+=、-=、[]、< 這組。
//     追問：C++17 之後還能繼承 std::iterator 嗎？（已 deprecated，改成直接寫 typedef，或
//         用 C++20 的 iterator concepts）／怎麼驗證自己寫對了？（static_assert
//         (std::forward_iterator<MyIt>)，concept 會明確指出違反哪一條）
//
// Q3. 自訂 iterator 只寫了 operator++ 和 operator*，為什麼 std::sort 編譯不過？
//     答：因為 sort 需要 random access——要 it + n、it1 - it2、it[n] 與 < 這一整組操作，
//         還要 iterator_category 標成 random_access_iterator_tag。category tag 標錯（宣稱
//         比實際能力高）不會有人幫你檢查，只會在演算法內部炸出難懂的錯誤訊息。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iterator>
#include <vector>
#include <numeric>
#include <algorithm>

// -----------------------------------------------------------------------------
// StrideIterator<T>
//   走訪一段連續記憶體，但每次跳 stride 步。
//   給定 [10,20,30,40,50,60], stride=2 → 走訪 10, 30, 50.
// -----------------------------------------------------------------------------
template <class T>
class StrideIterator {
public:
    // 五大 typedef — STL 演算法靠這個分派 (tag dispatch)
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    StrideIterator(T* p, std::ptrdiff_t stride) : p_(p), s_(stride) {}

    // ---- 解參考 ----
    reference operator*()  const { return *p_; }
    pointer   operator->() const { return  p_; }
    reference operator[](difference_type n) const { return *(p_ + n * s_); }

    // ---- 前/後增減 ----
    StrideIterator& operator++()    { p_ += s_; return *this; }
    StrideIterator  operator++(int) { auto tmp = *this; p_ += s_; return tmp; }
    StrideIterator& operator--()    { p_ -= s_; return *this; }
    StrideIterator  operator--(int) { auto tmp = *this; p_ -= s_; return tmp; }

    // ---- 算術 (RandomAccess 才有) ----
    StrideIterator& operator+=(difference_type n) { p_ += n * s_; return *this; }
    StrideIterator& operator-=(difference_type n) { p_ -= n * s_; return *this; }
    StrideIterator  operator+ (difference_type n) const { auto t=*this; return t+=n; }
    StrideIterator  operator- (difference_type n) const { auto t=*this; return t-=n; }
    difference_type operator- (const StrideIterator& o) const { return (p_ - o.p_) / s_; }

    // ---- 比較 ----
    bool operator==(const StrideIterator& o) const { return p_ == o.p_; }
    bool operator!=(const StrideIterator& o) const { return p_ != o.p_; }
    bool operator< (const StrideIterator& o) const { return p_ <  o.p_; }
    bool operator<=(const StrideIterator& o) const { return p_ <= o.p_; }
    bool operator> (const StrideIterator& o) const { return p_ >  o.p_; }
    bool operator>=(const StrideIterator& o) const { return p_ >= o.p_; }

private:
    T*               p_;
    std::ptrdiff_t   s_;
};

// -----------------------------------------------------------------------------
// HistoricalSimpleIter<T>  (歷史用法 — std::iterator 已 C++17 deprecated)
//   只示範「以前的人怎麼寫」，不建議在新 code 用。
//
//   std::iterator<Category, T, Distance=ptrdiff_t, Pointer=T*, Reference=T&>
//   會幫你把那五個 typedef 加進來，不必手寫；繼承後類別自動滿足 iterator_traits.
//
//   為什麼被淘汰？
//     * 模板參數順序對使用者不直觀，常常傳錯。
//     * 多數 STL 實作其實沒有真的去查這個基底，只是「方便加 typedef」。
//     * 與後來 (C++20) 的 iterator 概念體系不相容.
//   現代做法：直接在自己的 class 裡手寫那五個 typedef (= 上面 StrideIterator).
// -----------------------------------------------------------------------------
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

template <class T>
class HistoricalSimpleIter
    : public std::iterator<std::forward_iterator_tag, T> {  // ← 五個 typedef 自動產生
public:
    explicit HistoricalSimpleIter(T* p) : p_(p) {}
    T& operator*() const { return *p_; }
    HistoricalSimpleIter& operator++() { ++p_; return *this; }
    bool operator==(const HistoricalSimpleIter& o) const { return p_ == o.p_; }
    bool operator!=(const HistoricalSimpleIter& o) const { return p_ != o.p_; }
private:
    T* p_;
};

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50, 60, 70, 80};

    // -----------------------------------------------------------------------
    // 範例 1：用 StrideIterator 走「偶數位」(stride=2) — 應走訪 10, 30, 50, 70
    // -----------------------------------------------------------------------
    StrideIterator<int> begin(v.data(), 2);
    StrideIterator<int> end  (v.data() + v.size(), 2);

    std::cout << "stride=2 走訪: ";
    for (auto it = begin; it != end; ++it) std::cout << *it << ' ';
    std::cout << '\n';

    // -----------------------------------------------------------------------
    // 範例 2：直接餵進 std::accumulate — 因為我們宣告了 random_access_iterator_tag,
    //   STL 完全不知道也不在意我們是「自製」的。
    // -----------------------------------------------------------------------
    int sum = std::accumulate(begin, end, 0);
    std::cout << "stride=2 sum = " << sum << " (10+30+50+70 = 160)\n";

    // -----------------------------------------------------------------------
    // 範例 3：std::find_if 也通 — 只需要 InputIterator 等級
    // -----------------------------------------------------------------------
    auto found = std::find_if(begin, end, [](int x){ return x > 25; });
    if (found != end) std::cout << "第一個 >25 = " << *found
                                << " (期望 30)\n";

    // -----------------------------------------------------------------------
    // 範例 4：random access 跳躍 — it[n] 應為 *(begin + n*stride)
    // -----------------------------------------------------------------------
    std::cout << "begin[2] = " << begin[2]
              << " (= v[4] = 50)\n";

    // -----------------------------------------------------------------------
    // 範例 5：iterator_traits 確認類別正確
    // -----------------------------------------------------------------------
    using cat = std::iterator_traits<StrideIterator<int>>::iterator_category;
    std::cout << "我們宣告的 category 是 random_access_iterator_tag? "
              << std::boolalpha
              << std::is_same_v<cat, std::random_access_iterator_tag> << '\n';

    // -----------------------------------------------------------------------
    // LeetCode 風格示範：
    //   LC 268. Missing Number (0..n 中缺一個)
    //   假設輸入是 row-major 的 2D 結構，但只想看「每列第一個」 — 用 stride
    //   就可以避免複製成新 vector。這展示 custom iterator 的實用性：
    //   *把訪問模式包起來*，演算法不需要重寫。
    // -----------------------------------------------------------------------
    // matrix 第 0 欄 = {3, 0, 1, 2} → 4 個元素 = LC268 中的 n=4，
    //   範圍 [0,n]=[0,4] (5 個數)，已知 4 個，缺 1 個 → 答案 = 4
    int matrix[12] = {3,9,9,  0,9,9,  1,9,9,  2,9,9};   // 4 列 x 3 欄
    StrideIterator<int> col0_begin(matrix, 3);
    StrideIterator<int> col0_end  (matrix + 12, 3);

    int n = (int)std::distance(col0_begin, col0_end);   // 4
    int total_expected = n * (n + 1) / 2;               // 0+1+2+3+4 = 10
    int actual = std::accumulate(col0_begin, col0_end, 0); // 3+0+1+2 = 6
    std::cout << "missing number = " << (total_expected - actual)
              << " (期望 10 - 6 = 4)\n";

    // -----------------------------------------------------------------------
    // 範例 6 (歷史對照)：用 std::iterator 基底類別寫出來的 iterator
    //   * 編譯時可能會看到 [-Wdeprecated-declarations] 警告 (C++17 起)，
    //     檔案上方已用 pragma 抑制，僅本檔示範用。
    //   * 重點：不論用 std::iterator 還是手寫 typedef，從外面看完全等價 —
    //     iterator_traits 抓得到、STL 演算法也吃得進去.
    // -----------------------------------------------------------------------
    int arr[] = {7, 14, 21, 28, 35};
    HistoricalSimpleIter<int> hb(arr);
    HistoricalSimpleIter<int> he(arr + 5);
    std::cout << "historical iter sum = "
              << std::accumulate(hb, he, 0) << " (期望 105)\n";

    using hcat = std::iterator_traits<HistoricalSimpleIter<int>>::iterator_category;
    std::cout << "historical iter category = forward? "
              << std::is_same_v<hcat, std::forward_iterator_tag> << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：自製 iterator 必備的「五個 typedef」是哪五個？分別代表什麼？
    //    A：iterator_category（input/output/forward/bidirectional/random_access_tag 之一，
    //      告訴 STL「我能做到哪些操作」）、value_type（*it 解出來的型別）、
    //      difference_type（it1 - it2 的型別，通常 std::ptrdiff_t）、
    //      pointer（operator-> 的回傳型別，常 = value_type*）、
    //      reference（operator* 的回傳型別，常 = value_type&）。
    //      iterator_traits 透過這五個 typedef 萃取資訊，讓泛型演算法可以 tag dispatch。
    //
    //  Q2：為什麼 std::iterator (基底類別) 在 C++17 被 deprecated？
    //    A：std::iterator<Tag, T, Distance, Pointer, Reference> 雖然能省去手寫五個
    //      typedef，但模板參數順序對使用者不直觀、後三個有預設值容易誤用，
    //      多數 STL 實作其實也沒實際依賴這個基底，只把它當 typedef 倉庫。
    //      C++17 起官方標記為 deprecated，建議直接在自己的 class 裡手寫五個 typedef
    //      (清晰且永不過時)。讀舊 code 仍可能遇到，知道用法即可。
    //
    //  Q3：iterator_category 標籤錯了會怎樣？例如把只能 ++ 的標成 random_access_iterator_tag？
    //    A：編譯通過、執行可能直接 UB。因為 STL 演算法看到 random_access 標籤就
    //      會用 it+n、it[n] 做 tag dispatch，而你的 iterator 沒有這些操作 → 編譯錯，
    //      或更糟：有定義但行為錯誤 (例如 std::sort 走 introsort 的二分切割) →
    //      crash 或結果不對。標籤是「對 STL 的承諾」，要配合實際支援的 operator 寫，
    //      不要為了「啟用某個演算法」就拉高等級。
    //

    // -----------------------------------------------------------------------
    // LC 範例: LC 1672 — Richest Customer Wealth (最富顧客財富)
    // -----------------------------------------------------------------------
    // 給 m x n 矩陣 accounts,回傳「row sum 最大」的那一列的和。
    // 為何用 custom iterator:題目是 row-major 2D 表,要對「每列第 j 欄」做加總。
    // 用上面寫好的 StrideIterator 取 column-view,即可一行 std::accumulate 跑欄總和;
    // 換成 row-view 就是逐列加總。展示「自製 iterator 把訪問模式包起來」的價值。
    {
        // 4 列 x 3 欄,row-major 平鋪
        int m = 4, n = 3;
        int accounts[12] = {
            1, 2, 3,
            3, 2, 1,
            5, 5, 5,
            2, 2, 9
        };
        int best = 0;
        for (int r = 0; r < m; ++r) {
            int* row_start = accounts + r * n;
            // 對該列做 sum (普通指標就行;此處只是示範 vs column iterator 對比)
            int s = std::accumulate(row_start, row_start + n, 0);
            best = std::max(best, s);
        }
        std::cout << "LC1672 richest = " << best << " (期望 15)\n";

        // 也可用 StrideIterator 對每一欄做加總:
        for (int c = 0; c < n; ++c) {
            StrideIterator<int> col_begin(accounts + c, n);
            StrideIterator<int> col_end  (accounts + c + m * n, n);
            int col_sum = std::accumulate(col_begin, col_end, 0);
            std::cout << "  col " << c << " sum = " << col_sum << '\n';
        }
    }

    // -----------------------------------------------------------------------
    // 實戰範例:Audio Buffer 取某聲道 (interleaved sample)
    // -----------------------------------------------------------------------
    // 場景:WAV 檔的 stereo audio 是 LRLRLR... 交錯儲存,要對「左聲道做平均音量」
    // 直接走訪會跳到右聲道。用 StrideIterator(stride=2) 就能只訪問左聲道,
    // 不必把資料分離成兩個獨立 buffer (省一倍記憶體)。
    {
        // [L R L R L R L R] — stereo audio,8 個 sample
        int audio[8] = {10, -5, 20, -10, 15, -8, 25, -12};
        StrideIterator<int> left_begin(audio, 2);
        StrideIterator<int> left_end  (audio + 8, 2);
        int n = (int)std::distance(left_begin, left_end);
        int sum = std::accumulate(left_begin, left_end, 0);
        std::cout << "Audio 左聲道平均 = " << (sum / n)
                  << " (期望 (10+20+15+25)/4 = 17)\n";
    }

    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 12_custom_iterator.cpp -o 12_custom_iterator

// === 預期輸出 ===
// stride=2 走訪: 10 30 50 70
// stride=2 sum = 160 (10+30+50+70 = 160)
// 第一個 >25 = 30 (期望 30)
// begin[2] = 50 (= v[4] = 50)
// 我們宣告的 category 是 random_access_iterator_tag? true
// missing number = 4 (期望 10 - 6 = 4)
// historical iter sum = 105 (期望 105)
// historical iter category = forward? true
// LC1672 richest = 15 (期望 15)
//   col 0 sum = 11
//   col 1 sum = 11
//   col 2 sum = 18
// Audio 左聲道平均 = 17 (期望 (10+20+15+25)/4 = 17)
