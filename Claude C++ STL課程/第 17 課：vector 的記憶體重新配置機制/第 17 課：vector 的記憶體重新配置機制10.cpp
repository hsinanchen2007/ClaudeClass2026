#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;

    // 大量插入後再刪除
    for (int i = 0; i < 10000; ++i) {
        v.push_back(i);
    }
    std::cout << "插入後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    v.erase(v.begin() + 10, v.end());  // 只保留前 10 個
    std::cout << "刪除後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;
    // capacity 還是很大！

    v.shrink_to_fit();
    std::cout << "shrink 後：size=" << v.size()
              << ", capacity=" << v.capacity() << std::endl;

    return 0;
}
