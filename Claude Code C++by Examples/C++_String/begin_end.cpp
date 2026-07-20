// =============================================================================
// 檔名: begin_end.cpp
// 主題: std::string::begin / end (正向迭代器)
// 參考: https://en.cppreference.com/cpp/string/basic_string/begin
//       https://en.cppreference.com/cpp/string/basic_string/end
//       https://cplusplus.com/reference/string/string/begin/
//       https://cplusplus.com/reference/string/string/end/
// =============================================================================
//
// 【函式資訊 Information】
//   iterator       begin()  noexcept;
//   const_iterator begin()  const noexcept;
//   const_iterator cbegin() const noexcept;     // C++11
//   iterator       end()    noexcept;
//   const_iterator end()    const noexcept;
//   const_iterator cend()   const noexcept;     // C++11
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:讓字串成為「真正的 STL 容器」
//   STL 的核心抽象是「容器 + 迭代器 + 演算法」三合一。
//   只要容器提供 begin() / end(),所有 <algorithm> 中的演算法
//   (sort、find、reverse、count、transform、accumulate、unique、partition...)
//   都能對它運作,且寫法與 vector、list、deque、array 完全一致。
//   begin() / end() 就是這個契約的實作。它們把 std::string「升級」為
//   一個可被泛型程式碼處理的字元序列。
//
// (2) 半開區間 [begin, end) 的設計巧思
//   - end() 不是「最後一個元素」,而是「最後一個元素的下一個位置」。
//   - 為什麼這樣設計?
//     a. 空容器:begin() == end(),不需特殊處理。
//     b. 迴圈條件統一:while (it != end()) 永遠正確。
//     c. 區間長度直接相減:end() - begin() == size()。
//     d. 與 C 陣列指標的 [first, last) 慣例一致。
//   - past-the-end 迭代器不可解參考 (*end() 是 UB)。
//
// (3) 迭代器分類 (Iterator Category)
//   std::string::iterator 屬於 RandomAccessIterator,功能最強:
//     - 解參考: *it, it[n], it->member
//     - 雙向移動: ++it, --it, it++, it--
//     - 跳躍: it + n, it - n, n + it, it += n, it -= n
//     - 距離: it1 - it2 → ptrdiff_t
//     - 比較: ==, !=, <, <=, >, >=
//   這代表所有需要 RandomAccess 的演算法 (std::sort、二分搜、
//   std::nth_element 等) 都可用。
//
// (4) iterator vs const_iterator vs cbegin/cend
//   - 對 non-const string,begin() 回傳 iterator,可寫入。
//   - 對 const string,begin() 回傳 const_iterator,不可寫入。
//   - cbegin()/cend() 是 C++11 加入的,永遠回傳 const_iterator,
//     不論 string 本身是否 const。常用來明確表達「我不會改它」。
//
// (5) 歷史背景
//   - C++98: 只有 begin/end 兩個版本 (depending on const)。
//   - C++11: 加入 cbegin/cend、noexcept。
//   - C++11 後 std::begin(s)、std::end(s) 自由函式版本可一致呼叫所有容器與
//     C 陣列,範圍 for 迴圈底層就是用這個機制。
//   - C++20 起 ranges:: 機制讓 ranges::begin / ranges::end 更強。
//
// (6) 複雜度與實作
//   全部 O(1)。在 libstdc++、libc++、MSVC 上,iterator 通常就是
//   pointer 的 wrapper (debug 模式可能變成 checked iterator,
//   多帶一些檢查資訊),release 模式幾乎零開銷。
//
// 【注意事項 Pay Attention】
// 1. 任何「重新配置記憶體」的操作 (reserve、insert、append、+=、resize 等)
//    都可能讓所有迭代器失效 (iterator invalidation)。
// 2. erase 會讓「被刪除位置之後」的所有迭代器失效。
// 3. cbegin/cend 一定回傳 const_iterator,即使 string 不是 const,
//    用於明確表達「不會修改」。
// 4. 與 STL <algorithm> 的所有函式相容,例如 std::reverse、std::sort、std::find。
// 5. 切勿混用不同 string 的 iterator (例如 a.end() 跟 b.begin() 比較),
//    結果未定義。
// 6. it1 - it2 在 it1 < it2 時會回傳負數 (ptrdiff_t 是有號型別)。
//
// 【概念補充 Concept Deep Dive】
//
// ★ Iterator Categories (從弱到強)
//   1. InputIterator       單向、單次走訪 (例:istream_iterator)
//   2. OutputIterator      只能寫,不能讀 (例:back_inserter)
//   3. ForwardIterator     單向、可多次走訪
//   4. BidirectionalIterator 可前可後 (例:list、map)
//   5. RandomAccessIterator 任意跳躍 (例:vector、string、deque、array)
//   6. ContiguousIterator (C++17/20) 記憶體連續 (例:vector、string、array)
//   std::string 的 iterator 是 ContiguousIterator,保證底層記憶體連續,
//   等同於 pointer 的所有功能。
//
// ★ iterator 與 pointer 的關係
//   在連續記憶體容器 (vector、string、array) 中,iterator 概念上等同
//   pointer。許多實作中 iterator 確實只是 char* 的薄包裝。但「是」與
//   「等同」是兩回事:標準不保證 iterator 真的是 pointer,寫
//   `char* p = s.begin();` 不可移植,該寫 `auto it = s.begin();`。
//   要拿 raw pointer 用 &*it 或 s.data()。
//
// ★ 範圍 for 的去糖化 (range-based for desugar)
//   for (auto& c : s) { body }
//   等同於:
//     auto&& __r = s;
//     for (auto __it = std::begin(__r), __e = std::end(__r);
//          __it != __e; ++__it) {
//       auto& c = *__it;
//       body
//     }
//   所以任何提供 begin/end 的型別都能用範圍 for。
//
// ★ Iterator Invalidation 的具體規則 (string)
//   - reserve / shrink_to_fit / 任何讓 capacity 改變的操作 → 全部失效
//   - operator+= / append / push_back → 若觸發 realloc 就全部失效
//   - insert / erase / replace → 修改點之後 (含) 全部失效
//   - clear → 不一定失效 (capacity 通常不變),但 end() 可能變
//   保險作法:任何修改後重新取得 iterator。
//
// =============================================================================

