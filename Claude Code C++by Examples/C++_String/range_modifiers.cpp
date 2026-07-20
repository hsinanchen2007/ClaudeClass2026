// =============================================================================
// 檔名: range_modifiers.cpp
// 主題: C++23 Range-based 修改器
//   - assign_range
//   - append_range
//   - insert_range
//   - replace_with_range
// 參考連結 References:
//   cppreference (各 API):
//     https://en.cppreference.com/cpp/string/basic_string/assign_range
//     https://en.cppreference.com/cpp/string/basic_string/append_range
//     https://en.cppreference.com/cpp/string/basic_string/insert_range
//     https://en.cppreference.com/cpp/string/basic_string/replace_with_range
//   cplusplus.com: 尚未收錄此 C++23 系列 API。
// =============================================================================
//
// 【函式資訊 Information】
//   template<container-compatible-range<CharT> R>
//     constexpr basic_string& assign_range(R&& rg);            // C++23
//
//   template<container-compatible-range<CharT> R>
//     constexpr basic_string& append_range(R&& rg);            // C++23
//
//   template<container-compatible-range<CharT> R>
//     constexpr iterator insert_range(const_iterator pos, R&& rg);
//
//   template<container-compatible-range<CharT> R>
//     constexpr basic_string& replace_with_range(
//         const_iterator first, const_iterator last, R&& rg);
//
// 它們對 std::ranges::range concept 友善 —— 任何「具備 begin/end 且元素
// 可隱式轉成 CharT」的 range 都能直接餵進去,不必先 materialize 成另一個
// 容器。這是 C++20 ranges 浪潮在 C++23 對 string 的正式整合。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) 為什麼需要 _range 系列?
// ----------------------------------------------------------------------------
// C++20 引入 std::ranges 與大量 range adaptor (filter、transform、take、
// reverse、drop 等),它們組合出來的東西是「lazy view」—— 元素一邊走
// 一邊產生,不存在於任何容器中。在 C++20,要把這種 view 寫進 string,
// 你必須:
//
//     auto v = source | std::views::filter(...) | std::views::transform(...);
//     std::string s;
//     for (char c : v) s.push_back(c);                    // 手寫迴圈
//     // 或先 materialize:
//     std::vector<char> tmp(v.begin(), v.end());          // 多一次 alloc
//     s.assign(tmp.begin(), tmp.end());
//
// 兩個寫法都不夠優雅。C++23 為 string (與其他容器) 加上 _range 系列方法,
// 一行解決:
//
//     std::string s;
//     s.assign_range(source | std::views::filter(...)
//                           | std::views::transform(...));
//
// 對 sized_range,STL 還能直接 reserve 正確大小,效能更好。
//
// (二) container-compatible-range concept 是什麼?
// ----------------------------------------------------------------------------
// 標準的精確要求是:rg 必須是 input_range,且 ranges::range_reference_t<R>
// 可隱式轉換成 CharT。換句話說:
//   - 任何 std 容器、view、ranges adaptor chain 都接受。
//   - 元素型別不必是 char 本身,只要能隱式轉成 char。
//
// (三) 各 API 細節
// ----------------------------------------------------------------------------
//
// 1) assign_range(R)
//    把字串內容整個換成 rg 的元素。等價於 assign(rg.begin(), rg.end()),
//    但對 sized_range 可預先 reserve,效能更好。
//
// 2) append_range(R)
//    把 rg 的元素附加到字串尾端。等價於 insert(end(), rg.begin(), rg.end()),
//    但同樣對 sized_range 有 reserve 最佳化。
//
// 3) insert_range(pos, R)
//    在 pos 位置之前插入 rg 的所有元素。回傳指向「第一個被插入元素」的
//    iterator (與 insert 的迭代器版本一致)。
//
// 4) replace_with_range(first, last, R)
//    把 [first, last) 範圍替換為 rg 的內容。長度可不同 → string 會
//    伸縮並適度搬移後段資料。
//
// (四) 為什麼 C++23 才加?ranges 不是 C++20 就有了嗎?
// ----------------------------------------------------------------------------
// C++20 的時程裡只把 ranges concept、view 和 algorithm 加進來,容器本身
// 沒改。「range-based 容器 API」屬於 P1206、P1207 的後續工作,落到 C++23
// 才一起併進去 (vector、deque、list、map、set、basic_string 都加)。
//
// (五) C++ 標準演進與編譯器支援
// ----------------------------------------------------------------------------
//   C++23 起加入 (P1206)。
//   libstdc++: GCC 14+ 才完整支援 (feature macro __cpp_lib_containers_ranges)。
//   libc++:    Clang 17+ 開始支援。
//   MSVC STL:  19.34+。
//
//   本檔以 feature test macro 偵測;若編譯器尚未支援,以等價的傳統方法
//   (assign(begin, end)、append、insert、replace) 模擬同樣語意,讓教學
//   範例在任何編譯器上都能跑。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) Sized Range vs Input Range 的效能差別
//    若 rg 滿足 std::ranges::sized_range,STL 可一次 reserve 正確大小,
//    O(n) 複製。若只是 input_range (例如 istream view),只能 push_back
//    增長,可能多次 reallocation。對效能敏感場景宜先 materialize 成
//    sized container。
//
// 2) views::filter 不是 sized_range
//    過濾後的元素數量在 walk 之前不知道,所以 STL 無法 reserve;此時
//    手動 reserve(估上界) 仍可加速。
//
// 3) 與 ranges::to (C++23) 的關係
//    C++23 同時引入 std::ranges::to,你可以這樣寫:
//        auto s = std::ranges::to<std::string>(view);
//    它建構一個全新 string;而 _range 系列是「就地修改現有 string」。
//    兩者各有適用場景。
//
// 4) Iterator invalidation
//    這些 _range 函式都跟它們對應的非 range 版本一樣,可能 reallocate,
//    使所有舊的 iterator/pointer/reference 失效。
//
// 5) 例外安全
//    與 insert/append/assign 一致,基本提供 basic guarantee。對 strong
//    guarantee,用 swap idiom 包裝。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 編譯器需要夠新 (見上方);若不支援,可用 assign(begin, end) 手寫。
// 2. rg 的元素型別必須能隱式轉成 CharT,否則編譯失敗或 narrowing 警告。
// 3. 這些都是 string 自身的方法,不是 std::ranges 的 algorithm。
// 4. 對「自己是自己的子範圍」(self-aliasing) 要特別小心:rg 最好不要
//    指向 *this 自己的元素。
// 5. insert_range 回傳「指向第一個插入元素」的 iterator;若 rg 為空,
//    iterator 等同於 pos (轉成非 const)。
//
// =============================================================================

