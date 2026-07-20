// ============================================================================
//  14_fold_expressions.cpp  ──  Fold Expressions (C++17)
// ============================================================================
//
//  【本篇目標】
//    C++17 引入 fold expression，把 variadic template 的「遞迴展開」簡化成
//    「一行」。本篇徹底拆解 fold 的 4 種形式、每種的展開順序與差異。
//
//  【為什麼需要】
//    回顧 file 13 的 print_all：
//        void print_all() {}
//        template<typename T, typename... Rest>
//        void print_all(T head, Rest... rest) {
//            std::cout << head;
//            print_all(rest...);
//        }
//    要寫兩個 overload (終止 + 遞迴)。fold expression 一行：
//        template<typename... Args>
//        void print_all(Args... args) { ((std::cout << args << ' '), ...); }
//
//  【4 種 fold】
//
//    令 op 為一個 binary operator (+、*、&&、,、<< 等)，pack = E1, E2, ..., En。
//
//    ① Unary right fold：    (pack op ...)
//          展開為：           E1 op (E2 op (E3 op ... (E(n-1) op En)...))
//          結合方向：「右結合」
//
//    ② Unary left fold：     (... op pack)
//          展開為：           ((((E1 op E2) op E3) op ...) op En)
//          結合方向：「左結合」
//
//    ③ Binary right fold：   (pack op ... op init)
//          展開為：           E1 op (E2 op (... (En op init)))
//          init 在最右
//
//    ④ Binary left fold：    (init op ... op pack)
//          展開為：           ((init op E1) op E2) op ... op En
//          init 在最左
//
//    Binary 形式的好處：當 pack 為「空」時，unary fold 會編譯錯誤 (沒有
//    身分元)，binary fold 則回傳 init。例外：少數 operator (&&、||、,) 對
//    空 pack 有預設身分元 (true、false、void())。
//
//  【常用搭配】
//    - 求和：     (... + args)        左 fold
//    - 全部 AND： (... && conds)
//    - 全部印：   ((std::cout << args << ' '), ...)   逗號運算子展開
//    - 加進容器： (vec.push_back(args), ...)
//
//  參考：https://en.cppreference.com/cpp/language/fold
// ============================================================================

