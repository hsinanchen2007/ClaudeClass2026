#include <iostream>
#include <cstring>
#include <utility>

class Resource {
    int* integers_;
    size_t int_count_;
    char* text_;
    int file_descriptor_;  // 模擬檔案描述符

public:
    Resource(size_t count, const char* str, int fd)
        : integers_(new int[count]())
        , int_count_(count)
        , text_(new char[std::strlen(str) + 1])
        , file_descriptor_(fd)
    {
        std::strcpy(text_, str);
        std::cout << "  [建構] ints=" << int_count_
                  << ", text=\"" << text_
                  << "\", fd=" << file_descriptor_ << "\n";
    }

    ~Resource() {
        std::cout << "  [解構] ints=" << integers_
                  << ", text=" << (void*)text_
                  << ", fd=" << file_descriptor_ << "\n";
        delete[] integers_;       // nullptr 時 delete 安全
        delete[] text_;           // nullptr 時 delete 安全
        if (file_descriptor_ >= 0) {
            // close(file_descriptor_);  // 真實場景會關閉檔案
        }
    }

    // 複製建構子
    Resource(const Resource& other)
        : integers_(new int[other.int_count_])
        , int_count_(other.int_count_)
        , text_(new char[std::strlen(other.text_) + 1])
        , file_descriptor_(other.file_descriptor_)  // fd 的複製語意需要特別考慮
    {
        std::copy(other.integers_, other.integers_ + int_count_, integers_);
        std::strcpy(text_, other.text_);
        std::cout << "  [複製建構] 2 次 new + 資料複製\n";
    }

    // 移動建構子
    Resource(Resource&& other) noexcept
        : integers_(other.integers_)           // 接管整數陣列
        , int_count_(other.int_count_)         // 接管計數
        , text_(other.text_)                   // 接管文字緩衝區
        , file_descriptor_(other.file_descriptor_) // 接管檔案描述符
    {
        // 關鍵：讓來源放棄所有資源
        other.integers_ = nullptr;
        other.int_count_ = 0;
        other.text_ = nullptr;
        other.file_descriptor_ = -1;  // -1 表示無效的 fd

        std::cout << "  [移動建構] 0 次 new，只搬指標\n";
    }

    void print() const {
        std::cout << "  integers_=" << integers_
                  << ", count=" << int_count_
                  << ", text=" << (text_ ? text_ : "(null)")
                  << ", fd=" << file_descriptor_ << "\n";
    }
};

int main() {
    std::cout << "=== 建立原始物件 ===\n";
    Resource r1(1000, "important data", 42);
    r1.print();

    std::cout << "\n=== 複製建構 ===\n";
    Resource r2(r1);
    std::cout << "r1: "; r1.print();
    std::cout << "r2: "; r2.print();

    std::cout << "\n=== 移動建構 ===\n";
    Resource r3(std::move(r1));
    std::cout << "r1: "; r1.print();  // 已被掏空
    std::cout << "r3: "; r3.print();  // 擁有原本 r1 的資源

    std::cout << "\n=== 離開 main ===\n";
    return 0;
}
