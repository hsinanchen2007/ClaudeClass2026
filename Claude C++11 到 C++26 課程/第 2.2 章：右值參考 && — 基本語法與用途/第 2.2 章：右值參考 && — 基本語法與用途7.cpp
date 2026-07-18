#include <iostream>
#include <cstring>
#include <utility>

class Buffer {
    char* data_;
    size_t size_;

public:
    // 一般建構子
    Buffer(const char* str) : size_(std::strlen(str)) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, str);
        std::cout << "  [建構] \"" << data_ << "\" (配置 "
                  << (size_ + 1) << " bytes)\n";
    }

    // 解構子
    ~Buffer() {
        if (data_) {
            std::cout << "  [解構] \"" << data_ << "\"\n";
            delete[] data_;
        } else {
            std::cout << "  [解構] (空 buffer)\n";
        }
    }

    // 複製建構子
    Buffer(const Buffer& other) : size_(other.size_) {
        data_ = new char[size_ + 1];
        std::strcpy(data_, other.data_);
        std::cout << "  [複製建構] \"" << data_ << "\" ← 深拷貝，代價高！\n";
    }

    // 移動建構子（C++11 新增）
    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        other.data_ = nullptr;  // 重要：讓來源物件不再擁有資源
        other.size_ = 0;
        std::cout << "  [移動建構] \"" << data_ << "\" ← 只搬指標，幾乎零成本！\n";
    }

    const char* c_str() const { return data_ ? data_ : "(null)"; }
};

// 接收左值
void store(const Buffer& buf) {
    std::cout << "store(const Buffer&): 需要複製\n";
    Buffer local(buf);  // 複製建構
}

// 接收右值
void store(Buffer&& buf) {
    std::cout << "store(Buffer&&): 可以移動\n";
    Buffer local(std::move(buf));  // 移動建構
}

int main() {
    std::cout << "=== 建立原始 Buffer ===\n";
    Buffer original("Important Data Here");

    std::cout << "\n=== 傳入左值（複製）===\n";
    store(original);

    std::cout << "\n=== 傳入右值（移動）===\n";
    store(Buffer("Temporary Data"));

    std::cout << "\n=== 用 std::move 傳入（移動）===\n";
    store(std::move(original));
    std::cout << "original after move: " << original.c_str() << "\n";

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}
