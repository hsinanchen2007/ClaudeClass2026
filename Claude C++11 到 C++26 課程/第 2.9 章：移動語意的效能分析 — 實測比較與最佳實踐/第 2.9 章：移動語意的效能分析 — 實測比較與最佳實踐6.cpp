#include <iostream>
#include <vector>
#include <chrono>

class Timer {
    std::chrono::high_resolution_clock::time_point start_;
    const char* label_;
public:
    Timer(const char* label) : label_(label),
        start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto elapsed = std::chrono::high_resolution_clock::now() - start_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        std::cout << label_ << ": " << ms << " ms\n";
    }
};

struct WithNoexcept {
    std::vector<int> data;
    WithNoexcept() : data(1000, 42) {}
    WithNoexcept(const WithNoexcept& o) : data(o.data) {}
    WithNoexcept(WithNoexcept&& o) noexcept : data(std::move(o.data)) {}
};

struct WithoutNoexcept {
    std::vector<int> data;
    WithoutNoexcept() : data(1000, 42) {}
    WithoutNoexcept(const WithoutNoexcept& o) : data(o.data) {}
    WithoutNoexcept(WithoutNoexcept&& o) : data(std::move(o.data)) {}  // 沒有 noexcept
};

template<typename T>
void bench(const char* label) {
    Timer t(label);
    std::vector<T> v;
    // 不 reserve，讓 vector 反覆擴容
    for (int i = 0; i < 50000; ++i) {
        v.emplace_back();
    }
}

int main() {
    bench<WithNoexcept>("有 noexcept（擴容用移動）");
    bench<WithoutNoexcept>("沒 noexcept（擴容用複製）");
    return 0;
}
