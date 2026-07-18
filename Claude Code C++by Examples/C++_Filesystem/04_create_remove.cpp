// =============================================================================
//  04_create_remove.cpp  —  建立 / 刪除 / 拷貝 / rename
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/create_directory
//    - https://en.cppreference.com/w/cpp/filesystem/remove
//    - https://en.cppreference.com/w/cpp/filesystem/copy
//    - https://en.cppreference.com/w/cpp/filesystem/rename
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 常用「修改檔案系統」的 API                                 │
//  └────────────────────────────────────────────────────────────┘
//
//      fs::create_directory(p)       建一層；父目錄不存在會失敗
//      fs::create_directories(p)     遞迴建多層 (像 mkdir -p)
//      fs::remove(p)                 刪一個檔案 / 空目錄
//      fs::remove_all(p)             遞迴刪除（小心使用！）
//      fs::rename(from, to)          搬移／改名（同一檔案系統內 atomic）
//      fs::copy(from, to)            拷貝（檔案、目錄、含遞迴與 overwrite 的選項）
//      fs::copy_file(from, to)       只拷貝檔案、可指定覆寫策略
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ copy 的 options                                            │
//  └────────────────────────────────────────────────────────────┘
//
//      fs::copy_options::none                  預設
//      fs::copy_options::skip_existing         目標存在跳過
//      fs::copy_options::overwrite_existing    覆寫
//      fs::copy_options::update_existing       目標較舊才覆寫
//      fs::copy_options::recursive             進入子目錄
//      fs::copy_options::copy_symlinks
//      fs::copy_options::skip_symlinks
//      fs::copy_options::directories_only
//      fs::copy_options::create_symlinks
//      fs::copy_options::create_hard_links
//
//  可以用 | 組合使用，例如「遞迴拷一棵樹、目標衝突則覆寫」：
//      fs::copy(src, dst, fs::copy_options::recursive |
//                         fs::copy_options::overwrite_existing);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：mkdir -p（多層建立）
//   * Demo 2：寫檔、rename、copy、刪除
//   * Demo 3：remove_all 清理整顆樹
// =============================================================================

/*
補充筆記：std::create_remove
  - std::create_remove 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - create_directory 只建立一層，create_directories 才會建立整條缺少的路徑。
  - remove_all 很危險，應先確認目標路徑，避免刪到非預期目錄。
*/
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：簡易 log rotator (日誌輪替)
//   工作中常見：把 app.log 滾掉變成 app.log.1，把舊的 .1 變 .2…
//   用 fs::rename 即可，搭配「最大保留份數」。
// ─────────────────────────────────────────────────────────
static void rotate_log(const fs::path& base, int keep) {
    // 從最舊的開始 rename，避免覆蓋：keep-1 → keep, keep-2 → keep-1, ...
    for (int i = keep - 1; i >= 1; --i) {
        fs::path from = base; from += "." + std::to_string(i);
        fs::path to   = base; to   += "." + std::to_string(i + 1);
        std::error_code ec;
        if (fs::exists(from, ec)) fs::rename(from, to, ec);
    }
    // 最後把 base 改名為 base.1
    std::error_code ec;
    if (fs::exists(base, ec)) {
        fs::path target = base; target += ".1";
        fs::rename(base, target, ec);
    }
}

static void demo_practical_log_rotator() {
    std::cout << "[Practical] log rotator\n";
    const fs::path root{"tmp_log_dir"};
    fs::remove_all(root);
    fs::create_directories(root);
    fs::path log = root / "app.log";

    // 寫三次、每次 rotate 一次
    for (int round = 0; round < 3; ++round) {
        std::ofstream{log} << "round " << round << '\n';
        rotate_log(log, 3);
    }
    std::cout << "  rotation done. dir contents:\n";
    for (auto& e : fs::directory_iterator(root))
        std::cout << "    " << e.path().filename() << '\n';
    fs::remove_all(root);
}

int main() {
    const fs::path root = "tmp_fs_dir";

    // 起點先確保乾淨
    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // Demo 1：建立多層目錄
    // ─────────────────────────────────────────────────────────
    fs::create_directories(root / "a/b/c");
    std::cout << "[Demo1] created: " << (root / "a/b/c") << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：寫個檔、rename 改名、copy 拷一份、刪除原檔
    // ─────────────────────────────────────────────────────────
    fs::path file1 = root / "data.txt";
    {
        std::ofstream out{file1};
        out << "hello\n";
    }
    std::cout << "[Demo2] wrote " << file1 << '\n';

    fs::path file2 = root / "data_v2.txt";
    fs::rename(file1, file2);
    std::cout << "[Demo2] renamed " << file1.filename()
              << " → " << file2.filename() << '\n';

    fs::path file3 = root / "data_copy.txt";
    fs::copy_file(file2, file3,
                  fs::copy_options::overwrite_existing);
    std::cout << "[Demo2] copied to " << file3.filename() << '\n';

    fs::remove(file3);
    std::cout << "[Demo2] removed " << file3.filename() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：列一下目前狀態，再 remove_all 全砍
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo3] tree before clean:\n";
    for (auto& e : fs::recursive_directory_iterator(root)) {
        std::cout << "  " << e.path() << '\n';
    }
    auto removed = fs::remove_all(root);
    std::cout << "[Demo3] remove_all: removed " << removed << " entries\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：rename 跨檔案系統 (e.g. /tmp 到 /home) 會失敗嗎？
    //    A：很可能會。POSIX rename(2) 要求兩端在同一個 filesystem。要跨
    //       partition 應該用 copy_file + remove，或 fs::copy 加上
    //       recursive | remove_options。
    //
    //  Q2：remove vs remove_all 的差異？
    //    A：remove 對目錄只能是「空目錄」，否則回傳 false (或 ec 設錯誤)；
    //       remove_all 會遞迴砍光，回傳被刪除的「entry 數」。
    //       remove_all 是「危險工具」 — 確認 path 沒寫錯再用。
    //
    //  Q3：怎麼像 cp -r 一樣拷整棵樹？
    //    A：fs::copy(src, dst, fs::copy_options::recursive)。要想覆蓋衝突
    //       就 |= overwrite_existing。
    //
    demo_practical_log_rotator();
    return 0;
}
