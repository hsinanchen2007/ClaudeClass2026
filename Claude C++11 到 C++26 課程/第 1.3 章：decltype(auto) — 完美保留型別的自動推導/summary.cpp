// ============================================================================
// 總結檔案：decltype(auto) — 完美保留型別的自動推導 (C++14)
// ============================================================================
//
// 【主題資訊 Information】
//   語法    ：decltype(auto) 變數 = 表達式;
//             decltype(auto) 函式名(參數) { return 表達式; }
//   標準版本：decltype(auto)          C++14（N3638）
//             auto 回傳型別推導        C++14（同一份提案）
//             decltype               C++11
//   標頭檔  ：無需額外標頭（語言核心特性）
//   複雜度  ：純編譯期推導，執行期成本為零
//   本檔宣告的標準：C++14
//
// 【詳細解釋 Explanation】
//
// 【1. decltype(auto) 補的是哪個洞】
//   C++11 有兩種推導工具，但各缺一半：
//     auto     — 好寫，但用「模板推導規則」：剝掉 top-level const、剝掉參考。
//     decltype — 精確，但必須把整個表達式再寫一次，冗長且容易寫錯。
//   最痛的場景是「泛型包裝函式」：
//       template<class F> ??? wrapper(F f) { return f(); }
//       // f() 可能回傳 T、T&、const T& —— 我要「原封不動」轉發回去
//   用 auto 會把參考剝掉（回傳複本，語意錯了）；
//   用 decltype 得寫 -> decltype(f())，等於把 f() 寫兩次（C++11 的作法）。
//   C++14 的 decltype(auto) 意思是：「位置照 auto 寫，規則照 decltype 算」。
//       template<class F> decltype(auto) wrapper(F f) { return f(); }  // 完美
//
// 【2. 一句話記住三者的差別】
//       auto           x = expr;   // 一定是「值」，剝 const、剝 &
//       auto&          x = expr;   // 一定是「參考」，expr 必須是 lvalue
//       decltype(auto) x = expr;   // 是值還是參考，完全由 expr 自己決定
//   前兩者是「你決定」，第三個是「表達式決定」。這就是它強大也危險的原因。
//
// 【3. 括號陷阱：decltype(auto) 把 decltype 的規則一起繼承了】
//   decltype 規則 1（未加括號的識別字）→ 宣告型別；
//   decltype 規則 2（其他表達式）→ 依 value category，lvalue 得 T&。
//   因此：
//       int x = 42;
//       decltype(auto) a = x;    // int   ← 規則 1
//       decltype(auto) b = (x);  // int&  ← 規則 2，只差一對括號
//   在 return 語句裡，這對括號就是「回傳值」與「回傳懸空參考」的分界。
//
// 【4. 什麼時候該用、什麼時候不該用】
//   該用：
//     * 泛型轉發函式（wrapper / 中介層 / 記錄呼叫次數的 decorator）
//     * 需要原封不動保留 const 與參考的存取器
//   不該用：
//     * 一般變數宣告。你只是要一個值時，auto 更簡潔、也不會意外做出參考。
//   實務準則：decltype(auto) 是給「轉發」用的工具，不是 auto 的升級版。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼 auto 沒辦法保留參考
//     auto 的推導完全比照 template<class T> void f(T param) 的 by-value 推導。
//     by-value 的語意是「複製一份給你」，複製自然不會複製出參考，
//     top-level const 也沒有意義（你自己那份要不要 const 由你決定）。
//     所以 auto 剝掉它們不是缺陷，是 by-value 語意的必然結果。
//
// (B) decltype(auto) 只能單獨使用
//     它必須整個作為型別出現，不能加修飾：
//         const decltype(auto) x = ...;   // ❌ 不合法
//         decltype(auto)* p = ...;        // ❌ 不合法
//     原因是 decltype(auto) 的整個意義就是「型別完全由表達式決定」，
//     任何額外修飾都會和這個語意衝突。
//
// (C) 多個 return 的一致性要求
//     decltype(auto)（以及 auto）回傳型別推導要求所有 return 推出同一型別：
//         decltype(auto) f(bool c) {
//             int x = 1; static int y = 2;
//             if (c) return x;    // int
//             else   return (y);  // int&  → ❌ 兩個 return 型別不一致，編譯錯誤
//         }
//     這其實是好事：編譯器會逼你把「到底要回傳值還是參考」想清楚。
//
// (D) 懸空參考為何是 UB，而不是「一定會 crash」
//     return (local); 回傳指向已銷毀區域變數的參考。
//     標準對此不做任何行為保證 —— 那塊堆疊記憶體可能還沒被覆寫（看起來「正常」），
//     也可能已被下一次函式呼叫覆寫（讀到垃圾值），也可能觸發存取違規。
//     編譯最佳化等級、呼叫序列都會影響觀察結果。
//     因此絕不能把任何一次觀察到的輸出當成「這段程式的行為」。
//     GCC 對明顯的情況會給 -Wreturn-local-addr 警告，但它無法涵蓋所有寫法。
//
// 【注意事項 Pay Attention】
//   1. return (x); 與 return x; 在 decltype(auto) 下回傳型別不同（T& vs T）。
//   2. 回傳區域變數的參考是未定義行為（UB）；其後果不固定，不可依賴任何一次觀察結果。
//   3. decltype(auto) 不能與 const、*、& 等修飾詞併用。
//   4. 多個 return 必須推導出完全相同的型別，否則編譯錯誤。
//   5. 只想要「值」時請用 auto；decltype(auto) 是為轉發而生的工具。
//   6. decltype(auto) 是 C++14；在 -std=c++11 下編譯會失敗（已用 -pedantic-errors 驗證）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】decltype(auto)
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. auto、decltype、decltype(auto) 三者怎麼選？
//     答：auto 用模板推導規則，一定得到「值」（剝 const 與 &），寫起來最簡潔。
//         decltype 用宣告型別規則，完整保留 const 與 &，但要把表達式再寫一次。
//         decltype(auto) 是 C++14 的組合：寫的位置像 auto，推導規則用 decltype。
//         選擇準則：要值用 auto，要轉發用 decltype(auto)。
//     追問：為什麼不乾脆全用 decltype(auto)？→ 因為它會讓「是值還是參考」由
//         表達式決定，一不小心就做出參考，反而增加懸空風險；一般變數用 auto 更安全。
//
// 🔥 Q2. 下面這個函式有什麼問題？
//         decltype(auto) f() { int x = 42; return (x); }
//     答：return 加了括號 → (x) 是 lvalue 表達式 → 回傳型別被推導成 int&，
//         而 x 是區域變數，函式返回後即銷毀，呼叫端拿到的是懸空參考，
//         後續使用是未定義行為。拿掉括號寫 return x; 即回傳 int（安全）。
//     追問：編譯器會擋嗎？→ GCC 對這種明顯情況會給 -Wreturn-local-addr 警告，
//         但只要多繞一層（例如經由另一個函式回傳）就可能偵測不到，不能依賴它。
//
// 🔥 Q3. decltype(auto) 最典型的正當用途是什麼？
//     答：泛型轉發包裝函式。要把被包裝函式的回傳型別（值/參考/const 參考）
//         原封不動傳遞出去，只有 decltype(auto) 做得到。
//         template<class F, class... A>
//         decltype(auto) invokeLogged(F&& f, A&&... a) {
//             return std::forward<F>(f)(std::forward<A>(a)...);
//         }
//     追問：C++11 怎麼寫？→ 用後置回傳型別把表達式寫兩次：
//         auto invokeLogged(F&& f, A&&... a) -> decltype(f(std::forward<A>(a)...))
//
// ⚠️ 陷阱. decltype(auto) x = (y); 和 decltype(auto) x = y; 只差括號，會一樣嗎？
//     答：不一樣。y 是識別字走規則 1 → 得到 y 的宣告型別（值）。
//         (y) 是 lvalue 表達式走規則 2 → 得到 T&，x 變成 y 的別名，改 x 就是改 y。
//     為什麼會錯：把括號當成「不影響語意的排版」。在 decltype 家族裡，
//         括號是有語法意義的 —— 它決定走規則 1 還是規則 2。
// ═══════════════════════════════════════════════════════════════════════════
//
// 編譯指令: g++ -std=c++14 -Wall -Wextra -o summary summary.cpp
// 說明: 本檔案彙整第 1.3 章所有核心概念，涵蓋 decltype(auto) 的完整用法
// ============================================================================
//
// 【章節總覽】
//   1. decltype(auto) 的核心概念與動機
//   2. auto vs decltype(auto) 的變數推導差異
//   3. 參考保留的驗證
//   4. 括號對 decltype(auto) 的影響（極重要陷阱）
//   5. 函式回傳型別：auto vs decltype(auto)
//   6. 透過 decltype(auto) 回傳參考修改原始資料
//   7. 泛型轉發函式（完美轉發回傳型別）
//   8. 容器元素存取
//   9. 表達式推導規則
//  10. 常見陷阱與最佳實踐
//
// 【核心觀念】
//   decltype(auto) 是 C++14 引入的型別推導方式。
//   它結合了 auto 的便利性與 decltype 的精確性：
//     - auto   ：按照「模板引數推導規則」推導，會去除 const、參考
//     - decltype(auto)：按照「decltype 規則」推導，完整保留 const、參考、值類別
//
//   decltype 規則回顧：
//     - 若表達式是「不帶括號的識別符號」(id-expression)，
//       decltype 得到該變數宣告時的型別
//     - 若表達式是「左值」(lvalue)，decltype 得到 T&
//     - 若表達式是「純右值」(prvalue)，decltype 得到 T
//     - 若表達式是「將亡值」(xvalue)，decltype 得到 T&&
//
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>  // std::move, std::forward
#include <map>

