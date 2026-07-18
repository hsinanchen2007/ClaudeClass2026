// ============================================================================
//  28_capstone_mini_stl.cpp  ──  綜合應用：自製迷你容器 + Circular Queue
// ============================================================================
//
//  【本篇定位】
//    這是系列的最終整合篇。把前面學到的東西全部用上：
//      ◆ Class template (file 03)
//      ◆ Non-type template parameter (file 09)
//      ◆ Variadic template + perfect forwarding (file 13, 15)
//      ◆ SFINAE / static_assert / type_traits (file 16, 17)
//      ◆ if constexpr (file 19)
//      ◆ Iterator + tag dispatch 思想 (file 18)
//
//    我們會做兩件事：
//      ① 一個極簡 Vector<T>：dynamic array，提供 emplace_back + iterator。
//      ② 一個 SmallVector<T, N>：當元素 ≤ N 時用 inline buffer，避免 heap。
//      ③ Leetcode 622 Design Circular Queue：用我們自己的 SmallVector 當底。
//
// ----------------------------------------------------------------------------
//  【Vector<T> 設計重點】
//    - 三指標模型：begin / end / cap (current size + total capacity)
//    - emplace_back 用 perfect forwarding 接任意 constructor 參數
//    - 提供 begin()/end() iterator，讓 range-for 與 std::sort 都能用
//    - destructor 顯式呼叫每個元素的 ~T
//    - 沒有實作 copy/move，本檔示範重點放在 「template 整合」 而非完整容器
//
//  【SmallVector<T, N>】
//    Inline buffer 模式：
//      - data_ 一開始指向「inline 區」(stack 上 N 個元素的 storage)
//      - 元素數量超過 N 才搬到 heap
//      - 對「絕大多數小集合」的工作場景 (e.g. AST 子節點 ≤ 4) 性能極佳
//    這種模式在 Clang / LLVM、Rust SmallVec、Boost 都很常見。
//
//  【LC 622 Design Circular Queue】
//    題目：實作大小固定的 circular queue，操作：
//      enQueue(x)：滿時失敗
//      deQueue()：空時失敗
//      Front()/Rear()：空時 -1
//      isEmpty() / isFull()
//    解法：head + tail + count，模 K 索引。所有操作 O(1)。
//
//  參考：
//    https://en.cppreference.com/cpp/container/vector
//    https://en.cppreference.com/cpp/container/queue
// ============================================================================

/*
補充筆記：capstone_mini_stl
  - capstone_mini_stl 涉及模板實例化；請先判斷哪些型別由呼叫端推導，哪些型別由程式指定。
  - 泛型程式要把型別需求寫清楚，例如可比較、可移動、可呼叫或符合某個 concept。
  - 模板技巧的價值在減少重複且保留型別安全，不是讓錯誤訊息變得更長。
  - capstone_mini_stl 是 template 主題；template 的重點是讓型別或值在編譯期決定，產生對應的具體程式碼。
  - template 定義通常需要放在 header 或使用點可見的位置，否則編譯器無法實例化需要的版本。
  - 錯誤訊息常出現在實例化深處；閱讀時先找第一個 substitution 或 constraint 不成立的位置。
  - type trait、SFINAE、concepts 都是在表達「這個型別必須具備什麼能力」；C++20 後 concepts 通常更清楚。
  - perfect forwarding 需要 T&& 搭配 std::forward<T>，不要把所有 && 都誤認為 move。
  - template 可提升零成本抽象，但也可能造成編譯時間上升和二進位膨脹；共通實作可用非 template helper 收斂。
*/
#include <algorithm>
#include <iostream>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ─── 1. 極簡 Vector<T> ────────────────────────────────────────────────────
template <typename T>
class Vector {
public:
    Vector() = default;
    explicit Vector(std::size_t n)
        : data_(allocate(n)), size_(n), cap_(n) {
        for (std::size_t i = 0; i < n; ++i) ::new (data_ + i) T{};
    }

    ~Vector() {
        clear();
        operator delete(data_);
    }

