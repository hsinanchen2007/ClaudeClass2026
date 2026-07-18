#include <iostream>
#include <string>

int main() {
    // ===== 左值參考（複習）=====
    int a = 10;
    int& lref = a;        // OK：左值參考綁定到左值
    // int& lref2 = 42;   // 錯誤！左值參考不能綁定到右值

    const int& clref = 42; // OK：const 左值參考可以綁定右值（特殊規則）

    // ===== 右值參考（C++11 新增）=====
    int&& rref = 42;       // OK：右值參考綁定到右值
    // int&& rref2 = a;   // 錯誤！右值參考不能綁定到左值

    std::string&& sref = std::string("Hello");  // OK：綁定到臨時物件

    // 右值參考綁定後，會延長臨時物件的生命週期
    std::cout << "rref  = " << rref  << "\n";   // 42
    std::cout << "sref  = " << sref  << "\n";   // Hello

    // 右值參考本身是左值！（有名字、有地址）
    rref = 100;            // 可以修改
    std::cout << "rref  = " << rref  << "\n";   // 100
    std::cout << "&rref = " << &rref << "\n";   // 可以取址

    return 0;
}
