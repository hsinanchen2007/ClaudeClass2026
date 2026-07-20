// ============================================================================
// 一次性初始化：call_once、once_flag 與 function-local static
// ============================================================================
// C++11 起，function-local static 的初始化是 thread-safe：只有一個 thread 執行，其他
// 等待；若初始化丟 exception，下次呼叫會重試。std::call_once(flag, f) 適合初始化
// 既有物件或需帶參數流程；只有 f 正常返回才算完成，exception 同樣允許後續重試。
// once_flag 不可 copy/reset，表示 process lifetime 的一次事件。

#include <array>
#include <cassert>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

void basic_demo()
{
    std::once_flag flag;
    int initialized = 0;
    auto initialize = [&] {
        std::call_once(flag, [&] { ++initialized; });
    };
    std::thread a(initialize);
    std::thread b(initialize);
    a.join();
    b.join();
    assert(initialized == 1);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 70. Climbing Stairs（爬樓梯）
// 題目：每次走 1 或 2 階，求抵達 n 階的方法數；例如 n=5 得 8、n=10 得 89。
// 為何使用本章主題：固定 0..45 的 DP table 以 function-local static lazy 建立，首次併發呼叫只初始化一次；這是多次查詢的教學改寫。
// 思路：1. 首次呼叫建立 46 格 table。2. 設 ways[0]=ways[1]=1。3. 依 Fibonacci recurrence 填完。4. 後續 O(1) 查表。
// 複雜度：一次初始化時間/空間 O(K)，每次查詢 O(1)，K=46；同步只發生在尚未完成的首次初始化。
// 易錯點：steps 範圍不能只靠 release 會移除的 assert；初始化若丟例外會在下次呼叫重試，int 上限也依題目範圍而定。
// -----------------------------------------------------------------------------
int climb_stairs(int steps)
{
    assert(steps >= 0 && steps <= 45);
    static const std::array<int, 46> ways = [] {
        std::array<int, 46> table{};
        table[0] = 1;
        table[1] = 1;
        for (std::size_t index = 2; index < table.size(); ++index) {
            table[index] = table[index - 1U] + table[index - 2U];
        }
        return table;
    }();  // 第一次併發呼叫也只初始化一次。
    return ways[static_cast<std::size_t>(steps)];
}

void leetcode_demo()
{
    int first = 0;
    int second = 0;
    std::thread a([&] { first = climb_stairs(5); });
    std::thread b([&] { second = climb_stairs(10); });
    a.join();
    b.join();
    assert(first == 8 && second == 89);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】支援格式表的執行緒安全 Lazy 初始化
// 情境：多個 request thread 首次查詢服務支援的 json/yaml/toml 格式，所有人應取得同一份 process-lifetime immutable table。
// 為何使用本章主題：function-local static 由語言保證只成功初始化一次，比 double-checked locking 或手動 global flag 更安全。
// 設計：1. 首次進函式建 const vector。2. 其他同時 caller 等待完成。3. 回傳 const reference。4. 後續直接重用同一物件。
// 成本：首次配置 O(F)，後續呼叫/空間分別 O(1)/O(F)，F 為格式數；實作可能保留輕量 guard 檢查。
// 上線注意：回傳 reference 依賴 static lifetime 且內容必須保持 immutable；測試替換、動態 reload 或 shutdown ordering 需另設計。
// -----------------------------------------------------------------------------
const std::vector<std::string>& supported_formats()
{
    static const std::vector<std::string> formats{"json", "yaml", "toml"};
    return formats;
}

void practical_demo()
{
    const std::vector<std::string>* first = nullptr;
    const std::vector<std::string>* second = nullptr;
    std::thread a([&] { first = &supported_formats(); });
    std::thread b([&] { second = &supported_formats(); });
    a.join();
    b.join();
    assert(first == second);
    assert(first->at(0) == "json");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "init once：call_once 與 local static 測試通過\n";
}

// 【陷阱】初始化後物件若仍可被多 thread 修改，thread-safe initialization 不等於使用安全。
// 【陷阱】call_once callable 若遞迴呼叫同一 once_flag，可能 deadlock。
// 【面試】Meyers singleton 的初始化安全，但 global lifetime、測試替換與依賴仍是設計問題。
// 【練習】讓 call_once 首次故意丟 exception，驗證第二次會重試且成功。

/*
 * 【教科書補充：call_once 的重試與生命週期】
 * - 若 active call 拋例外，once_flag 不會標成完成；後續呼叫可再嘗試，成功者才發布初始化結果。
 * - 對同一 flag 的 active calls 有標準定義的順序，成功返回會同步到所有 passive callers。
 * - 同一初始化函式遞迴呼叫相同 once_flag 可能自我死鎖；flag 與被保護資料須活過所有 caller。
 * - LeetCode steps 是 runtime input，assert 在 release 消失；正式邊界要明確限制可索引範圍。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '15_init_once.cpp' -o '/tmp/codex_cpp_C_MultiThread_15_init_once' && '/tmp/codex_cpp_C_MultiThread_15_init_once'
//
// === 預期輸出（節錄）===
// init once：call_once 與 local static 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
