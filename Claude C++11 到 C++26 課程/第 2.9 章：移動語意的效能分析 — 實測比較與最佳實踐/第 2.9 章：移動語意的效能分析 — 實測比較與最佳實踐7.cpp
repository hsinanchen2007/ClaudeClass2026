#include <iostream>
#include <chrono>
#include <utility>

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

int main() {
    const int N = 10000000;

    // 場景 1：基本型別 → 移動 = 複製
    {
        int source = 42;
        Timer t("int 複製");
        for (int i = 0; i < N; ++i) {
            int copy = source;
            (void)copy;
        }
    }
    {
        int source = 42;
        Timer t("int 移動");
        for (int i = 0; i < N; ++i) {
            int moved = std::move(source);
            (void)moved;
        }
    }

    // 場景 2：短字串（SSO 優化範圍內）→ 差異很小
    {
        Timer t("短 string 複製");
        for (int i = 0; i < N; ++i) {
            std::string s = "Hi";     // SSO：字串存在物件內部，不在堆積上
            std::string copy = s;
        }
    }
    {
        Timer t("短 string 移動");
        for (int i = 0; i < N; ++i) {
            std::string s = "Hi";
            std::string moved = std::move(s);
        }
    }

    return 0;
}
