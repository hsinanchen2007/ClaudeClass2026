// ============================================================================
//  19_if_constexpr.cpp  ──  if constexpr (C++17)
// ============================================================================
//
//  【本篇目標】
//    if constexpr 是 C++17 的大殺器。它讓你寫「編譯期分支」── 不被選中
//    的分支「整段不會被 instantiate」，於是可以放原本不能放的程式碼 (例
//    如某分支會用到的成員，另一型別根本沒有)。
//
//  【為什麼一般 if 不行】
//    考慮：
//        template<typename T>
//        void f(T x) {
//            if (std::is_pointer_v<T>) std::cout << *x;     // 不是指標就糟了
//            else                       std::cout << x;
//        }
//    傳 int 進去：if (false) 是 runtime 不執行，但 *x 還是要編譯。
//    int 不能 deref → 編譯錯誤！
//
//  【if constexpr 的差別】
//    把 if 改成 if constexpr：
//        template<typename T>
//        void f(T x) {
//            if constexpr (std::is_pointer_v<T>) std::cout << *x;
//            else                                 std::cout << x;
//        }
//    這時候編譯器在 instantiate 時：
//      - T = int*：只 instantiate 「*x」分支，「else」整段直接丟掉。
//      - T = int ：只 instantiate 「else」分支，「if」整段丟掉。
//    於是兩邊各自只看到合法的程式碼，編譯通過。
//
//  【限制】
//    - 條件必須是「編譯期常數 bool」(constexpr 或 const expression)。
//    - 必須在 template 函式內 (才能依型別走不同分支)。
//    - 不能取代 SFINAE：例如「函式存不存在」這種事 if constexpr 救不了
//      (因為函式宣告本身就要能編譯)。
//
//  【用途】
//    1. 取代 tag dispatch / SFINAE：少寫一堆 overload。
//    2. 編譯期遞迴展開：基底條件用 if constexpr 中斷遞迴。
//    3. 寫「對所有型別都通用」的工具函式，內部依 trait 分流。
//
//  參考：https://en.cppreference.com/cpp/language/if#Constexpr_If
// ============================================================================

