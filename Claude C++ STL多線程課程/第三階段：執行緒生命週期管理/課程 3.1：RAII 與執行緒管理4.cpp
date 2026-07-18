#include <iostream>
#include <thread>

class FlexibleThread {
public:
    enum class Action { join, detach };
    
private:
    std::thread t;
    Action action;
    
public:
    FlexibleThread(std::thread thread, Action a) 
        : t(std::move(thread)), action(a) {}
    
    ~FlexibleThread() {
        if (t.joinable()) {
            if (action == Action::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
    
    FlexibleThread(const FlexibleThread&) = delete;
    FlexibleThread& operator=(const FlexibleThread&) = delete;
};

int main() {
    FlexibleThread ft1(
        std::thread([]() { std::cout << "Join 我" << std::endl; }),
        FlexibleThread::Action::join
    );
    
    FlexibleThread ft2(
        std::thread([]() { std::cout << "Detach 我" << std::endl; }),
        FlexibleThread::Action::detach
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
