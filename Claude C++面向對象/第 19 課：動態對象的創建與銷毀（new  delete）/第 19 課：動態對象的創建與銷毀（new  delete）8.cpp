#include <iostream>
#include <string>
using namespace std;

// ============================================================
// 一個簡單的動態字串類別（模擬 std::string 的核心概念）
// ============================================================
class MyString {
private:
    char* data;
    int length;

public:
    // 建構：分配記憶體, 複製字串
    MyString(const char* str = "") {
        length = 0;
        while (str[length] != '\0') length++;
        
        data = new char[length + 1];   // +1 給 '\0'
        for (int i = 0; i <= length; i++) {
            data[i] = str[i];
        }
        
        cout << "  [建構] MyString: \"" << data << "\" (長度: " 
             << length << ")" << endl;
    }
    
    // 解構：自動釋放記憶體, 避免洩漏
    ~MyString() {
        cout << "  [解構] MyString: \"" << data << "\"" << endl;
        delete[] data;     // 自動清理！
        data = nullptr;
    }
    
    void print() const {
        cout << "  \"" << data << "\"" << endl;
    }
    
    int getLength() const { return length; }
};

// 不管怎麼離開這個函數，MyString 都會自動清理
void safeFunction(bool earlyReturn) {
    MyString greeting("Hello, C++!");
    
    if (earlyReturn) {
        cout << "  提前返回..." << endl;
        return;   // greeting 自動解構，記憶體自動釋放, 不會洩漏
    }
    
    greeting.print();
    // greeting 在函數結束時自動解構. 不管是正常結束還是提前返回，都不會洩漏記憶體
}

int main() {
    cout << "=== 用局部對象管理記憶體 ===" << endl;
    
    cout << "\n--- 正常流程 ---" << endl;
    safeFunction(false);
    
    cout << "\n--- 提前返回 ---" << endl;
    safeFunction(true);
    
    cout << "\n--- 異常安全 ---" << endl;
    try {
        MyString msg("即將拋出異常");
        throw runtime_error("boom!");
        // msg 在異常傳播過程中自動解構（堆疊展開），記憶體自動釋放，不會洩漏
    } catch (...) {
        cout << "  異常已捕獲，記憶體已自動清理" << endl;
    }
    
    cout << "\n=== 完成 ===" << endl;
    return 0;
}
