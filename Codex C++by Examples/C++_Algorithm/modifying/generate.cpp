/*
 * std::generate / generate_n：每個位置呼叫 generator 產生值
 * ========================================================
 * generate(first,last,g) 對每格呼叫 g()，O(N)；generate_n(out,n,g) 回尾後位置。
 * generator 可有狀態，但執行政策平行版不能依賴固定呼叫順序，也必須處理同步。
 * 一般 sequential overload 會按範圍走訪。演算法不增加容器大小；需要 inserter。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// LeetCode 412：Fizz Buzz，以遞增 closure 產生答案。
std::vector<std::string> leetcode_fizz_buzz(int n) {
    std::vector<std::string> result(static_cast<std::size_t>(n));
    int current = 0;
    std::generate(result.begin(), result.end(), [&current] {
        ++current;
        if (current % 15 == 0) {
            return std::string{"FizzBuzz"};
        }
        if (current % 3 == 0) {
            return std::string{"Fizz"};
        }
        if (current % 5 == 0) {
            return std::string{"Buzz"};
        }
        return std::to_string(current);
    });
    return result;
}

// 實務：產生單調 request id。單執行緒範例；多執行緒需 atomic/集中服務。
std::vector<int> practical_allocate_ids(int first_id, std::size_t count) {
    std::vector<int> ids(count);
    int next = first_id;
    std::generate(ids.begin(), ids.end(), [&next] { return next++; });
    return ids;
}

int main() {
    std::vector<int> squares(5);
    int n = 0;
    std::generate(squares.begin(), squares.end(), [&n] {
        const int value = n;
        ++n;
        return value * value;
    });
    assert((squares == std::vector<int>{0, 1, 4, 9, 16}));

    assert((leetcode_fizz_buzz(5) ==
            std::vector<std::string>{"1", "2", "Fizz", "4", "Buzz"}));
    assert((practical_allocate_ids(100, 3U) ==
            std::vector<int>{100, 101, 102}));
    std::cout << "generate：狀態 generator、FizzBuzz、ID 配發測試通過\n";
}

/*
 * 易錯點：lambda 捕捉 local reference 後若被保存到 local 生命之外會懸空；本例
 * generator 只在 generate 呼叫期間使用。隨機數 generator 不要每格重新 seed。
 * 練習：用 mt19937 固定 seed 產生測試資料，測試只驗範圍與可重現性。
 *
 * 【LeetCode 解題觀察】
 * FizzBuzz 並不需要 generate 才能解；此處目的是示範「每個輸出位置由 callable
 * 產生」。面試時優先寫最清楚的 range-for，再視團隊風格使用演算法。
 *
 * 【實務陷阱】
 * ID 配發若跨 thread/process，local int 不具唯一性；需要 atomic、資料庫 sequence
 * 或集中式 ID service。generate 本身不提供交易性，也不會在 generator 丟例外時
 * rollback，前面已寫入的元素仍保持修改後狀態。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'generate.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_generate' && '/tmp/codex_cpp_C_Algorithm_modifying_generate'
//
// === 預期輸出（節錄）===
// generate：狀態 generator、FizzBuzz、ID 配發測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
