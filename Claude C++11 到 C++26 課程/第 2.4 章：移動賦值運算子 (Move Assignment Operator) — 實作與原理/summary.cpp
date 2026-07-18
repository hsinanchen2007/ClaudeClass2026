// ============================================================
// 第 2.4 章 總結：移動賦值運算子（Move Assignment Operator）
// 編譯：g++ -std=c++17 -o summary summary.cpp
// ============================================================
// 【簽名】ClassName& operator=(ClassName&& other) noexcept
//
// 【步驟】
//   1. 自我賦值檢查：if (this != &other)
//   2. 釋放自己的舊資源：delete[] data_;
//   3. 接管來源的資源：data_ = other.data_;
//   4. 清空來源：other.data_ = nullptr;
//   5. 回傳 *this
//
// 【三種實作風格】
//   風格 1（直接實作）：分別寫 operator=(const T&) 和 operator=(T&&)
//   風格 2（swap 慣用法）：各自用 swap 實作
//   風格 3（Copy-and-Swap）：operator=(T other) 傳值 + swap（推薦！）
//     → 一個函式同時處理複製和移動
//     → 左值 → 參數拷貝建構 → swap
//     → 右值 → 參數移動建構 → swap
//
// 【觸發時機】
//   a = std::move(b);        // 明確移動
//   a = Tracker("temp");     // 臨時物件是右值
//   a = make_func();         // 函式回傳值是右值
//   std::swap(a, b);         // swap 內部使用移動賦值
// ============================================================

#include <iostream>
#include <cstring>
#include <utility>
#include <algorithm>
#include <string>
#include <vector>

// ============================================================
// 風格 1：分別實作複製賦值和移動賦值
// ============================================================
class IntArray {
    int* data_;
    size_t size_;
public:
    explicit IntArray(size_t n = 0)
        : data_(n ? new int[n]() : nullptr), size_(n) {}

    ~IntArray() { delete[] data_; }

    IntArray(const IntArray& o)
        : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
        std::cout << "  [複製建構]\n";
    }

    IntArray(IntArray&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [移動建構]\n";
    }

    // ★ 複製賦值
    IntArray& operator=(const IntArray& o) {
        std::cout << "  [複製賦值]\n";
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            data_ = size_ ? new int[size_] : nullptr;
            if (data_) std::copy(o.data_, o.data_ + size_, data_);
        }
        return *this;
    }

    // ★ 移動賦值
    IntArray& operator=(IntArray&& o) noexcept {
        std::cout << "  [移動賦值]\n";
        if (this != &o) {
            delete[] data_;
            data_ = o.data_;  size_ = o.size_;
            o.data_ = nullptr; o.size_ = 0;
        }
        return *this;
    }

    void fill(int v) { for (size_t i = 0; i < size_; ++i) data_[i] = v; }
    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ============================================================
// 風格 3：Copy-and-Swap（推薦）
// ============================================================
class SwapArray {
    int* data_;
    size_t size_;
public:
    explicit SwapArray(size_t n = 0)
        : data_(n ? new int[n]() : nullptr), size_(n) {}

    ~SwapArray() { delete[] data_; }

    SwapArray(const SwapArray& o)
        : data_(o.size_ ? new int[o.size_] : nullptr), size_(o.size_) {
        if (data_) std::copy(o.data_, o.data_ + size_, data_);
        std::cout << "  [SwapArray 複製建構]\n";
    }

    SwapArray(SwapArray&& o) noexcept : data_(o.data_), size_(o.size_) {
        o.data_ = nullptr; o.size_ = 0;
        std::cout << "  [SwapArray 移動建構]\n";
    }

    friend void swap(SwapArray& a, SwapArray& b) noexcept {
        std::swap(a.data_, b.data_);
        std::swap(a.size_, b.size_);
    }

    // ★ 一個 operator= 同時處理複製和移動
    SwapArray& operator=(SwapArray other) noexcept {  // 傳值！
        std::cout << "  [SwapArray 統一賦值] swap\n";
        swap(*this, other);
        return *this;
    }

    size_t size() const { return size_; }
    int* data() const { return data_; }
};

// ============================================================
// 追蹤觸發時機
// ============================================================
class Tracker {
    std::string label_;
public:
    Tracker(const char* l) : label_(l) {}
    Tracker(const Tracker& o) : label_(o.label_) {}
    Tracker(Tracker&& o) noexcept : label_(std::move(o.label_)) { o.label_ = "(empty)"; }

    Tracker& operator=(const Tracker& o) {
        label_ = o.label_;
        std::cout << "  [Tracker 複製賦值] " << label_ << "\n";
        return *this;
    }
    Tracker& operator=(Tracker&& o) noexcept {
        label_ = std::move(o.label_); o.label_ = "(empty)";
        std::cout << "  [Tracker 移動賦值] " << label_ << "\n";
        return *this;
    }
    const std::string& label() const { return label_; }
};

Tracker make_tracker() { return Tracker("factory"); }

int main() {
    // ============================================================
    // 1. 風格 1：分別實作
    // ============================================================
    std::cout << "===== 1. 分別實作 =====\n";
    {
        IntArray a(5); a.fill(10);
        IntArray b(3); b.fill(20);

        std::cout << "  複製賦值 b = a:\n";
        b = a;

        IntArray c(8); c.fill(30);
        std::cout << "  移動賦值 b = move(c):\n";
        b = std::move(c);
        std::cout << "  c.size()=" << c.size() << " (已被掏空)\n";
    }
    std::cout << "\n";

    // ============================================================
    // 2. 風格 3：Copy-and-Swap
    // ============================================================
    std::cout << "===== 2. Copy-and-Swap =====\n";
    {
        SwapArray a(5);
        SwapArray b;

        std::cout << "  複製路徑 b = a:\n";
        b = a;  // 參數以複製建構 → swap

        std::cout << "  移動路徑 b = move(a):\n";
        b = std::move(a);  // 參數以移動建構 → swap

        std::cout << "  臨時物件 b = SwapArray(10):\n";
        b = SwapArray(10);
    }
    std::cout << "\n";

    // ============================================================
    // 3. 觸發時機
    // ============================================================
    std::cout << "===== 3. 觸發時機 =====\n";
    {
        Tracker a("A"), b("B"), c("C");

        std::cout << "  直接賦值左值（複製）:\n";
        a = b;

        std::cout << "  std::move（移動）:\n";
        a = std::move(c);

        std::cout << "  臨時物件（移動）:\n";
        a = Tracker("temp");

        std::cout << "  函式回傳值（移動）:\n";
        a = make_tracker();

        std::cout << "  swap 內部（移動）:\n";
        Tracker x("X"), y("Y");
        std::swap(x, y);
        std::cout << "  x=" << x.label() << " y=" << y.label() << "\n";
    }

    std::cout << "\n=== 重點整理 ===\n";
    std::cout << "  風格 1：分別寫 operator=(const T&) 和 operator=(T&&)\n";
    std::cout << "  風格 3（推薦）：operator=(T other) + swap\n";
    std::cout << "  移動觸發：std::move、臨時物件、函式回傳值、swap\n";
    std::cout << "  移動賦值要加 noexcept\n";

    return 0;
}
