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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1971. Find if Path Exists in Graph（圖中是否存在路徑）
// 題目：給無向圖、source 與 destination，判斷兩點是否相連；三角形圖的 0 到 2 為 true。
// 為何使用本章主題：題名的 path 是 graph route，不是 filesystem::path；本例刻意對照兩種完全不同的「存在」契約。
// 思路：1. 由 edges 建雙向 adjacency list；2. BFS queue 從 source 出發；3. visited 防環，抵達 destination 即成功。
// 複雜度：V 為節點數、E 為邊數；時間 O(V+E)、額外空間 O(V+E)。
// 易錯點：本實作依賴 LeetCode 保證節點與 edge 索引合法；visited 應在 enqueue 時設定以免重複入列。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】可讀 regular file 輸入驗證
// 情境：批次工具只接受可實際開啟的 regular file，目錄、不存在路徑與 metadata 錯誤都要拒絕。
// 為何使用本章主題：is_regular_file(error_code) 先分類 metadata，再以 ifstream 真正 open；不把 exists 當永久保證。
// 設計：1. 查 regular file 並檢查 error；2. 嘗試開啟 input stream；3. 只有 stream.good() 才接受。
// 成本：通常至少一次 metadata 查詢與一次 open I/O；時間受 filesystem 影響，額外記憶體 O(1)。
// 上線注意：檢查與 open 間仍有 TOCTOU；安全邊界應直接開 descriptor 並驗 fstat，還要明訂 symlink 政策。
// -----------------------------------------------------------------------------
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
