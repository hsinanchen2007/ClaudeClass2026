#include <iostream>
#include <vector>

int main() {
    std::cout << "=== std::vector ===" << std::endl;
    
    std::vector<int> vec;
    
    // 動態新增
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    
    std::cout << "大小: " << vec.size() << std::endl;
    std::cout << "容量: " << vec.capacity() << std::endl;
    
    // 繼續新增
    vec.push_back(40);
    vec.push_back(50);
    
    std::cout << "新增後大小: " << vec.size() << std::endl;
    std::cout << "新增後容量: " << vec.capacity() << std::endl;
    
    // 存取
    std::cout << "vec[2]: " << vec[2] << std::endl;
    std::cout << "front: " << vec.front() << std::endl;
    std::cout << "back: " << vec.back() << std::endl;
    
    // 刪除最後一個
    vec.pop_back();
    std::cout << "pop_back 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
