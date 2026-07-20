// ============================================================================
// 課題 12：實務 bit flags - enum class 與型別安全操作
// ============================================================================
//
// 多個獨立 bool 可壓入一個 unsigned mask，適合 protocol 欄位、permissions、feature flags。
// `enum class` 不會隱式轉 int、不同 flags 型別不會誤混，但需自行提供 `operator|` 與
// has/set/clear。每個 enumerator 必須是單一 bit；組合值才可同時含多 bits。
//
// Flags 適合「彼此可獨立組合」；互斥狀態（idle/running/stopped）應用普通 enum/variant，
// 否則可能同時出現 running+stopped 這種非法狀態。
// ============================================================================

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

enum class Capability : std::uint8_t {
    none = 0U,
    read = 1U << 0U,
    write = 1U << 1U,
    execute = 1U << 2U,
    audit = 1U << 3U
};

constexpr Capability operator|(Capability left, Capability right)
{
    return static_cast<Capability>(static_cast<std::uint8_t>(left) |
                                   static_cast<std::uint8_t>(right));
}

constexpr Capability operator&(Capability left, Capability right)
{
    return static_cast<Capability>(static_cast<std::uint8_t>(left) &
                                   static_cast<std::uint8_t>(right));
}

constexpr bool has(Capability flags, Capability requested)
{
    return (flags & requested) == requested;
}

constexpr Capability clear(Capability flags, Capability removed)
{
    return static_cast<Capability>(static_cast<std::uint8_t>(flags) &
        static_cast<std::uint8_t>(~static_cast<std::uint8_t>(removed)));
}

void basic_example()
{
    Capability flags = Capability::read | Capability::write;
    assert(has(flags, Capability::read));
    assert(!has(flags, Capability::execute));
    flags = flags | Capability::audit;
    flags = clear(flags, Capability::write);
    assert(has(flags, Capability::audit) && !has(flags, Capability::write));
    std::cout << "[基礎] enum class flags 保留 read+audit\n";
}

// LeetCode 401：Binary Watch。
// 10 個 LEDs 可視為 bitmask：高 4 bits 表 hour、低 6 bits 表 minute；set bits 總數等於
// turnedOn。這裡直接枚舉合法時間，避免生成非法 hour/minute。
std::vector<std::string> read_binary_watch(int turned_on)
{
    std::vector<std::string> result;
    if (turned_on < 0 || turned_on > 10) return result;
    for (unsigned hour = 0U; hour < 12U; ++hour) {
        for (unsigned minute = 0U; minute < 60U; ++minute) {
            const unsigned bits = static_cast<unsigned>(std::popcount(hour)) +
                                  static_cast<unsigned>(std::popcount(minute));
            if (bits == static_cast<unsigned>(turned_on)) {
                result.push_back(std::to_string(hour) + ":" +
                    (minute < 10U ? "0" : "") + std::to_string(minute));
            }
        }
    }
    return result;
}

void leetcode_401_example()
{
    const auto times = read_binary_watch(1);
    assert(times.size() == 10U);
    assert(times.front() == "0:01");
    std::cout << "[LeetCode 401] one LED yields 10 valid times\n";
}

// 實務：API authorization 一次要求 read+audit；has 比逐一 bool 判斷更集中。
bool may_read_audit_log(Capability capabilities)
{
    return has(capabilities, Capability::read | Capability::audit);
}

void practical_example()
{
    assert(may_read_audit_log(Capability::read | Capability::audit));
    assert(!may_read_audit_log(Capability::read));
    std::cout << "[實務] audit log 需同時具 read 與 audit\n";
}

int main()
{
    basic_example();
    leetcode_401_example();
    practical_example();
}

// 易錯與面試：旗標值應各占一個 bit，不能讓兩個 enum 常數重疊；反序列化外部 mask 時
// 也要拒絕未知 bits，否則新舊版本可能對同一資料產生不同安全語意。

// 練習：不用枚舉 hour/minute，改枚舉 10-bit mask 後過濾非法時間。
// 複雜度與生命週期：單次 flags 組合/查詢是 O(1)；enum value 自行保存 bits，無 owner/borrow。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '12_practical_flags.cpp' -o '/tmp/codex_cpp_C_Bit_12_practical_flags' && '/tmp/codex_cpp_C_Bit_12_practical_flags'
//
// === 預期輸出（節錄）===
// [實務] audit log 需同時具 read 與 audit
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
