/*
 * 第 10 章：std::as_const（C++17）
 *
 * as_const(object) 回傳 const T&，不複製物件。它用來在 non-const context 明確選 const overload，
 * 防止意外修改，或在實作 non-const overload 時重用 const overload。rvalue overload 被刪除，
 * 因為回傳 temporary 的 const reference 很容易懸空。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// 專門示範 const/non-const overload；它沒有依賴 values 的衍生 cache，修改後仍維持 invariant。
class OverloadedStore {
public:
    explicit OverloadedStore(std::vector<int> values) : values_(std::move(values)) {}

    int& at(std::size_t index) {
        ++mutable_reads_;
        return values_.at(index);
    }

    const int& at(std::size_t index) const { return values_.at(index); }
    int mutable_reads() const noexcept { return mutable_reads_; }

private:
    std::vector<int> values_;
    int mutable_reads_{};
};

// LeetCode 303 使用不可變輸入建立 prefix sum；建構後不提供任何可寫 reference。
class NumberSeries {
public:
    explicit NumberSeries(const std::vector<int>& values) {
        prefix_.reserve(values.size() + 1U);
        prefix_.push_back(0);
        for (int value : values) {
            prefix_.push_back(prefix_.back() + value);
        }
    }

    int sum_range(std::size_t left, std::size_t right) const {
        return prefix_.at(right + 1U) - prefix_.at(left);
    }

private:
    std::vector<int> prefix_;
};

// LeetCode 303：Range Sum Query，查詢介面是 const，不應修改 cache 或來源值。
int leetcode_range_sum(const NumberSeries& series, std::size_t left, std::size_t right) {
    return series.sum_range(left, right);
}

// 實務：即使 caller 手上是 mutable object，稽核流程明確只讀，強制選 const overload。
int practical_audit_first(OverloadedStore& store) {
    return std::as_const(store).at(0U);
}

int main() {
    OverloadedStore store({-2, 0, 3, -5, 2, -1});

    int& mutable_value = store.at(1U);
    mutable_value = 10;
    assert(store.mutable_reads() == 1);

    // as_const 不複製，型別是 const OverloadedStore&。
    const int& read_only = std::as_const(store).at(1U);
    assert(read_only == 10);
    assert(store.mutable_reads() == 1); // const overload 不增加計數

    // Prefix sum 是不可變 query object，不與可寫 store 共用同一 invariant。
    NumberSeries immutable({-2, 0, 3, -5, 2, -1});
    assert(leetcode_range_sum(immutable, 0U, 2U) == 1);
    assert(leetcode_range_sum(immutable, 2U, 5U) == -1);

    assert(practical_audit_first(store) == -2);
    std::cout << "as_const 測試完成\n";
}

/*
 * 【常見陷阱】
 * - as_const 是 shallow const：若物件持有 pointer，仍可能透過 pointer 修改 pointee。
 * - const_cast 去 const 後修改原本真正 const 的物件是 UB；as_const 不需要 cast 回去。
 * - mutable 成員仍可在 const 函式改動，應只用於 cache/同步等邏輯 const 情境。
 * - 本例將可寫 store 與 immutable prefix query 拆開，避免 mutable reference 使 cache 失效。
 *
 * 【面試段落】const member function 的 this 型別近似 `T const* const`，不能修改一般成員。
 * 【練習】為 NumberSeries 加 update(index,value)，但必須同步修正受影響的所有 prefix。
 */
