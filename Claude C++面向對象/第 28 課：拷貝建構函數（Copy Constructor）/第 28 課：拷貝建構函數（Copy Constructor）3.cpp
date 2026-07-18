// lesson28_int_array.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o intarray lesson28_int_array.cpp

#include <iostream>
#include <algorithm>  // std::copy

class IntArray {
private:
    int* m_data;
    std::size_t m_size;

public:
    // ──────── 一般建構函數 ────────
    explicit IntArray(std::size_t size)
        : m_size(size)
        , m_data(new int[size]{})  // 值初始化為 0
    {
        std::cout << "  [建構] 大小=" << m_size << "\n";
    }

    // ──────── 拷貝建構函數（深拷貝）────────
    IntArray(const IntArray& other)
        : m_size(other.m_size)
        , m_data(new int[other.m_size])   // 配置新記憶體
    {
        std::copy(other.m_data, other.m_data + m_size, m_data);  // 複製內容
        std::cout << "  [拷貝建構] 大小=" << m_size << "\n";
    }

    // ──────── 拷貝賦值運算子（深拷貝）────────
    IntArray& operator=(const IntArray& other) {
        std::cout << "  [拷貝賦值]\n";
        if (this == &other) return *this;

        delete[] m_data;                          // 釋放舊資源
        m_size = other.m_size;
        m_data = new int[m_size];                 // 配置新記憶體
        std::copy(other.m_data, other.m_data + m_size, m_data);

        return *this;
    }

    // ──────── 解構函數 ────────
    ~IntArray() {
        std::cout << "  [解構] 大小=" << m_size << "\n";
        delete[] m_data;
    }

    // ──────── 存取 ────────
    int& operator[](std::size_t index) { return m_data[index]; }
    const int& operator[](std::size_t index) const { return m_data[index]; }
    std::size_t size() const { return m_size; }

    void print(const char* label) const {
        std::cout << "  " << label << ": [";
        for (std::size_t i = 0; i < m_size; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << m_data[i];
        }
        std::cout << "]\n";
    }
};

int main() {
    std::cout << "=== 建立 arr1 ===\n";
    IntArray arr1(5);
    for (std::size_t i = 0; i < arr1.size(); ++i) {
        arr1[i] = static_cast<int>((i + 1) * 10);  // 10, 20, 30, 40, 50
    }
    arr1.print("arr1");

    std::cout << "\n=== 拷貝建構 arr2 = arr1 ===\n";
    IntArray arr2 = arr1;  // 拷貝建構
    arr2.print("arr2");

    std::cout << "\n=== 修改 arr2，驗證獨立性 ===\n";
    arr2[0] = 999;
    arr1.print("arr1");  // 不受影響
    arr2.print("arr2");  // 只有 arr2 改變

    std::cout << "\n=== 拷貝賦值 arr3 = arr1 ===\n";
    IntArray arr3(3);     // 先建構一個不同大小的
    arr3.print("arr3 (before)");
    arr3 = arr1;          // 拷貝賦值：釋放舊的 3 個元素，配置新的 5 個
    arr3.print("arr3 (after)");

    std::cout << "\n=== 結束 ===\n";
    return 0;
}
