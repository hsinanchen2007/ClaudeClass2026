#include <iostream>
#include <vector>

int main() {
    std::vector<std::vector<int>> vv;
    vv.reserve(4);

    // emplace_back 可能導致意外的隱式轉換
    vv.emplace_back(5, 0);
    // 這呼叫的是 vector<int>(5, 0)，建立一個含 5 個 0 的 vector
    // 你可能以為是把 5 和 0 當成兩個元素？不是！

    std::cout << "vv[0].size() = " << vv[0].size() << std::endl;
    std::cout << "vv[0] 內容：";
    for (int x : vv[0]) std::cout << x << " ";
    std::cout << std::endl;

    // push_back 在這種情況下更安全、更明確
    vv.push_back({5, 0});  // 明確：用初始化串列建立含 5 和 0 兩個元素的 vector
    std::cout << "vv[1].size() = " << vv[1].size() << std::endl;
    std::cout << "vv[1] 內容：";
    for (int x : vv[1]) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
