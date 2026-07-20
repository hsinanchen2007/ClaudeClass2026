// =============================================================================
//  03_exists_and_status.cpp  —  查詢檔案存在與屬性
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/exists
//    - https://en.cppreference.com/w/cpp/filesystem/status
//    - https://en.cppreference.com/w/cpp/filesystem/file_size
//    - https://en.cppreference.com/w/cpp/filesystem/last_write_time
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 常用查詢函式                                               │
//  └────────────────────────────────────────────────────────────┘
//
//      fs::exists(p)             路徑指向的東西存在嗎
//      fs::is_regular_file(p)    是普通檔案？
//      fs::is_directory(p)       是資料夾？
//      fs::is_symlink(p)         是 symbolic link？
//      fs::is_empty(p)           檔案 size==0 或目錄無內容
//      fs::file_size(p)          檔案 byte 數（對非 regular file 會 throw）
//      fs::status(p)             整包查 — 回傳 file_status，含 type + perms
//      fs::last_write_time(p)    最後修改時間（C++20 可直接 << ）
//
//  注意：每個函式都有兩個 overload：
//      f(const path&)            失敗會 throw filesystem_error
//      f(const path&, error_code& ec)   不 throw，把錯誤寫進 ec
//
//  在「不確定路徑是否存在 / 是否有權限」的脈絡用 ec 版本更乾淨。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：先建一個臨時檔，查屬性
//   * Demo 2：對不存在路徑用 ec 版避免 throw
//   * Demo 3：file_status 一次查到位
// =============================================================================

