// =============================================================================
// 第 1.2 章：decltype — 查詢表達式的型別（綜合複習）
// =============================================================================
// 編譯指令: g++ -std=c++14 -Wall -o summary summary.cpp
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
    decltype(rx) r1 = x;      // int&（保留參考）
    decltype(crx) cr1 = x;    // const int&（同時保留 const 和參考）

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
    decltype(value) declVal = 100;   // const int（decltype 保留 const）

    auto       autoRef = ref;        // int（auto 忽略參考，得到獨立的複製）
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
    decltype(i += j) ref2 = i;     // int&（lvalue → T&）

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
    decltype(getReference()) val2 = x;          // int&
    decltype(getConstReference()) val3 = x;     // const int&

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
    decltype(pt.y) py = 3.14;      // double
    decltype((pt.x)) prx = pt.x;   // int&（加括號 → 左值表達式 → 參考）

    prx = 999;
    std::cout << "修改 prx 後, pt.x = " << pt.x << " (decltype((pt.x)) 是 int&)\n";

    // 不需要物件實體，直接用類別名稱推導成員型別
    decltype(Point::x) memberX = 0;   // int
    decltype(Point::y) memberY = 0.0; // double
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

    std::cout << "===== 複習完成 =====\n";

    return 0;
}
