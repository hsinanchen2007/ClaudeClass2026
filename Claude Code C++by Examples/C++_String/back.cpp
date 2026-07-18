// =============================================================================
// 檔名: back.cpp
// 主題: std::string::back (取得最後一個字元)
// 參考連結 References:
//   cppreference: https://en.cppreference.com/cpp/string/basic_string/back
//   cplusplus.com: https://cplusplus.com/reference/string/string/back/
// =============================================================================
//
// 【函式資訊 Information】
//   reference       back();
//   const_reference back() const;
//
// 等同於 operator[](size() - 1)。
//
// =============================================================================
//
// 【詳細解釋 Explanation】
//
// (一) back() 為何重要?
// ----------------------------------------------------------------------------
// 「最後一個字元」在字串處理中的場景出奇地多:
//   - 判斷檔名副檔名 (filename.back() == 's' 等)
//   - 偵測尾端的換行 / CRLF / 空白
//   - 模擬 stack 的 top (push_back / pop_back / back 三件套)
//   - 判斷字串是否以特定字元結尾 (C++20 之後可用 ends_with 更直觀)
// 沒有 back(),你得寫 s[s.size() - 1],這既冗長又容易在 size==0 時出錯
// (size_t 是 unsigned,0-1 會 underflow 變成超大值,索引立刻 UB)。
//
// (二) 與相關 API 的差異
// ----------------------------------------------------------------------------
//   操作                  | 空字串時的行為              | 額外資訊
//   ----------------------|-----------------------------|---------------
//   back()                | Undefined Behavior          | 直接讀末端
//   *(end()-1)            | UB (空字串 end() 不可 -1)   | 與 back 等價
//   at(size()-1)          | UB (size_t underflow!)      | 不要這樣寫
//   c_str()[size()-1]     | UB (空字串 underflow)       | 與 back 等價
//   ends_with("x") (C++20)| 回傳 false                  | 安全且語意更清楚
//
// 結論:當你只需「最後一個 char」用 back();當你要「比對結尾字串/字元」
// 而想避免空字串檢查,改用 ends_with 更安全。
//
// (三) back() 不會回傳結尾的 '\0'
// ----------------------------------------------------------------------------
// std::string 在 C++11 起保證內部 buffer 結尾有一個 '\0',但這個 '\0'
// 不算 size 內的字元。所以:
//   std::string s = "Hi";    // size() == 2,buffer 內為 'H' 'i' '\0'
//   s.back();                // 回傳 'i',不是 '\0'
// 若要拿 '\0',要用 s[s.size()] 或 s.c_str()[s.size()],但實務上幾乎沒人
// 需要這樣做。
//
// (四) 與 push_back / pop_back 的搭配
// ----------------------------------------------------------------------------
// std::string 也支援像 vector 一樣的「stack-like」操作:
//   s.push_back('A');   // 接到尾端
//   char top = s.back();// 看末端
//   s.pop_back();       // 移除末端
// 這讓 string 可以直接當 stack<char> 用,介面更輕、效能更好 (避免 deque
// 內部分塊配置的 overhead)。
//
// (五) 時間複雜度
// ----------------------------------------------------------------------------
// O(1) — 直接以 buffer + size - 1 取得位址。
//
// (六) C++ 標準演進
// ----------------------------------------------------------------------------
//   C++11 起新增 (與 front() 一同進來)。
//   在這之前要寫 s[s.size() - 1],容易出錯。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// 1) size_t underflow 的常見地雷
//    std::string s;          // size() == 0
//    size_t i = s.size() - 1; // i 變成 SIZE_MAX (約 1.8e19)
//    這是 C++ 初學者最常踩的地雷之一。改用 back() (帶 empty 檢查) 或
//    ends_with 都比手算 size() - 1 安全。
//
// 2) Stack-like 操作的時間複雜度
//    string 的 back / push_back / pop_back 都是 amortized O(1)。
//    push_back 偶爾會觸發 reallocation (capacity 不夠時),但 amortized
//    仍是 O(1) — 因為「每次 reallocation 把 capacity 加倍」的均攤分析。
//
// 3) 為什麼用 string 當 stack 比 stack<char> 還好?
//    std::stack<char> 預設底層是 deque<char>,而 deque 是分塊儲存,
//    每塊獨立 allocator,記憶體不連續。對 char 這種小型別,直接用 string
//    當 stack 反而:
//      - 記憶體連續 (cache friendly)
//      - 沒有 deque 的塊邊界處理開銷
//      - 介面相容 (back/push_back/pop_back/empty 全都有)
//    很多競賽選手與面試解法都直接用 string 當 stack<char>。
//
// 4) C++20 之後的更高階介面
//    C++20 加入 ends_with(suffix),讓「判斷尾端是否為某字元/字串」更直觀:
//      if (s.ends_with('\n')) ...
//    它內部會處理空字串、長度檢查,你不用自己 if (!s.empty() && s.back()...).
//    但 back() 仍然是「拿尾字元當值用」的最佳工具,兩者並用。
//
// 5) 反向迭代器的另一條路
//    s.rbegin() 同樣指向最後一個字元;*s.rbegin() == s.back()。
//    當你需要「從尾巴往前掃描」時用 rbegin/rend;只取一個用 back。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 呼叫前需確保 !empty(),否則為 UB。
// 2. back() 回傳的不是 '\0';要拿到 '\0' 請用 c_str()[size()] 或 s[s.size()]。
// 3. 配合 push_back / pop_back 操作時,back() 是檢視「目前最末元素」的常用方法。
// 4. reallocation 後先前取得的 reference 失效。
// 5. C++20 起,「判斷字串是否以 X 結尾」首選 ends_with,語意比 back() 更清楚。
//
// =============================================================================

