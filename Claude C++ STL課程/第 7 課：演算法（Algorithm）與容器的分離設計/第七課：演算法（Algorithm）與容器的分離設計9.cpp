#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // replace：替換所有等於某值的元素, 不會改變容器大小
    std::cout << "=== replace ===" << std::endl;
    std::vector<int> vec = {1, 2, 3, 2, 4, 2, 5};
    std::replace(vec.begin(), vec.end(), 2, 99);
    std::cout << "把 2 替換為 99: ";
    for (int n : vec) std::cout << n << " ";
    std::cout << std::endl;
    
    // replace_if：條件替換, 不會改變容器大小
    std::cout << "\n=== replace_if ===" << std::endl;
    std::vector<int> vec2 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::replace_if(vec2.begin(), vec2.end(),
        [](int n) { return n % 2 == 0; }, 0);
    std::cout << "偶數替換為 0: ";
    for (int n : vec2) std::cout << n << " ";
    std::cout << std::endl;
    
    // remove：移除所有等於某值的元素, 但不會改變容器大小
    // 重要：remove 不會真正刪除元素，而是把要保留的移到前面！剩下的元素是未定義的
    std::cout << "\n=== remove（重要！）===" << std::endl;
    std::vector<int> vec3 = {1, 2, 3, 2, 4, 2, 5};
    std::cout << "原始: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;
    
    auto new_end = std::remove(vec3.begin(), vec3.end(), 2);
    std::cout << "remove 後: ";
    for (int n : vec3) std::cout << n << " ";
    std::cout << "(size=" << vec3.size() << ")" << std::endl;
    
    std::cout << "有效範圍: ";
    for (auto it = vec3.begin(); it != new_end; ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    
    // 真正刪除：erase-remove 慣用法, 先 remove 把要保留的元素移到前面，再用 erase 刪除多餘的元素
    std::cout << "\n=== erase-remove 慣用法 ===" << std::endl;
    std::vector<int> vec4 = {1, 2, 3, 2, 4, 2, 5};
    vec4.erase(std::remove(vec4.begin(), vec4.end(), 2), vec4.end());
    std::cout << "erase-remove 後: ";
    for (int n : vec4) std::cout << n << " ";
    std::cout << "(size=" << vec4.size() << ")" << std::endl;
    
    return 0;
}
