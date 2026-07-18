#include <iostream>
#include <string>
#include <utility>

int main() {
    // ===== 情境 1：移動確實發生 =====
    std::cout << "--- 情境 1：有移動建構子 ---\n";
    std::string a = "Hello";
    std::string b = std::move(a);  // string 有移動建構子 → 移動發生
    std::cout << "a = \"" << a << "\"\n";  // 空

    // ===== 情境 2：const 物件，移動不會發生 =====
    std::cout << "\n--- 情境 2：const 物件 ---\n";
    const std::string c = "World";
    std::string d = std::move(c);  // std::move(c) 型別是 const string&&
                                    // 匹配到複製建構子（不是移動建構子）
    std::cout << "c = \"" << c << "\"\n";  // 仍然是 "World"！

    // ===== 情境 3：基本型別，移動 = 複製 =====
    std::cout << "\n--- 情境 3：基本型別 ---\n";
    int x = 42;
    int y = std::move(x);  // int 沒有「資源」可搬，移動就是複製
    std::cout << "x = " << x << "\n";  // 仍然是 42

    return 0;
}
