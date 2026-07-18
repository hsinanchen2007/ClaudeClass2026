// =============================================================================
// 檔名: rbegin_rend.cpp
// 主題: std::string::rbegin / rend (反向迭代器)
// 參考: https://en.cppreference.com/cpp/string/basic_string/rbegin
//       https://en.cppreference.com/cpp/string/basic_string/rend
//       https://cplusplus.com/reference/string/string/rbegin/
//       https://cplusplus.com/reference/string/string/rend/
// =============================================================================
//
// 【函式資訊 Information】
//   reverse_iterator       rbegin()  noexcept;
//   const_reverse_iterator rbegin()  const noexcept;
//   const_reverse_iterator crbegin() const noexcept;     // C++11
//   reverse_iterator       rend()    noexcept;
//   const_reverse_iterator rend()    const noexcept;
//   const_reverse_iterator crend()   const noexcept;     // C++11
//
// 【詳細解釋 Explanation】
//
// (1) 設計理念:用一致的「迭代器介面」做反向走訪
//   程式經常需要從尾端往前處理字串 (找最後一個分隔符、判斷副檔名、
//   tail -n、parse 反向 token...)。如果只有 begin/end,使用者得自行處理
//   下標、注意無號數的 underflow。reverse_iterator 提供「跟正向迭代器
//   一模一樣的介面」,讓所有 STL 演算法 (find、reverse、copy、accumulate)
//   能無痛地反向操作。
//
// (2) reverse_iterator 是 adapter 不是新型別
//   標準函式庫定義:
//     using reverse_iterator = std::reverse_iterator<iterator>;
//   它包裝一個底層 iterator,但「方向相反」:
//     對 reverse_iterator 做 ++ ↔ 對底層 iterator 做 --
//     對 reverse_iterator 做 -- ↔ 對底層 iterator 做 ++
//   這樣使用者不用學新介面,既有的 algorithm 都能直接套用。
//
// (3) 對應關係 (rit vs base())
//   - rbegin() 對應 end() — 解參考會給出最後一個字元 (end() - 1)
//   - rend()   對應 begin() — 是「past-the-rend」位置,不能解參考
//   - rit.base() 永遠 = rit 對應的「下一個正向位置」
//     例如 s = "ABCDE":
//       s.rbegin()    解參考是 'E',base() == s.end()
//       s.rbegin()+1  解參考是 'D',base() == s.end()-1
//       s.rend()      不可解參考,base() == s.begin()
//   為什麼要這樣 offset?因為 begin()-1 在 C++ 中沒有合法位置可以表示,
//   而 reverse_iterator 必須對應到所有 N+1 個正向位置 (含 end()),
//   所以 base() 設成「右側位置」最自然。
//
// (4) 歷史背景
//   - C++98: 只有 rbegin/rend,且非 noexcept。
//   - C++11: 加入 crbegin/crend、所有版本標記 noexcept。
//   - C++20 起:可用 ranges::reverse_view 做反向 view (s | views::reverse),
//     但 rbegin/rend 仍是底層機制。
//
// (5) 複雜度
//   全部 O(1)。記憶體面跟 iterator 完全一樣 (連續記憶體),
//   只是封裝層多一個指標的薄包裝。
//
// 【注意事項 Pay Attention】
// 1. *rbegin() 是字串最後一個字元;對空字串呼叫 rbegin() 會等於 rend(),
//    只要不解參考就合法。
// 2. 反向走訪比手寫 size-1 → 0 安全,因為避免無號數的 underflow 陷阱:
//      for (size_t i = s.size() - 1; i >= 0; --i)   // 永不結束的 BUG (i 無號)
// 3. 將 reverse_iterator 轉回正向: it.base()。注意 base() 指向「下一個位置」。
//    要拿到對應的字元位置必須 base() - 1。
// 4. 同樣會被 invalidate — reserve、insert、erase 等修改後 reverse_iterator
//    全部失效。
// 5. erase 接受 iterator,不直接接受 reverse_iterator。要刪除反向找到的
//    位置請改寫成 s.erase((it+1).base())。
//
// 【概念補充 Concept Deep Dive】
//
// ★ 為什麼 base() 要 offset 一格?
//   reverse_iterator 必須涵蓋全部 N+1 個位置:
//     正向:  begin(), b+1, b+2, ..., end()
//     反向:  rbegin(), r+1, ..., rend()
//   標準的設計是把「反向位置 i」對應到「正向位置 i+1」,即:
//     &*rit == &*(rit.base() - 1)
//   這樣 rend() 能對應到 begin() (而非 begin()-1,後者非法)。
//   結果就是:看到一個 reverse_iterator 指的字元,base() 會回到「再過去一格」。
//   常見口訣:「base 是隔壁那位,你要的字元在 base 的左邊」。
//
// ★ reverse_iterator 也是 RandomAccessIterator
//   所有 begin/end 能用的演算法 (sort、reverse、find、binary_search...)
//   reverse_iterator 都能用,且方向自動反轉。例如:
//     std::find(s.rbegin(), s.rend(), 'a')
//   會找「最後一個 'a'」(因為從尾端開始找)。
//
// ★ 與 std::reverse 的差異
//   - rbegin/rend:不修改字串,只是反向走訪。
//   - std::reverse:真的把字串內容原地反轉。
//   反向走訪通常更快 (沒有寫入),且常常不需真的反轉就能達成目的。
//
// ★ ranges::reverse_view (C++20) 對比
//   for (char c : s | std::views::reverse) { ... }
//   底層其實也是用 rbegin/rend。新語法只是更清爽。
//
// =============================================================================

