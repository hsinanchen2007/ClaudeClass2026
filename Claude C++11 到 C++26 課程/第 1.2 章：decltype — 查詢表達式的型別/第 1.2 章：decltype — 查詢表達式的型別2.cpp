// =============================================================================
//  第 1.2 章：decltype — 查詢表達式的型別 (2)
//  後置回傳型別 (Trailing Return Type) — 讓回傳型別「看得到」參數
// =============================================================================
//
// 【主題資訊 Information】
//   語法：  auto 函式名(參數列) -> 回傳型別
//   標準：  C++11（後置回傳型別本身）
//           decltype 亦為 C++11
//   標頭檔：語言核心特性，不需要 #include
//           （本檔為了 static_assert 驗證才引入 <type_traits>）
//   複雜度：純編譯期語法，執行期零成本（不產生任何指令）
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼需要它 —— 名稱查找的順序問題】
//   C++ 的宣告是「由左往右」建立作用域的。傳統寫法：
//
//       decltype(a + b) add(T a, U b);      // ✗ 編譯錯誤
//       ^^^^^^^^^^^^^^^                      這裡 a、b 還不存在
//
//   編譯器讀到回傳型別時，參數列「還沒被解析」，a 與 b 尚未進入作用域，
//   所以 decltype(a + b) 找不到名字。這不是 decltype 的限制，而是
//   C++ 宣告文法的先天順序。
//
//   C++11 的解法是把回傳型別「搬到參數列後面」：
//
//       auto add(T a, U b) -> decltype(a + b);
//                             ^^^^^^^^^^^^^^^ 此時 a、b 已在作用域內
//
//   關鍵在於這裡的 auto 並不是型別推導，而是一個「佔位符 (placeholder)」，
//   它只是告訴編譯器「真正的回傳型別寫在後面的 -> 之後」。
//
// 【2. 為什麼要用 decltype(a + b)，而不是 T 或 U】
//   混合型別運算的結果型別由「usual arithmetic conversions」決定，
//   不見得是 T 也不見得是 U：
//       add(1, 2.5)      → int + double  → double（不是 T=int）
//       add(1.5f, 2.5)   → float + double → double（不是 T=float）
//   手寫任何一個參數型別都會在某些組合下失真（截斷或精度遺失）。
//   decltype(a + b) 直接把「這個運算式真正的型別」問出來，永遠正確。
//
// 【3. 與 C++14 的 auto 回傳推導如何分工】
//   C++14 起可以直接寫 `auto add(T a, U b) { return a + b; }`，由 return
//   敘述反推。既然更短，後置回傳型別是否就退休了？沒有——兩者用途不同：
//
//     * C++14 的 auto 回傳推導：型別藏在「函式本體」裡。函式本體不是
//       SFINAE 的替換脈絡，所以它不參與多載決議。
//     * 後置回傳型別：型別寫在「函式簽名」上。簽名是替換脈絡，
//       替換失敗會靜默把這個多載踢出候選集（SFINAE），不是編譯錯誤。
//
//   本檔 demoSfinae() 實測了這個差異：對沒有 f() 的型別，
//   `-> decltype(t.f())` 版本自動退位給 `call(...)` 版本。
//   這是後置回傳型別在 C++14 之後依然無可取代的理由。
//
// 【概念補充 Concept Deep Dive】
//   * decltype 的運算元是 unevaluated operand（未求值運算元）：
//     編譯器只做型別分析，不生成任何程式碼。decltype(a + b) 不會真的做加法，
//     即使寫 decltype(f()) 也不會呼叫 f，f 甚至可以「只有宣告、沒有定義」。
//     同屬未求值脈絡的還有 sizeof、noexcept 運算子，以及 std::declval 賴以
//     運作的環境。
//   * 不想依賴參數名時，可用 std::declval<T>()（C++11，<utility>）：
//         auto add(T a, U b) -> decltype(std::declval<T>() + std::declval<U>());
//     std::declval 只有宣告沒有定義，故意讓你無法在會求值的地方使用它，
//     一旦誤用，連結期或 static_assert 會擋下來。
//   * `auto f() -> R` 與 `R f()` 產生的函式型別完全相同，沒有 ABI 差異，
//     也沒有執行期成本；差別純粹在「你能不能在那個位置看到參數名」。
//
// 【注意事項 Pay Attention】
//   1. 整數提升 (integral promotion) 會讓小型別的結果變 int：
//      add(char, char) 的回傳型別是 int 而非 char（本檔 static_assert 已驗證，
//      本機 sizeof 為 4，此值為實作定義）。這是 C++ 算術轉換的既定規則，
//      不是 decltype 的問題——decltype 只是誠實回報了真正的型別。
//   2. 運算式會被寫兩次（一次在 decltype、一次在 return），兩邊若不慎寫得
//      不一致，回傳型別與實際回傳值就會對不上。C++14 的 decltype(auto)
//      正是為了消除這種重複（見第 1.3 章）。
//   3. 若 a + b 產生的是 proxy 物件（例如 std::vector<bool> 的 reference，
//      或運算式模板函式庫的暫存節點），decltype 會忠實保留那個 proxy 型別。
//      把它存起來跨越原物件生命週期就可能懸垂——這是保留精確型別的代價。
//   4. 後置回傳型別中若要用到類別成員，須注意此時尚未進入類別作用域的情況；
//      成員函式定義於類別外時，-> 之後已在類別作用域內，可直接用成員型別。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】後置回傳型別 / decltype
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 C++11 要發明後置回傳型別？直接寫 decltype(a+b) 在前面不行嗎？
//     答：不行。C++ 由左往右建立作用域，讀到前置回傳型別時參數列尚未解析，
//         a、b 還不在作用域內，名稱查找必然失敗。後置語法把回傳型別移到
//         參數列之後，此時參數已可見。前面的 auto 只是佔位符，不做推導。
//     追問：那 C++14 的 auto 回傳推導出現後，後置回傳型別是不是就沒用了？
//         （沒有。auto 推導把型別藏進函式本體，本體不是 SFINAE 替換脈絡，
//           不參與多載決議；要靠回傳型別做 SFINAE 就非得用後置語法。）
//
// 🔥 Q2. decltype(a + b) 會真的執行一次加法嗎？
//     答：不會。decltype 的運算元是 unevaluated operand，編譯器只做型別分析，
//         不產生任何程式碼。所以 decltype(f()) 不會呼叫 f，f 只要有宣告即可。
//     追問：還有哪些未求值脈絡？（sizeof、noexcept 運算子；std::declval
//         正是靠這個性質，才能「不建構物件」就取得其型別。）
//
// ⚠️ 陷阱 1. add('a', 'b') 的回傳型別是 char 嗎？
//     答：不是，是 int。char + char 會先做整數提升 (integral promotion)
//         轉成 int 再相加，運算式型別就是 int，decltype 只是誠實回報。
//     為什麼會錯：大家預期「同型別相加得到同型別」，但這條直覺對
//         小於 int 的整數型別不成立。真正的規則是 usual arithmetic
//         conversions，它會先把 char/short/bool 一律提升到 int。
//
// ⚠️ 陷阱 2. 既然 -> decltype(a+b) 這麼好，為什麼不無腦全用？
//     答：運算式被寫了兩次（decltype 一次、return 一次），兩邊不同步就出錯；
//         而且它會忠實保留 proxy 型別（如 vector<bool> 的 reference），
//         存起來可能懸垂。C++14 的 decltype(auto) 才是消除重複的正解。
//     為什麼會錯：把「保留精確型別」當成永遠是好事。精確保留代表
//         連 proxy 與 reference 都一起保留，生命週期責任跟著回到你身上。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>   // std::declval

