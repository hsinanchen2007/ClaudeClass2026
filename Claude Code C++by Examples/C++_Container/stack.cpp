// ============================================================================
//  stack.cpp — std::stack 完整學習筆記
// ----------------------------------------------------------------------------
//  編譯: g++ -std=c++20 -Wall -Wextra stack.cpp -o stack && ./stack
//  參考 (cppreference): https://en.cppreference.com/w/cpp/container/stack
//  參考 (cplusplus.com): https://cplusplus.com/reference/stack/stack/
// ----------------------------------------------------------------------------
//
//  ▌ 概述
//  std::stack 是「後進先出 (LIFO)」的容器轉接器 (container adaptor)。
//  它本身不是真正的 container — 而是包裝另一個 container,只暴露 stack 介面。
//
//  ▌ 底層 container
//  template <typename T, typename Container = std::deque<T>>
//  class stack;
//
//  預設底層為 std::deque (兩端皆 O(1))。也可指定為 vector / list:
//      std::stack<int, std::vector<int>>  s_vec;
//      std::stack<int, std::list<int>>    s_list;
//
//  底層 container 必須支援:back(), push_back(), pop_back(), empty(), size()
//
//  ▌ 所屬類別
//  Container adaptor (容器轉接器)
//
//  ▌ 時間複雜度
//      push / pop / top / size / empty   全部 O(1)
//      (轉嫁給底層 container)
//
//  ▌ 與其他 container 的比較
//      stack vs vector  : stack 是 LIFO 介面,vector 是通用 sequence
//      stack vs queue   : stack 後進先出,queue 先進先出
//
//  ▌ 適用情境
//      ✅ 需要 LIFO 行為:DFS、括號配對、回溯演算法、撤銷 (undo)
//      ✅ 想限制使用者只能用 push/pop/top,避免誤操作
//      ❌ 需要走訪所有元素 → stack 沒有 iterator,改用底層 container
//
//  ▌ 沒有的功能 (★與一般 container 對比)
//      • 沒有 begin / end / iterator (因此不能用 range-for)
//      • 沒有 operator[] / at / find
//      • 沒有 clear (要清空只能 while (!s.empty()) s.pop();)
//      • 沒有 reserve / capacity (即使底層是 vector)
//
// ============================================================================

