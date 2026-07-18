#include <iostream>
#include <vector>

void set_true(bool& b) {
    b = true;
}

int main() {
    std::vector<int> vi = {0, 0, 0};
    int& ref = vi[0];  // OK
    ref = 42;

    std::vector<bool> vb = {false, false, false};
    // bool& ref_b = vb[0];  // 編譯錯誤！
    // vb[0] 是代理物件，不能綁定到 bool&

    // set_true(vb[0]);  // 編譯錯誤！
    // 函數參數是 bool&，不接受代理物件

    return 0;
}
