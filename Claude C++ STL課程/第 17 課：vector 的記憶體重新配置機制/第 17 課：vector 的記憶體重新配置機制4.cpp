#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};
    v.reserve(3);  // 確保容量剛好是 3

    auto it = v.begin() + 1;  // 指向 20
    std::cout << "*it = " << *it << std::endl;

    // push_back 觸發重新配置（因為 size == capacity）
    v.push_back(40);

    // 此時 it 指向的是舊記憶體，已被釋放
    // *it 是未定義行為！就像使用懸空指標（dangling pointer）一樣
    // std::cout << "*it = " << *it << std::endl;  // 危險！

    // 正確做法：重新取得迭代器
    it = v.begin() + 1;
    std::cout << "*it = " << *it << std::endl;  // 20

    return 0;
}
