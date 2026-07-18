// 檔案：lesson_5_2_futex_concept.cpp
// 說明：Futex 概念的簡化說明（非實際實現）

#include <iostream>
#include <atomic>

/*
 * 這是 Futex 的概念說明，不是實際可編譯的程式碼
 * 
 * Futex 的核心思想：
 * 
 * struct futex_mutex {
 *     atomic<int> state;  // 0 = unlocked, 1 = locked (無等待者), 2 = locked (有等待者)
 * };
 * 
 * void lock() {
 *     int expected = 0;
 *     
 *     // Fast path：嘗試 0 → 1（無競爭）
 *     if (state.compare_exchange_strong(expected, 1)) {
 *         return;  // 成功獲取，無需系統呼叫
 *     }
 *     
 *     // Slow path：有競爭
 *     while (true) {
 *         // 標記為「有等待者」
 *         if (expected == 2 || state.compare_exchange_strong(expected, 2)) {
 *             // 系統呼叫：等待 state 變化
 *             futex_wait(&state, 2);
 *         }
 *         
 *         expected = 0;
 *         if (state.compare_exchange_strong(expected, 2)) {
 *             return;  // 成功獲取
 *         }
 *     }
 * }
 * 
 * void unlock() {
 *     // 如果 state 從 1 變成 0，無等待者
 *     if (state.fetch_sub(1) == 1) {
 *         return;  // 無需喚醒
 *     }
 *     
 *     // 有等待者，需要喚醒
 *     state.store(0);
 *     futex_wake(&state, 1);  // 喚醒一個等待者
 * }
 */

int main() {
    std::cout << "Futex 是 Linux 實現高效互斥鎖的機制" << std::endl;
    std::cout << "無競爭時：純用戶空間原子操作，無系統呼叫" << std::endl;
    std::cout << "有競爭時：使用系統呼叫讓執行緒睡眠" << std::endl;
    
    return 0;
}