// ============================================================================
// 【輔助工具】型別特性印出函式
// ============================================================================
// 透過 std::is_reference、std::is_const、std::is_lvalue_reference、
// std::is_rvalue_reference 來檢視推導結果的型別屬性
template<typename T>
void printTypeInfo(const char* name)
{
    std::cout << name << ":\n";
    std::cout << "  is_reference:     "
              << std::is_reference<T>::value << "\n";
    std::cout << "  is_const:         "
              << std::is_const<typename std::remove_reference<T>::type>::value << "\n";
    std::cout << "  is_lvalue_ref:    "
              << std::is_lvalue_reference<T>::value << "\n";
    std::cout << "  is_rvalue_ref:    "
              << std::is_rvalue_reference<T>::value << "\n";
    std::cout << "\n";
}

// ============================================================================
// 【全域變數】用於示範回傳參考的情境
// ============================================================================
int globalValue = 100;
std::vector<int> globalVec = {10, 20, 30, 40, 50};

// ============================================================================
// 【第 1 節】基本函式：回傳值、回傳左值參考、回傳 const 左值參考
// ============================================================================

// 回傳純值（prvalue）— 呼叫者取得副本
int getValue()
{
    return 42;
}

// 回傳左值參考（lvalue reference）— 呼叫者可透過參考修改原始資料
int& getReference()
{
    return globalValue;
}

