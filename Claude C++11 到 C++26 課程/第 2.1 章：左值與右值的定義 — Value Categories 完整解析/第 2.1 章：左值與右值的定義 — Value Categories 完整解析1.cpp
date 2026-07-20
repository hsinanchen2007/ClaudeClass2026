// =============================================================================
//  第 2.1 章 -1  —  lvalue（左值）：有身份、且不可被移動的表達式
// =============================================================================
//
// 【主題資訊 Information】
//   概念：lvalue 是 value category（值類別）之一，不是型別
//   標準版本：C++11 起正式定義（C++98 只有 lvalue / rvalue 兩分法）
//   判定工具：decltype((expr)) 推導為 T&  → 該表達式是 lvalue
//   標頭檔：本檔僅需 <iostream> <string>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「左值」這個名字會誤導你】
//   lvalue 原本是 C 語言的「left value」— 能放在 = 左邊的東西。這個名字在
//   C++ 已經不準確了：
//     const int c = 42;   // c 是 lvalue，但不能放在 = 左邊
//   真正的定義與「賦值」無關，而是問一個更根本的問題：
//     「這個表達式有沒有指向一個『可以被指認出來』的儲存位置？」
//   有 → 它有 identity（身份）→ 它是 glvalue；若又不可被移動，就是 lvalue。
//   所以現代的判準是 identity，不是 = 的左右邊。
//
// 【2. 為什麼「可以取位址」是可靠的操作型判準】
//   & 運算子的語言規則就是「只接受 lvalue（或函式）」。因此 &expr 能編譯，
//   幾乎等同於 expr 是 lvalue。這是課堂上最好用的檢驗法，因為它由編譯器裁決，
//   不靠直覺：
//     &42;        // 編譯錯誤 — 42 是 prvalue，沒有對應的儲存物件
//     &(x + 1);   // 編譯錯誤 — 運算結果是 prvalue
//   但要記得它是「充分好用」而非「定義」：真正的定義仍是 identity。
//   xvalue 同樣有 identity，卻不能直接對它取址（見 -3 檔）。
//
// 【概念補充 Concept Deep Dive】
//   C++11 把值類別重新拆成兩個獨立的問題，再組合出五個名詞：
//
//        問題 A：有 identity（可指認的儲存位置）嗎？
//        問題 B：可以被移動（安全地偷走它的資源）嗎？
//
//                       expression
//                  ┌─────────┴─────────┐
//               glvalue              rvalue
//            （有 identity）       （可被移動）
//             ┌────┴────┐       ┌────┴────┐
//          lvalue     xvalue ───┘       prvalue
//         有身份      有身份             無身份
//         不可移動    可移動             可移動
//
//   （xvalue 同時掛在 glvalue 與 rvalue 底下，所以只有 3 個基本類別）
//
//   三個「葉節點」lvalue / xvalue / prvalue 才是表達式真正的類別；
//   glvalue 與 rvalue 是兩個「聯集」概念：
//     glvalue = lvalue ∪ xvalue   （generalized lvalue，凡是有身份的）
//     rvalue  = xvalue ∪ prvalue  （凡是可被移動的）
//   注意 xvalue 同時屬於 glvalue 與 rvalue — 它就是為了填「有身份又可移動」
//   這一格才被發明出來的。C++98 的兩分法沒有這一格，所以無法表達移動語意。
//
//   ★ 由此可推出本章最重要的推論：
//     「有名字的右值引用變數，它自己是 lvalue。」
//     int&& r = 42;   // r 的『型別』是 int&&，但 r 這個『表達式』是 lvalue
//     因為 r 有名字、可以 &r，它有 identity；而編譯器不敢預設你要放棄它
//     （你後面可能還要用），所以它不落在「可被移動」那一邊。
//     型別（type）和值類別（value category）是兩個正交的概念 — 這是初學者
//     最常混淆的一點，也正是 std::move / std::forward 存在的理由。
//
// 【注意事項 Pay Attention】
//   1. 字串字面值 "Hello" 是 lvalue，型別是 const char[N]（N 含結尾 '\0'）；
//      其他字面值（42、3.14、true、nullptr）都是 prvalue。理由見下方面試題。
//   2. const 不影響值類別。const int c = 1; 的 c 仍然是 lvalue。
//   3. 「有名字 → lvalue」這條口訣對『變數』永遠成立，但別反過來推：
//      沒名字的東西不一定是 prvalue（std::move(x) 沒名字，卻是 xvalue）。
//   4. 位址值每次執行都不同（作業系統的 ASLR），下方預期輸出的位址僅供對照
//      「&x 與 &ref 相同」這類相對關係，絕對數值不會重現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】lvalue 的判定
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 字串字面值 "Hello" 是 lvalue 還是 rvalue？為什麼？
//     答：是 lvalue。它的型別是 const char[6]，是一個真實存在於程式靜態儲存區
//         （通常是 .rodata section）的陣列物件，整個程式執行期間都在、有確定
//         的位址，所以 &"Hello" 合法（型別為 const char(*)[6]）。
//         反觀 42 沒有對應的儲存物件，是 prvalue。
//     追問：那為什麼 char* p = "Hello"; 在 C++11 之後是錯的？
//         → 因為它的型別是 const char[6]，退化成 const char*，賦給 char* 會
//           丟掉 const。C++11 正式移除了這個舊的相容性轉換，要寫 const char*。
//
// 🔥 Q2. 五個值類別分別是什麼？彼此關係為何？
//     答：三個基本類別 lvalue / xvalue / prvalue，加上兩個聯集
//         glvalue（= lvalue ∪ xvalue，有 identity）與
//         rvalue（= xvalue ∪ prvalue，可被移動）。
//         用「有身份嗎 × 可移動嗎」兩個問題就能定位：有身份不可移動 = lvalue，
//         有身份可移動 = xvalue，無身份可移動 = prvalue。
//     追問：那「無身份且不可移動」那一格呢？
//         → 沒有這種表達式，所以只有三個基本類別，不是四個。
//
// ⚠️ 陷阱. 「能放在 = 左邊的就是 lvalue」— 這句話錯在哪？
//     答：兩個方向都不成立。const int c = 42; 的 c 是 lvalue 卻不能被賦值；
//         反過來 std::string("a") = "b"; 是合法的（operator= 是成員函式，
//         沒有 ref-qualifier 時可以對右值呼叫），但等號左邊是 prvalue。
//     為什麼會錯：把 C 語言時代的名稱由來當成定義。C++11 之後的判準是
//         identity 與 movability，與「能不能被賦值」無關。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>

