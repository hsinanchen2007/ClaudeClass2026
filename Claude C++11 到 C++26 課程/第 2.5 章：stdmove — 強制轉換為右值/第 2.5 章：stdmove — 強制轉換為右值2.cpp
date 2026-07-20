// =============================================================================
//  第 2.5 章：std::move — 強制轉換為右值 (2)
//  主題: 親手實作 my_move — 看懂 remove_reference 與 reference collapsing
// =============================================================================
//
// 【主題資訊 Information】
//   template<class T>
//   constexpr std::remove_reference_t<T>&& move(T&& t) noexcept;
//
//   標頭檔  : <utility>(本檔為了自己實作,另用 <type_traits> 取得 remove_reference)
//   標準版本: C++11 引入;C++14 起加上 constexpr;C++20 起標準要求 [[nodiscard]]
//   對照組  : template<class T> constexpr T&& forward(std::remove_reference_t<T>& t) noexcept;
//             std::forward 同樣是 C++11、同樣只有一行 cast,差別見【2.】
//
//   完整實作只有一行(本機 libstdc++ /usr/include/c++/15/bits/move.h:135-139):
//       return static_cast<typename std::remove_reference<_Tp>::type&&>(__t);
//   本檔的 my_move 與它逐字等價 — 你可以親手把標準庫這個函式重寫出來。
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼參數要寫 T&& 而不是 T 或 T&】
//   my_move 的參數 T&& 在「T 是待推導的 template 參數」這個前提下,不是右值引用,
//   而是 forwarding reference(轉發引用,舊稱 universal reference)。它的特殊之處
//   在於推導規則(§ [temp.deduct.call]/3):
//       傳入左值 std::string       → T 推導為 std::string&
//       傳入右值 std::string       → T 推導為 std::string
//       傳入左值 const std::string → T 推導為 const std::string&
//   這是唯一能「同時接受左值與右值、而且記得住原本是哪一種」的參數寫法。
//   若寫成 T&,右值就傳不進來;若寫成 T,會多複製一份 — 兩者都不能用。
//
// 【2. 為什麼一定要 remove_reference — reference collapsing 的陷阱】
//   假設偷懶寫成 static_cast<T&&>(arg),對「傳入左值」的情形會發生什麼事?
//       T 已經被推導成 std::string&
//       T&&  →  std::string& &&
//   C++ 不允許「引用的引用」直接存在,而是套用 reference collapsing(引用摺疊)規則:
//       & &   →  &          （只要有一個是左值引用,結果就是左值引用）
//       & &&  →  &
//       && &  →  &
//       && && →  &&         （只有兩個都是右值引用,結果才是右值引用）
//   所以 std::string& && 會摺回 std::string& — 一個左值引用。
//   結果是:my_move 對左值完全失效,傳回去還是左值,接收端會選到複製建構子。
//
//   remove_reference_t<T> 的作用就是先把引用「剝乾淨」:
//       remove_reference_t<std::string&>  →  std::string
//       remove_reference_t<std::string>   →  std::string
//   拿到不帶引用的純型別後再補上 &&,就能保證「不管傳進來的是左值還是右值,
//   出去的一定是 rvalue reference」。這正是 std::move 的全部祕密。
//
// 【3. std::move 與 std::forward 的分工】
//   兩者都是一行 static_cast,差別在「無條件」與「有條件」:
//       std::move(x)       無條件轉成右值   → 用在「我確定不再需要 x 了」
//       std::forward<T>(x) 依 T 決定        → 用在「原本是什麼,就還原成什麼」
//   std::forward 的典型場景是完美轉發(perfect forwarding):
//       template<class T> void wrapper(T&& arg) { callee(std::forward<T>(arg)); }
//   若這裡誤用 std::move,呼叫端傳進來的左值會被偷偷搬空 — 這是很難查的 bug。
//   記法:參數是 forwarding reference(T&&,T 待推導)就用 forward;
//         是具名的區域變數、而且你要放棄它了,才用 move。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 my_move 的回傳型別不能寫 auto(C++11)
//   C++11 的 auto 回傳型別推導會做 decay — 引用會被剝掉、變成回傳一個「值」,
//   等於多複製一次,整個 move 的意義就沒了。
//   C++14 起可以用 decltype(auto),它保留引用性質(見下方註解掉的 my_move_14)。
//   標準庫選擇明確寫出 remove_reference_t<T>&&,在所有版本都正確。
//
// (B) 參數 arg 在函式內是左值 — 這件事很反直覺
//   即使 arg 的宣告型別是右值引用,「一個具名的變數」在運算式中永遠是左值
//   (§ [expr.prim.id]:名字是 lvalue)。這正是為什麼函式內部還得再 cast 一次,
//   不能直接 return arg;。同一個道理也解釋了為什麼在 move constructor 內部
//   要對成員再寫一次 std::move — 參數雖是 T&&,用起來仍是左值。
//
// (C) noexcept 為什麼是必要的
//   move 只做型別轉換,不可能丟例外,所以標成 noexcept。這不只是裝飾:
//   std::vector 擴容時會用 std::move_if_noexcept 檢查元素的 move constructor
//   是不是 noexcept,不是的話就退回複製以維持強例外保證(strong exception
//   guarantee)。若 move 這條路上有任何一環不是 noexcept,整個最佳化就失效。
//
// 【注意事項 Pay Attention】
//   1. T&& 只有在「T 是本函式待推導的 template 參數」時才是 forwarding reference。
//      std::vector<T>&& 、const T&& 、以及 class template 的成員函式用到 class
//      的 T 時,都是純右值引用,不會做那套特殊推導。
//   2. 不要對 forwarding reference 參數用 std::move — 應該用 std::forward<T>,
//      否則會把呼叫端的左值搬空。
//   3. my_move / std::move 都不檢查也不在意目標型別有沒有 move constructor;
//      沒有就靜默退回複製。
//   4. 本檔示範的 test() 只是「綁定」引用,並沒有建構任何物件,所以呼叫之後
//      來源字串仍然完好 — 這再次證明「std::move 本身不搬東西」。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::move 的實作
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請當場手寫 std::move 的實作。
//     答：template<typename T>
//         constexpr std::remove_reference_t<T>&& my_move(T&& arg) noexcept {
//             return static_cast<std::remove_reference_t<T>&&>(arg);
//         }
//         三個關鍵:參數是 forwarding reference(T&&)、回傳前要 remove_reference
//         再加 &&、標上 noexcept。
//     追問：把 remove_reference 拿掉會怎樣?
//         → 傳入左值時 T 是 std::string&,T&& 經 reference collapsing 摺回
//           std::string&,函式退化成「什麼都沒做」,接收端改選複製建構子。
//
// 🔥 Q2. std::move 和 std::forward 差在哪?什麼時候用哪一個?
//     答：std::move 無條件把引數轉成右值;std::forward<T> 依 T 的推導結果
//         「有條件」還原 — 原本是左值就還原成左值,原本是右值才轉成右值。
//         參數是 forwarding reference(T&&,T 待推導)→ 用 forward;
//         具名區域變數且確定要放棄它 → 用 move。
//     追問：在 wrapper 裡對 T&& 參數誤用 std::move 有什麼後果?
//         → 呼叫端傳進來的左值會被搬空,呼叫端卻完全不知情,是典型的難查 bug。
//
// ⚠️ 陷阱. 函式簽名 void f(std::string&& s) 裡的 s,在函式內部是右值,
//          所以 g(s) 會呼叫到 g 的右值版本。這句話對嗎?
//     答：不對。s 是「具名變數」,任何具名變數在運算式中都是左值,
//         不管它宣告成什麼引用型別。要傳給右值版本必須再寫一次 g(std::move(s))。
//     為什麼會錯：把「宣告型別是右值引用」和「使用時的值類別是右值」混為一談。
//         正確的分法是:型別(type)決定能綁什麼,值類別(value category)
//         決定多載怎麼選 — 而具名變數的值類別恆為左值。
//         本檔 test() 的輸出正好示範了這件事。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// 我們自己的 move 實作
template<typename T>
typename std::remove_reference<T>::type&& my_move(T&& arg) noexcept {
    using ReturnType = typename std::remove_reference<T>::type&&;
    return static_cast<ReturnType>(arg);
}

