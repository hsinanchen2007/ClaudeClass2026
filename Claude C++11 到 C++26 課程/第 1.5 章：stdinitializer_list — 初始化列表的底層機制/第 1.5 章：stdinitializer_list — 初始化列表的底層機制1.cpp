// 檔案名稱: initializer_list_demo.cpp
// 編譯指令: g++ -std=c++11 -Wall -o initializer_list_demo initializer_list_demo.cpp
// 說明: 展示 std::initializer_list 的用法與特性

#include <iostream>
#include <initializer_list>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cstddef>

// ===== 1. 自訂類別使用 initializer_list 建構子 =====

class IntArray
{
private:
    int* data_;
    std::size_t size_;
    
public:
    // initializer_list 建構子
    IntArray(std::initializer_list<int> init)
        : data_{new int[init.size()]}
        , size_{init.size()}
    {
        std::cout << "[IntArray] initializer_list 建構子, size = " 
                  << size_ << "\n";
        
        // 從 initializer_list 複製元素
        std::copy(init.begin(), init.end(), data_);
    }
    
    // 指定大小的建構子
    explicit IntArray(std::size_t size)
        : data_{new int[size]{}}  // 零初始化
        , size_{size}
    {
        std::cout << "[IntArray] size 建構子, size = " << size << "\n";
    }
    
    // 解構子
    ~IntArray()
    {
        delete[] data_;
    }
    
    // 禁止複製（簡化範例）
    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;
    
    // 成員存取
    std::size_t size() const { return size_; }
    
    int& operator[](std::size_t index) { return data_[index]; }
    const int& operator[](std::size_t index) const { return data_[index]; }
    
    // 迭代器支援
    int* begin() { return data_; }
    int* end() { return data_ + size_; }
    const int* begin() const { return data_; }
    const int* end() const { return data_ + size_; }
    
    // 印出內容
    void print(const std::string& name) const
    {
        std::cout << name << ": [";
        for (std::size_t i = 0; i < size_; ++i)
        {
            if (i > 0) std::cout << ", ";
            std::cout << data_[i];
        }
        std::cout << "]\n";
    }
};

// ===== 2. 使用 initializer_list 作為函式參數 =====

void printValues(std::initializer_list<int> values)
{
    std::cout << "printValues: {";
    bool first = true;
    for (int v : values)
    {
        if (!first) std::cout << ", ";
        std::cout << v;
        first = false;
    }
    std::cout << "}\n";
}

int sum(std::initializer_list<int> values)
{
    return std::accumulate(values.begin(), values.end(), 0);
}

int findMax(std::initializer_list<int> values)
{
    if (values.size() == 0)
    {
        throw std::invalid_argument("Empty list");
    }
    return *std::max_element(values.begin(), values.end());
}

double average(std::initializer_list<double> values)
{
    if (values.size() == 0) return 0.0;
    double total = std::accumulate(values.begin(), values.end(), 0.0);
    return total / values.size();
}

// ===== 3. 泛型版本 =====

template<typename T>
T findMin(std::initializer_list<T> values)
{
    if (values.size() == 0)
    {
        throw std::invalid_argument("Empty list");
    }
    return *std::min_element(values.begin(), values.end());
}

