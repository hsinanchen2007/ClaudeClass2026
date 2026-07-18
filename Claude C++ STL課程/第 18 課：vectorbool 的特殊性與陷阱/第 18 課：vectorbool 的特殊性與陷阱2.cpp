#include <iostream>
#include <vector>

int main() {
    const int N = 1000;

    // 普通的 bool 陣列：每個 bool 佔 1 byte
    bool arr[N];
    std::cout << "bool[1000] 大小：" << sizeof(arr) << " bytes" << std::endl;

    // vector<char> 模擬存 bool：每個也佔 1 byte
    std::vector<char> vc(N);
    std::cout << "vector<char>(1000) 容量佔用：約 "
              << vc.capacity() * sizeof(char) << " bytes" << std::endl;

    // vector<bool>：每個 bool 只佔 1 bit
    std::vector<bool> vb(N);
    std::cout << "vector<bool>(1000) 容量佔用：約 "
              << vb.capacity() / 8 << " bytes" << std::endl;

    return 0;
}
