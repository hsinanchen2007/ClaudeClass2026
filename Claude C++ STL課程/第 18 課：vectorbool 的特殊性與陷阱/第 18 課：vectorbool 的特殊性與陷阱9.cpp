#include <iostream>
#include <vector>

int main() {
    std::vector<bool> vb = {true, false, true, true, false};

    std::cout << "翻轉前：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    // 翻轉單一元素（透過代理物件的 flip）
    vb[0].flip();
    std::cout << "vb[0].flip()：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    // 翻轉全部（使用 flip 成員函數）
    vb.flip();
    std::cout << "vb.flip()：";
    for (bool b : vb) std::cout << b;
    std::cout << std::endl;

    return 0;
}
