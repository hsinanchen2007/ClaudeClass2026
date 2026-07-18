#include <iostream>
#include <cstring>
using namespace std;

class DynamicArray {
private:
    int* data;
    int size;

public:
    DynamicArray(int sz) : size(sz) {
        data = new int[size];  // 建構時分配記憶體, 需要在解構時釋放
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
        cout << "  [建構] 分配了 " << size << " 個 int 的記憶體" << endl;
    }
    
    ~DynamicArray() {
        delete[] data;         // 解構時釋放記憶體, 避免記憶體洩漏
        cout << "  [解構] 釋放了 " << size << " 個 int 的記憶體" << endl;
    }
    
    void set(int index, int value) {
        if (index >= 0 && index < size) {
            data[index] = value;
        }
    }
    
    int get(int index) const {
        if (index >= 0 && index < size) {
            return data[index];
        }
        return -1;
    }
    
    void print() const {
        cout << "  [";
        for (int i = 0; i < size; i++) {
            if (i > 0) cout << ", ";
            cout << data[i];
        }
        cout << "]" << endl;
    }
};

int main() {
    cout << "=== 動態陣列範例 ===" << endl;
    
    {
        DynamicArray arr(5);
        arr.set(0, 10);
        arr.set(1, 20);
        arr.set(2, 30);
        arr.print();
        
        // arr 離開作用域時，解構函數自動釋放記憶體
        // 不需要手動 delete！這就是 C++ 相對於 C 的優勢
        cout << "  --- 即將離開區塊 ---" << endl;
    }
    
    cout << "  --- 記憶體已自動釋放 ---" << endl;
    return 0;
}
// 在這裡，DynamicArray 的解構函數會自動被調用，釋放分配的記憶體