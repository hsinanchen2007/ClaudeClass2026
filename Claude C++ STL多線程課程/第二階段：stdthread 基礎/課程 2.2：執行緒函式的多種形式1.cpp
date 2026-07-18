#include <iostream>
#include <thread>

void greet() {
    std::cout << "Hello!" << std::endl;
}

int main() {
    std::thread t(greet);
    t.join();
    return 0;
}
