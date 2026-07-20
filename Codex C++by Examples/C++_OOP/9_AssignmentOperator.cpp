// ============================================================================
// 課題 9：Copy assignment operator 與 copy-and-swap
// ============================================================================
//
// `left = right` 中 left 已存在，因此 operator= 必須先處理舊資源，再成為 right 的
// 副本。天真的「delete 舊資料，再 new/copy」若 new 丟 exception，left 已被破壞；
// 若 `left = left`，甚至先刪掉自己接下來要讀的來源。
//
// copy-and-swap 寫法：參數按值取得安全副本，swap 所有 state，離開函式時讓副本的
// destructor 清掉 left 舊資源。它自然處理 self-assignment，並提供 strong exception
// guarantee（複製失敗時 left 不變）。代價是永遠建立副本，極端效能路徑可另設計。
//
// 【簽章】`T& operator=(T other)`；回 `*this` 才能支援 `a = b = c`。
// 【陷阱】只 swap 一部分 member 會破壞 invariant；新增 member 時也要更新 swap。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>

class OwnedArray {
public:
    explicit OwnedArray(std::size_t size = 0U)
        : size_(size), data_(size == 0U ? nullptr : new int[size]{}) {}

    OwnedArray(const OwnedArray& other)
        : size_(other.size_), data_(other.size_ == 0U ? nullptr : new int[other.size_])
    {
        std::copy(other.data_, other.data_ + other.size_, data_);
    }

    OwnedArray& operator=(OwnedArray other) noexcept
    {
        swap(*this, other);
        return *this;
    }

    ~OwnedArray() noexcept { delete[] data_; }

    friend void swap(OwnedArray& left, OwnedArray& right) noexcept
    {
        using std::swap;
        swap(left.size_, right.size_);
        swap(left.data_, right.data_);
    }

    std::size_t size() const { return size_; }
    int& at(std::size_t index)
    {
        if (index >= size_) {
            throw std::out_of_range("OwnedArray index");
        }
        return data_[index];
    }
    int at(std::size_t index) const
    {
        if (index >= size_) {
            throw std::out_of_range("OwnedArray index");
        }
        return data_[index];
    }

private:
    std::size_t size_;
    int* data_;
};

void copy_assign(OwnedArray& destination, const OwnedArray& source)
{
    destination = source;
}

void basic_example()
{
    OwnedArray source(2);
    source.at(0) = 10;
    OwnedArray destination(1);
    destination.at(0) = 99;
    destination = source;
    destination.at(0) = 20;
    assert(source.at(0) == 10 && destination.at(0) == 20);
    // 經函式傳入同一物件，確實測到 runtime self-assignment 路徑。
    copy_assign(destination, destination);
    assert(destination.at(0) == 20);
    std::cout << "[基礎] copy-and-swap 深複製並安全處理 self-assignment\n";
}

// LeetCode 706：Design HashMap。
// Judge 的 put/get/remove 之外，本實作底層 OwnedArray 也能正確 copy-assign，
// 因此可在測試或 speculative update 前建立獨立副本。
class MyHashMap {
public:
    MyHashMap() : values_(kRange)
    {
        for (std::size_t index = 0U; index < kRange; ++index) {
            values_.at(index) = -1;
        }
    }

    void put(int key, int value) { values_.at(checked(key)) = value; }
    int get(int key) const { return values_.at(checked(key)); }
    void remove(int key) { values_.at(checked(key)) = -1; }

private:
    static constexpr std::size_t kRange = 10'001U; // 範例縮小；LC 原限制可改 1,000,001。
    static std::size_t checked(int key)
    {
        if (key < 0 || key >= static_cast<int>(kRange)) {
            throw std::out_of_range("hash-map key");
        }
        return static_cast<std::size_t>(key);
    }
    OwnedArray values_;
};

void leetcode_706_example()
{
    MyHashMap original;
    original.put(1, 10);
    MyHashMap backup;
    backup = original;
    original.put(1, 20);
    assert(original.get(1) == 20 && backup.get(1) == 10);
    backup.remove(1);
    assert(backup.get(1) == -1);
    std::cout << "[LeetCode 706] map 備份與原物件互不共享 array\n";
}

// 實務案例：transaction staging。先複製目前 limits，驗證成功後才 assignment commit。
void practical_example()
{
    OwnedArray active(2);
    active.at(0) = 4;
    active.at(1) = 8;
    OwnedArray candidate(active);
    candidate.at(0) = 16;
    const bool validation_ok = candidate.at(0) <= 32;
    if (validation_ok) {
        active = candidate;
    }
    assert(active.at(0) == 16);
    std::cout << "[實務] 驗證後以 assignment 原子式提交設定\n";
}

int main()
{
    basic_example();
    leetcode_706_example();
    practical_example();
}

// 練習：加入 move constructor/move assignment，觀察 copy-and-swap 參數如何利用 move。
// 複雜度：deep assignment O(N)，copy-and-swap 另建 temporary；self-assignment 仍安全但可能多做工。
// 生命週期：swap 後 temporary 擁有舊資源並在函式結尾釋放，目標只接手完整新狀態。

/*
【本課面試問答】
Q1：copy-and-swap 為何可提供 strong exception guarantee？
A：by-value 參數先完整複製；複製失敗時目標尚未改變。成功後 `swap` 應為 noexcept，舊資源交給
temporary 在函式結尾清理，所以結果是「完整成功」或「原狀不變」。代價是可能多一次配置/複製。

Q2：`if (this == &rhs)` 是否一定需要？
A：手寫先 delete 再 copy 的 assignment 需要防 self-assignment，否則會讀已釋放資料；良好的
copy-and-swap 天生安全，不必特判。是否特判應由正確性與實測成本決定，不能靠習慣加入。

Q3：move assignment 要處理 self-move 嗎？
A：泛型演算法可能讓物件被 self-move；類別至少應維持可解構、可指定的不變式。最簡單是資源操作
天然安全或先檢查地址，但不應宣稱 self-move 後值保持不變，除非類別契約明訂。
*/
