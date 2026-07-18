#include <vector>
#include <iostream>

int main() {
    std::vector<std::vector<int>> vv;
    
    // 想要加入一個含有 5 個元素的 vector
    // 錯誤理解：emplace_back(5) 會建立 vector<int>(5)，即 5 個 0
    vv.emplace_back(5);
    
    std::cout << "內層 size: " << vv[0].size() << std::endl;  // 5
    std::cout << "內層內容: ";
    for (int x : vv[0]) std::cout << x << " ";  // 0 0 0 0 0
    std::cout << std::endl;
    
    // 如果想要一個元素是 5 的 vector？
    vv.push_back({5});  // 這樣才對
    
    std::cout << "第二個內層 size: " << vv[1].size() << std::endl;  // 1
    std::cout << "第二個內層內容: ";
    for (int x : vv[1]) std::cout << x << " ";  // 5
    std::cout << std::endl;
    
    return 0;
}