/*
補充筆記：std::stack
  - stack 是 container adaptor，只開放 push/pop/top，刻意不讓你遍歷。
  - 預設底層容器通常是 deque；也可改用 vector 或 list，只要支援需要的操作。
  - 若需要搜尋或迭代，就不應使用 stack 這個介面。
  - std::stack 的核心是資料如何儲存、查找與保持順序；選容器前先想插入刪除位置、查找方式和 iterator 失效規則。
  - vector 連續記憶體適合索引和快取區域性，list/deque/set/map 則針對不同插入刪除或查找需求。
  - 關聯容器 set/map 依比較器排序，unordered 容器依 hash 分桶；兩者不是誰永遠比較快，而是前提與需求不同。
  - operator[] 在 map/unordered_map 查不到 key 會插入預設值；純查詢應使用 find、contains 或 at。
  - 容器元素型別若昂貴，優先理解 emplace、move 和 reference/iterator 有效性，不要盲目複製。
  - 所有容器都要考慮空容器邊界；front/back/top 在空容器上呼叫通常是未定義行為或前置條件違反。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::stack
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. stack 的底層預設容器是什麼？可以換嗎？
//     答：stack 是 container adaptor（本身不是真正的容器），預設底層是 std::deque。
//         只要底層提供 back() / push_back() / pop_back() / empty() / size() 就可以換，
//         所以 std::vector 與 std::list 都可以（`std::stack<int, std::vector<int>>`）。
//     追問：為什麼預設選 deque 而不是 vector？（deque 成長時不需要整塊搬移已有元素）
//
// 🔥 Q2. stack 為什麼沒有 iterator？沒有 clear？
//     答：adaptor 刻意只暴露 LIFO 介面（push / pop / top / empty / size），
//         以避免使用者繞過堆疊語意存取內部。因此沒有 begin / end（不能用 range-for）、
//         沒有 operator[] / find、沒有 clear（要清空只能 `while (!s.empty()) s.pop();`），
//         也沒有 reserve / capacity——即使底層是 vector 也一樣拿不到。
//
// ⚠️ 陷阱. 為什麼 pop() 不回傳被彈出的元素？
//     答：std::stack::pop() 的回傳型別是 void，要取值必須先 top() 再 pop()。
//         原因是 exception safety：若 pop() 要回傳 by value，它必須先移除元素再回傳副本，
//         而那個 copy / move constructor 一旦拋例外，元素已經不在容器裡了——資料永遠遺失。
//         拆成 top() + pop() 兩步，兩個動作都能各自安全。
//     為什麼會錯：很多人用其他語言（Python 的 list.pop()）的直覺，
//         以為 pop 一定會把值還給你。
// ═══════════════════════════════════════════════════════════════════════════

#include <stack>
#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <string>

template <typename Stack>
void dump(Stack s, const std::string& label = "") {
    // 注意:傳值,不影響原本的 stack
    if (!label.empty()) std::cout << label << " (top→bottom): ";
    std::cout << "[ ";
    while (!s.empty()) {
        std::cout << s.top() << ' ';
        s.pop();
    }
    std::cout << "]\n";
}

int main() {
    // ========================================================================
    //  1. 建構子
    // ========================================================================
    std::stack<int> s1;                                // 預設,底層為 deque
    std::stack<int, std::vector<int>> s2;              // 用 vector 為底層
    std::stack<int, std::list<int>>   s3;              // 用 list 為底層
    std::deque<int> d{1, 2, 3};
    std::stack<int> s4(d);                             // 由現有 container 建構
    std::stack<int> s5(std::move(d));

    s4.push(4);
    dump(s4, "s4                  ");

    // C++23: from container range
    // std::vector<int> v{1,2,3};
    // std::stack s_v{v.begin(), v.end()};   // CTAD,需要 C++23

    // ========================================================================
    //  2. 元素存取 — 只有 top()
    // ========================================================================
    //  top()       回傳堆頂元素的 reference (空 stack 是 UB)
    //  ★ 沒有 bottom(),也沒有任何位置查詢
    std::stack<int> s;
    s.push(10);
    s.push(20);
    s.push(30);
    std::cout << "\ntop = " << s.top() << '\n';        // 30

    // 修改頂端元素
    s.top() = 999;
    dump(s, "top modified        ");

    // ========================================================================
    //  3. 容量
    // ========================================================================
    //  empty / size  (沒有 capacity)
    std::cout << "size=" << s.size()
              << ", empty=" << std::boolalpha << s.empty() << '\n';

    // ========================================================================
    //  4. Modifiers
    // ========================================================================

    // ──── push(value) ────
    std::stack<int> p;
    p.push(1);
    p.push(2);
    p.push(3);
    dump(p, "after pushes        ");

    // ──── emplace(args...) ────
    // 直接以建構參數在底層建構元素,避免一次拷貝
    std::stack<std::string> ps;
    ps.emplace(5, 'A');                                 // string(5,'A')
    ps.emplace("hello");
    dump(ps, "emplace             ");

    // C++17 起 emplace 回傳 reference (對應 emplace_back 的回傳值)

    // ──── pop() ────
    // ★ 只刪除頂端元素,「不回傳」被刪元素 (與其他語言常不同)
    // 想取值,必須先 top() 再 pop()
    std::stack<int> pp;
    pp.push(1); pp.push(2); pp.push(3);
    int top_val = pp.top();
    pp.pop();
    std::cout << "\npop 取到 " << top_val << '\n';

    // ──── swap ────
    std::stack<int> a, b;
    a.push(1); a.push(2);
    b.push(9);
    a.swap(b);
    dump(a, "a after swap        ");
    dump(b, "b after swap        ");

    // ========================================================================
    //  5. 清空 stack 的慣用法
    // ========================================================================
    // ★ stack 沒有 clear()。三種常見方式:
    //
    //   while (!s.empty()) s.pop();             // 簡單但 O(n)
    //   s = std::stack<int>();                   // 重新賦值一個空的
    //   std::stack<int>().swap(s);               // swap idiom
    //
    std::stack<int> cl;
    cl.push(1); cl.push(2); cl.push(3);
    while (!cl.empty()) cl.pop();
    std::cout << "after clear, empty=" << cl.empty() << '\n';

    // ========================================================================
    //  6. 比較運算子
    // ========================================================================
    // 字典序比較 (依底層 container)
    std::stack<int> c1, c2;
    c1.push(1); c1.push(2);
    c2.push(1); c2.push(3);
    std::cout << "c1 < c2 ? " << (c1 < c2) << '\n';     // true

    // ========================================================================
    //  7. 不同底層 container 的差異
    // ========================================================================
    // deque (預設):兩端 O(1),不需連續記憶體
    // vector:        push 攤銷 O(1),元素連續,可能 rehash 觸發大量拷貝
    // list:          push 嚴格 O(1) (每次 new 一個節點),但常數大
    //
    // 一般用預設 deque 即可。除非有特殊理由 (例如你需要傳給 C API),
    // 否則改 vector 反而可能變慢。

    std::stack<int, std::vector<int>> sv;
    for (int i = 0; i < 5; ++i) sv.push(i);
    dump(sv, "stack on vector     ");

    // ========================================================================
    //  8. 經典應用:括號配對
    // ========================================================================
    auto balanced = [](const std::string& expr) {
        std::stack<char> stk;
        for (char c : expr) {
            if (c == '(' || c == '[' || c == '{') {
                stk.push(c);
            } else if (c == ')' || c == ']' || c == '}') {
                if (stk.empty()) return false;
                char open = stk.top();
                stk.pop();
                if ((c == ')' && open != '(') ||
                    (c == ']' && open != '[') ||
                    (c == '}' && open != '{'))
                    return false;
            }
        }
        return stk.empty();
    };
    std::cout << "\nbalanced(\"({[]})\") = " << balanced("({[]})") << '\n';
    std::cout << "balanced(\"({[)]})\") = " << balanced("({[)]})") << '\n';

    // ========================================================================
    //  9. 常見陷阱 (Pitfalls)
    // ========================================================================
    //
    //  (1) pop() 不回傳值
    //      要取值必須先 top() 再 pop()。
    //      這是「強例外保證」設計 — 若 pop 同時回傳值,
    //      移動建構若 throw 會讓 stack 進入詭異狀態。
    //
    //  (2) top() 對空 stack 是 UB
    //      呼叫前一定要 !empty()。
    //
    //  (3) 沒有 iterator,不能 for-range / find / erase
    //      若需要這些,直接用底層 container (vector / deque) 即可。
    //
    //  (4) 沒有 clear()
    //      用 while-pop 或 swap idiom。
    //
    //  (5) 用 vector 為底層時,push 大量元素仍可能觸發大量搬移
    //      因為 stack 沒有 reserve(),要先操作底層 container 自己 reserve。
    //
    //  (6) stack 不是 thread-safe
    //      多執行緒共享 stack 仍要自己加鎖。

    // ========================================================================
    //  10. 最佳實踐
    // ========================================================================
    //
    //  • LIFO 場景 → 用 stack 限制介面、減少誤操作
    //  • 想要走訪內容 → 用 vector 自己當 stack (push_back/pop_back)
    //  • 預設 deque 通常最佳;有特殊理由再換 vector / list
    //  • 用 emplace 而非 push 來避免暫時物件
    //  • 想清空 → swap idiom 最快 (常數時間,釋放記憶體)

    // ========================================================================
    //  11. LeetCode 實戰範例 ★ 真實場景常見題型
    // ========================================================================
    // 上面第 8 段已經示範了 LC 20: Valid Parentheses (有效括號)。
    // 這裡再示範 stack 在「單調棧 (monotonic stack)」與表達式求值類題目的應用。

    // ──── LC 739: Daily Temperatures (每日溫度) ────
    // 給每日氣溫,回傳一個陣列 ans[i] = 「第 i 天之後第幾天才會更熱」 (沒有則為 0)。
    // 經典「單調遞減棧」: 棧裡放下標,新元素若打破遞減,就把棧頂彈出並計算距離。
    // 每個元素最多進出一次 → O(n)。
    {
        std::vector<int> T{73, 74, 75, 71, 69, 72, 76, 73};
        std::vector<int> ans(T.size(), 0);
        std::stack<int> st;     // 存「下標」,nums[st] 嚴格遞減
        for (int i = 0; i < (int)T.size(); ++i) {
            while (!st.empty() && T[i] > T[st.top()]) {
                ans[st.top()] = i - st.top();
                st.pop();
            }
            st.push(i);
        }
        std::cout << "\n[LC739 Daily Temperatures] = [ ";
        for (int x : ans) std::cout << x << ' ';
        std::cout << "]\n";
        // 預期輸出: [ 1 1 4 2 1 1 0 0 ]
    }

    // ──── LC 150: Evaluate Reverse Polish Notation (後序表示式求值) ────
    // 輸入後序表達式 (RPN),用 stack 即可線性求值。
    // 遇到數字 → push;遇到運算子 → pop 兩個運算後 push 結果。
    {
        std::vector<std::string> tokens{"2", "1", "+", "3", "*"};   // (2+1)*3 = 9
        std::stack<int> st;
        for (const auto& t : tokens) {
            if (t == "+" || t == "-" || t == "*" || t == "/") {
                int b = st.top(); st.pop();
                int a = st.top(); st.pop();
                if (t == "+") st.push(a + b);
                else if (t == "-") st.push(a - b);
                else if (t == "*") st.push(a * b);
                else               st.push(a / b);
            } else {
                st.push(std::stoi(t));
            }
        }
        std::cout << "[LC150 RPN] (2+1)*3 = " << st.top() << '\n';
        // 預期輸出: 9
    }

    // ──── LC 155: Min Stack (最小棧) ────
    // 難度: medium
    // 設計支援 push / pop / top / getMin 都是 O(1) 的棧。
    // 經典作法:額外維護一個 min_stack,每次 push 時把「目前最小值」也 push 進去。
    // 這樣 getMin 就是 min_stack.top(),pop 時兩個棧同步彈。
    {
        struct MinStack {
            std::stack<int> data;
            std::stack<int> mins;   // 對應位置的「截至目前的最小值」
            void push(int x) {
                data.push(x);
                mins.push(mins.empty() ? x : std::min(mins.top(), x));
            }
            void pop()   { data.pop(); mins.pop(); }
            int  top()   { return data.top(); }
            int  getMin(){ return mins.top(); }
        };
        MinStack ms;
        ms.push(-2); ms.push(0); ms.push(-3);
        std::cout << "[LC155 MinStack] getMin=" << ms.getMin() << '\n';   // -3
        ms.pop();
        std::cout << "[LC155 MinStack] top   =" << ms.top()    << '\n';   // 0
        std::cout << "[LC155 MinStack] getMin=" << ms.getMin() << '\n';   // -2
    }

    // ========================================================================
    //  12. 實戰範例:函式呼叫追蹤 (Call Stack Profiler)
    // ========================================================================
    // 真實場景:做效能分析時,要追蹤「目前位於哪個函式呼叫鏈中」,例如:
    //   main -> parse -> tokenize -> read_char
    // 進入函式時 push 函式名 + 進入時間,離開時 pop 並計算停留時間。
    // 這正是 stack 的天職 — 後進先出完全對應函式呼叫的入棧出棧。
    {
        struct CallFrame { std::string func; long long enter_us; };
        std::stack<CallFrame> call_stack;
        auto enter = [&](const std::string& f, long long t){
            call_stack.push({f, t});
            std::cout << "  enter " << f << " @ t=" << t << '\n';
        };
        auto leave = [&](long long t){
            auto top = call_stack.top();
            call_stack.pop();
            std::cout << "  leave " << top.func
                      << " 耗時=" << (t - top.enter_us) << "us\n";
        };
        std::cout << "[Call Stack Profiler]\n";
        enter("main",     0);
        enter("parse",    5);
        enter("tokenize", 8);
        leave(20);      // tokenize 耗時 12us
        leave(30);      // parse    耗時 25us
        leave(40);      // main     耗時 40us
    }

    std::cout << "\n=== stack demo 結束 ===\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::stack 是 container 還是 adaptor?底層預設用什麼?
    //    A：是 container adaptor,預設底層是 std::deque (而非 vector)。原因是
    //       deque 在尾端 push/pop 同樣 O(1),但成長時不會像 vector 那樣 realloc
    //       搬移所有元素,提供更穩定的最壞情況效能。也可指定 vector 或 list。
    //
    //  Q2：為何 stack.pop() 沒有回傳值?要怎麼同時取頂端並彈出?
    //    A：若 pop 同時回傳 top,當回傳的拷貝建構丟例外時,元素已被刪除,違反
    //       strong exception guarantee。標準把它拆成 top() (查) + pop() (刪) 兩步,
    //       讓使用者自行控制例外安全。常見寫法: T x = st.top(); st.pop();
    //
    //  Q3：什麼題型最適合用單調棧 (monotonic stack)?
    //    A:「下一個更大 / 更小元素」相關問題 (LC739 每日溫度、LC496 下一個更大
    //       元素、LC84 柱狀圖最大矩形)。共通結構:由左至右掃,維持棧內元素遞減
    //       (找更大) 或遞增 (找更小);每個元素最多入棧出棧一次,整體 O(n)。
    //
    return 0;
}

/*
============================================================================
  附錄:std::stack 完整 member function 一覽
============================================================================
  Constructors / destructor / operator=

  Element access:  top

  Capacity:        empty, size

  Modifiers:       push, push_range (C++23), emplace, pop, swap

  Non-member:      operator==, !=, <, <=, >, >=, <=> (C++20)
                   std::swap

  ★ 沒有 iterator 系列、clear、reserve、capacity、find、operator[]
============================================================================
*/
