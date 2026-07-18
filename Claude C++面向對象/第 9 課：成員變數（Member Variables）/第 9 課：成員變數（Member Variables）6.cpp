#include <iostream>
#include <string>
#include <vector>
using namespace std;

class InitTest {
public:
    // === 不會自動初始化（危險！） ===
    int a;           // 垃圾值
    double b;        // 垃圾值
    bool c;          // 垃圾值
    char d;          // 垃圾值
    int* ptr;        // 野指標！

    // === 會自動初始化（安全） ===
    string name;            // 自動初始化為 ""
    vector<int> numbers;    // 自動初始化為空容器
};

int main() {
    InitTest t;

    cout << "--- 不安全的成員（垃圾值）---" << endl;
    cout << "a   = " << t.a << endl;
    cout << "b   = " << t.b << endl;
    cout << "c   = " << t.c << endl;
    // t.ptr 是野指標，千萬別 dereference！

    cout << "\n--- 安全的成員（自動初始化）---" << endl;
    cout << "name     = [" << t.name << "] (空字串)" << endl;
    cout << "numbers 大小 = " << t.numbers.size() << endl;

    return 0;
}
