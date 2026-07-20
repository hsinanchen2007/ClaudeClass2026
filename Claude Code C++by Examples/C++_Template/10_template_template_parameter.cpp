// ============================================================================
//  10_template_template_parameter.cpp  ──  Template template parameter
// ============================================================================
//
//  【本篇目標】
//    Template template parameter (簡稱 TTP) 是把「一個 template」當成參數
//    傳給另一個 template。聽起來繞口，看範例就懂。
//
//  【動機】
//    一般 template 參數是「型別」，但有時你想把整個容器策略傳進來。例如：
//        ContainerWrapper<std::vector>          // 我要用 vector 為底
//        ContainerWrapper<std::deque>           // 我要用 deque 為底
//
//    上面寫法傳的是「沒填型別」的 std::vector / std::deque 本身，這就是
//    template template parameter。
//
//  【語法】
//
//        template <template<typename> class C>      // C++17 起 typename 也可
//        class Wrapper {
//            C<int> v;            // 在 Wrapper 內把 C 填上 int
//        };
//
//        Wrapper<std::vector>  a;   // 等同 std::vector<int> v
//        Wrapper<std::list>    b;   // 等同 std::list<int>   v
//
//    為什麼是 class 而不是 typename？歷史原因：在 template template
//    parameter 位置，C++14 之前必須寫 class；C++17 起 typename 也可以。
//
//  【容器需要幾個 template 參數？】
//    很多 STL 容器其實需要 2 個 template 參數 (元素 + allocator)：
//        std::vector<T, Alloc = std::allocator<T>>
//
//    所以下面這樣寫：
//        template <template<typename> class C>
//        class Wrapper {};
//        Wrapper<std::vector> w;     // ❌ 早期 C++ 編譯器會抱怨參數數量不符
//
//    解法：
//      (a) 用 variadic template template parameter (C++11)：
//          template <template<typename...> class C>
//      (b) 寫上完整參數：
//          template <template<typename, typename> class C>
//
//    (a) 較通用，本篇示範。
//
//  【這個技巧用在哪？】
//    - 想替換底層容器的高階資料結構 (Stack / Queue / Adapter)
//    - 想做「容器無關」演算法
//    - Type list / metaprogramming 經典手法
//
//  參考：https://en.cppreference.com/cpp/language/template_parameters
// ============================================================================