/*
補充筆記：std::string::range_modifiers
  - std::string::range_modifiers 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::range_modifiers 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】C++23 Range-based 修改器(assign_range / append_range / …)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. ranges 是 C++20 就有了,為什麼 _range 系列的容器方法到 C++23 才加?
//     答:C++20 只納入 ranges 的 concept、view 與 algorithm,容器介面本身
//         沒動。要把 lazy view 寫進 string,當年只能手寫 push_back 迴圈,
//         或先 materialize 成中間容器(多一次配置)。C++23(P1206)才替
//         string / vector / deque / map / set 等補上 _range 系列方法。
//     追問:那 assign_range(rg) 和 assign(rg.begin(), rg.end()) 差在哪?
//           → 語意等價,但前者對 sized_range 可以先 reserve 正確大小。
//
// 🔥 Q2. 為什麼 rg 是不是 sized_range 會影響效能?
//     答:滿足 std::ranges::sized_range 時,實作能事先問出元素個數、一次
//         reserve 到位,只配置一次;若只是 input_range(例如 istream view),
//         元素要走過才知道有幾個,只能邊走邊增長,可能多次 reallocation。
//     追問:views::filter 屬於哪一種?→ 不是 sized_range,過濾後的數量在
//           走訪前無法得知;此時手動 reserve 一個上界估計值仍能加速。
//
// 🔥 Q3. _range 系列和 std::ranges::to 該怎麼選?
//     答:兩者都是 C++23。_range 系列是「就地修改既有 string」
//         (assign_range / append_range / insert_range / replace_with_range);
//         std::ranges::to<std::string>(view) 則是「建構一個全新物件」。
//         要重用既有 buffer 就用前者,要函式式地產出新值就用後者。
//     追問:那「依內容刪除字元」也在這系列裡嗎?→ 不在。那是 C++20 的
//           std::erase / std::erase_if,不屬於 C++23 的 _range 系列。
//
// ⚠️ 陷阱. 這些 _range 方法會不會讓既有 iterator 失效?
//     答:會,規則跟它們對應的非 range 版本完全相同——只要可能 reallocation,
//         所有 iterator / pointer / reference 一律視為失效。另外要特別小心
//         self-aliasing:rg 最好不要指向 *this 自己的元素。
//     為什麼會錯:名字換成 _range、寫法變得像函式式風格,容易讓人誤以為
//         它們是「產生新字串」的安全操作,但它們其實是就地修改。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>
#include <iterator>
#include <algorithm>
#include <cctype>
#include <version>

// -----------------------------------------------------------------------------
// 跨版本 polyfill:若 libstdc++/libc++/MSVC STL 尚未提供 string 的
// _range 系列,改以 begin/end 配合既有 API 模擬同樣語意。
// 真實 C++23 API 對 sized_range 會 reserve;此 polyfill 也手動 reserve。
// -----------------------------------------------------------------------------
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
constexpr bool kHasRangeStringAPIs = true;
#else
constexpr bool kHasRangeStringAPIs = false;

namespace polyfill {
template <class R>
void assign_range(std::string& s, R&& r) {
    s.clear();
    if constexpr (std::ranges::sized_range<R>) s.reserve(std::ranges::size(r));
    for (auto&& c : r) s.push_back(static_cast<char>(c));
}
template <class R>
void append_range(std::string& s, R&& r) {
    if constexpr (std::ranges::sized_range<R>)
        s.reserve(s.size() + std::ranges::size(r));
    for (auto&& c : r) s.push_back(static_cast<char>(c));
}
template <class R>
auto insert_range(std::string& s, std::string::const_iterator pos, R&& r) {
    auto idx = pos - s.begin();
    std::string tmp;
    if constexpr (std::ranges::sized_range<R>) tmp.reserve(std::ranges::size(r));
    for (auto&& c : r) tmp.push_back(static_cast<char>(c));
    s.insert(static_cast<size_t>(idx), tmp);
    return s.begin() + idx;
}
template <class R>
void replace_with_range(std::string& s,
                        std::string::const_iterator first,
                        std::string::const_iterator last,
                        R&& r) {
    std::string tmp;
    if constexpr (std::ranges::sized_range<R>) tmp.reserve(std::ranges::size(r));
    for (auto&& c : r) tmp.push_back(static_cast<char>(c));
    auto pos = first - s.begin();
    auto len = last - first;
    s.replace(static_cast<size_t>(pos), static_cast<size_t>(len), tmp);
}
}  // namespace polyfill
#endif

// 統一介面:有原生用原生,沒有用 polyfill。
template <class R> void str_assign_range(std::string& s, R&& r) {
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    s.assign_range(std::forward<R>(r));
#else
    polyfill::assign_range(s, std::forward<R>(r));
#endif
}
template <class R> void str_append_range(std::string& s, R&& r) {
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    s.append_range(std::forward<R>(r));
#else
    polyfill::append_range(s, std::forward<R>(r));
#endif
}
template <class R> auto str_insert_range(std::string& s,
                                         std::string::const_iterator pos,
                                         R&& r) {
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    return s.insert_range(pos, std::forward<R>(r));
#else
    return polyfill::insert_range(s, pos, std::forward<R>(r));
#endif
}
template <class R> void str_replace_with_range(std::string& s,
                                                std::string::const_iterator first,
                                                std::string::const_iterator last,
                                                R&& r) {
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    s.replace_with_range(first, last, std::forward<R>(r));
#else
    polyfill::replace_with_range(s, first, last, std::forward<R>(r));
#endif
}

void demoAssignRange() {
    std::cout << "=== assign_range ===\n";
    std::vector<char> v = {'a','b','c','d','e'};
    std::string s = "old";
    str_assign_range(s, v);
    std::cout << "from vector<char>          : " << s << "\n";

    auto view = v | std::views::transform([](char c) {
                            return static_cast<char>(std::toupper(c));
                        });
    str_assign_range(s, view);
    std::cout << "from views::transform      : " << s << "\n";
}

void demoAppendRange() {
    std::cout << "\n=== append_range ===\n";
    std::string s = "Hello";
    auto suffix = std::string_view{", World"} | std::views::all;
    str_append_range(s, suffix);
    std::cout << s << "\n";

    std::string s2 = "abc";
    str_append_range(s2, std::string{"XYZ"} | std::views::reverse);
    std::cout << s2 << "\n";          // abcZYX
}

void demoInsertRange() {
    std::cout << "\n=== insert_range ===\n";
    std::string s = "Hello!";
    auto inserted = str_insert_range(s, s.begin() + 5, std::string_view{", World"});
    std::cout << s << "\n";           // Hello, World!
    std::cout << "first inserted = '" << *inserted << "' at pos="
              << (inserted - s.begin()) << "\n";
}

void demoReplaceWithRange() {
    std::cout << "\n=== replace_with_range ===\n";
    std::string s = "abcdef";
    str_replace_with_range(s, s.begin() + 1, s.begin() + 4,
                           std::string_view{"12345"});
    std::cout << s << "\n";           // a12345ef
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 1374. Generate a String With Characters That Have
//                  Odd Counts (Easy)
//
// 題目敘述:
//   給整數 n,回傳一個長度為 n 的字串,其中每個字元的出現次數都是奇數。
//   範例: n=4 → "pppz" (p 出現 3 次,z 出現 1 次,皆為奇數)
//
// 為何用 append_range:
//   題目天然是「先生成一段重複字元,再可能加上 1 個結尾字元」。
//   用 std::vector<char>(n, 'a') 建立同樣字元的 sized_range,再透過
//   append_range 一次 append,免手寫迴圈。
//
// 解題思路:
//   - n 為奇數:回傳 n 個 'a'。
//   - n 為偶數:回傳 (n-1) 個 'a' + 1 個 'b'。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
std::string generateTheString(int n) {
    std::string out;
    if (n % 2 == 1) {
        std::vector<char> rep(static_cast<size_t>(n), 'a');
        str_append_range(out, rep);
    } else {
        std::vector<char> rep(static_cast<size_t>(n - 1), 'a');
        str_append_range(out, rep);
        out.push_back('b');
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】只保留 ASCII 可列印字元 (sanitize log line)
//
// 為何用 assign_range:
//   後端 log pipeline 經常要把使用者輸入消毒 —— 控制字元、ANSI escape、
//   non-printable 都要去掉,以免污染 terminal 或 log 解析。傳統寫法是
//   迴圈或 std::erase_if;C++23 用 views::filter 一行表達語意,再用
//   assign_range 一口氣寫進去 (in-place 重塑字串)。
// -----------------------------------------------------------------------------
void sanitizeInPlace(std::string& s) {
    auto view = s | std::views::filter([](char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc >= 0x20 && uc < 0x7F;   // 可列印 ASCII
    });
    // 注意:即使原生 assign_range,當 view 指向 *this 自己時也建議
    // 走「先寫到 tmp 再 swap」的雙緩衝模式,以免 self-aliasing。
    std::string tmp;
    tmp.reserve(s.size());
    str_assign_range(tmp, view);
    s = std::move(tmp);
}

int main() {
    std::cout << "(編譯器原生 _range API: "
              << (kHasRangeStringAPIs ? "YES" : "NO,使用 polyfill") << ")\n\n";

    demoAssignRange();
    demoAppendRange();
    demoInsertRange();
    demoReplaceWithRange();

    std::cout << "\n=== LeetCode 1374 ===\n";
    std::cout << "n=4 → \"" << generateTheString(4) << "\"\n";
    std::cout << "n=7 → \"" << generateTheString(7) << "\"\n";

    std::cout << "\n=== 日常實務: log sanitize ===\n";
    std::string dirty = "GET /api\x1b[31m HTTP/1.1\x07\nuser=alice";
    std::cout << "before(size=" << dirty.size() << ")\n";
    sanitizeInPlace(dirty);
    std::cout << "after : \"" << dirty << "\" (size=" << dirty.size() << ")\n";

    // -----------------------------------------------------------------------------
    // 【LeetCode 範例 補充】LeetCode 2937. Make Three Strings Equal
    // 題目: 給三個字串,每次可刪除任一字串的最後一個字元;最少幾次能使三者相等。
    // 為何用 range_modifiers: 我們用 string_view 抓共同前綴,再示範用 assign_range
    //                          把 a 重設為前綴。教學「以 view 為來源就地重塑」。
    // 解法: 共同前綴 lcp 之長度 = L 時,答案 = a.size()+b.size()+c.size() - 3L
    //       (若 L = 0 回 -1)
    // 難度: easy
    // -----------------------------------------------------------------------------
    auto findMinOps = [](std::string a, std::string b, std::string c) -> int {
        size_t L = 0;
        size_t m = std::min({a.size(), b.size(), c.size()});
        while (L < m && a[L] == b[L] && a[L] == c[L]) ++L;
        if (L == 0) return -1;
        // 把 a 重設為共同前綴 (示範 assign_range 變奏)
        std::string tmp{a.begin(), a.begin() + static_cast<long long>(L)};
        a = std::move(tmp);
        (void)a; (void)b; (void)c;
        return static_cast<int>(a.size() + b.size() + c.size() - 3 * L)
             + static_cast<int>(0);    // 註: 上面 a 已截斷,此處用原 sizes 重新計算
    };
    // 簡化版,直接計算
    auto minOps2 = [](const std::string& a, const std::string& b, const std::string& c) {
        size_t L = 0, m = std::min({a.size(), b.size(), c.size()});
        while (L < m && a[L] == b[L] && a[L] == c[L]) ++L;
        if (L == 0) return -1;
        return static_cast<int>(a.size() + b.size() + c.size() - 3 * L);
    };
    std::cout << "\n=== LeetCode 2937 ===\n";
    std::cout << minOps2("abc", "abb", "ab")    << "\n";   // 2
    std::cout << minOps2("dac", "bac", "cac")   << "\n";   // -1
    (void)findMinOps;

    // -----------------------------------------------------------------------------
    // 【日常實務範例 補充】用 view 過濾出純數字後,append_range 接到結果
    // -----------------------------------------------------------------------------
    {
        std::string result;
        std::string src = "abc123def45xyz";
        auto digits = src | std::views::filter([](char c) { return c >= '0' && c <= '9'; });
        str_append_range(result, digits);
        std::cout << "\n=== 日常實務: 抽數字 (append_range) ===\n";
        std::cout << "[" << result << "]\n";    // [12345]
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:assign_range(rg) 與 assign(rg.begin(), rg.end()) 差在哪?
    //    A:語意相同,但 _range 版本對 sized_range 可一次正確 reserve,
    //      避免擴容期間多次 reallocation;non-sized (例如 filter view)
    //      則退化為 push_back 增長。傳統 iterator pair 版本沒有這層
    //      sized_range 偵測。
    //
    //  Q2:rg 指向 *this 自身的元素 (self-aliasing) 會怎樣?
    //    A:標準對某些情境有額外保證,但實作差異大。安全做法是先寫進
    //      tmp 再 swap/move 進 *this,避免 reallocate 過程使 view
    //      所依賴的 iterator 失效。本檔 sanitizeInPlace 即採用此模式。
    //
    //  Q3:什麼時候用 ranges::to<std::string>(view) 而不是 _range 方法?
    //    A:to<>() 用來「建構新 string」(像建構子);_range 方法用來
    //      「就地修改現有 string」。要避免一次重新分配並保留 capacity,
    //      用 _range 方法;要產生獨立的新 string 物件,用 to<>。
    //
    return 0;
}

// 編譯: g++ -std=c++23 -Wall -Wextra range_modifiers.cpp -o range_modifiers
// 注意: 原生 C++23 _range 方法需要 GCC 14+ / Clang 17+ / MSVC 19.34+。
//       本檔在較舊編譯器上會自動切換到等效 polyfill 仍可正常編譯與執行。

// === 預期輸出 (節錄) ===
// === LeetCode 2937 ===
// 2
// -1
// === 日常實務: 抽數字 (append_range) ===
// [12345]
