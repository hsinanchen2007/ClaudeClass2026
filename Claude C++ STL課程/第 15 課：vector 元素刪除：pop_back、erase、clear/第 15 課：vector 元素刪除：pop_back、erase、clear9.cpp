#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // std::remove 把不符合條件的元素往前搬，回傳新的「邏輯結尾」
    // 然後用 erase 刪除尾端的「垃圾」
    
    // 刪除所有偶數
    auto new_end = std::remove_if(v.begin(), v.end(), 
                                  [](int x) { return x % 2 == 0; });
    
    std::cout << "remove_if 後: ";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    // 可能看到：1 3 5 7 9 6 7 8 9 10（後面是殘留資料）
    
    std::cout << "邏輯大小: " << (new_end - v.begin()) << std::endl;
    std::cout << "實際大小: " << v.size() << std::endl;
    
    // 真正刪除
    v.erase(new_end, v.end());
    
    std::cout << "erase 後: ";
    for (int x : v) std::cout << x << " ";  // 1 3 5 7 9
    std::cout << std::endl;
    
    return 0;
}
