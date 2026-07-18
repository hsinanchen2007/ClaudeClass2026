#include <iostream>
#include <vector>
#include <cstring>   // memcpy, memset
#include <cstdlib>   // qsort
#include <algorithm>  // std::sort（對比用）

// qsort 的比較函數
int compare_int(const void* a, const void* b) {
    return (*(const int*)a) - (*(const int*)b);
}

int main() {
    // ===== memcpy：在 vector 之間複製原始記憶體 =====
    std::vector<int> src = {50, 30, 10, 40, 20};
    std::vector<int> dst(src.size());

    std::memcpy(dst.data(), src.data(), src.size() * sizeof(int));

    std::cout << "memcpy 結果：";
    for (int x : dst) std::cout << x << " ";
    std::cout << std::endl;

    // ===== memset：批量設定記憶體 =====
    std::vector<int> zeros(5);
    std::memset(zeros.data(), 0, zeros.size() * sizeof(int));

    std::cout << "memset 結果：";
    for (int x : zeros) std::cout << x << " ";
    std::cout << std::endl;

    // ===== qsort：使用 C 的排序函數 =====
    std::vector<int> to_sort = {50, 30, 10, 40, 20};
    std::qsort(to_sort.data(), to_sort.size(), sizeof(int), compare_int);

    std::cout << "qsort 結果：";
    for (int x : to_sort) std::cout << x << " ";
    std::cout << std::endl;

    return 0;
}
