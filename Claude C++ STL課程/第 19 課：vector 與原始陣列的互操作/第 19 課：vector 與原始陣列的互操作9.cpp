#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {1, 2, 3};

    int arr[] = {100, 200, 300, 400};

    // assign 會清除原有內容，用新資料取代
    v.assign(std::begin(arr), std::end(arr));

    std::cout << "assign 後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
