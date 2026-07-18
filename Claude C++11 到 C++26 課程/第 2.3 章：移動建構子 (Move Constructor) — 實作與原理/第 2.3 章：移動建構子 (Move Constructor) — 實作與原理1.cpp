#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class IntArray {
    int* data_;
    size_t size_;

public:
    // ========== 一般建構子 ==========
    explicit IntArray(size_t size = 0)
        : data_(size ? new int[size]() : nullptr)
        , size_(size)
    {
        std::cout << "  [建構] size=" << size_
                  << ", data_=" << data_ << "\n";
    }

    // ========== 解構子 ==========
    ~IntArray() {
        std::cout << "  [解構] size=" << size_
                  << ", data_=" << data_ << "\n";
        delete[] data_;
    }

    // ========== 複製建構子 ==========
    IntArray(const IntArray& other)
        : data_(other.size_ ? new int[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }
        std::cout << "  [複製建構] size=" << size_
                  << " ← 配置新記憶體 + 逐元素複製\n";
    }

    // ========== 移動建構子 ==========
    IntArray(IntArray&& other) noexcept
        : data_(other.data_)      // 步驟 1：接管指標
        , size_(other.size_)      // 步驟 2：接管大小
    {
        other.data_ = nullptr;    // 步驟 3：讓來源放棄資源
        other.size_ = 0;
        std::cout << "  [移動建構] size=" << size_
                  << " ← 只搬指標，零成本\n";
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

int main() {
    std::cout << "=== 建立原始物件 ===\n";
    IntArray a(5);
    a.data()[0] = 10;
    a.data()[1] = 20;
    a.data()[2] = 30;

    std::cout << "\n=== 複製建構 ===\n";
    IntArray b(a);  // 呼叫複製建構子
    std::cout << "a.data() = " << a.data() << " (原始物件完好)\n";
    std::cout << "b.data() = " << b.data() << " (指向不同的記憶體)\n";

    std::cout << "\n=== 移動建構 ===\n";
    IntArray c(std::move(a));  // 呼叫移動建構子
    std::cout << "a.data() = " << a.data() << " (已被掏空)\n";
    std::cout << "a.size() = " << a.size() << "\n";
    std::cout << "c.data() = " << c.data() << " (接管了原本 a 的記憶體)\n";
    std::cout << "c[0]=" << c.data()[0]
              << ", c[1]=" << c.data()[1]
              << ", c[2]=" << c.data()[2] << "\n";

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}
