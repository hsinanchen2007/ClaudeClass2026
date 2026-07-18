#include <iostream>
using namespace std;

class Item {
public:
    int id;
    Item(int i) : id(i) {
        cout << "  [+] Item #" << id << endl;
    }
    ~Item() {
        cout << "  [-] Item #" << id << endl;
    }
};

Item* dangerousPointer() {
    Item local(99);
    return &local;    // 返回局部對象的地址——危險！
}   // local 死亡，返回的指標成為野指標, 使用它會導致未定義行為！
// 這段程式碼展示了 C++ 中對象生命週期的陷阱：
// - dangerousPointer() 函數返回了一個局部對象 local 的地址，但 local 在函數結束時就被解構了，
// 因此返回的指標成為野指標，使用它會導致未定義行為！


int main() {
    cout << "=== 陷阱：野指標 ===" << endl;
    
    // Item* ptr = dangerousPointer();
    // cout << ptr->id << endl;  // 未定義行為！
    
    // 正確做法：用 new 分配
    cout << "\n=== 正確做法 ===" << endl;
    Item* safePtr = new Item(99);
    cout << "  safePtr->id = " << safePtr->id << endl;
    delete safePtr;
    
    return 0;
}
