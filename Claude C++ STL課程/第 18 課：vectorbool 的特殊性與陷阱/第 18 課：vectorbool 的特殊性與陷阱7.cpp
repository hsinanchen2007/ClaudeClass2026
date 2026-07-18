#include <iostream>
#include <vector>

int main() {
    std::vector<int> vi = {10, 20, 30};
    int* data_i = vi.data();  // OK，取得底層陣列指標
    std::cout << "vi.data()[0] = " << data_i[0] << std::endl;

    std::vector<bool> vb = {true, false, true};
    // bool* data_b = vb.data();  // 編譯錯誤！
    // vector<bool> 沒有 data() 成員函數
    // 因為底層不是 bool 陣列，而是位元壓縮的結構

    return 0;
}
