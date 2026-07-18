#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;

    std::cout << "初始狀態：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    for (int i = 1; i <= 10; ++i) {
        // 記錄 push_back 之前的狀態
        size_t old_cap = v.capacity();
        const int* old_data = v.data();  // 底層陣列的位址

        v.push_back(i);

        if (v.capacity() != old_cap) {
            std::cout << "--- 重新配置！---" << std::endl;
            std::cout << "  舊容量: " << old_cap
                      << ", 新容量: " << v.capacity() << std::endl;
            std::cout << "  舊位址: " << old_data
                      << ", 新位址: " << v.data() << std::endl;
        }

        std::cout << "push_back(" << i << "): size=" << v.size()
                  << ", capacity=" << v.capacity() << std::endl;
    }

    return 0;
}
