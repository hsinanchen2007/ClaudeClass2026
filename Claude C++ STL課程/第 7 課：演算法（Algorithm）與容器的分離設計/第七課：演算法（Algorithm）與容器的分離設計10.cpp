#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // reverse：反轉, 這裡將 vec 的元素順序反轉
    std::cout << "=== reverse ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::reverse(vec.begin(), vec.end());
    std::cout << "反轉後: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // rotate：旋轉, 這裡將 vec2 的元素旋轉，使得 vec2.begin() + 2 的元素成為新的第一個元素
    std::cout << "\n=== rotate ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5};
    // 把 vec2.begin() + 2 變成新的第一個元素
    std::rotate(vec2.begin(), vec2.begin() + 2, vec2.end());
    std::cout << "rotate 後（3 變成第一個）: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    
    // unique：移除連續重複, 這裡將 vec3 中連續重複的元素移除，最後使用 erase 刪除多餘的元素
    std::cout << "\n=== unique ===" << std::endl;
    std::vector<int> vec3 = {1, 1, 2, 2, 2, 3, 3, 4, 5, 5};
    auto new_end = std::unique(vec3.begin(), vec3.end());
    vec3.erase(new_end, vec3.end());
    std::cout << "unique 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << std::endl;
    
    return 0;
}
