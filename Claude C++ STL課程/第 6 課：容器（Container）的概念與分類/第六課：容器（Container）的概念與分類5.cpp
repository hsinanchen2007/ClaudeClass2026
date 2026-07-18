#include <iostream>
#include <forward_list>

int main() {
    std::cout << "=== std::forward_list ===" << std::endl;
    
    std::forward_list<int> flst = {20, 30, 40};
    
    // 只能在前端插入
    flst.push_front(10);
    
    std::cout << "元素: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 在某個位置「之後」插入
    auto it = flst.begin();  // 指向 10
    flst.insert_after(it, 15);  // 在 10 之後插入 15
    
    std::cout << "insert_after 後: ";
    for (int n : flst) std::cout << n << " ";
    std::cout << std::endl;
    
    // 注意：forward_list 沒有 size() 成員函數！
    // 需要用 std::distance 計算
    auto count = std::distance(flst.begin(), flst.end());
    std::cout << "元素個數: " << count << std::endl;
    
    return 0;
}
