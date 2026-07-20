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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 190. Reverse Bits（反轉位元）
// 題目：反轉一個 32-bit unsigned integer 的位元順序；指定範例結果為 964176192。
// 為何使用本章主題：reverse_bits 是純位元演算法，example 再以明定 big-endian encode/decode
//       驗證結果可跨主機保存；binary fstream/wire encoding 並非原題必要步驟。
// 思路：1. result 從 0；2. 重複 32 次左移 result；3. 接上 value 最低位；4. value 右移。
// 複雜度：固定 32 回合，時間與額外空間 O(1)；四 byte 編解碼同為 O(1)。
// 易錯點：必須用 unsigned 位移；binary mode 不會自動處理 endian，格式仍需自行明定。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】固定寬度 uint32 截斷偵測
// 情境：wire format 要求恰好四個 big-endian bytes；輸入只有兩 bytes 時必須報錯，不能以零補滿。
// 為何使用本章主題：istream::read 做 unformatted fixed-size 讀取，gcount 回實際數量；相較 formatted
//       extraction，不受 whitespace/locale 影響並能保留任意 byte。
// 設計：1. 準備四格 buffer；2. read 四 bytes；3. gcount 不等於 4 就丟 truncated；4. 完整才 decode。
// 成本：固定欄位時間與空間 O(1)；一般 K-byte 欄位為 O(K)。
// 上線注意：還要分辨 badbit 與 EOF、記錄 offset/context；外部 length 在配置前需上限與 overflow 檢查。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：binary I/O 的完成邊界】
 * - open/read/write 必須先執行再檢查，不能藏在 assert；release build 仍需保留 I/O 與錯誤處理。
 * - gcount 只能告知本次抽取字元數，還要分辨正常 EOF、truncated record 與 badbit。
 * - destructor close 無法把錯誤可靠回傳給 caller；重要輸出應明確 flush/close 並檢查 stream state。
 * - size_t 轉 streamsize 前要驗範圍，wire format 也需明定 endian，不能直接寫 native struct。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_fstream_binary.cpp' -o '/tmp/codex_cpp_C_IOStream_06_fstream_binary' && '/tmp/codex_cpp_C_IOStream_06_fstream_binary'
//
// === 預期輸出（節錄）===
// [實務] truncated binary record rejected
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
