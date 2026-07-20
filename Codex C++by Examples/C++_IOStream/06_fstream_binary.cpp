// ============================================================================
// 課題 6：Binary fstream - 明定格式，不直接 dump struct
// ============================================================================
//
// ios::binary 關閉 text newline translation，但不自動處理 endian、padding、version、型別
// 寬度。直接 `write(reinterpret_cast<char*>(&struct),sizeof struct)` 會把 padding/host endian/
// ABI 寫進檔案，跨 compiler/architecture 不可靠。逐欄定義 fixed-width byte encoding。
//
// read/write 的 size 是 streamsize；大 buffer 轉換前檢查。讀固定 N bytes 後驗 gcount/state，
// truncated input 不可繼續使用半讀資料。格式應含 magic/version/length/checksum 與上限。
// ============================================================================

#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

class TempFile {
public:
    TempFile() : path_(fs::temp_directory_path() /
        ("codex_binary_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {}
    ~TempFile() { std::error_code e; fs::remove(path_, e); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

std::array<char, 4> encode_u32_be(std::uint32_t value)
{
    return {static_cast<char>((value >> 24U) & 0xFFU), static_cast<char>((value >> 16U) & 0xFFU),
            static_cast<char>((value >> 8U) & 0xFFU), static_cast<char>(value & 0xFFU)};
}

std::uint32_t decode_u32_be(const std::array<char, 4>& bytes)
{
    const auto byte = [](char value) { return static_cast<std::uint32_t>(static_cast<unsigned char>(value)); };
    return (byte(bytes[0]) << 24U) | (byte(bytes[1]) << 16U) |
           (byte(bytes[2]) << 8U) | byte(bytes[3]);
}

void basic_example()
{
    TempFile file;
    const auto encoded = encode_u32_be(0x12345678U);
    { std::ofstream output(file.path(), std::ios::binary); output.write(encoded.data(), 4); assert(output); }
    std::array<char, 4> input_bytes{};
    { std::ifstream input(file.path(), std::ios::binary); input.read(input_bytes.data(), 4); assert(input.gcount() == 4); }
    assert(decode_u32_be(input_bytes) == 0x12345678U);
    std::cout << "[基礎] explicit big-endian u32 round-trip\n";
}

// LeetCode 190：Reverse Bits，並將結果以明定 big-endian bytes 落檔。
std::uint32_t reverse_bits(std::uint32_t value)
{
    std::uint32_t result = 0U;
    for (unsigned bit = 0U; bit < 32U; ++bit) { result = (result << 1U) | (value & 1U); value >>= 1U; }
    return result;
}

void leetcode_190_example()
{
    const std::uint32_t result = reverse_bits(0b00000010100101000001111010011100U);
    assert(result == 964'176'192U);
    assert(decode_u32_be(encode_u32_be(result)) == result);
    std::cout << "[LeetCode 190] reversed value survives binary encoding\n";
}

// 實務：truncated read 必須 fail，不補零冒充合法 record。
std::uint32_t read_u32_be(std::istream& input)
{
    std::array<char, 4> bytes{};
    input.read(bytes.data(), 4);
    if (input.gcount() != 4) throw std::runtime_error("truncated u32");
    return decode_u32_be(bytes);
}

void practical_example()
{
    std::istringstream truncated(std::string("\x01\x02", 2));
    bool rejected = false;
    try { (void)read_u32_be(truncated); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    std::cout << "[實務] truncated binary record rejected\n";
}

int main()
{
    basic_example();
    leetcode_190_example();
    practical_example();
}

// 練習：加入 4-byte magic、1-byte version 與 payload length 上限。
// 複雜度：encode/decode O(payload bytes)；固定寬 header 是 O(1)，但仍要防惡意 length 配置。
// 生命週期：buffer 按值擁有 bytes；不要把 reinterpret pointer 保存到 vector reallocation 之後。

/*
【本課面試問答】
Q1：以 `ios::binary` 開檔，就能直接 `write(reinterpret_cast<char*>(&obj), sizeof obj)` 嗎？
A：不能據此得到可攜序列化。binary mode 主要關閉文字換行等轉換；object representation 仍可能含
padding、endianness、不同型別寬度、pointer/vptr，讀回非 trivially-copyable 型別更不合法。協定應逐欄
定義寬度、byte order、版本與長度上限。

Q2：`read(buf,n)` 後怎麼處理最後不足 n bytes 的區塊？
A：`read` 會設 fail/eof，但 `gcount()` 仍告訴本次實際讀到多少。串流複製可處理這些 bytes；固定長度
record 則必須把不足視為 truncated error，不能用補零後的 buffer 冒充完整資料。

Q3：binary parser 最重要的安全檢查是什麼？
A：先驗 magic/version，再對外部 length 做上限與算術 overflow 檢查，確認剩餘 bytes 足夠後才配置。
錯誤要攜帶 offset/context；不要信任檔案內的 size 直接配置數 GB 記憶體。
*/
