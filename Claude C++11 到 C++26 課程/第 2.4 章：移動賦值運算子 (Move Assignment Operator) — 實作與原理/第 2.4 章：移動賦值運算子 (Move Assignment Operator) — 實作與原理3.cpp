#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

class Tracker {
    std::string label_;
public:
    Tracker(const char* l) : label_(l) {}
    Tracker(const Tracker& o) : label_(o.label_) {}
    Tracker(Tracker&& o) noexcept : label_(std::move(o.label_)) { o.label_ = "(empty)"; }

    Tracker& operator=(const Tracker& o) {
        label_ = o.label_; label_ += "(copy=)";
        std::cout << "  [複製賦值] " << label_ << "\n";
        return *this;
    }
    Tracker& operator=(Tracker&& o) noexcept {
        label_ = std::move(o.label_); label_ += "(move=)";
        o.label_ = "(empty)";
        std::cout << "  [移動賦值] " << label_ << "\n";
        return *this;
    }

    const std::string& label() const { return label_; }
};

Tracker make() { return Tracker("factory"); }

int main() {
    Tracker a("A"), b("B"), c("C");

    std::cout << "--- 時機 1：直接賦值左值 ---\n";
    a = b;                    // 複製賦值

    std::cout << "\n--- 時機 2：std::move ---\n";
    a = std::move(c);         // 移動賦值

    std::cout << "\n--- 時機 3：賦值臨時物件 ---\n";
    a = Tracker("temp");      // 移動賦值（臨時物件是右值）

    std::cout << "\n--- 時機 4：賦值函式回傳值 ---\n";
    a = make();               // 移動賦值（回傳值是右值）

    std::cout << "\n--- 時機 5：容器中的搬移 ---\n";
    std::vector<Tracker> vec;
    vec.reserve(10);
    vec.push_back(Tracker("V1"));
    vec.push_back(Tracker("V2"));

    // swap 內部使用移動賦值
    std::cout << "swap:\n";
    std::swap(vec[0], vec[1]);

    return 0;
}
