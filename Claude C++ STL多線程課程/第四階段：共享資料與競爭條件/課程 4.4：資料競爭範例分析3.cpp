#include <iostream>
#include <thread>
#include <vector>

std::vector<int> vec;

void unsafeAppend(int value) {
    if (vec.size() < 10) {        // 檢查
        // ← 另一執行緒可能在此改變 size！
        vec.push_back(value);      // 可能超過限制
    }
}

void unsafeAccess() {
    if (!vec.empty()) {            // 檢查
        // ← 另一執行緒可能清空 vec！
        int last = vec.back();     // 可能崩潰
        std::cout << last << std::endl;
    }
}

int main() {
    std::thread t1([]{
        for (int i = 0; i < 15; ++i) {
            unsafeAppend(i);
        }
    });
    
    std::thread t2([]{
        for (int i = 0; i < 15; ++i) {
            unsafeAccess();
        }
    });
    
    t1.join();
    t2.join();
    
    return 0;
}