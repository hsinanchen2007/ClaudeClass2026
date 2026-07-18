#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string label_;

public:
    Tracker(const string& label) : label_(label) {
        cout << "  [建構] " << label_ << endl;
    }
    ~Tracker() {
        cout << "  [解構] " << label_ << endl;
    }

    void ping() const {
        cout << "  " << label_ << " 存活中" << endl;
    }
};

class MyClass {
public:
    // 靜態成員：程式開始時初始化
    // 靜態成員變數屬於類別本身，而不是任何特定的對象
    // 這裡我們定義了一個靜態 Tracker 成員，直接在類別內初始化
    inline static Tracker staticTracker{"靜態成員 Tracker"};

    // 普通成員
    // 每個 MyClass 對象都會有自己的 memberTracker
    // 這裡我們在建構函數中初始化 memberTracker，並給它一個獨特的標籤
    Tracker memberTracker;

    MyClass(const string& name) : memberTracker("普通成員 " + name) {
        cout << "  [建構] MyClass " << name << endl;
    }

    ~MyClass() {
        cout << "  [解構] MyClass" << endl;
    }
};

int main() {
    cout << "=== 靜態成員的生命週期 ===" << endl;
    cout << "(靜態成員已在 main 之前初始化)" << endl;

    cout << "\n--- 創建對象 ---" << endl;
    {
        MyClass obj("測試");

        cout << "\n--- 使用中 ---" << endl;
        MyClass::staticTracker.ping();
        obj.memberTracker.ping();

        cout << "\n--- 作用域結束 ---" << endl;
    }
    // obj 已銷毀，但靜態成員還活著
    // 靜態成員的生命週期超過任何對象，直到程式結束才會被銷毀

    cout << "\n--- obj 已銷毀，靜態成員仍在 ---" << endl;
    MyClass::staticTracker.ping();

    cout << "\n--- main 結束 ---" << endl;
    return 0;
}
// 程式結束後，靜態成員才會被銷毀
// [解構] 靜態成員 Tracker
