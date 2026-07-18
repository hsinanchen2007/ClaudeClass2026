#include <thread>

int main() {
    std::thread t1([]() {});
    std::thread t2([]() {});
    
    // 危險！t1 還是 joinable，程式會呼叫 std::terminate()
    // t1 = std::move(t2);
    
    // 正確做法：先 join 或 detach
    t1.join();
    t1 = std::move(t2);  // 現在安全了
    t1.join();
    
    return 0;
}
