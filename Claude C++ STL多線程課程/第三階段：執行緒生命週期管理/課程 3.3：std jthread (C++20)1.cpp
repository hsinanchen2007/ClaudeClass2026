#include <iostream>
#include <thread>

int main() {
    std::jthread jt([]() {
        std::cout << "Hello from jthread!" << std::endl;
    });
    
    // 不需要呼叫 join()！
    // 離開作用域時自動 join
    
    return 0;
}
