// =============================================================================
//  08_practical_tools.cpp  —  工作小工具：找最大檔、依副檔名統計
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/recursive_directory_iterator
//    - https://en.cppreference.com/w/cpp/filesystem/path/extension
//    - https://en.cppreference.com/w/cpp/filesystem/file_size
//
//  把前面學到的 API 組合，做兩個立刻能丟到工作中的小工具：
//
//   工具 1：findLargestFile(root)   → 回傳最大的檔案 path 與 size
//   工具 2：summarizeByExtension(root) → 統計每種副檔名的「檔案數」與「總大小」
//
//  兩個都跑在「先建立的測試樹」上，讓範例可重現。
// =============================================================================

/*
補充筆記：std::practical_tools
  - std::practical_tools 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - 實用檔案工具應明確處理錯誤：找不到、權限不足、目標已存在、磁碟滿。
  - 批次操作前先收集計畫再執行，可降低處理一半失敗時狀態不一致的風險。
*/
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

// ─── 工具 1：找最大檔 ───
struct Result { fs::path path; std::uintmax_t size; };

static std::optional<Result> findLargestFile(const fs::path& root) {
    std::optional<Result> best;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(root,
            fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (it->is_regular_file(ec)) {
            auto sz = it->file_size(ec);
            if (!ec && (!best || sz > best->size)) {
                best = Result{it->path(), sz};
            }
        }
    }
    return best;
}

// ─── 工具 1.5（新增）：依「大小門檻」找出超大檔 ───
// 工作中常用：找出佔空間超過 N bytes 的所有檔案 (du-like + filter)。
static std::vector<Result> findFilesLargerThan(const fs::path& root,
                                               std::uintmax_t threshold) {
    std::vector<Result> out;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(root,
            fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (it->is_regular_file(ec)) {
            auto sz = it->file_size(ec);
            if (!ec && sz > threshold) out.push_back({it->path(), sz});
        }
    }
    return out;
}

// ─── 工具 2：依副檔名統計 ───
struct Stat { std::size_t count = 0; std::uintmax_t bytes = 0; };

static std::map<std::string, Stat> summarizeByExtension(const fs::path& root) {
    std::map<std::string, Stat> stats;
    for (auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file()) continue;
        std::string ext = e.path().extension().string();
        if (ext.empty()) ext = "<no-ext>";
        auto& s = stats[ext];
        s.count += 1;
        s.bytes += e.file_size();
    }
    return stats;
}

int main() {
    const fs::path root = "tmp_fs_dir";
    fs::remove_all(root);

    // 建一個多樣化的測試樹
    fs::create_directories(root / "src");
    fs::create_directories(root / "doc");
    fs::create_directories(root / "img");

    auto write = [](const fs::path& p, const std::string& content) {
        std::ofstream{p} << content;
    };
    write(root / "src/main.cpp",       std::string(2048, 'a'));
    write(root / "src/util.cpp",       std::string(512,  'b'));
    write(root / "src/util.h",         std::string(256,  'c'));
    write(root / "doc/readme.md",      std::string(1024, 'd'));
    write(root / "doc/notes.md",       std::string(800,  'e'));
    write(root / "img/logo.png",       std::string(8192, 'f'));   // 最大
    write(root / "Makefile",           std::string(64,   'g'));

    // ─────────────────────────────────────────────────────────
    // 工具 1：最大檔
    // ─────────────────────────────────────────────────────────
    if (auto r = findLargestFile(root)) {
        std::cout << "[largest] " << r->path << " (" << r->size << " bytes)\n";
    } else {
        std::cout << "[largest] no file\n";
    }

    // ─────────────────────────────────────────────────────────
    // 工具 1.5：找出 > 1000 bytes 的檔案
    // ─────────────────────────────────────────────────────────
    auto big = findFilesLargerThan(root, 1000);
    std::cout << "[>1000 bytes] " << big.size() << " 個檔案：\n";
    for (auto& r : big) std::cout << "  " << r.path << " (" << r.size << ")\n";

    // ─────────────────────────────────────────────────────────
    // 工具 2：依副檔名統計
    // ─────────────────────────────────────────────────────────
    auto stats = summarizeByExtension(root);
    std::cout << "[ext stats]\n";
    for (auto& [ext, s] : stats) {
        std::cout << "  " << ext
                  << "  count=" << s.count
                  << "  bytes=" << s.bytes << '\n';
    }

    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼工具 1 用 ec 版本、工具 2 用 throw 版？
    //    A：findLargestFile 設計成「永遠跑得完」 — 遇到不能讀的目錄就跳過，
    //       而不是丟例外打斷。summarize 假設目錄都讀得到（單純的 demo），
    //       例外傳出去由 caller 處理。
    //
    //  Q2：怎麼擴充成「最大 N 個檔」？
    //    A：用 std::priority_queue 或 partial_sort 維護 top-N。或用我們在
    //       C++_Algorithm/heap 與 C++_Algorithm/partial_sort 的範例做法。
    //
    //  Q3：怎麼把結果序列化成 JSON？
    //    A：標準沒提供 JSON 函式庫；可以自己 ostringstream 拼，或用 nlohmann
    //       /json (header-only)、RapidJSON 等。
    //
    return 0;
}
