#include <iostream>
#include <stack>

int main() {
    std::cout << "=== std::stack ===" << std::endl;
    
    std::stack<int> stk;
    
    // 只能從頂端操作
    stk.push(10);
    stk.push(20);
    stk.push(30);
    
    std::cout << "頂端: " << stk.top() << std::endl;
    std::cout << "大小: " << stk.size() << std::endl;
    
    // 依序取出（後進先出）
    std::cout << "依序取出: ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
