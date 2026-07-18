#include <iostream>
using namespace std;

class Resource {
private:
    int id;

public:
    Resource(int i) : id(i) {
        cout << "  [建構] 資源 #" << id << endl;
    }
    
    ~Resource() {
        cout << "  [解構] 資源 #" << id << endl;
    }
};

int main() {
    cout << "=== 正確使用 delete ===" << endl;
    Resource* r1 = new Resource(1);
    delete r1;    // 解構函數被調用，資源被釋放
    
    cout << "\n=== 忘記 delete ===" << endl;
    Resource* r2 = new Resource(2);
    // 沒有 delete r2！
    // 解構函數永遠不會被調用！
    // 記憶體永遠不會被釋放！
    
    cout << "\n=== 局部對象（自動管理）===" << endl;
    {
        Resource r3(3);
        // 不需要 delete，離開作用域自動解構
    }
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
    // r2 指向的記憶體洩漏了！
    // r3 在這裡被解構，資源 #3 被釋放
    // r1 在這裡已經被解構了，資源 #1 被釋放
    // main() 結束後，r2 指向的記憶體仍然存在，但無法訪問，造成記憶體洩漏
}
