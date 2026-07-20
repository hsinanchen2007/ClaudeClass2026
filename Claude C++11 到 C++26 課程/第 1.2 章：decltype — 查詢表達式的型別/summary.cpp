// =============================================================================
// 第 1.2 章：decltype — 查詢表達式的型別（綜合複習）
// =============================================================================
//
// 【主題資訊 Information】
//   語法    ：decltype(expression)          // 只「查詢」型別，不對表達式求值
//   標準版本：decltype                       C++11
//             後置回傳型別 auto f() -> T     C++11
//             decltype(auto)                 C++14（本章末尾對照用）
//             generic lambda [](auto&){}     C++14 ← 本檔宣告 C++14 的原因
//   標頭檔  ：<type_traits>（is_same / is_reference / decay / declval）
//   複雜度  ：純編譯期構造，執行期成本為零
//   本檔宣告的標準：C++14
//
// 【詳細解釋 Explanation】
//
// 【1. decltype 要解決的問題：型別「無法被寫出來」】
//   C++11 之前，有些型別你根本沒辦法用手寫出來：
//     * lambda 的型別（每個 lambda 都是獨一無二的匿名類別）
//     * 模板中 a + b 的結果型別（依 T、U 而定，可能是第三種型別）
//     * 巢狀模板的迭代器型別
//   decltype 讓你可以「指著一個表達式說：就是它的型別」，
//   把型別的定義權從「人手抄」交還給「編譯器推導」。
//
// 【2. 不求值語境（unevaluated context）— 這是核心機制】
//   decltype(expr) 裡的 expr 只做型別分析，絕對不會被執行：
//       int f() { std::cout << "被呼叫了"; return 0; }
//       decltype(f()) x;   // x 是 int，但 f() 從未被呼叫，什麼都不會印
//   這和 sizeof、noexcept、typeid（非多型）同屬不求值語境。
//   實務上最重要的後果：可以對「根本無法建構的型別」做型別運算，
//   這就是 std::declval<T>() 的用途 —— 它宣告了回傳 T&& 卻沒有定義，
//   只能在不求值語境使用，用來「假裝有一個 T 物件」以查詢其成員型別。
//
// 【3. 兩條推導規則，順序不可顛倒】
//   規則 1：若運算元是「未加括號的識別字」或「類別成員存取」
//           → 結果就是它宣告時的型別（完整保留 const 與 &）。
//   規則 2：否則依表達式的 value category 決定：
//               prvalue → T        （純值，如 42、f() 回傳值）
//               lvalue  → T&       （有身分可取址，如 (x)、v[0]）
//               xvalue  → T&&      （將亡值，如 std::move(x)）
//   兩條規則的交界就是惡名昭彰的「括號陷阱」：
//       int x = 0;
//       decltype(x)   → int    （規則 1：x 是識別字）
//       decltype((x)) → int&   （規則 2：(x) 是 lvalue 表達式）
//
// 【4. 後置回傳型別（trailing return type）為什麼必要】
//   在 C++11，回傳型別寫在函式名稱「之前」，那時參數 a、b 還沒進入作用域：
//       template<class T, class U>
//       decltype(a + b) add(T a, U b);        // ❌ a、b 此時尚未宣告
//   把回傳型別移到參數列之後，名稱查找就看得到參數了：
//       template<class T, class U>
//       auto add(T a, U b) -> decltype(a + b) // ✅ C++11 正解
//   C++14 起可直接 auto add(T a, U b)（回傳型別推導），
//   但要「完整保留參考」仍需 decltype(auto)（見第 1.3 章）。
//
// 【5. decltype + SFINAE：編譯期偵測成員是否存在】
//   decltype 會在「替換失敗」時讓該候選函式安靜地退出重載集合，而不是編譯錯誤，
//   這就是 SFINAE（Substitution Failure Is Not An Error）。
//   本檔示範兩種經典寫法：函式重載法、類別模板特化法（void_t 模式的前身）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為何 decltype(v[0]) 是 T& 而不是 T
//     std::vector<T>::operator[] 的回傳型別就是 T&（reference），
//     函式呼叫回傳參考時，該呼叫表達式的 value category 是 lvalue，
//     依規則 2 得到 T&。這代表 decltype 能「無損」保留容器的參考語意 —
//     用 auto 則會複製一份，這是效能與正確性的實際差異。
//     反例：std::vector<bool>::operator[] 回傳的是 proxy 物件（prvalue），
//     decltype 會得到那個 proxy 型別而非 bool&，這是 vector<bool> 的著名坑。
//
// (B) decltype 與陣列：唯一不會 decay 的推導方式
//         int arr[5];
//         auto      a = arr;   // int*      ← decay（陣列退化成指標，長度資訊消失）
//         decltype(arr) b;     // int[5]    ← 完整保留，長度還在
//     需要「保留陣列長度」時（例如寫 sizeof(arr)/sizeof(arr[0]) 的泛型版本），
//     只能靠 decltype 或模板的 T(&)[N] 形式。
//
// (C) std::declval 的存在理由
//     想查詢 T::size() 的回傳型別，直覺會寫 decltype(T().size())，
//     但這要求 T 必須可預設建構 —— 對只有帶參數建構子的型別就失敗了。
//     std::declval<T>() 產生一個「假的」T&&，只在不求值語境合法：
//         decltype(std::declval<T>().size())   // 不需要 T 可建構
//     它刻意「只宣告不定義」，所以一旦不小心在求值語境用到，連結期就會報錯。
//
// (D) 為什麼 SFINAE 用 `...` 當備援
//     C++ 重載決議中，省略號（ellipsis）的匹配優先權最低。
//     把「偵測成功」的版本寫成模板 + decltype 條件，
//     「偵測失敗」的版本寫成 f(...)，就形成「有就選精準版、沒有才掉到 ... 版」，
//     這是 C++11/14 沒有 concepts 時的標準做法。C++20 之後可直接用 requires。
//
// 【注意事項 Pay Attention】
//   1. decltype 不求值：decltype(f()) 不會呼叫 f()，也不會有副作用。
//   2. 括號會改變結果：decltype((x)) 是 T&，decltype(x) 是 T。多一對括號就變型別。
//   3. decltype(v[0]) 對一般 vector 是 T&；對 std::vector<bool> 是 proxy 型別，不是 bool&。
//   4. decltype 不會讓陣列 decay，auto 會 —— 需要保留長度時務必用 decltype。
//   5. std::declval 只能用在不求值語境；在求值語境使用會連結失敗（刻意設計）。
//   6. 本檔多處變數僅為展示推導結果而宣告，故以 (void)x; 標示 —— 因為本檔是 C++14，
//      不能用 C++17 才有的 [[maybe_unused]]（已用 -pedantic-errors 驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. decltype(x) 與 decltype((x)) 差在哪？為什麼？
//     答：decltype(x) 走規則 1（未加括號的識別字）→ 得到 x 的宣告型別 int。
//         decltype((x)) 的運算元變成表達式，走規則 2；(x) 是 lvalue → int&。
//     追問：那什麼時候會不小心踩到？→ decltype(auto) f(){ int x=0; return (x); }
//         回傳型別變 int&，指向已銷毀的區域變數，是懸空引用（UB）。
//
// 🔥 Q2. decltype 和 auto 推導同一個表達式，結果會一樣嗎？
//     答：常常不一樣。auto 走模板推導：剝掉 top-level const、剝掉參考、陣列 decay 成指標。
//         decltype 完整保留 const 與參考，陣列也不 decay。
//         例：const int ci=0; auto a=ci;（int）但 decltype(ci) 是 const int。
//     追問：想要 auto 的便利 + decltype 的精確怎麼辦？→ decltype(auto)（C++14）。
//
// 🔥 Q3. 為什麼需要後置回傳型別 auto f(T a, U b) -> decltype(a+b)？
//     答：因為在傳統寫法中，回傳型別出現在參數列之前，此時 a、b 尚未進入作用域，
//         無法在回傳型別裡引用它們。後置語法把回傳型別移到參數列之後就解決了。
//     追問：C++14 可以只寫 auto 嗎？→ 可以（回傳型別推導），但 auto 會剝掉參考；
//         要無損保留必須用 decltype(auto)。
//
// ⚠️ 陷阱 1. decltype(f()) 會不會執行 f()？
//     答：不會。decltype 是不求值語境，只做型別分析。即使 f() 裡有 printf 也不會印，
//         即使 f() 有副作用也不會發生。
//     為什麼會錯：把 decltype 想成「執行一次看看結果是什麼型別」的執行期行為；
//         實際上它完全發生在編譯期，運算元從未被求值。
//
// ⚠️ 陷阱 2. 對 std::vector<bool> 用 decltype(v[0])，會得到 bool& 嗎？
//     答：不會。vector<bool> 是空間最佳化的特化版，operator[] 回傳的是 proxy 物件
//         （std::vector<bool>::reference）的 prvalue，所以 decltype 得到那個 proxy 型別。
//     為什麼會錯：以為「vector<T> 的 operator[] 一律回傳 T&」，
//         忽略了 vector<bool> 是標準明文規定的特例（每個 bool 只佔 1 bit）。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++14 -Wall -Wextra -o summary summary.cpp
// 說明: 本檔案彙整 decltype 的所有核心觀念與用法，
//       閱讀本檔即可完整複習本章所有內容。
// =============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>

