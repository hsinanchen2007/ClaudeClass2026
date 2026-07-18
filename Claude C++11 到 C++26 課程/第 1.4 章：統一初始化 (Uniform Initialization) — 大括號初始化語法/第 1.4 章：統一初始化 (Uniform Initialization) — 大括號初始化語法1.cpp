// 檔案名稱: uniform_init_demo.cpp
// 編譯指令: g++ -std=c++11 -Wall -o uniform_init_demo uniform_init_demo.cpp
// 說明: 展示 C++11 統一初始化的各種用法

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <array>
#include <memory>
#include <complex>

// ===== 用於展示的類別 =====

class Point
{
public:
    int x;
    int y;
    
    // 預設建構子
    Point() : x{0}, y{0}
    {
        std::cout << "  Point() 預設建構\n";
    }
    
    // 雙參數建構子
    Point(int x_, int y_) : x{x_}, y{y_}
    {
        std::cout << "  Point(" << x << ", " << y << ") 建構\n";
    }
    
    void print() const
    {
        std::cout << "  Point(" << x << ", " << y << ")\n";
    }
};

// 有 initializer_list 建構子的類別
class NumberList
{
    std::vector<int> data_;
    
public:
    // 一般建構子：指定大小和初始值
    NumberList(std::size_t size, int value)
        : data_(size, value)
    {
        std::cout << "  NumberList(size=" << size << ", value=" << value << ")\n";
    }
    
    // initializer_list 建構子
    NumberList(std::initializer_list<int> init)
        : data_(init)
    {
        std::cout << "  NumberList(initializer_list, size=" << init.size() << ")\n";
    }
    
    void print() const
    {
        std::cout << "  內容: ";
        for (int n : data_)
        {
            std::cout << n << " ";
        }
        std::cout << "\n";
    }
};

