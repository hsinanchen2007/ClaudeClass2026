#include <iostream>
#include <thread>

void work() {
    std::cout << "Working..." << std::endl;
}

int main() {
    std::thread t(work);
    // 忘記呼叫 join() 或 detach()
    return 0;  // 程式崩潰！呼叫 std::terminate()
}
