// =============================================================================
// 檔名: contains.cpp
// 主題: std::string::contains (C++23: 檢查是否包含子字串/字元)
// 參考連結 (References):
//   - cppreference: https://en.cppreference.com/cpp/string/basic_string/contains
//   - cplusplus.com: https://cplusplus.com/reference/string/string/
//     (cplusplus.com 尚未收錄此 C++23 新成員;主頁仍可作為 std::string 的整體參考)
// =============================================================================
//
// 【函式資訊 Information】
//   constexpr bool contains(std::string_view sv) const noexcept;       // (1)
//   constexpr bool contains(char ch) const noexcept;                    // (2)
//   constexpr bool contains(const char* s) const;                       // (3)
//
// 所屬類別: std::basic_string<CharT, Traits, Allocator>
// 回傳型別: bool
// 標準:    C++23 加入。等價於 find(...) != npos。
//
// 【詳細解釋 Explanation】
//
// (1) 函式定位 ── 一個語法糖,但極度需要的語法糖:
//     從 C++98 起判斷字串「是否包含」某子字串的標準寫法是:
//
//         if (s.find(needle) != std::string::npos) { ... }
//
//     這個寫法問題在於:
//       a. npos 是個「特殊值」概念,初學者要先理解才看懂
//       b. find 的回傳型別是 size_type (unsigned),== npos 比 == -1 不直觀
//       c. 雙重否定 ── "不等於 not-found" 才表示「找到」── 認知負擔大
//       d. 與 starts_with、ends_with (C++20) 風格不一致
//
//     C++23 加入 contains 後寫法變成:
//
//         if (s.contains(needle)) { ... }
//
//     語意一目了然。標準庫實作幾乎一定就是 return find(sv) != npos;
//     沒有效能差異,純粹是可讀性的勝利。
//
// (2) 補完「字串便利三劍客」── starts_with / ends_with / contains:
//     - C++20: starts_with、ends_with
//     - C++23: contains
//     三者同樣有 string_view / char / const char* 三種重載,簽章對稱。
//     從此 std::string 終於追上了 Java、Python 二十年前就有的便利 API。
//
// (3) 與 find 的比較:
//     - find    : 回傳「位置」── 你需要知道在哪裡時用
//     - contains: 回傳「是/否」── 你只想知道有沒有時用
//     如果只想知道是否存在,用 contains 表達意圖更清晰,且若標準庫有特殊優化
//     (例如某些實作可在找到第一個就回傳,不必算精確位置),也能受惠。
//
// (4) 三種重載:
//     - sv (string_view) : 最常用,可吃任何字串型別
//     - char             : 比 contains("c") 還精準,絕對 noexcept;
//                          對「字串中有沒有逗號」這類單字元檢查最合適
//     - const char*      : 不接受 nullptr;與 sv 重疊
//
// (5) 時間複雜度: O(N*M),N = *this 長度,M = needle 長度。
//     與 find 相同 (因為內部就是 find)。實作通常用 KMP 或類 Boyer-Moore 加速。
//
// 【注意事項 Pay Attention】
// 1. 需 C++23 (g++ 14+ 或 clang++ 16+ 才完整支援)。
// 2. 對舊編譯器,fallback 寫法:
//      bool ok = (s.find(needle) != std::string::npos);
// 3. contains 不接受 nullptr。
// 4. 不是「正規表達式比對」── 只能找字面子字串。要 regex 用 std::regex_search。
// 5. 不會做 case-insensitive 比對;若要不分大小寫,先 tolower 整段或用 ICU。
// 6. 與 std::set::contains (C++20)、std::map::contains (C++20) 的命名一致,
//    讓「容器是否包含」這個概念跨容器統一。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼是 C++23 才加 contains,而不是 C++20 一起加?
//     根據 P1679R3 提案,contains 在 C++20 週期被討論時被認為「太瑣碎」,
//     且當時優先處理 spaceship、modules、coroutine 等大議題。
//     提案人 (Wim Leflere、Paul Fee) 在 C++20 後重提,引用「Java/Python/Rust 都有」
//     和「教學現場學生最常問」兩大論點,終於在 C++23 通過。
//
// (B) vs find != npos ── 真實的可讀性差距:
//
//         // 寫法 A:傳統 find
//         if (header.find("application/json") != std::string::npos) {
//             parseJson();
//         }
//
//         // 寫法 B:C++23 contains
//         if (header.contains("application/json")) {
//             parseJson();
//         }
//
//     code review 時,寫法 B 一秒看懂;寫法 A 要先掃完整行才知道在做什麼。
//     在大型 codebase 累積數萬處,可讀性差距會非常顯著。
//
// (C) 與 std::set::contains 共享命名 ── 「contains」現在是 C++ 容器的通用慣例:
//     - std::map::contains (C++20)
//     - std::set::contains (C++20)
//     - std::unordered_map::contains (C++20)
//     - std::string::contains (C++23)
//     - std::ranges::contains (C++23)
//     從此 if (container.contains(x)) 對任何容器都自然。
//
// (D) C++23 的脈絡 ── 與其他「補洞」一起來:
//     C++23 補了不少二十年沒解決的問題:
//       - std::string::contains  (本檔)
//       - std::print / std::println  (取代 printf 與 cout)
//       - std::expected  (Rust 風格錯誤處理)
//       - 把 ranges 強化到能寫 to_vector 等
//     可以說 C++23 是「日常便利」大躍進,而 contains 是其中最「有感」的一個。
//
// (E) 別把 contains 用在效能敏感路徑而以為它比 find 快:
//     contains 內部就是 find(sv) != npos,沒有更快。
//     若要做「先 contains 判斷,再用 find 取位置」── 那是 O(2N),
//     直接用 find 一次取位置再判斷 != npos 才是最高效。
//
// =============================================================================

