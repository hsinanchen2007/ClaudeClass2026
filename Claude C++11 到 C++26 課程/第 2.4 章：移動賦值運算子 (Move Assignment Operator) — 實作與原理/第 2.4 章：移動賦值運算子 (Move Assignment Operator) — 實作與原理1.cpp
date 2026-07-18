#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

class IntArray {
    int* data_;
    size_t size_;

    void log(const char* action) const {
        std::cout << "  [" << action << "] size=" << size_
                  << ", data_=" << data_ << "\n";
    }

public:
    // 一般建構子
    explicit IntArray(size_t size = 0)
        : data_(size ? new int[size]() : nullptr)
        , size_(size)
    {
        log("建構");
    }

    // 解構子
    ~IntArray() {
        log("解構");
        delete[] data_;
    }

    // 複製建構子
    IntArray(const IntArray& other)
        : data_(other.size_ ? new int[other.size_] : nullptr)
        , size_(other.size_)
    {
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }
        log("複製建構");
    }

    // 移動建構子
    IntArray(IntArray&& other) noexcept
        : data_(other.data_)
        , size_(other.size_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        log("移動建構");
    }

    // ========== 複製賦值運算子 ==========
    IntArray& operator=(const IntArray& other) {
        std::cout << "  [複製賦值] 開始\n";

        if (this == &other) {  // 自我賦值檢查
            std::cout << "    自我賦值，略過\n";
            return *this;
        }

        // 步驟 1：釋放舊資源
        delete[] data_;

        // 步驟 2：配置新資源並複製
        size_ = other.size_;
        data_ = size_ ? new int[size_] : nullptr;
        if (data_) {
            std::copy(other.data_, other.data_ + size_, data_);
        }

        std::cout << "    配置新記憶體 + 複製完成\n";
        return *this;
    }

    // ========== 移動賦值運算子 ==========
    IntArray& operator=(IntArray&& other) noexcept {
        std::cout << "  [移動賦值] 開始\n";

        if (this == &other) {  // 自我賦值檢查
            std::cout << "    自我移動賦值，略過\n";
            return *this;
        }

        // 步驟 1：釋放自己的舊資源
        delete[] data_;

        // 步驟 2：接管來源的資源
        data_ = other.data_;
        size_ = other.size_;

        // 步驟 3：清空來源
        other.data_ = nullptr;
        other.size_ = 0;

        std::cout << "    釋放舊資源 + 接管完成，零成本搬移\n";
        return *this;
    }

    // 輔助方法
    void fill(int value) {
        for (size_t i = 0; i < size_; ++i) data_[i] = value;
    }

    void print(const char* label) const {
        std::cout << label << ": size=" << size_ << ", data_=" << data_;
        if (data_ && size_ > 0) {
            std::cout << ", 前3個=[";
            for (size_t i = 0; i < 3 && i < size_; ++i) {
                if (i) std::cout << ",";
                std::cout << data_[i];
            }
            std::cout << "...]";
        }
        std::cout << "\n";
    }
};

int main() {
    std::cout << "=== 建立物件 ===\n";
    IntArray a(5);
    a.fill(10);
    IntArray b(3);
    b.fill(20);

    a.print("a");
    b.print("b");

    std::cout << "\n=== 複製賦值：b = a ===\n";
    b = a;
    a.print("a");
    b.print("b");

    std::cout << "\n=== 建立新物件 c ===\n";
    IntArray c(8);
    c.fill(30);
    c.print("c");

    std::cout << "\n=== 移動賦值：b = std::move(c) ===\n";
    b = std::move(c);
    b.print("b");
    c.print("c");  // 已被掏空

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}
