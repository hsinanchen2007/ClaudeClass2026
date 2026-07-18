// ============================================================================
// 總結檔案：decltype(auto) — 完美保留型別的自動推導 (C++14)
// 編譯指令: g++ -std=c++14 -Wall -o summary summary.cpp
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
#include <utility>  // std::move

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

    return 0;
}
