#include <iostream>
#include <thread>
#include <vector>

int main() {
    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    
    std::cout << "建立 " << numThreads << " 個執行緒" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            std::cout << "執行緒 " << i 
                      << " (ID: " << std::this_thread::get_id() << ")" 
                      << std::endl;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
