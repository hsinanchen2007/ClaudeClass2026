#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
 
int main() {
    std::cout << "=== Input Iterator 示範 ===" << std::endl;
    std::cout << "請輸入一些整數（輸入非數字結束）：" << std::endl;
    
    // istream_iterator 是 Input Iterator
    std::istream_iterator<int> input_begin(std::cin);
    std::istream_iterator<int> input_end;  // 預設建構 = 結束標記
    
    // 讀取所有輸入到 vector, 使用 Input Iterator 的範圍建構函式
    std::vector<int> numbers(input_begin, input_end);
    
    std::cout << "你輸入了 " << numbers.size() << " 個數字：";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
 