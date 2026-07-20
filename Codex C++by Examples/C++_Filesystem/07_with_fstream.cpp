// ============================================================================
// 課題 7：filesystem + fstream，安全發佈檔案
// ============================================================================
//
// filesystem 管 path/metadata，fstream 管內容。ofstream 建構成功不代表最後 flush/close
// 成功；寫完檢查 stream state。避免直接 truncate 正式檔：先在同一 directory 寫 temp、
// flush/close 驗證，再 rename 發佈。POSIX 同 filesystem rename 通常 atomic replacement；
// Windows/既有 destination/跨 filesystem 行為不同，需按平台處理。
//
// Atomic visibility 不等於 durable storage；斷電安全還涉及 fsync temp file 與 parent dir，
// standard fstream 沒有 portable fsync。重要資料需 OS API/DB。
// ============================================================================

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() : path_(fs::temp_directory_path() /
        ("codex_fstream_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    { fs::create_directory(path_); }
    ~TempDir() { std::error_code e; fs::remove_all(path_, e); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

void write_text(const fs::path& path, const std::string& text)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot open output");
    output << text;
    output.close();
    if (!output) throw std::runtime_error("write/close failed");
}

std::string read_text(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input");
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void basic_example()
{
    TempDir temp;
    const fs::path file = temp.path() / "note.txt";
    write_text(file, "hello\n");
    assert(read_text(file) == "hello\n");
    assert(fs::file_size(file) == 6U);
    std::cout << "[基礎] filesystem path + checked fstream round-trip\n";
}

// LeetCode 1929：把檔案中的整數 vector concatenate 後寫回，真正執行題目邏輯。
std::vector<int> concatenate(const std::vector<int>& nums)
{
    std::vector<int> output = nums;
    output.insert(output.end(), nums.begin(), nums.end());
    return output;
}

void leetcode_1929_example()
{
    const auto output = concatenate({1, 2, 1});
    assert((output == std::vector<int>{1, 2, 1, 1, 2, 1}));
    std::cout << "[LeetCode 1929] content transform remains separate from file transport\n";
}

// 實務：同 directory temp + rename 發佈。此測試 destination 原先不存在，跨平台較一致。
void publish_text(const fs::path& destination, const std::string& text)
{
    const fs::path temporary = destination.string() + ".tmp";
    write_text(temporary, text);
    fs::rename(temporary, destination);
}

void practical_example()
{
    TempDir temp;
    const fs::path manifest = temp.path() / "MANIFEST";
    publish_text(manifest, "format=1\n");
    assert(read_text(manifest) == "format=1\n");
    assert(!fs::exists(manifest.string() + ".tmp"));
    std::cout << "[實務] temp file renamed into published manifest\n";
}

int main()
{
    basic_example();
    leetcode_1929_example();
    practical_example();
}

// 易錯與面試：close/rename 成功不等於 power-loss durable；高可靠發布還需 fsync file 與
// parent directory（且依 OS/filesystem 調整）。rename 是否可覆寫 destination 也跨平台不同。
// 練習：為 overwrite destination 寫 Linux/Windows 分流，並研究 fsync durability。
// 複雜度：copy/read/write 是 O(bytes)；rename 常只改同 filesystem metadata，但跨 filesystem 不保證。
// 生命週期：fstream 以 RAII 持有 file descriptor，離開 scope 會 close；close 成功仍不等於 durable fsync。
