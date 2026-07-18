#include <iostream>
#include <chrono>
#include <string>
#include <thread>
using namespace std;

class ScopedTimer {
private:
    string taskName;
    chrono::high_resolution_clock::time_point startTime;

public:
    ScopedTimer(const string& name) : taskName(name) {
        startTime = chrono::high_resolution_clock::now();
        cout << "  [計時開始] " << taskName << endl;
    }
    
    ~ScopedTimer() {
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(
            endTime - startTime
        ).count();
        cout << "  [計時結束] " << taskName 
             << " 耗時: " << duration << " ms" << endl;
    }
};

void simulateWork() {
    ScopedTimer timer("模擬工作");
    
    // 模擬一些耗時操作
    int sum = 0;
    for (int i = 0; i < 100000000; i++) {
        sum += i;
    }
    cout << "  計算結果: " << sum << endl;
    
    // timer 在函數返回時自動解構，印出耗時
}

int main() {
    cout << "=== 自動計時器範例 ===" << endl;
    simulateWork();
    cout << "=== 完成 ===" << endl;
    return 0;
}
// 在這裡，ScopedTimer 的解構函數會自動被調用，印出模擬工作的耗時