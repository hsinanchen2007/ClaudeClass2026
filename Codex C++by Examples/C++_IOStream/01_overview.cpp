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

// LeetCode 2235：Add Two Integers，將 transport/parsing 與純演算法分離。
int sum(int num1, int num2) { return num1 + num2; }

void leetcode_2235_example()
{
    std::istringstream input("12 -5");
    int left = 0, right = 0;
    input >> left >> right;
    assert(input && sum(left, right) == 7);
    std::cout << "[LeetCode 2235] parsed inputs, pure sum answer=7\n";
}

// 實務：dependency injection 讓 parser 同時可讀檔、stdin、socket adapter 或測試字串。
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
