#include <iostream>
#include <vector>
#include <string>

class Widget {
    std::string name_;
    int value_;
public:
    // 一般建構子
    Widget(const std::string& name, int value)
        : name_(name), value_(value) {
        std::cout << "  建構 Widget(" << name_ << ", " << value_ << ")" << std::endl;
    }

    // 拷貝建構子
    Widget(const Widget& other)
        : name_(other.name_), value_(other.value_) {
        std::cout << "  拷貝 Widget(" << name_ << ")" << std::endl;
    }

    // 移動建構子
    Widget(Widget&& other) noexcept
        : name_(std::move(other.name_)), value_(other.value_) {
        std::cout << "  移動 Widget(" << name_ << ")" << std::endl;
    }

    ~Widget() = default;
};

int main() {
    std::vector<Widget> v;
    v.reserve(4);  // 預留空間，避免重新配置干擾觀察

    std::cout << "=== push_back（臨時物件）===" << std::endl;
    v.push_back(Widget("A", 1));
    // 過程：建構臨時物件 → 移動到 vector 內部 → 銷毀臨時物件

    std::cout << "\n=== emplace_back（原地建構）===" << std::endl;
    v.emplace_back("B", 2);
    // 過程：直接在 vector 內部的記憶體上建構，沒有臨時物件

    std::cout << "\n=== push_back（具名物件）===" << std::endl;
    Widget w("C", 3);
    v.push_back(w);
    // 過程：拷貝 w 到 vector 內部

    std::cout << "\n=== push_back（std::move 具名物件）===" << std::endl;
    Widget w2("D", 4);
    v.push_back(std::move(w2));
    // 過程：移動 w2 到 vector 內部

    return 0;
}
