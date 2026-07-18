#include <iostream>
#include <queue>
#include <vector>

int main() {
    std::cout << "=== std::priority_queue ===" << std::endl;
    
    // 預設是最大堆（最大的在頂端）
    std::priority_queue<int> max_pq;
    
    max_pq.push(30);
    max_pq.push(10);
    max_pq.push(50);
    max_pq.push(20);
    max_pq.push(40);
    
    std::cout << "最大堆依序取出: ";
    while (!max_pq.empty()) {
        std::cout << max_pq.top() << " ";
        max_pq.pop();
    }
    std::cout << std::endl;
    
    // 最小堆
    std::priority_queue<int, std::vector<int>, std::greater<int>> min_pq;
    
    min_pq.push(30);
    min_pq.push(10);
    min_pq.push(50);
    min_pq.push(20);
    min_pq.push(40);
    
    std::cout << "最小堆依序取出: ";
    while (!min_pq.empty()) {
        std::cout << min_pq.top() << " ";
        min_pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
