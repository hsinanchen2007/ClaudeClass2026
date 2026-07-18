#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

class Widget {
private:
    string name;
    int* data;

public:
    Widget(const string& n) : name(n) {
        data = new int[10];
        cout << "  [建構] " << name << "：分配了內部資源" << endl;
    }
    
    ~Widget() {
        delete[] data;
        cout << "  [解構] " << name << "：釋放了內部資源" << endl;
    }
};

int main() {
    cout << "=== new vs malloc ===" << endl;
    
    // 正確：使用 new, 會調用建構函數，內部資源被正確初始化
    // 錯誤：使用 malloc, 只分配了記憶體，但建構函數沒被調用，內部資源未初始化
    cout << "\n--- 使用 new ---" << endl;
    Widget* w1 = new Widget("正確的 Widget");
    delete w1;   // 解構函數被調用，內部資源被釋放
    
    // 錯誤示範（概念上）：如果用 malloc, 會導致未定義行為
    cout << "\n--- 如果用 malloc（概念說明）---" << endl;
    cout << "  Widget* w2 = (Widget*)malloc(sizeof(Widget));" << endl;
    cout << "  // 記憶體分配了，但建構函數沒被調用！" << endl;
    cout << "  // name 沒被初始化，data 沒被分配" << endl;
    cout << "  // 使用 w2 會導致未定義行為！" << endl;
    cout << "  // free(w2) 也不會調用解構函數，data 洩漏！" << endl;
    
    // 我們不實際執行 malloc，因為會崩潰
    
    return 0;
}