// =============================================================================
// 【輔助工具】型別資訊列印巨集
// =============================================================================
// 利用 <type_traits> 的 std::is_reference 和 std::is_const
// 來在執行期印出某個表達式被 decltype 推導後的型別特性。
// #expr 會把巨集參數轉成字串，方便閱讀輸出。
#define PRINT_TYPE_INFO(expr) \
    std::cout << #expr << ":\n"; \
    std::cout << "  is_reference: " << std::is_reference<decltype(expr)>::value << "\n"; \
    std::cout << "  is_const: " << std::is_const<typename std::remove_reference<decltype(expr)>::type>::value << "\n\n";

// =============================================================================
// 【第一部分】decltype 基本概念
// =============================================================================
//
// decltype 是 C++11 引入的關鍵字，用途是「查詢」一個表達式的型別，
// 而不會對該表達式求值（即不會產生副作用、不會呼叫函式）。
//
// 語法：decltype(表達式)
//
// 與 auto 的關鍵差異：
//   - auto  會忽略頂層 const 和參考（類似模板參數推導）。
//   - decltype 會完整保留表達式的 const 與參考修飾。
//
// decltype 的兩大推導規則：
//   規則 1：若表達式是「未加括號的識別符號」或「類別成員存取」，
//           則推導結果就是該變數/成員的宣告型別。
//   規則 2：若表達式不是單純的識別符號（包括加了括號的情況），
//           則根據表達式的值類別推導：
//             - prvalue（純右值）→ T
//             - lvalue（左值）  → T&
//             - xvalue（將亡值）→ T&&
// =============================================================================

