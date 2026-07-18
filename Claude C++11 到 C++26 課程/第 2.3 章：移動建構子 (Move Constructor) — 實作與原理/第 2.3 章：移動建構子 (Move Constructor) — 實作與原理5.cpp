#include <iostream>
#include <string>
#include <vector>
#include <utility>

class Tracker {
    std::string label_;
public:
    explicit Tracker(const std::string& l) : label_(l) {
        std::cout << "  [建構] " << label_ << "\n";
    }
    Tracker(const Tracker& o) : label_(o.label_ + "(copy)") {
        std::cout << "  [複製] " << label_ << "\n";
    }
    Tracker(Tracker&& o) noexcept : label_(std::move(o.label_)) {
        label_ += "(moved)";
        o.label_ = "(empty)";
        std::cout << "  [移動] " << label_ << "\n";
    }
    ~Tracker() {
        std::cout << "  [解構] " << label_ << "\n";
    }
    const std::string& label() const { return label_; }
};

Tracker make_tracker() {
    Tracker t("factory");
    return t;  // 可能觸發 NRVO 或移動
}

int main() {
    std::cout << "--- 時機 1：std::move ---\n";
    Tracker a("A");
    Tracker b(std::move(a));  // 移動建構

    std::cout << "\n--- 時機 2：接收臨時物件 ---\n";
    Tracker c(Tracker("temp"));  // 可能被 RVO 省略，也可能移動

    std::cout << "\n--- 時機 3：函式回傳值 ---\n";
    Tracker d = make_tracker();  // NRVO 或移動

    std::cout << "\n--- 時機 4：容器操作 ---\n";
    std::vector<Tracker> vec;
    vec.reserve(2);
    vec.push_back(Tracker("V1"));          // 移動（臨時物件）
    vec.push_back(std::move(b));           // 移動（明確轉換）

    std::cout << "\n--- 離開 main ---\n";
    return 0;
}
