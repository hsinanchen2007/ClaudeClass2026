#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {10, 20, 30};
    
    // 危險！當 v.size() 是 0 時，v.size() - 1 會溢位成超大數字
    // for (size_t i = v.size() - 1; i >= 0; --i) {  // 永遠為真！
    //     std::cout << v[i] << std::endl;
    // }
    
    // 安全的反向遍歷方式
    for (size_t i = v.size(); i > 0; --i) {
        std::cout << v[i - 1] << " ";  // 30 20 10
    }
    std::cout << std::endl;
    
    // 或者使用反向迭代器（第 16 課會講）
    
    return 0;
}