/*
補充筆記：std::string::back
  - std::string::back 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::back 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoBack() {
    std::string s = "Hello";
    std::cout << "s.back() = " << s.back() << "\n";   // 'o'
    s.back() = '!';
    std::cout << "after write: " << s << "\n";        // "Hell!"
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例】LeetCode 20. Valid Parentheses (Easy)
//
// 題目敘述:
//   給定一個只含 '(', ')', '{', '}', '[', ']' 的字串,判斷它是否為合法的
//   括號字串。合法的條件是:
//     - 開括號必須以對應型別的閉括號關閉。
//     - 開括號的關閉順序必須正確 (last-opened, first-closed)。
//   範例: "()[]{}" → true
//        "(]"     → false
//        "([{}])" → true
//
// 為何用 back:
//   這題是 stack 的最經典應用 — 用 std::string 當 stack<char> 容器,
//   遇開括號 push_back,遇閉括號比對 back() 是否為對應的開括號,
//   然後 pop_back。語意清晰、效能極佳,是面試常見的最簡寫法。
//
// 解題思路:
//   1. 走訪每個字元,開括號就 push 到 stack。
//   2. 閉括號:若 stack 為空 → false;否則檢查 back() 是否配對。
//      - 配對成功 → pop_back;配對失敗 → false。
//   3. 走完後,stack 必須為空 (所有開括號都關掉了) 才回傳 true。
//
// 複雜度: 時間 O(n),空間 O(n)
// -----------------------------------------------------------------------------
bool isValidParentheses(const std::string& s) {
    std::string stack;          // 用 string 當 stack 容器
    stack.reserve(s.size());

    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stack += c;
        } else {
            if (stack.empty()) return false;
            char top = stack.back();
            bool match = (c == ')' && top == '(') ||
                         (c == ']' && top == '[') ||
                         (c == '}' && top == '{');
            if (!match) return false;
            stack.pop_back();
        }
    }
    return stack.empty();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】chomp - 移除尾端的換行符 (Unix \n / Windows \r\n)
//
// 為何用 back:
//   從 stdin / 檔案 / socket 讀取資料常會多出尾端換行字元;若忘記去除,
//   後續比對、parse 都會出錯。逐一檢查 back() 並 pop_back() 是最直接
//   清晰的寫法。Perl 的 chomp、Python 的 rstrip 都對應這個操作。
//   每天的 log 處理、CSV 解析、CLI 工具開發幾乎一定要做。
// -----------------------------------------------------------------------------
std::string chomp(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
    return s;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 2】LeetCode 1留. 1留.Final Value of Variable After Performing Operations
// 題目: LeetCode 2011. Final Value of Variable After Performing Operations
// 給定 ops 例如 ["X++","++X","--X"];從 X=0 起依次套用,回傳最終 X 值。
// 為何用 back: 每個 op 是 "++X"/"X++"/"--X"/"X--" 之一,只看 op.back() 是 '+' 或 '-'
//              就能判斷加減,完全免去判斷前綴/後綴,是 back() 的經典用法。
// 複雜度: O(N)。
// -----------------------------------------------------------------------------
int finalValueAfterOperations(const std::vector<std::string>& ops) {
    int x = 0;
    for (const auto& op : ops) {
        if (op.back() == '+') ++x;       // "X++" 或 "++X"
        else                  --x;       // "X--" 或 "--X"
    }
    return x;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】判斷 URL/路徑是否以 '/' 結尾並補上
// 為何用 back: 拼路徑時若 base 沒以 '/' 結尾,直接 + "child" 會變成錯誤的 host
//              一部分。檢查 back() == '/' 是最直觀的條件。
// -----------------------------------------------------------------------------
std::string joinPath(std::string base, const std::string& child) {
    if (base.empty() || base.back() != '/') base.push_back('/');
    base += child;
    return base;
}

int main() {
    demoBack();
    std::cout << "\n=== LeetCode 20 ===\n";
    std::cout << std::boolalpha
              << isValidParentheses("()[]{}") << "\n"   // true
              << isValidParentheses("(]")     << "\n"   // false
              << isValidParentheses("([{}])") << "\n";  // true

    std::cout << "\n=== LeetCode 2011 ===\n";
    std::cout << finalValueAfterOperations({"--X","X++","X++"}) << "\n";   // 1
    std::cout << finalValueAfterOperations({"++X","++X","X++"}) << "\n";   // 3

    std::cout << "\n=== 日常實務: chomp ===\n";
    std::cout << "[" << chomp("hello\r\n") << "]\n";    // "[hello]"
    std::cout << "[" << chomp("no newline") << "]\n";   // "[no newline]"

    std::cout << "\n=== 日常實務: joinPath ===\n";
    std::cout << joinPath("/var/log",  "app.log") << "\n";   // /var/log/app.log
    std::cout << joinPath("/var/log/", "app.log") << "\n";   // /var/log/app.log

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：對空字串呼叫 back() 會發生什麼?
    //    A：是 Undefined Behavior。標準明文規定「對空字串呼叫 back/front
    //       是 UB」,實作不會檢查。除錯模式 (libstdc++ _GLIBCXX_DEBUG、
    //       libc++ debug build) 會 assert,但 release build 會直接讀越界
    //       記憶體,可能拿到 '\0' 也可能 SEGV。呼叫前必先 if (!s.empty())。
    //
    //  Q2：back() 會回傳結尾的 '\0' 嗎?
    //    A：不會。string 內部 buffer 確實在 size() 位置存有 '\0' (C++11 起
    //       保證),但這個 '\0' 不算 size 內的字元。back() 等同 [size()-1],
    //       回傳的是最後一個「有效」字元。要拿那個 '\0' 得用 [size()] 或
    //       c_str()[size()]。
    //
    //  Q3：back() 是否可寫?搭配 push_back / pop_back 的典型用法?
    //    A：non-const string 的 back() 回傳 reference&,可以直接賦值修改
    //       (例如 s.back() = '!'). 三件套 push_back / back / pop_back 讓
    //       std::string 可當 stack<char> 用,效能比 std::stack<char> 好
    //       (避免 deque 的分塊配置 overhead)。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra back.cpp -o back

// === 預期輸出 (節錄) ===
// === LeetCode 2011 ===
// 1
// 3
// === 日常實務: joinPath ===
// /var/log/app.log
// /var/log/app.log
