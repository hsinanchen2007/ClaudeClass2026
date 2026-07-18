#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;
    int quantity;
    
    Item(const std::string& n, int q) : name(n), quantity(q) {}
};

int main() {
    std::vector<Item> items;
    items.reserve(3);
    
    // C++17：emplace_back 回傳新元素的參考
    Item& apple = items.emplace_back("Apple", 10);
    apple.quantity += 5;  // 直接修改
    
    // 鏈式操作
    items.emplace_back("Banana", 20).quantity *= 2;
    
    for (const auto& item : items) {
        std::cout << item.name << ": " << item.quantity << std::endl;
    }
    // Apple: 15
    // Banana: 40
    
    return 0;
}
