// =============================================================================
// 檔名: substr.cpp
// 主題: std::string::substr (取得子字串)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/substr
//   - https://cplusplus.com/reference/string/string/substr/
// =============================================================================
//
// 【函式資訊 Information】
//   string substr(size_type pos = 0, size_type count = npos) const;
//   // C++23 起加上 rvalue 版本,可移動內部緩衝區
//   string substr(size_type pos = 0, size_type count = npos) &&;
//
// 【詳細解釋 Explanation】
//
// 一、設計理念
//   substr 是「取出片段」的最簡單介面 ── 給 (pos, count),回傳新字串。
//   它「總是」回傳新物件,意味著:
//     - 完全獨立 (修改 substr 不影響原字串)。
//     - 可以安全跨函式邊界傳遞、儲存到容器、回傳。
//     - 但每次都要配置記憶體 + 拷貝資料,效能不是最佳。
//   這就是為什麼 C++17 推出 std::string_view 作為「只讀片段」的替代方案。
//
// 二、底層運作
//   substr(pos, count) 內部相當於:
//     return string(data() + pos, std::min(count, size() - pos));
//   也就是「呼叫 string 的 (const char*, size_t) 建構子」── 配置新緩衝區、
//   拷貝指定範圍、補上 '\0'。
//
//   小字串優化 (SSO):若 substr 結果很短 (通常 < 16 字元),
//   會被存在 string 物件「內部 stack 空間」,不會配置 heap,效能極佳。
//
// 三、C++23 的 rvalue 版本
//   原本 substr 一律是 const&,即使 *this 是 rvalue 也得拷貝。
//   C++23 加入 && 版本,當你寫:
//     auto s = std::move(orig).substr(5);
//   實作可以「移動」內部緩衝區並原地裁切,免去拷貝。
//   實務應用:配合 ranges、配合 string factory 函式。
//
// 四、與 std::string_view 的取捨 (重點!)
//   兩種「取片段」的工具:
//     std::string sv  = s.substr(pos, count);    // 拷貝、安全、可獨立存活
//     std::string_view sv = std::string_view(s).substr(pos, count); // 不拷貝、危險、生命週期綁原字串
//
//   string_view 的優勢:
//     - 零配置、零拷貝、O(1) 構造。
//     - 大量切片 (parser、tokenizer) 效能可提升數十倍。
//   string_view 的代價:
//     - 不擁有資料,原字串若銷毀 → dangling view → UB。
//     - 不一定 null-terminated,不能直接傳給 C API (printf 的 %s)。
//     - 不能修改。
//
//   一般原則:
//     - 短期觀察 (parsing、searching、僅 read-only 處理) → string_view。
//     - 長期保存、跨執行緒、或需要修改 → substr。
//
// 五、與其他函式的關係
//   - copy()  : 同樣抽片段,但寫入 char*,不配置 string;適合 C 介接。
//   - find()  : 通常 find 找到位置後接 substr 取出片段。
//   - C++20 starts_with/ends_with : 用前後綴判斷可避免不必要的 substr。
//
// 六、版本演進
//   - C++03 : 基本提供。
//   - C++11 : noexcept 微調、move-friendly。
//   - C++17 : std::string_view 出現,提供更輕量替代方案。
//   - C++23 : 加入 rvalue 版本 (move-from-self optimization)。
//
// 時間複雜度: O(count)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為何回傳新 string 而非 reference / view?
//   substr 最早設計於 C++98,當時還沒有 string_view 的概念,
//   且「擁有資料 (owning)」的設計讓使用者完全不用擔心生命週期。
//   後續為了效能,才補上 string_view 作為輕量替代。
//   舊有 substr 不能直接改成 view,會破壞無數既有程式碼。
//
// (B) substr 與「子字串搜尋」的常見組合
//   經典模式:
//     auto pos = s.find('=');
//     auto key = s.substr(0, pos);
//     auto val = s.substr(pos + 1);
//   分割 key=value、parsing URL、解析 header 都是這個模式。
//   進階版可改用 string_view 避免兩次拷貝。
//
// (C) substr 在 LeetCode 的常見場景
//   最長迴文 (LC 5)、最長公共前綴 (LC 14)、字串分割 (LC 763)、
//   IP 還原 (LC 93) 等,凡是「找到區間 → 取出來 → 比較或回傳」的題目,
//   都會用到 substr。考點通常不是函式本身,而是「邊界 (pos+count)」處理。
//
// (D) 字串連接陷阱
//   壞:s = a.substr(0,5) + b.substr(3,4);  // 兩次配置 + 兩次拷貝 + 一次 operator+
//   好:string out; out.reserve(9); out.append(a, 0, 5); out.append(b, 3, 4);
//   substr 在熱點區慎用。
//
// 【注意事項 Pay Attention】
// 1. pos > size() 會丟 std::out_of_range。
// 2. substr 一定回傳新字串(會配置記憶體);若只是要「看一眼」,
//    C++17 起的 std::string_view 不需要拷貝,效能更好。
// 3. 不要用 substr 做大量拼接的中介物件,效率不佳。
// 4. 串接版本可考慮 substr 後再 += / append,或更好地用 string_view。
// 5. count 超過剩餘長度時會「自動截短」,不會丟例外 (與 pos 越界不同)。
// 6. C++23 起若已知物件是 rvalue,寫 std::move(s).substr(...) 可省一次拷貝。
//
// =============================================================================

