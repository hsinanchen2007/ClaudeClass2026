#include <iostream>
#include <string>
using namespace std;

class Box {
private:
    string label;
    
public:
    Box() {
        label = "空盒子";
        cout << "建構函數: 創建了 [" << label << "]" << endl;
    }
    
    string getLabel() const { return label; }
};

// ====== 全域對象 ======
Box globalBox;  // 程式啟動時，在 main() 之前就建構

int main() {
    cout << "\n=== main() 開始 ===" << endl;
    
    // ====== 局部對象 ======
    Box localBox;  // 執行到這一行時建構
    
    cout << "\n=== 進入區塊 ===" << endl;
    {
        // ====== 區塊內的對象 ======
        Box blockBox;  // 進入這個區塊時建構
        cout << "區塊內..." << endl;
    }  // blockBox 在這裡離開作用域
    
    cout << "\n=== 動態對象 ===" << endl;
    // ====== 動態分配的對象 ======
    Box* heapBox = new Box();  // new 的時候建構
    delete heapBox;  // 記得釋放
    
    cout << "\n=== main() 結束 ===" << endl;
    return 0;
}
