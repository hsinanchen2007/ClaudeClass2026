/*
 * std::move 演算法：把來源範圍逐一 move-assign 到目的範圍
 * =======================================================
 * 注意有兩個同名概念：<utility> 的 std::move 是 cast；<algorithm> 的四參數
 * std::move(first,last,out) 才是範圍演算法。複雜度 O(N)。來源元素移動後仍然
 * valid，但內容 unspecified；可以銷毀、重新指定，不可假設仍保留舊值。
 *
 * 目的端需有空間，或搭配 move_iterator/back_inserter。危險重疊向右搬應使用
 * move_backward。對 int 等 trivial type，move 與 copy 效果相同。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 189. Rotate Array（輪轉陣列）
// 題目：將 nums 向右輪轉 k 步；例如 [1,2,3,4,5,6,7]、k=3 變成
// [5,6,7,1,2,3,4]。
// 為何使用本章主題：範圍 std::move 將尾段與前段搬到暫存輸出的正確位置；對 int
// 效果等同 copy，本例用來展示演算法版 move，並非 O(1) 空間最佳解。
// 思路：1. 空陣列直接回；2. 對 N 取 k 餘數；3. move 尾 K 項到輸出前端；4. move
// 其餘項到後端，再把 output 所有權移回 nums。
// 複雜度：時間 O(N)、額外空間 O(N)，N 為 nums 的元素數。
// 易錯點：空陣列不可做 k%N；來源元素 move 後內容未指定；最佳題解可用三次 reverse 降到 O(1) 空間。
// -----------------------------------------------------------------------------
void leetcode_rotate_right(std::vector<int>& nums, std::size_t k) {
    if (nums.empty()) {
        return;
    }
    k %= nums.size();
    std::vector<int> output(nums.size());
    std::move(nums.end() - static_cast<std::ptrdiff_t>(k), nums.end(),
              output.begin());
    std::move(nums.begin(), nums.end() - static_cast<std::ptrdiff_t>(k),
              output.begin() + static_cast<std::ptrdiff_t>(k));
    nums = std::move(output);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Move-only 工作批次移交
// 情境：pending 以 unique_ptr 擁有工作名稱，要把整批所有權交給 ready queue，不可
// 複製工作，也不能讓來源保留重複 owner。
// 為何使用本章主題：range std::move 配 back_inserter 逐項 move unique_ptr，目的
// vector 可自動成長；相較 copy，這是 move-only 型別唯一合法的所有權轉移。
// 設計：1. 依合併後大小 reserve ready；2. move 所有 pending 元素到 ready 尾端；
// 3. clear 已成 moved-from 的 pending。
// 成本：時間 O(P)、額外配置最多 O(R+P)，P/R 為 pending/ready 數；工作本體不被複製。
// 上線注意：中途配置或 move 例外可能形成部分移交；需要 all-or-nothing 時應設計 staging/commit。
// -----------------------------------------------------------------------------
using Job = std::unique_ptr<std::string>;

void practical_transfer_jobs(std::vector<Job>& pending,
                             std::vector<Job>& ready) {
    ready.reserve(ready.size() + pending.size());
    std::move(pending.begin(), pending.end(), std::back_inserter(ready));
    pending.clear();  // moved-from unique_ptr 可安全銷毀；清楚表達來源已消耗。
}

int main() {
    std::vector<std::string> source{"alpha", "beta"};
    std::vector<std::string> destination(2);
    std::move(source.begin(), source.end(), destination.begin());
    assert((destination == std::vector<std::string>{"alpha", "beta"}));
    // 不 assert source 字串內容；標準只保證 valid but unspecified。

    std::vector<int> nums{1, 2, 3, 4, 5, 6, 7};
    leetcode_rotate_right(nums, 3U);
    assert((nums == std::vector<int>{5, 6, 7, 1, 2, 3, 4}));

    std::vector<Job> pending;
    pending.push_back(std::make_unique<std::string>("compile"));
    pending.push_back(std::make_unique<std::string>("test"));
    std::vector<Job> ready;
    practical_transfer_jobs(pending, ready);
    assert(pending.empty() && ready.size() == 2U && *ready[1] == "test");
    std::cout << "move：所有權、LC189、工作移交測試通過\n";
}

/*
 * 面試：const T 無法真正 move 到只提供 T&& 的 move constructor，因 std::move
 * 產生 const T&&，通常退回 copy。不要為「保險」到處 std::move，會阻止 NRVO。
 * 練習：寫 move_backward 將 vector 內容向右移一格，先 resize 再搬移。
 *
 * 【LeetCode 複雜度】
 * LC189 此版本時間 O(N)、額外空間 O(N)；三次 reverse 可降為 O(1) 額外空間。
 * k 必須先對 size 取餘數，並先處理空 vector，否則 modulo zero 未定義。
 *
 * 【實務所有權】
 * transfer 完成後主動 clear pending，不是 move 的必要條件，而是 API 契約的清楚
 * 表達。若中途 move assignment 丟例外，兩個容器可能處於部分移交狀態；需要強
 * 例外保證時應設計 staging/commit，而非假設演算法自動 rollback。
 * 易錯陷阱：目的 vector 只有 reserve 沒 resize 時，destination.begin() 仍不可寫；
 * move-only 元素應用 back_inserter，或先建立正確數量的目的元素。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'move.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_move' && '/tmp/codex_cpp_C_Algorithm_modifying_move'
//
// === 預期輸出（節錄）===
// move：所有權、LC189、工作移交測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
