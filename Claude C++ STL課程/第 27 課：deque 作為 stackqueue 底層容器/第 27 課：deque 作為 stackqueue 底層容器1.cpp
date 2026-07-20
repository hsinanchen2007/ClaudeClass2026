// =============================================================================
//  第 27 課：deque 作為 stack/queue 底層容器 1  —  std::stack 基本操作
// =============================================================================
//
// 【主題資訊 Information】
//   template <class T, class Container = std::deque<T>> class stack;
//
//   void push(const T&);  void push(T&&);
//   template <class... Args> decltype(auto) emplace(Args&&...);  // C++17 起回傳 reference
//   void pop();                       // 回傳 void！
//   reference top();                  // 存取棧頂
//   bool empty() const;  size_type size() const;
//
//   標頭檔：<stack>
//   複雜度：push / pop / top / empty / size 全部 O(1)（攤銷）
//   本檔標準：C++17
//
// 【詳細解釋 Explanation】
//
// 【1. stack 不是容器，是「容器配接器」】
//   這是本課最重要的觀念。std::stack 內部只有一個資料成員：
//       protected: Container c;
//   它自己不管理任何記憶體，所有工作都委託給底層容器：
//       push(v)  →  c.push_back(v)
//       pop()    →  c.pop_back()
//       top()    →  c.back()
//       empty()  →  c.empty()
//       size()   →  c.size()
//   所以 stack 的效能完全由底層容器決定，它自己幾乎沒有成本
//   （這些函式都是 inline 的一行轉呼叫）。
//
// 【2. 配接器存在的意義：限制介面】
//   既然 deque 本來就能當堆疊用（push_back / pop_back / back），
//   為什麼還要包一層 stack？答案是**刻意拿掉你不該用的功能**：
//       stack 沒有 begin() / end()   → 不能遍歷
//       stack 沒有 operator[]        → 不能隨機存取
//       stack 沒有 insert / erase    → 不能從中間動手腳
//   這不是功能缺失，而是**設計意圖的宣告**：
//   當你在程式碼裡看到 stack<int>，你立刻知道「這裡只會後進先出」，
//   不必再去讀完整個函式確認有沒有人偷偷從中間插入。
//   型別本身就是文件，而且是編譯器強制執行的文件。
//
// 【3. 預設底層為什麼是 deque】
//   stack 只需要 push_back / pop_back / back，vector 完全能勝任。
//   選 deque 當預設的理由是**擴容行為**：
//       vector 擴容 → 配置新記憶體 + 搬移全部既有元素 + 釋放舊的
//       deque  擴容 → 只是加掛一塊新 chunk，既有元素完全不動
//   對「不斷成長的堆疊」而言，deque 沒有那個週期性的 O(n) 尖峰。
//   代價是 deque 的存取常數較大、快取局部性較差。
//   若你的堆疊大小可預期，stack<int, vector<int>> + reserve 往往更快。
//
// 【4. pop() 為什麼不回傳值】
//   與 deque 的 pop_front/pop_back 同一個理由：**例外安全**。
//   若 pop() 回傳 T，就必須複製建構一個回傳值；
//   萬一該複製建構子丟出例外，元素已從堆疊移除、值卻沒交到你手上 → 資料遺失。
//   拆成 top()（看）+ pop()（移除）兩步，任一步失敗容器狀態都還完整。
//   所以標準寫法永遠是：
//       int v = s.top();   // 先取值
//       s.pop();           // 再移除
//
// 【概念補充 Concept Deep Dive】
//   ● 底層容器的最低要求
//     能當 stack 底層的容器必須提供：back()、push_back()、pop_back()、
//     empty()、size()，以及 value_type / reference / size_type 等型別別名。
//     符合的有 vector、deque、list。**不符合的有 std::set、std::forward_list**
//     （後者只有 push_front，沒有 push_back）。
//
//   ● 成員是 protected 而非 private
//     stack 的 Container c 宣告為 protected，這是刻意的：
//     它允許你繼承 stack 並在衍生類別中存取底層容器
//     （例如寫一個能印出全部內容的 DebugStack）。
//     ⚠️ 但 stack 沒有虛擬解構子，不要用它做多型的基底類別。
//
//   ● 對空 stack 呼叫 top() / pop() 是 UB
//     標準沒有規定要檢查，也不會丟例外。
//     不保證崩潰、不保證任何特定值——一定要先 empty() 檢查。
//
// 【注意事項 Pay Attention】
//   1. 對空 stack 呼叫 top() 或 pop() 是 **未定義行為**，務必先檢查 empty()。
//   2. pop() 回傳 void；要取值請先 top()。
//   3. stack **沒有** begin()/end()/operator[]——這是刻意的限制，不是缺陷。
//   4. 底層可換：stack<int, vector<int>>；但底層必須提供 back/push_back/pop_back。
//   5. stack 沒有虛擬解構子，不適合當多型基底類別。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::stack
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::stack 是容器嗎？它的預設底層是什麼？
//     答：不是容器，是**容器配接器**。它內部只有一個 Container c 成員，
//         所有操作都委託給它：push→push_back、pop→pop_back、top→back。
//         預設底層是 std::deque<T>。
//     追問：為什麼預設不是 vector？→ 因為 deque 擴容時只加掛新 chunk、
//         不搬移既有元素，沒有 vector 那種週期性的 O(n) 尖峰。
//         但若堆疊大小可預期，stack<int, vector<int>> 配合 reserve 往往更快。
//
// 🔥 Q2. stack 為什麼不提供 begin() / end()？
//     答：這是配接器的核心價值——**刻意限制介面**。
//         堆疊的語意就是「只能從頂端操作」，提供遍歷能力會破壞這個保證。
//         當程式碼裡出現 stack<int>，讀者立刻知道這裡只有 LIFO 行為，
//         不必讀完整段程式確認有沒有人從中間插入。型別本身就是編譯器強制的文件。
//     追問：那我真的需要遍歷怎麼辦？→ 代表你需要的不是堆疊，直接用 deque 或 vector。
//         強行繞過（例如繼承後存取 protected 的 c）通常是設計選錯容器的信號。
//
// ⚠️ 陷阱. 寫 int v = s.pop(); 為什麼編譯不過？這個設計是不是很不方便？
//     答：pop() 的回傳型別是 void，必須寫成兩步：int v = s.top(); s.pop();
//         這不是疏忽，而是**例外安全**的必然結果：
//         若 pop 要回傳值，就得複製建構一個 T；萬一那次複製丟出例外，
//         元素已經被移除、值卻沒送到呼叫端 → 資料永久遺失且無法復原。
//     為什麼會錯：拿其他語言（Python 的 list.pop()、Java 的 Stack.pop()）
//         的習慣來套。那些語言有 GC 與參考語意，「回傳」不需要複製物件，
//         自然沒有這個問題。C++ 是值語意，複製可能丟例外，
//         所以標準選擇把「觀察」與「修改」拆開——這是 C++ 例外安全設計的經典案例。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <vector>
#include <deque>
#include <string>
#include <type_traits>
using namespace std;

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：判斷只含 ()[]{} 的字串括號是否正確配對。
//   為什麼用到本主題：這是 std::stack 最經典的應用，也是「為什麼需要堆疊」
//     最好的說明——括號配對的本質就是「最近未配對的左括號要最先被配對」，
//     完全對應 LIFO。整題只用到 push / top / pop / empty 四個操作。
// -----------------------------------------------------------------------------
bool isValid(const string& s) {
    stack<char> stk;                        // 預設底層 deque<char>
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stk.push(c);
        } else {
            if (stk.empty()) return false;  // 先檢查！對空 stack 呼叫 top() 是 UB
            char t = stk.top();             // 先看
            if ((c == ')' && t != '(') ||
                (c == ']' && t != '[') ||
                (c == '}' && t != '{')) {
                return false;
            }
            stk.pop();                      // 再移除（兩步式，例外安全）
        }
    }
    return stk.empty();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】巢狀設定檔 / JSON 的深度追蹤與縮排檢查
