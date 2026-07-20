// =============================================================================
// 檔名: insert.cpp
// 主題: std::string::insert (在指定位置插入內容)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/insert
//   - https://cplusplus.com/reference/string/string/insert/
// =============================================================================
//
// 【函式資訊 Information - 主要重載】
//   // index 版 (回傳 *this)
//   string& insert(size_type index, size_type count, char ch);
//   string& insert(size_type index, const char* s);
//   string& insert(size_type index, const char* s, size_type count);
//   string& insert(size_type index, const string& str);
//   string& insert(size_type index, const string& str,
//                  size_type s_index, size_type count = npos);
//
//   // iterator 版 (回傳 iterator,指向插入點)
//   iterator insert(const_iterator pos, char ch);
//   iterator insert(const_iterator pos, size_type count, char ch);
//   template<InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last);
//   iterator insert(const_iterator pos, std::initializer_list<char>);
//
//   // string_view 版 (C++17 起)
//   string& insert(size_type index, std::string_view sv);
//   string& insert(size_type index, std::string_view sv,
//                  size_type sv_index, size_type count = npos);
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 基本語意
//    -----------------------------------------------------------------
//    insert(pos, ...) 在 pos 指向的位置「之前」插入新內容,原本 pos 之後
//    的字元全部往後搬移。這代表插入位置的字元 *仍會保留*,只是被推到後面。
//      s = "Hi", s.insert(1, "ello") → "Hello"
//      s = "abc", s.insert(s.begin(), '*') → "*abc"
//      s = "abc", s.insert(s.end(), '!') → "abc!"  (等同 push_back)
//
// 2. 為什麼有這麼多重載?
//    -----------------------------------------------------------------
//    insert 是 string 介面中重載最多的函式 (連同 string_view 共 12+ 個)。
//    設計思路是「對應 assign / append 的所有插入來源」:
//      - 字元 + 重複次數: insert(pos, n, ch);
//      - C-string:       insert(pos, "lit");
//      - C-string + 長度: insert(pos, ptr, n);
//      - string:         insert(pos, str);
//      - string substring: insert(pos, str, sub_pos, sub_count);
//      - iterator range:  insert(it, first, last);
//      - initializer_list: insert(it, {'a','b','c'});
//      - string_view:     insert(pos, sv);
//    了解類別模式之後,就不用記每一個簽名 — 只記「我有什麼來源」即可。
//
// 3. 兩大族群:index 版 vs iterator 版
//    -----------------------------------------------------------------
//    - index 版:回傳 *this (string&),可鏈式呼叫 s.insert(0,"x").append("y");
//    - iterator 版:回傳 iterator (指向插入後的「第一個新字元」),適合
//      在演算法 (std::find + insert) 中接續操作。
//    記法:「想要鏈 string 操作 → 用 index;想要接續迭代器 → 用 iterator」。
//
// 4. 時間複雜度
//    -----------------------------------------------------------------
//    - 一律 O(N) — 因為「插入位置之後的字元」都要往後搬移;
//    - 若插入後 size > capacity,還會額外 O(N) 的 reallocation;
//    - 即使是「插在尾端」(insert at end()),iterator 版仍會走完整流程,
//      但因為沒有後續字元需要搬,實際成本接近 O(1)。push_back 仍更快。
//
// 5. 邊界與例外
//    -----------------------------------------------------------------
//    - index > size() → std::out_of_range;
//    - 插入後 size > max_size() → std::length_error;
//    - 配置失敗 → std::bad_alloc;
//    - insert(index, const char*) 傳入 nullptr → UB (不檢查);
//    - 自我插入 (insert(0, *this)) 是允許的 — 標準保證實作會處理 alias。
//
// 6. 例外安全 (Exception Safety)
//    -----------------------------------------------------------------
//    所有重載均提供 strong exception guarantee:若中途丟例外,字串內容、
//    size、capacity 全部回到呼叫前狀態。實作通常先配置新 buffer、組裝完
//    再 swap,確保 atomicity。
//
// 7. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    - 若觸發 reallocation:全部失效;
//    - 若沒觸發 reallocation:
//        - 插入點之前的迭代器/指標仍有效;
//        - 插入點之後的迭代器/指標都失效 (位置改變了);
//        - end() 一律失效。
//    結論:insert 後請假設所有迭代器都需重新取得。
//
// 8. 為什麼「迴圈中對同一字串多次 insert」是反模式?
//    -----------------------------------------------------------------
//    每次 insert 都是 O(N),N 次插入 → O(N²)。100 萬字元的字串做 100 萬
//    次 insert(0, ...) 會是 10^12 次操作 (約 16 分鐘)。正確做法:
//      - 改用 reverse + push_back + reverse;
//      - 改用 ostringstream + 一次性轉 string;
//      - 改成「先收集片段 → 算總長度 → reserve → 一次組裝」。
//
// 9. C++17 / 20 / 23 的演進
//    -----------------------------------------------------------------
//    - C++17: 加入 string_view 重載,可直接傳 string_view 作為來源。
//    - C++20: constexpr-friendly。
//    - C++23: 加入 insert_range (容器來源)、不再侷限 InputIt 概念。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) Strong Exception Guarantee 的代價
//     strong guarantee 要求「失敗時狀態完全還原」,實作策略是:
//       1. 配置新 buffer;
//       2. 把舊內容 + 新內容組合到新 buffer;
//       3. 全部成功才 swap 進去 (swap 是 noexcept);
//       4. 釋放舊 buffer。
//     這代表 insert 即使「沒實際擴容」也可能發生 reallocation
//     (取決於實作)。標準保證行為,實作可有效率優化 (in-place memmove)。
//     效能 sensitive 的 hot path 請參考實作文件。
//
// (B) 迭代器失效規則的記憶法
//     vector / string 的失效規則:
//       「reallocation → 全部失效;否則 → 修改點之後失效」
//     insert / push_back / += / append 都遵循這條規則。
//     例外:list / forward_list 的 insert 不失效任何迭代器。
//
// (C) Self-aliasing 的處理
//     s.insert(0, s) 是合法的 — 把自己插到自己前面 (s 變成 s+s)。
//     標準要求實作正確處理「source 與 destination 重疊」的情況。
//     實作通常先把參數值 cache 下來再做修改。
//
// (D) insert vs replace vs assign
//     - insert: 新增,size 變大;
//     - replace: 取代某段,size 可變大或變小;
//     - assign: 整個重設;
//     語意層次:assign > replace > insert。三者底層都會走 reallocation
//     檢查 + memmove,差別僅在「有沒有保留原內容」。
//
// (E) 反模式:用 insert 模擬 prepend
//     s.insert(0, "prefix") 看似簡潔,但會把 s 全部往後搬。若你會做
//     很多次 prepend,改用 std::deque<char> 或反向組裝再 reverse。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. index > size() 會丟 std::out_of_range。
// 2. insert 後迭代器 / 指標 / 參考可能失效(尤其 reallocation 之後)。
// 3. 在迴圈中對同一個 string 多次 insert 為 O(N²),反模式。
// 4. 例外安全: strong exception guarantee — 拋例外則狀態還原。
// 5. 含 nullptr 的 insert(index, const char*) 是 UB。
// 6. 自我插入 (insert(i, *this)) 是合法的,實作會處理 aliasing。
// =============================================================================

