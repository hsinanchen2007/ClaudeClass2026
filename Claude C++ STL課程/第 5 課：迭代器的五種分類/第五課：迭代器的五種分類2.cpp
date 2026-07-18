#include <iostream>
#include <sstream>
#include <iterator>
 
int main() {
    // Input Iterator 的限制示範, 這裡使用 istringstream 模擬輸入串流
    std::istringstream iss("10 20 30");
    // istream_iterator 是 Input Iterator
    std::istream_iterator<int> it(iss);
    
    // 可以讀取
    std::cout << "第一次讀取: " << *it << std::endl;
    
    // 可以前進
    ++it;
    std::cout << "前進後讀取: " << *it << std::endl;
    
    // 關鍵限制：不能回頭！
    // --it;  // 編譯錯誤：Input Iterator 不支援 --
    
    // 也不能跳躍
    // it + 1;  // 編譯錯誤：Input Iterator 不支援 +
    
    // 一旦遍歷過，就不能重新遍歷同一個串流
    // （除非重新建立串流）
    
    return 0;
}
 