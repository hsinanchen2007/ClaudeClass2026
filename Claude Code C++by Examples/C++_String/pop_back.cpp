// =============================================================================
// 檔名: pop_back.cpp
// 主題: std::string::pop_back (移除最後一個字元)
// 參考:
//   - https://en.cppreference.com/cpp/string/basic_string/pop_back
//   - https://cplusplus.com/reference/string/string/pop_back/
// =============================================================================
//
// 【函式資訊 Information】
//   void pop_back();        // C++11 起
//
// 參數: 無
// 回傳: 無 (沒有「回傳被移除的字元」— 若需要請先用 back() 取出)
// 例外: 無正式 noexcept 標註,但實作上不會丟例外。
//       前提條件: 字串非空。對空字串呼叫是 Undefined Behavior。
//
// =============================================================================
//
// 【詳細解釋 Explanation - 設計理念與底層運作】
//
// 1. 它做了什麼?
//    -----------------------------------------------------------------
//    把 size() 減 1,並在新的 size() 位置寫入結尾哨兵 '\0'。
//    內容上等同於:
//      - erase(size() - 1, 1)
//      - erase(end() - 1)
//      - resize(size() - 1)
//    但 pop_back 是「移除尾端元素」的 idiomatic 寫法,效能也最好
//    (沒有額外參數計算、沒有迭代器有效性檢查)。
//
// 2. 與 push_back 的對稱關係
//    -----------------------------------------------------------------
//    push_back / pop_back 是天然的對稱對。語意上:
//      push_back: 新增一個尾端元素;
//      pop_back:  移除一個尾端元素 (但不傳回它的值)。
//    這個對稱在 stack 運算中極其重要 — back() / push_back / pop_back
//    三個函式合起來就是完整的 stack 介面。
//
// 3. 時間複雜度: O(1)
//    -----------------------------------------------------------------
//    純粹是把 size 減 1、寫入哨兵 '\0',沒有任何搬移。比 erase(size()-1, 1)
//    更明確 (那個版本實作可能要做額外檢查)。capacity 完全不變。
//
// 4. 空字串的 UB 與防禦性程式設計
//    -----------------------------------------------------------------
//    對 empty() == true 的字串呼叫 pop_back() 是 *Undefined Behavior*:
//      - debug 模式 (libstdc++ -D_GLIBCXX_DEBUG) 會 assert 失敗;
//      - release 模式可能沒事 (size 變成 SIZE_MAX),但接下來任何操作
//        都會崩潰 — 隱藏的 bug。
//    防禦寫法:
//      if (!s.empty()) s.pop_back();
//    或在演算法中保證「呼叫 pop_back 前已驗證非空」(例如先檢查 stack
//    匹配條件)。標準函式選擇 UB 而非丟例外,是為了「不為大多數正確的
//    呼叫付出檢查成本」。
//
// 5. 為什麼 pop_back 不回傳被移除的字元?
//    -----------------------------------------------------------------
//    與 std::vector::pop_back / std::stack::pop 一致 — 設計理由:
//      - 回傳值需要拷貝,可能丟例外 (對 char 不會,但對泛型 T 會);
//      - 若拷貝中途丟例外,「已經彈出」與「還沒彈出」的狀態難以保證
//        強例外安全;
//      - 強制「先 back() 再 pop_back()」分兩步,語意清晰、可控。
//    若你需要 pop 並取值,標準寫法:
//      char c = s.back();
//      s.pop_back();
//
// 6. 例外安全
//    -----------------------------------------------------------------
//    pop_back 不會配置記憶體、不會丟例外。標準雖未明標 noexcept,
//    但實作上是 noexcept-equivalent。可放心用在 destructor / catch 內。
//
// 7. 迭代器/指標/參考失效規則
//    -----------------------------------------------------------------
//    - 不會觸發 reallocation;
//    - 「被移除字元」的迭代器/指標/參考失效;
//    - end() 失效 (位置往前挪一格);
//    - 其他迭代器仍有效。
//
// 8. C++11 / 17 / 20 / 23 的演進
//    -----------------------------------------------------------------
//    - C++11: 引入 (在此之前需要用 erase(size()-1, 1) 或 resize(size()-1))。
//    - C++17/20: 行為不變,持續強化 constexpr 友善度。
//    - C++23: constexpr string 中 pop_back 也是 constexpr。
//
// =============================================================================
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 string 模擬 stack 是高頻技巧?
//     LeetCode / 演算法題大量出現「string-as-stack」模式:
//       - 括號匹配 (LeetCode 20、921、1249);
//       - 相鄰消去 (LeetCode 1047、1544、1003);
//       - monotonic stack 字串題 (LeetCode 316、402、1081);
//       - 表達式求值 (LeetCode 224、227、772)。
//     用 string 而非 std::stack<char> 的優勢:
//       1. back() / push_back / pop_back 完整對應 stack 介面;
//       2. 結果通常本來就是字串,直接回傳免轉換;
//       3. operator[] 與迭代器讓你可以隨時遍歷檢查;
//       4. reserve(N) 可一次配足,效能比 stack 預設好。
//     注意:string 不適合「需要 push 多種型別」的場景 (例如表達式求值要
//     存運算子與運算元),那種情況請用 std::stack。
//
// (B) UB vs Exception:標準函式的設計選擇
//     pop_back 對空字串選擇 UB,而非 throw out_of_range,是 C++ 哲學:
//       「你不為你不用的功能付出代價」(Don't pay for what you don't use)。
//     檢查 empty() 對「正確使用 pop_back」的程式而言是純粹開銷。標準
//     把責任交給呼叫方,以換取最高效能。比較:
//       - at()  → 範圍外 throw out_of_range  (要安全用這個);
//       - operator[] → 範圍外 UB             (要效能用這個);
//       - pop_back → empty 時 UB             (與 operator[] 同哲學)。
//
// (C) pop_back 與 resize(n-1) 的細微差別
//     兩者效果相同,但:
//       - pop_back: O(1)、語意明確「移除尾端」;
//       - resize(n-1): O(1)、但 resize 還能放大 (語意更廣);
//       - resize 還會接受 fill 參數,API 較複雜。
//     表達「移除一個尾端元素」時請用 pop_back。
//
// (D) 與 std::stack<char>::pop 的差異
//     std::stack 是 container adaptor,內部預設用 std::deque<char>。
//     若改成 std::stack<char, std::string> 可以讓 stack 用 string 當底層。
//     但實務上直接用 string 即可,無需多此一舉的 adaptor 層。
//
// (E) 配對 pattern: while (!s.empty()) ... s.pop_back();
//     這是「逐字元清空」的標準寫法,等同 s.clear() 但成本相同 (都是 O(N))。
//     真要清空建議用 clear() — 一次到位、語意清楚。while-pop_back 適合
//     「邊清邊處理每個字元」的場景。
//
// =============================================================================
//
// 【注意事項 Pay Attention】
// 1. 呼叫前必須確保 !empty(),否則為 UB。
// 2. pop_back 不會改變 capacity()。要釋放需配合 shrink_to_fit。
// 3. 與 push_back 是天然搭檔,常見於回溯演算法 (backtracking) 與 stack 模式。
// 4. 若需要拿到「被 pop 的字元」,要先 back() 取出再 pop_back():
//      char c = s.back(); s.pop_back();
// 5. 例外安全: 不會丟例外 (前提 !empty())。
// 6. 與 erase(end()-1) / resize(size()-1) 等價,但 pop_back 最 idiomatic。
// =============================================================================

