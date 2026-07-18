#include <iostream>
#include <string>
using namespace std;

class TextBuffer {
private:
    string content_;

public:
    TextBuffer(const string& text) : content_(text) {}

    // const 版本：返回 const 引用（只讀）
    // 在 const 成員函數中，this 的類型是 const TextBuffer* const，表示 this 是一個指向 TextBuffer 對象的常量指針，並且指向的對象也是常量（不可修改）
    // 這裡的 getText() 函數有兩個版本：一個是 const 成員函數，返回 const 引用；另一個是非 const 成員函數，返回非 const 引用
    const string& getText() const {
        cout << "  [調用 const 版本]" << endl;
        return content_;
    }

    // 非 const 版本：返回非 const 引用（可讀寫）
    // 在非 const 成員函數中，this 的類型是 TextBuffer* const，表示 this 是一個指向 TextBuffer 對象的常量指針（指針本身不可修改，但指向的對象可以修改）
    // 這裡的 getText() 函數有兩個版本：一個是 const 成員函數，返回 const 引用；另一個是非 const 成員函數，返回非 const 引用
    string& getText() {
        cout << "  [調用非 const 版本]" << endl;
        return content_;
    }

    void print() const {
        cout << "  內容：「" << content_ << "」" << endl;
    }
};

int main() {
    cout << "=== const 重載 ===" << endl;

    // 非 const 對象
    cout << "\n--- 非 const 對象 ---" << endl;
    TextBuffer buf("Hello");
    buf.getText();                   // 調用非 const 版本
    buf.getText() = "Modified!";     // 可以通過引用修改
    buf.print();

    // const 對象
    cout << "\n--- const 對象 ---" << endl;
    const TextBuffer constBuf("ReadOnly");
    constBuf.getText();              // 調用 const 版本
    // constBuf.getText() = "Hack!"; // ❌ 編譯錯誤！返回的是 const 引用
    constBuf.print();

    // const 引用
    cout << "\n--- const 引用 ---" << endl;
    const TextBuffer& ref = buf;
    ref.getText();                   // 調用 const 版本
    ref.print();

    return 0;
}
