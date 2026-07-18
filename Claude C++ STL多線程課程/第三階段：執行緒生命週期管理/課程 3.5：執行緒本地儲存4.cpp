#include <iostream>
#include <thread>

thread_local int lastError = 0;

void setError(int code) {
    lastError = code;
}

int getError() {
    return lastError;
}

void worker(int id) {
    setError(id * 100);
    std::cout << "Thread " << id << " error: " << getError() << std::endl;
}

int main() {
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
