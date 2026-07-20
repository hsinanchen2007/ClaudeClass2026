/*
 * std::swap 與 ADL swap：交換兩個物件的狀態
 * ========================================
 * 一般 std::swap(a,b) 以 move construct + move assign 完成；陣列 overload 逐元素交換。
 * 自訂型別若有更有效率/需維持 invariant 的交換，提供同 namespace 的 swap(T&,T&)
 * 並在 generic code 使用 `using std::swap; swap(a,b);`，讓 ADL 找到自訂版本。
 *
 * 複雜度通常 O(1)，陣列/大型固定聚合可能 O(N)。swap 不改物件生命，只交換值/資源。
 * 若要容器操作具強例外安全，move/swap 最好 noexcept。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：原地反轉字元陣列並使用 O(1) 額外空間；例如 hello 變成 olleh。
// 為何使用本章主題：std::swap 交換左右對稱字元，手寫雙指標可清楚展示 swap 在
// 原地演算法中的角色。
// 思路：1. 空輸入直接回；2. left/right 指向首尾；3. left<right 時交換並向中間靠攏。
// 複雜度：時間 O(N)、額外空間 O(1)，N 為字元數。
// 易錯點：空陣列不可先算 size-1；迴圈條件必須是 left<right，避免中點重複處理。
// -----------------------------------------------------------------------------
void leetcode_reverse_string(std::vector<char>& text) {
    if (text.empty()) {
        return;
    }
    std::size_t left = 0;
    std::size_t right = text.size() - 1U;
    while (left < right) {
        std::swap(text[left], text[right]);
        ++left;
        --right;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】驗證後設定原子式發布準備
// 情境：active 是目前設定，staging 已完成驗證；發布時交換兩者，讓 staging 保留舊
// 設定供 rollback，而不逐項複製大型 limits。
// 為何使用本章主題：同 namespace 的 noexcept swap 可透過 ADL 交換 string/vector
// 資源 handle；相較 copy assignment，通常為常數時間且適合 commit 階段。
// 設計：1. Configuration 提供逐 member ADL swap；2. publish 引入 std::swap fallback；
// 3. 交換 active 與 staging 的完整狀態。
// 成本：時間 O(1)、額外空間 O(1)，交換的是容器資源；配置器契約需允許常數時間 swap。
// 上線注意：noexcept swap 不等於 thread-safe；多讀者需 mutex 或 immutable shared_ptr 發布。
// -----------------------------------------------------------------------------
struct Configuration {
    std::string version;
    std::vector<int> limits;

    friend void swap(Configuration& lhs, Configuration& rhs) noexcept {
        using std::swap;
        swap(lhs.version, rhs.version);
        swap(lhs.limits, rhs.limits);
    }
};

void practical_publish(Configuration& active, Configuration& staging) noexcept {
    using std::swap;
    swap(active, staging);
}

int main() {
    int a = 1;
    int b = 2;
    std::swap(a, b);
    assert(a == 2 && b == 1);

    std::vector<char> text{'h', 'e', 'l', 'l', 'o'};
    leetcode_reverse_string(text);
    assert((text == std::vector<char>{'o', 'l', 'l', 'e', 'h'}));

    Configuration active{"v1", {10, 20}};
    Configuration staging{"v2", {30, 40}};
    practical_publish(active, staging);
    assert(active.version == "v2" && active.limits[0] == 30);
    assert(staging.version == "v1");
    std::cout << "swap：LeetCode 344 與實務設定發布測試通過\n";
}

/*
 * 易錯陷阱：在自訂 swap 裡直接 `swap(lhs,rhs)` 會遞迴自己；應逐 member 交換。
 * 不要在 namespace std 為自訂型別新增一般函式 overload（少數允許 specialization
 * 情況也不建議新手使用）。
 *
 * 面試：copy-and-swap assignment 如何提供強例外保證？先按值取得安全副本，再與
 * *this swap；副本解構時帶走舊資源。實務 publish 若有多 thread 讀 active，單純
 * swap 仍不是 thread-safe，需 mutex 或 immutable shared state。
 * 練習：實作 resource-owning class 的 noexcept swap 與 copy assignment。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'swap.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_swap' && '/tmp/codex_cpp_C_Algorithm_modifying_swap'
//
// === 預期輸出（節錄）===
// swap：LeetCode 344 與實務設定發布測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
