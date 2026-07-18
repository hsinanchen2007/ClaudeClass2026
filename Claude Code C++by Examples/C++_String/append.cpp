// =============================================================================
// 檔名: append.cpp
// 主題: std::string::append (在尾端附加多種來源的內容)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/append
//   - https://cplusplus.com/reference/string/string/append/
// =============================================================================
//
// 【函式資訊 Information - 主要重載】
//   string& append(size_type count, char ch);
//   string& append(const string& str);
//   string& append(const string& str, size_type pos, size_type count = npos);
//   string& append(const char* s, size_type count);
//   string& append(const char* s);
//   template<class InputIt>
//   string& append(InputIt first, InputIt last);
//   string& append(std::initializer_list<char> ilist);
//   string& append(std::string_view sv);                  // C++17
//
// 全部都回傳 *this (string&),所以可以鏈式呼叫:s.append("a").append("b");
//
// 【詳細解釋 Explanation】
//
// 一、設計理念
//   append 是「把另一段字元序列接到尾端」的標準介面。它與 operator+= 在功能
//   上幾乎完全重疊,但 append 提供了「更多重載」── 特別是支援:
//     - substring 子字串 (str, pos, count)
//     - 從 char* 指定長度 (s, count) 可包含內嵌 '\0'
//     - iterator 範圍 (first, last)
//     - count 個重複字元 (n, ch)
//   這些是 operator+= 沒有的;故當需要「子片段」或「重複 N 個」時,只能用 append。
//
// 二、底層運作
//   append 等價於 insert(size(), ...) 但實作上更直接(不用搬移既有字元)。
//   其流程大致是:
//     1. 計算新的 size 與所需 capacity。
//     2. 若 capacity 不足 → reallocate (通常是「2 倍」或「1.5 倍」成長策略,
//        實作而定),把舊資料搬到新緩衝區。
//     3. 把新內容拷貝到原 size 之後的位置。
//     4. 更新 size、寫入結尾的 '\0' (string 內部一定保留結尾 null)。
//
// 三、與其他函式的關係
//   - operator+=  : 是 append 的子集 (語法糖);可讀性較好,但功能受限。
//   - push_back   : 只新增「單一」字元 (相當於 append(1, ch));逐字元時用。
//   - insert      : append 是 insert 在「結尾」位置的特化,效能更好。
//   - operator+   : 非成員函式,會建立新字串 (含一次配置 + 兩次拷貝),
//                   迴圈裡千萬不要用,會變 O(N^2)。
//
// 四、版本演進
//   - C++03  : 提供基本重載。
//   - C++11  : 加入 initializer_list 與 noexcept 標註。
//   - C++17  : 加入 string_view 重載 (避免 const char*+size 二參版本)。
//   - C++20  : 加入 constexpr (允許 constant-evaluation 中使用)。
//   - C++23  : append_range (從任何 range 附加,需 <ranges>)。
//
// 時間複雜度: O(M),M 是被附加內容長度。若不觸發 reallocation 為純拷貝;
//             觸發 reallocation 時為 O(N+M),但「攤銷 (amortized)」仍是 O(M)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 攤銷 O(N) 與成長策略 (Amortized analysis)
//   string 採用「幾何成長」(通常 capacity 翻倍或 1.5 倍)。意思是:
//   逐字元 append N 次,雖然「個別」一次 append 在 reallocation 時是 O(現有長度),
//   但「平均」每次仍只花 O(1) → 總時間是 O(N) 而非 O(N^2)。
//   這個技巧叫「Amortized Analysis (攤銷分析)」,vector 也是一樣的原理。
//
// (B) 何時會 reallocation?
//   只在 size() + 新增長度 > capacity() 時才會。
//   實務上若已知最終長度,先 reserve(estimate) 可避免多次搬家:
//     std::string out;
//     out.reserve(1024);             // 提前配置一次
//     for (...) out.append(token);   // 不會再 realloc
//
// (C) self-append 安全性
//   標準保證 a.append(a) 是合法的;實作上會先把參數內容快照,
//   或者在 realloc 之前完成讀取。新手常擔心的「邊讀邊寫」其實標準已處理。
//
// (D) iterator 範圍的特殊性
//   template<class InputIt> append(InputIt, InputIt) 接受任何符合 InputIterator
//   概念的迭代器。若是 RandomAccessIterator (例如 vector<char>::iterator),
//   實作可一次計算長度並 reserve;若是 InputIterator (例如 istream_iterator),
//   只能逐字附加,效能較差。
//
// 【注意事項 Pay Attention】
// 1. append 後迭代器 / 指標 / 參考可能失效(若觸發 reallocation)。
// 2. 若 string& str 與 *this 是同一個物件 (a.append(a)),仍為合法 (self-append)。
// 3. 大量 append 前先 reserve,可顯著提升效能(避免多次成長與搬家)。
// 4. append(const char*, count) 允許包含 '\0' 的 buffer (二進位資料友善)。
// 5. 不要在 hot loop 中混用 operator+ 與 append:operator+ 會產生暫時物件。
// 6. append 提供 strong exception guarantee:若拋出例外,字串保持原狀。
//
// =============================================================================

