#include <vector>
#include <iostream>
#include <string>

int main() {
    std::vector<std::string> lines = {
        "Line 1",
        "Line 2",
        "// INSERT HERE",
        "Line 3",
        "Line 4"
    };
    
    // 找到標記並插入新內容
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (*it == "// INSERT HERE") {
            it = lines.insert(it, "New Content");
            ++it;  // 跳過剛插入的元素，指向標記
            it = lines.erase(it);  // 刪除標記
            --it;  // 調整迭代器，因為 erase 後 it 指向下一個元素
            break;
        }
    }
    
    for (const auto& line : lines) {
        std::cout << line << std::endl;
    }
    
    return 0;
}
