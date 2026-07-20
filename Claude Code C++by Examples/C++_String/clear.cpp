// =============================================================================
// 檔名: clear.cpp
// 主題: std::string::clear (清空字串內容,保留 capacity)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/clear
//   - https://cplusplus.com/reference/string/string/clear/
// =============================================================================
//
// 【函式資訊 Information】
//   void clear() noexcept;          // C++11 起 noexcept
//
// 參數: 無
// 回傳: 無
// 例外: noexcept — 絕不丟例外。
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 它做了什麼?
//    -----------------------------------------------------------------
//    把 size() 設為 0,字串變成空字串 "":
//      - size() == 0
//      - empty() == true
//      - data()[0] == '\0' (結尾哨兵保留)
//      - capacity() *不變* (這是與 vector 一致的關鍵設計)
//      - 不會釋放 heap buffer
//
//    語意上等價於:
//      - erase(begin(), end())
//      - erase(0, npos)
//      - assign("")
//      - assign(0, '\0')
//    但 clear() 是「最直接、最有意圖性」的寫法,編譯器最容易優化。
//
// 2. 為什麼 clear 不釋放 capacity?
//    -----------------------------------------------------------------
//    這是 vector / string / deque 統一的設計哲學:
//      "Reuse, don't release."
//    在迴圈中重複使用同一個物件處理多筆輸入時,保留 buffer 可以省下
//    每次的 malloc/free 成本。例如:
//        std::string buf;
//        buf.reserve(4096);
//        while (read_line(buf)) {       // read_line 內部會先 buf.clear()
//            process(buf);
//        }
//    這個模式只配置一次記憶體,完全不會在迴圈內 allocate。
//    若想連 capacity 一起釋放,有以下選項:
//      - s.shrink_to_fit() (請求,non-binding);
//      - s = std::string();   (賦值新空字串,通常會釋放);
//      - std::string().swap(s); (C++03 的 swap idiom);
//      - 讓物件 destroy 然後重新建立。
//
// 3. 時間複雜度
//    -----------------------------------------------------------------
//    C++11 起為 O(1):實作只要把 size 設為 0、把 buffer[0] 設為 '\0'。
//    C++03 規格寫的是 "constant or linear" — 某些舊實作會線性掃過呼叫
//    char 的解構子 (對 string 而言是 trivially-destructible,所以實際
//    仍是 O(1))。現代實作一律 O(1)。
//
// 4. noexcept 保證的價值
//    -----------------------------------------------------------------
//    clear() noexcept 意味著它可以安全用在:
//      - destructor 內 (例如 reset 內部狀態);
//      - catch block 內 (錯誤恢復路徑);
//      - move constructor / move assignment 內 (源物件清空);
//      - terminate handler 內。
//    而不需要擔心「清空 string 反而再丟一個例外導致 std::terminate」。
//
// 5. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    呼叫 clear() 後:
//      - 所有迭代器、指標、參考「在標準層面」都失效;
//      - 但實際上,因為 buffer 沒被釋放,c_str() / data() 回傳的指標
//        所指向的記憶體仍存活 — 你只是不該再用它讀取「之前」的內容
//        (那塊記憶體現在邏輯上是空的)。
//      - end() 與 begin() 變成同一個位置 (都在 buffer[0])。
//    結論:呼叫 clear() 後請重新取得迭代器。
//
// 6. clear vs erase 的差異
//    -----------------------------------------------------------------
//    erase(0, npos) 與 clear() 行為相同,但:
//      - clear 是「整個清空」的 idiomatic 寫法,讀者一眼就懂意圖;
//      - erase 適合「清空一部分」(指定 index 與 count)。
//    寫程式時請依「想表達的語意」選擇 — 不是只看效果。
//
// 7. C++11 / 17 / 20 / 23 的演進
//    -----------------------------------------------------------------
//    - C++11: 加上 noexcept、複雜度明確為 O(1)。
//    - C++17/20: 行為不變,持續強化 constexpr 友善度。
//    - C++23: 對 constexpr string 而言 clear 也是 constexpr。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) 「Buffer reuse」設計模式
//     在高效能 I/O / 解析器 / 序列化器中,「同一個 buffer 反覆利用」
//     是減少 allocator pressure 的關鍵手法。常見搭配:
//         buffer.reserve(預估上限);  // 一次配足
//         while (有資料) {
//             buffer.clear();         // 邏輯上重置,實體 buffer 保留
//             fill_buffer(buffer);
//             process(buffer);
//         }
//     這個 pattern 在 protobuf 解析、JSON 序列化、HTTP server 接收
//     裡無所不在。clear() noexcept O(1) 的特性是這個 pattern 成立的基石。
//
// (B) clear vs erase vs resize(0)
//     三者最終效果都是 size == 0,但語意與成本略有差異:
//       - clear():       O(1)、noexcept、最 idiomatic;
//       - erase(0,npos): 與 clear 相同,但寫法繞;
//       - resize(0):     等價,但 resize 還會在「擴大」時填字元,語意更廣;
//       - assign(""):    O(1) 但要解析 const char* (loop 找 '\0');
//       - assign(0,'x'): O(1)。
//     全部都不釋放 capacity。記憶釋放請額外 shrink_to_fit。
//
// (C) clear 後的安全範圍
//     呼叫 clear() 後,以下操作仍然安全:
//       - empty() / size() / capacity()
//       - operator[](0) → 讀到 '\0' (不是 UB);
//       - c_str() → 回傳指向 "" 的合法指標;
//       - data() → 同上;
//       - += / push_back / append → 開始重建內容,buffer 重用。
//     不安全:
//       - 使用 clear 之前取得的 iterator / pointer / reference (UB)。
//
// (D) 與 stringstream 的對比
//     某些初學者用 std::stringstream 累積字串、最後 .str()。問題:
//     stringstream 內部仍是 string + 額外狀態 (位置、flags),成本更高。
//     若只是要拼字串,直接用 string + reserve + clear 重用,效能更好、
//     程式碼更簡單。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. clear() 後 capacity() 通常不變,適合「重複使用同一個 string buffer」。
// 2. 若要連同 capacity 一起釋放,需配合 shrink_to_fit() 或重新建立物件。
// 3. clear() noexcept,絕不丟例外 — 可安全用在 destructor / catch 內。
// 4. 所有先前取得的迭代器 / 指標 / 參考在 clear 後失效。
// 5. 與 erase(0, npos) 等價,但語意更清楚,建議優先使用 clear。
// =============================================================================

