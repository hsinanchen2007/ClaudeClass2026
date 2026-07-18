#include <iostream>
#include <vector>

int main() {
    std::vector<int> v;  // 空 vector

    std::cout << std::boolalpha;
    std::cout << "begin() == end(): " << (v.begin() == v.end()) << std::endl;

    // 空 vector 的 begin() 和 end() 相等
    // 所以 for 迴圈的條件 (it != v.end()) 一開始就不成立，不會執行
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";  // 不會執行
    }
    std::cout << "（迴圈未執行）" << std::endl;

    return 0;
}
