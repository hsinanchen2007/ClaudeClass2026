// 檔案名稱: auto_demo.cpp
// 編譯指令: g++ -std=c++11 -Wall -o auto_demo auto_demo.cpp
// 說明: 展示 C++11 auto 關鍵字的各種用法

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <typeinfo>

int main()
{
    // ===== 1. 基本型別推導 =====
    std::cout << "===== 1. 基本型別推導 =====\n";
    
    auto i = 42;           // int
    auto d = 3.14;         // double
    auto f = 3.14f;        // float (因為有 f 後綴)
    auto c = 'A';          // char
    auto b = true;         // bool
    auto ll = 100LL;       // long long (因為有 LL 後綴)
    
    std::cout << "i = " << i << " (int)\n";
    std::cout << "d = " << d << " (double)\n";
    std::cout << "f = " << f << " (float)\n";
    std::cout << "c = " << c << " (char)\n";
    std::cout << "b = " << std::boolalpha << b << " (bool)\n";
    std::cout << "ll = " << ll << " (long long)\n\n";
    
    // ===== 2. 字串型別推導 =====
    std::cout << "===== 2. 字串型別推導 =====\n";
    
    auto str1 = "Hello";              // const char* (字串字面值)
    auto str2 = std::string("World"); // std::string
    
    std::cout << "str1 = " << str1 << " (const char*)\n";
    std::cout << "str2 = " << str2 << " (std::string)\n\n";
    
    // ===== 3. 容器迭代器 — auto 最常見的用途 =====
    std::cout << "===== 3. 容器迭代器 =====\n";
    
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    
    // C++03 寫法 (冗長)
    // std::vector<int>::iterator it = numbers.begin();
    
    // C++11 使用 auto (簡潔)
    std::cout << "vector 內容: ";
    for (auto it = numbers.begin(); it != numbers.end(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << "\n\n";
    
    // ===== 4. 複雜型別的簡化 =====
    std::cout << "===== 4. 複雜型別的簡化 =====\n";
    
    std::map<std::string, std::vector<int>> data;
    data["Alice"] = {90, 85, 88};
    data["Bob"] = {78, 82, 80};
    
    // 沒有 auto 的寫法:
    // std::map<std::string, std::vector<int>>::iterator mapIt = data.begin();
    
    // 使用 auto 的寫法:
    for (auto mapIt = data.begin(); mapIt != data.end(); ++mapIt)
    {
        std::cout << mapIt->first << " 的成績: ";
        for (auto score : mapIt->second)  // 範圍式 for 也用了 auto
        {
            std::cout << score << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
    
    // ===== 5. const 與參考的處理 =====
    std::cout << "===== 5. const 與參考的處理 =====\n";
    
    int x = 100;
    const int cx = 200;
    int& rx = x;
    
    auto a1 = x;     // int (複製)
    auto a2 = cx;    // int (頂層 const 被忽略，是複製)
    auto a3 = rx;    // int (參考被忽略，是複製)
    
    // 明確保留 const 或參考
    const auto a4 = x;     // const int
    auto& a5 = x;          // int&
    const auto& a6 = x;    // const int&
    auto& a7 = cx;         // const int& (底層 const 被保留)
    
    // 驗證 a5 確實是參考
    a5 = 999;
    std::cout << "修改 a5 後, x = " << x << " (證明 a5 是 x 的參考)\n\n";
    
    // ===== 6. 指標的處理 =====
    std::cout << "===== 6. 指標的處理 =====\n";
    
    int value = 42;
    int* ptr = &value;
    
    auto p1 = ptr;    // int* (指標被保留)
    auto* p2 = ptr;   // int* (明確表示是指標，效果相同)
    
    std::cout << "*p1 = " << *p1 << "\n";
    std::cout << "*p2 = " << *p2 << "\n\n";
    
    // ===== 7. 陣列退化為指標 =====
    std::cout << "===== 7. 陣列退化為指標 =====\n";
    
    int arr[5] = {10, 20, 30, 40, 50};
    
    auto arrAuto = arr;     // int* (陣列退化為指標)
    auto& arrRef = arr;     // int(&)[5] (參考保留陣列型別)
    
    std::cout << "arrAuto[0] = " << arrAuto[0] << "\n";
    std::cout << "sizeof(arr) = " << sizeof(arr) << " bytes\n";
    std::cout << "sizeof(arrAuto) = " << sizeof(arrAuto) << " bytes (指標大小)\n";
    std::cout << "sizeof(arrRef) = " << sizeof(arrRef) << " bytes (保留陣列大小)\n\n";
    
    // ===== 8. 同一行宣告多個變數 =====
    std::cout << "===== 8. 同一行宣告多個變數 =====\n";
    
    // 所有變數必須推導為相同型別
    auto v1 = 1, v2 = 2, v3 = 3;  // 全部是 int，合法
    // auto v4 = 1, v5 = 3.14;    // 錯誤！int 和 double 不同型別
    
    std::cout << "v1 = " << v1 << ", v2 = " << v2 << ", v3 = " << v3 << "\n\n";
    
    // ===== 9. auto 與運算式結果 =====
    std::cout << "===== 9. auto 與運算式結果 =====\n";
    
    int a = 10;
    int b_val = 3;
    
    auto sum = a + b_val;        // int
    auto division = a / b_val;   // int (整數除法)
    auto realDiv = a / 3.0;      // double (因為 3.0 是 double)
    
    std::cout << "10 + 3 = " << sum << " (int)\n";
    std::cout << "10 / 3 = " << division << " (int，整數除法)\n";
    std::cout << "10 / 3.0 = " << realDiv << " (double)\n";
    
    return 0;
}



// // 1. 函式參數 (C++11/14 不行，C++20 才可以)
// void func(auto x);  // C++11/14 錯誤，C++20 合法

// // 2. 類別的非靜態成員變數
// class MyClass
// {
//     auto value = 10;  // 錯誤！
// };

// // 3. 沒有初始化
// auto x;  // 錯誤！必須初始化

// // 4. 陣列宣告
// auto arr[5] = {1, 2, 3, 4, 5};  // 錯誤！

