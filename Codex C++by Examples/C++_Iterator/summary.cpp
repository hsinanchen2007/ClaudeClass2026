// ============================================================================
// Iterator 章總整理：面試前一檔速讀版
// ============================================================================
//
// 【一、Iterator 是什麼】
// Iterator 是「位置抽象」：algorithm 不需知道底層是 array、linked list 或 stream，
// 只依 iterator 提供的 operations 工作。range 通常是半開區間 [first, last)：first
// 指第一項，last 指尾端後一格且不可解參考。空 range 即 first == last。
//
// 【二、可讀能力階梯，以及獨立的 output 契約】
// 可讀方向的強弱階梯如下：
// input          : 讀、++，single-pass；例 istream_iterator。
// forward        : input + multi-pass；例 forward_list、unordered_*。
// bidirectional  : forward + --；例 list、map/set。
// random-access  : +n、-n、差值、[]、次序比較，O(1) 跳躍；例 vector/deque。
// contiguous(C++20): random-access + 實體連續，可 std::to_address；例 vector/array。
//
// output iterator 是「可寫指定型別」的正交契約，不是上述 readable 階梯的一層：
// back_insert_iterator 只寫不可讀；mutable vector iterator 則同時 contiguous-readable 且
// output_iterator<int>。傳統 iterator_category 以單一 tag 近似分類，C++20 concepts 能
// 分別問 input_iterator<I> 與 output_iterator<I,T>，泛型 API 應只要求完成工作所需能力。
// 可讀階梯中越強者可交給要求越弱的 algorithm；std::sort 要 random-access，find 只需 input。
//
// 【三、常用 adapter】
// reverse_iterator : rbegin/rend 反向看 range；base() 指向反向元素的下一個正向位置。
// back_inserter    : assignment 轉成 push_back，不必預先 resize；adapter 不擁有容器。
// front_inserter   : assignment 轉成 push_front，逐次插前面會反轉輸入順序。
// inserter         : assignment 轉成 insert(hint,value)，重複 key 仍依容器規則處理。
// move_iterator    : 解參考改成 rvalue；建立 adapter 本身不搬，consumer 取值時才搬。
// stream iterator  : typed formatted I/O，受 locale/skipws/state 影響，輸入為 single-pass。
// streambuf iterator: 直接逐 char 讀寫 buffer，不做型別轉換，也不跳過 whitespace。
//
// 【四、operations 與複雜度】
// advance(it,n) 改 it；next/prev 回副本；distance 算距離。
// vector 上 advance/distance O(1)，list/forward_list 上 O(N)。在 linked-list loop
// 每輪 distance(begin,end) 可能把 O(N) 工作放大成 O(N²)。Iterator 不知道邊界，
// 越過 end 是 UB；先驗範圍或用 ranges 帶 sentinel 的 overload。
//
// 【五、invalidation 必背】
// vector reallocate -> 所有 iterator/reference/pointer 失效；erase 點之後失效。
// list/map/set      -> 通常只有被 erase node 失效。
// unordered_* rehash -> iterator 失效；既有元素 reference/pointer 仍有效。
// erase while loop -> 使用 `it = container.erase(it)`，不可 erase(it) 後再 ++it。
// 永遠以該容器該 operation 的標準表為準，deque 尤其不可用簡化直覺猜。
//
// 【六、高頻陷阱】
// 1. 解參考 end/rend、default iterator，或讓 iterator 越界：UB。
// 2. 容器修改後繼續用 stale iterator：UB，debug build 不一定抓到 release 問題。
// 3. 從 temporary container 取 begin/end：temporary statement 結束即析構，dangling。
// 4. input iterator 先 distance 再處理：第一趟可能已消耗 stream。
// 5. `reverse_iterator::base()` off-by-one：若 rit 指 x，rit.base() 指 x 後一格。
// 6. istream_iterator 遇壞 token 也變成 end；若格式錯誤不可接受，完成後必須檢查 stream state。
// 7. move_iterator 不是 transaction；中途失敗時來源可能只有前半段 moved-from。
// 8. 自訂 iterator 宣告過強 category，會讓 algorithm 的正確性/複雜度假設失真。
//
// 【七、選型表】
// 想讓 algorithm append 到 vector       -> back_inserter
// 想保留 raw 空白逐字複製 stream         -> streambuf_iterator
// 想從空白分隔文字解析 int              -> istream_iterator<int>
// 想把 unique_ptr range 轉交 ownership   -> move_iterator
// 想從尾端搜尋                          -> reverse_iterator/rbegin-rend
// 想遍歷時刪除                          -> 接住 erase 回傳的下一位置
// 想寫自訂 iterator                     -> 先定 single/multi-pass、reference、domain，再宣告 category
//
// 【八、面試快問快答】
// Q: iterator 和 pointer 相同嗎？
// A: pointer 可是一種 contiguous iterator，但一般 iterator 可能是 class/stream cursor，
//    不保證實體位址、連續性或多趟走訪。
// Q: 為何 std::lower_bound 用在 list 是線性走訪，而 set/map::lower_bound 是 O(logN)？
// A: 前者比較次數雖可為 O(logN)，但 list iterator 找中點需逐步走，總 increments O(N)；
//    std::list 本身沒有 lower_bound member。tree 容器成員能沿樹高 O(logN) 找。
// Q: `auto x = *it` 和 `auto& x = *it` 差在哪？
// A: 前者通常 copy value，後者綁 element；代理 iterator（如 vector<bool>）要特別注意
//    *it 未必真的是 T&，泛型程式可用 iter_reference_t。
// Q: moved-from range 能否再用？
// A: 元素仍有效但值未指定；可清除/賦值/析構，不可假設字串一定空。
// Q: OutputIterator 是否比 InputIterator 弱？
// A: 不是同一強弱軸；前者要求寫入 expression，後者要求讀取與 single-pass traversal。
// Q: istream_iterator<int> 為何不適合嚴格 parser？
// A: EOF 與格式失敗都讓 iterator 抵達 end；要診斷 token/位置時應逐 token 解析並查 state。
//
// 下方可執行程式涵蓋：concept/category、三種 insert adapter、reverse、custom forward、
// typed stream、streambuf、move、LeetCode 27 與安全 erase 工作案例。
// ============================================================================

