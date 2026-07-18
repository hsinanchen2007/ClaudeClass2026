#include <iostream>
#include <thread>
#include <string>

thread_local std::string cache;

std::string expensiveCompute(int id) {
    if (cache.empty()) {
        // 模擬耗時計算
        cache = "Result for thread " + std::to_string(id);
        std::cout << "Computing..." << std::endl;
    }
    return cache;
}

void worker(int id) {
    std::cout << expensiveCompute(id) << std::endl;
    std::cout << expensiveCompute(id) << std::endl;  // 使用快取
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
