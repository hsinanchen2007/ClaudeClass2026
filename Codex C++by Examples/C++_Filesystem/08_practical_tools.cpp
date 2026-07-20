// ============================================================================
// 課題 8：實務工具 - extension filter、copy、relative manifest
// ============================================================================
//
// 真實檔案工具要處理：輸入/輸出重疊、symlink policy、permission errors、stable ordering、
// overwrite policy、partial copy、dry-run、checksum verification。copy_options::recursive 與
// overwrite_existing 很強，使用前要明定方向，避免把 source/destination 寫反。
//
// 本例建立 manifest：只記 root-relative generic paths，排序後輸出，讓不同 absolute root
// 可比較。這不是內容完整性驗證；重要備份另算 hash/size 並在 copy 後 check。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() : path_(fs::temp_directory_path() /
        ("codex_tools_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    { fs::create_directory(path_); }
    ~TempDir() { std::error_code e; fs::remove_all(path_, e); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】可重現的原始碼 manifest
// 情境：遞迴掃描 root，只列 `.cpp`/`.h`，以 root-relative generic path 排序後供兩台機器比較。
// 為何使用本章主題：recursive_directory_iterator 走樹，extension 過濾，lexically_relative 移除機器專屬 absolute root。
// 設計：1. 遞迴保留 regular source files；2. 轉成 relative generic string；3. sort 產生穩定順序。
// 成本：E 為 entries、K 為來源檔；走訪 O(E)、排序 O(K log K)，結果字串空間 O(total path bytes)。
// 上線注意：manifest 只證明名稱，不證明內容；要明訂 symlink/permission policy，重要比較另加 size/hash。
// -----------------------------------------------------------------------------
std::vector<std::string> source_manifest(const fs::path& root)
{
    std::vector<std::string> paths;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (entry.is_regular_file() &&
            (entry.path().extension() == ".cpp" || entry.path().extension() == ".h")) {
            paths.push_back(entry.path().lexically_relative(root).generic_string());
        }
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

void basic_example()
{
    TempDir temp;
    fs::create_directory(temp.path() / "src");
    { std::ofstream(temp.path() / "src" / "main.cpp") << "int main(){}"; }
    { std::ofstream(temp.path() / "README.md") << "docs"; }
    assert((source_manifest(temp.path()) == std::vector<std::string>{"src/main.cpp"}));
    std::cout << "[基礎] manifest filters source and stores relative path\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 609. Find Duplicate File in System（尋找重複檔案）
// 題目：解析 `dir name(content)` 描述，依相同 content 分組，只回傳至少兩個 path 的群組。
// 為何使用本章主題：fs::path/operator/ 組合 directory 與 filename；題目資料是字串，不會讀取真實檔案。
// 思路：1. 每行先讀 directory；2. 拆出 filename 與括號 content；3. 依 content 分組並保留 size>1 的 groups。
// 複雜度：T 為輸入總字元數、F 為檔案數；平均時間 O(T)，額外空間 O(T)。
// 易錯點：本實作依賴格式保證，production 必須驗括號與 npos；真檔 duplicate 應用 size prefilter 加內容 hash。
// -----------------------------------------------------------------------------
std::vector<std::vector<std::string>> find_duplicate(const std::vector<std::string>& descriptions)
{
    std::unordered_map<std::string, std::vector<std::string>> by_content;
    for (const std::string& line : descriptions) {
        std::istringstream input(line);
        std::string directory;
        input >> directory;
        for (std::string file; input >> file;) {
            const std::size_t open = file.find('(');
            const std::string name = file.substr(0U, open);
            const std::string content = file.substr(open + 1U, file.size() - open - 2U);
            by_content[content].push_back((fs::path(directory) / name).generic_string());
        }
    }
    std::vector<std::vector<std::string>> result;
    for (auto& entry : by_content) if (entry.second.size() > 1U) result.push_back(std::move(entry.second));
    return result;
}

void leetcode_609_example()
{
    const auto groups = find_duplicate({"root/a 1.txt(x) 2.txt(y)", "root/b 3.txt(x)"});
    assert(groups.size() == 1U && groups.front().size() == 2U);
    std::cout << "[LeetCode 609] grouped two paths with content x\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】單檔備份與大小核對
// 情境：將 source.txt 複製為不存在的 backup.txt，並確認 copy 確實執行且目的大小相同。
// 為何使用本章主題：fs::copy_file 搭配 copy_options::none 明訂不覆寫政策，回傳 bool 表示是否真的複製。
// 設計：1. 建立來源；2. 執行 copy_file 並消費 bool；3. 比較 source/destination file_size。
// 成本：B 為檔案大小；複製 I/O O(B)、額外目的儲存 O(B)，metadata 驗證另有查詢成本。
// 上線注意：同 size 不代表內容相同；重要備份要 hash/check、保留來源到驗證完成，並處理 partial copy 與 close 錯誤。
// -----------------------------------------------------------------------------
void practical_example()
{
    TempDir temp;
    const fs::path source = temp.path() / "source.txt";
    const fs::path destination = temp.path() / "backup.txt";
    { std::ofstream(source) << "important"; }
    // copy_file 是要示範的必要 I/O，不能藏在 release build 會移除的 assert 內。
    const bool copied = fs::copy_file(source, destination, fs::copy_options::none);
    assert(copied);
    assert(fs::file_size(destination) == fs::file_size(source));
    std::cout << "[實務] explicit single-file copy and size verification passed\n";
}

int main()
{
    basic_example();
    leetcode_609_example();
    practical_example();
}

// 易錯與面試：相同 file_size 不是內容相同；checksum 也要選 collision policy。copy 完成後
// 若資料重要，應驗 size/hash 並保存來源直到驗證通過，不能把 `mv 到 mount` 當遠端完成。
// 練習：把 duplicate detection 改成 file_size prefilter + SHA-256，而非直接讀全部內容。
// 複雜度：樹掃描 O(files)，內容 hash 再加 O(total bytes)；先以 size 分組可省不必要讀取。
// 生命週期：回傳的 path/value 自行擁有文字，但它指向的檔案可能隨時被其他程序改動或刪除。

/*
 * 【教科書補充：檔名 parser 與 copy 驗證】
 * - 解析 duplicate suffix 前要驗 `(`/`)`、數字、空檔名與尾隨垃圾；不可對 npos 直接做算術。
 * - 若契約要求 regular file，程式本身要檢查，不可只在註解假設 caller 已驗證。
 * - source fixture 寫入也要檢查 close；copy 後只比 size 不能證明內容一致，重要資料應比 hash/bytes。
 * - symlink、權限、既有 destination 與部分失敗策略都要在 API 層明確選擇。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '08_practical_tools.cpp' -o '/tmp/codex_cpp_C_Filesystem_08_practical_tools' && '/tmp/codex_cpp_C_Filesystem_08_practical_tools'
//
// === 預期輸出（節錄）===
// [實務] explicit single-file copy and size verification passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
