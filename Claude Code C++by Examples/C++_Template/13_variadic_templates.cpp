// ============================================================================
//  13_variadic_templates.cpp  ──  Variadic Template (可變參數模板)
// ============================================================================
//
//  【本篇目標】
//    Variadic template = 可以接收「任意個數、任意型別」參數的 template。
//    C++11 引入。是現代 C++ 中最強大的工具之一 ── std::tuple、std::make_unique、
//    std::function、emplace_back 等都靠它實作。
//
//  【語法】
//
//        template <typename... Ts>          // Ts 是 「parameter pack」
//        void f(Ts... args) {                // args 是 function parameter pack
//            // 在函式內展開 args...
//        }
//
//        f();                  // Ts 為空
//        f(1);                 // Ts = (int)
//        f(1, 2.0, "hi");      // Ts = (int, double, const char*)
//
//  【關鍵術語】
//    - Template parameter pack：typename... Ts
//    - Function parameter pack：Ts... args
//    - Pack expansion：在能展開的位置寫 ...，pack 內每個元素被代入。
//        e.g. f(args...) → f(args1, args2, args3, ...)
//
//  【展開的位置】
//    Pack 展開不能憑空寫 ...，要寫在「展開上下文」裡，例如：
//      - 函式呼叫實參：foo(args...)
//      - initializer list：{args...}
//      - 基底類別清單：class Derived : public Bases... {};
//      - sizeof... ：sizeof...(args) 取得元素數量
//      - C++17 fold expression：(... + args)，下一篇 (file 14) 詳述
//
//  【傳統的展開：遞迴】
//    C++11 / C++14 沒有 fold expression，要用「遞迴 + 終止 overload」：
//
//        void print() {}                                    // 終止
//        template<typename T, typename... Rest>
//        void print(T head, Rest... rest) {                 // 遞迴
//            std::cout << head << ' ';
//            print(rest...);
//        }
//
//    每次展開一個 head，把剩下的 pack 繼續傳下去。
//    C++17 起可以改用 fold expression 一行解決，但「遞迴模式」依然是經
//    典必懂的技巧。
//
//  【sizeof...(pack)】
//    取得 pack 中元素的數量。沒有 runtime cost，是編譯期常數。
//
//        template<typename... Ts>
//        constexpr std::size_t count = sizeof...(Ts);
//
//  參考：https://en.cppreference.com/cpp/language/parameter_pack
// ============================================================================

/*
補充筆記：variadic_templates
  - variadic template 用 parameter pack 表示任意數量模板參數。
  - pack expansion 的位置決定展開結果，例如函式參數、初始化列表或繼承列表。
  - 遞迴展開在 C++17 後常可用 fold expression 簡化。
  - variadic_templates 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

// ─── 1. 終止版 + 遞迴版：通用 print ──────────────────────────────────────
//   先寫 0 參數版本當遞迴出口。
void print_all() { std::cout << "\n"; }

//   再寫遞迴版：把 head 印出來，剩下 pack 再傳給自己。
template <typename T, typename... Rest>
void print_all(const T& head, const Rest&... rest) {
    std::cout << head;
    if (sizeof...(rest) > 0) std::cout << ", ";
    print_all(rest...);
}

// ─── 2. sizeof... 範例：count_args ──────────────────────────────────────
template <typename... Args>
constexpr std::size_t count_args(Args&&...) {
    return sizeof...(Args);
}

// ─── 3. 工作實用：type-safe logger ──────────────────────────────────────
//   類比 printf 但完全型別安全 (printf 不檢查型別，是經典 C 安全坑)。
//   不必先把參數轉字串：cout 自己會處理 (operator<<)。
template <typename... Args>
void log(const std::string& level, const Args&... args) {
    std::cout << "[" << level << "] ";
    print_all(args...);
}

// ─── 4. Leetcode 1929 ── Concatenation of Array (用 variadic 擴展概念) ───
//   題目：給一個長度 n 的陣列 nums，回傳長度 2n 的 ans，其中 ans = nums + nums。
//   範例：[1,2,1] → [1,2,1,1,2,1]
//
//   為什麼用 variadic？
//     原題只 concat 兩個。我們做一個 type-safe variadic concat 版本：
//        concat(v1, v2, v3, ...) → 回傳一個全部接起來的 vector。
//     工作上常用：把多個資料源合併成一份輸入。
//
//   時間：O(n)，n = 全部元素總和
//   空間：O(n)
//
//   實作要點：
//     用 fold-by-recursion 寫法 (本篇主軸)；下一篇 (file 14) 改寫成 fold expression 對比。
template <typename T>
void concat_into(std::vector<T>& dst) { (void)dst; /* 終止 */ }

template <typename T, typename First, typename... Rest>
void concat_into(std::vector<T>& dst, const First& first, const Rest&... rest) {
    dst.insert(dst.end(), first.begin(), first.end());
    concat_into(dst, rest...);
}

template <typename T, typename... Vecs>
std::vector<T> concat(const Vecs&... vecs) {
    std::vector<T> out;
    out.reserve((vecs.size() + ...));     // C++17 fold (這裡偷用一下，下一篇詳講)
    concat_into(out, vecs...);
    return out;
}

// 解 LC 1929 的「自身串接」版本：
std::vector<int> get_concatenation(const std::vector<int>& nums) {
    return concat<int>(nums, nums);
}