// -----------------------------------------------------------------------------
// C++11 解法：後置回傳型別
// 回傳型別由 decltype(a + b) 決定，永遠等於運算式真正的型別。
// -----------------------------------------------------------------------------
template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b)
{
    return a + b;
}

// -----------------------------------------------------------------------------
// 不依賴參數名的寫法：用 std::declval<T>() 製造一個「假的 T 值」
// std::declval 只有宣告沒有定義，只能出現在未求值脈絡（此處為 decltype 內）。
// 好處：即使函式還沒有參數名（例如只寫宣告），也能表達回傳型別。
// -----------------------------------------------------------------------------
template<typename T, typename U>
auto addByDeclval(T a, U b) -> decltype(std::declval<T>() + std::declval<U>())
{
    return a + b;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】感測器讀數的加權平均 —— 為什麼回傳型別不能寫死
//   情境：資料採集程式從不同來源拿到讀數與權重，型別五花八門：
//         溫度是 int（原始 ADC 值）、權重是 double（校正係數）、
//         有些通道是 float。若把回傳型別寫成 T，int 通道就會把
//         校正後的小數部分整個截掉，成為靜默的精度 bug。
//   用 decltype(reading * weight) 讓編譯器決定，永遠不會截斷。
// -----------------------------------------------------------------------------
template<typename Reading, typename Weight>
auto calibrate(Reading reading, Weight weight) -> decltype(reading * weight)
{
    return reading * weight;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】用後置回傳型別做 SFINAE —— C++14 auto 做不到的事
//   情境：日誌函式庫想要「物件有 to_string() 就用它，沒有就退回預設字串」。
//   `-> decltype(t.to_string())` 寫在簽名上，替換失敗會靜默把這個多載
//   踢出候選集，改選 (...) 版本；若改用 C++14 的 auto 回傳推導，
//   型別藏在本體裡、不參與多載決議，沒有 to_string() 的型別會直接編譯錯誤。
// -----------------------------------------------------------------------------
struct Order
{
    int id;
    std::string to_string() const { return "Order#" + std::to_string(id); }
};

struct RawPacket
{
    int bytes;   // 沒有 to_string()
};

// 有 to_string() 時匹配這個（回傳型別寫在簽名上 → 參與 SFINAE）
template<typename T>
auto describe(const T& t) -> decltype(t.to_string())
{
    return t.to_string();
}

// 最低優先順序的退路：C 風格可變參數
std::string describe(...)
{
    return "<no to_string()>";
}

int main()
{
    // ─────────────────────────────────────────────────────────
    std::cout << "=== 1. 混合型別運算：回傳型別由 decltype 決定 ===\n";
    // ─────────────────────────────────────────────────────────
    auto result1 = add(1, 2);       // decltype(int + int)      = int
    auto result2 = add(1, 2.5);     // decltype(int + double)   = double
    auto result3 = add(1.5f, 2.5);  // decltype(float + double) = double

    // 用 static_assert 在編譯期證明推導結果（跑不到執行期就會擋下錯誤）
    static_assert(std::is_same<decltype(result1), int>::value,    "add(int,int) 應為 int");
    static_assert(std::is_same<decltype(result2), double>::value, "add(int,double) 應為 double");
    static_assert(std::is_same<decltype(result3), double>::value, "add(float,double) 應為 double");

    std::cout << "add(1, 2)      = " << result1 << "  (int)\n";
    std::cout << "add(1, 2.5)    = " << result2 << "  (double，沒有被截斷成 int)\n";
    std::cout << "add(1.5f, 2.5) = " << result3 << "  (double，float 被提升)\n\n";

    // ─────────────────────────────────────────────────────────
    std::cout << "=== 2. 整數提升陷阱：char + char 不是 char ===\n";
    // ─────────────────────────────────────────────────────────
    static_assert(std::is_same<decltype(add(char(1), char(2))), int>::value,
                  "char + char 會整數提升為 int");
    std::cout << "decltype(add(char,char)) 是 int，不是 char\n";
    std::cout << "sizeof = " << sizeof(add(char(1), char(2)))
              << " bytes（本機實測，int 大小為實作定義）\n\n";

    // ─────────────────────────────────────────────────────────
    std::cout << "=== 3. std::declval 版本（不依賴參數名） ===\n";
    // ─────────────────────────────────────────────────────────
    auto d = addByDeclval(3, 4.5);
    static_assert(std::is_same<decltype(d), double>::value, "declval 版應推導出 double");
    std::cout << "addByDeclval(3, 4.5) = " << d << "  (double)\n";
    std::cout << "(decltype 內是未求值脈絡，declval 沒有定義也不會出問題)\n\n";

    // ─────────────────────────────────────────────────────────
    std::cout << "=== 4. 實務：感測器讀數校正（避免靜默截斷） ===\n";
    // ─────────────────────────────────────────────────────────
    int    rawAdc    = 517;       // 原始 ADC 整數讀值
    double calFactor = 0.0625;    // 校正係數
    auto   celsius   = calibrate(rawAdc, calFactor);
    static_assert(std::is_same<decltype(celsius), double>::value,
                  "int * double 應為 double");
    std::cout << "rawAdc=517 * calFactor=0.0625 = " << celsius << " °C\n";
    std::cout << "(若回傳型別寫死成 Reading=int，結果會被截成 32，小數 .3125 靜默遺失)\n\n";

    // ─────────────────────────────────────────────────────────
    std::cout << "=== 5. 實務：用後置回傳型別做 SFINAE ===\n";
    // ─────────────────────────────────────────────────────────
    Order     ord{1024};
    RawPacket pkt{64};
    std::cout << "describe(Order)     = " << describe(ord) << "\n";
    std::cout << "describe(RawPacket) = " << describe(pkt) << "\n";
    std::cout << "(RawPacket 沒有 to_string()，樣板替換失敗 → 靜默改選 (...) 版本)\n";

    return 0;
}

// 編譯: g++ -std=c++11 -Wall -Wextra 第\ 1.2\ 章：decltype\ —\ 查詢表達式的型別2.cpp -o trailing_return
//
// 版本註記（本機以 -pedantic-errors 逐一實測）：
//   後置回傳型別、decltype、std::declval  → C++11 即可
//   auto 回傳型別推導 auto f() { ... }     → 需要 C++14（C++11 會編譯失敗）
//   decltype(auto)                         → 需要 C++14（見第 1.3 章）

// === 預期輸出 ===
// === 1. 混合型別運算：回傳型別由 decltype 決定 ===
// add(1, 2)      = 3  (int)
// add(1, 2.5)    = 3.5  (double，沒有被截斷成 int)
// add(1.5f, 2.5) = 4  (double，float 被提升)
//
// === 2. 整數提升陷阱：char + char 不是 char ===
// decltype(add(char,char)) 是 int，不是 char
// sizeof = 4 bytes（本機實測，int 大小為實作定義）
//
// === 3. std::declval 版本（不依賴參數名） ===
// addByDeclval(3, 4.5) = 7.5  (double)
// (decltype 內是未求值脈絡，declval 沒有定義也不會出問題)
//
// === 4. 實務：感測器讀數校正（避免靜默截斷） ===
// rawAdc=517 * calFactor=0.0625 = 32.3125 °C
// (若回傳型別寫死成 Reading=int，結果會被截成 32，小數 .3125 靜默遺失)
//
// === 5. 實務：用後置回傳型別做 SFINAE ===
// describe(Order)     = Order#1024
// describe(RawPacket) = <no to_string()>
// (RawPacket 沒有 to_string()，樣板替換失敗 → 靜默改選 (...) 版本)
