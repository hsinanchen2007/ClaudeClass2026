/*
 * std::fill / fill_n：以同一個值指定範圍
 * =======================================
 * fill(first,last,value) 寫入 [first,last)，O(N)；fill_n(out,count,value) 寫 count
 * 個並回傳尾後 iterator。它們不改容器 size，目的範圍必須有效。
 * 對 vector<bool> 等 proxy reference 仍可工作；對不可 copy-assign 的型別不可用。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

// LeetCode 73：Set Matrix Zeroes，示範以 fill 清掉整列。
void leetcode_set_zeroes(std::vector<std::vector<int>>& matrix) {
    if (matrix.empty() || matrix.front().empty()) {
        return;
    }
    const std::size_t rows = matrix.size();
    const std::size_t cols = matrix.front().size();
    std::vector<bool> zero_row(rows, false);
    std::vector<bool> zero_col(cols, false);
    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t col = 0; col < cols; ++col) {
            if (matrix[row][col] == 0) {
                zero_row[row] = true;
                zero_col[col] = true;
            }
        }
    }
    for (std::size_t row = 0; row < rows; ++row) {
        if (zero_row[row]) {
            std::fill(matrix[row].begin(), matrix[row].end(), 0);
        }
    }
    for (std::size_t col = 0; col < cols; ++col) {
        if (zero_col[col]) {
            for (auto& row : matrix) {
                row[col] = 0;
            }
        }
    }
}

// 實務：重用固定大小 byte buffer 前，先清除舊資料避免資訊殘留。
void practical_clear_buffer(std::vector<unsigned char>& buffer) {
    std::fill(buffer.begin(), buffer.end(), static_cast<unsigned char>(0));
}

int main() {
    std::vector<int> values(5, -1);
    std::fill(values.begin() + 1, values.begin() + 4, 7);
    assert((values == std::vector<int>{-1, 7, 7, 7, -1}));
    std::fill_n(values.begin(), 2, 0);
    assert((values == std::vector<int>{0, 0, 7, 7, -1}));

    std::vector<std::vector<int>> matrix{{1, 1, 1}, {1, 0, 1}, {1, 1, 1}};
    leetcode_set_zeroes(matrix);
    assert((matrix == std::vector<std::vector<int>>{{1, 0, 1},
                                                     {0, 0, 0},
                                                     {1, 0, 1}}));

    std::vector<unsigned char> secret{1U, 2U, 3U};
    practical_clear_buffer(secret);
    assert((secret == std::vector<unsigned char>{0U, 0U, 0U}));
    std::cout << "fill：區間填值、LC73、buffer 清理測試通過\n";
}

/*
 * 資安提醒：一般 fill 可能被最佳化器移除，不能當密碼金鑰的保證式安全清除；應用
 * 平台提供的 explicit_bzero/memset_s 等 API。練習：以 fill_n 寫 ring buffer
 * reset，驗證 count 不超過可寫空間。
 *
 * 【LeetCode 解題觀察】
 * LC73 的 fill 只能負責寫值；真正難點是先記錄哪些列/欄原本含 0。若邊掃邊清，
 * 新寫入的 0 會被誤當成原始訊號，最後可能把整張矩陣清空。
 *
 * 【實務判斷】
 * fill 適合重設既有 storage。若要建立 N 個相同值，vector(count,value) 通常更
 * 直接；若值由索引計算，應改用 generate/transform。對大型物件重複 copy-assign
 * 可能昂貴，應檢查資料結構是否真的需要完整重設。
 * 易錯陷阱：fill_n 的負 count 在新標準與舊實作語意需避免，API 邊界直接用
 * size_t/先驗證。面試時要能說 fill 不配置、不改 size，只做 assignment。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'fill.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_fill' && '/tmp/codex_cpp_C_Algorithm_modifying_fill'
//
// === 預期輸出（節錄）===
// fill：區間填值、LC73、buffer 清理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
