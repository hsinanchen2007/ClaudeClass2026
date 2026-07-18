#include <iostream>
#include <bitset>

int main() {
    // 大小必須是編譯期常數
    std::bitset<8> bs;

    bs[0] = 1;
    bs[3] = 1;
    bs[7] = 1;

    std::cout << "bs = " << bs << std::endl;
    std::cout << "count（1 的個數）= " << bs.count() << std::endl;

    bs.flip();
    std::cout << "flip 後：" << bs << std::endl;

    // 位元運算
    std::bitset<8> bs2("11001100");
    std::cout << "bs AND bs2 = " << (bs & bs2) << std::endl;

    return 0;
}
