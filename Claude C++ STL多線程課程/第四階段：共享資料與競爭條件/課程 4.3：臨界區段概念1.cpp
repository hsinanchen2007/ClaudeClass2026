#include <iostream>
#include <thread>

int sharedData = 0;

void worker() {
    int localVar = 0;        // 不是臨界區段：區域變數
    
    localVar = 42;           // 不是臨界區段：只存取區域變數
    
    sharedData = localVar;   // ← 臨界區段：寫入共享資料
    
    int temp = sharedData;   // ← 臨界區段：讀取共享資料（如有寫入者）
    
    std::cout << temp;       // 可能是臨界區段：cout 也是共享資源
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    
    t1.join();
    t2.join();
    
    return 0;
}