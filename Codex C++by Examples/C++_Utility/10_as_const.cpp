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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（區域和檢索：不可變）
// 題目：以整數陣列建立查詢物件，多次回傳閉區間 [left,right] 總和；[-2,0,3] 的 [0,2] 為 1。
// 為何使用本章主題：NumberSeries 只暴露 const 查詢，呼應 as_const 的唯讀介面選擇；實作路徑
// 並未直接呼叫 std::as_const，因此是 const-correctness 的相鄰教學案例。
// 思路：建構時先放 0；逐項累加 prefix；查詢用 prefix[right+1]-prefix[left]。
// 複雜度：建構時間 O(N)、空間 O(N)，每次查詢 O(1)，N 是原陣列長度。
// 易錯點：必須滿足 left<=right<N；at 越界會丟例外，int 前綴和也可能溢位。
// -----------------------------------------------------------------------------
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

int leetcode_range_sum(const NumberSeries& series, std::size_t left, std::size_t right) {
    return series.sum_range(left, right);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】強制唯讀的稽核查詢
// 情境：稽核流程收到可修改的 Store 參考，但政策要求只讀第一筆且不能觸發 mutable access 計數。
// 為何使用本章主題：std::as_const 建立零複製 const view，強制 overload resolution 選 const at；
// 相較 const_cast 或複製整個 store，意圖明確且不改變 ownership。
// 設計：接收 mutable store；轉成 const reference view；呼叫 const at 並以值回傳第一筆。
// 成本：轉換與查詢皆 O(1)，不複製 Store；vector::at 仍包含邊界檢查。
// 上線注意：空 store 會丟 out_of_range；as_const 是 shallow const，內部指標指向的資料未必不可寫。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_as_const.cpp' -o '/tmp/codex_cpp_C_Utility_10_as_const' && '/tmp/codex_cpp_C_Utility_10_as_const'
//
// === 預期輸出（節錄）===
// as_const 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
