#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>

// ===== 風格 1：直接實作 =====
class Style1 {
    int* data_;
    size_t size_;
public:
    explicit Style1(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style1() { delete[] data_; }

    Style1(const Style1& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style1(Style1&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    Style1& operator=(const Style1& o) {
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = size_ ? new int[size_] : nullptr;
            if (data_) std::copy(o.data_, o.data_ + size_, data_);
        }
        return *this;
    }

    Style1& operator=(Style1&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            data_ = o.data_;  size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }
};

// ===== 風格 2：swap 慣用法 =====
class Style2 {
    int* data_;
    size_t size_;
public:
    explicit Style2(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style2() { delete[] data_; }

    Style2(const Style2& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style2(Style2&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    friend void swap(Style2& a, Style2& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    Style2& operator=(const Style2& o) {
        if (this != &o) {
            Style2 tmp(o);       // 複製到臨時物件
            swap(*this, tmp);    // 交換
        }                        // tmp 解構，釋放舊資源
        return *this;
    }

    Style2& operator=(Style2&& o) noexcept {
        swap(*this, o);          // 交換，o 帶走舊資源
        return *this;            // o 解構時釋放
    }
};

// ===== 風格 3：Copy-and-Swap（一個 operator= 搞定全部）=====
class Style3 {
    int* data_;
    size_t size_;
public:
    explicit Style3(size_t n = 0) : data_(n ? new int[n]() : nullptr), size_(n) {}
    ~Style3() { delete[] data_; }

    Style3(const Style3& o) : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
    }
    Style3(Style3&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
    }

    friend void swap(Style3& a, Style3& b) noexcept {
        using std::swap;
        swap(a.data_, b.data_);
        swap(a.size_, b.size_);
    }

    // 一個 operator= 同時處理複製和移動
    Style3& operator=(Style3 other) noexcept {  // 注意：傳值！
        swap(*this, other);
        return *this;
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ===== 輔助：印出物件狀態 =====
template<typename T>
void show(const char* label, const T& obj) {
    std::cout << "  " << label << " → data=" << obj.data()
              << ", size=" << obj.size() << "\n";
}

int main() {
    std::cout << "===== 風格 1：直接實作 =====\n";
    {
        Style1 a(5);
        std::cout << "  a 建立 (size=5)\n";

        // 複製賦值
        Style1 b;
        b = a;
        std::cout << "  b = a（複製賦值）完成\n";

        // 移動賦值
        Style1 c;
        c = std::move(a);
        std::cout << "  c = std::move(a)（移動賦值）完成\n";

        // 自我賦值（有 if(this!=&o) 保護）
        b = b;
        std::cout << "  b = b（自我賦值）完成\n";
    }

    std::cout << "\n===== 風格 2：swap 慣用法 =====\n";
    {
        Style2 a(5);
        std::cout << "  a 建立 (size=5)\n";

        // 複製賦值：內部複製到 tmp 再 swap
        Style2 b;
        b = a;
        std::cout << "  b = a（複製賦值，內部 copy + swap）完成\n";

        // 移動賦值：直接 swap
        Style2 c;
        c = std::move(a);
        std::cout << "  c = std::move(a)（移動賦值，直接 swap）完成\n";
    }

    std::cout << "\n===== 風格 3：Copy-and-Swap（傳值參數）=====\n";
    {
        Style3 a(5);
        show("a 建立", a);

        // 複製賦值：參數以複製建構
        Style3 b;
        b = a;
        show("b = a 後的 b", b);
        show("b = a 後的 a", a);   // a 不受影響

        // 移動賦值：參數以移動建構
        Style3 c;
        c = std::move(a);
        show("c = move(a) 後的 c", c);
        show("c = move(a) 後的 a", a);  // a 已被搬空

        // 用暫時物件賦值
        b = Style3(10);
        show("b = Style3(10) 後的 b", b);
    }

    std::cout << "\n所有風格皆正常運作，資源已自動釋放。\n";
}

