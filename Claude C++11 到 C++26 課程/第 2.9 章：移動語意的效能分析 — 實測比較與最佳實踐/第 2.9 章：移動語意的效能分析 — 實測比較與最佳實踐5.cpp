#include <iostream>
#include <vector>
#include <string>
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

int main() {
    const int N = 1000000;

    {
        Timer t("push_back(string)");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            std::string s = "Hello, World!!!!";  // 超過 SSO 長度
            v.push_back(s);                       // 複製
        }
    }

    {
        Timer t("push_back(move)");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            std::string s = "Hello, World!!!!";
            v.push_back(std::move(s));            // 移動
        }
    }

    {
        Timer t("emplace_back");
        std::vector<std::string> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) {
            v.emplace_back("Hello, World!!!!");   // 直接建構，零複製零移動
        }
    }

    return 0;
}
