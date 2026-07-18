// =============================================================================
// 檔名: push_back.cpp
// 主題: std::string::push_back (在尾端附加單個字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/push_back
//   - https://cplusplus.com/reference/string/string/push_back/
// =============================================================================
//
// 【函式資訊 Information】
//   void push_back(char ch);
//
// 參數: ch - 要附加的字元
// 回傳: 無
// 例外: 僅在 reallocation 失敗時丟 std::bad_alloc;否則 noexcept-like。
//       提供 strong exception guarantee — 失敗時字串內容不變。
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 它是什麼?
//    -----------------------------------------------------------------
//    push_back 在字串尾端附加一個字元,使 size() 增加 1、結尾 '\0'
//    自動往後移。語意上等價於:
//      - operator+=(ch)
//      - append(1, ch)
//      - insert(end(), ch)
//      - resize(size()+1, ch) (僅在新長度確實寫入該字元時)
//    這四種寫法在底層幾乎產生相同的程式碼,但 push_back 是 idiomatic、
//    意圖最清楚的寫法,也與 vector / deque 的命名一致。
//
// 2. 為什麼存在 push_back? — 容器介面統一
//    -----------------------------------------------------------------
//    std::vector、std::list、std::deque、std::string 全部都有 push_back。
//    這讓泛型程式碼可以寫:
//        template<class C> void appendChar(C& c, char ch) { c.push_back(ch); }
//    無論 C 是 string、vector<char>、deque<char> 都能用。「容器無關」的
//    程式碼在演算法、迭代器轉換 (例如 std::back_inserter(s)) 中很重要。
//
// 3. 時間複雜度:Amortized O(1)
//    -----------------------------------------------------------------
//    單次 push_back 的成本:
//      - 若 size < capacity: O(1) — 只是 buffer[size++] = ch;
//      - 若 size == capacity: O(N) — 觸發 reallocation,要拷貝整個字串。
//    乍看之下最壞 O(N),但攤銷分析 (amortized analysis) 顯示 N 次連續
//    push_back 的總成本是 O(N),平均每次 O(1)。原理見「概念補充」。
//
// 4. 與 += / append 的差異
//    -----------------------------------------------------------------
//    語意完全等價,但:
//      - push_back(ch) 只接受單字元,意圖最明確;
//      - += 可接字元、字串、string_view、initializer_list,泛用;
//      - append 是「明確說我要附加」,可串接 .append(...).append(...)。
//    寫程式時若只是加單字元,push_back 最 idiomatic;若要附加字串請用
//    += 或 append。
//
// 5. 例外安全 (Exception Safety)
//    -----------------------------------------------------------------
//    Strong guarantee:
//      - 若 reallocation 過程 bad_alloc,原字串完全不變;
//      - 若 reallocation 成功但中間字元拷貝丟例外 (對 char 不可能),
//        實作仍會 rollback。
//    對 char 而言其實永遠不會丟例外 (只可能 bad_alloc),所以實務上
//    push_back 幾乎可視為 noexcept。
//
// 6. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    - 若觸發 reallocation:全部失效;
//    - 若沒觸發:end() 失效 (位置往後挪),其他仍有效。
//    結論:不要把 c_str() 結果存起來再 push_back,除非你確定不會擴容。
//
// 7. push_back vs operator+=(char) 的細微差別
//    -----------------------------------------------------------------
//    - 兩者語意完全相同,效能也一樣;
//    - operator+= 回傳 string& 以支援鏈式 (s += 'a' += 'b' 不合法,
//      因為 char 不能 += 給 string,但 (s+='a') += 'b' 合法);
//    - push_back 回傳 void,不能鏈式 — 這反而是優點:強制單行寫法,
//      可讀性高。
//
// 8. push_back('\0') 的特殊情況
//    -----------------------------------------------------------------
//    可以呼叫 push_back('\0') — 它會把一個 NUL 字元加入字串中:
//        s = "abc"; s.push_back('\0'); s.push_back('d');
//        s.size() == 5  (內容: a b c \0 d)
//        s.c_str()[3] == '\0'   (你 push 進去的)
//        s.c_str()[s.size()] == '\0'  (字串本身的結尾哨兵)
//    但若把這個 string 傳給 C API 用 c_str(),C API 看到的是 "abc"
//    (在第一個 '\0' 處停)。因此「字串中含 '\0'」對 C-style API 是危險的,
//    但對 STL 函式 (size、find、操作 byte 為單位) 完全沒問題。
//
// 9. C++11 / 17 / 20 / 23 的演進
//    -----------------------------------------------------------------
//    - C++11: 規格明確為 amortized O(1)。
//    - C++17/20: constexpr 友善度。
//    - C++23: 對 constexpr string 而言 push_back 也是 constexpr。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 push_back 是「攤銷 (amortized) O(1)」?
//     設成長係數 k = 2 (capacity 每次擴張一倍)。對 N 次 push_back:
//       - 總共會發生 log2(N) 次 reallocation;
//       - 第 i 次 reallocation 拷貝 2^i 個字元;
//       - 拷貝總成本: 1 + 2 + 4 + ... + N/2 + N = 2N - 1;
//       - 加上 N 次純粹的 buffer 寫入 = 3N - 1;
//       - 平均每次 push_back 成本 = (3N-1)/N ≈ 3 = O(1)。
//     這就是「攤銷分析」— 雖然個別操作可能很貴,但平均下來是常數時間。
//     reserve 的價值在於「直接跳到 N 大小」,連那 2N - 1 的拷貝都省了。
//
// (B) 為什麼用 string 模擬 stack 是常見技巧?
//     - back() 是 O(1) 取頂端;
//     - push_back / pop_back 是 amortized O(1);
//     - 整體可當 char-stack 用,且 string 本身就是「結果」(可直接回傳);
//     - 比 std::stack<char> 寫法簡單,還免去最後組裝步驟。
//     LeetCode 上「括號匹配」、「相鄰消去」、「monotonic stack」型題目
//     大量採用此手法。push_back/pop_back 與 stack 的 push/top/pop
//     語意完全對應。
//
// (C) push_back 與 std::back_inserter
//     STL 演算法配合 std::back_inserter(s) 可生出 OutputIterator,內部
//     會呼叫 s.push_back(...)。例如:
//         std::copy_if(src.begin(), src.end(),
//                      std::back_inserter(s),
//                      [](char c){ return std::isalpha((unsigned char)c); });
//     這在 STL 風格的程式中非常常用。
//
// (D) 與 vector 的差異 (細微)
//     vector::push_back 對 non-trivial type 可能 throw (move/copy ctor),
//     需要 strong guarantee 的「nothrow move」優化策略;
//     string::push_back 處理的是 char,純 POD,所以實作簡單很多 — 唯一可
//     能丟的例外只有 bad_alloc。
//
// (E) 何時不該 push_back?
//     - 已知總長度時,改用 reserve + 直接寫入 (避免 capacity 增長計算);
//     - 大量字元的情況改用 append 一次塞 (內部 memcpy 比迴圈 push_back 快);
//     - 在 hot loop 中對短字串 push,如果每次都 reallocate,可能有
//       30% 以上的成本是 capacity 增長的判斷邏輯,reserve 一次就好。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. push_back 觸發 reallocation 時,所有迭代器 / 指標 / 參考會失效。
// 2. 大量 push_back 前先 reserve(預估大小)可避免反覆 realloc,效能差數倍。
// 3. push_back('\0') 是合法的 — 字串中可含 NUL,但對 c_str() 行為要小心。
// 4. 與 += / append(1, ch) / insert(end(), ch) 完全等價,但 push_back 最 idiomatic。
// 5. 例外安全: strong guarantee — bad_alloc 失敗時字串不變。
// =============================================================================

