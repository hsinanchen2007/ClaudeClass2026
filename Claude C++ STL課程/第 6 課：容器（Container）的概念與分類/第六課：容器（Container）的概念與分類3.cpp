#include <iostream>
#include <deque>

int main() {
    std::cout << "=== std::deque ===" << std::endl;
    
    std::deque<int> deq;
    
    // 可以在兩端操作
    deq.push_back(30);
    deq.push_front(20);
    deq.push_back(40);
    deq.push_front(10);
    
    std::cout << "元素: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    // 隨機存取
    std::cout << "deq[2]: " << deq[2] << std::endl;
    
    // 兩端刪除
    deq.pop_front();
    deq.pop_back();
    
    std::cout << "刪除頭尾後: ";
    for (int n : deq) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
