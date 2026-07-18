// 檔案名稱: decltype_auto_demo.cpp
// 編譯指令: g++ -std=c++14 -Wall -o decltype_auto_demo decltype_auto_demo.cpp
// 說明: 展示 C++14 decltype(auto) 的用法與特性

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>

// ===== 輔助函式：印出型別特性 =====
template<typename T>
void printTypeInfo(const char* name)
{
    std::cout << name << ":\n";
    std::cout << "  is_reference:     " << std::is_reference<T>::value << "\n";
    std::cout << "  is_const:         " 
              << std::is_const<typename std::remove_reference<T>::type>::value << "\n";
    std::cout << "  is_lvalue_ref:    " << std::is_lvalue_reference<T>::value << "\n";
    std::cout << "  is_rvalue_ref:    " << std::is_rvalue_reference<T>::value << "\n";
    std::cout << "\n";
}

// ===== 展示回傳型別的函式 =====

// 全域變數，用於回傳參考
int globalValue = 100;
std::vector<int> globalVec = {10, 20, 30, 40, 50};

// 回傳值
int getValue()
{
    return 42;
}

// 回傳左值參考
int& getReference()
{
    return globalValue;
}

// 回傳 const 左值參考
const int& getConstReference()
{
    static int value = 200;
    return value;
}

// ===== 使用 auto 作為回傳型別 =====
auto autoReturn()
{
    return globalValue;  // 回傳 int (複製)
}

auto autoReturnRef()
{
    return getReference();  // 回傳 int (仍是複製！auto 忽略參考)
}

// ===== 使用 decltype(auto) 作為回傳型別 =====
decltype(auto) decltypeAutoReturn()
{
    return globalValue;  // 回傳 int (globalValue 是識別符號)
}

decltype(auto) decltypeAutoReturnRef()
{
    return getReference();  // 回傳 int& (保留參考！)
}

decltype(auto) decltypeAutoReturnConstRef()
{
    return getConstReference();  // 回傳 const int&
}

// ===== 泛型轉發函式 =====

// 使用 auto：會丟失參考
template<typename F>
auto callWithAuto(F&& f)
{
    return f();  // 回傳值型別，即使 f() 回傳參考
}

