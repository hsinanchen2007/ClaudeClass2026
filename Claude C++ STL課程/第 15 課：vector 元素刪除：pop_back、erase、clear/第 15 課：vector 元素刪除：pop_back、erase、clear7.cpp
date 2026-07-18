#include <vector>
#include <iostream>
#include <string>

struct Item {
    std::string name;
    
    Item(const std::string& n) : name(n) {
        std::cout << "建構 " << name << std::endl;
    }
    
    Item(const Item& other) : name(other.name) {
        std::cout << "複製 " << name << std::endl;
    }
    
    Item(Item&& other) noexcept : name(std::move(other.name)) {
        std::cout << "移動 " << name << std::endl;
    }
    
    Item& operator=(Item&& other) noexcept {
        name = std::move(other.name);
        std::cout << "移動賦值 " << name << std::endl;
        return *this;
    }
    
    ~Item() {
        std::cout << "銷毀 " << name << std::endl;
    }
};

int main() {
    std::vector<Item> v;
    v.reserve(5);
    
    v.emplace_back("A");
    v.emplace_back("B");
    v.emplace_back("C");
    v.emplace_back("D");
    
    std::cout << "\n=== erase B ===" << std::endl;
    v.erase(v.begin() + 1);
    // 觀察：C 和 D 會往前移動，然後最後一個位置的元素被銷毀
    
    std::cout << "\n=== 目前內容 ===" << std::endl;
    for (const auto& item : v) {
        std::cout << item.name << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n=== clear ===" << std::endl;
    v.clear();
    
    std::cout << "\n=== 程式結束 ===" << std::endl;
    return 0;
}
//```可以看到 `erase` 的運作方式：
//1. C 和 D 往前移動賦值
//2. 最後一個位置（已被移走的 D）被銷毀（name 已空）

