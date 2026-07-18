#include <iostream>
#include <string>
using namespace std;

class Hero {
private:
    string name;
    int level;

public:
    Hero(const string& n, int lv) : name(n), level(lv) {
        cout << "  [建構] " << name << " Lv." << level << endl;
    }
    
    ~Hero() {
        cout << "  [解構] " << name << " Lv." << level << endl;
    }
    
    void print() const {
        cout << "  " << name << " (Lv." << level << ")" << endl;
    }
};

int main() {
    cout << "=== new 與 delete 基本用法 ===" << endl;
    
    // ====== 基本型別 ======
    cout << "\n--- 基本型別 ---" << endl;
    int* p1 = new int;          // 分配一個 int（未初始化）
    int* p2 = new int(42);      // 分配一個 int 並初始化為 42
    int* p3 = new int{100};     // C++11 大括號初始化
    
    cout << "  *p1 = " << *p1 << " (垃圾值)" << endl;
    cout << "  *p2 = " << *p2 << endl;
    cout << "  *p3 = " << *p3 << endl;
    
    delete p1;
    delete p2;
    delete p3;
    
    // ====== 類別物件 ======
    cout << "\n--- 類別物件 ---" << endl;
    Hero* hero = new Hero("勇者", 10);   // new = 分配記憶體 + 調用建構函數, 返回指向物件的指標
    hero->print();
    delete hero;                          // delete = 調用解構函數 + 釋放記憶體, 釋放後指標變成懸空指標（dangling pointer）
    hero = nullptr;                       // 將指標設為 nullptr，避免懸空指標
    
    cout << "\n--- 完成 ---" << endl;
    return 0;
}
