// ============================================================================
// 課題 1：<chrono> 全覽 - duration、time_point、clock
// ============================================================================
//
// chrono 用型別區分三件事：
//   duration<Rep,Period>：時間長度，例如 250ms；可加減/比較。
//   time_point<Clock,Duration>：某個 clock domain 上的一個時間點。
//   Clock：提供 now() 與 epoch/單調性語意。
//
// `deadline = now + timeout` 合法；`elapsed = end - start` 得 duration。不同 clocks 的
// time_points 不可直接相減，因 epoch/跳動語意不同。量 elapsed/deadline 用 steady_clock；
// 顯示/持久化 wall time 用 system_clock。
//
// chrono 的 type safety 可防「毫秒誤當秒」，但 `.count()` 會丟掉單位資訊，API boundary
// 應盡量直接接 duration，而非裸 integer。
// ============================================================================

#include <cassert>
#include <chrono>
#include <deque>
#include <iostream>

using namespace std::chrono_literals;

void basic_example()
{
    const auto timeout = 2s + 500ms;
    assert(timeout == 2'500ms);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    assert(seconds.count() == 2); // duration_cast 對整數 duration 截斷，不是四捨五入。

    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + timeout;
    assert(deadline - start == timeout);
    std::cout << "[基礎] timeout=2500ms，cast seconds=2\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 933. Number of Recent Calls（最近的請求次數）
// 題目：依序輸入毫秒時間 t，回傳閉區間 [t-3000, t] 內的 ping 數；例如 3002ms 時移除 1ms 的紀錄後回傳 3。
// 為何使用本章主題：以 chrono::milliseconds 表達題目的毫秒時間與 3000ms 視窗，讓相減與單位在型別中可見。
// 思路：1. 將本次時間推入 deque。2. 計算 now-3000ms。3. 從前端移除早於下界的紀錄。4. 回傳剩餘筆數。
// 複雜度：每筆紀錄至多進出 deque 各一次，單次攤銷時間 O(1)、額外空間 O(W)，W 為目前視窗內的 ping 數。
// 易錯點：視窗下界是 inclusive，恰等於 t-3000 的紀錄不可刪；題目也保證 t 嚴格遞增。
// -----------------------------------------------------------------------------
class RecentCounter {
public:
    int ping(std::chrono::milliseconds now)
    {
        calls_.push_back(now);
        const auto earliest = now - 3'000ms;
        while (!calls_.empty() && calls_.front() < earliest) calls_.pop_front();
        return static_cast<int>(calls_.size());
    }
private:
    std::deque<std::chrono::milliseconds> calls_;
};

void leetcode_933_example()
{
    RecentCounter counter;
    const int at_1 = counter.ping(1ms);
    const int at_100 = counter.ping(100ms);
    const int at_3_001 = counter.ping(3'001ms);
    const int at_3_002 = counter.ping(3'002ms);
    assert(at_1 == 1);
    assert(at_100 == 2);
    assert(at_3_001 == 3);
    assert(at_3_002 == 3);
    std::cout << "[LeetCode 933] chrono window retains 3 recent calls\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP client timeout 的型別安全設定
// 情境：呼叫端要把請求逾時設為 5 秒，設定物件內部統一保存毫秒，避免把裸數字 5000 誤讀成秒。
// 為何使用本章主題：duration 讓 5s 在傳入時自動且安全地轉成 milliseconds，比整數加文件註記更能防止單位混用。
// 設計：1. set_timeout 直接接 milliseconds。2. 拒絕非正 duration。3. 按值保存。4. getter 仍回傳帶單位的 duration。
// 成本：設定與讀取皆為 O(1) 值拷貝，沒有計時器、同步或 I/O 成本。
// 上線注意：正式邊界不能只靠 assert 驗證；還要限制最大 timeout，並在序列化欄位名稱中保留 `_ms` 單位。
// -----------------------------------------------------------------------------
class ClientOptions {
public:
    void set_timeout(std::chrono::milliseconds timeout)
    {
        assert(timeout > 0ms);
        timeout_ = timeout;
    }
    std::chrono::milliseconds timeout() const { return timeout_; }
private:
    std::chrono::milliseconds timeout_{1s};
};

void practical_example()
{
    ClientOptions options;
    options.set_timeout(5s);
    assert(options.timeout() == 5'000ms);
    std::cout << "[實務] typed timeout=5000ms\n";
}

int main()
{
    basic_example();
    leetcode_933_example();
    practical_example();
}

// 練習：讓 set_timeout 接 template duration，內部統一轉 milliseconds 並檢查 overflow。
// 複雜度與生命週期：duration/time_point arithmetic 是 O(1) 值運算；ClientOptions 按值保存
// timeout，所以呼叫端的 temporary duration 結束後設定仍有效。

/*
【本課面試問答】
Q1：`duration<Rep,Period>` 的兩個模板參數代表什麼？
A：Rep 是 tick 計數型別，Period 是每 tick 相對於秒的 compile-time ratio；例如 milliseconds 是
`duration<...,milli>`。運算會用 common duration，避免把裸 int 的單位交給人腦猜。

Q2：為何 seconds 轉 milliseconds 可隱式，反向常要 `duration_cast`？
A：轉成更細單位通常不遺失 tick；轉成較粗單位會截斷，因此要求顯式表達。需要特定方向的 rounding
時用 floor/ceil/round，而不是誤以為 cast 會四捨五入。

Q3：`count()` 回傳值能否離開單位直接儲存？
A：能編譯但容易丟失語意。跨 API 優先傳 duration；只有序列化/顯示邊界才取 count，並把單位寫進
欄位名稱與協定，例如 `timeout_ms`，同時檢查目標整數範圍。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_overview.cpp' -o '/tmp/codex_cpp_C_Chrono_01_overview' && '/tmp/codex_cpp_C_Chrono_01_overview'
//
// === 預期輸出（節錄）===
// [實務] typed timeout=5000ms
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
