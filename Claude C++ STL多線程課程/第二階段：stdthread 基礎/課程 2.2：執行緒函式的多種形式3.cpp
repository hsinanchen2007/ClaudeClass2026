#include <iostream>
#include <thread>

class Worker {
public:
    void doWork() {
        std::cout << "Worker doing work" << std::endl;
    }
    
    void doWorkWithParam(int id) {
        std::cout << "Worker " << id << " working" << std::endl;
    }
};

int main() {
    Worker worker;
    
    // 成員函式（無參數）
    std::thread t1(&Worker::doWork, &worker);
    
    // 成員函式（有參數）
    std::thread t2(&Worker::doWorkWithParam, &worker, 123);
    
    t1.join();
    t2.join();
    return 0;
}
