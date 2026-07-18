#include <iostream>
#include <thread>
#include <vector>

std::vector<int> data = {1, 2, 3, 4, 5};

void reader() {
    for (auto it = data.begin(); it != data.end(); ++it) {
        // ← 另一執行緒可能修改 data，導致迭代器失效！
        std::cout << *it << " ";
    }
}

void writer() {
    data.push_back(6);  // 可能導致重新配置，所有迭代器失效
}

int main() {
    std::thread t1(reader);
    std::thread t2(writer);
    
    t1.join();
    t2.join();
    
    return 0;
}
