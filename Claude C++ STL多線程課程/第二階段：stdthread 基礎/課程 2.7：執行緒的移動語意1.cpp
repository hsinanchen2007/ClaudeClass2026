#include <thread>

int main() {
    std::thread t1([]() {});
    
    // std::thread t2 = t1;        // 編譯錯誤！
    // std::thread t3(t1);         // 編譯錯誤！
    
    t1.join();
    return 0;
}
