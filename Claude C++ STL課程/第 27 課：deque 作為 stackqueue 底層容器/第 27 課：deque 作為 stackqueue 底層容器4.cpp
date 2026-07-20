// =============================================================================
//  第 27 課：deque 作為 stack/queue 底層容器 4  —  stack 經典應用：括號匹配
// =============================================================================
//
// 【主題資訊 Information】
//   std::stack<char> s;   // 底層預設 deque<char>
//   s.push(c);  s.top();  s.pop();  s.empty();
//
//   標頭檔：<stack>、<string>
//   複雜度：時間 O(n)（每個字元最多進出堆疊各一次）；空間最壞 O(n)
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼括號匹配非用堆疊不可】
//   括號結構的本質是「**最近未配對的左括號，必須最先被配對**」。
//   看這個例子：{ ( [ ] ) }
//       遇到 ] 時，該和誰配對？答案是最近的 [，不是更早的 ( 或 {。
//   「最近的先處理」正是 LIFO 的定義，所以堆疊是唯一自然的選擇。
//   換句話說，不是「我們選擇用堆疊解這題」，而是「這個問題的結構就是堆疊」。
//
// 【2. 為什麼不能只用計數器】
//   只有一種括號時，用一個計數器就夠了（遇左 +1、遇右 -1，
//   中途不可為負、結束必須為 0）。
//   但一旦有多種括號，計數器就失效了：
//       "([)]"  → 小括號 +1-1、中括號 +1-1，兩個計數器都平衡，
//                 但這個字串其實是**不合法**的（交錯而非巢狀）。
//   計數器只記得「數量」，堆疊記得「順序」——而括號問題的核心正是順序。
//   本檔第 3 節會實際跑這個反例。
//
// 【3. 三個必須處理的失敗情境】
//   (a) 右括號來了但堆疊是空的      → 例如 ")("，多餘的右括號
//   (b) 右括號與棧頂的左括號不匹配  → 例如 "(]"
//   (c) 掃完了但堆疊還有東西        → 例如 "(("，未閉合的左括號
//   漏掉任何一個都會產生錯誤答案。特別是 (c)——很多人只檢查掃描過程，
//   忘了最後要 return s.empty()，於是 "(((" 會被誤判為合法。
//
// 【4. 對空堆疊呼叫 top() 是 UB】
//   情境 (a) 若沒有先檢查 s.empty() 就呼叫 s.top()，就是未定義行為。
//   標準沒有規定要檢查、也不會丟例外，不保證崩潰也不保證任何特定值。
//   本檔的實作把 if (s.empty()) return false; 放在 s.top() 之前，
//   順序不可對調。
//
// 【概念補充 Concept Deep Dive】
//   ● 為什麼堆疊裡存「左括號」而不是「期待的右括號」
//     兩種寫法都可行。存期待的右括號可以少一次比對邏輯：
//         遇到 ( 就 push(')')，遇到右括號時只要比 s.top() == c 即可。
//     本檔採用「存左括號」的版本，因為它更貼近「記住尚未閉合的東西」
//     這個心智模型，除錯時也比較好讀（堆疊內容就是實際看到的字元）。
//
//   ● 空字串為什麼回傳 true
//     空字串沒有任何未配對的括號，依定義是合法的（vacuous truth）。
//     LeetCode 20 的約束是 1 ≤ s.length，所以不會測到，
//     但一個正確的實作應該自然地處理它——而 return s.empty() 剛好就對了。
//
//   ● 複雜度
//     每個字元最多 push 一次、pop 一次 → 時間 O(n)。
//     空間最壞 O(n)（例如 "((((((" 全部是左括號）。
//     這已是最優：你至少得讀完整個字串。
//
//   ● 這個模式的延伸
//     同樣的「用堆疊追蹤未閉合結構」模式可以直接套用到：
//     HTML/XML 標籤配對、程式語言的區塊巢狀、運算式求值（雙堆疊）、
//     以及本檔的實務範例——SQL 語句的括號與引號檢查。
//
// 【注意事項 Pay Attention】
//   1. 呼叫 s.top() 前**必須**先確認 !s.empty()，否則是 UB。
//   2. 掃描結束後要 return s.empty()，否則 "(((" 會被誤判為合法。
//   3. 只用計數器無法處理多種括號交錯的情況（"([)]"）。
//   4. 若字串含括號以外的字元（本檔的第一個測資就有），
//      要明確決定是忽略還是視為錯誤——本檔選擇忽略。
//   5. isBalanced("") 回傳 true 是刻意的定義，不是邊界 bug。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】括號匹配
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼括號匹配一定要用堆疊？用兩個計數器不行嗎？
//     答：不行。計數器只記得「各種括號各出現幾次」，記不住**順序**。
//         "([)]" 的小括號與中括號數量都平衡，但它是不合法的交錯結構。
//         括號問題的本質是「最近未配對的左括號要最先配對」＝ LIFO，
//         唯有堆疊能表達這個順序關係。
//     追問：那什麼情況下計數器就夠了？→ 只有**單一種**括號時。
//         此時只需檢查「中途不可為負、結束必須為 0」。
//
// 🔥 Q2. 這題的時間與空間複雜度是多少？
//     答：時間 O(n)——每個字元最多 push 一次、pop 一次。
//         空間最壞 O(n)——例如全部都是左括號 "((((((" 時堆疊會裝滿。
//         這已經是最優解，因為你至少必須讀完整個字串。
//     追問：能不能把空間降到 O(1)？→ 對多種括號不行。
//         你必須記住「尚未閉合的左括號序列」，而那個序列最長可達 n，
//         資訊理論上就需要 O(n) 空間。
//
// ⚠️ 陷阱. 下面這個實作對 "(((" 會回傳什麼？錯在哪？
//         for (char c : s) {
//             if (是左括號) st.push(c);
//             else { if (st.empty() || 不匹配) return false; st.pop(); }
//         }
//         return true;                 // ← 這裡
//     答：會回傳 **true**，但正確答案是 false。
//         最後一行應該是 return st.empty()。
//         "(((" 從頭到尾沒有觸發任何 return false，掃描結束時堆疊裡還有
//         三個未閉合的左括號，卻被當成合法。
//     為什麼會錯：只專注在「掃描過程中的錯誤」，忘了「掃描結束時的狀態」
//         也是判斷條件的一部分。這是這題最常見的失分點——
//         測資只用 "()" "(]" 這類會在中途失敗的例子時，這個 bug 完全不會浮現。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <string>
#include <vector>
#include <utility>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：給定只含 '('、')'、'{'、'}'、'['、']' 的字串，判斷是否有效。
//         有效的條件：左括號必須以正確的順序閉合，且每個右括號都有對應的左括號。
//   為什麼用到本主題：這個函式**就是** LeetCode 20 的標準解。
//         本檔額外容許括號以外的字元（直接忽略），
//         所以它同時能當通用的運算式檢查工具。
//   複雜度：時間 O(n)、空間 O(n)。
// -----------------------------------------------------------------------------
bool isBalanced(const string& expr) {
    stack<char> s;   // 底層是 deque<char>

    for (char ch : expr) {
        if (ch == '(' || ch == '[' || ch == '{') {
            s.push(ch);
        }
        else if (ch == ')' || ch == ']' || ch == '}') {
            // 失敗情境 (a)：右括號來了但沒有待配對的左括號
            // 這個檢查必須在 s.top() 之前——對空 stack 呼叫 top() 是 UB
            if (s.empty()) return false;

            char top = s.top();
            if ((ch == ')' && top == '(') ||
                (ch == ']' && top == '[') ||
                (ch == '}' && top == '{')) {
                s.pop();
            } else {
                // 失敗情境 (b)：型別不匹配，例如 "(]"
                return false;
            }
        }
        // 其他字元一律忽略
    }
    // 失敗情境 (c)：掃完了但還有未閉合的左括號
    // ⚠️ 這裡若寫成 return true，"(((" 會被誤判為合法
    return s.empty();
}