/*
==============================================================================
【面試深挖：Iterators】

I1｜五種 legacy iterator category 的能力階梯？
答：input/output 是單向單遍用途；forward 可多遍；bidirectional 可 --；random access 可 O(1)
jump/difference；C++20 contiguous 再保證實體連續，可轉成 address。演算法要求的是最低能力契約。

I2｜input iterator 的「single-pass」有何實際意義？
答：複製出的 iterators 可能共享讀取狀態；推進一份可能影響另一份。istream_iterator 是典型。
因此不能先 distance 再重走，或假設保存副本即可回頭。

I3｜半開區間 [first,last) 為何成為標準？
答：空區間自然 first==last，長度是 last-first（random access），相鄰區間可無縫拼接，
end 不需指向有效元素。不可 dereference end。

I4｜iterator、reference、pointer 何時失效？
答：取決於 container 與 operation，三者不一定同步。unordered rehash 會使 iterator 失效但
元素 reference/pointer 可留存；vector reallocation 通常三者全失效。必須查具體契約。

I5｜`reverse_iterator::base()` 為何指向「下一個」位置？
答：reverse iterator r dereference 的元素是 *(r.base()-1)。這讓 [rbegin,rend) 與正向
[begin,end) 邊界一致。從 reverse search erase 元素常需 `std::next(r).base()`。

I6｜`std::distance` 一定 O(1) 嗎？
答：只有 random-access iterator 可常數相減；forward/bidirectional 需逐步走 O(n)。
`advance` 同理。演算法寫得泛型，不代表各 category 有相同成本。

I7｜`back_inserter`、`inserter`、`front_inserter` 如何選？
答：它們是 output iterator adaptor，把 assignment 轉成 container member operation。
vector 常用 back；set 用 inserter；front_inserter 會反轉逐項輸入的相對次序，要明確接受。

I8｜`move_iterator` 做了什麼？
答：dereference 時把 underlying reference 轉成 rvalue reference，使 algorithm 搬移元素。
它不自己搬，也不延長 lifetime；來源 moved-from 狀態仍由元素型別決定。

I9｜自訂 iterator 最容易錯在哪？
答：category/concept 宣告超過實際能力、post-increment 回錯型別、reference 實際是 proxy、
不同 range iterator 被比較，以及 lifetime/invalidation 未定義。C++20 iterator concepts
比只繼承 std::iterator 更能描述能力；std::iterator 已 deprecated/removed。

I10｜iterator 與 sentinel 為何在 ranges 分離？
答：結束條件不必與 iterator 同型，例如 counted input 或零結尾字串；可少存狀態並表達
無法建立「尾 iterator」的 range。演算法需接受 sentinel_for 契約。

I11｜為何 iterator invalidation 是高頻實作題？
答：典型 bug 是 range-for vector 時 push_back、erase 後仍 ++ 舊 iterator，或 unordered
插入 rehash 後沿用 iterator。正確模式要依 erase 回傳的新 iterator 或先收集修改。

I12｜contiguous iterator 是否等同 raw pointer？
答：能力上保證元素按位址連續，可由 to_address 取得 pointer；型別不必真的是 pointer。
vector/string/array iterator 通常 contiguous，deque 不是，vector<bool> proxy 也不是。
==============================================================================
*/

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(std::is_same_v<typename std::iterator_traits<std::vector<int>::iterator>::iterator_category,
                             std::random_access_iterator_tag>);