// C++14 可以寫得更簡潔：
// template<typename T>
// decltype(auto) my_move_14(T&& arg) noexcept {
//     return static_cast<std::remove_reference_t<T>&&>(arg);
// }

// 【反例】故意漏掉 remove_reference,用來實測 reference collapsing 的後果
template<typename T>
T&& broken_move(T&& arg) noexcept {
    return static_cast<T&&>(arg);   // 傳入左值時 T = string&,T&& 摺回 string& !
}

// 印出實際選到哪一個多載;順便印出收到的內容,證明「綁定引用不會搬走資料」
void test(const std::string& s) { std::cout << "  左值版本  (收到: \"" << s << "\")\n"; }
void test(std::string&& s)      { std::cout << "  右值版本  (收到: \"" << s << "\")\n"; }

int main() {
    std::cout << "=== 1. std::move / my_move / broken_move 對照 ===\n";
    std::string s = "Hello";

    std::cout << "std::move:\n";
    test(std::move(s));

    s = "Hello";

    std::cout << "my_move:\n";
    test(my_move(s));         // 傳入左值：T = string&,remove_reference 後仍轉成右值

    std::cout << "my_move (rvalue):\n";
    test(my_move(std::string("temp")));  // 傳入右值：T = string

    std::cout << "broken_move (漏了 remove_reference):\n";
    test(broken_move(s));     // 傳入左值：T&& 摺回 string& → 選到左值版本!

    std::cout << "\n=== 2. 型別層面的證明(編譯期,static_assert) ===\n";
    // 傳入左值時 T 被推導成 std::string&
    static_assert(std::is_same<decltype(my_move(s)), std::string&&>::value,
                  "my_move 對左值必須產生 string&&");
    static_assert(std::is_same<decltype(broken_move(s)), std::string&>::value,
                  "broken_move 對左值會摺回 string&");
    // reference collapsing 的四條規則,直接用 type traits 驗給你看
    using LRef = std::string&;
    using RRef = std::string&&;
    static_assert(std::is_same<LRef&,  std::string&>::value,  "& &   -> &");
    static_assert(std::is_same<LRef&&, std::string&>::value,  "& &&  -> &");
    static_assert(std::is_same<RRef&,  std::string&>::value,  "&& &  -> &");
    static_assert(std::is_same<RRef&&, std::string&&>::value, "&& && -> &&");
    std::cout << "全部 static_assert 通過(能編譯就代表通過):\n";
    std::cout << "  my_move(左值)     -> std::string&&   ✓\n";
    std::cout << "  broken_move(左值) -> std::string&    ✗ 正是漏 remove_reference 的後果\n";
    std::cout << "  reference collapsing: & & -> & / & && -> & / && & -> & / && && -> &&\n";

    std::cout << "\n=== 3. 綁定引用 != 搬走資料 ===\n";
    std::string keep = "still here";
    test(std::move(keep));   // 選到右值版本,但 test 只是綁定引用,沒有建構任何物件
    std::cout << "呼叫 test(std::move(keep)) 之後 keep = \"" << keep << "\"\n";
    std::cout << "(內容完好 — 因為沒有任何 move constructor 被呼叫)\n";

    std::cout << "\n=== 4. std::move vs std::forward ===\n";
    // forward 依 T 還原原本的值類別
    auto wrapper = [](auto&& arg) {
        // 這裡必須用 forward:傳左值就還原成左值,傳右值才轉成右值
        test(std::forward<decltype(arg)>(arg));
    };
    std::string lv = "lvalue-in";
    std::cout << "wrapper(左值)  -> "; wrapper(lv);
    std::cout << "wrapper(右值)  -> "; wrapper(std::string("rvalue-in"));
    std::cout << "呼叫後 lv = \"" << lv << "\" (forward 沒有把左值搬空)\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.5 章：stdmove — 強制轉換為右值2.cpp" -o move2
// 註: 第 4 段用到 generic lambda(auto&& 參數),那是 C++14 起的功能;
//     my_move / broken_move / static_assert 的部分 C++11 即可。

// === 預期輸出 ===
// (執行結果為固定值,不含位址或計時,每次執行皆相同)
//
// === 1. std::move / my_move / broken_move 對照 ===
// std::move:
//   右值版本  (收到: "Hello")
// my_move:
//   右值版本  (收到: "Hello")
// my_move (rvalue):
//   右值版本  (收到: "temp")
// broken_move (漏了 remove_reference):
//   左值版本  (收到: "Hello")
//
// === 2. 型別層面的證明(編譯期,static_assert) ===
// 全部 static_assert 通過(能編譯就代表通過):
//   my_move(左值)     -> std::string&&   ✓
//   broken_move(左值) -> std::string&    ✗ 正是漏 remove_reference 的後果
//   reference collapsing: & & -> & / & && -> & / && & -> & / && && -> &&
//
// === 3. 綁定引用 != 搬走資料 ===
//   右值版本  (收到: "still here")
// 呼叫 test(std::move(keep)) 之後 keep = "still here"
// (內容完好 — 因為沒有任何 move constructor 被呼叫)
//
// === 4. std::move vs std::forward ===
// wrapper(左值)  ->   左值版本  (收到: "lvalue-in")
// wrapper(右值)  ->   右值版本  (收到: "rvalue-in")
// 呼叫後 lv = "lvalue-in" (forward 沒有把左值搬空)
