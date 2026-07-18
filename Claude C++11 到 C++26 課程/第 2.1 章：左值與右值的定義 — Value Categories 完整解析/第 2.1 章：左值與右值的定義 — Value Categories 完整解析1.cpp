#include <iostream>
#include <string>

int main() {
    // ===== 以下全部是左值 =====

    int x = 42;               // x 是左值
    std::string name = "C++"; // name 是左值

    int& ref = x;             // ref 是左值（參考本身是左值）

    int arr[3] = {1, 2, 3};
    arr[0];                    // 陣列元素是左值

    std::cout;                 // std::cout 是左值

    // 字串字面值是左值！（因為它存在於靜態儲存區）
    // "Hello" 的型別是 const char[6]

    // 驗證：左值可以取位址
    std::cout << "&x     = " << &x     << "\n";
    std::cout << "&ref   = " << &ref   << "\n";  // 和 &x 相同
    std::cout << "&arr[0]= " << &arr[0]<< "\n";

    return 0;
}
