#include <vector>
#include <array>
#include <iostream>

int main() {
    // 從另一個 vector 複製
    std::vector<int> original = {1, 2, 3, 4, 5};
    std::vector<int> copy1(original);           // 複製建構
    std::vector<int> copy2 = original;          // 同上
    
    // 從迭代器範圍建構
    std::vector<int> partial(original.begin() + 1, original.begin() + 4);
    for (int x : partial) {
        std::cout << x << " ";  // 輸出：2 3 4
    }
    std::cout << std::endl;
    
    // 從 C 風格陣列建構
    int arr[] = {10, 20, 30, 40};
    std::vector<int> from_array(std::begin(arr), std::end(arr));
    // 或者：std::vector<int> from_array(arr, arr + 4);
    
    // 從 std::array 建構
    std::array<int, 3> std_arr = {100, 200, 300};
    std::vector<int> from_std_array(std_arr.begin(), std_arr.end());
    
    return 0;
}
