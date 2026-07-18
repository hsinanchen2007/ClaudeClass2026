#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "1. 建立執行緒" << std::endl;
    
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "3. 執行緒完成" << std::endl;
    });
    
    std::cout << "2. 呼叫 join()，開始等待..." << std::endl;
    t.join();  // 在這裡阻塞 2 秒
    
    std::cout << "4. join() 返回，繼續執行" << std::endl;
    return 0;
}
