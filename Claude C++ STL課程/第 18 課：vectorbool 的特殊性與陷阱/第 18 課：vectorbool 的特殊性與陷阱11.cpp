#include <iostream>
#include <deque>

int main() {
    std::deque<bool> db = {true, false, true};

    // 正常的 bool 引用
    bool& ref = db[0];
    ref = false;
    std::cout << "db[0] = " << db[0] << std::endl;  // 0

    // 可以取位址
    bool* ptr = &db[1];
    std::cout << "*ptr = " << *ptr << std::endl;     // 0

    return 0;
}
