#include <iostream>
#include <vector>
#include <list>

int main() {
    // vector 有 Random Access Iterator
    std::vector<int> vec = {10, 20, 30, 40, 50};
    auto vit = vec.begin();
    
    std::cout << "vector 迭代器可以：" << std::endl;
    std::cout << "  vit[2] = " << vit[2] << std::endl;      // 隨機存取
    std::cout << "  *(vit + 3) = " << *(vit + 3) << std::endl;  // 算術運算
    
    // list 只有 Bidirectional Iterator
    std::list<int> lst = {10, 20, 30, 40, 50};
    auto lit = lst.begin();
    
    std::cout << "\nlist 迭代器可以：" << std::endl;
    ++lit;  // 前進
    std::cout << "  ++lit → *lit = " << *lit << std::endl;
    --lit;  // 後退
    std::cout << "  --lit → *lit = " << *lit << std::endl;
    
    // 但 list 迭代器不能：
    // lit[2];      // 編譯錯誤！
    // lit + 3;     // 編譯錯誤！
    
    return 0;
}
