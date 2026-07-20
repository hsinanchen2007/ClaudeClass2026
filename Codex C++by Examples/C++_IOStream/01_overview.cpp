// ============================================================================
// 課題 1：iostream 架構 - stream、streambuf、formatted/unformatted I/O
// ============================================================================
//
// istream/ostream 提供 typed formatting 與狀態；底層 streambuf 負責字元來源/目的地。
// cin/cout、fstream、stringstream 共用同一介面，演算法可接 `std::istream&`，測試改用
// istringstream，不必綁 console/file。
//
// `operator>>` 是 formatted input（依 locale、跳空白）；getline/read/get 是 unformatted。
// stream 保存 formatting state 與 error bits；`if(stream)` 等價於沒有 fail/bad。輸出 `\n`
// 不強制 flush，std::endl 會 newline+flush，hot loop 不應濫用。
// ============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int sum_two(std::istream& input)
{
    int left = 0;
    int right = 0;
    if (!(input >> left >> right)) throw std::runtime_error("two integers required");
    return left + right;
}

void basic_example()
{
    std::istringstream input("20 22");
    assert(sum_two(input) == 42);
    std::ostringstream output;
    output << "answer=" << 42 << '\n';
    assert(output.str() == "answer=42\n");
    std::cout << "[基礎] same stream API works with in-memory buffers\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2235. Add Two Integers（兩整數相加）
// 題目：輸入兩個整數 num1、num2 並回傳總和；例如 12 與 -5 得 7。
// 為何使用本章主題：sum 本身保持純函式，example 才用 istringstream 模擬輸入 transport；
//       這是刻意分離 parsing 與演算法，iostream 並非加法所需資料結構。
// 思路：1. stream 抽取兩個 int；2. 檢查 stream 狀態；3. 將值交給純 sum；4. 驗證答案。
// 複雜度：加法時間與空間 O(1)；十進位解析成本為 O(D)，D 是兩個 token 的總位數。
// 易錯點：正式程式不可忽略 extraction 失敗或 signed overflow；純函式也不應偷偷讀 global cin。
// -----------------------------------------------------------------------------
int sum(int num1, int num2) { return num1 + num2; }

void leetcode_2235_example()
{
    std::istringstream input("12 -5");
    int left = 0, right = 0;
    input >> left >> right;
    assert(input && sum(left, right) == 7);
    std::cout << "[LeetCode 2235] parsed inputs, pure sum answer=7\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可注入 stream 的 key=value parser
// 情境：同一 parser 要能讀 stdin、檔案或測試用 istringstream，輸入 `threads=8\n` 得 `threads:8`。
// 為何使用本章主題：接收 std::istream& 抽象掉 transport，兩次 getline 又能保留 value 中的空白；
//       相較直接綁 cin，函式可測且可重用。
// 設計：1. 讀到第一個 `=` 作 key；2. 讀剩餘行作 value；3. 任一步失敗就丟格式錯誤；4. 組結果。
// 成本：時間 O(N)、額外空間 O(N)，N 是該行總字元數，key/value/result 都可能配置。
// 上線注意：要限制行長、拒空 key、定義多個 `=` 與 CRLF；借用的 stream 必須在呼叫期間有效，
//       並區分格式錯誤與底層 badbit。
// -----------------------------------------------------------------------------
std::string read_key_value(std::istream& input)
{
    std::string key;
    std::string value;
    if (!std::getline(input, key, '=') || !std::getline(input, value)) {
        throw std::runtime_error("key=value required");
    }
    return key + ":" + value;
}

void practical_example()
{
    std::istringstream config("threads=8\n");
    assert(read_key_value(config) == "threads:8");
    std::cout << "[實務] parser accepts generic istream dependency\n";
}

int main()
{
    basic_example();
    leetcode_2235_example();
    practical_example();
}

// 易錯與面試：istream/ostream 是有狀態物件；讀寫失敗後後續操作會短路，必須檢查 state。
// 依賴 `std::istream&` 而非 cin，可讓 production 接檔案、unit test 接 istringstream。
// 成本通常與輸入/輸出字元數成正比；formatted 數值轉換還受 locale 與 parsing 成本影響。
// 生命週期：函式只在呼叫期間借用 stream reference，不能保存指向 local stringstream 的
// pointer；ostringstream::str() 回傳 value，可在 stream 解構後繼續保存。
// 練習：讓 sum_two 接 trailing garbage 時回報，而非靜默接受 `1 2 xyz`。
// 複雜度：解析/輸出成本至少 O(處理字元數)，formatted conversion 還包含 locale/數值轉換。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_overview.cpp' -o '/tmp/codex_cpp_C_IOStream_01_overview' && '/tmp/codex_cpp_C_IOStream_01_overview'
//
// === 預期輸出（節錄）===
// [實務] parser accepts generic istream dependency
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
