#include <iostream>
#include <thread>
#include <random>

thread_local std::mt19937 rng{std::random_device{}()};

int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void worker(int id) {
    std::cout << "Thread " << id << ": " 
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << ", "
              << randomInt(1, 100) << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
