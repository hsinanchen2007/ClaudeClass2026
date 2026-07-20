// ============================================================================
// 課題 8：std::istream_iterator<T> / std::ostream_iterator<T>
// ============================================================================
//
// Stream iterator 把 operator>> / operator<< 包成 iterator 介面，讓 algorithm 可直接
// 接資料流。istream_iterator<T> 是 single-pass；遇 EOF 或解析失敗變成 end。
// ostream_iterator<T>(out, delimiter) 每次賦值都輸出 T 再輸出 delimiter。
//
// 適合：空白分隔數值、簡單 CLI pipeline、把容器格式化成一列。
// 不適合：需要精確錯誤位置、CSV quoting、欄位可缺省的複雜格式；那時應用 parser。
// 注意 ostream_iterator 的 delimiter 會在最後一項後也出現，不能把它當無尾逗號 join。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <unordered_set>
#include <vector>

std::vector<int> read_integers(std::istream& input)
{
    const std::istream_iterator<int> first(input);
    const std::istream_iterator<int> last;
    return {first, last};
}

void basic_example()
{
    std::istringstream input("10 20 30");
    const std::vector<int> values = read_integers(input);
    assert((values == std::vector<int>{10, 20, 30}));

    std::ostringstream output;
    std::copy(values.begin(), values.end(), std::ostream_iterator<int>(output, " "));
    assert(output.str() == "10 20 30 "); // 尾端也有 delimiter。
    std::cout << "[基礎] typed stream iterators parsed and emitted three ints\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 217. Contains Duplicate（存在重複元素）
// 題目：輸入整數序列，任兩個位置值相同即回 true；例如串流 "1 2 3 1" 為 true。
// 為何使用本章主題：本例把原陣列改成 istream_iterator 的 single-pass 輸入，讀到重複即可提早停止。
// 思路：建立空 seen；逐次解參考 stream iterator；insert 失敗即回 true；走到 sentinel 回 false。
// 複雜度：平均時間 O(N)、最壞 O(N^2)，額外空間 O(U)，N 為解析值數、U 為不同值數。
// 易錯點：提早回傳會留下未讀 stream；格式錯誤也會抵達 end；必須另查 stream state 才能區分 EOF。
// -----------------------------------------------------------------------------
bool contains_duplicate(std::istream& input)
{
    std::unordered_set<int> seen;
    for (std::istream_iterator<int> it(input), end; it != end; ++it) {
        if (!seen.insert(*it).second) return true;
    }
    return false;
}

void leetcode_217_example()
{
    std::istringstream duplicate("1 2 3 1");
    std::istringstream unique("1 2 3 4");
    assert(contains_duplicate(duplicate));
    assert(!contains_duplicate(unique));
    std::cout << "[LeetCode 217] duplicate detected during one-pass parsing\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】監控命令延遲資料的匯入與匯出
// 情境：工具 stdout 提供空白分隔 latency，程式計算平均，並轉成每行一筆供下一個工具讀取。
// 為何使用本章主題：istream_iterator/ostream_iterator 讓簡單 typed pipeline 直接接標準演算法與 stream。
// 設計：讀取所有整數；accumulate 後除以筆數；用換行 delimiter 將原值逐筆輸出。
// 成本：解析與輸出 O(N) 加 I/O，保存值空間 O(N)，N 為 latency 筆數。
// 上線注意：空輸入會除以零；總和可能溢位；需檢查輸入 failbit、輸出錯誤與不可信資料量。
// -----------------------------------------------------------------------------
void practical_example()
{
    std::istringstream command_output("12 18 15 11");
    const std::vector<int> latency_ms = read_integers(command_output);
    const int total = std::accumulate(latency_ms.begin(), latency_ms.end(), 0);
    assert(total / static_cast<int>(latency_ms.size()) == 14);

    std::ostringstream exported;
    std::copy(latency_ms.begin(), latency_ms.end(),
              std::ostream_iterator<int>(exported, "\n"));
    assert(exported.str() == "12\n18\n15\n11\n");
    std::cout << "[實務] command metrics parsed and exported line by line\n";
}

int main()
{
    basic_example();
    leetcode_217_example();
    practical_example();
}

// 易錯：formatted extraction 遇非法 token 設 failbit 並停止；不等於「檔案正常讀完」。
// 面試自問：解析到非法 token 時，如何區分正常 EOF 與 failbit？
// 練習：將輸入 "1 2 x 3" 改成逐 token parser，回報 x 的位置而非靜默停止。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_stream_iterators.cpp' -o '/tmp/codex_cpp_C_Iterator_08_stream_iterators' && '/tmp/codex_cpp_C_Iterator_08_stream_iterators'
//
// === 預期輸出（節錄）===
// [實務] command metrics parsed and exported line by line
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