// -----------------------------------------------------------------------------
// 對照組：只用計數器的錯誤實作，用來證明「順序」為什麼重要
// -----------------------------------------------------------------------------
bool isBalancedByCounters(const string& expr) {
    int round = 0, square = 0, curly = 0;
    for (char ch : expr) {
        switch (ch) {
            case '(': ++round;  break;
            case ')': --round;  if (round  < 0) return false; break;
            case '[': ++square; break;
            case ']': --square; if (square < 0) return false; break;
            case '{': ++curly;  break;
            case '}': --curly;  if (curly  < 0) return false; break;
            default: break;
        }
    }
    return round == 0 && square == 0 && curly == 0;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】SQL 語句的括號與字串引號檢查
//   情境：後端服務動態組 SQL 前，先做一次語法前檢查，
//         避免把明顯壞掉的語句送到資料庫（省下一次來回與錯誤日誌）。
//   重點：括號要巢狀正確，而且**引號內的括號必須被忽略**——
//         'a)b' 裡的 ) 是字串內容，不是語法括號。
//         這一點正是實務比 LeetCode 複雜的地方：要先做詞法狀態機，
//         再把堆疊套上去。
// -----------------------------------------------------------------------------
struct SqlCheck {
    bool ok = true;
    string reason;
    int position = -1;      // 出錯的字元位置（0-based）
};

SqlCheck checkSql(const string& sql) {
    SqlCheck r;
    stack<pair<char, int>> st;      // (左括號, 位置)
    bool inString = false;

    for (size_t i = 0; i < sql.size(); ++i) {
        char c = sql[i];

        // 單引號切換「字串內」狀態（簡化版：不處理 '' 逸出）
        if (c == '\'') { inString = !inString; continue; }
        if (inString) continue;     // 字串內的括號一律忽略

        if (c == '(') {
            st.push({c, static_cast<int>(i)});
        } else if (c == ')') {
            if (st.empty()) {
                r.ok = false;
                r.position = static_cast<int>(i);
                r.reason = "多餘的右括號";
                return r;
            }
            st.pop();
        }
    }

    if (inString) {
        r.ok = false;
        r.reason = "字串引號未閉合";
        r.position = static_cast<int>(sql.size());
    } else if (!st.empty()) {
        r.ok = false;
        r.position = st.top().second;
        r.reason = "左括號未閉合";
    }
    return r;
}

int main() {
    cout << "=== 1. 基本括號匹配 ===" << endl;
    cout << isBalanced("(a + b) * [c - d]")   << endl;  // 1 (true)
    cout << isBalanced("{(a + b) * [c - d]}")  << endl;  // 1 (true)
    cout << isBalanced("(a + b]")              << endl;  // 0 (false)
    cout << isBalanced("((a + b)")             << endl;  // 0 (false)
    cout << isBalanced("")                     << endl;  // 1 (true)

    cout << "\n=== 2. 三種失敗情境各自的觸發點 ===" << endl;
    cout << boolalpha;
    cout << "\")(\"   → " << isBalanced(")(")
         << "（情境 a：右括號來時堆疊是空的）" << endl;
    cout << "\"(]\"   → " << isBalanced("(]")
         << "（情境 b：型別不匹配）" << endl;
    cout << "\"(((\"  → " << isBalanced("(((")
         << "（情境 c：掃完仍有未閉合的左括號）" << endl;
    cout << "→ 情境 c 最容易漏：若最後寫 return true 而非 return s.empty()，" << endl;
    cout << "  \"(((\" 會被誤判為合法，而且一般測資往往測不出來。" << endl;

    cout << "\n=== 3. 為什麼計數器不夠：交錯 vs 巢狀 ===" << endl;
    const vector<string> tricky = {"([)]", "([])", "{[()]}", "{[(])}"};
    cout << "字串      堆疊解法   計數器解法   兩者是否一致" << endl;
    for (const string& t : tricky) {
        bool a = isBalanced(t);
        bool b = isBalancedByCounters(t);
        cout << "  " << t << "    " << (a ? "true " : "false")
             << "      " << (b ? "true " : "false")
             << "        " << (a == b ? "一致" : "★不一致★") << endl;
    }
    cout << "→ \"([)]\" 的各種括號數量都平衡，計數器誤判為合法；" << endl;
    cout << "  但它是交錯而非巢狀，堆疊正確地判為不合法。" << endl;
    cout << "  計數器只記得「數量」，堆疊記得「順序」。" << endl;

    cout << "\n=== 4. 空堆疊保護的重要性 ===" << endl;
    cout << "實作中 if (s.empty()) return false; 必須寫在 s.top() 之前。" << endl;
    cout << "順序對調的話，處理 \")\" 時就會對空 stack 呼叫 top()，" << endl;
    cout << "那是未定義行為——不保證崩潰、也不保證任何特定值。" << endl;
    cout << "本實作對 \")\" 的結果：" << isBalanced(")") << "（安全地回傳 false）" << endl;

    cout << "\n=== LeetCode 20. Valid Parentheses（官方測資）===" << endl;
    struct { const char* in; bool expect; } lc[] = {
        {"()",     true},
        {"()[]{}", true},
        {"(]",     false},
        {"([)]",   false},
        {"{[]}",   true}
    };
    for (auto& t : lc) {
        bool got = isBalanced(t.in);
        cout << "  \"" << t.in << "\" → " << got
             << "（預期 " << t.expect << "）"
             << (got == t.expect ? " ✓" : " ✗") << endl;
    }

    cout << "\n=== 日常實務：SQL 語句前檢查 ===" << endl;
    const vector<string> sqls = {
        "SELECT * FROM t WHERE (a > 1 AND (b < 2 OR c = 3))",
        "SELECT * FROM t WHERE name = 'a)b'",
        "SELECT * FROM t WHERE (a > 1",
        "SELECT * FROM t WHERE a > 1)",
        "SELECT * FROM t WHERE name = 'unterminated"
    };
    for (const string& q : sqls) {
        SqlCheck r = checkSql(q);
        cout << "  " << (r.ok ? "[OK]   " : "[錯誤] ") << q << endl;
        if (!r.ok) {
            cout << "         → " << r.reason
                 << "（位置 " << r.position << "）" << endl;
        }
    }
    cout << "→ 注意第 2 筆：'a)b' 裡的 ) 在字串內，被正確忽略。" << endl;
    cout << "  實務比 LeetCode 多這一層詞法狀態，但堆疊的角色完全相同。" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 27 課：deque 作為 stackqueue 底層容器4.cpp" -o demo27_4

// === 預期輸出 ===
// === 1. 基本括號匹配 ===
// 1
// 1
// 0
// 0
// 1
//
// === 2. 三種失敗情境各自的觸發點 ===
// ")("   → false（情境 a：右括號來時堆疊是空的）
// "(]"   → false（情境 b：型別不匹配）
// "((("  → false（情境 c：掃完仍有未閉合的左括號）
// → 情境 c 最容易漏：若最後寫 return true 而非 return s.empty()，
//   "(((" 會被誤判為合法，而且一般測資往往測不出來。
//
// === 3. 為什麼計數器不夠：交錯 vs 巢狀 ===
// 字串      堆疊解法   計數器解法   兩者是否一致
//   ([)]    false      true         ★不一致★
//   ([])    true       true         一致
//   {[()]}    true       true         一致
//   {[(])}    false      true         ★不一致★
// → "([)]" 的各種括號數量都平衡，計數器誤判為合法；
//   但它是交錯而非巢狀，堆疊正確地判為不合法。
//   計數器只記得「數量」，堆疊記得「順序」。
//
// === 4. 空堆疊保護的重要性 ===
// 實作中 if (s.empty()) return false; 必須寫在 s.top() 之前。
// 順序對調的話，處理 ")" 時就會對空 stack 呼叫 top()，
// 那是未定義行為——不保證崩潰、也不保證任何特定值。
// 本實作對 ")" 的結果：false（安全地回傳 false）
//
// === LeetCode 20. Valid Parentheses（官方測資）===
//   "()" → true（預期 true） ✓
//   "()[]{}" → true（預期 true） ✓
//   "(]" → false（預期 false） ✓
//   "([)]" → false（預期 false） ✓
//   "{[]}" → true（預期 true） ✓
//
// === 日常實務：SQL 語句前檢查 ===
//   [OK]   SELECT * FROM t WHERE (a > 1 AND (b < 2 OR c = 3))
//   [OK]   SELECT * FROM t WHERE name = 'a)b'
//   [錯誤] SELECT * FROM t WHERE (a > 1
//          → 左括號未閉合（位置 22）
//   [錯誤] SELECT * FROM t WHERE a > 1)
//          → 多餘的右括號（位置 27）
//   [錯誤] SELECT * FROM t WHERE name = 'unterminated
//          → 字串引號未閉合（位置 42）
// → 注意第 2 筆：'a)b' 裡的 ) 在字串內，被正確忽略。
//   實務比 LeetCode 多這一層詞法狀態，但堆疊的角色完全相同。
