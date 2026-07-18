#include <iostream>
#include <vector>

// 模擬一個 C 風格的函式庫函數
void c_library_function(const int* arr, int size) {
    std::cout << "C 函數收到：";
    for (int i = 0; i < size; ++i) {
        std::cout << arr[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    // data() 回傳底層陣列的指標
    // 如果 vector 是 const，回傳 const T*
    // 如果 vector 非 const，回傳 T*
    int* ptr = v.data();
    const int* cptr = v.data();  // 隱式轉為 const

    std::cout << "v.data() = " << ptr << std::endl;
    std::cout << "v.data()[0] = " << ptr[0] << std::endl;
    std::cout << "v.data()[4] = " << ptr[4] << std::endl;

    // 傳給 C 函數
    c_library_function(v.data(), v.size());

    return 0;
}
