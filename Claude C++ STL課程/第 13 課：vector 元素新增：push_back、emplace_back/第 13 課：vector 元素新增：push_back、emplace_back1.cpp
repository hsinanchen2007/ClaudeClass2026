#include <vector>
#include <iostream>
#include <string>

int main() {
    std::vector<int> v;
    
    // 基本用法
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    
    for (int x : v) {
        std::cout << x << " ";  // 10 20 30
    }
    std::cout << std::endl;
    
    // 對於字串
    std::vector<std::string> words;
    
    std::string hello = "Hello";
    words.push_back(hello);           // 複製 hello 進入 vector
    words.push_back("World");         // 從字面量建立臨時物件，然後移動進入
    words.push_back(std::move(hello)); // 移動 hello 進入 vector
    
    std::cout << "hello 變成: \"" << hello << "\"" << std::endl;  // 可能是空的
    
    for (const auto& w : words) {
        std::cout << w << " ";  // Hello World Hello
    }
    std::cout << std::endl;
    
    return 0;
}
