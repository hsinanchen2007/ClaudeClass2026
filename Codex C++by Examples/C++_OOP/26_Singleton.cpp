// ============================================================================
// 課題 26：Singleton pattern（單例模式）與其代價
// ============================================================================
//
// Singleton 保證 process 中只有一個可存取 instance。Modern C++ 常用 function-local
// static（Meyers singleton）；C++11 起 initialization thread-safe。刪除 copy/move 避免
// 產生第二份。
//
// 但 Singleton 本質是 hidden global dependency：測試難隔離、呼叫順序耦合、mutable
// state 需同步，且跨 process 根本不是同一個 instance。優先 dependency injection；
// 只有真正 process-wide 且生命週期等同程式的 registry/telemetry 才審慎使用。
//
// 【面試】初始化 thread-safe 不代表每個 member operation thread-safe。
// 【陷阱】double-checked locking 很容易寫錯；不要手刻，使用 local static/call_once。
// ============================================================================

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <utility>

class Settings {
public:
    static Settings& instance()
    {
        static Settings settings;
        return settings;
    }

    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    Settings(Settings&&) = delete;
    Settings& operator=(Settings&&) = delete;

    void set(std::string key, std::string value)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[std::move(key)] = std::move(value);
    }
    std::string get(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.at(key);
    }

private:
    Settings() = default;
    mutable std::mutex mutex_;
    std::map<std::string, std::string> values_;
};

void basic_example()
{
    Settings::instance().set("theme", "dark");
    Settings& same = Settings::instance();
    assert(&same == &Settings::instance());
    assert(same.get("theme") == "dark");
    std::cout << "[基礎] 所有呼叫取得同一 Settings instance\n";
}

// LeetCode 535：Encode and Decode TinyURL。
// 題目 Codec instances 共享後端 registry；Singleton 模擬 process 內唯一資料庫。
// 真實分散式服務不能靠 process singleton，需外部 durable database 與 collision policy。
class UrlRegistry {
public:
    static UrlRegistry& instance()
    {
        static UrlRegistry registry;
        return registry;
    }
    UrlRegistry(const UrlRegistry&) = delete;
    UrlRegistry& operator=(const UrlRegistry&) = delete;

    std::string store(const std::string& url)
    {
        const std::string key = std::to_string(next_++);
        urls_[key] = url;
        return key;
    }
    std::string load(const std::string& key) const { return urls_.at(key); }

private:
    UrlRegistry() = default;
    unsigned long next_ = 1UL;
    std::map<std::string, std::string> urls_;
};

class Codec {
public:
    std::string encode(const std::string& long_url)
    {
        return "https://tiny/" + UrlRegistry::instance().store(long_url);
    }
    std::string decode(const std::string& short_url) const
    {
        return UrlRegistry::instance().load(short_url.substr(short_url.find_last_of('/') + 1U));
    }
};

void leetcode_535_example()
{
    Codec encoder;
    Codec decoder;
    const std::string short_url = encoder.encode("https://leetcode.com/problems/design-tinyurl");
    assert(decoder.decode(short_url) == "https://leetcode.com/problems/design-tinyurl");
    std::cout << "[LeetCode 535] shared singleton registry decode 成功\n";
}

// 實務案例：telemetry process counter。這類需求才較接近合理 Singleton。
class Telemetry {
public:
    static Telemetry& instance()
    {
        static Telemetry telemetry;
        return telemetry;
    }
    void increment_jobs() { ++jobs_; }
    int jobs() const { return jobs_; }
private:
    Telemetry() = default;
    int jobs_ = 0; // 示範為單執行緒；production 多執行緒需 atomic/mutex。
};

void practical_example()
{
    Telemetry::instance().increment_jobs();
    Telemetry::instance().increment_jobs();
    assert(Telemetry::instance().jobs() == 2);
    std::cout << "[實務] process telemetry jobs=2\n";
}

int main()
{
    basic_example();
    leetcode_535_example();
    practical_example();
}

// 練習：把 Settings 改成普通 object，由 constructor injection 傳給兩個 services。
// 複雜度：function-local static 首次初始化後 access O(1)；hidden global contention 仍可能昂貴。
// 生命週期：singleton 通常 static lifetime，解構順序跨 translation units 是設計風險。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '26_Singleton.cpp' -o '/tmp/codex_cpp_C_OOP_26_Singleton' && '/tmp/codex_cpp_C_OOP_26_Singleton'
//
// === 預期輸出（節錄）===
// [實務] process telemetry jobs=2
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
