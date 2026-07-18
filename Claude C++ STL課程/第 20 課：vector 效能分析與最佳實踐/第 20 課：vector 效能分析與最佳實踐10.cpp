#include <iostream>
#include <vector>

// 把要刪除的元素和最後一個元素交換，然後 pop_back
// 時間複雜度：O(1)
template <typename T>
void unstable_erase(std::vector<T>& v, size_t index) {
    v[index] = std::move(v.back());
    v.pop_back();
}

int main() {
    std::vector<int> v = {10, 20, 30, 40, 50};

    std::cout << "刪除前：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;

    // 刪除索引 1（值 20）
    unstable_erase(v, 1);

    std::cout << "刪除後：";
    for (int x : v) std::cout << x << " ";
    std::cout << std::endl;
    // 注意：順序改變了（50 被移到索引 1 的位置）

    return 0;
}
