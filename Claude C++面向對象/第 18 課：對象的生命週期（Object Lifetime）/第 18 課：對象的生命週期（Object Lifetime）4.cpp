#include <iostream>
#include <string>
using namespace std;

class ExpensiveResource {
private:
    string name;
    
public:
    ExpensiveResource(const string& n) : name(n) {
        cout << "  [建構] " << name << "（模擬耗時初始化...）" << endl;
    }
    
    ~ExpensiveResource() {
        cout << "  [解構] " << name << endl;
    }
    
    void use() const {
        cout << "  [使用] " << name << endl;
    }
};

ExpensiveResource& getResource() {
    // 靜態局部對象：只在第一次調用時建構
    // C++11 保證這個初始化是線程安全的
    // 注意：這裡的資源名稱可以是固定的，因為它只建構一次
    static ExpensiveResource resource("共享資源");
    return resource;
}

int main() {
    cout << "=== 延遲初始化展示 ===" << endl;
    
    cout << "\n--- 程式啟動，但還沒使用資源 ---" << endl;
    cout << "  (注意：資源還沒被建構)\n" << endl;
    
    cout << "--- 第一次調用 getResource() ---" << endl;
    getResource().use();    // 第一次：觸發建構, 然後使用
    
    cout << "\n--- 第二次調用 getResource() ---" << endl;
    getResource().use();    // 第二次：不再建構，直接使用, 因為已經存在了
    
    cout << "\n--- 第三次調用 getResource() ---" << endl;
    getResource().use();    // 第三次：同上, 仍然使用同一個已建構的資源
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// 程式結束時，靜態局部對象才被解構
