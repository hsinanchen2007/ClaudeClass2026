#include <iostream>
#include <thread>

int main() {
    std::jthread jt([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::cout << "Running..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    });
    
    // 取得 stop_source
    std::stop_source& source = jt.get_stop_source();
    
    // 取得 stop_token
    std::stop_token token = jt.get_stop_token();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 兩種方式都可以請求停止
    // source.request_stop();
    jt.request_stop();
    
    return 0;
}
