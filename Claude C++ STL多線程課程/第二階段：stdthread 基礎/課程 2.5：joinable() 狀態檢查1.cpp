#include <iostream>
#include <thread>

int main() {
    // 情況 1：預設建構
    std::thread t1;
    std::cout << "預設建構: " << t1.joinable() << std::endl;
    
    // 情況 2：已經 join
    std::thread t2([]() {});
    t2.join();
    std::cout << "join 後: " << t2.joinable() << std::endl;
    
    // 情況 3：已經 detach
    std::thread t3([]() {});
    t3.detach();
    std::cout << "detach 後: " << t3.joinable() << std::endl;
    
    // 情況 4：被 move 走
    std::thread t4([]() {});
    std::thread t5 = std::move(t4);
    std::cout << "move 後(原): " << t4.joinable() << std::endl;
    std::cout << "move 後(新): " << t5.joinable() << std::endl;
    t5.join();
    
    return 0;
}
