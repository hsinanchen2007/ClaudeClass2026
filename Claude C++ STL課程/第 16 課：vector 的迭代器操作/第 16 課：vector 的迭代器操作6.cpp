#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    auto it = v.begin();

    // 1. 解參考
    std::cout << "*it = " << *it << std::endl;           // 10

    // 2. 遞增 / 遞減
    ++it;
    std::cout << "++it: " << *it << std::endl;           // 20
    --it;
    std::cout << "--it: " << *it << std::endl;           // 10

    // 3. 加減整數（隨機跳躍）
    it = it + 3;
    std::cout << "it + 3: " << *it << std::endl;         // 40
    it = it - 2;
    std::cout << "it - 2: " << *it << std::endl;         // 20

    // 4. 下標運算
    std::cout << "it[2] = " << it[2] << std::endl;       // 40（等同 *(it + 2)）

    // 5. 迭代器相減（計算距離）
    auto it1 = v.begin();
    auto it2 = v.end();
    std::cout << "距離 = " << (it2 - it1) << std::endl;  // 5

    // 6. 比較運算
    std::cout << std::boolalpha;
    std::cout << "(begin < end) = " << (v.begin() < v.end()) << std::endl;  // true

    return 0;
}
