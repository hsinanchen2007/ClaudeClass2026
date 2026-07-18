#include <iostream>
#include <string>
#include <vector>
#include <utility>

int main() {
    std::string s = "Hello";

    auto&& ref1 = s;                    // s 是左值 → auto = string& → ref1 是 string&
    auto&& ref2 = std::string("tmp");   // 右值 → auto = string → ref2 是 string&&

    // 實用場景：range-based for 中保持原始型別
    std::vector<std::string> vec = {"a", "b", "c"};

    for (auto&& elem : vec) {
        // elem 的型別是 string&（因為 vec 的元素是左值）
        // 如果 vec 回傳的是臨時值，elem 就會是 string&&
    }

    return 0;
}
