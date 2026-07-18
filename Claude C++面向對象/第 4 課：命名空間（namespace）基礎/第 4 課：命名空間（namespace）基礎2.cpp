#include <iostream>
#include <string>

using std::cout;    // 只引入 cout
using std::endl;    // 只引入 endl
using std::string;  // 只引入 string

int main() {
    cout << "Hello" << endl;  // 可以直接使用 cout 和 endl
    string name = "Alice";    // 可以直接使用 string
    
    // 其他成員仍需完整名稱
    std::cin >> name;
    
    return 0;
}
