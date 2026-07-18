#include <iostream>
#include <string>
using namespace std;

class Scope {
private:
    string name;
public:
    Scope(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Scope() {
        cout << "  [-] " << name << endl;
    }
};

int main() {
    cout << "=== 作用域嵌套觀察 ===" << endl;
    
    Scope a("a - main 層");
    
    {   // 第一層區塊
        Scope b("b - 第一層");
        
        {   // 第二層區塊
            Scope c("c - 第二層");
            
            {   // 第三層區塊
                Scope d("d - 第三層");
                cout << "  --- 最深處 ---" << endl;
            }   // d 死亡
            
            cout << "  --- 回到第二層 ---" << endl;
        }   // c 死亡
        
        cout << "  --- 回到第一層 ---" << endl;
    }   // b 死亡
    
    cout << "  --- 回到 main ---" << endl;
    return 0;
}   // a 死亡
