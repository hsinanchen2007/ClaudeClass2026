// ============================================================================
// 課題 11：static data member 與 static member function
// ============================================================================
//
// 一般 member 每個 object 各有一份；static data member 屬於 class，全體 object 共用。
// static member function 沒有 this，只能直接存取 static members。C++17 的
// `inline static` 可在 class 內定義並避免另外提供一個 translation-unit definition。
//
// static 適合 class-wide counter、factory、常數；但可變 global-like state 會造成測試
// 互相污染、初始化順序與多執行緒同步問題。跨 thread 修改 counter 應用 atomic/mutex。
//
// 【面試】function-local static 自 C++11 起初始化是 thread-safe，但後續修改不自動安全。
// 【陷阱】static object 的解構順序跨 translation units 不明確，稱 static destruction
// order fiasco；避免在 shutdown 時讓 global objects 互相依賴。
// ============================================================================

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>

class Ticket {
public:
    Ticket() : id_(next_id_++) { ++live_count_; }
    ~Ticket() noexcept { --live_count_; }
    Ticket(const Ticket&) = delete;
    Ticket& operator=(const Ticket&) = delete;

    int id() const { return id_; }
    static int live_count() { return live_count_; }

private:
    inline static int next_id_ = 1;
    inline static int live_count_ = 0;
    int id_;
};

void basic_example()
{
    assert(Ticket::live_count() == 0);
    {
        Ticket first;
        Ticket second;
        assert(first.id() == 1 && second.id() == 2);
        assert(Ticket::live_count() == 2);
    }
    assert(Ticket::live_count() == 0);
    std::cout << "[基礎] static counter 由所有 Ticket 共用\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 535. Encode and Decode TinyURL（TinyURL 的加密與解密）
// 題目：encode 將長網址轉短網址，decode 必須還原；例如某題目 URL 編碼後可由另一 Codec 解回。
// 為何使用本章主題：inline static key/map 讓不同 Codec instances 共用 process 內 registry；這只是題目服務的教學模型。
// 思路：以遞增 key 建短碼；保存 key->long URL；decode 取最後 path segment；由 static map 查回。
// 複雜度：encode/decode 平均 O(L) 字串成本與平均 O(1) hash 查找，空間 O(U)，U 為已編碼 URL 總長。
// 易錯點：static state 非執行緒安全且不持久；短網址格式與缺 key 要驗證；真實系統需碰撞、配額與資料庫策略。
// -----------------------------------------------------------------------------
class Codec {
public:
    std::string encode(const std::string& long_url)
    {
        const std::string key = std::to_string(next_key_++);
        urls_[key] = long_url;
        return "https://tiny/" + key;
    }

    static std::string decode(const std::string& short_url)
    {
        const std::size_t slash = short_url.find_last_of('/');
        return urls_.at(short_url.substr(slash + 1U));
    }

private:
    inline static unsigned long next_key_ = 1UL;
    inline static std::unordered_map<std::string, std::string> urls_;
};

void leetcode_535_example()
{
    Codec encoder;
    Codec another_instance;
    const std::string short_url = encoder.encode("https://leetcode.com/problems/design-tinyurl");
    assert(another_instance.decode(short_url) ==
           "https://leetcode.com/problems/design-tinyurl");
    std::cout << "[LeetCode 535] " << short_url << " 可由另一 instance decode\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】互動式與批次重試策略的命名工廠
// 情境：互動請求使用 3 次/100ms，批次工作使用 10 次/1000ms，呼叫端應以語意名稱選設定。
// 為何使用本章主題：static factory 在物件建立前即可呼叫且不需要 this，比暴露兩個裸 int 的 constructor 更不易傳反。
// 設計：interactive/batch 各呼叫 private constructor；物件保存 attempts/delay；提供唯讀 observers。
// 成本：建立與查詢皆 O(1)，無動態配置。
// 上線注意：delay 單位要進入型別或名稱；參數需依服務 SLA 設定，若可熱更新則不宜硬編碼 static factory。
// -----------------------------------------------------------------------------
class RetryPolicy {
public:
    static RetryPolicy interactive() { return RetryPolicy(3, 100); }
    static RetryPolicy batch() { return RetryPolicy(10, 1'000); }
    int attempts() const { return attempts_; }
    int delay_ms() const { return delay_ms_; }

private:
    RetryPolicy(int attempts, int delay_ms) : attempts_(attempts), delay_ms_(delay_ms) {}
    int attempts_;
    int delay_ms_;
};

void practical_example()
{
    const RetryPolicy policy = RetryPolicy::batch();
    assert(policy.attempts() == 10 && policy.delay_ms() == 1'000);
    std::cout << "[實務] static named factory 建立 batch retry policy\n";
}

int main()
{
    basic_example();
    leetcode_535_example();
    practical_example();
}

// 練習：把 next_id_ 改成 std::atomic<int>，思考 relaxed ordering 是否足夠。
// 複雜度：static counter increment 通常 O(1)；atomic 版仍 O(1) API 但有同步常數成本。
// 生命週期：inline static member 具 static storage duration，所有 instances 共用且活到程序結束。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_StaticMember.cpp' -o '/tmp/codex_cpp_C_OOP_11_StaticMember' && '/tmp/codex_cpp_C_OOP_11_StaticMember'
//
// === 預期輸出（節錄）===
// [實務] static named factory 建立 batch retry policy
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
