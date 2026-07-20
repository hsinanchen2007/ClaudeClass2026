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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 387. First Unique Character in a String（字串中的第一個唯一字元）
// 題目：找第一個只出現一次的字元索引，找不到回 -1；例如 "leetcode" 回傳 0。
// 為何使用本章主題：本例刻意以 streambuf iterator 保留每個原始 char；讀入 string 後再做題目要求的兩趟統計。
// 思路：逐字讀完整 stream；以 256 格陣列計數；按原順序找頻率 1 的首字元；否則回 -1。
// 複雜度：時間 O(N)、額外空間 O(N+1)，N 為字元數；固定頻率表為 O(1)。
// 易錯點：char 作索引前轉 unsigned char；索引轉 int 要驗範圍；本版按 byte 計數，不正確處理 UTF-8 字元。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】將 CRLF 日誌正規化為 LF
// 情境：跨平台 ETL 收到含空白與 CRLF 的 log，要移除每個 LF 前的 CR 且保留其他所有字元。
// 為何使用本章主題：streambuf iterator 不做 formatted extraction，不會吞掉空白，適合逐字保真的文字轉換。
// 設計：讀入全部 raw 字元；預留相同容量；遇到 CRLF 的 CR 就略過；其餘字元照序追加。
// 成本：時間 O(N)、額外空間 O(N)，N 為輸入字元數；另有 stream I/O 成本。
// 上線注意：孤立 CR 會保留；大型檔案宜串流處理避免整檔常駐；二進位檔與編碼需使用明確模式與政策。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08b_streambuf_iterators.cpp' -o '/tmp/codex_cpp_C_Iterator_08b_streambuf_iterators' && '/tmp/codex_cpp_C_Iterator_08b_streambuf_iterators'
//
// === 預期輸出（節錄）===
// [實務] CRLF log normalized without losing spaces
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
