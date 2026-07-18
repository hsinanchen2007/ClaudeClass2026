#include <iostream>
#include <thread>
#include <stdexcept>
#include <exception>
#include <functional>

class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;
    
public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        t = std::thread([this, f = std::forward<Func>(f), 
                         ...args = std::forward<Args>(args)]() mutable {
            try {
                f(args...);
            } catch (...) {
                exPtr = std::current_exception();
            }
        });
    }
    
    void join() {
        t.join();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    }
    
    ~SafeThread() {
        if (t.joinable()) {
            t.join();
        }
    }
};

int main() {
    try {
        SafeThread st([]() {
            throw std::runtime_error("錯誤！");
        });
        st.join();
    } catch (const std::exception& e) {
        std::cout << "捕獲: " << e.what() << std::endl;
    }
    
    return 0;
}
