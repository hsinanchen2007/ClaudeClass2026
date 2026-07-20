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

// LeetCode 303：Range Sum Query - Immutable 的固定尺寸版本。
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

enum class Endian { little, big };

// 實務：協定欄位寬度與位元序在編譯期固定，錯誤組合可及早被拒絕。
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
