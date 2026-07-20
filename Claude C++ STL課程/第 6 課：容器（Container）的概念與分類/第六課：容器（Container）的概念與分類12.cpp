// =============================================================================
//  第六課 12 — std::stack：用「限制介面」換取正確性的容器配接器
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T, class Container = std::deque<T>> class stack;  // <stack>,C++98 起
//
//   完整介面(就這麼少 —— 少是刻意的):
//     bool            empty() const;            // O(1)
//     size_type       size()  const;            // O(1)
//     reference       top();                    // O(1),存取頂端;空的時候是 UB
//     void            push(const T&);           // O(1) 攤銷
//     void            push(T&&);                // O(1) 攤銷,C++11
//     template<class... Args> void emplace(Args&&...);  // C++11(本機 -pedantic-errors 實測)
//     void            pop();                    // O(1),回傳 void;空的時候是 UB
//     void            swap(stack&);             // O(1),C++11
//
//   標準版本：stack 本身 C++98;emplace / push(T&&) / swap 是 C++11;
//             CTAD(std::stack s(vec) 免寫模板參數)是 C++17
//             —— 以上皆以 g++ -pedantic-errors 在本機逐一編譯驗證。
//   標頭檔：#include <stack>
//
// 【詳細解釋 Explanation】
//
// 【1. 最重要的一句話:stack 不是容器,是容器配接器(container adapter)】
// 這是初學者最常搞錯的一點。std::stack 自己不儲存任何資料、不管理記憶體、
// 也沒有配置器邏輯。它內部持有一個**真正的容器**(預設是 std::deque<T>),
// 所有工作都轉發給它:
//
//     stk.push(x)  →  c.push_back(x)
//     stk.top()    →  c.back()
//     stk.pop()    →  c.pop_back()
//     stk.size()   →  c.size()
//
// 也就是說,stack 做的事情不是「增加功能」,而是**減少功能**。deque 本來可以
// 隨機索引、可以從前端插入、可以用 iterator 遍歷;包成 stack 之後,這些全部
// 消失,只剩下堆疊該有的那幾個動作。
//
// 【2. 為什麼「減少功能」反而是價值?讓非法操作無法被寫出來】
// 這是整個 adapter 設計的核心,也是面試最愛問的點。
//
// 假設你直接用 std::vector 當堆疊。程式碼能跑,但半年後同事(或你自己)在
// 維護時寫下 v[0]、v.insert(v.begin(), x)、或 std::sort(v.begin(), v.end())
// —— 編譯器會很開心地全部放行,因為 vector 本來就支援這些。堆疊的「後進先出」
// 語意只存在於你的腦中和註解裡,編譯器完全不知情,也就無從保護。
//
// 換成 std::stack 之後,這些寫法**連編譯都過不了**。這叫做
// "make illegal states unrepresentable"(讓非法狀態無法被表達):
// 把約束從「靠人自律」上移到「靠型別系統強制」。程式碼一旦通過編譯,
// 就不可能有人不小心用非堆疊的方式碰它。
//
// 所以 stack 的介面貧乏不是功能缺失,而是**設計目的本身**。
//
// 【3. 沒有 iterator —— 這是刻意的,不是遺漏】
// std::stack 沒有 begin()/end()。這造成三個直接後果,全部都是預期中的:
//   (a) 不能用 range-based for 遍歷 stack;
//   (b) 不能餵給任何 STL 演算法(std::find、std::sort、std::accumulate 全部不行);
//   (c) 想「看看堆疊裡有什麼」只能一邊 top() 一邊 pop(),把它拆掉。
//
// 有人覺得這很不方便 —— 但反過來想:如果 stack 能被 std::sort 排序,那它還是
// 堆疊嗎?容許遍歷就等於容許「不照 LIFO 順序存取」,那條約束就破了。
// 需要遍歷,代表你要的其實不是堆疊,應該直接用 vector 或 deque。
//
// 【4. 為什麼預設底層是 deque 而不是 vector?】
// 這題很多人答不出來。stack 只需要在**一端**做 push/pop,兩者都能勝任,但:
//   * vector 容量滿時要重新配置一整塊更大的記憶體,再把**所有元素搬過去**。
//     單次 push 最壞是 O(n)(攤銷仍是 O(1)),而且搬移期間會有一段記憶體
//     尖峰(舊塊 + 新塊同時存在)。
//   * deque 由多個固定大小的區塊(chunk)組成,長大時只是**再配置一個新區塊**,
//     既有元素完全不必搬動,沒有大塊複製、沒有記憶體尖峰。
//
// 對「只在端點進出、且可能長很大」的堆疊來說,deque 的成長行為更平順,
// 所以標準選它當預設。這是深思熟慮的預設值,不是隨手挑的。
//
// 【5. 底層容器可以換 —— 而且有時該換】
//     std::stack<int>                      s1;  // 預設 deque(實作定義的預設)
//     std::stack<int, std::vector<int>>    s2;  // 改用 vector
//     std::stack<int, std::list<int>>      s3;  // 改用 list,也合法
//
// 一個容器只要提供 back()、push_back()、pop_back()、size()、empty(),
// 就能當 stack 的底層。實務上的取捨:
//   * 元素少、要極致的 cache 友善、且能先 reserve → 換 vector 常更快
//     (資料完全連續,deque 多一層區塊索引的間接存取)。
//   * 元素數量大且無法預估 → 留著預設的 deque。
// 注意 std::vector 版無法直接呼叫 reserve()(stack 沒轉發這個介面),
// 要先建好 vector 再用它建構 stack。
//
// 【6. pop() 為什麼回傳 void?—— 例外安全,不是設計失誤】
// 幾乎每個人第一次用都會想:為什麼不能寫 int x = stk.pop(); 要拆成兩行?
//
//     int x = stk.top();   // 先讀
//     stk.pop();           // 再移除
//
// 真正的理由是**強例外安全保證(strong exception safety)**。
// 假設 pop() 會回傳值,它的實作必然是:
//     T pop() { T tmp = c.back(); c.pop_back(); return tmp; }   // 假想
// 回傳時要複製/搬移建構一份給呼叫端。如果 T 的複製建構子在**這一刻丟出例外**,
// 元素已經從容器裡被移除了,而值又沒送到呼叫端手上 —— 資料就這樣人間蒸發,
// 且無法復原(容器狀態已改變,無法回捲)。
//
// 拆成 top() + pop() 就沒有這個問題:top() 只回傳 reference 不複製,不會丟例外;
// 複製動作發生在使用者自己的指派式,失敗了元素還好端端留在堆疊裡,可以重試。
// 這是 C++ 標準庫「寧可介面囉嗦,也不要讓資料可能遺失」的典型取捨。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體佈局:配接器是真正的零成本包裝
//     libstdc++ 的 stack 簡化後就是:
//         template<class T, class Container = deque<T>>
//         class stack {
//         protected:
//             Container c;                       // 唯一的資料成員
//         public:
//             void push(const T& x) { c.push_back(x); }
//             void pop()            { c.pop_back(); }
//             T&   top()            { return c.back(); }
//             // ...
//         };
//     只有一個成員。本機實測(GCC 15.2.0 / x86-64,實作定義):
//         sizeof(std::stack<int>) == 80 == sizeof(std::deque<int>)
//     完全相等 —— 配接器本身不佔任何額外空間。而那些一行的轉發函式在 -O2
//     下會被完全 inline 掉,機器碼與直接呼叫 c.push_back(x) 沒有差別。
//     所以「用 stack 比較慢」是錯的:它在執行期的成本是零,價值全部兌現在編譯期。
//
// (B) 成員是 protected 而非 private —— 一個少見但刻意的設計
//     標準規定底層容器 c 的存取權是 protected。這代表你可以繼承 std::stack
//     來取得 c,做一些配接器沒開放的事(例如為了除錯而遍歷內容):
//         class DebugStack : public std::stack<int> {
//         public: const container_type& raw() const { return c; }
//         };
//     這是標準留的一道後門。但要清楚:一旦這麼做,你就親手拆掉了本主題
//     【2】講的那層保護,應該只用於除錯或極特殊需求。
//
// (C) 「stack」這個字的兩種意思,別混在一起
//     * std::stack:一種資料結構(LIFO 容器),資料實際存在 **heap**
//       (由底層 deque/vector 配置)。
//     * call stack(呼叫堆疊):執行緒的區域變數與返回位址所在的那塊記憶體。
//     兩者只是名字都叫堆疊、行為都是後進先出,實體上毫無關係。
//     std::stack<int> s; 這個物件本身可能在 call stack 上,但它裝的元素在 heap。
//
// 【注意事項 Pay Attention】
//  1. 對空的 stack 呼叫 top() 或 pop() 是 **undefined behavior**。
//     配接器**不做任何檢查** —— 它會直接轉發給底層容器的 back()/pop_back(),
//     而那些也不檢查。行為不保證、不可預測:可能讀到殘留的舊值、可能讓
//     size() 溢位成極大值、可能當場崩潰,也可能看起來正常跑完。
//     絕不能拿「測試時沒事」當作正確的證據。正確寫法永遠是先 if (!s.empty())。
//     (本檔的可執行程式碼因此不會對空 stack 做任何存取。)
//  2. pop() 不回傳值,一定要先 top() 再 pop()。若寫成
//     std::cout << s.top(); s.pop(); 沒問題;但寫成
//     f(s.top(), (s.pop(), 0)) 這種把兩者塞進同一個運算式的花招,
//     求值順序會讓你踩到陷阱,別這樣寫。
//  3. 沒有 iterator,所以不能 range-based for、不能用 STL 演算法、
//     也沒有 clear()。想清空只能 while (!s.empty()) s.pop();
//     或直接 s = std::stack<int>();(整個換掉)。
//  4. 底層容器的預設型別(deque)是**實作定義**的觀察值嗎?—— 不是,
//     標準明文規定預設為 std::deque<T>。但 deque 自身的 chunk 大小、
//     sizeof(deque<int>)==80 這類數值才是實作定義,不同標準庫會不同。
//  5. std::stack 沒有 reserve()。若你選了 vector 當底層又想預留容量,
//     要先建好並 reserve 好一個 vector,再用它建構 stack。
//  6. 兩個 stack 可以用 ==、< 比較(轉發給底層容器的比較),但這是逐元素
//     比較底層容器,語意上未必是你想要的「堆疊相等」,用之前想清楚。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::stack
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. std::stack 是容器嗎?它和直接用 std::vector 當堆疊有什麼差別?
//     答：不是容器,是**容器配接器**。它內部包一個真正的容器(預設 std::deque),
//         把所有操作轉發過去,自己不存資料。關鍵差別在於它**移除**了介面:
//         vector 可以索引、可以中間插入、可以排序,這些在 stack 上連編譯都不過。
//         也就是把「只能後進先出」這條約束從註解與人的自律,上移到型別系統強制,
//         讓非法操作無法被寫出來。成本則是零 —— 本機實測
//         sizeof(std::stack<int>) == sizeof(std::deque<int>) == 80,
//         轉發函式在 -O2 下會被完全 inline。
//     追問：那為什麼 stack 沒有 iterator?
//         → 刻意的。開放遍歷就等於開放「不照 LIFO 存取」,那條約束就破了。
//           需要遍歷代表你要的其實不是堆疊,該直接用 vector/deque。
//
// 🔥 Q2. 為什麼 stack::pop() 回傳 void,而不是回傳被彈出的那個元素?
//     答：為了**強例外安全保證**。若 pop() 要回傳值,就得在移除元素之後、
//         把值送回呼叫端之前做一次複製/搬移建構;若 T 的複製建構子在這一刻
//         丟出例外,元素已經從容器移除、值又沒交到呼叫端手上,資料就遺失且
//         無法復原。拆成 top()(只回傳 reference,不複製、不丟例外)+ pop()
//         (只移除)之後,複製發生在使用者自己的指派式,失敗時元素仍在堆疊裡,
//         可以安全重試。
//     追問：C++11 有了 move semantics,現在可以加一個回傳值的 pop 了嗎?
//         → 仍然不行。move 建構子雖然通常 noexcept,但標準不保證所有型別
//           都如此;而且介面已經穩定二十年,改變會破壞相容性。需要這個語意
//           可以自己包一個 helper。
//
// ⚠️ 陷阱. 對空的 std::stack 呼叫 top(),會發生什麼事?
//     答：這是 **undefined behavior**,行為不保證也不可預測。stack 不做任何
//         檢查,直接轉發給底層容器的 back();底層同樣不檢查,於是就去讀一塊
//         不屬於任何有效元素的記憶體。可能讀到殘留舊值、可能崩潰、也可能
//         看起來完全正常。正確做法永遠是先檢查 empty()。
//     為什麼會錯：多數人把 at() 的行為(越界丟 std::out_of_range)投射到
//         整個標準庫,以為「STL 會幫我擋」。事實上只有 at() 這類**明文承諾
//         檢查**的介面才檢查;top()/pop()/front()/back()/operator[] 全都
//         走「不檢查、換效能」路線。更危險的是它常常「測試時剛好沒事」,
//         讓人誤以為程式是對的。
//
// ⚠️ 陷阱. 想看看 stack 裡目前有哪些元素,寫個 for 迴圈印出來就好了吧?
//     答：做不到。std::stack 沒有 begin()/end(),不能 range-based for、
//         也不能用任何 STL 演算法。唯一的辦法是複製一份出來,一邊 top()
//         一邊 pop() 把它拆掉(拆的是複製品,原件才不會被破壞)。
//     為什麼會錯：把配接器當成「功能比較少的容器」,以為容器的基本能力
//         (遍歷)總該還在。實際上配接器是**完全不同的東西** —— 它的介面
//         就是完整清單,上面沒有的就是不存在,而不是「藏起來了」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <stack>
#include <vector>
#include <string>
#include <deque>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 20. Valid Parentheses
//   題目：給一個只含 "()[]{}" 的字串,判斷括號是否正確配對且正確巢狀。
//   為什麼用到本主題：這是堆疊的教科書級應用,也是「為什麼需要堆疊」最好的答案。
//     括號的巢狀規則是「最後打開的必須最先關閉」—— 這正是 LIFO 的定義。
//     遇到左括號就 push,遇到右括號就檢查 top() 是否為對應的左括號:
//     配對成功就 pop,失敗就直接判定非法。
//   複雜度：時間 O(n),空間 O(n)(最壞情況全是左括號)。
// -----------------------------------------------------------------------------
bool isValidParentheses(const std::string& s) {
    std::stack<char> stk;

    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            stk.push(c);                      // 左括號:記住它,等待被關閉
        } else {
            // 右括號:堆疊空代表沒有對應的左括號 → 非法
            // 注意這個 empty() 檢查是必要的,不是防禦性冗餘 ——
            // 少了它,對空堆疊呼叫 top() 就是 undefined behavior。
            if (stk.empty()) return false;

            char open = stk.top();
            if ((c == ')' && open != '(') ||
                (c == ']' && open != '[') ||
                (c == '}' && open != '{')) {
                return false;                 // 型別不匹配,例如 "(]"
            }
            stk.pop();                        // 配對成功,消掉這一組
        }
    }
    // 全部處理完後堆疊必須是空的;有殘留代表有左括號沒被關閉,例如 "((("
    return stk.empty();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】文字編輯器的復原(Undo)/重做(Redo)機制
