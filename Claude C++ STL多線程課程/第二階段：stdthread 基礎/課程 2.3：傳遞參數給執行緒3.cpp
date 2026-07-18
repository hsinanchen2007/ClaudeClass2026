#include <iostream>
#include <thread>

void modify(int& x) {
    x = 100;
}

int main() {
    int value = 1;
    
    std::thread t(modify, std::ref(value));  // 傳遞引用
    t.join();
    
    std::cout << "value = " << value << std::endl;  // 現在是 100
    return 0;
}
