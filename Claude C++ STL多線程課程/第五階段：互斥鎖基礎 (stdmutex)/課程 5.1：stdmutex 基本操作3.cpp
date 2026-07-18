// 檔案：lesson_5_1_multiple_ops.cpp
// 說明：保護多個相關操作

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

std::vector<int> data;
std::mutex mtx;

void addAndPrint(int value) {
    mtx.lock();
    
    // 以下三個操作必須作為一個整體執行
    data.push_back(value);                    // 操作 1：新增元素
    std::cout << "Added: " << value;          // 操作 2：輸出訊息
    std::cout << ", Size: " << data.size();   // 操作 3：輸出大小
    std::cout << std::endl;
    
    mtx.unlock();
}

int main() {
    std::thread t1([]() {
        for (int i = 0; i < 5; ++i) {
            addAndPrint(i);
        }
    });
    
    std::thread t2([]() {
        for (int i = 100; i < 105; ++i) {
            addAndPrint(i);
        }
    });
    
    t1.join();
    t2.join();
    
    return 0;
}
