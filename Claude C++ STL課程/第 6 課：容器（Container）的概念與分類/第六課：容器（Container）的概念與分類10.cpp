#include <iostream>
#include <unordered_set>

int main() {
    std::cout << "=== std::unordered_set ===" << std::endl;
    
    std::unordered_set<int> us;
    
    us.insert(30);
    us.insert(10);
    us.insert(50);
    us.insert(20);
    us.insert(40);
    
    // 順序不固定（取決於雜湊）
    std::cout << "元素（順序不固定）: ";
    for (int n : us) std::cout << n << " ";
    std::cout << std::endl;
    
    // 查找：平均 O(1)
    if (us.find(30) != us.end()) {
        std::cout << "找到 30" << std::endl;
    }
    
    // 雜湊表資訊
    // bucket_count：桶的數量
    // load_factor：負載因子 = 元素數量 / 桶的數量
    // 負載因子越高，碰撞越多，性能可能下降
    // 注意：unordered_set 的元素是無序的，因為它們是根據雜湊值分佈在不同的桶中
    // bucket_count 和 load_factor 是 unordered_set 的內部資訊，可以用來分析性能
    // 注意：不同的實現可能有不同的 bucket_count 和 load_factor，這取決於元素的數量和雜湊函數的效率
    std::cout << "桶的數量: " << us.bucket_count() << std::endl;
    std::cout << "負載因子: " << us.load_factor() << std::endl;
    
    return 0;
}
