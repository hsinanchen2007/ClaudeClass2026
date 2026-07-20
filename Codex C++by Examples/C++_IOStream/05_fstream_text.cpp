// ============================================================================
// 課題 5：Text fstream - modes、open/write/close checks
// ============================================================================
//
// ifstream 預設 in；ofstream 預設 out|trunc；fstream 不同組合要明寫。app 每次寫到尾端，
// ate 只在開啟時定位尾端，之後仍可 seek。開啟、每段 critical write、flush/close 都可能
// 失敗；destructor close 無法回報給 caller，重要輸出應 explicit close 後檢查。
//
// Text mode 在 Windows 可能做 newline translation；需要 byte-exact 加 ios::binary。
// `while(getline(...))`/`while(input>>x)`，不要 `while(!eof())`。
// ============================================================================

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

class TempFile {
public:
    TempFile() : path_(fs::temp_directory_path() /
        ("codex_text_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) {}
    ~TempFile() { std::error_code error; fs::remove(path_, error); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

void write_lines(const fs::path& path, const std::vector<std::string>& lines)
{
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output) throw std::runtime_error("open output failed");
    for (const auto& line : lines) output << line << '\n';
    output.close();
    if (!output) throw std::runtime_error("write/close failed");
}

std::vector<std::string> read_lines(const fs::path& path)
{
    std::ifstream input(path);
    if (!input) throw std::runtime_error("open input failed");
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);) lines.push_back(line);
    if (!input.eof()) throw std::runtime_error("read failed before EOF");
    return lines;
}

void basic_example()
{
    TempFile file;
    write_lines(file.path(), {"first", "second"});
    assert((read_lines(file.path()) == std::vector<std::string>{"first", "second"}));
    std::cout << "[基礎] checked text file round-trip two lines\n";
}

// LeetCode 1480：從文字檔 transport 讀 nums，演算法仍是純 function。
std::vector<int> running_sum(std::vector<int> nums)
{
    std::partial_sum(nums.begin(), nums.end(), nums.begin());
    return nums;
}

void leetcode_1480_example()
{
    TempFile file;
    write_lines(file.path(), {"1", "2", "3", "4"});
    std::ifstream input(file.path());
    std::vector<int> nums;
    for (int value = 0; input >> value;) nums.push_back(value);
    assert((running_sum(nums) == std::vector<int>{1, 3, 6, 10}));
    std::cout << "[LeetCode 1480] text transport + pure running-sum logic\n";
}

// 實務：append audit line，不覆寫舊資料；durability 仍需 OS fsync policy。
void practical_example()
{
    TempFile file;
    write_lines(file.path(), {"start"});
    { std::ofstream output(file.path(), std::ios::app); output << "done\n"; assert(output); }
    assert((read_lines(file.path()) == std::vector<std::string>{"start", "done"}));
    std::cout << "[實務] app mode preserved existing audit line\n";
}

int main()
{
    basic_example();
    leetcode_1480_example();
    practical_example();
}

// 易錯與面試：ofstream constructor/open 失敗不一定丟 exception，預設要檢查 stream；
// flush/close 只推到 OS，不等同儲存媒體 durability。文字模式也不適合 raw binary schema。
// 練習：模擬 read-only/不存在路徑，分辨 open failure 與 clean EOF。
// 複雜度：逐行讀寫是 O(total bytes)；每行 string allocation 與 flush policy 也影響常數成本。
// 生命週期：fstream 擁有 OS handle，解構會 close；由 local line 取得的 string_view 不可帶出迴圈。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_fstream_text.cpp' -o '/tmp/codex_cpp_C_IOStream_05_fstream_text' && '/tmp/codex_cpp_C_IOStream_05_fstream_text'
//
// === 預期輸出（節錄）===
// [實務] app mode preserved existing audit line
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