static_assert(std::is_same_v<typename std::iterator_traits<std::forward_list<int>::iterator>::iterator_category,
                             std::forward_iterator_tag>);
static_assert(std::contiguous_iterator<std::vector<int>::iterator>);
static_assert(std::output_iterator<std::vector<int>::iterator, int>);
static_assert(std::output_iterator<std::back_insert_iterator<std::vector<int>>, int>);
static_assert(!std::input_iterator<std::back_insert_iterator<std::vector<int>>>);

// 自訂 iterator：產生整數半開區間。prefix ++ 回自身、postfix ++ 回舊值，且 copies
// 可獨立前進，所以可誠實宣告 forward/multi-pass；operator* 回 value 而非假裝有 int&。
class CountingIterator {
public:
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using reference = int;
    using pointer = void;
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr explicit CountingIterator(const int value = 0) noexcept : value_(value) {}

    [[nodiscard]] constexpr int operator*() const noexcept { return value_; }

    constexpr CountingIterator& operator++() noexcept
    {
        ++value_;
        return *this;
    }

    constexpr CountingIterator operator++(int) noexcept
    {
        CountingIterator previous = *this;
        ++(*this);
        return previous;
    }

    friend constexpr bool operator==(CountingIterator, CountingIterator) = default;

private:
    int value_{};
};

static_assert(std::forward_iterator<CountingIterator>);
static_assert(!std::bidirectional_iterator<CountingIterator>);

void category_and_adapter_demo()
{
    const std::vector<int> source{1, 2, 3};
    std::vector<int> appended;
    std::copy(source.begin(), source.end(), std::back_inserter(appended));
    assert(appended == source);

    std::deque<int> prepended;
    std::copy(source.begin(), source.end(), std::front_inserter(prepended));
    assert((prepended == std::deque<int>{3, 2, 1}));

    std::set<int> unique;
    const std::vector<int> with_duplicate{3, 1, 3, 2};
    std::copy(with_duplicate.begin(), with_duplicate.end(), std::inserter(unique, unique.end()));
    assert((unique == std::set<int>{1, 2, 3}));

    std::list<int> linked{10, 20, 30};
    auto middle = linked.begin();
    std::advance(middle, 1); // O(1 step)，一般 n 是 O(n)。
    assert(*middle == 20);
    assert(*std::next(linked.begin(), 2) == 30);
    assert(*std::prev(linked.end()) == 30);

    const auto reverse_first = source.rbegin();
    assert(*reverse_first == 3);
    assert(reverse_first.base() == source.end()); // base 指 3 的下一格。
}

void custom_iterator_demo()
{
    CountingIterator first(4);
    const CountingIterator last(7);
    const CountingIterator independent = first;
    ++first;
    assert(*first == 5 && *independent == 4); // multi-pass copy 不被另一份前進牽動。

    std::vector<int> generated;
    std::copy(independent, last, std::back_inserter(generated));
    assert((generated == std::vector<int>{4, 5, 6}));
}

