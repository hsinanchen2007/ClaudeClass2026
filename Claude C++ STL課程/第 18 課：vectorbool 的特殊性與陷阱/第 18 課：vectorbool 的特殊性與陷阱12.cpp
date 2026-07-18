#include <iostream>
#include <vector>
#include <cstdint>

int main() {
    // 用 char 或 uint8_t 代替 bool
    std::vector<uint8_t> vb = {1, 0, 1, 1, 0};

    uint8_t& ref = vb[0];  // 正常的引用
    uint8_t* ptr = &vb[0]; // 正常的指標
    uint8_t* data = vb.data(); // data() 可用

    // 與 C API 互操作也沒問題
    std::cout << "data[2] = " << static_cast<int>(data[2]) << std::endl;

    return 0;
}