/*
補充筆記：template_template_parameter
  - template_template_parameter 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - template_template_parameter 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】Template Template Parameter（TTP）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. TTP 跟直接傳 typename Container 差在哪？什麼時候才值得用？
//     答：判準是「元素型別由誰來填」。使用者若能自己寫 Stack<std::vector<int>>，
//         用 typename Container 就夠；若你想讓他只選容器策略、元素型別由類別內部
//         填（GenericStack<int, std::vector> 內部才組出 std::vector<int>），才輪到 TTP。
//     追問：實務常見嗎？（相對少見，std::stack 這類 adapter 是典型場景）
//
// 🔥 Q2. 為什麼 TTP 幾乎都寫成 template<typename...> class C？
//     答：因為多數 STL 容器不只一個模板參數，std::vector 是 <元素型別, Allocator>。
//         把 TTP 寫死成單一參數的 template<typename> class C 去接 std::vector 並不
//         可靠；改成 variadic 之後，不論容器有幾個模板參數都能接，是最通用的寫法。
//
// Q3. TTP 的位置該寫 class 還是 typename？
//     答：C++14 以前該位置只能寫 class，C++17 起 typename 也合法。注意這跟「宣告
//         型別參數時 typename 與 class 等價」是不同的位置、不同的規則 ── 面試官
//         問「typename 和 class 有沒有差」時，這正是那個「有差」的地方。
// ═══════════════════════════════════════════════════════════════════════════

#include <deque>
#include <iostream>
#include <list>
#include <queue>
#include <vector>

// ─── 1. 最簡單的 TTP：Wrapper<C> ────────────────────────────────────────
//   注意 template<typename...>：用 variadic 是因為 std::vector 等容器有
//   多個 template 參數 (元素型別 + allocator)。
template <template<typename...> class C>
class Wrapper {
public:
    void add(int x) { c_.push_back(x); }
    void show() const {
        for (int x : c_) std::cout << x << ' ';
        std::cout << "\n";
    }
private:
    C<int> c_;          // 把 int 填進去
};

// ─── 2. 一般的 Stack 適配器：底層容器可選 (vector / deque / list) ────────
//   類似 std::stack 的精簡實作。
//   要求 C<T> 提供 back() / push_back() / pop_back() / empty() / size()。
//
//   注意：因為 std::stack 自己內部就是 template template parameter 的經
//   典範例 (std::stack<T, Container = std::deque<T>>)，這裡示範自己寫。
template <typename T, template<typename...> class C = std::deque>
class GenericStack {
public:
    void push(const T& v) { c_.push_back(v); }
    void pop()             { c_.pop_back(); }
    T&   top()             { return c_.back(); }
    const T& top() const   { return c_.back(); }
    bool empty() const     { return c_.empty(); }
    std::size_t size() const { return c_.size(); }
private:
    C<T> c_;
};

// ─── 3. Leetcode 225 ── Implement Stack using Queues ─────────────────────
//   題目：用 queue 實作 stack (push/pop/top/empty)。
//
//   經典解法：每次 push 時把所有元素旋轉一遍，使新元素永遠在 queue 前頭。
//        push(x): q.push(x); for (k=q.size()-1; k--;) { q.push(q.front()); q.pop(); }
//
//   時間：push O(n)，其餘 O(1)
//   空間：O(n)
//
//   為什麼放這裡？
//     用 template template parameter 把 queue 抽出來：使用者可以選 std::queue
//     或別的 FIFO 容器策略。
//
//   底層 Q 必須是 queue-like：提供 push、pop、front、size、empty。
template <typename T, template<typename...> class Q = std::queue>
class MyStack {
public:
    void push(const T& v) {
        q_.push(v);
        // 把舊的元素全部繞到 v 後面，讓 v 變成 queue front。
        for (std::size_t k = q_.size() - 1; k > 0; --k) {
            q_.push(q_.front());
            q_.pop();
        }
    }
    T pop() {
        T v = q_.front();
        q_.pop();
        return v;
    }
    const T& top()  const { return q_.front(); }
    bool empty()    const { return q_.empty(); }
    std::size_t size() const { return q_.size(); }
private:
    Q<T> q_;
};

// ─── 4. 工作實用範例：通用 Adapter「印任何容器內容」 ─────────────────────
//   接受任何只要支援 begin()/end() 的容器。
//   實際上不一定需要 TTP；用普通 typename Container 就行。但本篇示範 TTP
//   寫法以對比學習。
template <template<typename...> class C, typename T>
void print_container(const C<T>& c) {
    std::cout << "[ ";
    for (const T& x : c) std::cout << x << ' ';
    std::cout << "]\n";
}

// ─── 5. Leetcode 20 ── Valid Parentheses (TTP 選底層 stack) ──────────────
//   難度: easy
//   題目：給字串只含 ()[]{}, 判斷是否「成對且正確嵌套」。
//   範例："()[]{}"→true; "(]"→false; "([)]"→false
//
//   經典 stack 解法：遇到開括號 push；遇到閉括號比對 top。
//
//   為什麼放在這裡？
//     用 template-template parameter 讓使用者選擇底層 stack 容器 (deque
//     或 vector)。STL std::stack 本身也是這種設計。
//
//   時間：O(n)；空間：O(n)。
template <template<typename...> class C = std::deque>
bool is_valid_parens(const std::string& s) {
    C<char> st;
    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') { st.push_back(c); continue; }
        if (st.empty()) return false;
        char top = st.back();
        if ((c == ')' && top != '(') ||
            (c == ']' && top != '[') ||
            (c == '}' && top != '{')) return false;
        st.pop_back();
    }
    return st.empty();
}

// ─── 6. 工作實用：通用「容器最大值」 ─────────────────────────────────────
//   用 TTP 接任意容器，元素型別 T 也是 template 參數。
template <template<typename...> class C, typename T>
T max_in(const C<T>& c) {
    T best = T{};
    bool first = true;
    for (const T& x : c) {
        if (first || x > best) { best = x; first = false; }
    }
    return best;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Wrapper：底層可選 vector / list / deque
    Wrapper<std::vector> wv;
    wv.add(1); wv.add(2); wv.add(3);
    std::cout << "Wrapper<vector>: "; wv.show();

    Wrapper<std::list> wl;
    wl.add(10); wl.add(20);
    std::cout << "Wrapper<list>:   "; wl.show();

    // (2) GenericStack
    GenericStack<int> s1;            // 預設 std::deque
    s1.push(1); s1.push(2); s1.push(3);
    std::cout << "GenericStack(deque) top = " << s1.top() << "\n";

    GenericStack<int, std::vector> s2;
    s2.push(7); s2.push(8); s2.push(9);
    std::cout << "GenericStack(vector) top = " << s2.top() << "\n";

    // (3) Leetcode 225
    MyStack<int> st;
    st.push(1); st.push(2); st.push(3);
    std::cout << "MyStack top = " << st.top() << " (expect 3)\n";
    std::cout << "MyStack pop = " << st.pop() << " (expect 3)\n";
    std::cout << "MyStack pop = " << st.pop() << " (expect 2)\n";

    // (4) print_container
    print_container(std::vector<int>{1, 2, 3});
    print_container(std::list<int>{4, 5, 6});

    // (5) Leetcode 20 Valid Parentheses
    std::cout << std::boolalpha;
    std::cout << "is_valid_parens(\"()[]{}\") = " << is_valid_parens<>("()[]{}") << "\n";
    std::cout << "is_valid_parens(\"(]\")     = " << is_valid_parens<>("(]") << "\n";
    std::cout << "is_valid_parens(\"([)]\")   = " << is_valid_parens<std::vector>("([)]") << "\n";

    // (6) max_in
    std::cout << "max_in(vector{3,1,4,1,5,9,2,6}) = "
              << max_in(std::vector<int>{3, 1, 4, 1, 5, 9, 2, 6}) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::map 為什麼是 `map<K,V,Compare>` 而不是用 template-template
    //      parameter 把比較器整個傳進來？
    //    A：因為 std::less<K> 已經是「填好型別」的具體型別 (functor 物件)，
    //       map 內部只需要 typename Compare = std::less<K> 這種一般型別
    //       參數就夠用，並能傳入 lambda、function pointer、stateful 比
    //       較器等。template-template parameter 用在「需要在 class 裡
    //       自己再填型別」的場合，例如把容器策略 std::vector / std::deque
    //       整個傳進去自己決定元素型別。實務上 TTP 是相對少見的工具。
    //
    //  Q2：為什麼 `template<template<typename> class C>` 寫死一個型別參
    //      數，傳 std::vector 進去常常會編譯錯？
    //    A：std::vector 實際上是 `template<typename, typename = allocator<T>>`
    //       兩個參數 (含預設)。早期編譯器嚴格匹配參數個數，會抱怨「期
    //       望 1 個參數但 vector 有 2 個」。解法：用 variadic 寫法
    //       `template<template<typename...> class C>`，這樣不論容器有多
    //       少 template 參數都能接。CWG 150 之後標準也明確允許「多餘的
    //       參數有預設值」算匹配，但 variadic 仍是最通用的寫法。
    //
    //  Q3：TTP 跟「直接傳完整型別 (typename Container)」相比，什麼時候
    //      值得用 TTP？
    //    A：判斷標準是「class 裡需不需要替使用者填型別」。如果使用者已
    //       經能寫 `Stack<std::vector<int>>` (容器型別整個給定)，那
    //       typename Container 就夠；如果你只想讓使用者選「容器策略」、
    //       元素型別由你自己決定 (例如 `Stack<int, std::vector>` 內部
    //       才把 int 填進 vector)，這就是 TTP 的舞台。後者比前者少打字、
    //       語意更貼近「策略選擇」。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. TTP 適合「想把容器策略當參數傳進來」的場景。
//    2. 多數 STL 容器有多個 template 參數，所以 TTP 通常寫成 variadic：
//          template<template<typename...> class C>
//    3. 實務上，能用普通 typename Container 就用普通的；TTP 是相對少見的
//       工具，知道存在即可。
//
//  【下一篇】
//    11_alias_template.cpp ── Alias template (using)。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 10_template_template_parameter.cpp -o 10_template_template_parameter

// === 預期輸出 ===
// Wrapper<vector>: 1 2 3
// Wrapper<list>:   10 20
// GenericStack(deque) top = 3
// GenericStack(vector) top = 9
// MyStack top = 3 (expect 3)
// MyStack pop = 3 (expect 3)
// MyStack pop = 2 (expect 2)
// [ 1 2 3 ]
// [ 4 5 6 ]
// is_valid_parens("()[]{}") = true
// is_valid_parens("(]")     = false
// is_valid_parens("([)]")   = false
// max_in(vector{3,1,4,1,5,9,2,6}) = 9