void stream_and_move_iterator_demo()
{
    // istream_iterator<int> 做 formatted extraction；壞 token 與 EOF 都會使它等於 end。
    std::istringstream typed_input("10 20 30");
    const std::istream_iterator<int> typed_first(typed_input);
    const std::istream_iterator<int> typed_last;
    const std::vector<int> parsed(typed_first, typed_last);
    assert((parsed == std::vector<int>{10, 20, 30}));

    std::ostringstream typed_output;
    std::copy(parsed.begin(), parsed.end(), std::ostream_iterator<int>(typed_output, ","));
    assert(typed_output.str() == "10,20,30,"); // delimiter 也會出現在最後一項後。

    // streambuf iterator 不套 skipws；空白與換行逐 char 保留。
    std::istringstream raw_input("A B\nC");
    const std::string raw{std::istreambuf_iterator<char>(raw_input),
                          std::istreambuf_iterator<char>()};
    assert(raw == "A B\nC");
    std::ostringstream raw_output;
    std::ostreambuf_iterator<char> raw_destination(raw_output);
    const auto raw_end = std::copy(raw.begin(), raw.end(), raw_destination);
    assert(!raw_end.failed());
    assert(raw_output.str() == raw);

    // make_move_iterator 只改解參考 value category；copy 消費時才移動 ownership。
    std::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(7));
    source.push_back(std::make_unique<int>(9));
    std::vector<std::unique_ptr<int>> destination;
    destination.reserve(source.size());
    std::copy(std::make_move_iterator(source.begin()),
              std::make_move_iterator(source.end()),
              std::back_inserter(destination));
    assert(destination.size() == 2U);
    assert(*destination[0] == 7 && *destination[1] == 9);
    assert(source[0] == nullptr && source[1] == nullptr); // unique_ptr 的移後狀態有明文保證。
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 27. Remove Element（移除元素）
// 題目：原地移除指定值並回傳保留長度；例如 [0,1,2,2,3,0,4,2] 移除 2 後前五項為 [0,1,3,0,4]。
// 為何使用本章主題：remove 產生 logical end iterator，erase 以該邊界完成實體刪除並示範失效契約。
// 思路：以 remove 壓縮非 target 元素；保存 logical_end；erase 尾段；回傳新 size。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為 vector 長度。
// 易錯點：std::remove 不縮短容器；erase 後 logical_end 不可再用；題目只保證前 k 項內容有意義。
// -----------------------------------------------------------------------------
int remove_element(std::vector<int>& values, int target)
{
    const auto logical_end = std::remove(values.begin(), values.end(), target);
    values.erase(logical_end, values.end()); // physical erase 才改 size。
    return static_cast<int>(values.size());
}

void leetcode_demo()
{
    std::vector<int> values{0, 1, 2, 2, 3, 0, 4, 2};
    assert(remove_element(values, 2) == 5);
    assert((values == std::vector<int>{0, 1, 3, 0, 4}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】安全淘汰過期的向量 session
// 情境：每輪掃描 session，TTL<=0 者刪除，其他 TTL 減一，並保持剩餘 session 原順序。
// 為何使用本章主題：vector erase 會使刪除點後方 iterator 失效，必須接住其回傳的新 iterator 才能繼續。
// 設計：用手動 iterator 迴圈；過期就 it=erase(it)；存活就更新 TTL 後 ++it；最後驗證順序。
// 成本：最壞時間 O(S^2)、額外空間 O(1)，S 為 session 數，因每次 erase 可能搬移後方元素。
// 上線注意：大量淘汰應用 erase_if 降至 O(S)；迴圈內不得保存元素 reference，並行存取需外部同步。
// -----------------------------------------------------------------------------
struct Session {
    std::string id;
    int ttl{};
};

void expire_sessions(std::vector<Session>& sessions)
{
    for (auto it = sessions.begin(); it != sessions.end();) {
        if (it->ttl <= 0) {
            it = sessions.erase(it);
        } else {
            --it->ttl;
            ++it;
        }
    }
}

void practical_demo()
{
    std::vector<Session> sessions{{"A", 2}, {"B", 0}, {"C", 1}};
    expire_sessions(sessions);
    assert(sessions.size() == 2);
    assert(sessions[0].id == "A" && sessions[0].ttl == 1);
    assert(sessions[1].id == "C" && sessions[1].ttl == 0);
    std::cout << "[實務] stale sessions removed with erase-return iterator\n";
}

int main()
{
    category_and_adapter_demo();
    custom_iterator_demo();
    stream_and_move_iterator_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Iterator summary: all checks passed\n";
}

// 最後複習練習：
// 1. 說明為何 `for (it...) { v.push_back(...); use(*it); }` 可能 UB。
// 2. 實作只接受 std::forward_iterator 的 contains template，拒絕 istream_iterator。
// 3. 用 reverse iterator 找最後一個符合條件的元素，正確換回正向 iterator。
// 4. 將 typed input 改成 "1 2 bad 3"，在 iterator 抵達 end 後分辨 eofbit 與 failbit。
// 5. 替 CountingIterator 加不同型別 sentinel，但不要虛報 random-access 能力。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Iterator_summary' && '/tmp/codex_cpp_C_Iterator_summary'
//
// === 預期輸出（節錄）===
// Iterator summary: all checks passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
