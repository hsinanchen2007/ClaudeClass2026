// lesson29_complete_class.cpp
// 編譯：g++ -std=c++17 -Wall -Wextra -g -o lesson29b lesson29_complete_class.cpp
// 驗證：valgrind --leak-check=full ./lesson29b

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class DynamicBuffer {
private:
    unsigned char* m_buffer;
    std::size_t m_capacity;

public:
    // ──────── 建構函數 ────────
    explicit DynamicBuffer(std::size_t capacity)
        : m_capacity(capacity)
        , m_buffer(new unsigned char[capacity]{})
    {
        std::cout << "  [建構] capacity=" << m_capacity << "\n";
    }

    // ──────── 拷貝建構函數（深拷貝）────────
    DynamicBuffer(const DynamicBuffer& other)
        : m_capacity(other.m_capacity)
        , m_buffer(new unsigned char[other.m_capacity])
    {
        std::copy(other.m_buffer, other.m_buffer + m_capacity, m_buffer);
        std::cout << "  [拷貝建構] capacity=" << m_capacity << "\n";
    }

    // ──────── 拷貝賦值運算子（Copy-and-Swap）────────
    DynamicBuffer& operator=(DynamicBuffer other) {
        std::cout << "  [拷貝賦值] swap\n";
        swap(other);
        return *this;
    }

    // ──────── 解構函數 ────────
    ~DynamicBuffer() {
        std::cout << "  [解構] capacity=" << m_capacity << "\n";
        delete[] m_buffer;
    }

    // ──────── swap ────────
    void swap(DynamicBuffer& other) noexcept {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_capacity, other.m_capacity);
    }

    // ──────── 存取 ────────
    unsigned char& operator[](std::size_t i) { return m_buffer[i]; }
    const unsigned char& operator[](std::size_t i) const { return m_buffer[i]; }
    std::size_t capacity() const { return m_capacity; }

    void print(const char* label) const {
        std::cout << "  " << label << " (cap=" << m_capacity << "): [";
        for (std::size_t i = 0; i < m_capacity && i < 8; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << static_cast<int>(m_buffer[i]);
        }
        if (m_capacity > 8) std::cout << ", ...";
        std::cout << "]\n";
    }
};

// 非成員 swap
inline void swap(DynamicBuffer& a, DynamicBuffer& b) noexcept {
    a.swap(b);
}

int main() {
    std::cout << "=== 建立 buf1 ===\n";
    DynamicBuffer buf1(5);
    for (std::size_t i = 0; i < buf1.capacity(); ++i) {
        buf1[i] = static_cast<unsigned char>(i * 11);
    }
    buf1.print("buf1");

    std::cout << "\n=== 拷貝建構 buf2 ===\n";
    DynamicBuffer buf2 = buf1;
    buf2.print("buf2");

    std::cout << "\n=== 修改 buf2，驗證獨立性 ===\n";
    buf2[0] = 255;
    buf1.print("buf1");
    buf2.print("buf2");

    std::cout << "\n=== 拷貝賦值：buf3 = buf1 ===\n";
    DynamicBuffer buf3(10);  // 不同大小
    buf3.print("buf3 before");
    buf3 = buf1;
    buf3.print("buf3 after");

    std::cout << "\n=== 用 std::swap 交換 buf1 和buf2 ===\n";
    buf1.print("buf1 before swap");
    buf2.print("buf2 before swap");
    swap(buf1, buf2);  // 使用我們的非成員 swap
    buf1.print("buf1 after swap");
    buf2.print("buf2 after swap");

    std::cout << "\n=== 結束 ===\n";
    return 0;
}
