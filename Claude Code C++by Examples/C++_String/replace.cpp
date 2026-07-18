// =============================================================================
// 檔名: replace.cpp
// 主題: std::string::replace (用新內容取代部分子字串)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/replace
//   - https://cplusplus.com/reference/string/string/replace/
// =============================================================================
//
// 【函式資訊 Information - 主要重載】
//   string& replace(size_type pos, size_type count, const string& str);
//   string& replace(const_iterator first, const_iterator last, const string& str);
//   string& replace(size_type pos, size_type count,
//                   const string& str, size_type pos2, size_type count2 = npos);
//   template<class InputIt>
//   string& replace(const_iterator first, const_iterator last,
//                   InputIt first2, InputIt last2);
//   string& replace(size_type pos, size_type count, const char* s, size_type count2);
//   string& replace(size_type pos, size_type count, const char* s);
//   string& replace(size_type pos, size_type count, size_type count2, char ch);
//   string& replace(const_iterator first, const_iterator last,
//                   std::initializer_list<char> ilist);
//   // C++17 起加上 string_view 重載
//
// 全部回傳 *this (string&),允許鏈式呼叫。
//
// 【詳細解釋 Explanation】
//
// 一、設計理念
//   replace 是「等同於 erase + insert 但更高效」的綜合操作:
//     - erase  : 從 [pos, pos+count) 移除字元
//     - insert : 在 pos 插入新內容
//   分開做需要兩次 memmove;合併成 replace 一次完成,效能較好。
//   它是 string 中「最強大」的修改器之一,因為新舊長度可以不同 ──
//   無論變短、變長、不變,都能正確處理後續字元的位置。
//
// 二、底層運作
//   replace(pos, count, newContent) 的步驟:
//     1. 計算新長度 = size() - count + newLen。
//     2. 若新長度 > 原 capacity → reallocate (連帶搬移整段字串)。
//     3. 把 [pos+count, size()) 的尾段,搬到 [pos+newLen, ...) 的位置:
//        - 若 newLen > count → 尾段往「後」搬 (要從尾部往前覆寫,避免覆蓋)。
//        - 若 newLen < count → 尾段往「前」搬。
//        - 若 newLen == count → 不搬,只直接覆寫 (最快)。
//     4. 把新內容拷貝到 [pos, pos+newLen)。
//     5. 更新 size 與結尾 '\0'。
//
//   重點:當 newLen == count 時 replace 退化為「原地覆寫」,完全沒有搬移成本。
//   IP defang、字元 mask 等場景特別適合。
//
// 三、與其他函式的關係
//   - erase + insert : 等價組合,但 replace 一次到位。
//   - assign         : assign 是「整段重設」,replace 只動指定範圍。
//   - operator[] / at: 只能單字元修改 (不改變 size);replace 可長度變動。
//   - find + replace : 配合做「全域取代」(類似 sed);非常實用。
//
// 四、版本演進
//   - C++11 : 加入 initializer_list 重載與 noexcept 標註。
//   - C++17 : 加入 string_view 重載。
//   - C++20 : constexpr 支援。
//
// 流程示意:
//   原字串: "Hello, World!"
//   replace(7, 5, "C++")  → "Hello, C++!"   (取代第 7 個位置 5 個字元)
//   replace(0, 5, "Hi")   → "Hi, World!"     (新長度較短,後段往前搬)
//   replace(0, 5, "GoodMorning") → "GoodMorning, World!" (新長度較長,後段往後搬)
//
// 時間複雜度: O(N + M),N 是後續需搬移字元數,M 是新內容長度。
//             若 newLen == count → 退化為 O(M)。
//
// 【概念補充 Concept Deep Dive】
//
// (A) Strong Exception Safety (強例外保證)
//   replace 提供強例外保證 ── 若操作中拋出例外 (例如 reallocation 失敗),
//   原字串「保持完全不變」。實作通常是「先建構新緩衝區成功後再 swap」。
//   這比 basic guarantee (只保證不洩漏資源) 更嚴格,讓使用者可以放心在
//   transaction 中呼叫,失敗就回到原狀。
//
// (B) Iterator / Pointer / Reference 失效
//   replace 後:
//     - 若 newLen == count 且未 reallocate → 範圍外迭代器仍有效,範圍內仍指向新內容。
//     - 若 reallocate → 所有迭代器、指標、參考全部失效。
//     - 若僅 size 改變但未 realloc → 從 pos 之後的迭代器位置雖仍合法,
//       但「所指向的字元」已改變,語意上要視為失效。
//   建議:replace 後不再使用之前保存的 iterator/pointer。
//
// (C) 為何「全域取代」需要小心 p += repl.size()?
//   經典陷阱:replace 後若把 p 留在原位,而新內容又包含 needle (例如把 "a" 換成 "aa"),
//   下一次 find 會立刻找到剛插入的 "a",造成「無窮迴圈」。
//   修正:每次替換後把 p 跳到新內容尾端,確保不會回頭掃描已寫入區。
//
// (D) replace vs std::regex_replace
//   replace 只能做「字面字串」取代,效能極佳。
//   std::regex_replace (在 <regex>) 支援正則,功能強但慢一個數量級。
//   只要不是 pattern,優先用 replace。
//
// (E) replace 還是 erase+insert?
//   兩者結果相同,但 replace 只做一次 memmove,erase+insert 做兩次。
//   且 replace 是 strong exception-safe;erase+insert 中間若失敗,字串已被改一半。
//   永遠優先 replace。
//
// 【注意事項 Pay Attention】
// 1. pos > size() 會丟 std::out_of_range。count 大於剩餘長度會自動截短。
// 2. replace 使迭代器 / 指標 / 參考可能失效(若觸發 reallocation)。
// 3. 想做「全域取代所有出現的子字串」時,replace 配合 find 在迴圈中使用:
//      while ((p = s.find(needle, p)) != std::string::npos) {
//          s.replace(p, needle.size(), repl);
//          p += repl.size();           // 跳過新插入的內容,避免無窮迴圈
//      }
// 4. 若新舊長度相等,replace 通常較快(不需搬移、原地覆寫)。
// 5. replace 提供強例外保證 (strong exception safety)。
// 6. 不接受空 needle 的全域取代 (會無窮迴圈),呼叫前要檢查 from.empty()。
//
// =============================================================================

