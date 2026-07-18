#include <iostream>
#include <map>
#include <string>

int main() {
    std::cout << "=== std::map ===" << std::endl;
    
    std::map<std::string, int> ages;
    
    // 插入方式一：operator[]
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    
    // 插入方式二：insert
    ages.insert({"Charlie", 35});
    ages.insert(std::make_pair("Diana", 28));
    
    // 按鍵排序
    std::cout << "所有人（按名字排序）:" << std::endl;
    for (const auto& pair : ages) {
        std::cout << "  " << pair.first << ": " << pair.second << std::endl;
    }
    
    // 查找
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        std::cout << "Bob 的年齡: " << it->second << std::endl;
    }
    
    // operator[] 的陷阱：會插入預設值！
    // 如果查找不存在的鍵，會插入一個預設值（對於 int 是 0）
    // 這裡會插入 Eve = 0，因為 Eve 不存在
    // 注意：這會改變 map 的內容，因為 operator[] 會插入預設值
    std::cout << "Eve 的年齡: " << ages["Eve"] << std::endl;  // 插入 Eve = 0
    std::cout << "現在的大小: " << ages.size() << std::endl;
    
    return 0;
}
