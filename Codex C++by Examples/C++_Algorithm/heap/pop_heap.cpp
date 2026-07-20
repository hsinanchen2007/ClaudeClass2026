/*
 * std::pop_heap：把 heap 頂端移到範圍尾端，但不刪除
 * ===================================================
 * 呼叫後，原本 front 的值位於 last-1，而 [first,last-1) 仍是 heap。
 * 若要真正移除，下一步才 container.pop_back()。這個二階段設計讓你先讀取或
 * move 出被選中的值。空範圍不可 pop_heap。
 *
 * 複雜度 O(log N)，不改 size/capacity；同一 heap 的 make/push/pop 必須使用
 * 同一 comparator。
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int pop_max(std::vector<int>& heap) {
    assert(!heap.empty());
    std::pop_heap(heap.begin(), heap.end());
    const int result = heap.back();
    heap.pop_back();
    return result;
}

// LeetCode 1046：Last Stone Weight。
int leetcode_last_stone_weight(std::vector<int> stones) {
    std::make_heap(stones.begin(), stones.end());
    while (stones.size() > 1U) {
        const int first = pop_max(stones);
        const int second = pop_max(stones);
        if (first != second) {
            stones.push_back(first - second);
            std::push_heap(stones.begin(), stones.end());
        }
    }
    return stones.empty() ? 0 : stones.front();
}

// 實務：priority 數字越大越先執行，依序排出處理順序。
std::vector<int> practical_dispatch_order(std::vector<int> priorities) {
    std::make_heap(priorities.begin(), priorities.end());
    std::vector<int> result;
    while (!priorities.empty()) {
        result.push_back(pop_max(priorities));
    }
    return result;
}

int main() {
    std::vector<int> heap{2, 9, 4, 7};
    std::make_heap(heap.begin(), heap.end());
    assert(pop_max(heap) == 9);
    assert(std::is_heap(heap.begin(), heap.end()));

    assert(leetcode_last_stone_weight({2, 7, 4, 1, 8, 1}) == 1);
    assert(leetcode_last_stone_weight({1}) == 1);
    assert((practical_dispatch_order({3, 10, 5}) ==
            std::vector<int>{10, 5, 3}));

    std::cout << "pop_heap：取頂、石頭碰撞、工作派送測試通過\n";
}

/*
 * 常見 bug：只呼叫 pop_heap 卻忘記 pop_back，下一次又把已取出的尾端包含進來；
 * 或先保存 front 的 reference，再 pop_back/reallocation 後繼續使用。
 * 面試：priority_queue 封裝了同樣概念；需要直接巡覽/批次 heap 演算法時才操作
 * vector。練習：用 min-heap 實作依 deadline 由小到大的派送。
 *
 * 【LeetCode 複雜度】
 * 每輪最多三個 heap 操作，總共 O(N log N)，空間 O(N)。若兩石相等，不要把 0
 * push 回 heap，否則增加無意義工作但答案雖可能仍正確。
 *
 * 【實務例外安全】
 * 若元素 move/swap 或 comparator 會丟例外，heap 可能只保證基本例外安全；工作
 * 排程資料通常讓 comparator noexcept 且只比較 primitive key，降低恢復複雜度。
 * 易錯陷阱：pop_max 取得的是 value copy；若元素是 unique_ptr，應在 pop_heap 後
 * 從 back std::move 出所有權，再 pop_back，不能嘗試複製。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'pop_heap.cpp' -o '/tmp/codex_cpp_C_Algorithm_heap_pop_heap' && '/tmp/codex_cpp_C_Algorithm_heap_pop_heap'
//
// === 預期輸出（節錄）===
// pop_heap：取頂、石頭碰撞、工作派送測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