// 回傳 const 左值參考 — 呼叫者只能讀取，不可修改
const int& getConstReference()
{
    static int value = 200;
    return value;
}

// ============================================================================
// 【第 2 節】auto vs decltype(auto) 作為函式回傳型別
// ============================================================================

// --- auto 作為回傳型別 ---
// auto 會套用模板引數推導規則，去除 const 和參考
// 因此即使函式內部回傳的是參考，auto 也會將它「衰退」成值型別

auto autoReturn()
{
    // globalValue 是 int，auto 推導為 int
    // 回傳的是 globalValue 的「副本」
    return globalValue;
}

auto autoReturnRef()
{
    // getReference() 回傳 int&，但 auto 會去除參考
    // 最終回傳型別仍為 int（複製），不是 int&
    return getReference();
}

// --- decltype(auto) 作為回傳型別 ---
// decltype(auto) 會完整套用 decltype 規則，保留回傳表達式的原始型別

decltype(auto) decltypeAutoReturn()
{
    // globalValue 是「不帶括號的識別符號」
    // decltype(globalValue) = int（宣告型別）
    // 所以回傳 int（值）
    return globalValue;
}

decltype(auto) decltypeAutoReturnRef()
{
    // getReference() 回傳 int&
    // decltype(getReference()) = int&
    // 所以回傳 int&（完整保留參考！）
    return getReference();
}

decltype(auto) decltypeAutoReturnConstRef()
{
    // getConstReference() 回傳 const int&
    // decltype(getConstReference()) = const int&
    // 所以回傳 const int&（保留 const 與參考！）
    return getConstReference();
}

// ============================================================================
// 【第 3 節】泛型轉發函式 — decltype(auto) 的經典應用場景
// ============================================================================
// 泛型轉發函式的目的：呼叫任意可呼叫物件（函式、lambda 等），
// 並將其回傳值「原封不動」地傳遞回去。
//
// 如果使用 auto，回傳型別會衰退，參考會遺失。
// 使用 decltype(auto) 才能「完美保留」回傳型別。

// 使用 auto 的版本：會丟失回傳的參考特性
template<typename F>
auto callWithAuto(F&& f)
{
    // 即使 f() 回傳 int&，這裡也會衰退成 int（複製）
    return f();
}

// 使用 decltype(auto) 的版本：完美保留回傳型別
template<typename F>
decltype(auto) callWithDecltypeAuto(F&& f)
{
    // 若 f() 回傳 int&，這裡也回傳 int&
    // 若 f() 回傳 int，這裡也回傳 int
    // 若 f() 回傳 const int&，這裡也回傳 const int&
    return f();
}

// ============================================================================
// 【第 4 節】進階：回傳括號包裹的區域變數（危險示範，僅供理解）
// ============================================================================
// 注意：以下函式會導致「未定義行為」(Undefined Behavior)
// 因為它回傳了區域變數的參考。此處僅用於說明括號的影響。
//
// decltype(auto) dangerousReturn()
// {
//     int local = 42;
//     return (local);  // decltype((local)) = int&，回傳區域變數的參考！
//                      // 函式結束後 local 被銷毀，呼叫者持有「懸空參考」
//                      // 這是未定義行為，絕對不可以這樣寫！
// }

// ============================================================================
// 【LeetCode 實戰範例】—— 本章刻意從缺，理由如下
// ============================================================================
// decltype(auto) 的存在價值是「完美轉發回傳型別」，這是函式庫/中介層的
// 設計問題，不是演算法問題。LeetCode 的題目是單一函式回傳一個值，
// 從來不需要保留參考語意，也沒有包裝層要轉發。
// 硬把某題的回傳型別改成 decltype(auto) 只會讓程式更難懂而毫無收穫。
// 因此本章不提供 LeetCode 範例，改以兩個真實工程情境示範。
// ============================================================================


