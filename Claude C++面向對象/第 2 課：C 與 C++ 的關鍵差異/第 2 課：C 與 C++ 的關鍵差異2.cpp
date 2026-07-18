#include <iostream>

int main() {
    // C++ 可以在任何位置宣告變數
    int sum = 0;
    
    // 甚至可以在 for 迴圈內宣告
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    
    // 需要時才宣告，更清晰
    int result = sum * 2;
    
    std::cout << "Sum = " << sum << std::endl;
    return 0;
}