/*
補充筆記：std::string::pop_back
  - std::string::pop_back 需要同時考慮內容、容量與舊指標失效；字串 API 常會配置或搬移緩衝區。
  - 回傳位置的函式要先處理 npos，回傳 pointer/view 的函式要追蹤來源 string 生命週期。
  - UTF-8 文字下，size 代表位元組數，不代表使用者看到的字元數。
  - std::string::pop_back 是 std::string 主題的一部分；先分清楚這個操作會讀取、追加、插入、刪除、搜尋，還是只暴露底層字元。
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

void demoPopBack() {
    std::string s = "Hello!";
    s.pop_back();                               // 移除 '!'
    std::cout << s << "\n";                     // "Hello"

    while (!s.empty()) s.pop_back();            // 防禦性檢查 + 逐字元清空
    std::cout << "after empty: \"" << s << "\"\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses (Easy)
// 題目: 給一個只含 '()[]{}' 的字串,判斷是否所有括號都正確匹配且巢狀正確。
//       例如 "()[]{}" → true、"([)]" → false。
// 為何用 pop_back: 經典 stack 題 — 用 string 當 stack:
//                  遇開括號 push_back,遇閉括號就比對 back() 是否為對應的開
//                  括號;對則 pop_back,不對則失敗。
// 思路: 線性掃過,維護一個 stack。最後 stack 必須為空才算合法。
// 複雜度: O(N) 時間、O(N) 空間。
// -----------------------------------------------------------------------------
bool isValidParentheses(const std::string& s) {
    std::string st;
    st.reserve(s.size() / 2 + 1);               // 上限預估
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            st.push_back(c);                    // 開括號入 stack
        } else {
            if (st.empty()) return false;       // 沒有開就遇到關 → 不合法
            char top = st.back();
            if ((c == ')' && top != '(') ||
                (c == ']' && top != '[') ||
                (c == '}' && top != '{')) {
                return false;                   // 不匹配
            }
            st.pop_back();                      // 匹配成功,彈出
        }
    }
    return st.empty();                          // 必須全部匹配完畢
}

// -----------------------------------------------------------------------------
// 【LeetCode 補充範例】LeetCode 1544. Make The String Great (Easy)
// 題目: 字串中相鄰兩字若為「同字母大小寫對」(如 'aA') 就可消除,反覆消除。
//       例如 "leEeetcode" → "leetcode" (中間 "Ee" 消去)。
// 為何用 pop_back: 用 string 當 stack,top == back();
//                  匹配時 pop_back,不匹配就 push_back。
// 複雜度: O(N) — 每字元最多 push/pop 各一次。
// -----------------------------------------------------------------------------
#include <cctype>
std::string makeGood(const std::string& s) {
    std::string st;
    st.reserve(s.size());
    for (char c : s) {
        if (!st.empty() &&
            std::tolower(static_cast<unsigned char>(st.back())) ==
            std::tolower(static_cast<unsigned char>(c)) &&
            st.back() != c) {                   // 同字母 + 大小寫不同 = bad pair
            st.pop_back();
        } else {
            st.push_back(c);
        }
    }
    return st;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 Daily Work】移除路徑尾端的斜線
// 為何用 pop_back: "/usr/bin/" → "/usr/bin"。檔案路徑正規化的常見步驟,
//                   避免後續 join 時出現 "//"。while-pop_back 處理多重尾斜線。
// -----------------------------------------------------------------------------
std::string removeTrailingSlash(std::string path) {
    while (path.size() > 1 && (path.back() == '/' || path.back() == '\\')) {
        path.pop_back();                        // 防禦: size > 1 確保根目錄 "/" 保留
    }
    return path;
}

// -----------------------------------------------------------------------------
// 【LeetCode 範例 3】LeetCode 1page. 1page0. Backspace String Compare
// 題目: LeetCode 844. Backspace String Compare
// 給兩個字串,'#' 代表退格。判斷處理後兩字串是否相等。
// 為何用 pop_back: '#' 表示退格 → 直接 pop_back() 把上一字元拿掉,完美符合語意。
// 複雜度: O(N+M)。
// -----------------------------------------------------------------------------
bool backspaceCompare(const std::string& s, const std::string& t) {
    auto build = [](const std::string& x) {
        std::string out;
        for (char c : x) {
            if (c == '#') { if (!out.empty()) out.pop_back(); }
            else          out.push_back(c);
        }
        return out;
    };
    return build(s) == build(t);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】從 log 行尾移除多餘的 ';' 或 ',' (常見 CSV 拼接尾巴遺漏)
// 為何用 pop_back: 簡單明確,把最後一個多餘分隔符移除。
// -----------------------------------------------------------------------------
std::string stripTrailingSep(std::string s, char sep) {
    if (!s.empty() && s.back() == sep) s.pop_back();
    return s;
}

int main() {
    demoPopBack();

    std::cout << "\n=== LeetCode 20 ===\n";
    std::cout << std::boolalpha;
    std::cout << isValidParentheses("()[]{}")  << "\n";   // true
    std::cout << isValidParentheses("([)]")    << "\n";   // false
    std::cout << isValidParentheses("{[()]}")  << "\n";   // true
    std::cout << isValidParentheses("(")       << "\n";   // false

    std::cout << "\n=== LeetCode 1544 ===\n";
    std::cout << makeGood("leEeetcode") << "\n";          // "leetcode"
    std::cout << makeGood("abBAcC")     << "\n";          // ""

    std::cout << "\n=== LeetCode 844 ===\n";
    std::cout << backspaceCompare("ab#c", "ad#c")  << "\n";   // true
    std::cout << backspaceCompare("a##c", "#a#c")  << "\n";   // true
    std::cout << backspaceCompare("a#c",  "b")     << "\n";   // false

    std::cout << "\n=== 日常實務: 移除路徑尾端斜線 ===\n";
    std::cout << "[" << removeTrailingSlash("/usr/bin/")    << "]\n";
    std::cout << "[" << removeTrailingSlash("/usr/bin///")  << "]\n";
    std::cout << "[" << removeTrailingSlash("/")            << "]\n";

    std::cout << "\n=== 日常實務: stripTrailingSep ===\n";
    std::cout << "[" << stripTrailingSep("a,b,c,", ',') << "]\n";   // [a,b,c]
    std::cout << "[" << stripTrailingSep("a,b,c",  ',') << "]\n";   // [a,b,c]

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1:在空字串呼叫 pop_back 為什麼是 UB 而不是 throw?
    //    A:標準遵循「不為正確使用付代價」原則。檢查 empty() 對絕大多數
    //      呼叫只是純開銷,標準把責任交給呼叫方,以換得最高效能。需要
    //      安全的場合請自行 if (!s.empty()) 守衛或先 back() 取值。
    //
    //  Q2:pop_back 會改變 capacity() 嗎?與 erase(end()-1)、resize(n-1)差?
    //    A:不會。三者都只把 size 減 1、寫入哨兵 '\0',capacity 不變。
    //      pop_back 是「移除尾端」最 idiomatic 的寫法;erase 接受迭代器
    //      或 pos+count,語意較廣;resize 還能放大或填字。
    //
    //  Q3:pop_back 後哪些 iterator/reference 會失效?
    //    A:被移除字元的 iterator/pointer/reference,以及 end() 都失效;
    //      其餘仍有效。它不會觸發 reallocation,因此前面字元的
    //      references 都還能用。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra pop_back.cpp -o pop_back

// === 預期輸出 (節錄) ===
// === LeetCode 844 ===
// true
// true
// false
// === 日常實務: stripTrailingSep ===
// [a,b,c]
// [a,b,c]
