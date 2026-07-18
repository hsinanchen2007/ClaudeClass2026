#include <iostream>
#include <vector>
#include <string>

class Tracker {
    std::string name_;
public:
    Tracker(const std::string& name) : name_(name) {
        std::cout << "  建構: " << name_ << std::endl;
    }

    // 拷貝建構子
    Tracker(const Tracker& other) : name_(other.name_) {
        std::cout << "  拷貝: " << name_ << std::endl;
    }

    // 移動建構子（標記 noexcept）
    Tracker(Tracker&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動: " << name_ << std::endl;
    }

    ~Tracker() {
        std::cout << "  銷毀: " << name_ << std::endl;
    }
};

int main() {
    std::cout << "=== 建立 vector，容量 2 ===" << std::endl;
    std::vector<Tracker> v;
    v.reserve(2);

    std::cout << "\n=== push_back A ===" << std::endl;
    v.push_back(Tracker("A"));

    std::cout << "\n=== push_back B ===" << std::endl;
    v.push_back(Tracker("B"));

    std::cout << "\n=== push_back C（觸發重新配置）===" << std::endl;
    v.push_back(Tracker("C"));

    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
