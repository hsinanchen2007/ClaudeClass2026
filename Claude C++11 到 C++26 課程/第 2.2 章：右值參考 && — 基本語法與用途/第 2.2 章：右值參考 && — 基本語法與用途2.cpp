// =============================================================================
//  第 2.2 章 之 2  —  三種參考的綁定規則總表（T& / const T& / T&&）
// =============================================================================
//
// 【主題資訊 Information】
//   語法：   T& / const T& / T&&
//   標準：   T& 與 const T& 為 C++98；T&& 為 C++11
//            （本機以 -std=c++03 -pedantic-errors 實測，&& 在 C++03 直接語法錯誤）
//   標頭檔： std::move 需要 <utility>
//   成本：   O(1)，綁定不複製
//
// 【詳細解釋 Explanation】
//
// 【1. 綁定規則的完整矩陣】
//   把「參考型別」對「實參的 value category」畫成表，只有四種組合是合法的：
//
//       參考型別        綁 lvalue     綁 rvalue     可否修改所綁物件
//       ─────────────────────────────────────────────────────────
//       T&              ✔            ✘            ✔
//       const T&        ✔            ✔            ✘
//       T&&             ✘            ✔            ✔
//       const T&&       ✘            ✔            ✘   （幾乎沒人用）
//
//   把這張表倒過來看，會冒出一個關鍵洞察：
//     * const T& 是唯一「什麼都接」的萬用讀取通道 → 所以它是 C++98 傳參的預設答案。
//     * T&& 是唯一「只接右值、又能寫」的通道     → 所以它是移動語意的專屬入口。
//   兩者合起來，才能讓同一個函式名同時支援「複製」與「移動」兩條路徑
//   （這正是本章第 3、4 個範例檔在做的事）。
//
//   const T&& 這一格存在但幾乎沒有實用價值：它只接右值，卻又不准你偷。
//   標準庫裡唯一常見的用途是把某些多載明確 `= delete` 掉，
//   例如 std::ref / std::cref 對右值的多載就被刪除，用來阻擋懸空。
//
// 【2. 為什麼 `int&& g = x + 1;` 合法，`int&& e = x;` 卻不合法？】
//   關鍵永遠不在變數，而在**運算式**：
//       x        → lvalue（有名字、可取址）              → T&& 接不了
//       x + 1    → prvalue（算式結果，沒有名字）         → T&& 接得到
//       std::move(x) → xvalue（將亡值），屬於 rvalue     → T&& 接得到
//
//   換句話說，判斷能不能綁，要問的是「等號右邊那個**運算式**是什麼 category」，
//   不是「右邊那個東西長得像不像變數」。x + 1 的結果是一個沒有名字的臨時值，
//   所以它是右值；即使 x 本身是個貨真價實的 lvalue 也一樣。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 三大 value category（C++11 起的正式分類）
//       glvalue ┬─ lvalue   有身分、不可搬移（具名變數、回傳 T& 的函式呼叫）
//               └─ xvalue   有身分、可搬移  （std::move(x)、回傳 T&& 的函式呼叫）
//       rvalue  ┬─ xvalue
//               └─ prvalue  無身分、可搬移  （42、x+1、std::string("a")）
//   「rvalue」= xvalue ∪ prvalue。T&& 能綁的正是這個聯集。
//   記憶法：**有身分（可取址）→ glvalue；可被偷 → rvalue**。
//   xvalue 兩者皆是，這就是為什麼 std::move 出來的東西「既能取址、又能被偷」。
//
// (B) std::move 的真面目
//   它不是一個「動作」，而是一個轉型。簡化後的實作就是：
//       template <class T>
//       constexpr typename std::remove_reference<T>::type&&
//       move(T&& t) noexcept {
//           return static_cast<typename std::remove_reference<T>::type&&>(t);
//       }
//   全部工作就是 static_cast 成 T&&，不產生任何機器碼、不搬任何位元組。
//   真正搬東西的是**接手方的移動建構子／移動指派**。
//   所以 `std::move(x)` 之後 x 有沒有被改變，完全取決於它有沒有被交給一個
//   會偷資源的函式。本檔的 `int&& h = std::move(x);` 只是綁定，
//   x 的值原封不動。
//
// (C) 為什麼要有「const T& 也能接右值」這條例外？
//   這條規則早於移動語意（C++98 就有），目的是讓「傳唯讀參數」不必為
//   lvalue / rvalue 各寫一個多載：`void f(const std::string&)` 一個就能接
//   變數、字面值、函式回傳值。代價是它唯讀 —— 而這個代價正是 C++11
//   必須補上 T&& 的原因。
//
// 【注意事項 Pay Attention】
//   1. 判斷「能不能綁」看的是**運算式的 value category**，不是變數長相。
//   2. 具名變數即使型別是 T&&，它自己當運算式時仍然是 lvalue
//      （本檔的 f、g、h 都可以再取址、再指派）。
//   3. std::move 需要 #include <utility>。實務上常因為 <iostream> 等標頭
//      間接帶進來而「剛好能編譯」，這是不可攜的，務必自己明確 include。
//   4. 對 int 這種 trivially copyable 的型別，「移動」和「複製」的成本完全相同。
//      移動語意只有在型別持有 heap 資源（string / vector / unique_ptr）時才有意義。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】三種參考的綁定規則
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 請默寫 T& / const T& / T&& 分別能綁定什麼？哪一個最「萬用」？
//     答：T& 只綁 lvalue；const T& 兩者皆可但唯讀；T&& 只綁 rvalue 且可寫。
//         最萬用的是 const T&（什麼都接）—— 但正因為它唯讀，無法實作移動，
//         C++11 才需要補上 T&&。
//     追問：那 const T&& 有什麼用？
//         → 幾乎沒有實用價值（只接右值卻不准偷）。標準庫主要拿它來
//           `= delete` 掉右值多載以阻擋懸空，例如 std::cref。
//
// 🔥 Q2. `int x = 10; int&& g = x + 1;` 合法嗎？`int&& e = x;` 呢？為什麼？
//     答：前者合法，後者不合法。因為判斷依據是**等號右邊運算式**的 value
//         category：`x + 1` 是 prvalue（右值），`x` 是 lvalue。
//         T&& 只接右值，所以接得到前者、接不到後者。
//     追問：怎麼讓 `int&& e = x;` 能編譯？
//         → 寫成 `int&& e = std::move(x);`，std::move(x) 是 xvalue（右值）。
//
// ⚠️ 陷阱. `int&& h = std::move(x);` 執行完，x 的值被搬走變成 0 了嗎？
//     答：沒有。x 仍然是 10。std::move 只是一個 static_cast，
//         它把運算式標成 rvalue，**完全沒有搬動任何資料**。
//         真正會改變來源的是「接手方的移動建構子／移動指派」，
//         而這裡只是綁定一個參考，沒有任何人接手。
//     為什麼會錯：名字取得太有誤導性。很多人把 std::move 讀成動詞「移動」，
//         以為呼叫它就會發生搬移。它更精確的名字應該叫 rvalue_cast。
//         另外對 int 這種 trivially copyable 型別，即使真的被移動，
//         標準也不要求來源歸零 —— 移動 int 就等於複製 int。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <utility>   // std::move（不要依賴其他標頭間接帶進來）