    // 接任意 constructor 參數，perfect forward 給 T 的 constructor
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == cap_) reserve(cap_ ? cap_ * 2 : 4);
        T* p = ::new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return *p;
    }

    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v)      { emplace_back(std::move(v)); }

    void pop_back() {
        if (size_ == 0) return;
        --size_;
        data_[size_].~T();
    }

    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    std::size_t size()     const { return size_; }
    std::size_t capacity() const { return cap_; }
    bool empty()           const { return size_ == 0; }

    T*       begin()       { return data_; }
    T*       end()         { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end()   const { return data_ + size_; }

    void reserve(std::size_t newcap) {
        if (newcap <= cap_) return;
        T* nd = allocate(newcap);
        // move 舊元素到新 buffer
        for (std::size_t i = 0; i < size_; ++i) {
            ::new (nd + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        operator delete(data_);
        data_ = nd;
        cap_  = newcap;
    }

    void clear() {
        for (std::size_t i = 0; i < size_; ++i) data_[i].~T();
        size_ = 0;
    }

private:
    static T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    T* data_   = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_  = 0;
};

// ─── 2. SmallVector<T, N>：inline buffer 優化 ───────────────────────────
//   設計簡化：只支援 push_back / pop_back / 走訪。
//   元素 ≤ N 時用 inline buffer (stack)，否則切換到 heap。
template <typename T, std::size_t N>
class SmallVector {
public:
    static_assert(N > 0, "SmallVector N 必須 > 0");

    SmallVector() : data_(reinterpret_cast<T*>(buffer_)), cap_(N) {}

    ~SmallVector() {
        for (std::size_t i = 0; i < size_; ++i) data_[i].~T();
        if (heap_) operator delete(data_);
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == cap_) grow();
        ::new (data_ + size_) T(std::forward<Args>(args)...);
        ++size_;
        return data_[size_ - 1];
    }
    void push_back(const T& v) { emplace_back(v); }

    void pop_back() {
        if (size_ == 0) return;
        --size_;
        data_[size_].~T();
    }

    std::size_t size()     const { return size_; }
    std::size_t capacity() const { return cap_; }
    bool inline_storage()  const { return !heap_; }

    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    T* begin() { return data_; }
    T* end()   { return data_ + size_; }

private:
    void grow() {
        std::size_t newcap = cap_ * 2;
        T* nd = static_cast<T*>(::operator new(newcap * sizeof(T)));
        for (std::size_t i = 0; i < size_; ++i) {
            ::new (nd + i) T(std::move(data_[i]));
            data_[i].~T();
        }
        if (heap_) operator delete(data_);
        data_ = nd;
        cap_  = newcap;
        heap_ = true;
    }

    alignas(T) unsigned char buffer_[N * sizeof(T)];
    T*          data_;
    std::size_t size_ = 0;
    std::size_t cap_  = 0;
    bool        heap_ = false;
};

// ─── 3. Leetcode 622 ── Design Circular Queue (用 SmallVector 當底) ─────
//   實作：固定容量 K 的 circular queue，操作都是 O(1)。
//
//   時間：所有操作 O(1)
//   空間：O(K)
//   邊界：滿 → enQueue 失敗；空 → deQueue 失敗、Front/Rear 回 -1。
template <typename T>
class MyCircularQueue {
public:
    explicit MyCircularQueue(int k)
        : k_(k), data_(static_cast<std::size_t>(k)) {}    // 內部 T{} 值初始化

    bool enQueue(const T& v) {
        if (count_ == k_) return false;
        data_[tail_] = v;
        tail_ = (tail_ + 1) % k_;
        ++count_;
        return true;
    }

    bool deQueue() {
        if (count_ == 0) return false;
        data_[head_] = T{};
        head_ = (head_ + 1) % k_;
        --count_;
        return true;
    }

    T Front() const { return count_ ? data_[head_]                  : T(-1); }
    T Rear()  const { return count_ ? data_[(tail_ - 1 + k_) % k_]  : T(-1); }

    bool isEmpty() const { return count_ == 0; }
    bool isFull()  const { return count_ == k_; }

private:
    int k_;
    Vector<T> data_;
    int head_ = 0, tail_ = 0, count_ = 0;
};

// ─── 4. 工作實用：用 SmallVector 收集少量 token，平均不超過 N ────────────
//   解析簡單命令字串 → token 列表。多數情況不到 8 個 token，避免 heap。
SmallVector<std::string, 8> simple_split(const std::string& s, char sep) {
    SmallVector<std::string, 8> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) { out.emplace_back(std::move(cur)); cur.clear(); }
        } else cur.push_back(c);
    }
    if (!cur.empty()) out.emplace_back(std::move(cur));
    return out;
}

// ─── 5. Leetcode 1051 ── Height Checker (用我們的 Vector + sort) ────────
//   難度: easy
//   題目：給 heights，計算「有多少位置與排序後的版本不同」。
//   範例：[1,1,4,2,1,3] → 排序成 [1,1,1,2,3,4] → 3 個位置不同
//
//   為什麼放在這裡？
//     展示我們自製的 Vector<T> 與 std::sort 相容 (有 random-access iterator)，
//     並結合 perfect-forwarding 的 emplace_back。
//
//   時間：O(n log n)；空間：O(n)。
int height_checker(const std::vector<int>& heights) {
    Vector<int> sorted;
    for (int h : heights) sorted.emplace_back(h);
    std::sort(sorted.begin(), sorted.end());

    int count = 0;
    for (std::size_t i = 0; i < heights.size(); ++i) {
        if (heights[i] != sorted[i]) ++count;
    }
    return count;
}

