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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 412. Fizz Buzz（Fizz Buzz）
// 題目：輸入正整數 n，產生 1..n；3 的倍數改為 Fizz、5 的倍數改為 Buzz、兩者
// 公倍數改為 FizzBuzz，例如 n=5 得 ["1","2","Fizz","4","Buzz"]。
// 為何使用本章主題：generate 對每個已配置輸出位置呼叫具狀態 closure；current
// 依序遞增並決定該位置字串。一般迴圈更直觀，本例是 API 教學改寫。
// 思路：1. 建立 n 格輸出；2. generator 每次先遞增 current；3. 依 15、3、5 倍數
// 順序回字串；4. 其他值轉十進位文字。
// 複雜度：時間 O(N)、額外空間 O(N)，N=n，字串轉換成本包含在輸出規模內。
// 易錯點：n 必須非負，否則轉 size_t 會產生巨大配置；15 的判斷必須先於 3 與 5。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】單執行緒批次 Request ID 配發
// 情境：單一程序要從 first_id 起產生 count 個連續測試或批次 request id，例如
// 100 起三筆得到 [100,101,102]。
// 為何使用本章主題：generate 讓具狀態 next closure 對每格產生不同值；相較 fill，
// 每次輸出會隨 generator 狀態遞增。
// 設計：1. 建立 count 格 ids；2. next 從 first_id 初始化；3. 每次回目前值後遞增。
// 成本：時間 O(C)、額外空間 O(C)，C=count。
// 上線注意：int 遞增可能溢位；跨執行緒或程序不能靠區域變數保唯一，需 atomic、資料庫 sequence 或 ID 服務。
// -----------------------------------------------------------------------------
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
