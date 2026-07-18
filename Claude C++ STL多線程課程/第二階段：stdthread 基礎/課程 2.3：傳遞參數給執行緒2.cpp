#include <iostream>
#include <thread>

void modify(int& x) {  // 期望引用
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, value);  // value 被複製！
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 仍然是 1
    return 0;
}
