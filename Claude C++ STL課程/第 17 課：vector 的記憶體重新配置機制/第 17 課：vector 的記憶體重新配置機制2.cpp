#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;

    size_t prev_cap = 0;
    std::cout << "容量成長序列：" << std::endl;

    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        if (v.capacity() != prev_cap) {
            if (prev_cap > 0) {
                double ratio = static_cast<double>(v.capacity()) / prev_cap;
                std::cout << "  " << prev_cap << " -> " << v.capacity()
                          << " (倍率: " << ratio << ")" << std::endl;
            }
            prev_cap = v.capacity();
        }
    }

    return 0;
}