//   情境：解析 YAML / JSON / XML 這類巢狀結構時，需要知道「目前在第幾層」
//         以及「這個結束符號對應哪個開始符號」。
//         用 stack 記錄「尚未閉合的區段名稱」，遇到結束符號就 pop，
//         這樣任何不匹配都能立刻定位到出錯的行號。
// -----------------------------------------------------------------------------
struct ParseResult {
    bool ok = true;
    int errorLine = -1;
    string message;
    int maxDepth = 0;
};

ParseResult checkNesting(const vector<string>& lines) {
    ParseResult r;
    stack<pair<string, int>> open;      // (區段名稱, 起始行號)

    for (size_t i = 0; i < lines.size(); ++i) {
        const string& line = lines[i];
        int lineNo = static_cast<int>(i) + 1;

        if (line.rfind("BEGIN ", 0) == 0) {             // 以 "BEGIN " 開頭
            open.push({line.substr(6), lineNo});
            r.maxDepth = max(r.maxDepth, static_cast<int>(open.size()));
        } else if (line.rfind("END ", 0) == 0) {
            string name = line.substr(4);
            if (open.empty()) {
                r.ok = false; r.errorLine = lineNo;
                r.message = "END " + name + " 沒有對應的 BEGIN";
                return r;
            }
            if (open.top().first != name) {             // 先 top() 看
                r.ok = false; r.errorLine = lineNo;
                r.message = "END " + name + " 與 BEGIN " + open.top().first + " 不匹配";
                return r;
            }
            open.pop();                                 // 再 pop() 移除
        }
    }
    if (!open.empty()) {
        r.ok = false;
        r.errorLine = open.top().second;
        r.message = "BEGIN " + open.top().first + " 沒有對應的 END";
    }
    return r;
}