// ─── 5. 經典應用：make_unique 自製版 (perfect forwarding 預告) ────────────
//   這個範例用到 perfect forwarding (file 15)，這裡先看 variadic 部分。
//   把所有引數 forward 給 T 的 constructor。
template <typename T, typename... Args>
std::unique_ptr<T> make_unique_t(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// ─── 6. Leetcode 26 ── Remove Duplicates (variadic concat 預處理多筆輸入) ─
//   難度: easy
//   題目：給排序好的 vector，原地移除重複元素，回傳新長度。
//
//   為什麼放在這裡？
//     工作上常先把「多個排序好的小段」concat 起來再去重。我們用 variadic
//     concat 接受任意數量輸入，然後做 dedup_sorted。
//
//   時間：O(n)；空間：O(1) (in-place)。
template <typename T>
std::size_t dedup_sorted(std::vector<T>& v) {
    if (v.empty()) return 0;
    std::size_t k = 0;
    for (std::size_t j = 1; j < v.size(); ++j) {
        if (v[j] != v[k]) { ++k; v[k] = v[j]; }
    }
    return k + 1;
}

// ─── 7. 工作實用：variadic min/max ───────────────────────────────────────
//   類似 std::min({a,b,c,...}) 但更靈活：可接受不同型別 (依 + 提升)。
template <typename T>
constexpr T min_var(T x) { return x; }     // 終止 overload (1 個參數時)

template <typename T, typename... Rest>
constexpr auto min_var(T head, Rest... rest) {
    auto m = min_var(rest...);
    return head < m ? head : m;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) print_all
    print_all();                                  // 0 參數
    print_all(1);                                 // 1 參數
    print_all(1, 2.5, std::string("hello"));      // 3 參數

    // (2) sizeof...
    std::cout << "count_args(1, 'a', 3.14) = "
              << count_args(1, 'a', 3.14) << "\n";

    // (3) log
    log("INFO",  "user login:", "alice", 42);
    log("ERROR", "code:", -1, "msg:", "disk full");

    // (4) Leetcode 1929 + 通用 concat
    auto a = get_concatenation({1, 2, 1});
    std::cout << "1929 result: ";
    for (int x : a) std::cout << x << ' ';
    std::cout << "\n";

    auto b = concat<int>(std::vector<int>{1,2}, std::vector<int>{3,4,5},
                         std::vector<int>{6});
    std::cout << "concat 3 vectors: ";
    for (int x : b) std::cout << x << ' ';
    std::cout << "\n";

    // (5) make_unique_t
    struct Point { int x; double y; Point(int xx, double yy) : x(xx), y(yy) {} };
    auto p = make_unique_t<Point>(3, 4.5);
    std::cout << "Point(" << p->x << ", " << p->y << ")\n";

    // (6) Leetcode 26 + variadic concat 預處理
    auto merged = concat<int>(std::vector<int>{1, 1, 2}, std::vector<int>{2, 3, 4});
    auto len = dedup_sorted(merged);
    std::cout << "dedup_sorted len = " << len << ", first " << len << ": ";
    for (std::size_t i = 0; i < len; ++i) std::cout << merged[i] << ' ';
    std::cout << "\n";

    // (7) variadic min_var
    std::cout << "min_var(5,3,7,1,9,4) = " << min_var(5, 3, 7, 1, 9, 4) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：parameter pack 跟 C 的 va_args (printf 那種) 差別在哪？
    //    A：va_args 是 runtime 機制，不檢查型別 (printf("%d", "hello")
    //       是 C 經典安全坑)、需要 va_start/va_end 巨集、要靠格式字串
    //       告訴它型別。parameter pack 是編譯期機制：型別完全保留並逐
    //       個展開、沒有 runtime cost、可以 perfect forward (std::forward
    //       搭配 Args&&...)、編譯器幫你做型別檢查。現代 C++ 的 std::format、
    //       emplace_back、make_unique 都是 variadic template 寫的。
    //
    //  Q2：`sizeof...(args)` 與一般的 `sizeof(args)` 是同一個東西嗎？
    //    A：不是。`sizeof(x)` 取單一 expression 的位元組大小 (runtime
    //       還沒到，但要看型別)；`sizeof...(pack)` 是專屬 parameter pack
    //       的運算子，回傳「pack 裡有幾個元素」，是 std::size_t 的
    //       constexpr。例如 `template<typename...Ts> void f(Ts...args)`
    //       裡 sizeof...(Ts) 與 sizeof...(args) 都會等於引數個數，可
    //       用在 static_assert、enable_if、編譯期 branch 上。
    //
    //  Q3：傳統「終止 + 遞迴」展開模式 vs C++17 fold expression，性能
    //      或編譯期成本差很多嗎？
    //    A：兩者最終生成的機器碼差不多 (現代編譯器都會 inline 掉遞迴)，
    //       但編譯期成本不同：遞迴版每次展開都要實例化一個新 overload
    //       (n 個元素 → n+1 個 instantiation)，pack 大時編譯時間明顯。
    //       fold expression 一次展開、編譯器處理為單一表達式，編譯快很
    //       多。可讀性上 fold expression 也更直觀，C++17 之後優先選用，
    //       遞迴模式仍是經典面試題與某些需要 head/rest 拆分情境的必備。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. variadic template 用 typename... Ts 宣告，使用時用 args... 展開。
//    2. C++11/14 必須遞迴展開：終止 overload + 遞迴 overload。
//    3. sizeof...(pack) 取大小，是 constexpr。
//    4. 應用無限：std::tuple、std::variant、emplace、log、forward 等。
//
//  【下一篇】
//    14_fold_expressions.cpp ── C++17 fold expression，把上面遞迴展開
//    寫成一行的神器。
// ============================================================================
