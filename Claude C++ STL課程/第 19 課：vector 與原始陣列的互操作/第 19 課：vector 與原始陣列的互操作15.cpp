#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {10, 20, 30};
    int* ptr = v.data();

    std::cout << "push_back 前：ptr[0] = " << ptr[0] << std::endl;

    // push_back 可能觸發重新配置
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }

    // ptr 可能已經失效！（和迭代器失效同理）
    // std::cout << "push_back 後：ptr[0] = " << ptr[0] << std::endl;  // 危險！

    // 正確：重新取得指標
    ptr = v.data();
    std::cout << "重新取得後：ptr[0] = " << ptr[0] << std::endl;

    return 0;
}
