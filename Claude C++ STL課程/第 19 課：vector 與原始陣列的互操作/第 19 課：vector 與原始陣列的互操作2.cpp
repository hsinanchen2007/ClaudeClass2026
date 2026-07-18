#include <iostream>
#include <vector>

void c_library_function(const int* arr, int size) {
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // C++03 風格：取第一個元素的位址
    int* ptr = &v[0];
    c_library_function(ptr, v.size());

    // 注意：空 vector 時 &v[0] 是未定義行為！
    std::vector<int> empty_v;
    // int* bad_ptr = &empty_v[0];  // 危險！v[0] 本身就越界了

    // data() 在空 vector 時回傳有效（但不可解參考）的指標或 nullptr
    int* safe_ptr = empty_v.data();  // 安全
    std::cout << "empty.data() = " << safe_ptr << std::endl;

    return 0;
}
