#include <iostream>
#include <string>
using namespace std;

class HeavyObject {
private:
    string data;

public:
    HeavyObject() {
        data = "";
        cout << "  HeavyObject 預設建構" << endl;
    }
    
    HeavyObject(const string& s) {
        data = s;
        cout << "  HeavyObject 帶參建構: " << s << endl;
    }
    
    HeavyObject& operator=(const string& s) {
        data = s;
        cout << "  HeavyObject 賦值: " << s << endl;
        return *this;
    }
};

// 使用函數體賦值
// 缺點：先預設建構，再賦值，效率較低，特別是當 HeavyObject 的建構和賦值都很重時
class ContainerA {
private:
    HeavyObject obj;

public:
    ContainerA(const string& s) {
        // 步驟：先預設建構 obj，再賦值
        cout << "  --- 開始賦值 ---" << endl;
        obj = s;
        cout << "  --- 賦值完成 ---" << endl;
    }
};

// 使用初始化列表
// 優點：直接帶參建構 obj，效率更高，特別是當 HeavyObject 的建構很重時
class ContainerB {
private:
    HeavyObject obj;

public:
    ContainerB(const string& s) 
        : obj(s)  // 直接帶參建構，不經過預設建構
    {
        cout << "  --- 初始化列表完成 ---" << endl;
    }
};

int main() {
    cout << "=== 方式 A：函數體賦值（兩步）===" << endl;
    ContainerA a("Hello");
    
    cout << "\n=== 方式 B：初始化列表（一步）===" << endl;
    ContainerB b("Hello");
    
    return 0;
}
