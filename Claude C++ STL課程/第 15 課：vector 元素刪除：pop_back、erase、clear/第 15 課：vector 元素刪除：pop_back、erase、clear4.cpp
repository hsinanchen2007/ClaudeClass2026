#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    
    auto it = v.erase(v.begin() + 2);  // 刪除 30
    
    std::cout << "刪除後 it 指向: " << *it << std::endl;  // 40
    
    // 如果刪除的是最後一個元素，回傳 end()
    it = v.erase(v.end() - 1);  // 刪除 50
    if (it == v.end()) {
        std::cout << "it 現在是 end()" << std::endl;
    }
    
    return 0;
}
