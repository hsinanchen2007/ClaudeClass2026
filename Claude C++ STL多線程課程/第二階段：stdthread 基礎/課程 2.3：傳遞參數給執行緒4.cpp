#include <iostream>
#include <thread>

void modifyPtr(int* p) {
    *p = 200;
}

int main() {
    int value = 1;
    
    std::thread t(modifyPtr, &value);
    t.join();
    
    std::cout << "value = " << value << std::endl;
    return 0;
}