/*
補充筆記：fold_expressions
  - fold expression 把 parameter pack 用運算子摺疊成一個表達式。
  - 左摺疊和右摺疊在非結合運算上結果不同。
  - 空 pack 只有少數運算有預設 identity，其他情況要提供初始值。
  - fold_expressions 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】fold expression（摺疊表達式）
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. fold expression 屬哪個標準？有哪四種形式？
//     答：C++17，不是 C++11／C++14——這是最常被追打的分界（本機以
//         -std=c++14 -pedantic-errors 實測，GCC 明確拒絕：fold-expressions
//         only available with -std=c++17）。四種形式：一元右摺 (pack op ...)、
//         一元左摺 (... op pack)、二元右摺 (pack op ... op init)、
//         二元左摺 (init op ... op pack)。
//     追問：左摺與右摺什麼時候結果不同？（對 +、*、&&、|| 這種滿足結合律的
//         運算相同；對 -、/、<< 這種非結合運算，順序會直接改變結果）
//
// 🔥 Q2. 想對 pack 做「副作用」（列印、push_back）為什麼常用逗號 fold？
//     答：因為逗號運算子接受任何 void 表達式當運算元，而且空 pack 也合法
//         （展開為 void()）。寫成 ((std::cout << args << ' '), ...) 就能讓
//         每個元素依序展開執行，取代 C++17 之前要寫兩個 overload 的遞迴。
//
// ⚠️ 陷阱. (... + args) 對「空 pack」是合法的嗎？
//     答：不合法，是編譯錯誤（本機實測 GCC 訊息：fold of empty expansion over
//         operator+）。一元摺疊只有三個運算子對空 pack 有預設身分元：
//         && 得 true、|| 得 false、逗號 得 void()（三者本機皆實測通過）。
//         其餘運算子一律 ill-formed。要允許空 pack，就改用帶初值的二元形式
//         (0 + ... + args)——本檔 sum0 正是為此而寫。
//     為什麼會錯：直覺認為「空的加總自然是 0、空的乘積自然是 1」，但標準只替
//         &&、||、逗號這三個定義了身分元，不會自作主張替 + 補一個 0。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>

// ─── 1. unary left fold：求和 ────────────────────────────────────────────
template <typename... Args>
auto sum(Args... args) {
    return (... + args);            // 從左到右：((a + b) + c) + d
}

// ─── 2. binary left fold：求和但允許空 pack ──────────────────────────────
template <typename... Args>
auto sum0(Args... args) {
    return (0 + ... + args);        // 0 作 init，pack 空時回傳 0
}

// ─── 3. 逗號 fold：列印 ─────────────────────────────────────────────────
//   ((std::cout << args << ' '), ...) 展開後等價於：
//        (cout<<a<<' '), (cout<<b<<' '), (cout<<c<<' ');
template <typename... Args>
void print_all(const Args&... args) {
    ((std::cout << args << ' '), ...);
    std::cout << "\n";
}

// ─── 4. 全部都通過某個謂詞 ──────────────────────────────────────────────
template <typename... Args>
bool all_positive(Args... args) {
    return (... && (args > 0));     // 短路求值，只要有一個 false 就停
}

// ─── 5. 一次塞進容器 ────────────────────────────────────────────────────
template <typename T, typename... Args>
void push_all(std::vector<T>& v, const Args&... args) {
    (v.push_back(args), ...);
}

// ─── 6. Leetcode 1480 ── Running Sum (用 fold 概念複習) ─────────────────
//   雖然 LC 1480 主要不是 fold expression 的場景，但我們可以用 fold 做出
//   「對任意筆資料一次計算 sum」的工具，用在 running sum 內部。
//
//   下面 sum_args + running_sum 結合 demo：
//   running_sum_with_fold 對 vector 做累加，內部用 sum_args 表演 fold。
template <typename T>
std::vector<T> running_sum(const std::vector<T>& nums) {
    std::vector<T> r;
    r.reserve(nums.size());
    T acc{};
    for (const T& x : nums) { acc += x; r.push_back(acc); }
    return r;
}

template <typename... Args>
auto sum_args(Args... args) { return (0 + ... + args); }

// ─── 7. 工作實用：min_of / max_of (variadic + fold) ───────────────────────
//   std::min/max 接受 initializer_list，已經夠用，但用 fold 寫法可以對「不
//   同型別」的引數做 promotion，更彈性。
template <typename T, typename... Rest>
auto min_of(T first, Rest... rest) {
    auto result = first;
    ((result = (rest < result ? rest : result)), ...);
    return result;
}

template <typename T, typename... Rest>
auto max_of(T first, Rest... rest) {
    auto result = first;
    ((result = (rest > result ? rest : result)), ...);
    return result;
}

// ─── 8. 工作實用：「all / any / none of」 編譯期版 ───────────────────────
template <typename... Bools>
constexpr bool all_of(Bools... b) { return (... && b); }       // unary &&

template <typename... Bools>
constexpr bool any_of(Bools... b) { return (... || b); }       // unary ||

template <typename... Bools>
constexpr bool none_of(Bools... b) { return !(... || b); }

// ─── 9. Leetcode 1486 ── XOR Operation in an Array (用 fold 把任意數 XOR 起來) ─
//   難度: easy
//   題目：定義 nums[i] = start + 2*i，i in [0, n)；回傳所有 nums[i] 的 XOR。
//   範例：n=5, start=0 → [0,2,4,6,8] → 8
//
//   解法：直接累 XOR，O(n)、O(1)。
//
//   為什麼放在這裡？
//     用 fold expression 表示「任意數量 XOR」更直觀，類似 sum 但運算是 ^。
//     順手定義 xor_all variadic 工具。
int xor_operation(int n, int start) {
    int r = 0;
    for (int i = 0; i < n; ++i) r ^= (start + 2 * i);
    return r;
}

template <typename... Args>
constexpr auto xor_all(Args... args) { return (0 ^ ... ^ args); }   // binary left fold

// ─── 10. 工作實用：variadic log_format ────────────────────────────────
//   生產環境的型別安全 log：用 fold 一次性把所有 args 寫進 stream。
#include <sstream>
template <typename... Args>
std::string log_format(const std::string& level, const Args&... args) {
    std::ostringstream os;
    os << "[" << level << "] ";
    ((os << args << ' '), ...);
    return os.str();
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) sum
    std::cout << "sum(1,2,3,4)         = " << sum(1, 2, 3, 4) << "\n";
    std::cout << "sum(1.5, 2, 3)       = " << sum(1.5, 2, 3) << "\n";

    // (2) sum0 允許空 pack
    std::cout << "sum0()               = " << sum0() << "\n";    // 0
    std::cout << "sum0(10, 20)         = " << sum0(10, 20) << "\n";

    // (3) print_all
    print_all("hi", 42, 3.14, std::string("OK"));

    // (4) all_positive
    std::cout << std::boolalpha;
    std::cout << "all_positive(1,2,3)   = " << all_positive(1, 2, 3) << "\n";
    std::cout << "all_positive(1,-1,3)  = " << all_positive(1, -1, 3) << "\n";

    // (5) push_all
    std::vector<int> v;
    push_all(v, 1, 2, 3, 4, 5);
    std::cout << "v = ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";

    // (6) Running sum + sum_args
    std::cout << "sum_args(1..5)         = " << sum_args(1, 2, 3, 4, 5) << "\n";
    auto rs = running_sum(std::vector<int>{1, 2, 3, 4});
    std::cout << "running_sum: ";
    for (int x : rs) std::cout << x << ' ';
    std::cout << "\n";

    // (7) min_of / max_of
    std::cout << "min_of(7, 3, 9, 4)   = " << min_of(7, 3, 9, 4) << "\n";
    std::cout << "max_of(7, 3, 9, 4)   = " << max_of(7, 3, 9, 4) << "\n";

    // (8) all/any/none
    std::cout << "all_of(1,1,1)        = " << all_of(true, true, true)  << "\n";
    std::cout << "any_of(0,0,1)        = " << any_of(false,false,true)  << "\n";
    std::cout << "none_of(0,0,0)       = " << none_of(false,false,false)<< "\n";

    // (9) Leetcode 1486 + xor_all
    std::cout << "xor_operation(5, 0)  = " << xor_operation(5, 0) << " (expect 8)\n";
    std::cout << "xor_all(1,2,3,4,5)   = " << xor_all(1, 2, 3, 4, 5) << "\n";

    // (10) log_format
    std::cout << log_format("INFO", "user", "alice", "score", 95) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：unary fold 在「空 parameter pack」時是否合法？
    //    A：只有三個運算子有預設 identity element 可用：&&（true）、||（false）、
    //       逗號（void()）。其他像 +、*、- 等運算子若 pack 為空會 ill-formed。
    //       所以求和、求積建議用 binary fold（如 (0 + ... + args)）較安全。
    //
    //  Q2：left fold 與 right fold 結果會不同嗎？
    //    A：對結合律成立的運算（+、*、&&、||）結果相同；但對非結合運算
    //       （-、/、<<）順序會影響結果。例如 (... - args) 是 ((a-b)-c)，
    //       (args - ...) 則是 (a-(b-c))。實作前要分清楚。
    //
    //  Q3：要在 pack 上做副作用（列印、push_back）為什麼常用「逗號 fold」？
    //    A：因為逗號運算子允許任何 void 表達式成為其運算元，且空 pack 也合法
    //       （展開為 void()）。寫法 ((std::cout << args << ' '), ...) 可讓編譯器
    //       對每個 arg 依序展開呼叫，等同 C++17 之前要遞迴展開的麻煩寫法。
    //
    return 0;
}

// ============================================================================
//  【小結 & 經驗法則】
//    1. 大部分情況用 unary left fold (... op args) 最自然。
//    2. 想處理「空 pack」改用 binary 形式 (init op ... op args)。
//    3. 想做副作用 (列印、push)：用 「逗號 fold」: ((expr), ...)。
//    4. fold 的可讀性遠勝遞迴展開，但讀者必須懂 fold 才看得懂；建議放上
//       簡短註解說明它在做什麼。
//
//  【下一篇】
//    15_perfect_forwarding.cpp ── 完美轉發：T&&、std::forward、reference collapsing。
// ============================================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 14_fold_expressions.cpp -o 14_fold_expressions

// === 預期輸出 ===
// sum(1,2,3,4)         = 10
// sum(1.5, 2, 3)       = 6.5
// sum0()               = 0
// sum0(10, 20)         = 30
// hi 42 3.14 OK
// all_positive(1,2,3)   = true
// all_positive(1,-1,3)  = false
// v = 1 2 3 4 5
// sum_args(1..5)         = 15
// running_sum: 1 3 6 10
// min_of(7, 3, 9, 4)   = 3
// max_of(7, 3, 9, 4)   = 9
// all_of(1,1,1)        = true
// any_of(0,0,1)        = true
// none_of(0,0,0)       = true
// xor_operation(5, 0)  = 8 (expect 8)
// xor_all(1,2,3,4,5)   = 1
// [INFO] user alice score 95
