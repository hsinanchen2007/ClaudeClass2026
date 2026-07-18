// ============================================================================
//  25_template_compilation_model.cpp  ──  Template 編譯模型與常見錯誤
// ============================================================================
//
//  【本篇定位】
//    這篇「沒有 Leetcode」，純粹是「該寫什麼放 header / 什麼放 .cpp」、
//    「為什麼某些錯誤訊息又長又怪」這類「養成期最容易踩的雷」的整理。
//    把這篇讀完，你就不會再被 「undefined reference to ...」 打敗。
//
// ----------------------------------------------------------------------------
//  【核心問題：Template 為何不能像普通函式拆 header / .cpp？】
//
//    普通函式：
//        // foo.h
//        int foo(int);
//        // foo.cpp
//        int foo(int x) { return x * 2; }
//
//    其他 .cpp 只要 #include "foo.h"，編譯時看到宣告即可，link 時找實作。
//
//    Template 函式：
//        // foo.h
//        template<typename T> T foo(T);
//        // foo.cpp
//        template<typename T> T foo(T x) { return x * 2; }
//
//    別的 .cpp 寫 `foo(3)` 時，編譯器需要「實際看到 foo 的 body」才能用
//    T = int 實例化 ── 但 body 在另一個 .cpp，當前翻譯單元看不見!
//    結果：link 時報「undefined reference to foo<int>(int)」。
//
//    解法 1 (主流)：把整個 template 定義放 header (.h / .hpp)。
//    解法 2：在 .cpp 末尾用「explicit instantiation」事先把要用到的型別實
//            例化好，使用者就 link 得到。下面有範例。
//
// ----------------------------------------------------------------------------
//  【ODR (One Definition Rule)】
//    一個非 inline 的非 template 函式，只能在整個程式有「一個定義」。
//    template 是「ODR-免疫」的 ── 多個翻譯單元各自實例化都不會違反 ODR
//    (這是語言為了 template 設計的特殊規則)。
//
//    但是：如果在 header 中寫「非 template inline 函式」(沒加 inline)，
//    多個翻譯單元 include 就會違反 ODR。所以：
//      - template 函式：直接定義在 header 即可 (本身免 ODR)。
//      - 非 template 函式：要在 header 裡定義就加 `inline`。
//
// ----------------------------------------------------------------------------
//  【Explicit Instantiation】
//
//    把熱門 template 在 .cpp 裡「先實例化好」，使用者只 link 不重編，可
//    以加快大型專案的 build：
//
//        // matrix.cpp
//        template class Matrix<int, 3, 3>;       // 顯式實例化整個類別
//        template double dot<double>(...);        // 顯式實例化函式
//
//    對應的 header 要用 `extern template`：
//
//        // matrix.h
//        extern template class Matrix<int, 3, 3>;     // 「我承諾 .cpp 會幫你實例化」
//        extern template double dot<double>(...);
//
//    這樣使用者的翻譯單元不會重複生出 Matrix<int,3,3> 的副本，編譯時間下降。
//
// ----------------------------------------------------------------------------
//  【常見錯誤訊息與對策】
//
//    ① "undefined reference to template<...>::xxx"
//       → 你把 template 定義放在 .cpp 卻沒 explicit instantiation。
//       → 解法：把定義搬到 header；或加 explicit instantiation。
//
//    ② "expected ';' before 'X'"，X 看起來是個型別。
//       → 大概是忘了加 `typename` 在 dependent type 前面 (file 08)。
//
//    ③ "no matching function for call to ..."
//       → SFINAE 把所有 candidate 都剔除了。檢查型別 trait 條件。
//
//    ④ 巨型錯誤訊息塞滿螢幕
//       → STL 容器 / 迭代器類型互動的錯誤是這樣。建議：
//         (a) 改用 concept (C++20)，錯誤訊息會清爽很多。
//         (b) 用 static_assert + always_false_v<T> 自己加守門員。
//
//  【Two-phase name lookup】
//    Template 編譯有兩階段：
//      ① 看到 template 定義時做「非 dependent name」的 lookup。
//      ② 實例化時對「dependent name」做第二次 lookup。
//
//    某些舊編譯器 (尤其 MSVC) 對第一階段不嚴格 → 你寫了不依賴 T 的非法名
//    字它不抓，搬到 GCC/Clang 就爆。建議寫法保持嚴謹，能在 GCC/Clang
//    通過為佳。
//
//  參考：
//    https://en.cppreference.com/cpp/language/function_template
//    https://en.cppreference.com/cpp/language/class_template
//    https://en.cppreference.com/cpp/language/explicit_instantiation
// ============================================================================


