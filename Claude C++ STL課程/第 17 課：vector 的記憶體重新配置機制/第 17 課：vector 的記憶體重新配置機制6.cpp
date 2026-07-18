#include <iostream>
#include <vector>

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    // 想刪除所有偶數 —— 錯誤寫法！
    for (auto it = v.begin(); it != v.end(); ++it) {
        if (*it % 2 == 0) {
            v.erase(it);  // erase 之後 it 失效了
            // 接著 ++it 操作一個失效的迭代器 → 未定義行為！
        }
    }

    return 0;
}
