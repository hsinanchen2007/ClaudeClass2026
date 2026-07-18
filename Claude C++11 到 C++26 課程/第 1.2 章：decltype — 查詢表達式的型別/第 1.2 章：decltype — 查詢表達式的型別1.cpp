// 檔案名稱: decltype_demo.cpp
// 編譯指令: g++ -std=c++11 -Wall -o decltype_demo decltype_demo.cpp
// 說明: 展示 C++11 decltype 的各種用法與推導規則

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>

// 輔助巨集：印出型別資訊
// 使用 type_traits 來檢查型別特性
#define PRINT_TYPE_INFO(expr) \
    std::cout << #expr << ":\n"; \
    std::cout << "  is_reference: " << std::is_reference<decltype(expr)>::value << "\n"; \
    std::cout << "  is_const: " << std::is_const<typename std::remove_reference<decltype(expr)>::type>::value << "\n\n";

// 用於展示函式回傳型別推導
int getValue()
{
    return 42;
}

int& getReference()
{
    static int value = 100;
    return value;
}

const int& getConstReference()
{
    static int value = 200;
    return value;
}

int main()
{
    // ===== 1. 基本用法：從變數推導型別 =====
    std::cout << "===== 1. 基本用法：從變數推導型別 =====\n";
    
    int x = 10;
    double y = 3.14;
    std::string str = "Hello";
    
    decltype(x) a = 20;        // int
    decltype(y) b = 2.718;     // double
    decltype(str) s = "World"; // std::string
    
    std::cout << "a = " << a << " (與 x 同型別: int)\n";
    std::cout << "b = " << b << " (與 y 同型別: double)\n";
    std::cout << "s = " << s << " (與 str 同型別: std::string)\n\n";
    
    // ===== 2. 保留 const 與參考 =====
    std::cout << "===== 2. 保留 const 與參考 =====\n";
    
    const int cx = 100;
    int& rx = x;
    const int& crx = x;
    
    decltype(cx) c1 = 50;     // const int (保留 const)
    decltype(rx) r1 = x;      // int& (保留參考)
    decltype(crx) cr1 = x;    // const int& (保留 const 和參考)
    
    // c1 = 60;  // 錯誤！c1 是 const int
    r1 = 999;    // 合法，r1 是 int&
    
    std::cout << "修改 r1 = 999 後:\n";
    std::cout << "  x = " << x << " (證明 r1 是 x 的參考)\n\n";
    
    // ===== 3. auto vs decltype 比較 =====
    std::cout << "===== 3. auto vs decltype 比較 =====\n";
    
    const int value = 42;
    int& ref = x;
    
    auto       autoVal = value;      // int (忽略 const)
    decltype(value) declVal = 100;   // const int (保留 const)
    
    auto       autoRef = ref;        // int (忽略參考，是複製)
    decltype(ref) declRef = x;       // int& (保留參考)
    
    autoRef = 111;   // 不影響 x
    declRef = 222;   // 會修改 x
    
    std::cout << "autoRef = 111 後, x = " << x << "\n";
    x = 500;  // 重設
    declRef = 222;
    std::cout << "declRef = 222 後, x = " << x << " (證明 declRef 是參考)\n\n";
    
    // ===== 4. 括號的影響 (重要！) =====
    std::cout << "===== 4. 括號的影響 (重要！) =====\n";
    
    int num = 10;
    
    decltype(num)   d1;    // int (識別符號，用宣告型別)
    // decltype((num)) d2; // int& (左值表達式) — 無法不初始化！
    decltype((num)) d2 = num;  // int& (必須初始化參考)
    
    d2 = 777;
    std::cout << "d2 = 777 後, num = " << num << "\n";
    std::cout << "(證明 decltype((num)) 是 int&)\n\n";
    
    // ===== 5. 表達式的型別推導 =====
    std::cout << "===== 5. 表達式的型別推導 =====\n";
    
    int i = 5, j = 10;
    
    decltype(i + j) sum;        // int (prvalue，純右值)
    decltype(i < j) cmp;        // bool
    decltype(i += j) ref2 = i;  // int& (i += j 回傳 i 的參考，是左值)
    
    sum = 100;
    cmp = false;
    
    std::cout << "decltype(i + j) -> int, sum = " << sum << "\n";
    std::cout << "decltype(i < j) -> bool\n";
    std::cout << "decltype(i += j) -> int&\n\n";
    
    // ===== 6. 函式回傳型別推導 =====
    std::cout << "===== 6. 函式回傳型別推導 =====\n";
    
    decltype(getValue()) val1;           // int
    decltype(getReference()) val2 = x;   // int&
    decltype(getConstReference()) val3 = x;  // const int&
    
    std::cout << "decltype(getValue()) -> int\n";
    std::cout << "decltype(getReference()) -> int&\n";
    std::cout << "decltype(getConstReference()) -> const int&\n";
    
    // 注意：decltype 不會真的呼叫函式！
    std::cout << "(以上推導過程中，函式都沒有被實際呼叫)\n\n";
    
    // ===== 7. 陣列與指標 =====
    std::cout << "===== 7. 陣列與指標 =====\n";
    
    int arr[5] = {1, 2, 3, 4, 5};
    int* ptr = arr;
    
    decltype(arr) arr2 = {10, 20, 30, 40, 50};  // int[5] (保留陣列型別！)
    decltype(ptr) ptr2;                          // int*
    
    std::cout << "sizeof(arr) = " << sizeof(arr) << "\n";
    std::cout << "sizeof(arr2) = " << sizeof(arr2) << " (decltype 保留陣列型別)\n";
    
    // 對比 auto
    auto arrAuto = arr;  // int* (退化為指標)
    std::cout << "sizeof(arrAuto) = " << sizeof(arrAuto) << " (auto 退化為指標)\n\n";
    
    // ===== 8. 容器元素存取 =====
    std::cout << "===== 8. 容器元素存取 =====\n";
    
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // vec[0] 回傳 int&
    decltype(vec[0]) elem = vec[0];  // int&
    elem = 100;
    
    std::cout << "修改 elem 後, vec[0] = " << vec[0] << "\n";
    
    // vec.size() 回傳 size_t (prvalue)
    decltype(vec.size()) sz = vec.size();  // std::vector<int>::size_type
    std::cout << "vec.size() = " << sz << "\n\n";
    
    // ===== 9. 搭配 typedef / using =====
    std::cout << "===== 9. 搭配 typedef / using =====\n";
    
    std::vector<std::pair<std::string, int>> data;
    data.push_back({"Alice", 95});
    data.push_back({"Bob", 87});
    
    // 使用 decltype 定義型別別名
    using DataType = decltype(data);
    using ElemType = decltype(data[0]);
    using IterType = decltype(data.begin());
    
    DataType data2;
    data2.push_back({"Charlie", 92});
    
    std::cout << "成功使用 decltype 定義複雜型別別名\n";
    std::cout << "data2[0].first = " << data2[0].first << "\n\n";
    
    // ===== 10. 類別成員的 decltype =====
    std::cout << "===== 10. 類別成員的 decltype =====\n";
    
    struct Point
    {
        int x;
        double y;
    };
    
    Point pt{10, 20.5};
    
    decltype(pt.x) px = 100;      // int
    decltype(pt.y) py = 3.14;     // double
    decltype((pt.x)) prx = pt.x;  // int& (括號使其成為左值表達式)
    
    prx = 999;
    std::cout << "修改 prx 後, pt.x = " << pt.x << "\n";
    
    // 不需實體也能推導（使用類別名稱）
    decltype(Point::x) memberX;   // int
    decltype(Point::y) memberY;   // double
    std::cout << "decltype(Point::x) -> int\n";
    std::cout << "decltype(Point::y) -> double\n";
    
    return 0;
}