//   情境：任何編輯器(VS Code、Word、Photoshop)的 Ctrl+Z / Ctrl+Y。
//     使用者每做一個動作就記錄一筆,復原時必須從**最近一筆**開始往回退 ——
//     這是 LIFO,所以用兩個堆疊是業界標準做法:
//       undo_stack:已執行的動作,top() 是最近一筆
//       redo_stack:被復原掉的動作,等著被重做
//   為什麼用到本主題：用 stack 而不是 vector,是因為這裡**只需要**後進先出,
//     而型別系統會確保沒有人不小心寫出 undo_stack[3] 這種跳著存取的程式碼
//     —— 那會破壞復原順序,而且是極難追查的 bug。
// -----------------------------------------------------------------------------
class TextEditor {
public:
    // 執行一個編輯動作:記進 undo 堆疊,並清空 redo
    // (產生新動作後,原本的重做鏈就失效了 —— 這也是所有編輯器的行為)
    void type(const std::string& text) {
        content_ += text;
        undo_stack_.push(text);
        redo_stack_ = std::stack<std::string>();   // stack 沒有 clear(),整個換掉
    }

    bool undo() {
        if (undo_stack_.empty()) return false;     // 沒東西可復原,絕不能直接 top()
        std::string last = undo_stack_.top();      // 先讀
        undo_stack_.pop();                         // 再移除(pop 回傳 void)
        content_.erase(content_.size() - last.size());
        redo_stack_.push(last);
        return true;
    }