/*
補充筆記：std::string::contains
  - std::string::contains 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::contains 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoContains() {
#if __cplusplus >= 202302L
    std::string s = "Hello, World!";
    std::cout << std::boolalpha;
    std::cout << s.contains("World") << "\n";   // true
    std::cout << s.contains('Z')     << "\n";   // false
    std::cout << s.contains("foo")   << "\n";   // false
#else
    std::cout << "(此編譯器尚未啟用 C++23,使用 find != npos 替代)\n";
    std::string s = "Hello, World!";
    bool has = (s.find("World") != std::string::npos);
    std::cout << std::boolalpha << "has World? " << has << "\n";
#endif
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1684. Count the Number of Consistent Strings
// 題目敘述:
//   給定字串 allowed (允許的字元集合) 與字串陣列 words,
//   若某 word 中每個字元都出現在 allowed 中,稱該 word 為 "consistent"。
//   請計算 words 中 consistent 字串的個數。
// 為何用 contains:
//   對每個 word 的每個字元,檢查 allowed 是否「包含」此字元 ──
//   contains(char) 一行解決,比 find(c) != npos 直觀許多。
// 解題思路:
//   外層走訪每個 word;內層走訪每個字元,若有任何字元不在 allowed 就 break;
//   若整個字串走完都通過,計數 +1。
// 複雜度: O(N * L * |allowed|),N=word 數、L=平均長度。
//          (allowed 若預先放進 std::bitset / 256 array,可降到 O(N*L))
// -----------------------------------------------------------------------------
#include <vector>
int countConsistentStrings(const std::string& allowed,
                           const std::vector<std::string>& words) {
    int cnt = 0;
    for (const auto& w : words) {
        bool ok = true;
        for (char c : w) {
            // C++23 contains;若無則退回 find
#if __cplusplus >= 202302L
            if (!allowed.contains(c)) { ok = false; break; }
#else
            if (allowed.find(c) == std::string::npos) { ok = false; break; }
#endif
        }
        if (ok) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Log 警示偵測:檢查行內是否含關鍵字
// 為何用 contains: 監控 / on-call 自動化常做。比 find != npos 更可讀。
//                  對舊編譯器仍可 fallback 到 find。
// -----------------------------------------------------------------------------
bool isAlertLog(const std::string& line) {
    static const std::vector<std::string> keywords = {
        "ERROR", "FATAL", "panic", "Exception", "OOM"
    };
    for (const auto& k : keywords) {
#if __cplusplus >= 202302L
        if (line.contains(k)) return true;
#else
        if (line.find(k) != std::string::npos) return true;
#endif
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1768. Merge Strings Alternately - 變奏 (含子字串檢查)
// 題目: LeetCode 1374 變奏 ── 計算 words 中有幾個含有任何給定 "禁用片段" 的字串。
// 為何用 contains: C++23 contains 一行解,語意比 find!=npos 直觀。
// 複雜度: O(總長度)。
// -----------------------------------------------------------------------------
int countWithAnyForbidden(const std::vector<std::string>& words,
                         const std::vector<std::string>& forbidden) {
    int cnt = 0;
    for (const auto& w : words) {
        for (const auto& f : forbidden) {
#if __cplusplus >= 202302L
            if (w.contains(f)) { ++cnt; break; }
#else
            if (w.find(f) != std::string::npos) { ++cnt; break; }
#endif
        }
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】判斷 URL 是否含 query string
// 為何用 contains(char): 只要判斷是否含 '?',contains(char) 是 noexcept + SIMD memchr,
//                       最簡潔且最快。
// -----------------------------------------------------------------------------
bool urlHasQuery(const std::string& url) {
#if __cplusplus >= 202302L
    return url.contains('?');
#else
    return url.find('?') != std::string::npos;
#endif
}

int main() {
    demoContains();
    std::cout << "\n=== LeetCode 1684 ===\n";
    std::cout << countConsistentStrings("ab", {"ad","bd","aaab","baa","badab"}) << "\n"; // 2
    std::cout << countConsistentStrings("abc", {"a","b","c","ab","ac","bc","abc"}) << "\n"; // 7

    std::cout << "\n=== LeetCode 1374 變奏 ===\n";
    std::cout << countWithAnyForbidden({"hello","worlds","helmet","wonder"}, {"hel","wor"}) << "\n";  // 4

    std::cout << "\n=== 日常實務: log alert ===\n";
    for (auto& line : {
        "2026-05-04 INFO  startup ok",
        "2026-05-04 ERROR connection refused",
        "2026-05-04 WARN  slow query",
        "2026-05-04 FATAL OOM killed"}) {
        std::cout << (isAlertLog(line) ? "[ALERT] " : "        ") << line << "\n";
    }

    std::cout << "\n=== 日常實務: urlHasQuery ===\n";
    std::cout << std::boolalpha
              << urlHasQuery("https://example.com/path?x=1") << "\n"   // true
              << urlHasQuery("https://example.com/path")     << "\n";  // false

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：contains 是 C++ 哪個版本加入的?為什麼這麼晚?
    //    A：C++23 才加入,比 starts_with / ends_with (C++20) 還晚。原因是
    //       C++ 標準委員會本來認為 find(...) != npos 就夠用,但實際 review
    //       後承認可讀性差太多 (雙重否定、特殊值 npos),最終 P1679 被接受。
    //       Java 1.5、Python 從來都有這 API,C++ 確實晚了 20+ 年。
    //
    //  Q2：s.contains(needle) 與 s.find(needle) != npos 完全等價嗎?
    //    A：行為等價,標準庫實作幾乎就是 return find(sv) != npos;。沒有
    //       效能差異 (內部仍要跑 find)。但若實作願意做特殊化 (找到第一個
    //       即停,不必算精確位置),理論上 contains 可省一些尾段比對,但
    //       目前主流實作沒做這層優化。
    //
    //  Q3：contains 有 char overload,為什麼?和 contains("c") 差在哪?
    //    A：contains(char) 是 noexcept、可走 memchr 的 SIMD 路徑、無需建構
    //       string_view;contains("c") 走 const char* overload,需 strlen
    //       後再走 substring 搜尋,雖然編譯器可能優化,但語意與成本明確選
    //       contains(char) 較好。判斷「字串中有沒有逗號」這類最常用。
    //
    return 0;
}

// 編譯 (建議): g++ -std=c++23 -Wall -Wextra contains.cpp -o contains
//        若版本不支援 C++23: g++ -std=c++20 -Wall -Wextra contains.cpp -o contains

// === 預期輸出 (節錄) ===
// === LeetCode 1374 變奏 ===
// 4
// === 日常實務: urlHasQuery ===
// true
// false
