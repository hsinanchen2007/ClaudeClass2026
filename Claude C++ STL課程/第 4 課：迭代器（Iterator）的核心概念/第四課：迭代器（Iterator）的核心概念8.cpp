#include <iostream>
#include <vector>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // 危險！在遍歷時修改容器
    /*
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if (*it == 3) {
            vec.erase(it);  // 刪除後，it 失效！
            // 繼續使用 it 是未定義行為
        }
    }
    */
    
    // 正確做法：使用 erase 的回傳值
    std::cout << "原始: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    for (auto it = vec.begin(); it != vec.end(); /* 不在這裡 ++ */) {
        if (*it == 3) {
            it = vec.erase(it);  // erase 回傳下一個有效迭代器
        } else {
            ++it;
        }
    }
    
    std::cout << "刪除 3 後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
