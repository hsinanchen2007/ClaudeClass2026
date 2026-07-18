#include <iostream>
#include <thread>

std::thread createThread(int id) {
    return std::thread([id]() {
        std::cout << "執行緒 " << id << std::endl;
    });
}

int main() {
    std::thread t = createThread(42);
    t.join();
    return 0;
}
