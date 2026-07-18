#include <iostream>
#include <string>
using namespace std;

class Soldier {
private:
    int id;
    static int nextId;

public:
    Soldier() : id(nextId++) {
        cout << "  [建構] 士兵 #" << id << endl;
    }
    
    ~Soldier() {
        cout << "  [解構] 士兵 #" << id << endl;
    }
    
    void report() const {
        cout << "  士兵 #" << id << " 報到！" << endl;
    }
};

int Soldier::nextId = 1;

int main() {
    cout << "=== new[] 與 delete[] ===" << endl;
    
    // ====== 基本型別陣列 ======
    cout << "\n--- 基本型別陣列 ---" << endl;
    int* nums = new int[5];           // 5 個 int，未初始化, 可能是垃圾值
    int* zeros = new int[5]();        // 5 個 int，全部初始化為 0, 因為 () 會值初始化
    int* init = new int[5]{10, 20, 30, 40, 50};  // C++11 初始化列表, 初始化為指定值
    
    cout << "  nums:  ";
    for (int i = 0; i < 5; i++) cout << nums[i] << " ";
    cout << "(可能是垃圾值)" << endl;
    
    cout << "  zeros: ";
    for (int i = 0; i < 5; i++) cout << zeros[i] << " ";
    cout << endl;
    
    cout << "  init:  ";
    for (int i = 0; i < 5; i++) cout << init[i] << " ";
    cout << endl;
    
    delete[] nums;
    delete[] zeros;
    delete[] init;
    
    // ====== 類別物件陣列 ======
    cout << "\n--- 類別物件陣列 ---" << endl;
    cout << "  創建 3 個士兵：" << endl;
    Soldier* squad = new Soldier[3];   // 調用 3 次預設建構函數, 返回指向陣列首元素的指標
    
    cout << "\n  點名：" << endl;
    for (int i = 0; i < 3; i++) {
        squad[i].report();
    }
    
    cout << "\n  解散：" << endl;
    delete[] squad;   // 調用 3 次解構函數，然後釋放記憶體, 注意要用 delete[] 來對應 new[]，
                      // 否則只會調用第一個元素的解構函數，導致資源洩漏
    
    return 0;
}
