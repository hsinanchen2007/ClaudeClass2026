#include <iostream>
using namespace std;

class Light {
public:
    bool isOn = false;

    void toggle() {
        isOn = !isOn;
    }

    void show() {
        cout << "燈: " << (isOn ? "開" : "關") << endl;
    }
};

int main() {
    Light lamp;
    lamp.show();        // ✅ 可以呼叫 public 函數
    lamp.toggle();      // ✅ 可以呼叫 public 函數
    lamp.show();
    lamp.isOn = false;  // ✅ 可以直接存取 public 變數
    lamp.show();
    return 0;
}
