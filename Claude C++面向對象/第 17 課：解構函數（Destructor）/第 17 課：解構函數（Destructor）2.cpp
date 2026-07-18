#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string name;

public:
    Tracker(const string& n) : name(n) {
        cout << "  [建構] " << name << endl;
    }
    
    ~Tracker() {
        cout << "  [解構] " << name << endl;
    }
};

// ====== 全域對象 ======
Tracker globalObj("全域物件");

void testFunction() {
    cout << "\n--- 進入 testFunction ---" << endl;
    Tracker funcObj("函數局部物件");
    cout << "--- 離開 testFunction ---" << endl;
}  // funcObj 在這裡被解構

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    // ====== 局部對象 ======
    Tracker localObj("局部物件");
    
    // ====== 區塊內對象 ======
    {
        cout << "\n--- 進入區塊 ---" << endl;
        Tracker blockObj("區塊物件");
        cout << "--- 離開區塊 ---" << endl;
    }  // blockObj 在這裡被解構
    
    cout << "\n--- 區塊已結束 ---" << endl;
    
    // ====== 函數調用 ======
    testFunction();
    
    // ====== 動態對象 ======
    cout << "\n--- 動態對象 ---" << endl;
    Tracker* heapObj = new Tracker("動態物件");
    cout << "--- 手動 delete ---" << endl;
    delete heapObj;  // 解構函數在 delete 時調用
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
// localObj 在這裡被解構
// globalObj 在 main() 結束後被解構