/*
補充筆記：std::string::push_back
  - std::string::push_back 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::push_back 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoPushBack() {
    std::string s;
    for (char c = 'a'; c <= 'e'; ++c) s.push_back(c);
    std::cout << s << "\n";                     // "abcde"
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1768. Merge Strings Alternately (Easy)
// 題目: 給兩個字串 word1 和 word2,從 word1 開始交替合併。
//       若一字串較長,將剩餘部分附加在合併後的字串末尾。
//       例如 word1 = "abc", word2 = "pqr" → "apbqcr"
//       例如 word1 = "ab",  word2 = "pqrs" → "apbqrs"
// 為何用 push_back: 雙指標逐字元交替 push_back,簡潔自然。
//                   reserve 預估總長 = word1.size() + word2.size() 避免 realloc。
// 思路: i 從兩字串各取一字元 push_back,直到任一耗盡;剩下的整段 append 即可。
// 複雜度: O(n + m)。
// -----------------------------------------------------------------------------
std::string mergeAlternately(const std::string& word1, const std::string& word2) {
    std::string result;
    result.reserve(word1.size() + word2.size());            // 一次配足
    size_t i = 0, j = 0;
    while (i < word1.size() && j < word2.size()) {
        result.push_back(word1[i++]);                       // 從 word1 取
        result.push_back(word2[j++]);                       // 從 word2 取
    }
    // 任一字串還有剩餘,直接附加
    while (i < word1.size()) result.push_back(word1[i++]);
    while (j < word2.size()) result.push_back(word2[j++]);
    return result;
}

// -----------------------------------------------------------------------------
// 【LeetCode 補充範例】LeetCode 22. Generate Parentheses (Medium)
// 題目: 給 n,生成所有 n 對合法括號的組合 (例如 n=3 → "((()))"、"(()())"...)
// 為何用 push_back: 回溯時 push_back 加字元,回溯前 pop_back 還原 — 經典樣板。
//                   是「string 當 stack」最教科書的用法。
// 思路: 維持「已用左括號數 open、已用右括號數 close」,只要 open<n 可加 '(',
//       只要 close<open 可加 ')'。當長度達 2n 收集結果。
// 複雜度: O(C_n * n) (Catalan 數)。
// -----------------------------------------------------------------------------
#include <vector>
void backtrack(std::vector<std::string>& res, std::string& cur,
               int open, int close, int n) {
    if (static_cast<int>(cur.size()) == 2 * n) {
        res.push_back(cur);                     // 收結果
        return;
    }
    if (open < n) {
        cur.push_back('(');
        backtrack(res, cur, open + 1, close, n);
        cur.pop_back();                         // 回溯,恢復 cur
    }
    if (close < open) {
        cur.push_back(')');
        backtrack(res, cur, open, close + 1, n);
        cur.pop_back();
    }
}

std::vector<std::string> generateParenthesis(int n) {
    std::vector<std::string> res;
    std::string cur;
    cur.reserve(2 * n);                         // 最終長度確定為 2n
    backtrack(res, cur, 0, 0, n);
    return res;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】把整數轉為固定寬度的二進位字串
// 為何用 push_back: 從高位到低位逐 bit append。Debug 旗標、權限位元、二進位
//                   通訊協定都會這樣印出來。
// -----------------------------------------------------------------------------
std::string toBinary(unsigned value, int bits) {
    std::string s;
    s.reserve(bits);                            // 已知長度
    for (int i = bits - 1; i >= 0; --i) {
        s.push_back(((value >> i) & 1) ? '1' : '0');
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 2696. Minimum String Length After Removing Substrings
// 題目: 給字串 s,可不斷移除 "AB" 或 "CD" 子字串;求最終最小長度。
// 為何用 push_back: 用 stack 概念,若新進字元能與 stack 頂組成 AB/CD 就 pop,
//                    否則 push_back。string 當 char-stack 的經典用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int minStringLength(const std::string& s) {
    std::string st;
    for (char c : s) {
        if (!st.empty() &&
            ((st.back() == 'A' && c == 'B') ||
             (st.back() == 'C' && c == 'D'))) {
            st.pop_back();
        } else {
            st.push_back(c);
        }
    }
    return static_cast<int>(st.size());
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】簡易 base64 編碼 alphabet 反查 (建表)
// 為何用 push_back: 一個字元一個字元加入 alphabet 字串,reserve 後 O(N)。
// (僅示範 push_back 場景,完整 base64 編碼省略 padding 處理)
// -----------------------------------------------------------------------------
std::string buildBase64Alphabet() {
    std::string a;
    a.reserve(64);
    for (char c = 'A'; c <= 'Z'; ++c) a.push_back(c);
    for (char c = 'a'; c <= 'z'; ++c) a.push_back(c);
    for (char c = '0'; c <= '9'; ++c) a.push_back(c);
    a.push_back('+');
    a.push_back('/');
    return a;
}

int main() {
    demoPushBack();

    std::cout << "\n=== LeetCode 1768 ===\n";
    std::cout << mergeAlternately("abc", "pqr") << "\n";    // "apbqcr"
    std::cout << mergeAlternately("ab",  "pqrs") << "\n";   // "apbqrs"

    std::cout << "\n=== LeetCode 22 (n=3) ===\n";
    for (auto& s : generateParenthesis(3)) std::cout << s << "\n";

    std::cout << "\n=== LeetCode 2696 ===\n";
    std::cout << minStringLength("ABFCACDB") << "\n";   // 2
    std::cout << minStringLength("ACBBD")    << "\n";   // 5

    std::cout << "\n=== 日常實務: 二進位字串 ===\n";
    std::cout << toBinary(0b10110100, 8) << "\n";           // 10110100
    std::cout << toBinary(0755, 9)       << "\n";           // 111101101 (Unix 權限)

    std::cout << "\n=== 日常實務: base64 alphabet ===\n";
    std::cout << buildBase64Alphabet() << "\n";   // ABC..XYZabc..xyz012..9+/

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:push_back(c) 與 append(1, c) 與 += c 有效能差別嗎?
    //    A:語意完全等價,實作上幾乎產生相同程式碼,效能差距可忽略。
    //      idiomatic 上「加單一 char」首選 push_back;字串/序列就用
    //      append 或 +=。在 hot loop 中真正能省的是「事先 reserve」。
    //
    //  Q2:N 次 push_back 的攤銷 O(1) 是怎麼證明的?
    //    A:成長係數 k=2 時,N 次 push 共觸發 log2(N) 次 reallocate,
    //      搬移總量 1+2+4+...+N/2+N = 2N-1。加上 N 次純寫入,共 3N-1,
    //      平均每次成本 = 3N-1/N ≈ O(1)。reserve(N) 可消除那 2N-1 的搬移。
    //
    //  Q3:push_back('\0') 合法嗎?後果是什麼?
    //    A:合法。string 內可以含 '\0',size() 仍正確、find/operator[]
    //      行為正常。但傳給 c_str() 的 C API 只會看到第一個 '\0' 之前的
    //      內容,因為 C-style API 以 NUL 為終止。要用 data() + size()
    //      傳給以長度為界的 API (例如 std::string_view 或 write())。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra push_back.cpp -o push_back

// === 預期輸出 (節錄) ===
// === LeetCode 2696 ===
// 2
// 5
// === 日常實務: base64 alphabet ===
// ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