// =============================================================================
// 【第二部分】用於展示函式回傳型別推導的輔助函式
// =============================================================================

// 回傳值（prvalue，純右值）
int getValue()
{
    return 42;
}

// 回傳左值參考
int& getReference()
{
    static int value = 100;
    return value;
}

// 回傳 const 左值參考
const int& getConstReference()
{
    static int value = 200;
    return value;
}

// =============================================================================
// 【第三部分】後置回傳型別（Trailing Return Type）
// =============================================================================
//
// 問題背景：
//   在傳統 C++ 語法中，函式的回傳型別寫在函式名稱之前，
//   但模板函式的參數 a、b 在回傳型別的位置尚未被宣告，
//   因此無法使用 decltype(a + b) 作為回傳型別。
//
// C++11 的解法 — 後置回傳型別語法：
//   auto func(T a, U b) -> decltype(a + b)
//   用 auto 作為佔位符，真正的回傳型別寫在 -> 之後，
//   此時參數 a、b 已經在作用域中，可以使用 decltype 推導。
//
// 注意：C++14 起可以直接寫 auto（讓編譯器從 return 推導），
//       但後置回傳型別在某些 SFINAE 場景仍然必要。
// =============================================================================

template<typename T, typename U>
auto add(T a, U b) -> decltype(a + b)
{
    // 回傳型別由 decltype(a + b) 決定：
    //   add(1, 2)      → int + int    = int
    //   add(1, 2.5)    → int + double = double
    //   add(1.5f, 2.5) → float + double = double
    return a + b;
}

// =============================================================================
// 【第四部分】decltype + SFINAE 檢測成員函式
// =============================================================================
//
// SFINAE（Substitution Failure Is Not An Error）：
//   模板參數替換失敗不是錯誤，編譯器會自動嘗試其他候選。
//
// 搭配 decltype 可以在編譯期檢測一個型別是否擁有某個成員函式，
// 這是泛型程式設計中極為重要的技巧。
// =============================================================================

// -----------------------------------------------------------------------------
// 方法 1：函式重載 + SFINAE
// -----------------------------------------------------------------------------
// 主模板：當 T 有 .size() 時，decltype(t.size(), std::true_type{}) 合法，
//         逗號運算子使整個 decltype 的結果是 std::true_type。
// 備用模板：使用 ... (C 風格可變參數) 作為最低優先順序的匹配，
//           當主模板替換失敗（SFINAE）時才會匹配到這裡。
// -----------------------------------------------------------------------------

template<typename T>
auto hasSize(T& t) -> decltype(t.size(), std::true_type{})
{
    (void)t;  // 避免未使用參數的編譯警告
    return std::true_type{};
}

std::false_type hasSize(...)
{
    return std::false_type{};
}

// -----------------------------------------------------------------------------
// 方法 2：類別模板特化 + SFINAE（更通用的寫法）
// -----------------------------------------------------------------------------
// 預設模板：繼承 std::false_type（value = false）。
// 部分特化：第二個模板參數使用 decltype(std::declval<T>().size(), void())，
//           如果 T 有 .size()，此表達式合法，void() 確保結果型別是 void，
//           與預設參數 typename = void 匹配，選擇這個特化版本。
//
// std::declval<T>() 可以在不建構物件的情況下產生 T 的右值參考，
// 用於 decltype 內部的型別推導（不實際求值）。
// -----------------------------------------------------------------------------

template<typename T, typename = void>
struct HasSizeMethod : std::false_type {};

template<typename T>
struct HasSizeMethod<T, decltype(std::declval<T>().size(), void())>
    : std::true_type {};

// =============================================================================
// 【第五部分】測試用的自定義類別
// =============================================================================

// 有 size() 成員函式（回傳 std::size_t）
class WithSize
{
public:
    std::size_t size() const { return 42; }
};

// 沒有 size() 成員函式
class WithoutSize
{
public:
    int getValue() const { return 100; }
};

// 有 size() 但回傳型別是 int（仍然會被偵測到）
class WithIntSize
{
public:
    int size() const { return 10; }
};

// =============================================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// =============================================================================
// decltype 是「型別查詢機制」，屬於語言層工具，不是演算法。
// LeetCode 題目考的是資料結構與演算法，沒有任何一題的解法核心是 decltype；
// 硬套一題（例如把 Two Sum 的 iterator 宣告改用 decltype）只會製造
// 「為了用而用」的假範例，反而誤導讀者以為實務上該這樣寫。
// 因此本章不提供 LeetCode 範例，改以兩個真實會發生的工程情境示範。
// =============================================================================


