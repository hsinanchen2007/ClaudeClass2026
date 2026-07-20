// ============================================================================
// 課題 4：create_directory/create_directories/remove/remove_all
// ============================================================================
//
// create_directory 建一層；create_directories 遞迴建 parents，已存在通常回 false 而非失敗。
// remove 刪單檔或空目錄；remove_all 遞迴刪整棵並回刪除數。remove_all 是高風險 API：
// 先驗證 root/magic marker/allowlist，不可直接吃未信任字串，也不可對 `/`/home 使用。
//
// 建目錄後到寫入間仍有 race；安全敏感程式應使用 OS descriptor-relative APIs。一般工具
// 至少使用 canonical root、拒空/root path、dry-run 與清楚 logging。
// ============================================================================

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

class TempDir {
public:
    TempDir() : path_(fs::temp_directory_path() /
        ("codex_create_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    { fs::create_directory(path_); }
    ~TempDir() { std::error_code error; fs::remove_all(path_, error); }
    const fs::path& path() const { return path_; }
private: fs::path path_;
};

void basic_example()
{
    TempDir temp;
    const fs::path nested = temp.path() / "a" / "b" / "c";
    // 建立/刪除是範例本身的必要動作，不能放在會被 -DNDEBUG 移除的 assert 內。
    const bool created = fs::create_directories(nested);
    const bool created_again = fs::create_directories(nested);
    const bool removed = fs::remove(nested);
    assert(created);
    assert(!created_again); // idempotent: already exists。
    assert(removed);
    assert(!fs::exists(nested));
    std::cout << "[基礎] recursive create is idempotent; remove deletes empty leaf\n";
}

// LeetCode 1166：Design File System。每個 logical path 是可再擁有 children 的 node，
// 所以不能把 `/a` 建成普通檔案；本例以 directory 表 node，`.value` 保存該 node 的 int。
class FileSystem {
public:
    explicit FileSystem(fs::path root) : root_(std::move(root)) {}

    bool createPath(const std::string& logical_path, int value)
    {
        const auto relative = parse_relative(logical_path);
        if (!relative.has_value()) return false;
        const fs::path target = root_ / *relative;
        if (fs::exists(target) || !fs::is_directory(target.parent_path())) return false;
        if (!fs::create_directory(target)) return false;

        std::ofstream output(target / ".value");
        output << value;
        if (output) return true;

        std::error_code cleanup_error;
        fs::remove_all(target, cleanup_error);
        return false;
    }

    int get(const std::string& logical_path) const
    {
        const auto relative = parse_relative(logical_path);
        if (!relative.has_value()) return -1;
        std::ifstream input(root_ / *relative / ".value");
        int value = -1;
        return input >> value ? value : -1;
    }

private:
    static std::optional<fs::path> parse_relative(const std::string& logical_path)
    {
        if (logical_path.size() < 2U || logical_path.front() != '/') return std::nullopt;
        const fs::path relative = fs::path(logical_path).relative_path().lexically_normal();
        if (relative.empty() || *relative.begin() == "..") return std::nullopt;
        return relative;
    }

    fs::path root_;
};

void leetcode_1166_example()
{
    TempDir temp;
    FileSystem filesystem(temp.path());
    const bool created_a = filesystem.createPath("/a", 1);
    assert(created_a);
    assert(filesystem.get("/a") == 1);
    const bool created_b = filesystem.createPath("/a/b", 2);
    assert(created_b);
    assert(filesystem.get("/a/b") == 2);
    const bool created_without_parent = filesystem.createPath("/c/d", 3);
    const bool created_duplicate = filesystem.createPath("/a", 9);
    assert(!created_without_parent); // parent /c 不存在。
    assert(!created_duplicate);      // duplicate path。
    assert(filesystem.get("/missing") == -1);
    std::cout << "[LeetCode 1166] createPath/get 與 parent/duplicate 契約完整驗證\n";
}

// 實務：安全清理只允許由 caller 建立的 temporary child，並要求 marker。
std::uintmax_t remove_marked_tree(const fs::path& root)
{
    if (root.empty() || root == root.root_path() || !fs::exists(root / ".codex-temp")) {
        throw std::runtime_error("refuse unmarked removal");
    }
    return fs::remove_all(root);
}

void practical_example()
{
    TempDir parent;
    const fs::path target = parent.path() / "work";
    fs::create_directory(target);
    { std::ofstream marker(target / ".codex-temp"); marker << "marker"; }
    const std::uintmax_t removed_entries = remove_marked_tree(target);
    assert(removed_entries >= 2U);
    assert(!fs::exists(target));
    std::cout << "[實務] remove_all guarded by explicit marker\n";
}

int main()
{
    basic_example();
    leetcode_1166_example();
    practical_example();
}

// 練習：加入 dry-run，先列出 recursive_iterator 會刪的項目，不實際 remove。
// 複雜度：create_directory 單點操作近似 O(1) namespace I/O；remove_all 是 O(nodes) 且不可逆。
// 生命週期：TempDir 應以 RAII 綁 scope；解構 cleanup 不應 throw，失敗需另留可診斷訊息。
