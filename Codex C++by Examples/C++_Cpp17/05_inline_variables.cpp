/*
 * C++17 教科書：inline variables
 *
 * header 中定義 non-inline global variable 會在多個 translation units 造成 ODR/link error。
 * inline variable 允許相同定義出現在多個 TU，program 中視為同一 entity/address；這是
 * inline function 規則對變數的延伸，不代表 compiler 一定把資料「內嵌」到使用處。
 *
 * 【常見用途】header-only library constants、class 的 inline static data member。
 * namespace-scope constexpr 原本有 internal linkage；`inline constexpr` 明確建立共享常數。
 * 【初始化】動態初始化順序問題仍存在；inline 不會神奇修好跨 TU initialization fiasco。
 * 優先 constexpr constant 或 function-local static。
 * 【常見陷阱】inline 解決 ODR 定義問題，不解決動態初始化順序或共享 mutable state race。
 * 【面試題】inline 的核心語意是 ODR/linkage，不是效能 hint。
 * 【練習】將 practical_limits 改成 struct inline static members。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace basic {
inline constexpr int default_port = 8080;

class Counter {
public:
    Counter() { ++instances; }
    ~Counter() { --instances; }
    Counter(const Counter&) = delete;
    Counter& operator=(const Counter&) = delete;
    inline static int instances = 0;
};

void demo() {
    assert(default_port == 8080 && Counter::instances == 0);
    {
        Counter first;
        Counter second;
        assert(Counter::instances == 2);
    }
    assert(Counter::instances == 0);
}
}  // namespace basic

namespace leetcode {
inline constexpr int maximum_fibonacci_input = 46;  // int 不 overflow 的教材上限。

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 509. Fibonacci Number（費波那契數）
// 題目：回傳 F(n)，其中 F(0)=0、F(1)=1；例如 F(10)=55。
// 為何使用本章主題：inline constexpr 將 int 安全上限放在可由 header 多個 TU 共用的單一定義；迭代演算法本身不依賴 inline。
// 思路：1. previous/current 從 0/1 開始；2. 每輪計算下一項並前移；3. 完成 n 輪後回 previous。
// 複雜度：N 為輸入值；時間 O(N)、額外空間 O(1)。
// 易錯點：本例以 assert 限制 0..46，release build 可能移除；正式 API 要 runtime 驗證並處理更大數值。
// -----------------------------------------------------------------------------
int leetcode_fibonacci(int value) {
    assert(value >= 0 && value <= maximum_fibonacci_input);
    int previous = 0;
    int current = 1;
    for (int index = 0; index < value; ++index) {
        const int next = previous + current;
        previous = current;
        current = next;
    }
    return previous;
}

void leetcode_test() {
    assert(leetcode_fibonacci(0) == 0);
    assert(leetcode_fibonacci(10) == 55);
}
}  // namespace leetcode

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】header-only 區域與批次政策
// 情境：多個 translation unit 都要查詢支援區域及單批最多 500 筆的共同設定。
// 為何使用本章主題：inline variables 允許設定直接定義在 header 而不違反 ODR，所有 TU 指向同一 entity。
// 設計：1. inline const vector 列出兩個區域；2. inline constexpr 保存批次上限；3. 同時驗 region 與 batch_size。
// 成本：目前固定兩區，查詢字串比較成本 O(L)；共享 vector 儲存空間 O(R*L)。
// 上線注意：inline 不解決動態初始化順序或 mutable race；區域增多時應改集合並避免可變全域狀態。
// -----------------------------------------------------------------------------
inline const std::vector<std::string> supported_regions{"us-west", "us-east"};
inline constexpr std::size_t maximum_batch_size = 500U;

bool practical_supported(const std::string& region, std::size_t batch_size) {
    const bool known = region == supported_regions[0] || region == supported_regions[1];
    return known && batch_size <= maximum_batch_size;
}

void practical_test() {
    assert(practical_supported("us-west", 100U));
    assert(!practical_supported("eu", 100U));
    assert(!practical_supported("us-east", 501U));
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "inline variables：ODR、Fibonacci、共享設定測試通過\n";
}

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_inline_variables.cpp' -o '/tmp/codex_cpp_C_Cpp17_05_inline_variables' && '/tmp/codex_cpp_C_Cpp17_05_inline_variables'
//
// === 預期輸出（節錄）===
// inline variables：ODR、Fibonacci、共享設定測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
