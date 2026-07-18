#include <iostream>
#include <vector>

int main() {
    // 模擬從 C 函數收到動態配置的陣列
    int size = 5;
    int* dynamic_arr = new int[size];
    for (int i = 0; i < size; ++i) {
        dynamic_arr[i] = (i + 1) * 11;
    }

    // 複製到 vector，接管資料的生命週期
    std::vector<int> v(dynamic_arr, dynamic_arr + size);

    // 釋放原始陣列（vector 有自己的副本）
    delete[] dynamic_arr;
    dynamic_arr = nullptr;

    // vector 的資料獨立存在
    std::cout << "vector 內容：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
