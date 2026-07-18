#include <iostream>
#include <vector>
#include <typeinfo>

int main() {
    std::vector<int> vi = {1, 2, 3};
    std::vector<bool> vb = {true, false, true};

    // vector<int>::operator[] 回傳 int&
    auto ri = vi[0];   // auto 推導為 int
    std::cout << "vi[0] 的型別：" << typeid(ri).name() << std::endl;

    // vector<bool>::operator[] 回傳 reference（代理物件）
    auto rb = vb[0];   // auto 推導為 vector<bool>::reference，不是 bool！
    std::cout << "vb[0] 的型別：" << typeid(rb).name() << std::endl;

    // 大小也不同
    std::cout << "sizeof(ri) = " << sizeof(ri) << std::endl;
    std::cout << "sizeof(rb) = " << sizeof(rb) << std::endl;

    return 0;
}