void show_binding() {
    int x = 10;

    // T&：只能綁定左值
    int& a = x;            // OK
    // int& b = 42;        // 錯誤：T& 不能綁右值

    // const T&：左值右值都能綁定
    const int& c = x;      // OK：綁定左值
    const int& d = 42;     // OK：綁定右值（延長臨時物件生命週期）

    // T&&：只能綁定右值
    // int&& e = x;        // 錯誤：x 是左值
    int&& f = 42;          // OK：綁定右值（prvalue）
    int&& g = x + 1;       // OK：x + 1 的結果是右值（prvalue）

    // 如果硬要讓右值參考綁定左值，用 std::move
    int&& h = std::move(x);  // OK：std::move(x) 是 xvalue（右值）

    std::cout << "所有綁定成功\n\n";

    std::cout << "  T&        a = " << a << "   （x 的別名）\n";
    std::cout << "  const T&  c = " << c << "   （綁 lvalue，唯讀）\n";
    std::cout << "  const T&  d = " << d << "   （綁 rvalue，唯讀，壽命被延長）\n";
    std::cout << "  T&&       f = " << f << "   （綁 prvalue 42）\n";
    std::cout << "  T&&       g = " << g << "   （綁 prvalue x+1）\n";
    std::cout << "  T&&       h = " << h << "   （綁 xvalue std::move(x)）\n";

    // 重點：std::move 完全沒有搬動 x
    std::cout << "\n  std::move(x) 之後，x 依然是 " << x
              << "（std::move 只是轉型，不搬資料）\n";

    // 這些具名的右值參考，自己當運算式時全都是 lvalue
    f = 99;
    std::cout << "  f 可以被指派成 " << f << " → 證明具名的 T&& 是 lvalue\n";
}

int main() {
    show_binding();
    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra "第 2.2 章：右值參考 && — 基本語法與用途2.cpp" -o rv2

// === 預期輸出 ===
// 所有綁定成功
//
//   T&        a = 10   （x 的別名）
//   const T&  c = 10   （綁 lvalue，唯讀）
//   const T&  d = 42   （綁 rvalue，唯讀，壽命被延長）
//   T&&       f = 42   （綁 prvalue 42）
//   T&&       g = 11   （綁 prvalue x+1）
//   T&&       h = 10   （綁 xvalue std::move(x)）
//
//   std::move(x) 之後，x 依然是 10（std::move 只是轉型，不搬資料）
//   f 可以被指派成 99 → 證明具名的 T&& 是 lvalue
