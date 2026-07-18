#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::vector<std::thread> threads;
    
    // 建立多個執行緒
    for (int i = 0; i < 5; ++i) {
        threads.push_back(std::thread([i]() {
            std::cout << "執行緒 " << i << std::endl;
        }));
        // 或使用 emplace_back
        // threads.emplace_back([i]() { ... });
    }
    
    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
