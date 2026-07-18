#include <iostream>
#include <stack>
#include <queue>

int main() {
    // stack：後進先出（LIFO）, 使用 push() 添加元素，top() 讀取頂部元素，pop() 移除頂部元素
    std::stack<int> stk;
    stk.push(1);
    stk.push(2);
    stk.push(3);
    
    std::cout << "stack (LIFO): ";
    while (!stk.empty()) {
        std::cout << stk.top() << " ";
        stk.pop();
    }
    std::cout << std::endl;
    
    // queue：先進先出（FIFO）, 使用 push() 添加元素，front() 讀取隊首元素，pop() 移除隊首元素
    std::queue<int> que;
    que.push(1);
    que.push(2);
    que.push(3);
    
    std::cout << "queue (FIFO): ";
    while (!que.empty()) {
        std::cout << que.front() << " ";
        que.pop();
    }
    std::cout << std::endl;
    
    // priority_queue：優先佇列（預設最大值優先）, 使用 push() 添加元素，top() 讀取優先級最高的元素，pop() 移除優先級最高的元素
    std::priority_queue<int> pq;
    pq.push(30);
    pq.push(10);
    pq.push(50);
    pq.push(20);
    
    std::cout << "priority_queue: ";
    while (!pq.empty()) {
        std::cout << pq.top() << " ";
        pq.pop();
    }
    std::cout << std::endl;
    
    return 0;
}