/*
補充筆記：std::string::replace
  - replace 會把範圍內符合舊值的元素改成新值，容器大小不變。
  - replace_if 用 predicate 決定哪些元素要改，適合條件比對。
  - 若需要保留原資料並輸出新資料，應看 replace_copy。
  - std::string::replace 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoReplace() {
    std::string s = "Hello, World!";

    // (1) pos+count 替換
    s.replace(7, 5, "C++");
    std::cout << "(1) " << s << "\n";   // "Hello, C++!"

    // (2) iterator 範圍替換
    s.replace(s.begin(), s.begin() + 5, "Hi");
    std::cout << "(2) " << s << "\n";   // "Hi, C++!"

    // (3) 用 N 個字元替換
    s.replace(0, 2, 3, '*');
    std::cout << "(3) " << s << "\n";   // "***, C++!"
}

// -----------------------------------------------------------------------------
// 【實務範例】全域字串取代 (常見實務需求)
// 為何用 replace: 配合 find 迴圈,做「sed s/old/new/g」式的取代。
// -----------------------------------------------------------------------------
std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t p = 0;
    while ((p = s.find(from, p)) != std::string::npos) {
        s.replace(p, from.size(), to);
        p += to.size();
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1108. Defanging an IP Address
// 題目: 把 IP 字串中的 '.' 換成 "[.]"。例如 "1.1.1.1" → "1[.]1[.]1[.]1"。
// 為何用 replace: 經典「字面字串取代」場景,搭配 find 在迴圈中替換。
//                 注意新內容 "[.]" 較長,不會引發無窮迴圈 (跳過 to.size())。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string defangIPaddr(std::string address) {
    size_t p = 0;
    while ((p = address.find('.', p)) != std::string::npos) {
        address.replace(p, 1, "[.]");
        p += 3;     // "[.]" 長度為 3,跳過避免重掃描
    }
    return address;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1844. Replace All Digits with Characters
// 題目: 偶數索引是字母,奇數索引是數字。把每個數字 d 換成 「shift(s[i-1], d)」 字母。
// 為何用 replace: 雖然 operator[] 寫入也可,我們示範 replace 單字元取代。
// -----------------------------------------------------------------------------
std::string replaceDigits(std::string s) {
    for (size_t i = 1; i < s.size(); i += 2) {
        char ch = s[i - 1] + (s[i] - '0');
        s.replace(i, 1, 1, ch);
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Log 機敏資料遮蔽 (password=xxx → password=[REDACTED])
// 為何用 replace: 把符合 pattern 的 value 替換成 [REDACTED],避免 log 洩漏。
//                 PCI / GDPR / 內部稽核都會要求做這件事。
// -----------------------------------------------------------------------------
std::string redactSecrets(std::string line, const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        std::string needle = key + "=";
        size_t p = 0;
        while ((p = line.find(needle, p)) != std::string::npos) {
            size_t valStart = p + needle.size();
            // 找這個值的結尾(空白或結尾)
            size_t valEnd = line.find_first_of(" &\n", valStart);
            if (valEnd == std::string::npos) valEnd = line.size();
            line.replace(valStart, valEnd - valStart, "[REDACTED]");
            p = valStart + 10;     // strlen("[REDACTED]")
        }
    }
    return line;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1page. 1page0. Replace Question Marks - 簡化版
// 題目: LeetCode 2696 變奏 ── 把所有 '?' 替換為 'a' (簡化規則)
// 為何用 replace: 用 size_t pos + replace(pos, 1, "a") 處理每個 '?'。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
std::string replaceQuestionMarks(std::string s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '?') s.replace(i, 1, "a");
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】簡易 templating: 把 "Hello {name}!" 中的 {name} 替換為實際值
// 為何用 replace: 找到 {key} 範圍後一次性 replace 為 value。
// -----------------------------------------------------------------------------
std::string renderTemplate(std::string tmpl,
                            const std::vector<std::pair<std::string,std::string>>& vars) {
    for (auto& [k, v] : vars) {
        std::string needle = "{" + k + "}";
        size_t p = 0;
        while ((p = tmpl.find(needle, p)) != std::string::npos) {
            tmpl.replace(p, needle.size(), v);
            p += v.size();
        }
    }
    return tmpl;
}

int main() {
    demoReplace();
    std::cout << "\n=== replaceAll ===\n";
    std::cout << replaceAll("foo bar foo bar", "foo", "BAZ") << "\n";

    std::cout << "\n=== LeetCode 1108 ===\n";
    std::cout << defangIPaddr("1.1.1.1")          << "\n";   // 1[.]1[.]1[.]1
    std::cout << defangIPaddr("255.100.50.0")     << "\n";   // 255[.]100[.]50[.]0

    std::cout << "\n=== LeetCode 1844 ===\n";
    std::cout << replaceDigits("a1c1e1") << "\n";   // "abcdef"

    std::cout << "\n=== LeetCode 2696 變奏 (replaceQuestionMarks) ===\n";
    std::cout << replaceQuestionMarks("a?b?c") << "\n";   // aabac

    std::cout << "\n=== 日常實務: log 遮蔽 ===\n";
    std::cout << redactSecrets("user=alice password=hunter2 token=abc123",
                               {"password", "token"}) << "\n";

    std::cout << "\n=== 日常實務: renderTemplate ===\n";
    std::cout << renderTemplate("Hello {name}, you have {n} messages.",
                                {{"name", "Alice"}, {"n", "5"}}) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:replace 與「erase 後再 insert」效能差多少?
    //    A:replace 只執行一次 memmove (尾段往左或往右搬一次到正確位置),
    //      erase+insert 會搬兩次 (erase 把尾段往前、insert 又把它往後)。
    //      replace 還提供 strong exception guarantee,失敗時字串不變;
    //      erase+insert 中途失敗會留下半改的狀態。永遠優先 replace。
    //
    //  Q2:為什麼 newLen == oldCount 是最快的 replace?
    //    A:此時新內容剛好填滿被刪掉的洞,不需要搬移後段字元、也不會
    //      改變 size,實作直接退化為「memcpy 到 [pos, pos+count)」。
    //      IP defang 變相為 1→3 字元 (多搬),但若把 '.' 換成 '_' 就是
    //      原地覆寫,速度可達數 GB/s。
    //
    //  Q3:replaceAll 為何要 p += to.size() 而不是 p++ 或 p = 0?
    //    A:p++ 會在 to 包含 from 時 (例如 from="a", to="aa") 持續找到
    //      剛插入的字元,造成無窮迴圈。p = 0 重新從頭找等於把演算法變成
    //      O(N^2) 且也仍可能無窮迴圈。p += to.size() 跳過新內容,確保
    //      O(N) 且正確。也要注意 from.empty() 必須先排除。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra replace.cpp -o replace

// === 預期輸出 (節錄) ===
// === LeetCode 2696 變奏 (replaceQuestionMarks) ===
// aabac
// === 日常實務: renderTemplate ===
// Hello Alice, you have 5 messages.
