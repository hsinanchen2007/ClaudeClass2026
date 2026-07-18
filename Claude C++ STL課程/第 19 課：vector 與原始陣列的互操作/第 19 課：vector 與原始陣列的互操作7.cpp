#include <iostream>
#include <vector>

int main() {
    // C 風格陣列
    int arr[] = {10, 20, 30, 40, 50};
    int arr_size = sizeof(arr) / sizeof(arr[0]);  // 5

    // 方法一：用指標範圍建構
    std::vector<int> v1(arr, arr + arr_size);

    // 方法二：用 std::begin / std::end（C++11）
    std::vector<int> v2(std::begin(arr), std::end(arr));

    std::cout << "v1: ";
    for (int x : v1) std::cout << x << " ";
    std::cout << std::endl;

    std::cout << "v2: ";
    for (int x : v2) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