int main() {
    // ===== 以下全部是左值 =====

    int x = 42;               // x 是左值
    std::string name = "C++"; // name 是左值

    int& ref = x;             // ref 是左值（參考本身是左值）

    int arr[3] = {1, 2, 3};
    (void)arr[0];              // 陣列元素是左值（(void) 僅用來消除「無作用」警告）

    (void)std::cout;           // std::cout 是左值

    // 字串字面值是左值！（因為它存在於靜態儲存區）
    // "Hello" 的型別是 const char[6]

    // 驗證：左值可以取位址
    std::cout << "=== 左值都可以取位址 ===\n";
    std::cout << "&x     = " << &x     << "\n";
    std::cout << "&ref   = " << &ref   << "\n";  // 和 &x 相同
    std::cout << "&arr[0]= " << &arr[0]<< "\n";
    std::cout << "&name  = " << &name  << "\n";

    // 驗證：字串字面值也可以取位址 → 它確實是左值
    std::cout << "\n=== 字串字面值是左值（可取址）===\n";
    std::cout << "sizeof(\"Hello\") = " << sizeof("Hello")
              << " （5 個字元 + 1 個結尾 '\\0'）\n";
    std::cout << "&\"Hello\"        = "
              << static_cast<const void*>(&"Hello") << "\n";
    std::cout << "型別是 const char[6]，位於靜態儲存區，整個程式期間都有效\n";

    // 驗證：const 不影響值類別
    const int c = 7;
    std::cout << "\n=== const 不改變值類別 ===\n";
    std::cout << "const int c = 7; &c = " << &c
              << " → c 仍是左值（只是不能被賦值）\n";

    std::cout << "\n（以上位址每次執行都不同，因為作業系統的 ASLR；\n"
                 "  只有「&x 與 &ref 相同」這個關係是必然的）\n";

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra "第 2.1 章：左值與右值的定義 — Value Categories 完整解析1.cpp" -o lvalue_demo

// （本機實測；所有位址每次執行都不同，只有「&x 與 &ref 相同」是固定關係）
// （以上位址每次執行都不同，因為作業系統的 ASLR；

// === 預期輸出 ===
// === 左值都可以取位址 ===
// &x     = 0x7ffcf278fbc8
// &ref   = 0x7ffcf278fbc8
// &arr[0]= 0x7ffcf278fbe4
// &name  = 0x7ffcf278fbf0
//
// === 字串字面值是左值（可取址）===
// sizeof("Hello") = 6 （5 個字元 + 1 個結尾 '\0'）
// &"Hello"        = 0x5c8debda40e2
// 型別是 const char[6]，位於靜態儲存區，整個程式期間都有效
//
// === const 不改變值類別 ===
// const int c = 7; &c = 0x7ffcf278fbcc → c 仍是左值（只是不能被賦值）
//
//   只有「&x 與 &ref 相同」這個關係是必然的）
