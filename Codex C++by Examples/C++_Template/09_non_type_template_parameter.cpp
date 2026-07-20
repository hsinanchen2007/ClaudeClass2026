/*
 * 第 09 章：非型別模板參數（NTTP）
 *
 * NTTP 把編譯期常數放進型別：array<int,4> 與 array<int,8> 不同型別。
 * C++20 可接受整數、enum、pointer/reference（限制很多）以及符合 structural type
 * 規則的類別值。常見用途是固定尺寸、策略旗標、維度與編譯期字串。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <stdexcept>

template <typename T, std::size_t Rows, std::size_t Columns>
class Matrix {
public:
    T& at(std::size_t row, std::size_t column) {
        if (row >= Rows || column >= Columns) {
            throw std::out_of_range("Matrix index");
        }
        return data_[row * Columns + column];
    }

    const T& at(std::size_t row, std::size_t column) const {
        if (row >= Rows || column >= Columns) {
            throw std::out_of_range("Matrix index");
        }
        return data_[row * Columns + column];
    }

private:
    std::array<T, Rows * Columns> data_{};
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 303. Range Sum Query - Immutable（區域和檢索：不可變）
// 題目：由整數陣列建立物件，多次查詢閉區間 [left,right] 總和；[-2,0,3] 的 [0,2] 為 1。
// 為何使用本章主題：N 是 NTTP，PrefixSum<N> 在型別與 std::array 儲存中固定輸入長度，
// 並可在 constant evaluation 建構；原題長度在 runtime，這是固定尺寸教學版。
// 思路：建構時建立前綴和陣列；查詢以 prefix[right+1]-prefix[left] 回答。
// 複雜度：建構時間 O(N)、空間 O(N)，每次查詢時間 O(1)，N 是編譯期元素數。
// 易錯點：必須滿足 left<=right<N；函式未做邊界檢查，前綴 int 加總也可能溢位。
// -----------------------------------------------------------------------------
template <std::size_t N>
class PrefixSum {
public:
    constexpr explicit PrefixSum(const std::array<int, N>& values) {
        for (std::size_t i = 0; i < N; ++i) {
            prefix_[i + 1] = prefix_[i] + values[i];
        }
    }

    constexpr int sum_range(std::size_t left, std::size_t right) const {
        return prefix_[right + 1] - prefix_[left];
    }

private:
    std::array<int, N + 1> prefix_{};
};

template <std::size_t N>
int leetcode_range_sum(const PrefixSum<N>& prefix, std::size_t left, std::size_t right) {
    return prefix.sum_range(left, right);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】固定寬度協定整數編碼
// 情境：網路協定欄位在規格中固定為 Bytes 個位元組，並明訂 little 或 big endian。
// 為何使用本章主題：Bytes 與 Endian 都是 NTTP，非法寬度在實體化時由 static_assert 拒絕；
// 相較 runtime 參數，可得到固定大小回傳型別並消除每次的 policy 分支。
// 設計：逐次取 value 最低 8 bits；依 endian 算目標索引；寫入後右移處理下一 byte。
// 成本：時間與回傳空間 O(Bytes)，Bytes 是編譯期常數，沒有動態配置。
// 上線注意：超過 Bytes 可容納範圍的高位會被截斷；需先驗值域並與協定 signedness 對齊。
// -----------------------------------------------------------------------------
enum class Endian { little, big };

template <std::size_t Bytes, Endian Order>
std::array<unsigned char, Bytes> practical_encode_unsigned(unsigned int value) {
    static_assert(Bytes >= 1 && Bytes <= sizeof(unsigned int));
    std::array<unsigned char, Bytes> result{};
    for (std::size_t i = 0; i < Bytes; ++i) {
        const std::size_t target = Order == Endian::little ? i : Bytes - 1U - i;
        result[target] = static_cast<unsigned char>(value & 0xFFU);
        value >>= 8U;
    }
    return result;
}

int main() {
    Matrix<int, 2, 3> matrix;
    matrix.at(1, 2) = 42;
    assert(matrix.at(1, 2) == 42);

    constexpr PrefixSum prefix(std::array{-2, 0, 3, -5, 2, -1});
    static_assert(prefix.sum_range(0, 2) == 1);
    assert(leetcode_range_sum(prefix, 2U, 5U) == -1);

    const auto bytes = practical_encode_unsigned<2, Endian::big>(0x1234U);
    assert(bytes[0] == 0x12U && bytes[1] == 0x34U);

    std::cout << "NTTP 測試完成\n";
}

/*
 * 【複雜度】PrefixSum 建構 O(N)，每次 query O(1)，空間 O(N)。
 * 【陷阱】不同 N 的型別不能直接互相指定；這既是安全性也是泛用 API 的負擔。
 * 【陷阱】NTTP 若直接使用 floating point，跨編譯器/標準版本需確認支援與語意。
 * 【面試】為何將 Rows/Columns 放模板而非成員？可讓尺寸進型別並支援 constexpr 展開。
 * 【練習】實作只接受相容維度的 Matrix 乘法。
 */

/*
 * 【教科書補充：NTTP 版本與數值契約】
 * - 整數、enum、pointer/reference 等 NTTP 早已有之；structural class type 與浮點支援是較晚標準擴充。
 * - PrefixSum::sum_range 的前置條件是 left<=right<N；本例有效輸入不代表公開 API 可省略驗證。
 * - prefix 累加仍可能發生 signed overflow；「在編譯期」不會自動變成 arbitrary precision。
 * - encode 的 Bytes 只決定輸出寬度，超出寬度的高位目前會截斷；協定 API 應先拒絕超界值。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '09_non_type_template_parameter.cpp' -o '/tmp/codex_cpp_C_Template_09_non_type_template_parameter' && '/tmp/codex_cpp_C_Template_09_non_type_template_parameter'
//
// === 預期輸出（節錄）===
// NTTP 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
