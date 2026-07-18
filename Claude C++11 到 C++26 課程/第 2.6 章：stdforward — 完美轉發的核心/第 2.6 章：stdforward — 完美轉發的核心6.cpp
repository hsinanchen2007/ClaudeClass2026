#include <iostream>
#include <string>
#include <utility>
#include <chrono>

// 通用的計時包裝器
template<typename Func, typename... Args>
auto timed_call(Func&& func, Args&&... args)
    -> decltype(std::forward<Func>(func)(std::forward<Args>(args)...))
{
    auto start = std::chrono::high_resolution_clock::now();

    // 完美轉發函式物件和所有引數
    auto result = std::forward<Func>(func)(std::forward<Args>(args)...);

    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "  耗時: " << us << " us\n";

    return result;
}

std::string process(const std::string& input, int repeat) {
    std::string result;
    for (int i = 0; i < repeat; ++i) result += input;
    return result;
}

int main() {
    std::string input = "Hello";
    auto result = timed_call(process, input, 10000);
    std::cout << "結果長度: " << result.size() << "\n";

    return 0;
}
