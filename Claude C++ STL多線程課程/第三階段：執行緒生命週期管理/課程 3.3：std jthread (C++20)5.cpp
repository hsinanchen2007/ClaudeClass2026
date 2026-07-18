#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        // 註冊停止時的回調
        std::stop_callback callback(stoken, []() {
            std::cout << "停止回調被觸發！" << std::endl;
        });
        
        while (!stoken.stop_requested()) {
            std::cout << "工作中..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        
        std::cout << "執行緒結束" << std::endl;
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    jt.request_stop();
    
    return 0;
}
