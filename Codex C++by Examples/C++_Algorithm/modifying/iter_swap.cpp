/*
 * std::iter_swap：交換兩個 iterator 所指的元素
 * ============================================
 * iter_swap(a,b) 等價概念是 swap(*a,*b)，但能正確支援某些 proxy reference 與
 * 自訂 iterator。複雜度取決於元素 swap，通常 O(1)。它不使 iterator 自身移動，
 * 也不改容器大小；兩 iterator 必須可解參考且元素可交換。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// LeetCode 283：Move Zeroes。write 指向下一個非零應放的位置；iter_swap 保序。
void leetcode_move_zeroes(std::vector<int>& nums) {
    auto write = nums.begin();
    for (auto read = nums.begin(); read != nums.end(); ++read) {
        if (*read != 0) {
            std::iter_swap(write, read);
            ++write;
        }
    }
}

// 教學：selection sort 展示 iter_swap；O(N^2)，正式排序請用 std::sort。
void selection_sort(std::vector<int>& values) {
    for (auto first = values.begin(); first != values.end(); ++first) {
        const auto smallest = std::min_element(first, values.end());
        std::iter_swap(first, smallest);
    }
}

// 實務：原地把選中的 active record 移到目前處理位置。
struct Record {
    std::string id;
    bool active;
};

void practical_active_first(std::vector<Record>& records) {
    auto out = records.begin();
    for (auto it = records.begin(); it != records.end(); ++it) {
        if (it->active) {
            std::iter_swap(out, it);
            ++out;
        }
    }
}

int main() {
    std::vector<int> values{1, 2, 3};
    std::iter_swap(values.begin(), values.begin() + 2);
    assert((values == std::vector<int>{3, 2, 1}));

    std::vector<int> zeros{0, 1, 0, 3, 12};
    leetcode_move_zeroes(zeros);
    assert((zeros == std::vector<int>{1, 3, 12, 0, 0}));

    std::vector<int> unsorted{4, 2, 3, 1};
    selection_sort(unsorted);
    assert((unsorted == std::vector<int>{1, 2, 3, 4}));

    std::vector<Record> records{{"A", false}, {"B", true}, {"C", true}};
    practical_active_first(records);
    assert(records[0].id == "B" && records[1].id == "C");
    std::cout << "iter_swap：LC283、selection sort、record 分區測試通過\n";
}

/*
 * 陷阱：傳入 end iterator 不能解參考；跨不相容容器的 iterator 也不可交換。
 * 面試：為何不用 std::swap(it1,it2)？那只會交換 iterator 變數，不交換元素。
 * 練習：用 iter_swap 手寫 partition，並維持迴圈不變量說明。
 *
 * 【LeetCode 不變量】
 * LC283 執行時，[begin,write) 永遠是已處理且保序的非零元素；[write,read) 是
 * 已處理的零。read 找到非零便交換到 write，因此不需第二次填零。
 *
 * 【實務選擇】
 * active_first 的寫法會保留 active 元素相對順序，但 inactive 區的順序未明確作為
 * 契約；若兩區都必須穩定，直接使用 stable_partition 表意更清楚但可能用額外空間。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'iter_swap.cpp' -o '/tmp/codex_cpp_C_Algorithm_modifying_iter_swap' && '/tmp/codex_cpp_C_Algorithm_modifying_iter_swap'
//
// === 預期輸出（節錄）===
// iter_swap：LC283、selection sort、record 分區測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
