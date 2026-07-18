#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        int count = 0;
        while (!stoken.stop_requested()) {
            std::cout << "工作中... " << ++count << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "收到停止請求，結束" << std::endl;
    });
    
    // 讓執行緒跑一下
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 請求停止
    jt.request_stop();
    
    // jt 解構時會等待執行緒結束
    return 0;
}
