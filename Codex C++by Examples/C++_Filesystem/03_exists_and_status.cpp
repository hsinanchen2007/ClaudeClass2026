// ============================================================================
// 課題 3：exists/status/symlink_status 與 error_code overload
// ============================================================================
//
// `status(path)` 跟隨 symlink，`symlink_status(path)` 看 link 本身。exists 不代表下一行
// open 一定成功：兩者之間檔案可被刪除/替換（TOCTOU），權限也可能不同。真正操作仍需
// 檢查 open/read/write 結果。
//
// filesystem functions 有 throwing 與 error_code overload。掃描/診斷工具常用 error_code
// 繼續處理其他項目；關鍵操作可讓 filesystem_error 帶 path/code 傳上去。不要把 permission
// denied 誤報成「不存在」。
// ============================================================================

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir()
        : path_(fs::temp_directory_path() /
                ("codex_status_" + std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count())))
    {
        fs::create_directory(path_);
    }
    ~TempDir() { std::error_code error; fs::remove_all(path_, error); }
    const fs::path& path() const { return path_; }
private:
    fs::path path_;
};

void basic_example()
{
    TempDir temp;
    const fs::path file = temp.path() / "data.txt";
    { std::ofstream output(file); output << "hello"; }
    assert(fs::exists(file));
    assert(fs::is_regular_file(fs::status(file)));
    assert(!fs::exists(temp.path() / "missing"));
    std::cout << "[基礎] status identifies regular file and missing path\n";
}

// LeetCode 1971：Find if Path Exists in Graph。名稱雖也叫 path，這裡的 path 是 graph route，
// 不是 filesystem::path；BFS 以 visited 防 cycle，時間 O(V+E)、空間 O(V+E)。
bool valid_path(int node_count, const std::vector<std::vector<int>>& edges,
                int source, int destination)
{
    std::vector<std::vector<int>> graph(static_cast<std::size_t>(node_count));
    for (const auto& edge : edges) {
        graph.at(static_cast<std::size_t>(edge.at(0))).push_back(edge.at(1));
        graph.at(static_cast<std::size_t>(edge.at(1))).push_back(edge.at(0));
    }
    std::vector<bool> visited(static_cast<std::size_t>(node_count), false);
    std::queue<int> pending;
    pending.push(source);
    visited.at(static_cast<std::size_t>(source)) = true;
    while (!pending.empty()) {
        const int current = pending.front();
        pending.pop();
        if (current == destination) return true;
        for (const int neighbor : graph.at(static_cast<std::size_t>(current))) {
            if (!visited.at(static_cast<std::size_t>(neighbor))) {
                visited.at(static_cast<std::size_t>(neighbor)) = true;
                pending.push(neighbor);
            }
        }
    }
    return false;
}

void leetcode_1971_example()
{
    assert(valid_path(3, {{0, 1}, {1, 2}, {2, 0}}, 0, 2));
    assert(!valid_path(6, {{0, 1}, {0, 2}, {3, 5}, {5, 4}, {4, 3}}, 0, 5));
    assert(valid_path(1, {}, 0, 0));
    std::cout << "[LeetCode 1971] BFS 正確區分相連與不相連 components\n";
}

// 實務：status metadata 可判是否接受 input；實際開檔仍再次檢查 stream。
bool readable_regular_file(const fs::path& path)
{
    std::error_code error;
    if (!fs::is_regular_file(path, error) || error) return false;
    std::ifstream input(path);
    return input.good();
}

void practical_example()
{
    TempDir temp;
    const fs::path file = temp.path() / "input";
    { std::ofstream output(file); output << 42; }
    assert(readable_regular_file(file));
    assert(!readable_regular_file(temp.path()));
    std::cout << "[實務] metadata check plus actual open validates input\n";
}

int main()
{
    basic_example();
    leetcode_1971_example();
    practical_example();
}

// 易錯與面試：exists 後再 open 存在 TOCTOU；metadata 是提示，不是授權或可讀保證。
// status 跟隨 symlink，symlink_status 看 link 本身；安全工具必須明確選擇所需語意。
// 練習：在支援 symlink 的系統建立 dangling link，比較 status/symlink_status。
// 複雜度：每次 status/exists 通常至少一次 filesystem 查詢，批次時應避免同一路徑重複查。
// 生命週期：file_status 是當下 snapshot；路徑可能在下一條指令前被刪除/替換，不能當永久保證。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_exists_and_status.cpp' -o '/tmp/codex_cpp_C_Filesystem_03_exists_and_status' && '/tmp/codex_cpp_C_Filesystem_03_exists_and_status'
//
// === 預期輸出（節錄）===
// [實務] metadata check plus actual open validates input
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
