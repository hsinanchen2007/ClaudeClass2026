//這是最容易出錯的地方：
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // 目標：刪除所有偶數
    
    // ❌ 錯誤做法 1：索引會亂掉
    /*
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] % 2 == 0) {
            v.erase(v.begin() + i);
            // 問題：刪除後，原本 i+1 變成 i，但 i 會 ++，跳過了一個元素
        }
    }
    */
    
    // ❌ 錯誤做法 2：迭代器失效
    /*
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it % 2 == 0) {
            v.erase(it);
            // 問題：erase 後 it 失效，++it 是未定義行為
        }
    }
    */
    
    // ✅ 正確做法 1：利用 erase 的回傳值
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) {
            it = v.erase(it);  // erase 回傳下一個元素的迭代器
        } else {
            ++it;
        }
    }
    
    std::cout << "刪除偶數後: ";
    for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << std::endl;
    
    return 0;
}