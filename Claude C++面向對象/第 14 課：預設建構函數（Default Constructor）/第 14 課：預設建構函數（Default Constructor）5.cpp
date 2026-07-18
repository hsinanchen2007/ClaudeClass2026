#include <iostream>
using namespace std;

class A {
public:
    int value;
    A() = default;  // 編譯器生成的版本, 不會初始化 value，仍然是垃圾值
};

class B {
public:
    int value;
    B() { }  // 使用者自己寫的空建構函數, 也不會初始化 value，仍然是垃圾值
};

int main() {
    // === 情境 1：一般宣告 ===
    A a1;    // value 是垃圾值（= default 不初始化基本型別）
    B b1;    // value 也是垃圾值（空建構函數也不初始化）
    
    // === 情境 2：值初始化（Value Initialization）===
    A a2{};  // value 被初始化為 0！因為 A() = default 生成的建構函數會對基本型別進行值初始化
    B b2{};  // value 仍然是垃圾值！因為 B() 是使用者定義的空建構函數，不會對基本型別進行值初始化
    
    cout << "=== 一般宣告 ===" << endl;
    cout << "a1.value = " << a1.value << " (垃圾值)" << endl;
    cout << "b1.value = " << b1.value << " (垃圾值)" << endl;
    
    cout << "\n=== 值初始化 {} ===" << endl;
    cout << "a2.value = " << a2.value << " (應該是 0)" << endl;
    cout << "b2.value = " << b2.value << " (可能仍是垃圾值)" << endl;
    
    return 0;
}