// ─── 6. 工作實用：sliding window max (用 SmallVector 當底，小視窗高效) ──
//   實務常見：監控指標移動視窗的最大值。視窗 K 小於 16 時用 inline buffer。
SmallVector<int, 16> sliding_window_max(const std::vector<int>& nums, int k) {
    SmallVector<int, 16> result;
    if (k <= 0 || (int)nums.size() < k) return result;
    for (std::size_t i = 0; i + k <= nums.size(); ++i) {
        int mx = nums[i];
        for (int j = 1; j < k; ++j) if (nums[i + j] > mx) mx = nums[i + j];
        result.emplace_back(mx);
    }
    return result;
}

// ─── main ────────────────────────────────────────────────────────────────
int main() {
    // (1) Vector
    Vector<int> v;
    for (int i = 1; i <= 5; ++i) v.emplace_back(i * i);
    std::cout << "Vector size = " << v.size() << ", cap = " << v.capacity() << "\n";
    std::cout << "elements: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";
    std::sort(v.begin(), v.end(), std::greater<int>{});
    std::cout << "sorted desc: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";

    // (2) SmallVector
    SmallVector<int, 4> sv;
    for (int i = 1; i <= 3; ++i) sv.emplace_back(i);
    std::cout << "SmallVector size=" << sv.size()
              << " inline=" << std::boolalpha << sv.inline_storage() << "\n";
    for (int i = 4; i <= 8; ++i) sv.emplace_back(i);          // 超過 N 切 heap
    std::cout << "SmallVector size=" << sv.size()
              << " inline=" << sv.inline_storage() << "\n";

    // (3) Leetcode 622
    MyCircularQueue<int> q(3);
    std::cout << "enq(1) = " << q.enQueue(1) << "\n";
    std::cout << "enq(2) = " << q.enQueue(2) << "\n";
    std::cout << "enq(3) = " << q.enQueue(3) << "\n";
    std::cout << "enq(4) = " << q.enQueue(4) << " (expect 0/false)\n";
    std::cout << "Rear   = " << q.Rear()     << " (expect 3)\n";
    std::cout << "isFull = " << q.isFull()   << "\n";
    std::cout << "deq    = " << q.deQueue()  << "\n";
    std::cout << "enq(4) = " << q.enQueue(4) << "\n";
    std::cout << "Rear   = " << q.Rear()     << " (expect 4)\n";

    // (4) simple_split
    auto tokens = simple_split("the quick brown fox jumps", ' ');
    std::cout << "tokens (" << tokens.size() << "): ";
    for (std::size_t i = 0; i < tokens.size(); ++i) std::cout << tokens[i] << '|';
    std::cout << "\n";

    // (5) Leetcode 1051 Height Checker (使用自製 Vector)
    std::cout << "height_checker({1,1,4,2,1,3}) = "
              << height_checker({1, 1, 4, 2, 1, 3}) << " (expect 3)\n";

    // (6) sliding window max (使用 SmallVector)
    auto w = sliding_window_max({1, 3, -1, -3, 5, 3, 6, 7}, 3);
    std::cout << "sliding max (k=3): ";
    for (std::size_t i = 0; i < w.size(); ++i) std::cout << w[i] << ' ';
    std::cout << "\n";

    return 0;
}

// ============================================================================
//  【系列總結】
//    走完 28 篇，你應該已經建立完整的 C++ Template 心智模型：
//      ◆ Function / Class template 與 instantiation 機制
//      ◆ Template 參數三種類型 (type / NTTP / template template)
//      ◆ Specialization / Partial specialization
//      ◆ Default args / Alias template / Variable template
//      ◆ Variadic / Fold / Perfect forwarding
//      ◆ SFINAE / Type traits / Tag dispatch / if constexpr / Concepts
//      ◆ CRTP / Policy-based / Type erasure
//      ◆ Compilation model / Template + Lambda / Constexpr / Capstone
//
//    後續建議：
//      - 讀 STL 任一容器的原始碼 (e.g. libc++ 的 vector / unordered_map)
//      - 讀 LLVM 的 SmallVector / DenseMap
//      - 寫一個自己的 std::function 與 std::any
//      - 嘗試把舊 SFINAE 程式碼遷移為 C++20 concept
//      - 探索 std::ranges、std::expected、std::format
//
//  Happy Templating!
// ============================================================================
