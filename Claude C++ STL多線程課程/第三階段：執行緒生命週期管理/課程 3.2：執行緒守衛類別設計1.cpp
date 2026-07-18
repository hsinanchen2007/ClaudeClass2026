#include <iostream>
#include <thread>
#include <utility>

class JoiningThread {
    std::thread t;
    
public:
    // 預設建構
    JoiningThread() noexcept = default;
    
    // 從 std::thread 移動建構
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}
    
    // 直接建構執行緒（完美轉發）
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}
    
    // 移動建構
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}
    
    // 移動賦值
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();  // 先處理當前執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }
    
    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;
    
    // 解構：自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 工具方法
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    
    void join() {
        t.join();
    }
    
    void detach() {
        t.detach();
    }
    
    std::thread& get() noexcept { return t; }
};

#include <iostream>
#include <vector>

int main() {
    // 方式一：直接建構
    JoiningThread jt1([]() {
        std::cout << "執行緒 1" << std::endl;
    });
    
    // 方式二：從 std::thread 移動
    std::thread t([]() {
        std::cout << "執行緒 2" << std::endl;
    });
    JoiningThread jt2(std::move(t));
    
    // 方式三：帶參數
    JoiningThread jt3([](int x) {
        std::cout << "執行緒 3: " << x << std::endl;
    }, 42);
    
    // 不需要手動 join！
    return 0;
}
