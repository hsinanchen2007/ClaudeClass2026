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
    std::vector<JoiningThread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i]() {
            std::cout << "Worker " << i << std::endl;
        });
    }
    
    std::cout << "所有執行緒已建立" << std::endl;
    
    // 離開作用域時，vector 解構
    // 每個 JoiningThread 解構時自動 join
    return 0;
}
