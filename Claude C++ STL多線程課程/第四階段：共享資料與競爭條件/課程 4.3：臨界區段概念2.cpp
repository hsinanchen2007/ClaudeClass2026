#include <iostream>
#include <thread>
#include <vector>

// 共享資源
int globalCounter = 0;                    // 全域變數
static int staticCounter = 0;             // 靜態變數
std::vector<int> sharedVector;            // 全域容器

class MyClass {
    int memberVar;                        // 若多執行緒存取同一物件
    static int staticMember;              // 靜態成員
    
public:
    void method() {
        // memberVar 是否為共享資源？
        // 取決於是否多執行緒存取同一個 MyClass 物件
    }
};

void function() {
    int localVar = 0;                     // 不是共享資源
    static int localStatic = 0;           // 是共享資源！
    thread_local int tlVar = 0;           // 不是共享資源
}

void worker() {
    int localVar = 0;        // 不是臨界區段：區域變數
    
    localVar = 42;           // 不是臨界區段：只存取區域變數
    
    globalCounter = localVar;   // ← 臨界區段：寫入共享資料
    
    int temp = globalCounter;   // ← 臨界區段：讀取共享資料（如有寫入者）
    
    std::cout << temp;       // 可能是臨界區段：cout 也是共享資源
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    
    t1.join();
    t2.join();
    
    return 0;
}