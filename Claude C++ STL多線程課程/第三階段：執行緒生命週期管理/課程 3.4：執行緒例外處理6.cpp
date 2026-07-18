#include <iostream>
#include <thread>
#include <future>
#include <stdexcept>

void worker(std::promise<int> prom) {
    try {
        throw std::runtime_error("Worker 錯誤！");
        prom.set_value(42);
    } catch (...) {
        prom.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    
    std::thread t(worker, std::move(prom));
    
    try {
        int value = fut.get();
        std::cout << "結果: " << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    t.join();
    return 0;
}
