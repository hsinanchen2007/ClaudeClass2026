#include <iostream>
#include <thread>

int main() {
    // 取得主執行緒 ID
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << std::endl;
    
    std::thread t([]() {
        // 在執行緒內部取得自己的 ID
        std::cout << "子執行緒 ID: " << std::this_thread::get_id() << std::endl;
    });
    
    // 從外部取得執行緒 ID
    std::cout << "t 的 ID: " << t.get_id() << std::endl;
    
    t.join();
    return 0;
}
