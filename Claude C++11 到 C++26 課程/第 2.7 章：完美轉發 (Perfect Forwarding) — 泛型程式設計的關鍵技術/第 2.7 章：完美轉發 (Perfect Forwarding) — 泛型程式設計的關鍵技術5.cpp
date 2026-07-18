#include <iostream>
#include <string>
#include <utility>
#include <chrono>
#include <functional>

// ===== 日誌包裝器 =====
template<typename Func, typename... Args>
auto logged_call(const std::string& tag, Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    std::cout << "[LOG] " << tag << " 開始\n";

    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);

    std::cout << "[LOG] " << tag << " 結束\n";
    return result;
}

// ===== 重試包裝器 =====
template<typename Func, typename... Args>
auto retry(int max_attempts, Func&& func, Args&&... args)
    -> decltype(func(args...))
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            std::cout << "  嘗試第 " << attempt << " 次...\n";
            // 注意：這裡不能 forward args，因為要重複使用
            // 只有最後一次才 forward（但這裡無法預知哪次成功）
            // 所以用普通傳遞，犧牲一點效率換取正確性
            return func(args...);
        } catch (const std::exception& e) {
            std::cout << "  失敗: " << e.what() << "\n";
            if (attempt == max_attempts) throw;
        }
    }
    throw std::runtime_error("unreachable");
}

// 模擬不穩定的操作
int unstable_counter = 0;
std::string fetch_data(const std::string& url, int timeout) {
    ++unstable_counter;
    if (unstable_counter < 3) {
        throw std::runtime_error("connection timeout");
    }
    return "data from " + url;
}

int main() {
    // 日誌包裝器
    auto result = logged_call("process",
        [](int a, int b) { return a + b; },
        10, 20);
    std::cout << "結果: " << result << "\n\n";

    // 重試包裝器
    try {
        auto data = retry(5, fetch_data, std::string("https://api.example.com"), 30);
        std::cout << "成功: " << data << "\n";
    } catch (...) {
        std::cout << "最終失敗\n";
    }

    return 0;
}
