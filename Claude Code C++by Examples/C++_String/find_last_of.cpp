// =============================================================================
// 檔名: find_last_of.cpp
// 主題: std::string::find_last_of (從後往前找「任一」目標字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/find_last_of
//   - https://cplusplus.com/reference/string/string/find_last_of/
// =============================================================================
//
// 【函式資訊 Information】
//   size_type find_last_of(const string& str, size_type pos = npos)              const noexcept; // (1)
//   size_type find_last_of(const char*   s,   size_type pos = npos)              const;          // (2)
//   size_type find_last_of(const char*   s,   size_type pos, size_type count)    const;          // (3)
//   size_type find_last_of(char          ch,  size_type pos = npos)              const noexcept; // (4)
//   size_type find_last_of(string_view   sv,  size_type pos = npos)              const noexcept; // (5) C++17
//
// 回傳: 最大的位置 i (i ≤ pos),使得 s[i] 屬於 str 字元集合;沒找到回 npos。
//
// 【詳細解釋 Explanation】
// find_last_of 是 find_first_of 的反向版本 — 從後往前掃,找最後一個「屬於字元
// 集 str」的字元位置。回傳值仍是 forward 索引(從 0 起算的字元 offset)。
//
// 【1. 與 find_first_of 的對照】
//   find_first_of(set, 0)        從前往後找第一個「屬於 set」的字元
//   find_last_of(set, npos)      從後往前找最後一個「屬於 set」的字元
// 範例:s = "Hello, World!"
//   s.find_first_of("aeiou")   → 1   (第一個母音 'e')
//   s.find_last_of("aeiou")    → 8   (最後一個母音 'o')
//   s.find_first_of(",!")      → 5   (第一個標點 ',')
//   s.find_last_of(",!")       → 12  (最後一個標點 '!')
//
// 【2. pos 參數語意】
// pos 是「搜尋上界」 — 找出最大的 i,使 i ≤ pos 且 s[i] 屬於集合。
// 預設 npos 表示「不設上界」,等同 size() - 1。
// 範例:s = "abcabc"
//   s.find_last_of("ab")        → 4   ('b' at 4)
//   s.find_last_of("ab", 3)     → 3   ('a' at 3)
//   s.find_last_of("ab", 2)     → 1   ('b' at 1)
//   s.find_last_of("ab", 0)     → 0   ('a' at 0)
//   s.find_last_of("z")         → npos
//
// 【3. find_last_of vs rfind:容易混淆!】
// 兩個都「從後往前」,但找的「單位」完全不同:
//   * rfind("abc")          找子字串 "abc" 連續出現的最後位置
//   * find_last_of("abc")   找「'a' 或 'b' 或 'c'」最後出現的位置
// 例:s = "xyzab"
//   s.rfind("abc")          → npos (沒有 "abc" 子字串)
//   s.find_last_of("abc")   → 4    (最後一個 'b' 在 4)
//
// 【4. 演算法與效能】
// naive 實作從 min(pos, size()-1) 往下掃:
//   for i in [min(pos, N-1), 0]:
//       if s[i] in str: return i
//   return npos
// 對 char 字元集可用 bool[256] 加速為 O(N + M)。複雜度與 find_first_of 一樣。
//
// 【5. 各重載細節】
// 與 find_first_of 完全對應,差別只在「掃描方向」與「pos 預設值是 npos」。
//
// 【6. 邊界規則】
//   * str 為空集合:永遠回傳 npos。
//   * 在空字串上:回傳 npos。
//   * pos > size():視為 size() - 1(不設上界)。
//
// 【7. 常見應用情境】
//   * 取副檔名:path.find_last_of('.')
//   * 取目錄部分:path.find_last_of("/\\")(同時匹配 Linux 與 Windows 分隔符)
//   * 取最後一個分隔符位置:dotted.find_last_of('.')
//   * 找最後一個非空白字元(通常用 find_last_not_of)
//
// 【概念補充 Concept Deep Dive】
// (A) 為什麼路徑解析常用 find_last_of 而非 rfind
//   跨平台路徑常需要同時識別 '/' 與 '\\':
//     path.find_last_of("/\\")     一次搞定兩種分隔符
//   若用 rfind 要分別呼叫兩次再取較大者,程式碼囉唆。
//
// (B) 與 std::filesystem::path 的關係 (C++17)
//   現代 C++ 處理路徑請優先用 std::filesystem::path:
//     fs::path p("/etc/hosts");
//     p.filename();    // "hosts"
//     p.extension();   // ""
//     p.stem();        // "hosts"
//   它已經處理跨平台、UTF-8、特殊字元等所有細節。但若是處理「URL、識別字、
//   點分名稱」等非檔案系統字串,find_last_of 仍是首選。
//
// (C) find_last_of vs find_last_not_of
//   兩者構成「集合」與「補集」搜尋:
//     find_last_of("0123456789")     最後一個數字
//     find_last_not_of("0123456789") 最後一個非數字(用於「去尾數字」)
//
// 【注意事項 Pay Attention】
// 1. 與 rfind 差異:
//      rfind        : 找「子字串」整體最後出現位置
//      find_last_of : 找「字元集」中任一最後出現
// 2. 結果為「位置 ≤ pos 中最大的一個」 — 別誤以為從末端 0 號位置算起。
// 3. str 為空字串:永遠回傳 npos。
// 4. 跨平台路徑分隔符處理:find_last_of("/\\") 是慣用法。
// 5. 回傳的索引是 forward 的,不是反向的。
//
// =============================================================================