/*
補充筆記：std::string::substr
  - substr 會建立新的 string，這代表配置與拷貝成本，不是 view。
  - pos 超過 size 會丟 out_of_range；count 超過剩餘長度則只取到結尾。
  - 若只想借看一段文字且來源會活得夠久，string_view::substr 成本更低。
  - std::string::substr 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoSubstr() {
    std::string s = "Hello, World!";

    std::cout << s.substr()       << "\n";  // 完整複製
    std::cout << s.substr(7)      << "\n";  // "World!"
    std::cout << s.substr(7, 5)   << "\n";  // "World"
    std::cout << s.substr(0, 5)   << "\n";  // "Hello"
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 14. Longest Common Prefix
// 題目: 給一組字串,找出最長共同前綴。例如 ["flower","flow","flight"] → "fl"。
// 為何用 substr: 用「水平掃描法」── 取第一個字串為候選 prefix,
//                若不是 strs[i] 的前綴 → 用 substr 截短 prefix,直到符合或變空。
// 思路: 反覆呼叫 substr(0, prefix.size() - 1) 縮短候選,直到 find 回傳 0。
// 複雜度: 最差 O(S),S 是所有字元總和。
// -----------------------------------------------------------------------------
#include <vector>
std::string longestCommonPrefixBySubstr(const std::vector<std::string>& strs) {
    if (strs.empty()) return "";
    std::string prefix = strs[0];
    for (size_t i = 1; i < strs.size(); ++i) {
        // 若 prefix 不是 strs[i] 的前綴,就持續砍尾
        while (strs[i].find(prefix) != 0) {
            if (prefix.empty()) return "";
            prefix = prefix.substr(0, prefix.size() - 1);
        }
    }
    return prefix;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 5. Longest Palindromic Substring
// 題目: 找字串中最長的迴文子字串。
// 為何用 substr: 找到最佳區間 [start, start+len) 後,呼叫 substr 取出結果。
// 演算法: 中心擴展法,O(N^2) 時間 / O(1) 額外空間。
// -----------------------------------------------------------------------------
std::string longestPalindrome(const std::string& s) {
    if (s.empty()) return "";

    auto expand = [&](int l, int r) {
        while (l >= 0 && r < static_cast<int>(s.size()) && s[l] == s[r]) {
            --l; ++r;
        }
        return std::pair<int,int>(l + 1, r - 1);    // 收回最後合法的位置
    };

    int start = 0, maxLen = 1;
    for (int i = 0; i < static_cast<int>(s.size()); ++i) {
        auto [l1, r1] = expand(i, i);          // 奇數長度
        auto [l2, r2] = expand(i, i + 1);      // 偶數長度
        if (r1 - l1 + 1 > maxLen) { start = l1; maxLen = r1 - l1 + 1; }
        if (r2 - l2 + 1 > maxLen) { start = l2; maxLen = r2 - l2 + 1; }
    }
    return s.substr(start, maxLen);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 28. strStr (Find the Index of the First Occurrence)
// 題目: 在 haystack 中找 needle 的第一個出現位置。
// 為何用 substr: 可用 substr + compare 暴力解 (示範用途)。實務上 find 更快。
// -----------------------------------------------------------------------------
int strStr(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return 0;
    if (haystack.size() < needle.size()) return -1;
    for (size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        if (haystack.substr(i, needle.size()) == needle) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】解析 Authorization header 的 Bearer token
// 為何用 substr: HTTP 請求驗證的標準動作 ── 拿到 "Bearer abc123" 後切出 token。
//                 後端 middleware 必備功能。
// -----------------------------------------------------------------------------
std::string parseBearerToken(const std::string& authHeader) {
    const std::string prefix = "Bearer ";
    if (authHeader.size() <= prefix.size()) return "";
    if (authHeader.substr(0, prefix.size()) != prefix) return "";
    return authHeader.substr(prefix.size());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 4】LeetCode 1location. 1location0. Reverse Prefix of Word (Easy)
// 題目: LeetCode 2000. Reverse Prefix of Word
// 給定 word 與 ch,反轉從開頭到第一個 ch (含) 的子字串;ch 不存在則回原字串。
// 為何用 substr: 找到 ch 位置後,用 substr(0, idx+1) 取前段、substr(idx+1) 取後段,
//                前段 reverse 再串回。教學「分段切片再組合」的經典套路。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
#include <algorithm>
std::string reversePrefix(const std::string& word, char ch) {
    size_t idx = word.find(ch);
    if (idx == std::string::npos) return word;
    std::string head = word.substr(0, idx + 1);
    std::reverse(head.begin(), head.end());
    return head + word.substr(idx + 1);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從檔名取得「主檔名」與「副檔名」
// 為何用 substr: 找最後的 '.' 後,前段是 stem、後段是 extension。
//                打包工具、檔案瀏覽器、上傳檔案格式檢查的必備動作。
//                注意處理「沒副檔名」與「以 . 開頭的隱藏檔」邊界。
// -----------------------------------------------------------------------------
std::pair<std::string, std::string> splitExtension(const std::string& filename) {
    size_t dot = filename.rfind('.');
    // 沒有 '.' 或 '.' 出現在開頭 (隱藏檔如 ".bashrc") → 整個視為 stem
    if (dot == std::string::npos || dot == 0) return {filename, ""};
    return {filename.substr(0, dot), filename.substr(dot + 1)};
}

int main() {
    demoSubstr();

    std::cout << "\n=== LeetCode 14 ===\n";
    std::cout << longestCommonPrefixBySubstr({"flower","flow","flight"}) << "\n";  // "fl"
    std::cout << "[" << longestCommonPrefixBySubstr({"dog","racecar","car"}) << "]\n"; // 空

    std::cout << "\n=== LeetCode 5 ===\n";
    std::cout << longestPalindrome("babad") << "\n";        // "bab" 或 "aba"
    std::cout << longestPalindrome("cbbd")  << "\n";        // "bb"

    std::cout << "\n=== LeetCode 28 ===\n";
    std::cout << strStr("hello", "ll")    << "\n";          // 2
    std::cout << strStr("aaaaa", "bba")   << "\n";          // -1

    std::cout << "\n=== LeetCode 2000 ===\n";
    std::cout << reversePrefix("abcdefd", 'd') << "\n";     // "dcbaefd"
    std::cout << reversePrefix("xyxzxe",  'z') << "\n";     // "zxyxxe"
    std::cout << reversePrefix("abcd",    'z') << "\n";     // "abcd"

    std::cout << "\n=== 日常實務: Bearer token ===\n";
    std::cout << "[" << parseBearerToken("Bearer eyJhbGc.aaa.bbb") << "]\n";
    std::cout << "[" << parseBearerToken("Basic abc123")           << "]\n";  // empty

    std::cout << "\n=== 日常實務: 切分主檔名/副檔名 ===\n";
    auto [stem1, ext1] = splitExtension("report.pdf");
    auto [stem2, ext2] = splitExtension("archive.tar.gz");
    auto [stem3, ext3] = splitExtension("Makefile");
    auto [stem4, ext4] = splitExtension(".bashrc");
    std::cout << "report.pdf       -> stem=[" << stem1 << "] ext=[" << ext1 << "]\n";
    std::cout << "archive.tar.gz   -> stem=[" << stem2 << "] ext=[" << ext2 << "]\n";
    std::cout << "Makefile         -> stem=[" << stem3 << "] ext=[" << ext3 << "]\n";
    std::cout << ".bashrc          -> stem=[" << stem4 << "] ext=[" << ext4 << "]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:substr(pos, count) 與 string_view{s}.substr(pos, count) 哪個快?
    //    A:string_view 的 substr 是 O(1) — 只調整 (ptr, len) 兩個欄位,
    //      零配置零拷貝。string::substr 必定 O(count) 配置新 buffer 並複製。
    //      只讀取片段一律優先 string_view,需要獨立、可修改、長期保存的
    //      副本才用 string::substr。
    //
    //  Q2:C++23 對 substr 加了什麼新東西?
    //    A:加上 rvalue-ref 重載 string substr(pos, count) &&。當呼叫者
    //      明顯是 rvalue (例如 std::move(s).substr(...)) 時,實作可以直接
    //      move 內部 buffer 並原地裁切,免去一次完整拷貝。對「臨時 string
    //      的最後一次切片」場景能省下 O(n) 配置。
    //
    //  Q3:substr 的 pos 與 count 越界規則?
    //    A:pos > size() 丟 std::out_of_range (與 operator[] 不同,後者是 UB)。
    //      count 超過剩餘長度則自動 clamp 為 size()-pos,不丟例外。pos==size()
    //      合法,回傳空字串。寫 trim 或解析時用這個性質可省 if 守衛。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra substr.cpp -o substr

// === 預期輸出 ===
// Hello, World!
// World!
// World
// Hello
//
// === LeetCode 14 ===
// fl
// []
//
// === LeetCode 5 ===
// bab
// bb
//
// === LeetCode 28 ===
// 2
// -1
//
// === LeetCode 2000 ===
// dcbaefd
// zxyxxe
// abcd
//
// === 日常實務: Bearer token ===
// [eyJhbGc.aaa.bbb]
// []
//
// === 日常實務: 切分主檔名/副檔名 ===
// report.pdf       -> stem=[report] ext=[pdf]
// archive.tar.gz   -> stem=[archive.tar] ext=[gz]
// Makefile         -> stem=[Makefile] ext=[]
// .bashrc          -> stem=[.bashrc] ext=[]
