#include <iostream>
#include <string>
#include <utility>
#include <type_traits>

class Widget {
    std::string name_;

public:
    // 用 enable_if 排除 Widget 型別本身
    template<typename T,
             typename = typename std::enable_if<
                 !std::is_same<typename std::decay<T>::type, Widget>::value
             >::type>
    Widget(T&& name) : name_(std::forward<T>(name)) {
        std::cout << "  模板建構子\n";
    }

    Widget(const Widget& other) : name_(other.name_) {
        std::cout << "  複製建構子\n";
    }

    Widget(Widget&& other) noexcept : name_(std::move(other.name_)) {
        std::cout << "  移動建構子\n";
    }
};

int main() {
    Widget w1("Hello");         // 模板建構子 ✅
    Widget w2(w1);              // 複製建構子 ✅（不再被模板搶走）
    Widget w3(std::move(w1));   // 移動建構子 ✅

    const Widget w4("World");
    Widget w5(w4);              // 複製建構子 ✅

    return 0;
}
