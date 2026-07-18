#include <iostream>

// 這是一個函數物件類別
class Adder {
public:
    int operator()(int a, int b) const {
        return a + b;
    }
};

int main() {
    Adder add;  // 建立一個物件
    
    // 像呼叫函數一樣使用物件
    int result = add(3, 5);  // 實際上是 add.operator()(3, 5)
    
    std::cout << "3 + 5 = " << result << std::endl;
    
    return 0;
}
