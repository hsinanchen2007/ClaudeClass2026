#include <iostream>
#include <vector>
#include <cstring>  // memcpy

// 模擬一個 C 函數：把結果寫入 buffer
void fill_buffer(int* buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer[i] = (i + 1) * 100;
    }
}

int main() {
    int count = 5;

    // 步驟 1：建立足夠大小的 vector
    std::vector<int> v(count);  // 5 個元素，初始為 0

    // 步驟 2：讓 C 函數直接寫入 vector 的底層記憶體
    fill_buffer(v.data(), count);

    // 步驟 3：vector 已經擁有資料了
    std::cout << "結果：";
    for (int x : v) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    return 0;
}
