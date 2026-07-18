#include <iostream>
#include <vector>

int main() {
    int arr[] = {10, 20, 30, 40, 50, 60, 70, 80};

    // 只複製索引 2~5 的元素（30, 40, 50, 60）
    std::vector<int> v(arr + 2, arr + 6);

    std::cout << "部分複製：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
