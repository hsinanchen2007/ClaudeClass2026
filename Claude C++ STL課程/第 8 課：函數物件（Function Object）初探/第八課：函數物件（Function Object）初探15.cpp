#include <iostream>
#include <functional>
#include <vector>

// 普通函數
int add(int a, int b) {
    return a + b;
}

// 函數物件
class Multiplier {
public:
    int operator()(int a, int b) const {
        return a * b;
    }
};

int main() {
    // std::function 可以儲存各種可呼叫物件
    std::function<int(int, int)> func;
    
    // 儲存普通函數
    func = add;
    std::cout << "普通函數: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存函數物件
    func = Multiplier();
    std::cout << "函數物件: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 儲存 Lambda
    func = [](int a, int b) { return a - b; };
    std::cout << "Lambda: func(3, 5) = " << func(3, 5) << std::endl;
    
    // 實際應用：儲存多個操作
    std::cout << "\n=== 儲存多個操作 ===" << std::endl;
    std::vector<std::function<int(int, int)>> operations;
    
    operations.push_back(add);
    operations.push_back(Multiplier());
    operations.push_back([](int a, int b) { return a - b; });
    operations.push_back([](int a, int b) { return a / b; });
    
    int a = 20, b = 4;
    std::vector<std::string> names = {"加", "乘", "減", "除"};
    
    for (size_t i = 0; i < operations.size(); ++i) {
        std::cout << a << " " << names[i] << " " << b << " = " 
                  << operations[i](a, b) << std::endl;
    }
    
    return 0;
}