/*
補充筆記：if_constexpr
  - if_constexpr 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - if_constexpr 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】if constexpr
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. if constexpr 屬哪個標準？跟普通 if 的關鍵差別在哪？
//     答：C++17，常被誤答成 C++14（本機以 -std=c++14 -pedantic-errors 實測，
//         GCC 明確拒絕：'if constexpr' only available with -std=c++17）。
//         差別：if constexpr 在編譯期求值，未選中的分支在模板實例化時被「丟棄」
//         ——只需語法合法，語意可以不合法（例如對 int 呼叫 .size()）。普通 if
//         兩個分支都必須能編譯，所以寫不出這種泛型分流。
//     追問：條件可以是 runtime 變數嗎？（不行，必須是可轉成 bool 的 constant
//         expression，常見是 trait::value、sizeof(T)、constexpr 變數）
//
// 🔥 Q2. if constexpr 能取代所有 SFINAE 嗎？
//     答：不能。它處理的是「進到函式以後要走哪條路」；SFINAE 與 concepts 處理的是
//         「這個函式該不該進入 overload set」。if constexpr 不影響重載決議與偏序，
//         也沒辦法讓一個函式「消失」，更不能改變函式簽章。要做 overload 層級的
//         選擇，還是得用 concepts / SFINAE / tag dispatch。
//
// ⚠️ 陷阱. 「if constexpr 條件是 false，那一整段就不會被檢查」——任何地方都成立嗎？
//     答：不成立，只有在「模板」裡才成立。本機實測：非模板函式中寫
//         if constexpr (false) { x.size(); } 而 x 是 int，照樣編譯錯誤
//         （error: request for member 'size' in 'x', which is of non-class type 'int'）；
//         同一段程式碼搬進 template 就順利通過。
//     為什麼會錯：把 if constexpr 記成「條件 false 就整段跳過不看」。編譯器不是
//         不看，而是「不實例化」——沒有模板就沒有實例化這回事，該做的語意檢查
//         一項都不會少。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

// ─── 1. 與一般 if 的對比：通用 print ────────────────────────────────────
template <typename T>
void smart_print(const T& v) {
    if constexpr (std::is_pointer_v<T>) {
        if (v) std::cout << "*ptr=" << *v << "\n";
        else   std::cout << "null\n";
    } else if constexpr (std::is_arithmetic_v<T>) {
        std::cout << "[number] " << v << "\n";
    } else {
        std::cout << "[other] " << v << "\n";
    }
}

// ─── 2. 編譯期遞迴：印 tuple 所有元素 ────────────────────────────────────
//   tuple 的「逐個元素操作」是 if constexpr 最常見的應用。
//   sizeof...(T) 拿型別數；用 std::tuple_size_v / std::get<I>。
#include <tuple>

template <std::size_t I = 0, typename... Ts>
void print_tuple(const std::tuple<Ts...>& t) {
    if constexpr (I < sizeof...(Ts)) {
        if constexpr (I > 0) std::cout << ", ";
        std::cout << std::get<I>(t);
        print_tuple<I + 1>(t);
    }
}

// ─── 3. 通用 to_string：依型別走不同路徑 ────────────────────────────────
//   實務常見場景：debug log 工具想把任意型別轉字串。
template <typename T>
std::string to_str(const T& v) {
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(v);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return v;
    } else if constexpr (std::is_same_v<T, const char*>) {
        return std::string(v);
    } else {
        return "[unprintable]";
    }
}

// ─── 4. Leetcode 9 ── Palindrome Number (用 if constexpr 對 int / string 共用) ──
//   題目：判斷整數 x 是否為回文。負數一律不是。
//   範例：121 → true；-121 → false；10 → false
//
//   經典解法 (整數版)：反轉一半位數，比較兩半。
//        若 x < 0 或 (x % 10 == 0 且 x != 0) 直接 false。
//
//   為什麼放這裡？
//     做成 template，依「型別是 integer 還是 string」走不同邏輯：
//        - integer：用反轉一半位數的數學技巧
//        - string ：兩端往中間夾擊
//     使用者一個 is_palindrome(...) 就能同時處理兩種情境。
//
//   時間：O(log10 x)（整數版）；O(n)（字串版）
//   空間：O(1)
template <typename T>
bool is_palindrome(const T& v) {
    if constexpr (std::is_integral_v<T>) {
        if (v < 0) return false;
        if (v != 0 && v % 10 == 0) return false;
        T x = v;
        T rev = 0;
        while (x > rev) {                  // 反轉一半就夠
            rev = rev * 10 + x % 10;
            x /= 10;
        }
        return x == rev || x == rev / 10;
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::size_t i = 0, j = v.size();
        while (i + 1 < j) {
            --j;
            if (v[i] != v[j]) return false;
            ++i;
        }
        return true;
    } else {
        static_assert(std::is_integral_v<T> || std::is_same_v<T, std::string>,
                      "is_palindrome 只支援整數或 std::string");
        return false; // unreachable，避免 warning
    }
}

// ─── 5. 工作實用：通用 printer 不同型別不同格式 ─────────────────────────
template <typename T>
void log_value(const std::string& name, const T& v) {
    std::cout << name << " = ";
    if constexpr (std::is_floating_point_v<T>) {
        std::printf("%.4f\n", static_cast<double>(v));    // 浮點固定 4 位
    } else if constexpr (std::is_same_v<T, bool>) {
        std::cout << (v ? "true" : "false") << "\n";
    } else {
        std::cout << v << "\n";
    }
}

// ─── 6. Leetcode 412 ── Fizz Buzz (template + if constexpr 切換輸出型別) ─
//   難度: easy
//   題目：給 n，回傳長度 n 的字串陣列，符合：
//     - 3 的倍數 → "Fizz"；5 的倍數 → "Buzz"；15 倍數 → "FizzBuzz"；
//     - 其他 → 該數字本身的字串。
//
//   為什麼放在這裡？
//     用 template + if constexpr：根據呼叫端「想要 string 還是 int 標記」
//     切換邏輯。Tag = 0/3/5/15。
//
//   時間：O(n)；空間：O(n)。
enum class FizzMark : int { Number = 0, Fizz = 3, Buzz = 5, FizzBuzz = 15 };

template <typename Out>
std::vector<Out> fizz_buzz(int n) {
    std::vector<Out> result;
    result.reserve(n);
    for (int i = 1; i <= n; ++i) {
        if constexpr (std::is_same_v<Out, std::string>) {
            if      (i % 15 == 0) result.push_back("FizzBuzz");
            else if (i % 3  == 0) result.push_back("Fizz");
            else if (i % 5  == 0) result.push_back("Buzz");
            else                  result.push_back(std::to_string(i));
        } else {
            // 數字版：用 FizzMark 標記分類
            FizzMark m = FizzMark::Number;
            if      (i % 15 == 0) m = FizzMark::FizzBuzz;
            else if (i % 3  == 0) m = FizzMark::Fizz;
            else if (i % 5  == 0) m = FizzMark::Buzz;
            result.push_back(static_cast<Out>(m));
        }
    }
    return result;
}

// ─── 7. 工作實用：generic byte size — 容器算總 byte 數、scalar 算 sizeof ─
//   依「是不是容器」走兩條路。
template <typename T>
std::size_t byte_size(const T& v) {
    if constexpr (std::is_arithmetic_v<T>) {
        return sizeof(v);
    } else {
        // 假設有 size() 與 value_type，這對 vector / string 都成立
        return v.size() * sizeof(typename T::value_type);
    }
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) smart_print
    int x = 7;
    smart_print(x);
    smart_print(&x);
    smart_print(std::string("hello"));

    // (2) tuple
    auto t = std::make_tuple(1, 3.14, std::string("hi"), 'A');
    std::cout << "tuple = ("; print_tuple(t); std::cout << ")\n";

    // (3) to_str
    std::cout << "to_str(42)        = " << to_str(42) << "\n";
    std::cout << "to_str(3.14)      = " << to_str(3.14) << "\n";
    std::cout << "to_str(string)    = " << to_str(std::string("hi")) << "\n";
    std::cout << "to_str(\"raw\")    = " << to_str("raw") << "\n";

    // (4) Leetcode 9
    std::cout << std::boolalpha;
    std::cout << "is_palindrome(121)              = " << is_palindrome(121)              << "\n";
    std::cout << "is_palindrome(-121)             = " << is_palindrome(-121)             << "\n";
    std::cout << "is_palindrome(10)               = " << is_palindrome(10)               << "\n";
    std::cout << "is_palindrome(string \"abcba\") = " << is_palindrome(std::string("abcba")) << "\n";
    std::cout << "is_palindrome(string \"abc\")   = " << is_palindrome(std::string("abc")) << "\n";

    // (5) log_value
    log_value("temp",   36.6);
    log_value("flag",   true);
    log_value("count",  100);
    log_value("name",   std::string("alice"));

    // (6) Leetcode 412 FizzBuzz
    auto fz_str = fizz_buzz<std::string>(15);
    std::cout << "fizz_buzz<string>(15): ";
    for (const auto& s : fz_str) std::cout << s << ' ';
    std::cout << "\n";

    // (7) byte_size
    std::cout << "byte_size(int)          = " << byte_size(42) << "\n";
    std::cout << "byte_size(\"hello\")      = " << byte_size(std::string("hello")) << "\n";
    std::cout << "byte_size(vector<int>×5)= " << byte_size(std::vector<int>(5)) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：if constexpr 跟普通 if 在 template 裡差別關鍵在哪？
    //    A：普通 if 兩個分支都會被編譯，未走到的分支若型別不合法（例如對 int 呼叫
    //       .size()）就會編譯失敗。if constexpr 在 template instantiation 時會
    //       「丟棄」不成立的分支，未取分支裡用到的成員可以不存在。
    //
    //  Q2：if constexpr 條件需要是什麼？可以放 runtime 變數嗎？
    //    A：條件必須是「constant expression converted to bool」，常見有 trait::value、
    //       sizeof(T)、constexpr 變數等。runtime 的條件不能用，要寫成普通 if。
    //       此外丟棄分支只發生在 template；非 template 函式裡 if constexpr 兩邊
    //       都還是要型別合法。
    //
    //  Q3：if constexpr 能完全取代 SFINAE / tag dispatch 嗎？
    //    A：函式內部的「型別分支」幾乎可以；但 if constexpr 不能把候選函式移出
    //       overload set，也不能影響回傳型別 deduction。要做 overload-level 的
    //       選擇仍需 concepts/SFINAE/tag dispatch。
    //
    return 0;
}

// ============================================================================
//  【小結】
//    1. if constexpr 在編譯期分支，未選中的分支整段不被 instantiate。
//    2. 大幅簡化以前用 SFINAE / tag dispatch 的場景。
//    3. 條件必須是編譯期常數；通常配 type_traits 用。
//    4. 適合：依型別屬性走不同實作 / tuple 遞迴 / 編譯期常數計算。
//
//  【下一篇】
//    20_concepts_intro.cpp ── C++20 Concepts，metaprogramming 的未來。
// ============================================================================