/*
補充筆記：std::string::insert
  - std::string::insert 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::insert 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::insert
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. insert 的時間複雜度是多少?為什麼不是 O(1)?
//     答:O(N)。插入點之後的所有字元都必須往後搬移才能空出位置;
//         若插入後 size 超過 capacity,還要額外一次 reallocation
//         (再搬一次整條字串)。插在 end() 沒有後續字元要搬,成本才接近 O(1)。
//     追問:那 s.insert(0, "x") 在迴圈裡跑 N 次是多少?→ O(N²),
//           每次 prepend 都要搬全部字元;正確做法是反向組裝後 reverse,
//           或先 reserve 再從尾端追加。
//
// 🔥 Q2. insert 之後,iterator / pointer / reference 的失效規則是什麼?
//     答:若觸發 reallocation → 全部失效;若沒有 → 插入點「之前」的仍有效,
//         插入點「之後」的全部失效(位置被推移了),end() 一律失效。
//         這條規則與 append / += / push_back 相同。
//     追問:c_str() 拿到的指標呢?→ 規則完全一樣,一併失效。
//
// 🔥 Q3. index 版和 iterator 版的 insert 差在哪?
//     答:index 版 insert(pos, ...) 回傳 *this(string&),可鏈式呼叫;
//         iterator 版 insert(it, ...) 回傳指向「第一個新插入字元」的 iterator,
//         方便接續 std::find 之類的演算法。
//     追問:index 越界會怎樣?→ index > size() 丟 std::out_of_range;
//           但 index == size() 合法,等同 append。
//
// ⚠️ 陷阱. s.insert(0, s) 把字串插到自己前面,是 UB 嗎?
//     答:不是,這是合法的 self-aliasing,標準要求實作正確處理來源與目的
//         重疊的情況(結果是 s 變成 s+s)。真正的 UB 是傳 nullptr 給
//         insert(index, const char*)——那個沒有任何檢查。
//     為什麼會錯:很多人把「memcpy 重疊是 UB」的直覺直接套到 string 成員
//         函式上,但標準對這些成員函式另有明確保證。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoInsert() {
    std::string s = "Hi";

    // (1) 在 index 1 插入字串 "ello"
    s.insert(1, "ello");
    std::cout << "(1) " << s << "\n";       // "Hello"

    // (2) 用 iterator 插入單個字元
    s.insert(s.begin(), '*');
    std::cout << "(2) " << s << "\n";       // "*Hello"

    // (3) 在尾端 (index == size()) 插入合法 → 等同 append
    s.insert(s.size(), "!!");
    std::cout << "(3) " << s << "\n";       // "*Hello!!"

    // (4) 重複插入 N 個字元
    s.insert(0, 3, '-');
    std::cout << "(4) " << s << "\n";       // "---*Hello!!"
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1480. Running Sum of 1d Array 風格小工具
// 題目背景: 工具函式 — 把整數每三位加上千分位逗號 (財報、log 顯示常用)。
// 為何用 insert: 反向掃描,每三位呼叫 insert(",") 插入。雖然每次 insert
//                是 O(N),但插入次數僅 N/3,總成本 O(N) — 可接受。
// 思路: 從右往左每隔 3 位插入逗號;處理負號特例。
// 複雜度: O(N) (N = 字串長度;插入次數 N/3,平均搬移量 N/2)。
// -----------------------------------------------------------------------------
std::string addThousandSeparator(long long n) {
    std::string s = std::to_string(n);
    int start = (s[0] == '-') ? 1 : 0;          // 跳過負號
    for (int i = static_cast<int>(s.size()) - 3; i > start; i -= 3) {
        s.insert(i, ",");
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 71. Simplify Path 的中段邏輯
// 題目: 處理 Unix 風格路徑。我們示範用 insert 在開頭加 '/',確保路徑為絕對路徑。
// 為何用 insert: insert(0, 1, '/') 是「在前面加單一字元」的標準寫法。
// -----------------------------------------------------------------------------
std::string ensureLeadingSlash(std::string p) {
    if (p.empty() || p.front() != '/') p.insert(0, 1, '/');
    return p;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】程式碼編輯器:幫一段程式碼加 indentation
// 為何用 insert: 在每一行的開頭 insert 縮排字元。code formatter / linter / 程式碼
//                重排工具的核心操作。也適用 markdown blockquote 加 ">" 之類。
// -----------------------------------------------------------------------------
std::string indentBlock(std::string text, int spaces) {
    std::string indent(spaces, ' ');
    if (text.empty()) return text;
    text.insert(0, indent);                     // 第一行
    for (size_t i = 0; i < text.size() - 1; ++i) {
        if (text[i] == '\n') {
            text.insert(i + 1, indent);         // 換行後插入縮排
            i += indent.size();                 // 跳過剛插入的部分,避免重覆處理
        }
    }
    return text;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 2810. Faulty Keyboard
// 題目: 鍵盤每次按 'i' 會把已輸入文字反轉。給輸入 s,模擬輸出。
// 為何用 insert: 對於非 'i' 的字元,於目前游標位置 insert;遇到 'i' 反轉。
//                示範 insert 配合動態建構字串的用法。
// 複雜度: 最壞 O(N^2);實務 N 小可接受。
// 難度: easy
// -----------------------------------------------------------------------------
#include <algorithm>
std::string finalString(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == 'i') std::reverse(out.begin(), out.end());
        else          out.insert(out.end(), c);
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】把長數字依國際慣例每 3 位插入空白 (SWIFT 銀行格式)
// 為何用 insert: 跟千分位類似,但分隔字元改空白,展示同樣邏輯的另一情境。
// -----------------------------------------------------------------------------
std::string groupBySpace(std::string digits, size_t group = 4) {
    if (digits.size() <= group) return digits;
    for (long long i = static_cast<long long>(digits.size()) - group; i > 0; i -= group) {
        digits.insert(static_cast<size_t>(i), " ");
    }
    return digits;
}

int main() {
    demoInsert();
    std::cout << "\n=== 千分位 ===\n";
    std::cout << addThousandSeparator(1234567890) << "\n";  // 1,234,567,890
    std::cout << addThousandSeparator(-9876)      << "\n";  // -9,876

    std::cout << "\n=== ensureLeadingSlash ===\n";
    std::cout << ensureLeadingSlash("usr/bin") << "\n";     // /usr/bin

    std::cout << "\n=== 日常實務: 加縮排 ===\n";
    std::cout << indentBlock("if (x) {\n  return 1;\n}\n", 4);

    std::cout << "\n=== LeetCode 2810 ===\n";
    std::cout << finalString("string") << "\n";       // rtsng
    std::cout << finalString("poiinter") << "\n";     // ponter

    std::cout << "\n=== 日常實務: groupBySpace ===\n";
    std::cout << groupBySpace("1234567812345678") << "\n";   // 1234 5678 1234 5678

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:insert 的 index 版回傳 *this,iterator 版回傳什麼?
    //    A:iterator 版回傳 iterator,指向「插入後的第一個新字元」 — 適合接續
    //       std::find 等演算法。index 版回傳 string& 方便鏈式呼叫
    //       (s.insert(0,"a").insert(0,"b"))。記法:鏈 string 操作用 index、
    //       接續迭代器用 iterator。
    //
    //  Q2:insert 的迭代器失效規則為何?跟 append / += 一樣嗎?
    //    A:規則跟 vector / string 其他增長類相同:若觸發 reallocation → 全部失效;
    //       否則插入點之前的還有效、插入點之後的全失效、end() 一律失效。
    //       += / append 也是這條規則。insert 後請假設所有 iterator 都需重新取得。
    //
    //  Q3:s.insert(0, "prefix") 為什麼是反模式?在迴圈裡更糟?
    //    A:每次 insert(0, ...) 要把全部字元往後搬,單次 O(N);迴圈 N 次 prepend
    //       就是 O(N²)。正確做法:反向組裝 + 最後 reverse,或改用 std::deque<char>,
    //       或先 reserve 總長後從尾端 += 接片段。標準 insert 提供 strong exception
    //       guarantee,但不是 prepend 的好選擇。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra insert.cpp -o insert

// === 預期輸出 (節錄) ===
// === LeetCode 2810 ===
// rtsng
// ponter
// === 日常實務: groupBySpace ===
// 1234 5678 1234 5678
