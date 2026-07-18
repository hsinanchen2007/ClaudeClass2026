#include <iostream>
#include <string>
using namespace std;

class Tracker {
private:
    string name;
public:
    Tracker(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Tracker() {
        cout << "  [-] " << name << endl;
    }
    void work() const {
        cout << "  [=] " << name << " 工作中" << endl;
    }
};

int main() {
    // ====== if 語句中的對象 ======
    cout << "=== if 語句 ===" << endl;
    if (bool condition = true) {
        Tracker t("if 區塊物件");
        t.work();
    }   // t 在這裡死亡
    cout << "  if 區塊已結束\n" << endl;
    
    // ====== for 迴圈中的對象 ======
    cout << "=== for 迴圈 ===" << endl;
    for (int i = 0; i < 3; i++) {
        Tracker t("迴圈物件 #" + to_string(i));
        t.work();
        // t 在每次迭代結束時死亡，下次迭代重新建構
    }
    cout << "  for 迴圈已結束\n" << endl;
    
    // ====== for 初始化部分的對象 ======
    cout << "=== for 初始化區 ===" << endl;
    // 注意：C++ 沒有直接在 for 初始化區放自定義對象的語法
    // 但可以這樣理解等效行為
    {
        Tracker outer("迴圈外圍物件");
        for (int i = 0; i < 2; i++) {
            Tracker inner("迴圈內部 #" + to_string(i));
            inner.work();
        }
    }
    cout << "  完成\n" << endl;
    
    return 0;
}
