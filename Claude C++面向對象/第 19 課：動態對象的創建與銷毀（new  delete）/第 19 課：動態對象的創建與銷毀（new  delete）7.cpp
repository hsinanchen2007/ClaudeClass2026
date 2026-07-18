#include <iostream>
#include <string>
using namespace std;

class Resource {
private:
    string name;
public:
    Resource(const string& n) : name(n) {
        cout << "  [+] " << name << endl;
    }
    ~Resource() {
        cout << "  [-] " << name << endl;
    }
};

void leak1_forget_delete() {
    cout << "\n--- 洩漏 1：忘記 delete ---" << endl;
    Resource* r = new Resource("被遺忘的資源");
    // 忘記 delete r;
    // r 指向的記憶體永遠無法釋放
    // 程式結束時，r 的記憶體會被 OS 回收，但在此之前它一直佔用著資源，可能導致記憶體洩漏和資源浪費
}

void leak2_overwrite_pointer() {
    cout << "\n--- 洩漏 2：覆蓋指標 ---" << endl;
    Resource* r = new Resource("第一個資源");
    r = new Resource("第二個資源");  // r 指向新對象
    // 第一個資源的地址丟失了，永遠無法 delete！
    delete r;  // 只釋放了第二個
    // 第一個資源永遠無法釋放，造成洩漏
}

void leak3_early_return() {
    cout << "\n--- 洩漏 3：提前返回 ---" << endl;
    Resource* r = new Resource("可能洩漏的資源");
    
    bool error = true;  // 模擬錯誤
    if (error) {
        cout << "  發生錯誤，提前返回！" << endl;
        return;         // 直接返回，忘記 delete！
    }
    
    delete r;   // 這行永遠不會執行
    // r 的記憶體永遠無法釋放，造成洩漏
}

void leak4_exception() {
    cout << "\n--- 洩漏 4：異常中斷 ---" << endl;
    Resource* r = new Resource("異常中洩漏的資源");
    
    // 如果這裡拋出異常...
    throw runtime_error("模擬異常");
    
    delete r;   // 這行永遠不會執行
    // r 的記憶體永遠無法釋放，造成洩漏
}

int main() {
    cout << "=== 記憶體洩漏的常見場景 ===" << endl;
    
    leak1_forget_delete();
    leak2_overwrite_pointer();
    leak3_early_return();
    
    try {
        leak4_exception();
    } catch (const exception& e) {
        cout << "  捕獲異常: " << e.what() << endl;
    }
    
    cout << "\n=== 注意：以上有多個資源沒被解構 ===" << endl;
    return 0;
}
