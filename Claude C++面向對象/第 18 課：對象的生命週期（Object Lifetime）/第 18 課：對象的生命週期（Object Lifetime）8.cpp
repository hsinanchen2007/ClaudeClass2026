#include <iostream>
#include <string>
using namespace std;

class Data {
public:
    int value;
    Data(int v) : value(v) {
        cout << "  [+] Data(" << value << ")" << endl;
    }
    ~Data() {
        cout << "  [-] Data(" << value << ")" << endl;
    }
};

// 危險！返回局部對象的引用
Data& dangerous() {
    Data local(42);       // local 是局部對象
    return local;         // 返回 local 的引用
}   // local 在這裡死亡！返回的引用指向已死的對象！
// 這段程式碼展示了 C++ 中對象生命週期的陷阱：
// - dangerous() 函數返回了一個局部對象 local 的引用，但 local 在函數結束時就被解構了，因此返回的引用指向一個已經死亡的對象，這會導致未定義行為。

// 安全：返回值（複製）
Data safe() {
    Data local(42);
    return local;         // 返回副本（編譯器可能優化掉複製）
                          // local 在這裡死亡，但返回的是副本，所以不會有問題
}

int main() {
    cout << "=== 陷阱：懸空引用 ===" << endl;
    
    // Data& ref = dangerous();   // 未定義行為！
    // cout << ref.value << endl;  // 可能印出垃圾值或崩潰
    // 編譯器通常會警告：returning reference to local variable
    
    cout << "\n=== 安全：返回值 ===" << endl;
    Data d = safe();
    cout << "  d.value = " << d.value << endl;  // OK
    
    return 0;
}
