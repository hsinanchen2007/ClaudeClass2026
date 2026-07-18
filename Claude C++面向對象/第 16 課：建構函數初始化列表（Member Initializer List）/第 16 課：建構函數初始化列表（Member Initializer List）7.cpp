#include <iostream>
using namespace std;

class DangerousOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 危險！看起來 data 先被初始化，但實際上 length 先執行
    // DangerousOrder 的建構函數使用了初始化列表，但順序錯誤
    // 優點：效率更高，特別是對於 const 成員和參考成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值
    DangerousOrder(int len) 
        : data(new int[length]),  // 用 length 來分配，但 length 還沒初始化！
          length(len)              // length 在 data 之前宣告，所以先初始化
    {
        // 此時 data 分配了垃圾大小的記憶體！
    }
    
    ~DangerousOrder() { delete[] data; }
};

class SafeOrder {
private:
    int length;    // 第 1 個宣告
    int* data;     // 第 2 個宣告

public:
    // 安全：初始化列表的順序和宣告順序一致
    // SafeOrder 的建構函數使用了初始化列表，且順序正確
    // 優點：效率更高，特別是對於 const 成員和參考成員來說，必須使用初始化列表來初始化，因為它們不能在函數體內賦值
    // 初始化列表的語法：冒號後面跟著成員變數和對應的初始值
    // 注意：初始化列表的語法是冒號後面跟著成員變數和對應的初始值，且初始化順序與成員宣告順序一致  
    SafeOrder(int len) 
        : length(len),            // 先初始化 length
          data(new int[length])   // 再用 length 分配
    {
        cout << "安全分配了 " << length << " 個元素" << endl;
    }
    
    ~SafeOrder() { delete[] data; }
};

int main() {
    // DangerousOrder d(10);  // 未定義行為！不要這樣做
    SafeOrder s(10);          // OK
    return 0;
}
