#include <iostream>

int main() {
    int x;
    
    std::cin >> x;
    
    // 檢查輸入狀態, 以確保輸入成功
    // good() 返回 true 如果輸入成功，沒有錯誤
    // eof() 返回 true 如果達到輸入流的結尾, 例如，當用戶按下 Ctrl+D (Unix/Linux) 或 Ctrl+Z (Windows) 時
    // fail() 返回 true 如果輸入失敗（例如，輸入的類型不匹配）, 但流仍然可用
    // bad() 返回 true 如果輸入流處於不可恢復的錯誤狀態, 例如，硬件故障
    std::cout << "good: " << std::cin.good() << std::endl;
    std::cout << "eof: " << std::cin.eof() << std::endl;
    std::cout << "fail: " << std::cin.fail() << std::endl;
    std::cout << "bad: " << std::cin.bad() << std::endl;
    
    return 0;
}
