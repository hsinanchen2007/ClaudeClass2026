// ============================================================
// 第 2.1 章 總結：左值與右值的定義 — Value Categories 完整解析
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【C++ 值類別三分法】
//
//        expression
//       /          \
//    glvalue      rvalue
//    /    \      /    \
// lvalue  xvalue    prvalue
//
// 【lvalue 左值】有身份（identity），不可移動
//   - 有名字的變數：int x; → x 是 lvalue
//   - 引用：int& ref = x; → ref 是 lvalue
//   - 陣列元素：arr[0]
//   - 解引用：*ptr
//   - 字串字面值："Hello" → 是 lvalue！（存在靜態儲存區）
//   - 前置遞增：++x（回傳引用 → lvalue）
//   - 回傳引用的函式呼叫：get_ref(x)
//   → 可以取位址（&x 合法）
//
// 【prvalue 純右值】沒有身份，不可移動（將要移動）
//   - 非字串的字面值：42, 3.14, true, nullptr
//   - 算術運算結果：x + y, x > 0
//   - 回傳非引用型別的函式呼叫：func()
//   - 型別轉換：int(42), static_cast<double>(x)
//   - Lambda 表達式：[](int a){ return a; }
//   - 後置遞增：x++（回傳舊值的副本 → prvalue）
//   → 不能取位址
//
// 【xvalue 將亡值】有身份，且可被移動
//   - std::move(x) → 將 lvalue 轉為 xvalue
//   → 告訴編譯器「這個物件可以被移動」
//
// 【判斷規則】
//   有名字 → lvalue    沒名字 → rvalue
//   可取址 → lvalue    不可取址 → rvalue
//   std::move(x) → xvalue（有身份 + 可移動）
// ============================================================

#include <iostream>
#include <string>
#include <utility>
#include <type_traits>

// ============================================================
// 值類別判定工具
// ============================================================
template<typename T> struct value_category         { static constexpr const char* value = "prvalue"; };
template<typename T> struct value_category<T&>     { static constexpr const char* value = "lvalue"; };
template<typename T> struct value_category<T&&>    { static constexpr const char* value = "xvalue"; };

#define PRINT_CATEGORY(expr) \
    std::cout << "  " << #expr << " → " \
              << value_category<decltype((expr))>::value << "\n"

// 測試用函數
std::string make_string()                  { return "temp"; }
std::string& get_ref(std::string& s)       { return s; }

int main() {
    // ============================================================
    // 1. Lvalue 範例
    // ============================================================
    std::cout << "===== 1. Lvalue（有名字、可取址）=====\n";
    int x = 42;
    int& ref = x;
    int arr[3] = {1, 2, 3};

    PRINT_CATEGORY(x);        // lvalue
    PRINT_CATEGORY(ref);      // lvalue
    PRINT_CATEGORY(arr[0]);   // lvalue

    // 驗證：lvalue 可以取位址
    std::cout << "  &x     = " << &x << "\n";
    std::cout << "  &ref   = " << &ref << " （和 &x 相同）\n";
    std::cout << "  &arr[0]= " << &arr[0] << "\n\n";

    // ============================================================
    // 2. Prvalue 範例
    // ============================================================
    std::cout << "===== 2. Prvalue（臨時值、不可取址）=====\n";
    PRINT_CATEGORY(42);               // prvalue — 整數字面值
    PRINT_CATEGORY(x + 1);            // prvalue — 算術運算結果
    PRINT_CATEGORY(x > 0);            // prvalue — 比較結果
    PRINT_CATEGORY(make_string());     // prvalue — 回傳非引用
    PRINT_CATEGORY(static_cast<double>(x)); // prvalue — 型別轉換
    // &42;  // ❌ 編譯錯誤！prvalue 不可取址
    std::cout << "\n";

    // 特別注意：前置 vs 後置遞增
    std::cout << "===== 前置 vs 後置遞增 =====\n";
    int y = 10;
    PRINT_CATEGORY(++y);   // lvalue（回傳引用）
    PRINT_CATEGORY(y++);   // prvalue（回傳舊值副本）
    std::cout << "\n";

    // ============================================================
    // 3. Xvalue 範例
    // ============================================================
    std::cout << "===== 3. Xvalue（std::move 產生）=====\n";
    std::string s = "Hello";
    PRINT_CATEGORY(std::move(x));    // xvalue
    PRINT_CATEGORY(std::move(s));    // xvalue
    std::cout << "\n";

    // ============================================================
    // 4. 函式回傳值的值類別
    // ============================================================
    std::cout << "===== 4. 函式回傳值的值類別 =====\n";
    std::string str = "Hello";
    PRINT_CATEGORY(make_string());    // prvalue（回傳值）
    PRINT_CATEGORY(get_ref(str));     // lvalue（回傳引用）
    std::cout << "\n";

    // ============================================================
    // 5. 值類別對移動的影響
    // ============================================================
    std::cout << "===== 5. 值類別與移動 =====\n";
    {
        std::string original = "This is a long string on the heap";

        // 傳入 lvalue → 觸發複製
        std::cout << "  傳入 lvalue（複製）：\n";
        std::string copy = original;
        std::cout << "    original 仍然有效：\"" << original << "\"\n";

        // 傳入 xvalue（std::move）→ 觸發移動
        std::cout << "  傳入 xvalue（移動）：\n";
        std::string moved = std::move(original);
        std::cout << "    original 被掏空：\"" << original << "\"\n";
        std::cout << "    moved = \"" << moved << "\"\n";
    }

    // ============================================================
    // 重點整理
    // ============================================================
    std::cout << "\n=== 值類別速查 ===\n";
    std::cout << "  lvalue：有名字、可取址（變數、引用、*ptr、arr[i]）\n";
    std::cout << "  prvalue：臨時值（42、x+y、func()、cast）\n";
    std::cout << "  xvalue：std::move(x)（有身份但可被移動）\n";
    std::cout << "  字串字面值 \"hello\" 是 lvalue（特殊！）\n";
    std::cout << "  ++x 是 lvalue，x++ 是 prvalue\n";

    return 0;
}
