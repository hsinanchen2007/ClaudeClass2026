#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Widget {
    std::string name_;

public:
    // 使用 enable_if 排除 Widget 本身，避免模板搶走複製/移動建構子
    template<typename T,
             std::enable_if_t<!std::is_same_v<std::decay_t<T>, Widget>, int> = 0>
    Widget(T&& name) : name_(std::forward<T>(name)) {
        std::cout << "  模板建構子\n";
    }

    // 現在編譯器生成的複製建構子不會被模板搶走
    Widget(const Widget& other) : name_(other.name_) {
        std::cout << "  複製建構子\n";
    }
};

int main() {
    Widget w1("Hello");        // OK：T = const char*

    const Widget w2(w1);       // OK：呼叫複製建構子（const Widget&）

    Widget w3(w1);             // 問題！w1 是非 const Widget
                                // 模板建構子 T = Widget& 比
                                // 複製建構子 Widget(const Widget&) 更精確匹配
                                // → 呼叫模板建構子，試圖用 Widget 建構 string → 編譯錯誤！

    return 0;
}