template<typename T>
void printAll(std::initializer_list<T> values)
{
    std::cout << "printAll<" << typeid(T).name() << ">: ";
    for (const T& v : values)
    {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

// ===== 4. 結合多載：展示優先順序 =====

class Ambiguous
{
public:
    Ambiguous(int a, int b)
    {
        std::cout << "Ambiguous(int, int): a=" << a << ", b=" << b << "\n";
    }
    
    Ambiguous(std::initializer_list<int> init)
    {
        std::cout << "Ambiguous(initializer_list<int>): size=" 
                  << init.size() << ", values = {";
        bool first = true;
        for (int v : init)
        {
            if (!first) std::cout << ", ";
            std::cout << v;
            first = false;
        }
        std::cout << "}\n";
    }
};

// ===== 5. 類別內使用 initializer_list 設定資料 =====

class Config
{
private:
    std::vector<std::string> options_;
    
public:
    Config() = default;
    
    // 接受字串列表
    Config(std::initializer_list<std::string> opts)
        : options_(opts)
    {
    }
    
    // 設定方法也接受 initializer_list
    void setOptions(std::initializer_list<std::string> opts)
    {
        options_.assign(opts.begin(), opts.end());
    }
    
    // 新增多個選項
    void addOptions(std::initializer_list<std::string> opts)
    {
        options_.insert(options_.end(), opts.begin(), opts.end());
    }
    
    void print() const
    {
        std::cout << "Config options:\n";
        for (const auto& opt : options_)
        {
            std::cout << "  - " << opt << "\n";
        }
    }
};

int main()
{
    std::cout << std::boolalpha;
    
    // ===== 1. 基本使用 =====
    std::cout << "===== 1. std::initializer_list 基本使用 =====\n";
    
    std::initializer_list<int> list1 = {10, 20, 30, 40, 50};
    
    std::cout << "list1 size: " << list1.size() << "\n";
    std::cout << "list1 elements: ";
    for (int n : list1)
    {
        std::cout << n << " ";
    }
    std::cout << "\n";
    
    // 使用 begin() 和 end()
    std::cout << "first element: " << *list1.begin() << "\n";
    std::cout << "last element: " << *(list1.end() - 1) << "\n\n";
    
    // ===== 2. 輕量複製特性 =====
    std::cout << "===== 2. 輕量複製特性 =====\n";
    
    std::initializer_list<int> list2 = list1;  // 只複製指標和大小
    
    std::cout << "list1.begin(): " << static_cast<const void*>(list1.begin()) << "\n";
    std::cout << "list2.begin(): " << static_cast<const void*>(list2.begin()) << "\n";
    std::cout << "兩者指向相同記憶體位址: " 
              << (list1.begin() == list2.begin()) << "\n\n";
    
    // ===== 3. 元素是 const 的 =====
    std::cout << "===== 3. 元素是 const 的 =====\n";
    
    std::initializer_list<int> list3 = {1, 2, 3};
    
    // *list3.begin() = 100;  // 編譯錯誤！元素是 const
    
    std::cout << "無法修改 initializer_list 中的元素\n";
    std::cout << "型別是: const int*\n\n";
    
    // ===== 4. 自訂類別的 initializer_list 建構子 =====
    std::cout << "===== 4. 自訂類別的 initializer_list 建構子 =====\n";
    
    IntArray arr1{1, 2, 3, 4, 5};
    arr1.print("arr1");
    
    IntArray arr2{100, 200};
    arr2.print("arr2");
    
    // 明確使用 size 建構子
    IntArray arr3(10);  // 使用 () 避免匹配 initializer_list
    arr3.print("arr3");
    
    std::cout << "\n";
    
    // ===== 5. 函式參數使用 initializer_list =====
    std::cout << "===== 5. 函式參數使用 initializer_list =====\n";
    
    printValues({1, 2, 3, 4, 5});
    
    std::cout << "sum({1, 2, 3, 4, 5}) = " << sum({1, 2, 3, 4, 5}) << "\n";
    std::cout << "findMax({3, 7, 2, 9, 1}) = " << findMax({3, 7, 2, 9, 1}) << "\n";
    std::cout << "average({1.5, 2.5, 3.5, 4.5}) = " 
              << average({1.5, 2.5, 3.5, 4.5}) << "\n\n";
    
    // ===== 6. 泛型函式 =====
    std::cout << "===== 6. 泛型函式 =====\n";
    
    std::cout << "findMin({5, 3, 8, 1, 9}) = " 
              << findMin({5, 3, 8, 1, 9}) << "\n";
    std::cout << "findMin({3.14, 1.41, 2.72}) = " 
              << findMin({3.14, 1.41, 2.72}) << "\n";
    std::cout << "findMin({\"zebra\", \"apple\", \"mango\"}) = " 
              << findMin<std::string>({"zebra", "apple", "mango"}) << "\n";
    
    printAll({10, 20, 30});
    printAll({1.1, 2.2, 3.3});
    printAll<std::string>({"Hello", "World"});
    
    std::cout << "\n";
    
    // ===== 7. initializer_list 建構子優先順序 =====
    std::cout << "===== 7. initializer_list 建構子優先順序 =====\n";
    
    Ambiguous a1(5, 10);     // 一般建構子
    Ambiguous a2{5, 10};     // initializer_list 建構子！
    Ambiguous a3{5};         // initializer_list 建構子！
    // Ambiguous a4{};       // 預設建構子（如果有）
    
    std::cout << "\n";
    
    // ===== 8. 標準容器的 initializer_list =====
    std::cout << "===== 8. 標準容器的 initializer_list =====\n";
    
    // vector
    std::vector<int> v1{1, 2, 3, 4, 5};
    std::cout << "vector<int>{1,2,3,4,5}: ";
    for (int n : v1) std::cout << n << " ";
    std::cout << "\n";
    
    // assign 方法
    std::vector<int> v2;
    v2.assign({10, 20, 30});
    std::cout << "v2.assign({10,20,30}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n";
    
    // insert 方法
    v2.insert(v2.end(), {40, 50});
    std::cout << "v2.insert(end, {40,50}): ";
    for (int n : v2) std::cout << n << " ";
    std::cout << "\n\n";
    
    // ===== 9. Config 類別示範 =====
    std::cout << "===== 9. Config 類別示範 =====\n";
    
    Config cfg{"verbose", "debug", "color"};
    cfg.print();
    
    cfg.addOptions({"timestamps", "logging"});
    cfg.print();
    
    cfg.setOptions({"minimal"});
    cfg.print();
    
    std::cout << "\n";
    
    // ===== 10. 空的 initializer_list =====
    std::cout << "===== 10. 空的 initializer_list =====\n";
    
    std::initializer_list<int> emptyList{};
    std::cout << "empty list size: " << emptyList.size() << "\n";
    std::cout << "begin() == end(): " << (emptyList.begin() == emptyList.end()) << "\n\n";
    
    // ===== 11. auto 與 initializer_list =====
    std::cout << "===== 11. auto 與 initializer_list =====\n";
    
    auto autoList = {1, 2, 3};  // std::initializer_list<int>
    std::cout << "auto autoList = {1, 2, 3};\n";
    std::cout << "  type: std::initializer_list<int>\n";
    std::cout << "  size: " << autoList.size() << "\n";
    
    // C++17 之後，單一元素有變化
    // auto singleBrace{42};  // C++11/14: initializer_list<int>
                               // C++17+: int
    
    std::cout << "\n";
    
    // ===== 12. 生命週期注意事項 =====
    std::cout << "===== 12. 生命週期注意事項 =====\n";
    
    // 危險！底層陣列在表達式結束後銷毀
    // std::initializer_list<int> getList()
    // {
    //     return {1, 2, 3};  // 回傳後底層陣列已銷毀！
    // }
    
    // 安全：立即使用
    auto safeUse = [](std::initializer_list<int> list) {
        int total = 0;
        for (int n : list) total += n;
        return total;
    };
    
    int result = safeUse({1, 2, 3, 4, 5});
    std::cout << "safeUse({1,2,3,4,5}) = " << result << "\n";
    
    // 安全：在同一表達式內使用
    std::vector<int> safeVec({1, 2, 3, 4, 5});
    std::cout << "直接傳給建構子是安全的\n";
    
    return 0;
}
