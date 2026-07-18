#include <iostream>
#include <string>
using namespace std;

class Probe {
private:
    string name;
public:
    Probe(const string& n) : name(n) {
        cout << "  [誕生] " << name << endl;
    }
    ~Probe() {
        cout << "  [死亡] " << name << endl;
    }
    void hello() const {
        cout << "  [存活] " << name << " 正在工作" << endl;
    }
};

// ====== 1. 靜態存儲期：全域對象 ======
Probe globalObj("全域物件");

void func() {
    // ====== 2. 自動存儲期：局部對象 ======
    Probe localObj("func 局部物件");
    localObj.hello();
    
    // ====== 3. 靜態存儲期：靜態局部對象 ======
    static Probe staticLocal("func 靜態局部物件");
    staticLocal.hello();
}

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    cout << "\n--- 第一次調用 func() ---" << endl;
    func();
    
    cout << "\n--- 第二次調用 func() ---" << endl;
    func();
    
    cout << "\n--- 動態對象 ---" << endl;
    // ====== 4. 動態存儲期：堆上對象 ======
    Probe* heapObj = new Probe("動態物件");
    heapObj->hello();
    delete heapObj;
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// localObj 在這裡被解構
// staticLocal 在 main() 結束後被解構   
// globalObj 在 main() 結束後被解構
