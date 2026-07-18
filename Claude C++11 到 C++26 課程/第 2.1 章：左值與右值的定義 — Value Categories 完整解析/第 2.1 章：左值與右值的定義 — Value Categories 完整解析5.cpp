#include <iostream>
#include <string>
#include <utility>
#include <vector>

void process(std::string s) {
    std::cout << "  processing: " << s << "\n";
}

int main() {
    std::string original = "This is a long string that lives on the heap";

    std::cout << "=== 傳入左值（觸發複製）===\n";
    process(original);  // original 是左值 → 複製
    std::cout << "original after copy: " << original << "\n\n";

    std::cout << "=== 傳入右值（觸發移動）===\n";
    process(std::move(original));  // std::move(original) 是 xvalue → 移動
    std::cout << "original after move: \"" << original << "\"\n";

    return 0;
}
