#include <iostream>
#include <queue>

int main() {
    std::cout << "=== std::queue ===" << std::endl;
    
    std::queue<int> que;
    
    // 從尾端加入
    que.push(10);
    que.push(20);
    que.push(30);
    
    std::cout << "前端: " << que.front() << std::endl;
    std::cout << "後端: " << que.back() << std::endl;
    
    // 依序取出（先進先出）
    std::cout << "依序取出: ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();  // 從前端移除
    }
    std::cout << std::endl;
    
    return 0;
}
