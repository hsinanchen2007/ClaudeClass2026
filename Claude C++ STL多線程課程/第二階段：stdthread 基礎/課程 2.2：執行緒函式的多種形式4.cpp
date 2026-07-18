#include <iostream>
#include <thread>

class Task {
public:
    void operator()() const {
        std::cout << "Task executed" << std::endl;
    }
};

int main() {
    Task task;
    
    // 方式一：傳入物件
    std::thread t1(task);
    
    // 方式二：傳入臨時物件（注意額外括號）
    std::thread t2((Task()));
    
    // 方式三：使用大括號初始化（推薦）
    std::thread t3{Task()};
    
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