/*
補充筆記：template_compilation_model
  - template 不是已經編譯好的單一函式，而是讓編譯器在使用點依具體型別產生程式碼的樣板。
  - 呼叫 foo(3) 時，編譯器需要看到 foo<T> 的完整定義才能實例化 foo<int>；只看到宣告通常不夠。
  - 因此 template 定義多半放在 header，這不是風格偏好，而是 C++ 編譯模型要求使用點可見。
  - 若把 template 定義放在 .cpp，必須用 explicit instantiation 明確列出要產生的型別版本，否則其他 translation unit link 時會 undefined reference。
  - extern template 可告訴其他 translation unit 不要重複實例化某個版本，常用於大型專案降低編譯時間和二進位膨脹。
  - template 錯誤常在實例化時才爆出很長訊息；讀錯誤要先找第一個真正不符合型別需求的表達式。
  - dependent name 需要 typename/template 關鍵字時要明確寫出，因為第一階段 name lookup 無法知道它到底是型別還是值。
  - header-only template 要注意 ODR：template 本身可在多個 translation unit 實例化，但非 template helper 若放 header 通常要 inline 或放到 detail/template 中。
*/

#include <iostream>
#include <type_traits>
#include <vector>

// ─── 1. 示範：把 template 整個定義在 header-style (本檔內全部) ────────────
//   這個跟 file 02 的 max_t 一樣，本檔示意最常見且推薦的寫法 ── 整個放
//   header 內。
template <typename T>
T add_t(T a, T b) { return a + b; }

// ─── 2. 示範 explicit instantiation 的寫法 (語法演示，本檔自包) ──────────
//   想像你有個熱門 template：matrix multiply。在 header 宣告，定義在 .cpp
//   末端做 explicit instantiation，並在 header 用 extern template 公告。
template <typename T>
class HotMath {
public:
    static T square(T x) { return x * x; }
};

// 「假裝」我們在 header：宣告 extern template (告訴使用者：別自己實例化 int 版)
extern template class HotMath<int>;
// 「假裝」我們在 .cpp：定義 explicit instantiation
template class HotMath<int>;
// 注意：實務上這兩個會分散在 header 與 cpp，這裡為示範放一起。

// ─── 3. always_false：讓 static_assert 在 instantiation 時才觸發 ─────────
//   錯誤訊息工程：當使用者用了不支援的型別，給出友善訊息。
template <typename...>
inline constexpr bool always_false_v = false;

template <typename T>
void only_arithmetic(T x) {
    if constexpr (std::is_arithmetic_v<T>) {
        std::cout << "OK arithmetic: " << x << "\n";
    } else {
        static_assert(always_false_v<T>,
                      "only_arithmetic: 需要算術型別 (整數或浮點)");
    }
}

// ─── 4. 兩階段 lookup demo：dependent name 必須加 typename ───────────────
template <typename Container>
typename Container::value_type sum_all(const Container& c) {
    typename Container::value_type s{};        // 第二階段才解析
    for (const auto& x : c) s += x;
    return s;
}

// ─── 5. 工作實用：「inline 變數」省去 ODR 麻煩 (C++17) ────────────────────
//   header 裡有全域常數想被多個 cpp include。用 inline 可避免 ODR 違反。
inline constexpr int kMaxRetries = 5;

