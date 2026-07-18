#include <iostream>
#include <thread>

std::thread::id mainThreadId;

void checkThread() {
    if (std::this_thread::get_id() == mainThreadId) {
        std::cout << "這是主執行緒" << std::endl;
    } else {
        std::cout << "這是子執行緒" << std::endl;
    }
}

int main() {
    mainThreadId = std::this_thread::get_id();
    
    checkThread();  // 在主執行緒呼叫
    
    std::thread t(checkThread);  // 在子執行緒呼叫
    t.join();
    
    return 0;
}
