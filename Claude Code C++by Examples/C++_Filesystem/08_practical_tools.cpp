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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】實務檔案工具
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. file_size() 有哪些注意事項？
//     答：它只對 regular file 有明確定義；對目錄或裝置檔的行為是實作定義或直接是錯誤，
//     所以呼叫前應先確認 is_regular_file()。它也有 error_code 多載，掃描大量檔案時用
//     error_code 版可以讓單一檔案的失敗（例如掃描途中被刪除）不中斷整個流程。另外在
//     迴圈中應優先用 directory_entry 的 e.file_size()，可以省下重複的 stat。
//     追問：symlink 的 file_size 是什麼？（會跟隨到目標；要看 symlink 本身得用其他方式）
//
// 🔥 Q2. last_write_time() 回傳的時間怎麼轉成可讀的時間？
//     答：它回傳 fs::file_time_type，其 clock 是實作定義的檔案系統時鐘，「不保證」是
//     std::chrono::system_clock——因為檔案系統的時間解析度、epoch 與範圍都可能不同
//     （例如 Windows 的 FILETIME 從 1601 年起算）。C++17 沒有標準的轉換方法；C++20 起
//     可用 std::chrono::clock_cast<std::chrono::system_clock>(ft) 正式轉換。
//
// ⚠️ 陷阱. 「先收集清單、再逐一處理」的批次工具，最容易在哪裡出錯？
//     答：在「收集」與「處理」之間，檔案系統可能已經改變——清單裡的檔案被刪除、被移動、
//     或大小改變了。所以批次工具必須把每一項的失敗當成正常情況處理（用 error_code 版
//     API、記錄失敗項目並繼續），而不是假設清單永遠有效。這是 TOCTOU 在批次工具上的
//     具體表現：清單本身就是一份會過期的快照。
//     為什麼會錯：把目錄掃描的結果當成一個穩定的資料結構，忽略檔案系統是共享的可變狀態。
//
// Q. std::filesystem::space() 回傳的 free 和 available 差在哪？
//     答：space_info 有 capacity / free / available 三欄，常被誤以為只有兩個：
//     capacity 是檔案系統總容量，free 是檔案系統上實際未使用的位元組數，available 則是
//     「非特權行程」實際可用的位元組數。差別來自 root 保留區（Linux ext4 預設保留 5%
//     給 root，避免磁碟寫滿時系統無法運作；保留比例屬檔案系統設定，非標準規定）。
//     本機實測 /（1005 GB）：free = 914 GB、available = 863 GB，差約 51 GB。
//     問「還剩多少可以寫」要看 available，用 free 會高估。
//     追問：呼叫失敗時怎麼判斷？（throwing 版會拋 filesystem_error；error_code 版則把
//     三個欄位都填成 static_cast<uintmax_t>(-1)，必須檢查 ec，否則會拿到一個看起來像
//     「超大容量」的值。另外這個數字回傳當下就可能過期——別的行程隨時在寫，先查空間
//     再寫入是典型的 TOCTOU，正確作法是直接寫並處理錯誤）
// ═══════════════════════════════════════════════════════════════════════════

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
