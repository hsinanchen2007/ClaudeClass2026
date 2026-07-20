// ============================================================================
// ThreadSanitizer：動態找 data race，不是正確性的形式證明
// ============================================================================
// 建議命令：
//   clang++ -std=c++20 -O1 -g -fsanitize=thread -fno-omit-frame-pointer -pthread 14_thread_sanitizer.cpp -o /tmp/tsan_demo && /tmp/tsan_demo
// TSan 會 instrument memory access 並回報未同步衝突、thread stack 與 lock 關係；成本
// 可達數倍 CPU/大量記憶體，只適合測試。它只能看到本次執行走過的路徑；報告 0 個
// race 不等於證明所有 interleaving 正確，也不抓一般 deadlock/高階邏輯 race。
//
// 故意帶 race 的程式只應在隔離測試用 macro 建立；本教材預設路徑全部 race-free。

#include <cassert>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

void basic_demo()
{
    int counter = 0;
    std::mutex mutex;
    auto increment = [&] {
        for (int i = 0; i < 1'000; ++i) {
            const std::lock_guard<std::mutex> lock(mutex);
            ++counter;
        }
    };
    std::thread a(increment);
    std::thread b(increment);
    a.join();
    b.join();
    assert(counter == 2'000);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（存在重複元素）
// 題目：若陣列任兩個不同 index 的值相同就回 true；例如 [1,2,3,1] 為 true，[1,2,3,4] 為 false。
// 為何使用本章主題：兩段平行掃描共享 unordered_set，所有 insert 與 duplicate flag 都由同一 mutex 保護，形成可由 TSan 驗證的 race-free 路徑。
// 思路：1. 切成兩段。2. 每個值在鎖內插入 set。3. insert 失敗就設 duplicate。4. join 後回傳旗標。
// 複雜度：平均總時間 O(N)、空間 O(N)，但每個元素都競爭同一鎖，最壞 hash 行為可退化。
// 易錯點：unordered_set 即使不同 bucket 也可能 rehash 全域結構，不能無鎖 insert；TSan 無報告也只涵蓋本次執行路徑。
// -----------------------------------------------------------------------------
bool contains_duplicate(const std::vector<int>& values)
{
    std::unordered_set<int> seen;
    std::mutex mutex;
    bool duplicate = false;
    const std::size_t middle = values.size() / 2U;
    auto scan = [&](std::size_t first, std::size_t last) {
        for (std::size_t index = first; index < last; ++index) {
            const std::lock_guard<std::mutex> lock(mutex);
            if (!seen.insert(values.at(index)).second) {
                duplicate = true;
            }
        }
    };
    std::thread left(scan, 0U, middle);
    std::thread right(scan, middle, values.size());
    left.join();
    right.join();
    return duplicate;
}

void leetcode_demo()
{
    assert(contains_duplicate({1, 2, 3, 1}));
    assert(!contains_duplicate({1, 2, 3, 4}));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】Thread-local 批次結果的主執行緒合併
// 情境：兩個 worker 分別產生 [1,2] 與 [3,4]，不能同時向同一 vector push_back，最後需得到穩定串接結果。
// 為何使用本章主題：每個 worker 唯一擁有自己的 vector，join 後才由 main transfer/merge，比共享容器每次加鎖簡單且容易通過 TSan。
// 設計：1. 準備兩個獨立輸出。2. worker 各自填值。3. join 等待 ownership 回到 main。4. main 單執行緒 insert 合併。
// 成本：產生與合併總時間 O(N)，額外空間 O(N)，join 後 insert 可能配置與複製元素。
// 上線注意：worker 例外不可逃出 thread entry；大型結果可預先 reserve 或 move，且所有 local buffers 必須活到 join。
// -----------------------------------------------------------------------------
void practical_demo()
{
    std::vector<int> left;
    std::vector<int> right;
    std::thread a([&] { left = {1, 2}; });
    std::thread b([&] { right = {3, 4}; });
    a.join();
    b.join();
    left.insert(left.end(), right.begin(), right.end());  // join 後由主 thread 合併。
    assert((left == std::vector<int>{1, 2, 3, 4}));
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "TSan 教材：預設 race-free 範例測試通過\n";
}

// 【陷阱】TSan 通常不能與 ASan 同一 binary 同時啟用；分開建置測試。
// 【陷阱】自訂 assembly/synchronization 若 TSan 不理解，可能 false positive/negative。
// 【面試】TSan 與 race detector 的限制：動態 coverage、排程概率、額外成本。
// 【練習】在 /tmp 寫一個未鎖 counter，確認 TSan 報告後刪除，不污染教材原始碼。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '14_thread_sanitizer.cpp' -o '/tmp/codex_cpp_C_MultiThread_14_thread_sanitizer' && '/tmp/codex_cpp_C_MultiThread_14_thread_sanitizer'
//
// === 預期輸出（節錄）===
// TSan 教材：預設 race-free 範例測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