// ----------------------------------------------------------------------------
// 【日常實務範例 1】呼叫計時／記錄包裝器（decorator）
//   情境：要幫既有函式加上「呼叫次數統計」，但不能改變任何呼叫端的語意 ——
//         原本回傳參考的，包裝後還是要回傳參考（否則呼叫端拿到的是複本，
//         寫入就寫不進去了，會變成很難查的 bug）。
//   為什麼一定要用 decltype(auto)：
//     被包裝的函式可能回傳 T、T& 或 const T&。
//     用 auto 會一律剝成 T（複本）→ 呼叫端的寫入失效。
//     用 decltype(auto) 才能原封不動地把型別轉發出去。
// ----------------------------------------------------------------------------
int g_callCount = 0;   // 這個包裝器累積的呼叫次數

template <typename Func, typename... Args>
decltype(auto) invokeCounted(Func&& f, Args&&... args)
{
    ++g_callCount;
    // 注意：這裡 return 沒有加括號，直接轉發呼叫表達式
    return std::forward<Func>(f)(std::forward<Args>(args)...);
}

// 對照組：用 auto 的錯誤版本 —— 會把參考剝成值
template <typename Func, typename... Args>
auto invokeCounted_WRONG(Func&& f, Args&&... args)
{
    ++g_callCount;
    return std::forward<Func>(f)(std::forward<Args>(args)...);
}

// ----------------------------------------------------------------------------
// 【日常實務範例 2】設定表的存取器 — 回傳參考才能就地修改
//   情境：服務的執行期設定放在一張表裡，取用端要能「讀」也能「就地改」。
//   為什麼用到本主題：
//     存取器必須回傳 std::string&（參考），呼叫端才改得到原表。
//     若寫成 auto，回傳的是複本，setting("host") = "..." 會改到複本上，
//     編譯得過、執行不報錯，但設定永遠沒生效 —— 典型的靜默 bug。
// ----------------------------------------------------------------------------
class RuntimeConfig
{
    std::map<std::string, std::string> m_values;

public:
    RuntimeConfig()
    {
        m_values["host"]    = "127.0.0.1";
        m_values["port"]    = "8080";
        m_values["logging"] = "info";
    }

    // decltype(auto) 推導出 std::string&（因為 operator[] 回傳參考 → lvalue → T&）
    decltype(auto) at(const std::string& key)
    {
        return m_values[key];          // 沒有括號：轉發 operator[] 的參考
    }

    // 對照組：用 auto → 推導成 std::string（複本），呼叫端改不到原表
    auto at_WRONG(const std::string& key)
    {
        return m_values[key];
    }

    void dump(const char* label) const
    {
        std::cout << "  " << label << " → ";
        for (std::map<std::string, std::string>::const_iterator it = m_values.begin();
             it != m_values.end(); ++it)
            std::cout << it->first << "=" << it->second << " ";
        std::cout << "\n";
    }
};


