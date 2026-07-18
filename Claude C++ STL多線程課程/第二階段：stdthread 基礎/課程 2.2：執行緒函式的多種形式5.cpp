#include <iostream>
#include <thread>

class Counter {
    int count;
public:
    Counter(int c) : count(c) {}
    
    void operator()() const {
        for (int i = 0; i < count; ++i) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
    }
};

int main() {
    std::thread t{Counter(5)};
    t.join();
    return 0;
}
