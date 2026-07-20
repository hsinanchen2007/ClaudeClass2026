// ============================================================================
// 課題 8B：std::istreambuf_iterator / std::ostreambuf_iterator
// ============================================================================
//
// 這一對 iterator 直接讀寫 stream buffer 的「字元」，不做 formatted I/O：
//   * 不跳過空白、不把 "123" 轉成整數 123。
//   * 適合逐字複製文字、計算 byte/character、實作簡單 filter。
//   * istreambuf_iterator 是 single-pass input iterator；讀過便消耗來源。
//   * 預設建構的 istreambuf_iterator<char>{} 是 end sentinel。
//
// 與 istream_iterator<T> 的差別：前者拿原始 char；後者反覆呼叫 operator>>
// 解析 T，通常會受 skipws、locale 與格式錯誤影響。
//
// 易錯點：
//   1. 讀文字可用 string，但真正任意 binary data 應用 unsigned char/byte 並確認
//      平台與 stream mode；Windows 檔案要用 ios::binary 避免 newline translation。
//   2. input iterator 不可先 distance 再 copy；第一次走訪已把 stream 吃完。
// ============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

std::string read_all_characters(std::istream& input)
{
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void basic_example()
{
    std::istringstream input("A B\nC");
    const std::string text = read_all_characters(input);
    assert(text == "A B\nC"); // 空白與換行都保留。

    std::ostringstream output;
    std::copy(text.begin(), text.end(), std::ostreambuf_iterator<char>(output));
    assert(output.str() == text);
    std::cout << "[基礎] streambuf iterators preserve every character\n";
}

// LeetCode 387：First Unique Character in a String。
// 這裡刻意從 stream 讀入，示範 single-pass 統計；回傳字元 index，找不到回 -1。
int first_unique_index(std::istream& input)
{
    const std::string text = read_all_characters(input);
    std::array<int, 256> frequency{};
    for (const char raw : text) {
        const auto ch = static_cast<unsigned char>(raw);
        ++frequency[ch];
    }
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (frequency[static_cast<unsigned char>(text[index])] == 1) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void leetcode_387_example()
{
    std::istringstream first("leetcode");
    std::istringstream second("aabb");
    assert(first_unique_index(first) == 0);
    assert(first_unique_index(second) == -1);
    std::cout << "[LeetCode 387] first unique character index = 0\n";
}

// 實務：把 CRLF log 正規化成 LF。真實 ETL 常先逐字保留，再做明確轉換，
// 而不是依賴 formatted extraction 把空白吃掉。
std::string normalize_crlf(std::istream& input)
{
    const std::string raw = read_all_characters(input);
    std::string normalized;
    normalized.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (raw[i] == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n') continue;
        normalized.push_back(raw[i]);
    }
    return normalized;
}

void practical_example()
{
    std::istringstream log("INFO ok\r\nWARN retry\r\n");
    assert(normalize_crlf(log) == "INFO ok\nWARN retry\n");
    std::cout << "[實務] CRLF log normalized without losing spaces\n";
}

int main()
{
    basic_example();
    leetcode_387_example();
    practical_example();
}

// 面試自問：istreambuf_iterator<char> 與 istream_iterator<char> 為何讀空白不同？
// 練習：加入計算行數、最長一行長度的 single-pass log analyzer。
