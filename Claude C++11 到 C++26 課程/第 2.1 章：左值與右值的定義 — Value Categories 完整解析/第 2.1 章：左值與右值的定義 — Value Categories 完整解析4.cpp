#include <iostream>
#include <type_traits>
#include <string>
#include <utility>

// 利用模板推導規則判斷值類別
template<typename T>
struct value_category {
    static constexpr const char* value = "prvalue";
};

template<typename T>
struct value_category<T&> {
    static constexpr const char* value = "lvalue";
};

template<typename T>
struct value_category<T&&> {
    static constexpr const char* value = "xvalue";
};

// 巨集：印出表達式的值類別
#define PRINT_VALUE_CATEGORY(expr) \
    std::cout << #expr << " is " \
              << value_category<decltype((expr))>::value << "\n"

std::string make_string() { return "temp"; }
std::string& get_ref(std::string& s) { return s; }

int main() {
    int x = 42;
    int& r = x;
    std::string s = "Hello";

    PRINT_VALUE_CATEGORY(x);              // lvalue
    PRINT_VALUE_CATEGORY(r);              // lvalue
    PRINT_VALUE_CATEGORY(42);             // prvalue
    PRINT_VALUE_CATEGORY(x + 1);          // prvalue
    PRINT_VALUE_CATEGORY(std::move(x));   // xvalue
    PRINT_VALUE_CATEGORY(make_string());  // prvalue
    PRINT_VALUE_CATEGORY(get_ref(s));     // lvalue
    PRINT_VALUE_CATEGORY(std::move(s));   // xvalue

    return 0;
}