// ============================================================================
// 【主程式】依序展示所有概念
// ============================================================================
int main()
{
    std::cout << std::boolalpha;

    // ========================================================================
    // 【範例 1】基本變數推導比較：auto vs decltype(auto)
    // ========================================================================
    std::cout << "===== 1. 基本變數推導比較 =====\n";

    int x = 10;
    const int cx = 20;       // cx 的宣告型別是 const int
    int& rx = x;             // rx 的宣告型別是 int&
    const int& crx = x;     // crx 的宣告型別是 const int&

    // --- auto 推導（模板引數推導規則）---
    // auto 會去除頂層 const 和參考，產生全新的獨立副本
    auto a1 = x;     // int        （複製 x 的值）
    auto a2 = cx;    // int        （忽略 const，複製 cx 的值）
    auto a3 = rx;    // int        （忽略參考，複製 rx 所參考的值）
    auto a4 = crx;   // int        （忽略 const 和參考，複製）

    // --- decltype(auto) 推導（decltype 規則）---
    // decltype(auto) 保留初始化表達式的完整型別，包含 const 和參考
    decltype(auto) d1 = x;     // int        （x 是識別符號，宣告型別 int）
    decltype(auto) d2 = cx;    // const int  （cx 是識別符號，宣告型別 const int）
    decltype(auto) d3 = rx;    // int&       （rx 是識別符號，宣告型別 int&）
    decltype(auto) d4 = crx;   // const int& （crx 是識別符號，宣告型別 const int&）

    // 驗證 auto 是否保留 const
    std::cout << "auto a2 = cx (const int):\n";
    std::cout << "  a2 is const: "
              << std::is_const<decltype(a2)>::value << "\n";
    // 輸出: false — auto 去除了 const

    // 驗證 decltype(auto) 是否保留 const
    std::cout << "decltype(auto) d2 = cx:\n";
    std::cout << "  d2 is const: "
              << std::is_const<decltype(d2)>::value << "\n";
    // 輸出: true — decltype(auto) 保留了 const

    // 驗證 auto 是否保留參考
    std::cout << "\nauto a3 = rx (int&):\n";
    std::cout << "  a3 is reference: "
              << std::is_reference<decltype(a3)>::value << "\n";
    // 輸出: false — auto 去除了參考

    // 驗證 decltype(auto) 是否保留參考
    std::cout << "decltype(auto) d3 = rx:\n";
    std::cout << "  d3 is reference: "
              << std::is_reference<decltype(d3)>::value << "\n";
    // 輸出: true — decltype(auto) 保留了參考
    std::cout << "\n";

    // 抑制未使用變數警告
    (void)a1; (void)a4; (void)d1; (void)d4;

    // ========================================================================
    // 【範例 2】驗證參考保留 — 修改是否影響原始值
    // ========================================================================
    std::cout << "===== 2. 驗證參考保留 =====\n";

    int value = 100;
    int& refValue = value;

    // auto 版本：產生副本，修改不影響 value
    auto autoVar = refValue;
    (void)autoVar;  // 僅為展示推導結果而宣告
    // decltype(auto) 版本：保留參考，修改會影響 value
    decltype(auto) decltypeAutoVar = refValue;

    autoVar = 999;
    std::cout << "修改 autoVar = 999 後:\n";
    std::cout << "  value = " << value << " (不變，因為 autoVar 是複製)\n";
    // 輸出: value = 100

    decltypeAutoVar = 888;
    std::cout << "修改 decltypeAutoVar = 888 後:\n";
    std::cout << "  value = " << value << " (改變，因為 decltypeAutoVar 是參考)\n\n";
    // 輸出: value = 888

    // ========================================================================
    // 【範例 3】括號的影響（極重要！最常見的陷阱）
    // ========================================================================
    // 關鍵觀念：
    //   decltype(num)   — num 是「不帶括號的識別符號」，得到宣告型別 int
    //   decltype((num)) — (num) 是「帶括號的左值表達式」，得到 int&
    //
    // 這個差異在 decltype(auto) 中會直接影響推導結果：
    //   decltype(auto) da1 = num;   → 等同 decltype(num)   → int
    //   decltype(auto) da2 = (num); → 等同 decltype((num)) → int&
    std::cout << "===== 3. 括號的影響 (極重要！) =====\n";

    int num = 42;

    decltype(auto) da1 = num;    // int  （識別符號 → 宣告型別）
    decltype(auto) da2 = (num);  // int& （左值表達式 → T&）

    std::cout << "decltype(auto) da1 = num:\n";
    std::cout << "  is_reference: "
              << std::is_reference<decltype(da1)>::value << "\n";
    // 輸出: false

    std::cout << "decltype(auto) da2 = (num):\n";
    std::cout << "  is_reference: "
              << std::is_reference<decltype(da2)>::value << "\n";
    // 輸出: true — 加了括號就變成參考！

    da2 = 123;
    std::cout << "修改 da2 = 123 後, num = " << num << "\n\n";
    // 輸出: num = 123 — 透過參考修改了 num

    // ========================================================================
    // 【範例 4】函式回傳型別比較：auto vs decltype(auto)
    // ========================================================================
    std::cout << "===== 4. 函式回傳型別比較 =====\n";

    // auto 回傳型別測試
    std::cout << "autoReturn() 回傳型別:\n";
    printTypeInfo<decltype(autoReturn())>("  autoReturn()");
    // 結果: 非參考、非 const — 因為 auto 將 int 原樣回傳

    std::cout << "autoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(autoReturnRef())>("  autoReturnRef()");
    // 結果: 非參考、非 const — 即使內部 getReference() 回傳 int&，
    //        auto 也會去除參考，變成回傳 int（複製）

    // decltype(auto) 回傳型別測試
    std::cout << "decltypeAutoReturn() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturn())>("  decltypeAutoReturn()");
    // 結果: 非參考 — 因為 return globalValue 中 globalValue 是識別符號，
    //        decltype(globalValue) = int

    std::cout << "decltypeAutoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturnRef())>("  decltypeAutoReturnRef()");
    // 結果: 是左值參考 — 完整保留了 getReference() 的 int& 回傳型別

    std::cout << "decltypeAutoReturnConstRef() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturnConstRef())>("  decltypeAutoReturnConstRef()");
    // 結果: 是左值參考 + const — 完整保留了 const int&

    // ========================================================================
    // 【範例 5】透過 decltype(auto) 回傳的參考修改全域變數
    // ========================================================================
    std::cout << "===== 5. 透過回傳的參考修改全域變數 =====\n";

    std::cout << "globalValue 原始值: " << globalValue << "\n";

    // decltypeAutoReturnRef() 回傳 int&（參考 globalValue）
    // 因此可以直接對回傳值賦值，等同修改 globalValue
    decltypeAutoReturnRef() = 500;

    std::cout << "執行 decltypeAutoReturnRef() = 500 後:\n";
    std::cout << "globalValue 新值: " << globalValue << "\n\n";
    // 輸出: 500 — 成功透過回傳的參考修改了全域變數

    // ========================================================================
    // 【範例 6】泛型轉發比較 — decltype(auto) 的殺手級應用
    // ========================================================================
    // 這是 decltype(auto) 最重要的實際應用場景之一：
    // 撰寫泛型包裝函式時，需要完美保留被包裝函式的回傳型別。
    std::cout << "===== 6. 泛型轉發比較 =====\n";

    globalValue = 100;  // 重設

    // 建立一個回傳 int& 的 lambda
    auto refLambda = []() -> int& { return globalValue; };

    // --- 使用 auto 轉發（會丟失參考）---
    auto result1 = callWithAuto(refLambda);
    (void)result1;  // 僅為展示推導結果而宣告
    // callWithAuto 的回傳型別是 auto → int（衰退）
    // result1 是 int，持有的是 globalValue 的副本
    result1 = 777;
    std::cout << "callWithAuto 後修改 result1 = 777:\n";
    std::cout << "  globalValue = " << globalValue << " (不變)\n";
    // 輸出: 100 — 因為 result1 只是副本

    // --- 使用 decltype(auto) 轉發（保留參考）---
    decltype(auto) result2 = callWithDecltypeAuto(refLambda);
    // callWithDecltypeAuto 的回傳型別是 decltype(auto) → int&（保留）
    // result2 是 int&，直接參考 globalValue
    result2 = 888;
    std::cout << "callWithDecltypeAuto 後修改 result2 = 888:\n";
    std::cout << "  globalValue = " << globalValue << " (改變！)\n\n";
    // 輸出: 888 — 因為 result2 是 globalValue 的參考

    // ========================================================================
    // 【範例 7】容器元素存取
    // ========================================================================
    // std::vector<int>::operator[] 回傳 int&
    // 使用 auto 會得到副本，使用 decltype(auto) 會得到參考
    std::cout << "===== 7. 容器元素存取 =====\n";

    std::vector<int> vec = {1, 2, 3, 4, 5};

    auto elem1 = vec[0];             // int （複製）
    (void)elem1;  // 僅為展示推導結果而宣告
    decltype(auto) elem2 = vec[1];   // int& （參考容器內的元素！）

    elem1 = 100;
    std::cout << "修改 elem1 = 100 後, vec[0] = " << vec[0] << " (不變)\n";
    // 輸出: vec[0] = 1

    elem2 = 200;
    std::cout << "修改 elem2 = 200 後, vec[1] = " << vec[1] << " (改變！)\n\n";
    // 輸出: vec[1] = 200 — 透過參考直接修改了容器內容

    // ========================================================================
    // 【範例 8】表達式的推導 — 值類別決定推導結果
    // ========================================================================
    // decltype(auto) 的推導結果取決於初始化表達式的值類別：
    //   - 純右值 (prvalue)：推導為 T     （例如 a + b 的結果）
    //   - 左值 (lvalue)：推導為 T&        （例如賦值運算的回傳值）
    //   - 將亡值 (xvalue)：推導為 T&&     （例如 std::move(x) 的結果）
    std::cout << "===== 8. 表達式的推導 =====\n";

    int a = 5, b = 10;

    // a + b 是純右值 (prvalue) → decltype(a + b) = int
    decltype(auto) sum = a + b;
    std::cout << "decltype(auto) sum = a + b:\n";
    std::cout << "  is_reference: "
              << std::is_reference<decltype(sum)>::value << "\n";
    // 輸出: false — 純右值推導為 int

    // (a = b) 中，賦值運算子回傳左值參考 → decltype((a = b)) = int&
    decltype(auto) assign = (a = b);
    std::cout << "decltype(auto) assign = (a = b):\n";
    std::cout << "  is_reference: "
              << std::is_reference<decltype(assign)>::value << "\n";
    // 輸出: true — 賦值表達式是左值，推導為 int&

    assign = 999;
    std::cout << "修改 assign = 999 後, a = " << a << "\n\n";
    // 輸出: a = 999 — assign 是 a 的參考

    // ========================================================================
    // 【範例 9】decltype(auto) 推導總結表
    // ========================================================================
    std::cout << "===== 9. 推導規則總結 =====\n";
    std::cout << "+---------------------------+----------------+----------------+\n";
    std::cout << "| 初始化表達式              | auto 推導      | decltype(auto) |\n";
    std::cout << "+---------------------------+----------------+----------------+\n";
    std::cout << "| int x = 10;              |                |                |\n";
    std::cout << "|   = x                     | int            | int            |\n";
    std::cout << "| const int cx = 20;       |                |                |\n";
    std::cout << "|   = cx                    | int            | const int      |\n";
    std::cout << "| int& rx = x;             |                |                |\n";
    std::cout << "|   = rx                    | int            | int&           |\n";
    std::cout << "| const int& crx = x;     |                |                |\n";
    std::cout << "|   = crx                   | int            | const int&     |\n";
    std::cout << "|   = (x)                   | int            | int&           |\n";
    std::cout << "|   = x + y                 | int            | int            |\n";
    std::cout << "|   = (x = y)               | int            | int&           |\n";
    std::cout << "+---------------------------+----------------+----------------+\n\n";

    // ========================================================================
    // 【範例 10】常見陷阱與最佳實踐
    // ========================================================================
    std::cout << "===== 10. 常見陷阱與最佳實踐 =====\n";
    std::cout << "\n";
    std::cout << "【陷阱 1】括號會改變推導結果\n";
    std::cout << "  decltype(auto) r1 = var;   // T   (識別符號)\n";
    std::cout << "  decltype(auto) r2 = (var); // T&  (左值表達式)\n";
    std::cout << "  結論: 使用 decltype(auto) 時，不要隨意加括號！\n\n";

    std::cout << "【陷阱 2】函式回傳時加括號會回傳區域變數的參考\n";
    std::cout << "  decltype(auto) bad() {\n";
    std::cout << "      int local = 42;\n";
    std::cout << "      return (local); // 回傳 int& → 懸空參考！未定義行為！\n";
    std::cout << "  }\n";
    std::cout << "  結論: return 語句中絕對不要加括號包裹區域變數！\n\n";

    std::cout << "【最佳實踐 1】泛型轉發函式應使用 decltype(auto)\n";
    std::cout << "  template<typename F>\n";
    std::cout << "  decltype(auto) wrapper(F&& f) { return f(); }\n";
    std::cout << "  理由: 完美保留被包裝函式的回傳型別\n\n";

    std::cout << "【最佳實踐 2】需要保留 const/參考特性時使用 decltype(auto)\n";
    std::cout << "  理由: auto 會衰退掉 const 和參考，decltype(auto) 不會\n\n";

    std::cout << "【最佳實踐 3】只想要值語意時，使用 auto 即可\n";
    std::cout << "  理由: auto 更簡潔，且不用擔心意外產生參考\n\n";

    // ========================================================================
    // 【範例 11】日常實務 1：計次包裝器保留參考語意
    // ========================================================================
    std::cout << "===== 11. 日常實務：計次包裝器（轉發回傳型別）=====\n";

    std::vector<int> scores{10, 20, 30};

    // 這個 lambda 回傳 int&（容器元素的參考）
    auto elemAt = [&scores](std::size_t i) -> int& { return scores[i]; };

    // decltype(auto) 版本：回傳型別仍是 int& → 是 lvalue，可以直接賦值
    invokeCounted(elemAt, 0) = 999;

    // auto 版本：回傳型別被剝成 int（prvalue 複本）
    //   注意：這裡「不能」寫 invokeCounted_WRONG(elemAt, 1) = 888;
    //   因為 auto 剝掉參考後回傳的是 prvalue，不是 lvalue，無法作為賦值左運算元。
    //   實測 GCC 15.2 的錯誤訊息：
    //       error: lvalue required as left operand of assignment
    //   只能先接成變數再改 —— 而那只是改複本，原容器不受影響：
    int copyOnly = invokeCounted_WRONG(elemAt, 1);
    copyOnly = 888;                       // 改的是 copyOnly，不是 scores[1]

    std::cout << "  decltype(auto) 版本改 scores[0] → " << scores[0]
              << "（成功寫入原容器）\n";
    std::cout << "  auto 版本取得 scores[1] 的複本   → 複本改成 " << copyOnly
              << "，但原容器仍是 " << scores[1] << "\n";
    std::cout << "  包裝器共被呼叫 " << g_callCount << " 次\n";
    std::cout << "  結論：auto 把 int& 剝成 int。\n";
    std::cout << "        本例中「賦值給回傳值」會直接編譯失敗（算是幸運，錯誤很明顯）；\n";
    std::cout << "        但只要接成變數再改，就會變成靜默的邏輯錯誤 —— 改到複本上。\n\n";

    // ========================================================================
    // 【範例 12】日常實務 2：設定表存取器
    // ========================================================================
    std::cout << "===== 12. 日常實務：設定表存取器 =====\n";

    RuntimeConfig cfg;
    cfg.dump("初始設定");

    cfg.at("host") = "10.0.0.7";          // decltype(auto) → string&，改得到
    cfg.at_WRONG("port") = "9999";        // auto → string 複本，改不到

    cfg.dump("修改後  ");
    std::cout << "  host 已變更（decltype(auto) 回傳參考）\n";
    std::cout << "  port 沒變（auto 回傳複本，賦值寫在複本上就被丟棄了）\n\n";

    return 0;
}

