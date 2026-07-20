// ============================================================================
// 課題 5：directory_iterator - 單層遍歷
// ============================================================================
//
// directory_iterator 只列直接 children，順序 unspecified；需要穩定輸出就收集後 sort。
// entry 提供 path/status/file_size 等，但 metadata 可能在迭代後已改變。遍歷中刪改同一
// directory 的觀察結果未指定，先收集再操作通常較安全。
//
// 權限/I/O errors 可丟 filesystem_error；批次工具可用 error_code overload/option
// skip_permission_denied，但不能把略過項目誤宣稱為完整結果。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() : path_(fs::temp_directory_path() /
        ("codex_iter_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    { fs::create_directory(path_); }
    ~TempDir() { std::error_code e; fs::remove_all(path_, e); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

std::vector<std::string> sorted_children(const fs::path& directory)
{
    std::vector<std::string> names;
    for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
        names.push_back(entry.path().filename().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

void basic_example()
{
    TempDir temp;
    { std::ofstream(temp.path() / "b.txt") << 'b'; }
    { std::ofstream(temp.path() / "a.txt") << 'a'; }
    fs::create_directory(temp.path() / "subdir");
    assert((sorted_children(temp.path()) == std::vector<std::string>{"a.txt", "b.txt", "subdir"}));
    std::cout << "[基礎] unspecified iterator order normalized by sort\n";
}

// LeetCode 1166：Design File System。這是 logical path-value store，不是 OS filesystem；
// createPath 只有在 parent 已存在且 target 不存在時成功，get 不存在回 -1。
class VirtualFileSystem {
public:
    bool createPath(const std::string& path, int value)
    {
        if (path.size() < 2U || path.front() != '/' || values_.contains(path)) return false;
        const std::size_t slash = path.rfind('/');
        const std::string parent = path.substr(0U, slash);
        if (!parent.empty() && !values_.contains(parent)) return false;
        values_.emplace(path, value);
        return true;
    }

    int get(const std::string& path) const
    {
        const auto found = values_.find(path);
        return found == values_.end() ? -1 : found->second;
    }

private:
    std::unordered_map<std::string, int> values_;
};

void leetcode_1166_example()
{
    VirtualFileSystem filesystem;
    // createPath 會改變資料結構；先呼叫再 assert，release build 才仍會跑完整流程。
    const bool created_parent = filesystem.createPath("/leet", 1);
    const bool created_child = filesystem.createPath("/leet/code", 2);
    assert(created_parent);
    assert(created_child);
    assert(filesystem.get("/leet/code") == 2);
    const bool created_without_parent = filesystem.createPath("/c/d", 1);
    assert(!created_without_parent);
    assert(filesystem.get("/c") == -1);
    std::cout << "[LeetCode 1166] parent、duplicate 與 get 契約均驗證\n";
}

// 實務：只選 regular .log files；directory entries 仍可能在下一步被替換，open 要再驗。
std::vector<fs::path> log_files(const fs::path& directory)
{
    std::vector<fs::path> result;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".log") result.push_back(entry.path());
    }
    std::sort(result.begin(), result.end());
    return result;
}

void practical_example()
{
    TempDir temp;
    { std::ofstream(temp.path() / "one.log") << 1; }
    { std::ofstream(temp.path() / "two.txt") << 2; }
    const auto logs = log_files(temp.path());
    assert(logs.size() == 1U && logs.front().filename() == "one.log");
    std::cout << "[實務] filtered one regular .log file\n";
}

int main()
{
    basic_example();
    leetcode_1166_example();
    practical_example();
}

// 易錯與面試：directory iterator 的遍歷順序未指定；需要 deterministic report 就先收集
// path 再 sort。遍歷期間目錄可被外部改動，entry metadata 與稍後 open 結果可能不同。
// 練習：使用 error_code 版本，回傳「結果 + 是否完整」而非默默忽略錯誤。
// 複雜度：完整列舉為 O(E)，E 是目錄 entries；每筆 status 查詢還可能增加 I/O。
// 生命週期：directory_entry 可保存 path/status cache，但外部變更會讓 snapshot 過期；iterator 只在 range 有效。