/*
補充筆記：std::string::rbegin_rend
  - std::string::rbegin_rend 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::rbegin_rend 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/
#include <iostream>
#include <string>
#include <algorithm>

void demoRBeginREnd() {
    std::string s = "ABCDE";

    std::cout << "reverse traversal: ";
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        std::cout << *it;       // 'E','D','C','B','A'
    }
    std::cout << "\n";

    // 安全的反向走訪 (對比無號數 underflow 陷阱)
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        // 處理每個字元
    }

    // base() 示範
    auto rit = s.rbegin();          // 指向 'E'
    std::cout << "*rit = " << *rit << "\n";
    std::cout << "*(rit.base()-1) = " << *(rit.base() - 1) << "\n"; // 也是 'E'
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 125. Valid Palindrome (簡化版)
// 題目: 判斷字串是否為迴文 (此處示範純字母版本,未處理大小寫過濾)。
// 為何用 rbegin/rend: std::equal 配合 begin 與 rbegin 是判斷迴文的最簡寫法 —
//                     一行就能完成,讓 STL 替我們做雙端比對。
// 解題思路: equal 會逐一比對 [begin, end) 與從 rbegin 開始的對應字元。
// 複雜度: 時間 O(n)、空間 O(1)。
// -----------------------------------------------------------------------------
bool isPalindrome(const std::string& s) {
    return std::equal(s.begin(), s.end(), s.rbegin());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從尾端解析檔案的 N 行 (tail -n)
// 為何用 rbegin/rend: tail -n 5 是日常 ops 工具。逆向找 '\n' 一次走訪 N 個換行,
//                     比每次 substr 高效。在 log viewer / debug tool 很實用。
//                     用 rbegin 走訪的好處:不需要處理 size_t underflow,
//                     且 it.base() 直接拿到「該位置之後的正向 iterator」,
//                     方便切片回傳。
// -----------------------------------------------------------------------------
std::string tailLines(const std::string& text, int n) {
    int found = 0;
    auto it = text.rbegin();
    if (it != text.rend() && *it == '\n') ++it;     // 跳過尾端換行
    for (; it != text.rend(); ++it) {
        if (*it == '\n') {
            if (++found == n) {
                // it 指向的是 '\n',回傳它後面的內容
                return std::string(it.base(), text.end());
            }
        }
    }
    return text;        // 整個檔案不到 n 行
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 344. Reverse String
// 題目: 反轉一個字元陣列 (本題對 std::string 等價於 reverse(begin(), end()))。
// 為何用 rbegin/rend: 我們用 string + rbegin/rend 從尾向前複製出新字串,
//                     示範反向迭代器的最直接用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string reverseString(const std::string& s) {
    return std::string(s.rbegin(), s.rend());
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從字串尾端找最後一個非空白字元 (例如解析資料尾巴)
// 為何用 rbegin/rend: 從 rbegin 開始 find 非空白,避免「先 size-1 再 -- 處理空字串」。
// -----------------------------------------------------------------------------
std::string trimRightFast(std::string s) {
    auto rit = std::find_if(s.rbegin(), s.rend(),
                             [](char c) { return c != ' ' && c != '\t' && c != '\n'; });
    s.erase(rit.base(), s.end());
    return s;
}

int main() {
    demoRBeginREnd();

    std::cout << "\n=== LeetCode 125 (簡化版) ===\n";
    std::cout << std::boolalpha
              << isPalindrome("racecar") << "\n"     // true
              << isPalindrome("hello")   << "\n";    // false

    std::cout << "\n=== LeetCode 344 (reverseString) ===\n";
    std::cout << reverseString("hello") << "\n";    // olleh
    std::cout << reverseString("abc")   << "\n";    // cba

    std::cout << "\n=== 日常實務: tail -n 2 ===\n";
    std::cout << tailLines("line1\nline2\nline3\nline4\n", 2);

    std::cout << "\n=== 日常實務: trimRightFast ===\n";
    std::cout << "[" << trimRightFast("hello   \t\n") << "]\n";   // [hello]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:為什麼 rit.base() 要對應到「下一個正向位置」而不是同一格?
    //    A:因為 reverse_iterator 必須涵蓋 N+1 個位置 (含 rend),若 base()
    //      與 rit 對齊,rend() 會對應到非法的 begin()-1。標準採取 offset
    //      設計使 rend() 對應 begin()。口訣:你要的字元在 base() 的左邊。
    //
    //  Q2:erase 為什麼不能直接吃 reverse_iterator?如何刪除 rit 指的字元?
    //    A:erase 接受 iterator 而非 reverse_iterator。要刪除 rit 指的字元,
    //      正確寫法是 s.erase((rit + 1).base()) — (rit+1) 對應到正向往左
    //      偏一格的位置,base() 取出即得想刪的正向 iterator。
    //
    //  Q3:reverse_iterator 與 std::views::reverse (C++20) 有何差別?
    //    A:語意相同,底層機制也一樣 (views::reverse 內部就是 rbegin/rend)。
    //      views::reverse 是 lazy view,可在 ranges pipeline 中組合。需要
    //      隨機存取 (rit + n) 或傳給舊版 algorithm 時直接用 rbegin/rend。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra rbegin_rend.cpp -o rbegin_rend

// === 預期輸出 (節錄) ===
// === LeetCode 344 (reverseString) ===
// olleh
// cba
// === 日常實務: trimRightFast ===
// [hello]