int main() {
    cout << "=== 1. stack 基本操作 ===" << endl;
    stack<int> s;       // 預設底層是 deque<int>

    s.push(10);
    s.push(20);
    s.push(30);

    cout << "top: " << s.top() << endl;    // 30
    cout << "size: " << s.size() << endl;  // 3

    s.pop();
    cout << "pop 後 top: " << s.top() << endl;  // 20

    // 逐一取出
    cout << "逐一取出：";
    while (!s.empty()) {
        cout << s.top() << " ";
        s.pop();
    }
    cout << endl;
    // 輸出：20 10

    cout << "\n=== 2. 證明預設底層真的是 deque ===" << endl;
    cout << "is_same_v<stack<int>::container_type, deque<int>> = "
         << boolalpha
         << is_same_v<stack<int>::container_type, deque<int>> << endl;
    cout << "is_same_v<stack<int>::container_type, vector<int>> = "
         << is_same_v<stack<int>::container_type, vector<int>> << endl;
    cout << "→ 這是編譯期就能驗證的事實，不需要猜" << endl;

    cout << "\n=== 3. 換底層容器：語意不變，效能特性改變 ===" << endl;
    stack<int, vector<int>> sv;         // 用 vector 當底層
    stack<int, deque<int>>  sd;         // 明確指定 deque
    for (int i = 1; i <= 3; ++i) { sv.push(i * 10); sd.push(i * 10); }
    cout << "vector 底層 top = " << sv.top()
         << "，deque 底層 top = " << sd.top() << endl;
    cout << "→ 對使用者而言介面完全相同；差別只在擴容行為與快取局部性" << endl;

    cout << "\n=== 4. 配接器「拿掉」了什麼（刻意的限制）===" << endl;
    cout << "stack 沒有 begin()/end()  → 不能遍歷" << endl;
    cout << "stack 沒有 operator[]     → 不能隨機存取" << endl;
    cout << "stack 沒有 insert/erase   → 不能從中間動手腳" << endl;
    cout << "以下三行若取消註解都會編譯失敗（這正是設計目的）：" << endl;
    cout << "  // for (int x : s) ...        ← 沒有 begin/end" << endl;
    cout << "  // s[0];                      ← 沒有 operator[]" << endl;
    cout << "  // s.insert(s.begin(), 99);   ← 沒有 insert" << endl;

    cout << "\n=== 5. pop() 不回傳值：正確的取值寫法 ===" << endl;
    stack<string> names;
    names.push("Alice");
    names.push("Bob");
    // string x = names.pop();          // ← 編譯失敗：pop() 回傳 void
    string x = names.top();             // ✅ 先取值
    names.pop();                        // ✅ 再移除
    cout << "取出的值 = " << x << "，剩餘 size = " << names.size() << endl;
    cout << "→ 兩步式是例外安全的必然結果，不是設計疏忽" << endl;

    cout << "\n=== LeetCode 20. Valid Parentheses ===" << endl;
    const vector<string> cases = {"()", "()[]{}", "(]", "([)]", "{[]}", "", "((("};
    for (const string& c : cases) {
        cout << "  \"" << c << "\" → " << (isValid(c) ? "true" : "false") << endl;
    }

    cout << "\n=== 日常實務：巢狀設定檔的區段配對檢查 ===" << endl;
    vector<string> goodCfg = {
        "BEGIN server", "  listen 8080", "BEGIN tls",
        "  cert /etc/cert.pem", "END tls", "END server"
    };
    ParseResult r1 = checkNesting(goodCfg);
    cout << "設定檔 A：" << (r1.ok ? "格式正確" : "錯誤")
         << "，最大巢狀深度 = " << r1.maxDepth << endl;

    vector<string> badCfg = {
        "BEGIN server", "BEGIN tls", "END server", "END tls"
    };
    ParseResult r2 = checkNesting(badCfg);
    cout << "設定檔 B：" << (r2.ok ? "格式正確" : "錯誤")
         << "（第 " << r2.errorLine << " 行：" << r2.message << "）" << endl;

    vector<string> unclosedCfg = {"BEGIN server", "  listen 80"};
    ParseResult r3 = checkNesting(unclosedCfg);
    cout << "設定檔 C：" << (r3.ok ? "格式正確" : "錯誤")
         << "（第 " << r3.errorLine << " 行：" << r3.message << "）" << endl;
    cout << "→ 靠 stack 記住「尚未閉合的區段」，任何不匹配都能立刻定位行號" << endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 27 課：deque 作為 stackqueue 底層容器1.cpp" -o demo27_1

// === 預期輸出 ===
// === 1. stack 基本操作 ===
// top: 30
// size: 3
// pop 後 top: 20
// 逐一取出：20 10
//
// === 2. 證明預設底層真的是 deque ===
// is_same_v<stack<int>::container_type, deque<int>> = true
// is_same_v<stack<int>::container_type, vector<int>> = false
// → 這是編譯期就能驗證的事實，不需要猜
//
// === 3. 換底層容器：語意不變，效能特性改變 ===
// vector 底層 top = 30，deque 底層 top = 30
// → 對使用者而言介面完全相同；差別只在擴容行為與快取局部性
//
// === 4. 配接器「拿掉」了什麼（刻意的限制）===
// stack 沒有 begin()/end()  → 不能遍歷
// stack 沒有 operator[]     → 不能隨機存取
// stack 沒有 insert/erase   → 不能從中間動手腳
// 以下三行若取消註解都會編譯失敗（這正是設計目的）：
//   // for (int x : s) ...        ← 沒有 begin/end
//   // s[0];                      ← 沒有 operator[]
//   // s.insert(s.begin(), 99);   ← 沒有 insert
//
// === 5. pop() 不回傳值：正確的取值寫法 ===
// 取出的值 = Bob，剩餘 size = 1
// → 兩步式是例外安全的必然結果，不是設計疏忽
//
// === LeetCode 20. Valid Parentheses ===
//   "()" → true
//   "()[]{}" → true
//   "(]" → false
//   "([)]" → false
//   "{[]}" → true
//   "" → true
//   "(((" → false
//
// === 日常實務：巢狀設定檔的區段配對檢查 ===
// 設定檔 A：格式正確，最大巢狀深度 = 2
// 設定檔 B：錯誤（第 3 行：END server 與 BEGIN tls 不匹配）
// 設定檔 C：錯誤（第 1 行：BEGIN server 沒有對應的 END）
// → 靠 stack 記住「尚未閉合的區段」，任何不匹配都能立刻定位行號
