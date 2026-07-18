#include <iostream>
#include <thread>
#include <memory>

void process(std::unique_ptr<int> ptr) {
    std::cout << "Value: " << *ptr << std::endl;
}

int main() {
    auto ptr = std::make_unique<int>(42);
    
    std::thread t(process, std::move(ptr));  // 必須 move
    t.join();
    
    // ptr 現在是空的
    return 0;
}
