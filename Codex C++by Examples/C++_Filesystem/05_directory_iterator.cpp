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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1166. Design File System（設計邏輯檔案系統）
// 題目：維護 path 到 int 的映射，只有 parent 已存在且 target 未存在時 createPath 才成功。
// 為何使用本章主題：此題其實不需 directory_iterator；VirtualFileSystem 用 unordered_map 誠實呈現 logical store，與 OS 目錄列舉作對照。
// 思路：1. 驗 absolute-like logical path 與 duplicate；2. 由最後斜線求 parent；3. parent 合法才 emplace，get 則查 map。
// 複雜度：L 為 path 長度；平均 create/get 時間 O(L)，雜湊最壞 O(N*L)，儲存空間 O(N*L)。
// 易錯點：root parent 以空字串特判；本 parser 未正規化重複斜線、`.`、`..`，依賴題目輸入契約。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】單層 log 檔案清單
// 情境：從指定目錄只列出直接 children 中的 regular `.log` files，輸出需穩定排序供報表比較。
// 為何使用本章主題：directory_iterator 恰好只走一層；directory_entry 分類檔案，path::extension 過濾副檔名。
// 設計：1. 逐 entry 走訪；2. 保留 regular 且 extension==.log；3. 收集後 sort。
// 成本：E 為 entries；走訪 O(E) 加排序 O(K log K)，結果空間 O(K)，metadata 另有 I/O 成本。
// 上線注意：iterator 順序未指定且 metadata 會過期；稍後 open 要重新處理錯誤，並明訂 symlink/permission policy。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_directory_iterator.cpp' -o '/tmp/codex_cpp_C_Filesystem_05_directory_iterator' && '/tmp/codex_cpp_C_Filesystem_05_directory_iterator'
//
// === 預期輸出（節錄）===
// [實務] filtered one regular .log file
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
