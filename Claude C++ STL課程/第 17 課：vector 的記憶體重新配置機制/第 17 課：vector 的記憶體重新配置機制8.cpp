#include <iostream>
#include <vector>
#include <algorithm>  // std::remove_if

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    // std::remove_if 把不滿足條件的元素「壓縮」到前面
    // 回傳新的邏輯結尾
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [](int x) { return x % 2 == 0; });

    // 此時 v 的內容可能是 {1, 3, 5, 7, ?, ?, ?, ?}
    //                                     ↑ new_end

    // 真正刪除尾部的無用元素
    v.erase(new_end, v.end());

    std::cout << "結果：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
