#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {10, 20, 30, 40, 50};
    
    // 範圍 for（語法糖）
    std::cout << "範圍 for: ";
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    // 等價的迭代器寫法
    std::cout << "迭代器: ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 編譯器實際上把範圍 for 轉換成類似這樣：
    /*
    {
        auto&& __range = vec;
        auto __begin = __range.begin();
        auto __end = __range.end();
        for (; __begin != __end; ++__begin) {
            int n = *__begin;
            // 迴圈主體
        }
    }
    */
    
    // 如果要修改元素，使用參考
    std::cout << "\n修改元素（使用參考）: ";
    for (int& n : vec) {
        n *= 2;
    }
    for (int n : vec) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