/*
補充筆記：std::string::begin_end
  - std::string::begin_end 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::begin_end 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::begin / end (正向迭代器)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::string 的 iterator 什麼時候失效?請完整列一次。
//     答：任何可能重新配置緩衝區或改變長度的操作都會失效 ——
//         operator+=、append、push_back、insert、erase、replace、resize、
//         reserve、shrink_to_fit、assign、operator=、clear、swap。
//         只讀操作 (size、find、compare、const operator[]、substr) 不會。
//     追問：c_str() / data() 回傳的指標也一樣嗎?
//         → 是,規則完全相同。這是實務上更常踩的版本。
//
// 🔥 Q2. begin()/end() 和 cbegin()/cend() 差在哪?該用哪個?
//     答：cbegin/cend (C++11) 無論物件本身是不是 const,都回傳
//         const_iterator;begin/end 則看物件的 const 性 —— 非 const 物件
//         得到 iterator,const 物件得到 const_iterator。不需要修改時用
//         cbegin/cend,可以把唯讀意圖寫在型別上,也避免在 auto 推導下
//         不小心拿到可寫的 iterator。
//
// Q3. 為什麼 C++11 要求「非 const 的 begin() 不得使 iterator 失效」?
//     答：這正是 COW (copy-on-write) 實作在 C++11 之後不再合法的原因之一。
//         COW 下多個 string 共享同一塊 buffer,一旦有人索取可寫的 iterator
//         就必須「分家」複製,而分家一定會讓既有的指標與 iterator 失效 ——
//         這與新標準的要求直接衝突。結果是現代實作全面改用 SSO + eager copy。
//
// ⚠️ 陷阱. 下面這段刪除字元的迴圈錯在哪?
//         for (auto it = s.begin(); it != s.end(); ++it)
//             if (*it == 'x') s.erase(it);
//     答：erase 之後 it 已經失效,接下來的 ++it 與解參考都是 UB。
//         正確寫法是 it = s.erase(it) (erase 回傳下一個有效位置,且該輪
//         不要再 ++it);C++20 更簡單:std::erase(s, 'x')。
//     為什麼會錯：以為 erase 只是「拿掉一個元素、其他不動」。對連續儲存
//         的容器而言,被刪位置之後的元素全部往前搬,該位置起的所有
//         iterator 都失效。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <climits>

void demoBeginEnd() {
    std::string s = "Hello";

    // (1) 範圍 for 底層也是用 begin/end
    for (auto it = s.begin(); it != s.end(); ++it) {
        std::cout << *it;
    }
    std::cout << "\n";

    // (2) 用標準演算法
    std::reverse(s.begin(), s.end());
    std::cout << "reversed: " << s << "\n";

    // (3) cbegin / cend
    for (auto it = s.cbegin(); it != s.cend(); ++it) {
        // *it = 'X';   // 編譯失敗 — const_iterator 不能寫入
        (void)*it;
    }

    // (4) RandomAccess 算術
    auto it = s.begin() + 2;            // 第 3 個字元
    std::cout << "begin+2 = " << *it << "\n";
    std::cout << "distance = " << (s.end() - s.begin()) << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String
// 題目: 原地反轉字元陣列。
// 為何用 begin/end: 這是最自然示範「迭代器配演算法」的題目 —
//                   一行 std::reverse(s.begin(), s.end()) 就解完。
// 解題思路: 演算法用對立 pair (begin/end) 雙指針互換,內部 O(n) 時間。
// 複雜度: 時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
void reverseString(std::string& s) {
    std::reverse(s.begin(), s.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】簡單的字串雜湊 (DJB2)
// 為何用 begin/end: 配合 std::accumulate 寫成函數式風格。
//                   在快取 key、字典分桶、簡易雜湊表時常用。
//                   begin/end 一交給 accumulate,就是「對所有字元逐一累積」
//                   的標準慣用法,程式短、語意明確、易於測試。
// -----------------------------------------------------------------------------
#include <numeric>
unsigned long djb2Hash(const std::string& s) {
    return std::accumulate(s.begin(), s.end(), 5381UL,
        [](unsigned long h, char c) {
            return ((h << 5) + h) + static_cast<unsigned char>(c); // h*33 + c
        });
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1119. Remove Vowels from a String (Easy)
// 題目: 把字串中所有的母音 a/e/i/o/u 全部移除。
// 為何用 begin/end: 用 std::remove_if + erase 是經典 erase-remove idiom,完全
//                   靠 begin()/end() 標出範圍。教學 STL 演算法與迭代器的搭配。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string removeVowels(std::string s) {
    auto isVowel = [](char c) {
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
    };
    s.erase(std::remove_if(s.begin(), s.end(), isVowel), s.end());
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】統計字串中某字元的出現次數
// 為何用 begin/end: std::count 直接接 begin/end,寫法乾淨,O(N) 完成。
// 場景: log 統計、CSV 欄位數推算 (數 ',' 個數 + 1)。
// -----------------------------------------------------------------------------
size_t countChar(const std::string& s, char c) {
    return std::count(s.begin(), s.end(), c);
}

int main() {
    demoBeginEnd();

    std::cout << "\n=== LeetCode 344 ===\n";
    std::string s = "hello";
    reverseString(s);
    std::cout << s << "\n";          // olleh

    std::cout << "\n=== LeetCode 1119 ===\n";
    std::cout << removeVowels("leetcodeisacommunityforcoders") << "\n";   // ltcdscmmntyfrcdrs
    std::cout << removeVowels("aeiou") << "\n";                           // (empty)

    std::cout << "\n=== 日常實務: DJB2 hash ===\n";
    std::cout << "hash(\"hello\") = " << djb2Hash("hello") << "\n";
    std::cout << "hash(\"world\") = " << djb2Hash("world") << "\n";

    std::cout << "\n=== 日常實務: count char ===\n";
    std::cout << "commas in \"a,b,c,d\" = " << countChar("a,b,c,d", ',') << "\n";  // 3

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::string 的 begin() / end() 屬於哪一種 iterator category?
    //    A：RandomAccessIterator,功能最強的一種:支援 ++ -- + - [] <
    //       與 distance 相減。代表所有需要 random access 的演算法 (std::sort、
    //       std::nth_element、std::lower_bound、std::partition 等) 都能對
    //       string 直接運作。
    //
    //  Q2：begin() / end() 在 C++11 之後一律 noexcept 嗎?
    //    A：是。C++11 起所有 STL 容器的 begin/end/cbegin/cend/rbegin/rend
    //       全部 noexcept,因為它們只回傳指標或 wrapper,絕不會 throw。
    //       這保證可在 noexcept 函式或 destructor 內安全使用。
    //
    //  Q3：為什麼是「半開區間 [begin, end)」而非閉區間 [begin, end]?
    //    A：四個原因:(1) 空容器 begin == end,不需特殊處理;(2) 迴圈條件
    //       it != end() 永遠正確;(3) 區間長度 = end - begin = size();
    //       (4) 與 C 陣列指標慣例 [first, last) 一致。但代價是 *end() 是
    //       UB,不可解參考。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra begin_end.cpp -o begin_end

// === 預期輸出 (節錄) ===
// === LeetCode 1119 ===
// ltcdscmmntyfrcdrs
//
// === 日常實務: count char ===
// commas in "a,b,c,d" = 3
