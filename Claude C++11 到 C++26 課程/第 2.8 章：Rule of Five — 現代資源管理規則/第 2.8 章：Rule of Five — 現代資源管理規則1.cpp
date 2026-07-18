#include <iostream>
#include <cstring>
#include <utility>

class String {
    char* data_;
    size_t size_;

public:
    // ===== 建構子 =====
    String(const char* s = "")
        : size_(std::strlen(s))
        , data_(new char[std::strlen(s) + 1])
    {
        std::strcpy(data_, s);
    }

    // ===== 1. 解構子 =====
    ~String() {
        delete[] data_;
    }

    // ===== 2. 複製建構子 =====
    String(const String& other)
        : size_(other.size_)
        , data_(new char[other.size_ + 1])
    {
        std::strcpy(data_, other.data_);
    }

    // ===== 3. 複製賦值 =====
    String& operator=(const String& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = new char[size_ + 1];
            std::strcpy(data_, other.data_);
        }
        return *this;
    }

    // ===== 4. 移動建構子 =====
    String(String&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    // ===== 5. 移動賦值 =====
    String& operator=(String&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    const char* c_str() const { return data_ ? data_ : ""; }
    size_t size() const { return size_; }
};

int main() {
    String a("Hello");
    String b = a;              // 複製建構
    String c;
    c = a;                     // 複製賦值
    String d = std::move(a);   // 移動建構
    String e;
    e = std::move(b);          // 移動賦值

    std::cout << "a: \"" << a.c_str() << "\"\n";  // 空（被移動）
    std::cout << "b: \"" << b.c_str() << "\"\n";  // 空（被移動）
    std::cout << "c: \"" << c.c_str() << "\"\n";  // Hello
    std::cout << "d: \"" << d.c_str() << "\"\n";  // Hello
    std::cout << "e: \"" << e.c_str() << "\"\n";  // Hello

    return 0;
}