/*
補充筆記：std::exists_and_status
  - std::exists_and_status 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - exists(p) 可能因權限或競態得到不完整資訊，工作程式應考慮 error_code overload。
  - status 和 symlink_status 對符號連結處理不同，掃描工具要先決定是否跟隨連結。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】查詢檔案存在與屬性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 if (fs::exists(p)) { open(p); } 是有問題的？
//     答：這是典型的 TOCTOU（Time-Of-Check to Time-Of-Use）競態。檢查與使用之間有時間
//     窗，其他行程或執行緒可以：刪掉該檔案讓 open 失敗；用 symlink 取代該路徑指向敏感
//     檔案，讓你的程式以自己的權限寫入不該寫的地方（經典提權漏洞）；或建立同名檔案，
//     讓你以為在建新檔卻覆蓋了別人的。正解是「不要先檢查再操作，直接操作並處理失敗」：
//     讀檔就 std::ifstream ifs(p); if (!ifs) { ... }；建檔要「不存在才建立」就用
//     O_CREAT | O_EXCL（這個組合本身是原子的）。
//     追問：<filesystem> 能消除 TOCTOU 嗎？（不能。它是對 syscall 的薄封裝，exists、
//     create_directory 等都是各自獨立的呼叫，之間必然有時間窗；需要嚴格保證得靠
//     O_NOFOLLOW 這類 open 旗標或平台專屬 API）
//
// 🔥 Q2. <filesystem> 的兩套 API（拋例外 vs error_code）該用哪個？
//     答：幾乎每個函式都有兩個多載：bool exists(const path&) 錯誤時拋 filesystem_error
//     （帶有 path1()、path2()、code()）；bool exists(const path&, error_code&) noexcept
//     則設定 ec、不拋。拋例外版程式碼乾淨，適合「檔案操作失敗就是致命錯誤」的情境；
//     error_code 版適合「失敗是預期情況」（掃描目錄時遇到權限不足的子目錄）、效能敏感
//     路徑，或不能用例外的環境。關鍵是：用了 error_code 版就「一定要檢查 ec」，否則
//     錯誤被完全吞掉，函式回傳預設值看起來像正常結果。
//     追問：exists(p, ec) 回傳 false 有幾種意思？（至少三種：檔案真的不存在、權限不足
//     無法判斷、路徑過長等其他錯誤——必須看 ec 才能區分）
//
// ⚠️ 陷阱. status() 和 symlink_status() 差在哪？exists() 對斷掉的 symlink 回什麼？
//     答：status(p) 會「跟隨」symlink，回傳目標的狀態，symlink 斷掉時回
//     file_type::not_found；symlink_status(p) 不跟隨，回傳 symlink 本身的狀態
//     （file_type::symlink）。因此 fs::exists(p) 對一個斷掉的 symlink 會回 false——
//     即使那個 symlink 檔案本身確實存在。要判斷「symlink 本身存在」得寫
//     fs::exists(fs::symlink_status(p))。
//     為什麼會錯：把 exists() 理解成「這個路徑上有沒有東西」，實際上它問的是「跟隨到
//     最後有沒有一個存在的目標」。
// ═══════════════════════════════════════════════════════════════════════════

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：安全大小查詢 (safe_file_size)
//   工作中常見：批次處理檔案前要查大小。要對「不存在 / 不是檔案 / 沒權限」
//   都優雅處理 — 用 ec 版避免 throw。回傳 -1 表示無法取得。
// ─────────────────────────────────────────────────────────
static long long safe_file_size(const fs::path& p) {
    std::error_code ec;
    if (!fs::is_regular_file(p, ec) || ec) return -1;
    auto sz = fs::file_size(p, ec);
    if (ec) return -1;
    return static_cast<long long>(sz);
}

static void demo_practical_safe_size() {
    std::cout << "[Practical] safe_file_size\n";
    // 先建一個臨時檔案做測試
    fs::path tmp{"tmp_safe_size.bin"};
    {
        std::ofstream out{tmp};
        out << "1234567890";                          // 10 bytes
    }
    std::vector<fs::path> targets{
        tmp, "not_existing.bin", "/tmp"  // 第 3 個是目錄，不是 regular file
    };
    for (auto& t : targets) {
        long long sz = safe_file_size(t);
        std::cout << "  " << t << " size = " << sz
                  << (sz < 0 ? " (失敗)" : " bytes") << '\n';
    }
    fs::remove(tmp);
}

int main() {
    fs::path file{"tmp_fs_query.txt"};

    // 先寫一個 35 byte 的小檔
    {
        std::ofstream out{file};
        out << "filesystem query test - 35 bytes!!";   // 35 chars
    }

    // ─────────────────────────────────────────────────────────
    // Demo 1：基本查詢
    // ─────────────────────────────────────────────────────────
    std::cout << std::boolalpha;
    std::cout << "[Demo1] exists           = " << fs::exists(file) << '\n';
    std::cout << "[Demo1] is_regular_file  = " << fs::is_regular_file(file) << '\n';
    std::cout << "[Demo1] is_directory     = " << fs::is_directory(file) << '\n';
    std::cout << "[Demo1] file_size        = " << fs::file_size(file) << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 error_code 處理「不存在」
    // ─────────────────────────────────────────────────────────
    fs::path nope{"this/does/not/exist"};
    std::error_code ec;
    auto sz = fs::file_size(nope, ec);
    if (ec) {
        std::cout << "[Demo2] file_size failed: " << ec.message()
                  << " (got " << sz << ")\n";
    } else {
        std::cout << "[Demo2] file_size = " << sz << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：status 一次取多項
    //   status 回傳 file_status，可以 query type() 與 permissions()
    // ─────────────────────────────────────────────────────────
    fs::file_status st = fs::status(file);
    using ft = fs::file_type;
    const char* typeStr = "?";
    switch (st.type()) {
        case ft::regular:   typeStr = "regular";   break;
        case ft::directory: typeStr = "directory"; break;
        case ft::symlink:   typeStr = "symlink";   break;
        case ft::not_found: typeStr = "not_found"; break;
        default:            typeStr = "other";     break;
    }
    std::cout << "[Demo3] type = " << typeStr << '\n';

    // 印 perms（POSIX 三元組 owner / group / other）
    auto perms = st.permissions();
    auto bit = [&](fs::perms b, char yes, char no) {
        return ((perms & b) != fs::perms::none) ? yes : no;
    };
    std::cout << "[Demo3] perms = "
              << bit(fs::perms::owner_read,  'r', '-')
              << bit(fs::perms::owner_write, 'w', '-')
              << bit(fs::perms::owner_exec,  'x', '-')
              << bit(fs::perms::group_read,  'r', '-')
              << bit(fs::perms::group_write, 'w', '-')
              << bit(fs::perms::group_exec,  'x', '-')
              << bit(fs::perms::others_read, 'r', '-')
              << bit(fs::perms::others_write,'w', '-')
              << bit(fs::perms::others_exec, 'x', '-')
              << '\n';

    // 清掉
    fs::remove(file);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：throw 版 vs ec 版？
    //    A：「不存在」是預期可能發生的情況時，用 ec 版比較乾淨。
    //       「我已經知道一定要存在，否則就是 bug」用 throw 版讓錯誤直接傳出。
    //
    //  Q2：file_size 對目錄會怎樣？
    //    A：throw filesystem_error（或 ec 版設錯誤）。要算「目錄總大小」要
    //       自己用 recursive_directory_iterator 累加 (見 06)。
    //
    //  Q3：last_write_time 怎麼印？
    //    A：C++20 起 std::format 直接支援 file_time_type；C++17 要自己用
    //       chrono 換成 system_clock::time_point 再 to_time_t。
    //
    demo_practical_safe_size();
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 03_exists_and_status.cpp -o 03_exists_and_status

// === 預期輸出 ===
// [Demo1] exists           = true
// [Demo1] is_regular_file  = true
// [Demo1] is_directory     = false
// [Demo1] file_size        = 34
// [Demo2] file_size failed: No such file or directory (got 18446744073709551615)
// [Demo3] type = regular
// [Demo3] perms = rw-rw-r--
// [Practical] safe_file_size
//   "tmp_safe_size.bin" size = 10 bytes
//   "not_existing.bin" size = -1 (失敗)
//   "/tmp" size = -1 (失敗)
