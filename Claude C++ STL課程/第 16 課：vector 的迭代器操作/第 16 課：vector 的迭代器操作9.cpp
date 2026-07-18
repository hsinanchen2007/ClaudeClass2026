#include <iostream>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};

    // 1. 拷貝（值語意）—— 不會修改原容器
    std::cout << "值拷貝：" << std::endl;
    for (std::string name : names) {
        name += "!";  // 修改的是拷貝
        std::cout << "  " << name << std::endl;
    }

    // 驗證原容器未被修改
    std::cout << "原容器：" << names[0] << std::endl;  // Alice（沒有 !）

    // 2. 引用 —— 可以修改原容器
    for (std::string& name : names) {
        name += "!";  // 修改原容器中的元素
    }
    std::cout << "修改後：" << names[0] << std::endl;   // Alice!

    // 3. const 引用 —— 不拷貝、不修改（最常用的唯讀方式）
    for (const std::string& name : names) {
        std::cout << name << " ";
        // name += "?";  // 編譯錯誤！
    }
    std::cout << std::endl;

    return 0;
}