// 使用 decltype(auto)：完美保留回傳型別
template<typename F>
decltype(auto) callWithDecltypeAuto(F&& f)
{
    return f();  // 完整保留 f() 的回傳型別
}

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本變數推導比較 =====
    std::cout << "===== 1. 基本變數推導比較 =====\n";
    
    int x = 10;
    const int cx = 20;
    int& rx = x;
    const int& crx = x;
    
    // auto 推導
    auto a1 = x;     // int
    auto a2 = cx;    // int (忽略 const)
    auto a3 = rx;    // int (忽略參考)
    auto a4 = crx;   // int (忽略 const 和參考)
    
    // decltype(auto) 推導
    decltype(auto) d1 = x;     // int
    decltype(auto) d2 = cx;    // const int
    decltype(auto) d3 = rx;    // int&
    decltype(auto) d4 = crx;   // const int&
    
    std::cout << "auto a2 = cx (const int):\n";
    std::cout << "  a2 is const: " << std::is_const<decltype(a2)>::value << "\n";
    
    std::cout << "decltype(auto) d2 = cx:\n";
    std::cout << "  d2 is const: " << std::is_const<decltype(d2)>::value << "\n";
    
    std::cout << "\nauto a3 = rx (int&):\n";
    std::cout << "  a3 is reference: " << std::is_reference<decltype(a3)>::value << "\n";
    
    std::cout << "decltype(auto) d3 = rx:\n";
    std::cout << "  d3 is reference: " << std::is_reference<decltype(d3)>::value << "\n";
    std::cout << "\n";
    
    // ===== 2. 驗證參考保留 =====
    std::cout << "===== 2. 驗證參考保留 =====\n";
    
    int value = 100;
    int& refValue = value;
    
    auto autoVar = refValue;
    decltype(auto) decltypeAutoVar = refValue;
    
    // 修改看看是否影響原值
    autoVar = 999;
    std::cout << "修改 autoVar = 999 後:\n";
    std::cout << "  value = " << value << " (不變，因為是複製)\n";
    
    decltypeAutoVar = 888;
    std::cout << "修改 decltypeAutoVar = 888 後:\n";
    std::cout << "  value = " << value << " (改變，因為是參考)\n\n";
    
    // ===== 3. 括號的影響 (極重要！) =====
    std::cout << "===== 3. 括號的影響 (極重要！) =====\n";
    
    int num = 42;
    
    decltype(auto) da1 = num;    // int   (識別符號)
    decltype(auto) da2 = (num);  // int&  (左值表達式！)
    
    std::cout << "decltype(auto) da1 = num:\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(da1)>::value << "\n";
    
    std::cout << "decltype(auto) da2 = (num):\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(da2)>::value << "\n";
    
    da2 = 123;
    std::cout << "修改 da2 = 123 後, num = " << num << "\n\n";
    
    // ===== 4. 函式回傳型別比較 =====
    std::cout << "===== 4. 函式回傳型別比較 =====\n";
    
    // 測試 auto 回傳
    std::cout << "autoReturn() 回傳型別:\n";
    printTypeInfo<decltype(autoReturn())>("  autoReturn()");
    
    std::cout << "autoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(autoReturnRef())>("  autoReturnRef()");
    
    // 測試 decltype(auto) 回傳
    std::cout << "decltypeAutoReturn() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturn())>("  decltypeAutoReturn()");
    
    std::cout << "decltypeAutoReturnRef() 回傳型別:\n";
    printTypeInfo<decltype(decltypeAutoReturnRef())>("  decltypeAutoReturnRef()");
    
    // ===== 5. 透過回傳的參考修改全域變數 =====
    std::cout << "===== 5. 透過回傳的參考修改全域變數 =====\n";
    
    std::cout << "globalValue 原始值: " << globalValue << "\n";
    
    decltypeAutoReturnRef() = 500;  // 直接修改全域變數！
    
    std::cout << "執行 decltypeAutoReturnRef() = 500 後:\n";
    std::cout << "globalValue 新值: " << globalValue << "\n\n";
    
    // ===== 6. 泛型轉發比較 =====
    std::cout << "===== 6. 泛型轉發比較 =====\n";
    
    globalValue = 100;  // 重設
    
    // Lambda 回傳參考
    auto refLambda = []() -> int& { return globalValue; };
    
    // 使用 auto 轉發（會丟失參考）
    auto result1 = callWithAuto(refLambda);
    result1 = 777;
    std::cout << "callWithAuto 後修改 result1 = 777:\n";
    std::cout << "  globalValue = " << globalValue << " (不變)\n";
    
    // 使用 decltype(auto) 轉發（保留參考）
    decltype(auto) result2 = callWithDecltypeAuto(refLambda);
    result2 = 888;
    std::cout << "callWithDecltypeAuto 後修改 result2 = 888:\n";
    std::cout << "  globalValue = " << globalValue << " (改變！)\n\n";
    
    // ===== 7. 容器元素存取 =====
    std::cout << "===== 7. 容器元素存取 =====\n";
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    auto elem1 = vec[0];             // int (複製)
    decltype(auto) elem2 = vec[1];   // int& (參考！)
    
    elem1 = 100;
    std::cout << "修改 elem1 = 100 後, vec[0] = " << vec[0] << " (不變)\n";
    
    elem2 = 200;
    std::cout << "修改 elem2 = 200 後, vec[1] = " << vec[1] << " (改變！)\n\n";
    
    // ===== 8. 表達式的推導 =====
    std::cout << "===== 8. 表達式的推導 =====\n";
    
    int a = 5, b = 10;
    
    decltype(auto) sum = a + b;       // int (prvalue)
    decltype(auto) assign = (a = b);  // int& (賦值回傳左值參考)
    
    std::cout << "decltype(auto) sum = a + b:\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(sum)>::value << "\n";
    
    std::cout << "decltype(auto) assign = (a = b):\n";
    std::cout << "  is_reference: " << std::is_reference<decltype(assign)>::value << "\n";
    
    assign = 999;
    std::cout << "修改 assign = 999 後, a = " << a << "\n";
    
    return 0;
}
