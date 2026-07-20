// ============================================================================
// 課題 6：recursive_directory_iterator - 深度遍歷與控制
// ============================================================================
//
// recursive_directory_iterator 走完整 subtree，可用 depth()、disable_recursion_pending()、
// pop() 控制。預設不跟 directory symlink；follow_directory_symlink 可能形成 cycle，需記錄
// canonical/inode 或限制深度。skip_permission_denied 只代表繼續，不代表結果完整。
//
// 大樹掃描成本高且 metadata 可能變動；不要在每個 entry 無必要地重複 status/open。
// 需要刪除時通常先收集、按深度反序處理，或直接使用經嚴格守衛的 remove_all。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() : path_(fs::temp_directory_path() /
        ("codex_recursive_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    { fs::create_directory(path_); }
    ~TempDir() { std::error_code e; fs::remove_all(path_, e); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

std::size_t regular_file_count(const fs::path& root)
{
    std::size_t count = 0U;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file()) ++count;
    }
    return count;
}

void basic_example()
{
    TempDir temp;
    fs::create_directories(temp.path() / "a" / "b");
    { std::ofstream(temp.path() / "root.txt") << 1; }
    { std::ofstream(temp.path() / "a" / "b" / "leaf.txt") << 2; }
    assert(regular_file_count(temp.path()) == 2U);
    std::cout << "[基礎] recursive traversal found root and nested files\n";
}

// LeetCode 388：Longest Absolute File Path。每行前方 tab 數就是 DFS depth；
// length_at_depth[d] 保存「走到 depth d 父目錄後，已包含 separator 的長度」。
int length_longest_path(const std::string& encoded_tree)
{
    std::vector<std::size_t> length_at_depth{0U};
    std::size_t longest = 0U;
    std::istringstream input(encoded_tree);

    for (std::string line; std::getline(input, line);) {
        std::size_t depth = 0U;
        while (depth < line.size() && line.at(depth) == '\t') ++depth;
        const std::string name = line.substr(depth);
        if (depth >= length_at_depth.size()) {
            throw std::invalid_argument("encoded depth jumps over missing parent");
        }

        const std::size_t current_length = length_at_depth.at(depth) + name.size();
        if (name.find('.') != std::string::npos) {
            longest = std::max(longest, current_length);
        } else {
            length_at_depth.resize(depth + 2U);
            length_at_depth.at(depth + 1U) = current_length + 1U; // 加路徑 `/`。
        }
    }
    return static_cast<int>(longest);
}

void leetcode_388_example()
{
    const std::string first = "dir\n\tsubdir1\n\tsubdir2\n\t\tfile.ext";
    const std::string second = "file1.txt\nfile2.txt\nlongfile.txt";
    assert(length_longest_path(first) == 20); // dir/subdir2/file.ext
    assert(length_longest_path(second) == 12);
    assert(length_longest_path("a") == 0);   // 沒有檔案。
    std::cout << "[LeetCode 388] tab-encoded tree 最長絕對路徑長度=20\n";
}

// 實務：略過名為 .git 的 subtree，避免掃描 repository internals。
std::vector<fs::path> source_files(const fs::path& root)
{
    std::vector<fs::path> result;
    for (fs::recursive_directory_iterator iterator(root), end; iterator != end; ++iterator) {
        if (iterator->is_directory() && iterator->path().filename() == ".git") {
            iterator.disable_recursion_pending();
        } else if (iterator->is_regular_file() && iterator->path().extension() == ".cpp") {
            result.push_back(iterator->path());
        }
    }
    return result;
}

void practical_example()
{
    TempDir temp;
    fs::create_directories(temp.path() / ".git" / "objects");
    { std::ofstream(temp.path() / "main.cpp") << "int main(){}"; }
    { std::ofstream(temp.path() / ".git" / "objects" / "hidden.cpp") << "x"; }
    const auto sources = source_files(temp.path());
    assert(sources.size() == 1U && sources.front().filename() == "main.cpp");
    std::cout << "[實務] recursion skipped .git subtree\n";
}

int main()
{
    basic_example();
    leetcode_388_example();
    practical_example();
}

// 易錯與面試：follow_directory_symlink 可能形成 cycle；預設不跟隨較安全。要略過 subtree
// 必須在 iterator 尚指目錄時 disable_recursion_pending，並決定 permission error policy。
// 練習：加入 max_depth，並說明 depth() 在 root children 時的值。
// 複雜度：遞迴走訪是 O(nodes)，額外 traversal state 與深度成正比；symlink policy 會改圖形。
// 生命週期：iterator 保存目前 traversal state；increment 後舊 entry reference 不應長期留用。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_recursive_iterator.cpp' -o '/tmp/codex_cpp_C_Filesystem_06_recursive_iterator' && '/tmp/codex_cpp_C_Filesystem_06_recursive_iterator'
//
// === 預期輸出（節錄）===
// [實務] recursion skipped .git subtree
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
