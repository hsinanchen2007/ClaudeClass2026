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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 73. Set Matrix Zeroes（矩陣置零）
// 題目：若 m*n 矩陣任一元素為 0，將其整列與整欄設為 0；例如中央為 0 的 3*3
// 全 1 矩陣，輸出中央列與中央欄皆為 0。
// 為何使用本章主題：先標記原始零所在列欄，再以 std::fill 一次清掉整列；欄則逐列
// 指定。本版用 O(R+C) 標記空間，未採原題可達 O(1) 的最佳技巧。
// 思路：1. 掃描矩陣記錄 zero_row/zero_col；2. fill 所有被標記列；3. 逐格清除被標記欄。
// 複雜度：時間 O(R*C)、額外空間 O(R+C)，R/C 分別為列數與欄數。
// 易錯點：不可邊掃原始零邊清除，否則新寫的零會擴散；輸入必須是矩形矩陣。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】可重用 Byte Buffer 一般清零
// 情境：固定大小 buffer 即將交給下一個普通資料處理階段，先把舊 byte 全設為 0，
// 避免下一次邏輯誤讀殘留內容。
// 為何使用本章主題：std::fill 對既有儲存空間逐格指定同一值，不改 size，也不需要
// 重新配置新 vector。
// 設計：1. 取得整個 buffer 半開區間；2. 以 unsigned char 0 填滿所有位置。
// 成本：時間 O(B)、額外空間 O(1)，B 為 buffer byte 數；主要成本是記憶體寫入頻寬。
// 上線注意：這不是密碼金鑰的保證式安全抹除，最佳化器可能移除寫入；秘密資料要用平台 secure-zero API。
// -----------------------------------------------------------------------------
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
