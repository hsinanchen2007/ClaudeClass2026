#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};
    v.reserve(100);  // 預留足夠空間，確保不會重新配置

    // ===== insert 的影響 =====
    auto it_30 = v.begin() + 2;  // 指向 30
    auto it_10 = v.begin();      // 指向 10

    // 在位置 1 插入 15
    v.insert(v.begin() + 1, 15);
    // 結果：{10, 15, 20, 30, 40, 50}

    // it_10 仍然有效（插入點之前）
    std::cout << "*it_10 = " << *it_10 << std::endl;  // 10 ✓

    // it_30 已失效！（插入點之後的元素都被往後移了）
    // 它原本指向的記憶體位置現在存的是不同的值
    // std::cout << "*it_30 = " << *it_30 << std::endl;  // 不可預期

    std::cout << std::endl;

    // ===== erase 的影響 =====
    std::vector<int> v2 = {10, 20, 30, 40, 50};
    v2.reserve(100);

    auto it_40 = v2.begin() + 3;  // 指向 40
    auto it_10b = v2.begin();      // 指向 10

    // 刪除位置 1 的元素（20）
    v2.erase(v2.begin() + 1);
    // 結果：{10, 30, 40, 50}

    // it_10b 仍然有效（刪除點之前）
    std::cout << "*it_10b = " << *it_10b << std::endl;  // 10 ✓

    // it_40 已失效！（刪除點之後的元素都被往前移了）
    // std::cout << "*it_40 = " << *it_40 << std::endl;  // 不可預期

    return 0;
}
