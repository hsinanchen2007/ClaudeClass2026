#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Element {
private:
    string name;
public:
    Element(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    // 拷貝建構函數（vector 操作時會用到）
    // 注意：這裡為了演示，拷貝建構函數會修改 name，讓我們能看到什麼時候被調用
    Element(const Element& other) : name(other.name + "(副本)") {
        cout << "  [拷貝+] " << name << endl;
    }
    ~Element() {
        cout << "  [-] " << name << endl;
    }
};

int main() {
    cout << "=== vector 中的對象生命週期 ===" << endl;
    
    {
        vector<Element> vec;
        cout << "\n--- push_back ---" << endl;
        vec.push_back(Element("A"));   // 臨時對象 → 拷貝進 vector → 臨時對象死亡
        
        cout << "\n--- 再 push_back ---" << endl;
        vec.push_back(Element("B"));   // vector 可能重新分配記憶體
        // 如果重新分配，舊的元素會被拷貝到新位置，然後舊的被解構
        // 注意：實際行為取決於 vector 的實現和當前容量，可能會有多次拷貝和解構
        
        cout << "\n--- 離開區塊 ---" << endl;
    }
    // vector 解構，裡面所有元素也被解構
    // 這段程式碼展示了 C++ 中對象生命週期的複雜性：
    // - 當我們向 vector 中添加元素時，可能會觸發拷貝建構函數，導致對象被多次創建和銷毀。
    
    cout << "\n--- 完成 ---" << endl;
    return 0;
}