    bool redo() {
        if (redo_stack_.empty()) return false;
        std::string act = redo_stack_.top();
        redo_stack_.pop();
        content_ += act;
        undo_stack_.push(act);
        return true;
    }

    const std::string& content() const { return content_; }
    size_t undoDepth() const { return undo_stack_.size(); }

private:
    std::string content_;
    std::stack<std::string> undo_stack_;
    std::stack<std::string> redo_stack_;
};

int main() {
    // ── 原始課堂示範:std::stack 的基本操作 ────────────────────────────────
    std::cout << "=== std::stack ===" << std::endl;

    std::stack<int> stk;

    // 只能從頂端操作
    stk.push(10);
    stk.push(20);
    stk.push(30);

    std::cout << "頂端: " << stk.top() << std::endl;
    std::cout << "大小: " << stk.size() << std::endl;

    // 依序取出（後進先出）
    std::cout << "依序取出: ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;

    // ── 配接器是零成本包裝的實證 ──────────────────────────────────────────
    std::cout << "\n=== 配接器零成本實證 ===" << std::endl;
    std::cout << "sizeof(std::stack<int>) = " << sizeof(std::stack<int>) << std::endl;
    std::cout << "sizeof(std::deque<int>) = " << sizeof(std::deque<int>) << std::endl;
    std::cout << "兩者相等 → 配接器本身不佔額外空間(數值為實作定義)" << std::endl;
    // 預設底層容器由標準規定為 std::deque<T>
    static_assert(std::is_same<std::stack<int>::container_type, std::deque<int>>::value,
                  "stack 的預設底層容器是 std::deque");

    // ── 換掉底層容器 ──────────────────────────────────────────────────────
    std::cout << "\n=== 更換底層容器 ===" << std::endl;
    std::stack<int, std::vector<int>> vstk;    // 改用 vector 當底層
    vstk.push(1);
    vstk.push(2);
    vstk.push(3);
    std::cout << "std::stack<int, std::vector<int>> 頂端: " << vstk.top()
              << ",大小: " << vstk.size() << std::endl;
    std::cout << "介面完全一樣,只是底層儲存策略換了" << std::endl;

    // ── 用既有容器建構(CTAD,C++17) ─────────────────────────────────────
    std::cout << "\n=== 從既有容器建構(CTAD,C++17) ===" << std::endl;
    std::vector<int> src = {7, 8, 9};
    std::stack s2(src);                        // 免寫模板參數,自動推導
    std::cout << "由 vector{7,8,9} 建成的 stack,頂端 = " << s2.top()
              << "(vector 的最後一個元素)" << std::endl;

    // ── 空堆疊的安全處理 ──────────────────────────────────────────────────
    std::cout << "\n=== 空堆疊的安全處理 ===" << std::endl;
    std::stack<int> empty_stk;
    std::cout << "empty() = " << std::boolalpha << empty_stk.empty() << std::endl;
    std::cout << "對空堆疊呼叫 top()/pop() 是 undefined behavior,不保證結果,故不示範"
              << std::endl;

    // ── LeetCode 20 ───────────────────────────────────────────────────────
    std::cout << "\n=== LeetCode 20. Valid Parentheses ===" << std::endl;
    for (const char* t : {"()", "()[]{}", "(]", "([)]", "{[]}", "((("}) {
        std::cout << "  \"" << t << "\" → " << std::boolalpha
                  << isValidParentheses(t) << std::endl;
    }

    // ── 日常實務:編輯器的 Undo / Redo ────────────────────────────────────
    std::cout << "\n=== 日常實務: 編輯器 Undo / Redo ===" << std::endl;
    TextEditor ed;
    ed.type("Hello");
    ed.type(", ");
    ed.type("World");
    std::cout << "輸入三段後: \"" << ed.content() << "\" (undo 深度 "
              << ed.undoDepth() << ")" << std::endl;

    ed.undo();
    std::cout << "undo 一次:   \"" << ed.content() << "\"" << std::endl;
    ed.undo();
    std::cout << "undo 再一次: \"" << ed.content() << "\"" << std::endl;

    ed.redo();
    std::cout << "redo 一次:   \"" << ed.content() << "\"" << std::endl;

    ed.type("!");   // 產生新動作 → redo 鏈失效
    std::cout << "輸入新內容:  \"" << ed.content() << "\"" << std::endl;
    std::cout << "此時再 redo → " << (ed.redo() ? "成功" : "失敗(新動作已使重做鏈失效)")
              << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第六課：容器（Container）的概念與分類12.cpp" -o stack_demo

// === 預期輸出 ===
// === std::stack ===
// 頂端: 30
// 大小: 3
// 依序取出: 30 20 10
//
// === 配接器零成本實證 ===
// sizeof(std::stack<int>) = 80
// sizeof(std::deque<int>) = 80
// 兩者相等 → 配接器本身不佔額外空間(數值為實作定義)
//
// === 更換底層容器 ===
// std::stack<int, std::vector<int>> 頂端: 3,大小: 3
// 介面完全一樣,只是底層儲存策略換了
//
// === 從既有容器建構(CTAD,C++17) ===
// 由 vector{7,8,9} 建成的 stack,頂端 = 9(vector 的最後一個元素)
//
// === 空堆疊的安全處理 ===
// empty() = true
// 對空堆疊呼叫 top()/pop() 是 undefined behavior,不保證結果,故不示範
//
// === LeetCode 20. Valid Parentheses ===
//   "()" → true
//   "()[]{}" → true
//   "(]" → false
//   "([)]" → false
//   "{[]}" → true
//   "(((" → false
//
// === 日常實務: 編輯器 Undo / Redo ===
// 輸入三段後: "Hello, World" (undo 深度 3)
// undo 一次:   "Hello, "
// undo 再一次: "Hello"
// redo 一次:   "Hello, "
// 輸入新內容:  "Hello, !"
// 此時再 redo → 失敗(新動作已使重做鏈失效)