// 編譯: g++ -std=c++14 -Wall -Wextra "summary.cpp" -o dta_summary

// === 預期輸出 ===
// ===== 1. 基本變數推導比較 =====
// auto a2 = cx (const int):
//   a2 is const: false
// decltype(auto) d2 = cx:
//   d2 is const: true
//
// auto a3 = rx (int&):
//   a3 is reference: false
// decltype(auto) d3 = rx:
//   d3 is reference: true
//
// ===== 2. 驗證參考保留 =====
// 修改 autoVar = 999 後:
//   value = 100 (不變，因為 autoVar 是複製)
// 修改 decltypeAutoVar = 888 後:
//   value = 888 (改變，因為 decltypeAutoVar 是參考)
//
// ===== 3. 括號的影響 (極重要！) =====
// decltype(auto) da1 = num:
//   is_reference: false
// decltype(auto) da2 = (num):
//   is_reference: true
// 修改 da2 = 123 後, num = 123
//
// ===== 4. 函式回傳型別比較 =====
// autoReturn() 回傳型別:
//   autoReturn():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// autoReturnRef() 回傳型別:
//   autoReturnRef():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// decltypeAutoReturn() 回傳型別:
//   decltypeAutoReturn():
//   is_reference:     false
//   is_const:         false
//   is_lvalue_ref:    false
//   is_rvalue_ref:    false
//
// decltypeAutoReturnRef() 回傳型別:
//   decltypeAutoReturnRef():
//   is_reference:     true
//   is_const:         false
//   is_lvalue_ref:    true
//   is_rvalue_ref:    false
//
// decltypeAutoReturnConstRef() 回傳型別:
//   decltypeAutoReturnConstRef():
//   is_reference:     true
//   is_const:         true
//   is_lvalue_ref:    true
//   is_rvalue_ref:    false
//
// ===== 5. 透過回傳的參考修改全域變數 =====
// globalValue 原始值: 100
// 執行 decltypeAutoReturnRef() = 500 後:
// globalValue 新值: 500
//
// ===== 6. 泛型轉發比較 =====
// callWithAuto 後修改 result1 = 777:
//   globalValue = 100 (不變)
// callWithDecltypeAuto 後修改 result2 = 888:
//   globalValue = 888 (改變！)
//
// ===== 7. 容器元素存取 =====
// 修改 elem1 = 100 後, vec[0] = 1 (不變)
// 修改 elem2 = 200 後, vec[1] = 200 (改變！)
//
// ===== 8. 表達式的推導 =====
// decltype(auto) sum = a + b:
//   is_reference: false
// decltype(auto) assign = (a = b):
//   is_reference: true
// 修改 assign = 999 後, a = 999
//
// ===== 9. 推導規則總結 =====
// +---------------------------+----------------+----------------+
// | 初始化表達式              | auto 推導      | decltype(auto) |
// +---------------------------+----------------+----------------+
// | int x = 10;              |                |                |
// |   = x                     | int            | int            |
// | const int cx = 20;       |                |                |
// |   = cx                    | int            | const int      |
// | int& rx = x;             |                |                |
// |   = rx                    | int            | int&           |
// | const int& crx = x;     |                |                |
// |   = crx                   | int            | const int&     |
// |   = (x)                   | int            | int&           |
// |   = x + y                 | int            | int            |
// |   = (x = y)               | int            | int&           |
// +---------------------------+----------------+----------------+
//
// ===== 10. 常見陷阱與最佳實踐 =====
//
// 【陷阱 1】括號會改變推導結果
//   decltype(auto) r1 = var;   // T   (識別符號)
//   decltype(auto) r2 = (var); // T&  (左值表達式)
//   結論: 使用 decltype(auto) 時，不要隨意加括號！
//
// 【陷阱 2】函式回傳時加括號會回傳區域變數的參考
//   decltype(auto) bad() {
//       int local = 42;
//       return (local); // 回傳 int& → 懸空參考！未定義行為！
//   }
//   結論: return 語句中絕對不要加括號包裹區域變數！
//
// 【最佳實踐 1】泛型轉發函式應使用 decltype(auto)
//   template<typename F>
//   decltype(auto) wrapper(F&& f) { return f(); }
//   理由: 完美保留被包裝函式的回傳型別
//
// 【最佳實踐 2】需要保留 const/參考特性時使用 decltype(auto)
//   理由: auto 會衰退掉 const 和參考，decltype(auto) 不會
//
// 【最佳實踐 3】只想要值語意時，使用 auto 即可
//   理由: auto 更簡潔，且不用擔心意外產生參考
//
// ===== 11. 日常實務：計次包裝器（轉發回傳型別）=====
//   decltype(auto) 版本改 scores[0] → 999（成功寫入原容器）
//   auto 版本取得 scores[1] 的複本   → 複本改成 888，但原容器仍是 20
//   包裝器共被呼叫 2 次
//   結論：auto 把 int& 剝成 int。
//         本例中「賦值給回傳值」會直接編譯失敗（算是幸運，錯誤很明顯）；
//         但只要接成變數再改，就會變成靜默的邏輯錯誤 —— 改到複本上。
//
// ===== 12. 日常實務：設定表存取器 =====
//   初始設定 → host=127.0.0.1 logging=info port=8080 
//   修改後   → host=10.0.0.7 logging=info port=8080 
//   host 已變更（decltype(auto) 回傳參考）
//   port 沒變（auto 回傳複本，賦值寫在複本上就被丟棄了）