/*
補充筆記：std::string::append
  - std::string::append 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::append 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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
#include <vector>

void demoAppend() {
    std::string s = "Hi";
    s.append(", World");                            // const char*
    std::cout << "(1) " << s << "\n";

    s.append(3, '!');                               // 重複字元
    std::cout << "(2) " << s << "\n";

    std::string ext = "[ABCDEFG]";
    s.append(ext, 1, 3);                            // 取 ext 的 [1,4) → "ABC"
    std::cout << "(3) " << s << "\n";

    std::vector<char> v = {'X', 'Y', 'Z'};
    s.append(v.begin(), v.end());                   // 從 iterator
    std::cout << "(4) " << s << "\n";

    s.append({'.', '.', '.'});                      // initializer_list
    std::cout << "(5) " << s << "\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 14. Longest Common Prefix
// 題目: 給一組字串,找出它們共同的最長前綴。例如 ["flower","flow","flight"] → "fl"。
// 為何用 append: 一邊比對字元,確認所有字串該位置相同,就 append 一個字元到結果。
// 思路: 以第一個字串的字元為基準,逐欄比對。若某字串提前結束或字元不同 → 停止。
// 複雜度: O(S),S 是所有字元總數。
// -----------------------------------------------------------------------------
#include <vector>
#include <algorithm>
std::string longestCommonPrefix(const std::vector<std::string>& strs) {
    std::string prefix;
    if (strs.empty()) return prefix;
    for (size_t i = 0; ; ++i) {
        if (i >= strs[0].size()) break;       // 第一個字串走完
        char ch = strs[0][i];
        for (size_t k = 1; k < strs.size(); ++k) {
            if (i >= strs[k].size() || strs[k][i] != ch) {
                return prefix;                 // 不一致就停
            }
        }
        prefix.append(1, ch);                  // 全部相同 → 收進 prefix
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 67. Add Binary
// 題目: 兩個二進位字串相加,結果亦為字串。
// 為何用 append: 反向產出每位數字後 append,最後 reverse。
// 複雜度: O(max(N1, N2))。
// -----------------------------------------------------------------------------
std::string addBinary(const std::string& a, const std::string& b) {
    std::string res;
    int i = static_cast<int>(a.size()) - 1;
    int j = static_cast<int>(b.size()) - 1;
    int carry = 0;
    while (i >= 0 || j >= 0 || carry) {
        int x = (i >= 0 ? a[i--] - '0' : 0);
        int y = (j >= 0 ? b[j--] - '0' : 0);
        int sum = x + y + carry;
        res.append(1, static_cast<char>('0' + sum % 2));
        carry = sum / 2;
    }
    std::reverse(res.begin(), res.end());
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】建立 HTTP query string
// 為何用 append: 把 key=value 用 '&' 串起來,append 每個區段。
//                呼叫 REST API、組裝 URL 時的標準操作。
//                (簡化版未做 URL encoding;實務應加 percent-encoding)
// -----------------------------------------------------------------------------
std::string buildQueryString(const std::vector<std::pair<std::string, std::string>>& kv) {
    std::string out;
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i > 0) out.append("&");
        out.append(kv[i].first);
        out.append("=");
        out.append(kv[i].second);
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 3】LeetCode 1108. Defanging an IP Address
// 題目: 把 "1.1.1.1" 變成 "1[.]1[.]1[.]1"。
// 為何用 append: 逐字檢查,遇到 '.' 就 append "[.]"、其它字元用 append(1, ch)。
//                完美示範「逐 token 拼接」這個 append 最常見的用法。
// 思路: 遍歷一次原字串,reserve 後 append。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string defangIPaddr(const std::string& address) {
    std::string out;
    out.reserve(address.size() + 6);     // 4 個 '.' 各加 2 個字元
    for (char c : address) {
        if (c == '.') out.append("[.]");
        else          out.append(1, c);
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用 append 組裝 SQL IN 子句的 placeholder
// 為何用 append: prepared statement 需要 "?,?,?,?" 這種長度動態的 placeholder
//                字串;最自然就是迴圈 append。注意第一個前面不加逗號。
// 場景: 後端常用 (例如查多個 user_id),需要產生 "WHERE id IN (?,?,?)"。
// -----------------------------------------------------------------------------
std::string buildInClausePlaceholders(size_t n) {
    std::string out;
    out.reserve(n * 2);                  // 每個 placeholder 約 2 字元 (?,)
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) out.append(",");
        out.append("?");
    }
    return out;
}

int main() {
    demoAppend();

    std::cout << "\n=== LeetCode 14 ===\n";
    std::cout << longestCommonPrefix({"flower", "flow", "flight"}) << "\n";  // "fl"
    std::cout << "[" << longestCommonPrefix({"dog", "racecar", "car"}) << "]\n"; // 空

    std::cout << "\n=== LeetCode 67 ===\n";
    std::cout << addBinary("11", "1")     << "\n";  // "100"
    std::cout << addBinary("1010", "1011")<< "\n";  // "10101"

    std::cout << "\n=== LeetCode 1108 ===\n";
    std::cout << defangIPaddr("1.1.1.1")        << "\n";  // 1[.]1[.]1[.]1
    std::cout << defangIPaddr("255.100.50.0")   << "\n";  // 255[.]100[.]50[.]0

    std::cout << "\n=== 日常實務: query string ===\n";
    std::cout << buildQueryString({{"q", "cpp"}, {"page", "2"}, {"sort", "date"}}) << "\n";

    std::cout << "\n=== 日常實務: SQL IN placeholders ===\n";
    std::cout << "WHERE id IN (" << buildInClausePlaceholders(3) << ")\n"; // (?,?,?)
    std::cout << "WHERE id IN (" << buildInClausePlaceholders(5) << ")\n"; // (?,?,?,?,?)

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：append 與 operator+= 的差別在哪?哪個比較快?
    //    A：兩者效能完全相同 (operator+= 通常就是呼叫 append 的單一參數版本)。
    //       差別在 append 提供更多重載 (substring、iterator range、count+ch)。
    //       單純串接用 += 較簡潔;需要從子片段、迭代器範圍、重複字元附加時
    //       才必須用 append。
    //
    //  Q2：在迴圈中大量 append,如何避免反覆 reallocation 拖垮效能?
    //    A：呼叫 reserve(estimate) 預先擴大 capacity。geometric growth 雖然
    //       是攤銷 O(1),但每次 reallocate 仍要搬移所有舊字元;若已知最終
    //       長度,reserve 可把總成本從 O(N log N) 拷貝降為一次 O(N)。
    //
    //  Q3：a.append(a) 自我附加會不會邊讀邊寫造成 UB?
    //    A：不會。標準明文要求 append 必須對 self-aliasing 安全,實作通常
    //       先快照長度或在 reallocate 前完成讀取。但 append 仍會使所有舊
    //       iterator/pointer 失效 (若觸發 reallocation)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra append.cpp -o append

// === 預期輸出 ===
// (1) Hi, World
// (2) Hi, World!!!
// (3) Hi, World!!!ABC
// (4) Hi, World!!!ABCXYZ
// (5) Hi, World!!!ABCXYZ...
//
// === LeetCode 14 ===
// fl
// []
//
// === LeetCode 67 ===
// 100
// 10101
//
// === LeetCode 1108 ===
// 1[.]1[.]1[.]1
// 255[.]100[.]50[.]0
//
// === 日常實務: query string ===
// q=cpp&page=2&sort=date
//
// === 日常實務: SQL IN placeholders ===
// WHERE id IN (?,?,?)
// WHERE id IN (?,?,?,?,?)
