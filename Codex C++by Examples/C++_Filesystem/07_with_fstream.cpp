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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1929. Concatenation of Array（陣列串接自身）
// 題目：回傳 nums 後再接一份 nums；[1,2,1] 得 [1,2,1,1,2,1]。
// 為何使用本章主題：演算法本身不需 filesystem/fstream；本檔刻意將 content transform 與檔案 transport 分離。
// 思路：1. 以 nums 複製建立 output；2. 將 nums 範圍 insert 到尾端；3. 以值回傳完整結果。
// 複雜度：N 為元素數；時間 O(N)，輸出空間 O(N)。
// 易錯點：輸出需預期配置 2N 個元素；不要把檔案讀寫錯誤混成演算法的 LeetCode 契約。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】設定檔暫存後 rename 發布
// 情境：先完整寫入同目錄 temporary，再改名為 MANIFEST，避免讀者看到半份內容。
// 為何使用本章主題：filesystem 管路徑與 rename，fstream 負責 checked write/close；兩者各處理自己的錯誤邊界。
// 設計：1. 產生 destination.tmp；2. write_text 並確認 close；3. rename 成 destination。
// 成本：B 為內容 bytes；寫入 O(B)，同 filesystem rename 通常是 metadata I/O，另占 O(B) 暫存檔空間。
// 上線注意：固定 `.tmp` 會碰撞，rename 覆寫語意跨平台；atomic visibility 也不等於 fsync durability。
// -----------------------------------------------------------------------------
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

/*
 * 【教科書補充：atomic visibility 不等於 durability】
 * - iterator 讀檔可能把 I/O error 表現成提早結束；完成後仍要檢查 stream state。
 * - 固定 `.tmp` 名稱會碰撞，且 rename 失敗會留下垃圾；production 用同目錄唯一 temp + scope cleanup。
 * - rename 的覆寫與原子可見性細節受平台/檔案系統影響；跨 filesystem 通常不是原子 rename。
 * - 即使 rename 成功，斷電 durability 仍可能需要檔案與目錄 fsync；「看到新名稱」不是持久化證明。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '07_with_fstream.cpp' -o '/tmp/codex_cpp_C_Filesystem_07_with_fstream' && '/tmp/codex_cpp_C_Filesystem_07_with_fstream'
//
// === 預期輸出（節錄）===
// [實務] temp file renamed into published manifest
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
