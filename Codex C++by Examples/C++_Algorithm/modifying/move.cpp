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

// LeetCode 189：Rotate Array。先把尾端 move 到暫存，再重組；O(N) 額外空間。
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

// 實務：把 move-only 工作所有權從 pending 批次移交給 worker queue。
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
