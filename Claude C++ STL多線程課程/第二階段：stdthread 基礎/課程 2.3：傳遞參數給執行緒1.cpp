#include <iostream>
#include <thread>

void printSum(int a, int b) {
    std::cout << a << " + " << b << " = " << (a + b) << std::endl;
}

int main() {
    std::thread t(printSum, 3, 5);
    t.join();
    return 0;
}
