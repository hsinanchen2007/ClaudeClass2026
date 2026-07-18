// 檔案名稱: sfinae_hassize.cpp
// 編譯指令: g++ -std=c++11 -Wall -o sfinae_hassize sfinae_hassize.cpp
// 說明: 展示使用 decltype + SFINAE 檢測成員函式

#include <iostream>
#include <vector>
#include <string>
#include <type_traits>

// ===== 方法 1：函式重載 + SFINAE =====

// 主模板：當 T 有 size() 時匹配
template<typename T>
auto hasSize(T& t) -> decltype(t.size(), std::true_type{})
{
    (void)t;  // 避免未使用警告
    return std::true_type{};
}

// 備用模板：當主模板失敗時匹配
// 使用 ... (variadic) 作為最低優先順序的匹配
std::false_type hasSize(...)
{
    return std::false_type{};
}

// ===== 方法 2：使用類別模板（更通用的寫法）=====

// 預設情況：假設沒有 size()
template<typename T, typename = void>
struct HasSizeMethod : std::false_type {};

// 特化版本：當 T 有 size() 時匹配
template<typename T>
struct HasSizeMethod<T, decltype(std::declval<T>().size(), void())> 
    : std::true_type {};

// ===== 測試用的類別 =====

// 有 size() 成員函式
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

// 有 size() 但回傳型別不同
class WithIntSize
{
public:
    int size() const { return 10; }
};

int main()
{
    std::cout << std::boolalpha;  // 輸出 true/false 而非 1/0
    
    // ===== 測試方法 1：函式重載 =====
    std::cout << "===== 方法 1：函式重載 + SFINAE =====\n";
    
    std::vector<int> vec = {1, 2, 3};
    std::string str = "Hello";
    WithSize objWith;
    WithoutSize objWithout;
    int number = 42;
    
    // decltype(hasSize(x))::value 取得回傳型別的 value 成員
    std::cout << "std::vector<int> has size(): " 
              << decltype(hasSize(vec))::value << "\n";
    
    std::cout << "std::string has size():      " 
              << decltype(hasSize(str))::value << "\n";
    
    std::cout << "WithSize has size():         " 
              << decltype(hasSize(objWith))::value << "\n";
    
    std::cout << "WithoutSize has size():      " 
              << decltype(hasSize(objWithout))::value << "\n";
    
    std::cout << "int has size():              " 
              << decltype(hasSize(number))::value << "\n";
    
    std::cout << "\n";
    
    // ===== 測試方法 2：類別模板 =====
    std::cout << "===== 方法 2：類別模板特化 =====\n";
    
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
              << HasSizeMethod<WithIntSize>::value << "\n";
    
    std::cout << "\n";
    
    // ===== 實際應用：條件式呼叫 =====
    std::cout << "===== 實際應用：條件式呼叫 =====\n";
    
    // 根據是否有 size() 選擇不同行為
    auto printInfo = [](auto& container)
    {
        // 這裡使用執行期判斷（C++17 之前的做法）
        // C++17 可用 if constexpr 做編譯期判斷
        using ContainerType = typename std::decay<decltype(container)>::type;
        
        if (HasSizeMethod<ContainerType>::value)
        {
            std::cout << "此容器有 size() 方法\n";
        }
        else
        {
            std::cout << "此容器沒有 size() 方法\n";
        }
    };
    
    printInfo(vec);
    printInfo(objWithout);
    
    return 0;
}
