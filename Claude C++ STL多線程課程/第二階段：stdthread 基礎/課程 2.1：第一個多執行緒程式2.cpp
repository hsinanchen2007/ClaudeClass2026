#include <iostream>
#include <thread>

void work() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Work done!" << std::endl;
}

int main() {
    std::thread t(work);
    std::cout << "Waiting..." << std::endl;
    t.join();  // 阻塞，直到 t 完成
    std::cout << "Thread finished" << std::endl;
    return 0;
}