// 聚合類別（Aggregate）：無使用者定義建構子、無私有成員等
struct Aggregate
{
    int a;
    double b;
    std::string c;
};

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本型別的初始化 =====
    std::cout << "===== 1. 基本型別的初始化 =====\n";
    
    int i1{42};          // 直接列表初始化
    int i2 = {42};       // 複製列表初始化
    int i3{};            // 值初始化，i3 = 0
    double d{3.14};
    char c{'A'};
    bool b{true};
    
    std::cout << "i1 = " << i1 << "\n";
    std::cout << "i2 = " << i2 << "\n";
    std::cout << "i3 = " << i3 << " (值初始化為 0)\n";
    std::cout << "d = " << d << "\n";
    std::cout << "c = " << c << "\n";
    std::cout << "b = " << b << "\n\n";
    
    // ===== 2. 防止窄化轉換 (Narrowing Conversion) =====
    std::cout << "===== 2. 防止窄化轉換 =====\n";
    
    int largeValue = 1000;
    
    // 使用 () 或 = 不會檢查窄化
    char c1 = largeValue;     // 允許，但可能資料遺失
    char c2(largeValue);      // 允許，但可能資料遺失
    
    // 使用 {} 會在編譯期檢查窄化
    // char c3{largeValue};   // 編譯錯誤！窄化轉換
    // char c4 = {largeValue}; // 編譯錯誤！窄化轉換
    
    // 常量表達式在範圍內則允許
    char c5{65};              // 合法，65 在 char 範圍內
    
    // double 到 int 也是窄化
    double pi = 3.14159;
    // int truncated{pi};     // 編譯錯誤！窄化轉換
    int truncated(pi);        // 允許，但會截斷
    
    std::cout << "c1 = " << static_cast<int>(c1) << " (可能不正確)\n";
    std::cout << "c5 = " << c5 << " (65 = 'A')\n";
    std::cout << "truncated = " << truncated << " (從 3.14159 截斷)\n";
    std::cout << "大括號初始化會在編譯期防止窄化轉換！\n\n";
    
    // ===== 3. 陣列初始化 =====
    std::cout << "===== 3. 陣列初始化 =====\n";
    
    // C 風格陣列
    int arr1[5]{1, 2, 3, 4, 5};
    int arr2[5]{1, 2};         // 其餘元素為 0
    int arr3[5]{};             // 全部為 0
    
    std::cout << "arr1: ";
    for (int n : arr1) std::cout << n << " ";
    std::cout << "\n";
    
    std::cout << "arr2: ";
    for (int n : arr2) std::cout << n << " ";
    std::cout << "(其餘為 0)\n";
    
    std::cout << "arr3: ";
    for (int n : arr3) std::cout << n << " ";
    std::cout << "(全部為 0)\n";
    
    // std::array
    std::array<int, 4> stdArr{10, 20, 30, 40};
    std::cout << "std::array: ";
    for (int n : stdArr) std::cout << n << " ";
    std::cout << "\n\n";
    
    // ===== 4. STL 容器初始化 =====
    std::cout << "===== 4. STL 容器初始化 =====\n";
    
    // vector
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::cout << "vector: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << "\n";
    
    // map
    std::map<std::string, int> scores{
        {"Alice", 95},
        {"Bob", 87},
        {"Charlie", 92}
    };
    std::cout << "map:\n";
    for (const auto& pair : scores)
    {
        std::cout << "  " << pair.first << ": " << pair.second << "\n";
    }
    
    // pair
    std::pair<int, std::string> p{42, "Answer"};
    std::cout << "pair: (" << p.first << ", " << p.second << ")\n";
    
    // 巢狀容器
    std::vector<std::vector<int>> matrix{
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };
    std::cout << "matrix:\n";
    for (const auto& row : matrix)
    {
        std::cout << "  ";
        for (int n : row) std::cout << n << " ";
        std::cout << "\n";
    }
    std::cout << "\n";
    
    // ===== 5. 類別初始化 =====
    std::cout << "===== 5. 類別初始化 =====\n";
    
    std::cout << "Point p1{}:\n";
    Point p1{};        // 呼叫預設建構子
    p1.print();
    
    std::cout << "Point p2{10, 20}:\n";
    Point p2{10, 20};  // 呼叫雙參數建構子
    p2.print();
    
    std::cout << "\n";
    
    // ===== 6. 聚合初始化 =====
    std::cout << "===== 6. 聚合初始化 =====\n";
    
    Aggregate agg1{42, 3.14, "Hello"};
    Aggregate agg2{100};                    // b = 0.0, c = ""
    Aggregate agg3{};                       // 全部預設初始化
    
    std::cout << "agg1: a=" << agg1.a << ", b=" << agg1.b 
              << ", c=\"" << agg1.c << "\"\n";
    std::cout << "agg2: a=" << agg2.a << ", b=" << agg2.b 
              << ", c=\"" << agg2.c << "\"\n";
    std::cout << "agg3: a=" << agg3.a << ", b=" << agg3.b 
              << ", c=\"" << agg3.c << "\"\n\n";
    
    // ===== 7. initializer_list 建構子的優先順序 =====
    std::cout << "===== 7. initializer_list 建構子的優先順序 =====\n";
    
    std::cout << "NumberList n1{5, 10}:\n";
    NumberList n1{5, 10};    // 呼叫 initializer_list 版本！
    n1.print();
    
    std::cout << "NumberList n2(5, 10):\n";
    NumberList n2(5, 10);    // 呼叫一般建構子 (size, value)
    n2.print();
    
    std::cout << "\n注意：大括號優先匹配 initializer_list 建構子！\n\n";
    
    // ===== 8. 動態記憶體配置 =====
    std::cout << "===== 8. 動態記憶體配置 =====\n";
    
    // new 搭配大括號
    int* pInt = new int{42};
    std::cout << "*pInt = " << *pInt << "\n";
    delete pInt;
    
    // 陣列
    int* pArr = new int[5]{1, 2, 3, 4, 5};
    std::cout << "pArr: ";
    for (int i = 0; i < 5; ++i) std::cout << pArr[i] << " ";
    std::cout << "\n";
    delete[] pArr;
    
    // 智慧指標
    auto sp = std::make_shared<Point>(30, 40);
    sp->print();
    std::cout << "\n";
    
    // ===== 9. 函式參數與回傳值 =====
    std::cout << "===== 9. 函式參數與回傳值 =====\n";
    
    // Lambda 接收 vector
    auto printVec = [](const std::vector<int>& v) {
        std::cout << "  ";
        for (int n : v) std::cout << n << " ";
        std::cout << "\n";
    };
    
    // 直接傳遞大括號初始化列表
    printVec({10, 20, 30});
    
    // Lambda 回傳大括號初始化
    auto createPoint = []() -> Point {
        return {100, 200};  // 直接回傳，無需寫 Point
    };
    
    std::cout << "createPoint() 回傳:\n";
    Point p3 = createPoint();
    p3.print();
    std::cout << "\n";
    
    // ===== 10. 成員初始化列表 =====
    std::cout << "===== 10. 成員初始化列表中的大括號 =====\n";
    
    struct Container
    {
        std::vector<int> data;
        Point point;
        int value;
        
        Container()
            : data{1, 2, 3}      // 直接初始化 vector
            , point{5, 6}        // 直接初始化 Point
            , value{42}          // 直接初始化 int
        {
            std::cout << "Container 建構完成\n";
        }
    };
    
    Container cont;
    std::cout << "data: ";
    for (int n : cont.data) std::cout << n << " ";
    std::cout << "\n";
    cont.point.print();
    std::cout << "value: " << cont.value << "\n\n";
    
    // ===== 11. 零初始化 vs 預設初始化 =====
    std::cout << "===== 11. 零初始化 vs 預設初始化 =====\n";
    
    struct Data
    {
        int x;
        double y;
    };
    
    Data d1;      // 預設初始化：成員值不確定！
    Data d2{};    // 值初始化：x = 0, y = 0.0
    Data d3 = {}; // 同上
    
    // d1.x 和 d1.y 是未初始化的，讀取是未定義行為
    std::cout << "Data d2{}: x = " << d2.x << ", y = " << d2.y << "\n";
    std::cout << "使用 {} 確保成員被零初始化\n";
    
    return 0;
}