// -----------------------------------------------------------------------------
// 【日常實務範例 1】泛型統計工具 — 用 decltype 讓回傳型別自動跟隨容器
//   情境：監控系統要對「各種數值容器」算總和與平均，容器元素可能是
//         int（請求數）、double（延遲秒數）、long long（位元組數）。
//   為什麼一定要用 decltype：
//     總和的型別必須由「元素型別」決定，不能寫死成 int（會截斷 double），
//     也不能寫死成 double（long long 大值會失去精度）。
//     用 decltype(*c.begin()) 取得元素型別，再用 std::decay 去掉參考，
//     就得到「這個容器的元素值型別」，回傳型別因此自動正確。
//   注意 std::decay 的必要性：*c.begin() 是 lvalue，decltype 會給 T&，
//         拿 T& 當累加變數的型別會編譯失敗（參考必須綁定既有物件）。
// -----------------------------------------------------------------------------
template <typename Container>
auto sumOf(const Container& c)
    -> typename std::decay<decltype(*c.begin())>::type
{
    // 元素值型別（已去掉 const 與 &）
    using Elem = typename std::decay<decltype(*c.begin())>::type;
    Elem total = Elem();          // 值初始化：int→0, double→0.0
    for (const auto& v : c) total += v;
    return total;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】就地調整容器內容 — decltype 保留參考，auto 會失敗
//   情境：資料清洗。把一批感測器讀數中的負值（顯然是故障值）就地夾到 0。
//   為什麼用到本主題：
//     decltype(v[0]) 是 T&（參考），拿它宣告變數就能「就地改到原容器」。
//     若改用 auto x = v[i]; 會複製一份，改了複本、原容器毫髮無傷 ——
//     這是實務上非常常見、而且很難用眼睛看出來的 bug。
// -----------------------------------------------------------------------------
void clampNegativeToZero(std::vector<double>& readings)
{
    for (std::size_t i = 0; i < readings.size(); ++i) {
        // decltype(readings[i]) 推導為 double&（operator[] 回傳參考 → lvalue → T&）
        decltype(readings[i]) slot = readings[i];   // slot 是原元素的別名，不是複本
        if (slot < 0.0) slot = 0.0;                 // 改 slot == 改原容器
    }
}

// 對照組：故意用 auto，示範「改不到原容器」的錯誤版本
void clampNegativeToZero_WRONG(std::vector<double>& readings)
{
    for (std::size_t i = 0; i < readings.size(); ++i) {
        auto slot = readings[i];      // ← auto 剝掉參考，slot 是「複本」
        if (slot < 0.0) slot = 0.0;   // 只改到複本，原容器完全沒變
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例 3】用 SFINAE + decltype 決定「這個型別能不能直接印大小」
//   情境：寫通用 log 函式。若物件有 size()（容器、字串）就印筆數，
//         沒有就只印「單一物件」。這是 log/除錯工具的常見需求。
//   為什麼用到本主題：
//     用 decltype(std::declval<T>().size()) 偵測成員是否存在。
//     declval 讓我們不需要真的建構一個 T 就能查詢其成員 —— 對只有
//     帶參數建構子的型別（實務上非常多）這是唯一可行的做法。
// -----------------------------------------------------------------------------
template <typename T>
std::string describeSize(const T& obj, decltype(std::declval<T>().size(), 0) = 0)
{
    return "容器，元素數 = " + std::to_string(obj.size());
}

std::string describeSize(...)
{
    return "單一物件（沒有 size()）";
}


// =============================================================================
// 【主程式】完整展示所有 decltype 用法
// =============================================================================

int main()
{
    // =========================================================================
    // 1. 基本用法：從變數推導型別
    // =========================================================================
    // decltype(變數) 會得到該變數的宣告型別，
    // 可用來宣告同型別的新變數。
    // =========================================================================
    std::cout << "===== 1. 基本用法：從變數推導型別 =====\n";

    int x = 10;
    double y = 3.14;
    std::string str = "Hello";

    decltype(x) a = 20;        // a 的型別是 int（與 x 相同）
    decltype(y) b = 2.718;     // b 的型別是 double（與 y 相同）
    decltype(str) s = "World"; // s 的型別是 std::string（與 str 相同）

    std::cout << "a = " << a << " (與 x 同型別: int)\n";
    std::cout << "b = " << b << " (與 y 同型別: double)\n";
    std::cout << "s = " << s << " (與 str 同型別: std::string)\n\n";

    // =========================================================================
    // 2. 保留 const 與參考（decltype 的核心優勢）
    // =========================================================================
    // decltype 完整保留變數的 const 修飾與參考修飾，
    // 這是 decltype 與 auto 最大的不同。
    //   - decltype(const int) → const int
    //   - decltype(int&)      → int&
    //   - decltype(const int&)→ const int&
    // =========================================================================
    std::cout << "===== 2. 保留 const 與參考 =====\n";

    const int cx = 100;        // const int
    int& rx = x;               // int& — x 的參考
    const int& crx = x;       // const int& — x 的常數參考

    decltype(cx) c1 = 50;     // const int（保留 const）
    (void)c1;  // 僅為展示推導結果而宣告
    decltype(rx) r1 = x;      // int&（保留參考）
    decltype(crx) cr1 = x;    // const int&（同時保留 const 和參考）
    (void)cr1;  // 僅為展示推導結果而宣告

    // c1 = 60;  // 編譯錯誤！c1 是 const int，不可修改
    r1 = 999;    // 合法，r1 是 int&，修改 r1 就是修改 x

    std::cout << "修改 r1 = 999 後:\n";
    std::cout << "  x = " << x << " (證明 r1 是 x 的參考)\n\n";

    // =========================================================================
    // 3. auto vs decltype 的重要差異
    // =========================================================================
    // auto 的推導規則類似模板引數推導：
    //   - 忽略頂層 const（auto val = const_var → 非 const 的複製）
    //   - 忽略參考（auto val = ref_var → 非參考的複製）
    //
    // decltype 則完整保留原始型別的所有修飾：
    //   - decltype(const_var) → const（保留）
    //   - decltype(ref_var)   → 參考（保留）
    //
    // 總結：需要保留 const/參考時用 decltype，
    //       需要「值語意的複製」時用 auto。
    // =========================================================================
    std::cout << "===== 3. auto vs decltype 比較 =====\n";

    const int value = 42;
    int& ref = x;

    auto       autoVal = value;      // int（auto 忽略 const，得到非 const 的複製）
    (void)autoVal;  // 僅為展示推導結果而宣告
    decltype(value) declVal = 100;   // const int（decltype 保留 const）
    (void)declVal;  // 僅為展示推導結果而宣告

    auto       autoRef = ref;        // int（auto 忽略參考，得到獨立的複製）
    (void)autoRef;  // 僅為展示推導結果而宣告
    decltype(ref) declRef = x;       // int&（decltype 保留參考）

    autoRef = 111;   // 修改 autoRef 不影響 x（因為是獨立複製）
    std::cout << "autoRef = 111 後, x = " << x << " (auto 複製，不影響 x)\n";

    x = 500;         // 重設 x 的值
    declRef = 222;   // 修改 declRef 就是修改 x（因為是參考）
    std::cout << "declRef = 222 後, x = " << x << " (decltype 保留參考，修改 x)\n\n";

    // =========================================================================
    // 4. 括號的影響（非常重要的陷阱！）
    // =========================================================================
    // 這是 decltype 最容易出錯的地方：
    //
    //   decltype(var)   → 使用規則 1：結果是 var 的宣告型別
    //   decltype((var)) → 使用規則 2：(var) 是左值表達式，結果是 T&
    //
    // 差一個括號，型別就不同！
    //   decltype(num)   → int
    //   decltype((num)) → int&（因為 (num) 是左值表達式）
    //
    // 實務建議：除非刻意需要參考語意，否則不要在 decltype 裡加多餘括號。
    // =========================================================================
    std::cout << "===== 4. 括號的影響 (重要！) =====\n";

    int num = 10;

    decltype(num)   d1 = 0;       // int（識別符號，用宣告型別）
    (void)d1;  // 僅為展示推導結果而宣告
    decltype((num)) d2 = num;     // int&（左值表達式，必須初始化參考）

    d2 = 777;
    std::cout << "d2 = 777 後, num = " << num << "\n";
    std::cout << "(證明 decltype((num)) 是 int&，修改 d2 會影響 num)\n\n";

    // =========================================================================
    // 5. 表達式的值類別與 decltype 推導
    // =========================================================================
    // decltype 對非識別符號的表達式，根據值類別推導：
    //
    //   prvalue（純右值，如算術運算結果）→ T
    //     decltype(i + j)  → int（因為 i + j 是 prvalue）
    //     decltype(i < j)  → bool
    //
    //   lvalue（左值，如賦值運算的結果）→ T&
    //     decltype(i += j) → int&（因為 i += j 回傳 i 的參考，是 lvalue）
    //
    // 注意：decltype 不會對表達式求值，i += j 不會真的被執行。
    // =========================================================================
    std::cout << "===== 5. 表達式的型別推導 =====\n";

    int i = 5, j = 10;

    decltype(i + j) sum = 0;       // int（prvalue → T）
    decltype(i < j) cmp = true;    // bool（prvalue → T）
    (void)cmp;  // 僅為展示推導結果而宣告
    decltype(i += j) ref2 = i;     // int&（lvalue → T&）
    (void)ref2;  // 僅為展示推導結果而宣告

    sum = 100;
    std::cout << "decltype(i + j)  -> int,  sum = " << sum << "\n";
    std::cout << "decltype(i < j)  -> bool\n";
    std::cout << "decltype(i += j) -> int& (i += j 回傳左值參考)\n\n";

    // =========================================================================
    // 6. 函式回傳型別的推導
    // =========================================================================
    // decltype(函式呼叫) 推導函式的回傳型別，但不會實際呼叫函式。
    //
    //   decltype(getValue())         → int（回傳值，prvalue）
    //   decltype(getReference())     → int&（回傳左值參考）
    //   decltype(getConstReference())→ const int&（回傳 const 左值參考）
    //
    // 重點：decltype 只做型別推導，函式不會被執行，沒有任何副作用。
    // =========================================================================
    std::cout << "===== 6. 函式回傳型別推導 =====\n";

    decltype(getValue()) val1 = 0;              // int
    (void)val1;  // 僅為展示推導結果而宣告
    decltype(getReference()) val2 = x;          // int&
    (void)val2;  // 僅為展示推導結果而宣告
    decltype(getConstReference()) val3 = x;     // const int&
    (void)val3;  // 僅為展示推導結果而宣告

    std::cout << "decltype(getValue())         -> int\n";
    std::cout << "decltype(getReference())     -> int&\n";
    std::cout << "decltype(getConstReference())-> const int&\n";
    std::cout << "(以上推導過程中，函式都沒有被實際呼叫)\n\n";

    // =========================================================================
    // 7. 陣列與指標的推導差異
    // =========================================================================
    // decltype 保留完整的陣列型別，不會退化為指標：
    //   decltype(arr) → int[5]（保留陣列型別和大小）
    //
    // 對比 auto 的行為：
    //   auto arrAuto = arr → int*（陣列退化為指標）
    //
    // 這在需要保留陣列資訊的場景非常重要。
    // =========================================================================
    std::cout << "===== 7. 陣列與指標 =====\n";

    int arr[5] = {1, 2, 3, 4, 5};
    int* ptr = arr;

    decltype(arr) arr2 = {10, 20, 30, 40, 50};  // int[5]（保留陣列型別！）
    decltype(ptr) ptr2 = nullptr;                // int*
    (void)ptr2;  // 僅為展示推導結果而宣告

    std::cout << "sizeof(arr)  = " << sizeof(arr) << " bytes\n";
    std::cout << "sizeof(arr2) = " << sizeof(arr2) << " bytes (decltype 保留陣列型別)\n";

    auto arrAuto = arr;  // int*（auto 使陣列退化為指標）
    std::cout << "sizeof(arrAuto) = " << sizeof(arrAuto) << " bytes (auto 退化為指標)\n\n";

    // =========================================================================
    // 8. 容器元素存取的型別推導
    // =========================================================================
    // decltype(vec[0]) → int&（因為 operator[] 回傳參考）
    // decltype(vec.size()) → std::vector<int>::size_type（prvalue）
    //
    // 這在泛型程式設計中很有用，可以自動推導容器元素的正確型別。
    // =========================================================================
    std::cout << "===== 8. 容器元素存取 =====\n";

    std::vector<int> vec = {1, 2, 3, 4, 5};

    // vec[0] 的回傳型別是 int&（operator[] 回傳左值參考）
    decltype(vec[0]) elem = vec[0];  // int&
    elem = 100;                      // 透過參考修改容器內的元素

    std::cout << "修改 elem 後, vec[0] = " << vec[0] << " (elem 是 vec[0] 的參考)\n";

    // vec.size() 的回傳型別是 size_type（prvalue）
    decltype(vec.size()) sz = vec.size();
    std::cout << "vec.size() = " << sz << "\n\n";

    // =========================================================================
    // 9. 搭配 typedef / using 定義型別別名
    // =========================================================================
    // 當型別非常複雜時（如巢狀模板），可以用 decltype 搭配 using
    // 來自動推導並建立型別別名，避免手動寫出冗長的型別名稱。
    //
    // 範例：
    //   using DataType = decltype(data);          // vector<pair<string,int>>
    //   using ElemType = decltype(data[0]);        // pair<string,int>&
    //   using IterType = decltype(data.begin());   // vector<...>::iterator
    // =========================================================================
    std::cout << "===== 9. 搭配 typedef / using =====\n";

    std::vector<std::pair<std::string, int>> data;
    data.push_back({"Alice", 95});
    data.push_back({"Bob", 87});

    // 使用 decltype 自動推導複雜型別，建立簡潔的別名
    using DataType = decltype(data);           // std::vector<std::pair<std::string, int>>
    using ElemType = decltype(data[0]);        // std::pair<std::string, int>&
    using IterType = decltype(data.begin());   // std::vector<...>::iterator

    // 用 static_assert 在編譯期「證明」上面三行註解寫的型別真的是對的。
    // 特別注意 ElemType 帶著 & — 因為 vector::operator[] 回傳的是參考（lvalue），
    // 依 decltype 規則 2：lvalue → T&。這正是 decltype 與 auto 的分水嶺：
    // 若改寫成 auto e = data[0];，會退化成 pair<string,int>（複製一份，不是參考）。
    static_assert(std::is_same<ElemType, std::pair<std::string, int>&>::value,
                  "decltype(data[0]) 應為 pair<string,int>& — operator[] 回傳參考");
    static_assert(std::is_same<IterType, DataType::iterator>::value,
                  "decltype(data.begin()) 應為該容器的 iterator");

    DataType data2;
    data2.push_back({"Charlie", 92});
    std::cout << "成功使用 decltype 定義複雜型別別名\n";
    std::cout << "data2[0].first = " << data2[0].first << "\n\n";

    // =========================================================================
    // 10. 類別成員的 decltype
    // =========================================================================
    // 對類別成員使用 decltype 有兩種方式：
    //
    // (a) 透過物件存取（不加括號）→ 得到成員的宣告型別
    //     decltype(pt.x) → int
    //
    // (b) 透過物件存取（加括號）→ 成為左值表達式，得到參考
    //     decltype((pt.x)) → int&
    //
    // (c) 透過類別名稱（不需要物件實體）
    //     decltype(Point::x) → int
    // =========================================================================
    std::cout << "===== 10. 類別成員的 decltype =====\n";

    struct Point
    {
        int x;
        double y;
    };

    Point pt{10, 20.5};

    decltype(pt.x) px = 100;       // int（成員存取，宣告型別）
    (void)px;  // 僅為展示推導結果而宣告
    decltype(pt.y) py = 3.14;      // double
    (void)py;  // 僅為展示推導結果而宣告
    decltype((pt.x)) prx = pt.x;   // int&（加括號 → 左值表達式 → 參考）

    prx = 999;
    std::cout << "修改 prx 後, pt.x = " << pt.x << " (decltype((pt.x)) 是 int&)\n";

    // 不需要物件實體，直接用類別名稱推導成員型別
    decltype(Point::x) memberX = 0;   // int
    (void)memberX;  // 僅為展示推導結果而宣告
    decltype(Point::y) memberY = 0.0; // double
    (void)memberY;  // 僅為展示推導結果而宣告
    std::cout << "decltype(Point::x) -> int\n";
    std::cout << "decltype(Point::y) -> double\n\n";

    // =========================================================================
    // 11. 後置回傳型別實際測試
    // =========================================================================
    // 使用前面定義的 add() 模板函式，展示 decltype 在後置回傳型別的效果。
    // 編譯器根據 decltype(a + b) 自動推導正確的回傳型別。
    // =========================================================================
    std::cout << "===== 11. 後置回傳型別 (Trailing Return Type) =====\n";

    auto result1 = add(1, 2);       // decltype(int + int)       = int
    auto result2 = add(1, 2.5);     // decltype(int + double)    = double
    auto result3 = add(1.5f, 2.5);  // decltype(float + double)  = double

    std::cout << "add(1, 2)      = " << result1 << " (int)\n";
    std::cout << "add(1, 2.5)    = " << result2 << " (double)\n";
    std::cout << "add(1.5f, 2.5) = " << result3 << " (double)\n\n";

    // =========================================================================
    // 12. decltype + SFINAE：檢測型別是否有 size() 成員函式
    // =========================================================================
    // 方法 1（函式重載）：
    //   利用 decltype(t.size(), std::true_type{}) 做回傳型別，
    //   若 t.size() 不合法，SFINAE 會排除此重載，
    //   轉而匹配 std::false_type hasSize(...) 備用版本。
    //
    // 方法 2（類別模板特化）：
    //   預設模板繼承 std::false_type，
    //   部分特化使用 decltype(std::declval<T>().size(), void())，
    //   若合法則匹配特化版本（繼承 std::true_type）。
    //
    // 兩種方法都能在編譯期判斷型別是否擁有特定成員函式。
    // =========================================================================
    std::cout << "===== 12. SFINAE 方法 1：函式重載 =====\n";
    std::cout << std::boolalpha;  // 輸出 true/false 而非 1/0

    WithSize objWith;
    WithoutSize objWithout;
    WithIntSize objWithInt;
    (void)objWithInt;  // 僅為展示推導結果而宣告
    std::string testStr = "Hello";
    int number = 42;

    // decltype(hasSize(x))::value 取得回傳型別（true_type 或 false_type）的 value
    std::cout << "std::vector<int> has size(): "
              << decltype(hasSize(vec))::value << "\n";
    std::cout << "std::string has size():      "
              << decltype(hasSize(testStr))::value << "\n";
    std::cout << "WithSize has size():         "
              << decltype(hasSize(objWith))::value << "\n";
    std::cout << "WithoutSize has size():      "
              << decltype(hasSize(objWithout))::value << "\n";
    std::cout << "int has size():              "
              << decltype(hasSize(number))::value << "\n\n";

    std::cout << "===== 13. SFINAE 方法 2：類別模板特化 =====\n";

    std::cout << "HasSizeMethod<std::vector<int>>: "
              << HasSizeMethod<std::vector<int>>::value << "\n";
    std::cout << "HasSizeMethod<std::string>:      "
              << HasSizeMethod<std::string>::value << "\n";
    std::cout << "HasSizeMethod<WithSize>:         "
              << HasSizeMethod<WithSize>::value << "\n";
    std::cout << "HasSizeMethod<WithoutSize>:      "
              << HasSizeMethod<WithoutSize>::value << "\n";
    std::cout << "HasSizeMethod<int>:              "
              << HasSizeMethod<int>::value << "\n";
    std::cout << "HasSizeMethod<WithIntSize>:      "
              << HasSizeMethod<WithIntSize>::value << "\n\n";

    // =========================================================================
    // 14. 實際應用：根據 SFINAE 結果選擇不同行為
    // =========================================================================
    // 使用 HasSizeMethod 在泛型 lambda 中做執行期判斷。
    // （C++17 可用 if constexpr 改為編譯期判斷，效率更好）
    // std::decay<T> 用來去除參考和 cv 修飾，得到純粹的型別。
    // =========================================================================
    std::cout << "===== 14. 實際應用：條件式呼叫 =====\n";

    auto printInfo = [](auto& container)
    {
        using ContainerType = typename std::decay<decltype(container)>::type;

        if (HasSizeMethod<ContainerType>::value)
        {
            std::cout << "此型別有 size() 方法\n";
        }
        else
        {
            std::cout << "此型別沒有 size() 方法\n";
        }
    };

    printInfo(vec);         // 有 size()
    printInfo(objWithout);  // 沒有 size()

    std::cout << "\n";

    // =========================================================================
    // 【總結】decltype 核心要點速查
    // =========================================================================
    //
    // 1. 基本語法：decltype(表達式) 推導型別，不求值表達式。
    //
    // 2. 推導規則：
    //    - 未加括號的識別符號 → 宣告型別（規則 1）
    //    - 其他表達式依值類別（規則 2）：
    //        prvalue → T, lvalue → T&, xvalue → T&&
    //
    // 3. 與 auto 的差異：
    //    - auto 忽略頂層 const 和參考
    //    - decltype 完整保留 const 和參考
    //
    // 4. 括號陷阱：
    //    - decltype(var) → 宣告型別
    //    - decltype((var)) → 左值表達式，得到 T&
    //
    // 5. 後置回傳型別：
    //    - auto func(T a, U b) -> decltype(a + b)
    //    - 解決參數在回傳型別位置尚未宣告的問題
    //
    // 6. SFINAE 應用：
    //    - 搭配 decltype 在編譯期檢測型別是否有特定成員函式
    //    - 函式重載法和類別模板特化法都能實現
    //
    // 7. 實用場景：
    //    - 保留容器元素存取的參考語意
    //    - 保留陣列型別（不退化為指標）
    //    - 定義複雜型別的別名
    //    - 類別成員型別推導
    // =========================================================================

    // =========================================================================
    // 13. 日常實務範例
    // =========================================================================
    std::cout << "===== 13. 日常實務範例 =====\n";

    // --- 實務 1：泛型統計，回傳型別自動跟隨元素型別 ---
    std::vector<int>    requests{120, 340, 90, 512};
    std::vector<double> latencies{0.12, 0.35, 0.08, 0.51};

    auto reqTotal = sumOf(requests);     // int
    auto latTotal = sumOf(latencies);    // double
    static_assert(std::is_same<decltype(reqTotal), int>::value,
                  "int 容器的總和應為 int");
    static_assert(std::is_same<decltype(latTotal), double>::value,
                  "double 容器的總和應為 double — 不能被截斷成 int");

    std::cout << "請求數總和 (int)    : " << reqTotal << "\n";
    std::cout << "延遲總和   (double) : " << latTotal << "\n";
    std::cout << "→ 回傳型別由 decltype 從元素推導，double 沒有被截斷成 int\n\n";

    // --- 實務 2：decltype 保留參考 vs auto 複製 ---
    std::vector<double> sensorA{1.5, -2.0, 3.25, -0.75};
    std::vector<double> sensorB = sensorA;   // 同樣的資料給錯誤版本用

    clampNegativeToZero(sensorA);        // decltype 版本：真的改到了
    clampNegativeToZero_WRONG(sensorB);  // auto 版本：只改到複本

    std::cout << "decltype 版本 (正確): ";
    for (double v : sensorA) std::cout << v << " ";
    std::cout << "\nauto 版本     (錯誤): ";
    for (double v : sensorB) std::cout << v << " ";
    std::cout << "\n→ auto 剝掉了參考，負值完全沒有被修正\n\n";

    // --- 實務 3：SFINAE + declval 偵測 size() ---
    std::cout << "std::vector<int> : " << describeSize(requests) << "\n";
    std::cout << "std::string      : " << describeSize(std::string("hello")) << "\n";
    std::cout << "int              : " << describeSize(42) << "\n\n";

    std::cout << "===== 複習完成 =====\n";

    return 0;
}

// 編譯: g++ -std=c++14 -Wall -Wextra "summary.cpp" -o ch12_summary

// === 預期輸出 ===
// ===== 1. 基本用法：從變數推導型別 =====
// a = 20 (與 x 同型別: int)
// b = 2.718 (與 y 同型別: double)
// s = World (與 str 同型別: std::string)
//
// ===== 2. 保留 const 與參考 =====
// 修改 r1 = 999 後:
//   x = 999 (證明 r1 是 x 的參考)
//
// ===== 3. auto vs decltype 比較 =====
// autoRef = 111 後, x = 999 (auto 複製，不影響 x)
// declRef = 222 後, x = 222 (decltype 保留參考，修改 x)
//
// ===== 4. 括號的影響 (重要！) =====
// d2 = 777 後, num = 777
// (證明 decltype((num)) 是 int&，修改 d2 會影響 num)
//
// ===== 5. 表達式的型別推導 =====
// decltype(i + j)  -> int,  sum = 100
// decltype(i < j)  -> bool
// decltype(i += j) -> int& (i += j 回傳左值參考)
//
// ===== 6. 函式回傳型別推導 =====
// decltype(getValue())         -> int
// decltype(getReference())     -> int&
// decltype(getConstReference())-> const int&
// (以上推導過程中，函式都沒有被實際呼叫)
//
// ===== 7. 陣列與指標 =====
// sizeof(arr)  = 20 bytes
// sizeof(arr2) = 20 bytes (decltype 保留陣列型別)
// sizeof(arrAuto) = 8 bytes (auto 退化為指標)
//
// ===== 8. 容器元素存取 =====
// 修改 elem 後, vec[0] = 100 (elem 是 vec[0] 的參考)
// vec.size() = 5
//
// ===== 9. 搭配 typedef / using =====
// 成功使用 decltype 定義複雜型別別名
// data2[0].first = Charlie
//
// ===== 10. 類別成員的 decltype =====
// 修改 prx 後, pt.x = 999 (decltype((pt.x)) 是 int&)
// decltype(Point::x) -> int
// decltype(Point::y) -> double
//
// ===== 11. 後置回傳型別 (Trailing Return Type) =====
// add(1, 2)      = 3 (int)
// add(1, 2.5)    = 3.5 (double)
// add(1.5f, 2.5) = 4 (double)
//
// ===== 12. SFINAE 方法 1：函式重載 =====
// std::vector<int> has size(): true
// std::string has size():      true
// WithSize has size():         true
// WithoutSize has size():      false
// int has size():              false
//
// ===== 13. SFINAE 方法 2：類別模板特化 =====
// HasSizeMethod<std::vector<int>>: true
// HasSizeMethod<std::string>:      true
// HasSizeMethod<WithSize>:         true
// HasSizeMethod<WithoutSize>:      false
// HasSizeMethod<int>:              false
// HasSizeMethod<WithIntSize>:      true
//
// ===== 14. 實際應用：條件式呼叫 =====
// 此型別有 size() 方法
// 此型別沒有 size() 方法
//
// ===== 13. 日常實務範例 =====
// 請求數總和 (int)    : 1062
// 延遲總和   (double) : 1.06
// → 回傳型別由 decltype 從元素推導，double 沒有被截斷成 int
//
// decltype 版本 (正確): 1.5 0 3.25 0 
// auto 版本     (錯誤): 1.5 -2 3.25 -0.75 
// → auto 剝掉了參考，負值完全沒有被修正
//
// std::vector<int> : 容器，元素數 = 4
// std::string      : 容器，元素數 = 5
// int              : 單一物件（沒有 size()）
//
// ===== 複習完成 =====
