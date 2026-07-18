#include <iostream>
#include <string>
using namespace std;

class LifeCycle {
private:
    string name;
    static int count;  // 追蹤當前存活的物件數量

public:
    LifeCycle(const string& n) : name(n) {
        count++;
        cout << "  [建構] " << name 
             << " (目前存活: " << count << " 個)" << endl;
    }
    
    ~LifeCycle() {
        cout << "  [解構] " << name 
             << " (目前存活: " << count - 1 << " 個)" << endl;
        count--;
    }
    
    static int getCount() { return count; }
};

int LifeCycle::count = 0;  // 靜態成員初始化

int main() {
    cout << "=============================" << endl;
    cout << "  物件生命週期觀察" << endl;
    cout << "=============================" << endl;
    
    LifeCycle a("Alpha");
    
    {
        LifeCycle b("Beta");
        LifeCycle c("Charlie");
        
        cout << "\n  --- 區塊內：存活 " 
             << LifeCycle::getCount() << " 個 ---\n" << endl;
    }
    // b 和 c 在這裡被解構
    
    cout << "\n  --- 區塊外：存活 " 
         << LifeCycle::getCount() << " 個 ---\n" << endl;
    
    LifeCycle d("Delta");
    
    cout << "\n  --- main() 即將結束 ---" << endl;
    return 0;
}
// d 和 a 在這裡被解構
