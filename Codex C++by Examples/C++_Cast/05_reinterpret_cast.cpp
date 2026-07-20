// ============================================================================
// 課題 5：reinterpret_cast - 低階 representation boundary
// ============================================================================
//
// reinterpret_cast 可在 unrelated pointer types、pointer/integer（足夠寬）間轉換。它通常
// 只改 compiler 對 bits 的「解讀方式」，不保證 target alignment、object lifetime、
// strict aliasing 或有效值；cast 成功不代表 dereference 合法。
//
// 合理用途集中在 OS/driver/C callback/serialization 邊界。查看 object bytes 優先
// `std::as_bytes(span)` 或 memcpy/bit_cast；type punning 優先 std::bit_cast。pointer→
// integer 用 uintptr_t（若平台提供），只適合 logging/hash/ABI round-trip，不做算術後
// 再任意 dereference。
// ============================================================================

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

void basic_example()
{
    int value = 42;
    int* pointer = &value;
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
    int* round_trip = reinterpret_cast<int*>(address);
    assert(round_trip == pointer && *round_trip == 42);

    const std::span<const int> values(&value, 1U);
    const std::span<const std::byte> bytes = std::as_bytes(values); // 比 alias 成 char* 更具意圖。
    assert(bytes.size() == sizeof(int));
    std::cout << "[基礎] pointer round-trip and byte view size=" << bytes.size() << '\n';
}

// LeetCode 190：Reverse Bits。演算法只需 unsigned shifts，不需要 reinterpret_cast；
// 這是刻意的反例：一般 bit algorithm 不應因「位元」兩字就使用低階 cast。
std::uint32_t reverse_bits(std::uint32_t input)
{
    std::uint32_t output = 0U;
    for (unsigned bit = 0U; bit < 32U; ++bit) {
        output = (output << 1U) | (input & 1U);
        input >>= 1U;
    }
    return output;
}

void leetcode_190_example()
{
    assert(reverse_bits(0b00000010100101000001111010011100U) == 964'176'192U);
    std::cout << "[LeetCode 190] unsigned operations suffice; no reinterpret_cast\n";
}

// 實務：C callback 的 void* user_data。轉回的型別必須與註冊時完全相同，且 object
// 必須仍存活；void* 不帶 ownership/lifetime 資訊。
using Callback = void (*)(void*);

struct Counter {
    int calls = 0;
};

void count_callback(void* user_data)
{
    auto* counter = static_cast<Counter*>(user_data); // void* 還原 object pointer 用 static_cast。
    ++counter->calls;
}

void invoke(Callback callback, void* user_data)
{
    callback(user_data);
}

void practical_example()
{
    Counter counter;
    invoke(&count_callback, &counter);
    invoke(&count_callback, &counter);
    assert(counter.calls == 2);
    std::cout << "[實務] C callback user_data calls=2\n";
}

int main()
{
    basic_example();
    leetcode_190_example();
    practical_example();
}

// 易錯與面試：reinterpret_cast 成功編譯不代表解參考合法；alignment、object lifetime、
// strict aliasing 與 representation 都要各自成立。序列化應逐 byte 編碼，別 dump struct。
// 練習：說明為何把 `double*` reinterpret_cast 成 `int*` 後 dereference 可能同時違反
// alignment、strict aliasing 與 object representation 規則。
// 複雜度：reinterpret_cast 通常只是 O(1) 位址/型別解讀；危險發生在後續 dereference。
// 生命週期：新 pointer 不開始目標型別物件生命，也不延長原 storage 或 callback context 存活。
