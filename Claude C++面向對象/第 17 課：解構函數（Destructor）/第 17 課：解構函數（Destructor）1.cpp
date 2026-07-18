#include <iostream>
#include <string>
using namespace std;

class SimpleObject {
private:
    string name;

public:
    SimpleObject(const string& n) : name(n) {
        cout << "  [建構] " << name << " 被創建了" << endl;
    }
    
    ~SimpleObject() {
        cout << "  [解構] " << name << " 被銷毀了" << endl;
    }
};

int main() {
    cout << "=== main() 開始 ===" << endl;
    
    SimpleObject a("物件A");
    SimpleObject b("物件B");
    
    cout << "=== main() 結束 ===" << endl;
    return 0;
}
// a 和 b 在這裡離開作用域，解構函數被自動調用
