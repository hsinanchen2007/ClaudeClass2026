#include <iostream>
#include <thread>

void background() {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Background task" << std::endl;  // 可能不會印出！
}

int main() {
    std::thread t(background);
    t.detach();  // 分離，不等待
    
    std::cout << "Main exits" << std::endl;
    return 0;  // 主程式結束，背景執行緒可能還沒完成
}
