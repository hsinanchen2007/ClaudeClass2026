#include <iostream>
#include <vector>

void process(const double* arr, size_t size) {
    if (size == 0) {
        std::cout << "空陣列，不處理" << std::endl;
        return;
    }
    for (size_t i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<double> v;  // 空的

    // 安全的寫法：data() + size()
    process(v.data(), v.size());

    v = {1.1, 2.2, 3.3};
    process(v.data(), v.size());

    return 0;
}
