#include <iostream>
#include <vector>

int main() {
    // ===== 不使用 reserve =====
    {
        std::vector<int> v;
        int realloc_count = 0;
        size_t prev_cap = 0;

        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "不用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    // ===== 使用 reserve =====
    {
        std::vector<int> v;
        v.reserve(10000);  // 預先配置
        int realloc_count = 0;
        size_t prev_cap = v.capacity();

        for (int i = 0; i < 10000; ++i) {
            v.push_back(i);
            if (v.capacity() != prev_cap) {
                ++realloc_count;
                prev_cap = v.capacity();
            }
        }
        std::cout << "使用 reserve：重新配置 " << realloc_count
                  << " 次，最終容量 " << v.capacity() << std::endl;
    }

    return 0;
}