// ─── 6. Leetcode 1342 ── Number of Steps to Reduce a Number to Zero ─────
//   難度: easy
//   題目：給整數 num，每步操作：偶數 → 除 2、奇數 → 減 1。回傳到 0 的步數。
//   範例：num=14 → 14→7→6→3→2→1→0 → 6 步
//
//   為什麼放在這裡？
//     寫成 template + always_false_v 守門：只接受整數型別，否則編譯期就吐
//     出友善訊息，演示「instantiation-time error」工程慣例。
template <typename T>
int number_of_steps(T num) {
    if constexpr (std::is_integral_v<T>) {
        int steps = 0;
        while (num != T(0)) {
            if (num % T(2) == T(0)) num /= T(2);
            else                    num -= T(1);
            ++steps;
        }
        return steps;
    } else {
        static_assert(always_false_v<T>, "number_of_steps 只支援整數型別");
        return 0;
    }
}

// ─── 7. 工作實用：「explicit instantiation」常用情境演示 ─────────────────
//   想像 BigSerializer<T> 在 production 只用 int / double 兩種。為加快編譯，
//   header 用 extern template，cpp 做 explicit instantiation：
template <typename T>
class BigSerializer {
public:
    std::string to_string(const T& v) const { return std::to_string(v); }
};

// 「假裝」cpp 端的 explicit instantiation：
template class BigSerializer<int>;
template class BigSerializer<double>;
// 對應 header 端可用 extern template class BigSerializer<int>;
// 來避免每個翻譯單元都實例化一份，大型專案常見。

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    std::cout << "add_t(3, 4)        = " << add_t(3, 4) << "\n";
    std::cout << "HotMath<int>::sq(5)= " << HotMath<int>::square(5) << "\n";
    only_arithmetic(42);
    only_arithmetic(3.14);
    // only_arithmetic(std::string("hi"));   // 取消註解 → 觸發 static_assert，錯誤訊息友善

    std::cout << "sum_all({1,2,3})   = " << sum_all(std::vector<int>{1, 2, 3}) << "\n";
    std::cout << "kMaxRetries        = " << kMaxRetries << "\n";

    // (6) Leetcode 1342
    std::cout << "number_of_steps(14)= " << number_of_steps(14) << " (expect 6)\n";
    std::cout << "number_of_steps(8) = " << number_of_steps(8)  << " (expect 4)\n";

    // (7) BigSerializer (explicit instantiated 為 int / double)
    BigSerializer<int>    si;
    BigSerializer<double> sd;
    std::cout << "BigSerializer<int>(42)    = " << si.to_string(42) << "\n";
    std::cout << "BigSerializer<double>(3.14)= " << sd.to_string(3.14) << "\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼 template 通常要寫在 header（.h / .ipp）裡？
    //    A：template 不是程式碼，是「程式碼的食譜」。每個 TU 必須在自己看到使用
    //       點時 instantiate 一份；如果定義只放在 .cpp，其他 TU 看不到實作就會
    //       linker error。例外是 explicit instantiation：把所有要用的型別在
    //       一個 .cpp 一次 instantiate 完畢，header 只放宣告。
    //
    //  Q2：什麼是「兩階段查名（two-phase name lookup）」？
    //    A：階段一在「定義 template 時」就查 non-dependent name；階段二要等到
    //       instantiate 時才查 dependent name（依賴 template 參數的）。為了讓
    //       parser 能在階段一就知道 dependent name 是 type 還是 value，需要
    //       明確寫 typename / template 關鍵字。
    //
    //  Q3：ODR（One Definition Rule）對 template 有什麼影響？
    //    A：template 的「同一個 instantiation」可在多個 TU 出現，但每份必須完全
    //       相同。inline / constexpr / template 函式預設都是 inline，linker 會
    //       去重。宣告為 inline 變數（C++17）也可避免 header 全域常數的 ODR
    //       違反問題。
    //
    return 0;
}

// ============================================================================
//  【小結 ── 編譯期常見 build 失誤的 SOP】
//    1. template 定義「放 header」；除非你刻意做 explicit instantiation。
//    2. dependent type 前加 typename；dependent template 前加 .template。
//    3. 用 concept (C++20) 或 static_assert + always_false_v 做友善錯誤
//       訊息。
//    4. 大量重複實例化拖累 build？用 explicit instantiation + extern
//       template 把熱點集中到一個 .cpp。
//    5. 全域常數加 inline (C++17) 不會違反 ODR。
//
//  【下一篇】
//    26_template_with_lambda.cpp ── generic / template lambda。
// ============================================================================