/*
補充筆記：std::string::clear
  - std::string::clear 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::clear 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
  - std::string 的索引型別是 size_type；和 int 混用時要小心 unsigned underflow，尤其是倒著迴圈或拿 npos 比較。
  - 修改字串內容或容量可能讓先前取得的 iterator、reference、pointer、c_str() 指標失效。
  - operator[] 不做範圍檢查，at() 越界會丟 std::out_of_range；教學範例可比較兩者，工作程式要依錯誤處理需求選。
  - find/rfind/find_first_of 這類搜尋函式失敗回傳 std::string::npos；判斷時應和 npos 比，不要寫成 >= 0。
  - 字串和 C string 互通時要記得 null terminator；std::string 可以保存內含 \0 的資料，但很多 C API 會在第一個 \0 停止。
  - 若只是傳入唯讀文字片段且不需要擁有資料，std::string_view 可避免複製；但它不能延長原字串生命週期。
  - 處理中文或 UTF-8 時，std::string 的 size() 回傳 byte 數，不是人眼看到的字元數。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::string::clear
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. clear() 做了什麼?它會釋放記憶體嗎?
//     答:把 size() 設為 0(empty() 變 true、結尾 '\0' 保留),
//         但不會歸還已配置的 heap buffer。值得注意的是:標準對
//         basic_string::clear 並未明文要求 capacity() 不變,
//         不過現有實作都不改變 capacity。所以「清空內容」與
//         「釋放記憶體」是兩件不同的事,別把 clear() 當成後者。
//     追問:那 clear() 是 noexcept 嗎?→ 是,C++11 起標為 noexcept;
//         它只改長度、不配置記憶體,沒有失敗的理由。
//
// 🔥 Q2. 那要怎樣才真的把記憶體還回去?
//     答:兩條路 ——
//         (1) s.clear(); s.shrink_to_fit();  但 shrink_to_fit 是
//             「非約束性請求」(non-binding),標準允許實作完全忽略它,
//             所以不保證一定釋放。
//         (2) swap trick:std::string().swap(s);  用一個空的臨時物件
//             跟 s 交換,s 拿到臨時物件的最小狀態,原本的大 buffer 隨
//             臨時物件在該表達式結束時解構而釋放。這是 C++11 之前的
//             慣用手法,至今仍是唯一能確定生效的方式。
//     追問:為什麼標準不乾脆保證 shrink_to_fit 一定縮?→ 縮小需要重新配置
//         並複製,可能丟例外、也可能反而更慢,標準把取捨留給實作。
//
// ⚠️ 陷阱. 迴圈裡重複用同一個 string 當 buffer,每輪呼叫 clear() —— 這樣好嗎?
//     答:這其實是好事,而且正是 clear() 保留 capacity 的用意:第一輪之後
//         buffer 已經夠大,後續各輪不必再配置,是常見的效能優化寫法。
//         真正要小心的是相反情境 —— 某輪意外塞進一個超大字串後,
//         之後即使 clear() 了,那塊大 buffer 仍會一直佔著不還。
//     為什麼會錯:很多人直覺認為「clear() 之後記憶體就回收了」,
//         於是在長生命週期的物件上放了一個曾經爆量的 string,造成看不見的
//         記憶體滯留。判斷方式是印出 capacity(),而不是看 size()。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

void demoClear() {
    std::string s = "Hello";
    std::cout << "before: \"" << s << "\", capacity=" << s.capacity() << "\n";

    s.clear();
    std::cout << "after : \"" << s << "\", size=" << s.size()
              << ", capacity=" << s.capacity() << " (capacity 通常不變)\n";
}

// -----------------------------------------------------------------------------
// 【實務範例】重複使用同一個 buffer 處理多筆輸入
// 為何用 clear: 在迴圈中讀入多筆資料,清掉舊內容但保留 capacity,
//               大幅減少記憶體配置次數。經典的 buffer-reuse 模式。
// -----------------------------------------------------------------------------
#include <vector>
std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string buffer;
    buffer.reserve(64);                         // 預估每行長度,一次配足

    for (char c : text) {
        if (c == '\n') {
            lines.push_back(buffer);            // 拷貝進 vector
            buffer.clear();                     // 不釋放 capacity,下次直接用
        } else {
            buffer += c;
        }
    }
    if (!buffer.empty()) lines.push_back(buffer);
    return lines;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1071. Greatest Common Divisor of Strings (Easy)
// 題目: 找兩字串的最大共同字串。若 s1 + s2 == s2 + s1,則 GCD 存在,
//       長度為 gcd(len(s1), len(s2)),即 s1 的前 gcd 個字元。
// 為何用 clear: 雖然此題本身不一定要清空 buffer,但展示「字串容器處理」
//               的常見模式 — 在驗證候選答案時可能需要清空暫存。
// 思路: 數學性質 — 若兩字串可由同一個底字串重複組成,順序無關,故 s1+s2 == s2+s1。
// 複雜度: O(n + m)。
// -----------------------------------------------------------------------------
std::string gcdOfStrings(const std::string& s1, const std::string& s2) {
    if (s1 + s2 != s2 + s1) return "";          // 不可由同一底字串組成
    auto gcd = [](int a, int b) {
        while (b) { int t = b; b = a % b; a = t; }
        return a;
    };
    int len = gcd(static_cast<int>(s1.size()), static_cast<int>(s2.size()));
    return s1.substr(0, len);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】可重複使用的字串處理器
// 為何用 clear: 在處理多筆訊息(多個 Slack message、多筆 protobuf 解碼)時,
//               重複建立 string 會頻繁配置記憶體;clear() + reserve 一次,之後
//               每筆訊息都重複利用同一塊 buffer,效能好很多。
// -----------------------------------------------------------------------------
class LineProcessor {
    std::string buffer;
public:
    LineProcessor() { buffer.reserve(256); }    // 一次配足,終身複用

    // 將輸入轉成大寫並回傳 — 內部 buffer 重複使用
    std::string process(const std::string& line) {
        buffer.clear();                         // 不釋放 capacity
        for (char c : line) {
            if (c >= 'a' && c <= 'z') buffer += static_cast<char>(c - 'a' + 'A');
            else                      buffer += c;
        }
        return buffer;                          // 回傳拷貝;buffer 保留 capacity 供下次用
    }
};

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1684. Count the Number of Consistent Strings
// 題目: allowed 是允許的字元集,計算 words 內所有字元都在 allowed 內的字串數。
// 為何用 clear: 用一個重用 buffer 暫存 allowed 字元,每換一個 testcase 就 clear。
//                示範 buffer 重用模式。
// 複雜度: O(總長度)。
// -----------------------------------------------------------------------------
int countConsistentStrings(const std::string& allowed, const std::vector<std::string>& words) {
    bool ok[128] = {false};
    for (unsigned char c : allowed) ok[c] = true;
    int cnt = 0;
    std::string bad;
    bad.reserve(16);
    for (const auto& w : words) {
        bad.clear();                          // 重用 bad buffer
        for (unsigned char c : w) if (!ok[c]) { bad.push_back(c); break; }
        if (bad.empty()) ++cnt;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】回應緩衝區 (Response buffer) 重置
// 為何用 clear: HTTP server worker 在處理完一個 request 後,response buffer
//                必須清空但保留 capacity 等下一個 request,clear() 是標配。
// -----------------------------------------------------------------------------
class ResponseWriter {
    std::string buf;
public:
    ResponseWriter() { buf.reserve(4096); }
    void reset()                       { buf.clear(); }    // 不釋放 capacity
    void write(const std::string& s)   { buf += s; }
    const std::string& body() const    { return buf; }
};

int main() {
    demoClear();
    std::cout << "\n=== splitLines ===\n";
    for (auto& l : splitLines("a\nbb\nccc")) std::cout << "[" << l << "]\n";

    std::cout << "\n=== LeetCode 1071 ===\n";
    std::cout << "\"" << gcdOfStrings("ABCABC", "ABC") << "\"\n"; // "ABC"
    std::cout << "\"" << gcdOfStrings("LEET", "CODE") << "\"\n";  // ""

    std::cout << "\n=== LeetCode 1684 ===\n";
    std::cout << countConsistentStrings("ab", {"ad","bd","aaab","baa","badab"}) << "\n";  // 2
    std::cout << countConsistentStrings("abc", {"a","b","c","ab","ac","bc","abc"}) << "\n"; // 7

    std::cout << "\n=== 日常實務: 重用 buffer ===\n";
    LineProcessor lp;
    std::cout << lp.process("hello") << "\n";
    std::cout << lp.process("WoRlD") << "\n";

    std::cout << "\n=== 日常實務: ResponseWriter ===\n";
    ResponseWriter rw;
    rw.write("HTTP/1.1 200 OK\n");
    rw.write("Content-Type: text/plain\n\n");
    rw.write("hello");
    std::cout << "[" << rw.body() << "]\n";
    rw.reset();
    std::cout << "after reset: size=" << rw.body().size() << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：clear() 會釋放 capacity (heap buffer) 嗎?
    //    A：不會。這是 STL 容器一致的「Reuse, don't release」哲學。clear()
    //       後 size() == 0 但 capacity() 維持原值,heap buffer 不釋放。要
    //       連 capacity 一起放掉:std::string().swap(s) (C++03) 或
    //       s.shrink_to_fit() (C++11,但是 non-binding 請求,實作可拒絕)。
    //
    //  Q2：clear()、erase()、assign("") 三者效能與語意有差別嗎?
    //    A：語意等價,但編譯器最容易把 clear() 優化成「size = 0; buf[0]='\0'」
    //       兩條指令。erase(0, npos) 與 erase(begin(), end()) 多一層判斷。
    //       assign("") 則可能多走 strlen(空字串) 的開銷。寫 clear() 最直接、
    //       意圖最清楚、優化最徹底。
    //
    //  Q3：clear() 是否 noexcept?C++03 與 C++11 後的差別?
    //    A：C++11 起明文 noexcept,且時間複雜度 O(1)。C++03 規格只寫
    //       "constant or linear",某些古老實作會走訪所有元素呼叫 destructor
    //       (對 char 是 trivial,實際仍 O(1))。現代實作一律 O(1) noexcept,
    //       可在 destructor 或 noexcept 函式內安全呼叫。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra clear.cpp -o clear

// === 預期輸出 (節錄) ===
// === LeetCode 1684 ===
// 2
// 7
// === 日常實務: ResponseWriter ===
// [HTTP/1.1 200 OK
// Content-Type: text/plain
//
// hello]
// after reset: size=0