/*
補充筆記：std::string::find_last_of
  - std::string::find_last_of 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::find_last_of 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::find_last_of
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. find_last_of 與 rfind 差在哪?
//     答：find_last_of 找「字元集合」中任一字元的最後出現位置;
//         rfind 找「整個子字串」最後一次出現的位置。
//         抽副檔名用 find_last_of(".");找最後一個 "://" 這種整串則要用 rfind。
//     追問：回傳的是反向索引嗎?→ 不是,是從 0 起算的 forward 索引。
//
// 🔥 Q2. 典型用途是什麼?
//     答：切路徑與副檔名。find_last_of("/\\") 一次容忍兩種目錄分隔符——
//         這正是「集合」語意帶來的好處;find_last_of(".") 取副檔名起點。
//
// ⚠️ 陷阱. s.substr(s.find_last_of(".") + 1) 沒檢查 npos 會怎樣?
//     答：找不到 '.' 時回傳 npos(無號的 -1),npos + 1 無號回繞成 0,
//         於是 substr(0) 悄悄回傳「整個字串」——不拋例外、不 crash,安靜地給錯答案。
//     為什麼會錯：以為「沒找到就會出錯讓我知道」,
//         但無號溢位是良好定義的回繞,錯誤會被完全吞掉。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoFindLastOf() {
    std::string s = "Hello, World!";
    std::cout << "last vowel at " << s.find_last_of("aeiou") << "\n";   // 8 ('o')
    std::cout << "last of \",!\" at " << s.find_last_of(",!") << "\n";  // 12
}

// -----------------------------------------------------------------------------
// 【實務範例】取得檔名的副檔名(以 '.' 分隔)
// 為何用 find_last_of: 從末端找 '.',若找不到就視為無副檔名。
//                      用 find_last_of 而非 rfind 是為了同時處理多個分隔候選。
// -----------------------------------------------------------------------------
std::string getExtension(const std::string& path) {
    size_t dot = path.find_last_of('.');
    size_t slash = path.find_last_of("/\\");
    if (dot == std::string::npos) return "";
    if (slash != std::string::npos && dot < slash) return ""; // .hidden 在資料夾裡
    return path.substr(dot + 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 434. Number of Segments in a String
// 題目: 統計字串中有多少個段(以空白分隔)。
// 為何用 find_last_of: 我們示範「找最後一個非空白」 - 雖然這題簡單可一次掃描,
//                      但展示如何用 find_*_of 系列。
// -----------------------------------------------------------------------------
int countSegments(const std::string& s) {
    int cnt = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != ' ' && (i == 0 || s[i - 1] == ' ')) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】從點分識別字取最後一段 (event.user.created → "created")
// 為何用 find_last_of: 監控 / metric / event name 常用點分階層命名。
//                      取最後一段做 routing / dashboard 標籤。
// -----------------------------------------------------------------------------
std::string lastSegment(const std::string& dotted, char sep = '.') {
    size_t p = dotted.find_last_of(sep);
    return (p == std::string::npos) ? dotted : dotted.substr(p + 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1location. 1location0. Number of Substrings Containing All Three Characters - 簡化版
// 題目: LeetCode 1422. Maximum Score After Splitting a String
// 給只有 0/1 的字串 s,在某位置切兩段 left/right,score = left 的 0 個數 + right 的 1 個數,
// 求最大 score。
// 為何用 find_last_of: 我們可預先用 find_last_of('1') 找最右邊的 '1',輔助快速計算。
// 此處示範以 prefix 累積 + 一次掃描的簡潔版。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int maxScoreSplit(const std::string& s) {
    int ones = 0;
    for (char c : s) if (c == '1') ++ones;
    int best = 0, zerosLeft = 0, onesLeft = 0;
    for (size_t i = 0; i + 1 < s.size(); ++i) {
        if (s[i] == '0') ++zerosLeft; else ++onesLeft;
        int score = zerosLeft + (ones - onesLeft);
        if (score > best) best = score;
    }
    return best;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 email 取出 domain (最後 '@' 之後)
// 為何用 find_last_of: email 可能含多個 '@' (技術上很罕見但允許),取最後一個
//                      較安全;一行解。
// -----------------------------------------------------------------------------
std::string emailDomain(const std::string& email) {
    size_t at = email.find_last_of('@');
    return (at == std::string::npos) ? "" : email.substr(at + 1);
}

int main() {
    demoFindLastOf();

    std::cout << "\n=== getExtension ===\n";
    std::cout << "[" << getExtension("file.tar.gz") << "]\n";        // gz
    std::cout << "[" << getExtension("/etc/.config") << "]\n";       // ""
    std::cout << "[" << getExtension("noext") << "]\n";              // ""

    std::cout << "\n=== LeetCode 434 ===\n";
    std::cout << countSegments("Hello, my name is John") << "\n";    // 5
    std::cout << countSegments("   ")                    << "\n";    // 0

    std::cout << "\n=== LeetCode 1422 ===\n";
    std::cout << maxScoreSplit("011101")   << "\n";   // 5
    std::cout << maxScoreSplit("00111")    << "\n";   // 5
    std::cout << maxScoreSplit("1111")     << "\n";   // 3

    std::cout << "\n=== 日常實務: 取最後一段 ===\n";
    std::cout << lastSegment("event.user.created")     << "\n";   // created
    std::cout << lastSegment("com.example.app.Service") << "\n";  // Service
    std::cout << lastSegment("plain")                  << "\n";   // plain

    std::cout << "\n=== 日常實務: emailDomain ===\n";
    std::cout << "[" << emailDomain("alice@example.com")       << "]\n";   // example.com
    std::cout << "[" << emailDomain("no-at-sign")             << "]\n";   // (empty)

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:find_last_of 跟 rfind 是同一件事嗎?
    //    A:不是。兩個都「從後往前」,但 rfind("abc") 找子字串 "abc",
    //       find_last_of("abc") 找「'a' 或 'b' 或 'c'」其中一個字元。
    //       例 s="xyzab":rfind("abc") = npos,find_last_of("abc") = 4 ('b')。
    //
    //  Q2:跨平台路徑分隔處理為什麼偏好 find_last_of("/\\") 而非 rfind?
    //    A:Windows 用 '\\'、Linux/Mac 用 '/',find_last_of("/\\") 一次就能
    //       匹配兩種分隔符。用 rfind 要呼叫兩次再取較大值,囉唆又易錯。
    //       現代 C++17 起更推薦直接用 std::filesystem::path。
    //
    //  Q3:回傳的 index 是「從前算起」還是「從尾算起」?
    //    A:一律是 forward 索引 (從 0 起算),與 find 系列一致。即使從後往前
    //       搜,我們關心的還是該字元在字串中的絕對位置,所以 substr(pos+1)
    //       這種「取最後一段」的慣用法才能直接套用。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra find_last_of.cpp -o find_last_of

// === 預期輸出 (節錄) ===
// === LeetCode 1422 ===
// 5
// 5
// 3
// === 日常實務: emailDomain ===
// [example.com]
// []
