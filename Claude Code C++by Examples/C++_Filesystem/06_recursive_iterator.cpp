// =============================================================================
//  06_recursive_iterator.cpp  —  遞迴列目錄
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/recursive_directory_iterator
//    - https://en.cppreference.com/w/cpp/filesystem/directory_options
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、recursive_directory_iterator                           │
//  └────────────────────────────────────────────────────────────┘
//
//  跟 directory_iterator 用法一樣，但會「自動進入子目錄」。類似 ls -R 或
//  find .。
//
//      for (auto& e : fs::recursive_directory_iterator(p)) {
//          std::cout << e.path() << '\n';
//      }
//
//  每個 entry 還有：
//   * .depth()      目前在第幾層（0 = root 直接小孩）
//   * .disable_recursion_pending()
//                   呼叫後「下一個 entry 不要進去那個子目錄」
//                   (e.g. 跳過 .git / node_modules 之類的常用 trick)
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、處理權限不足等錯誤                                     │
//  └────────────────────────────────────────────────────────────┘
//
//  預設遇到不能進去的子目錄會 throw filesystem_error。可以指定 options：
//
//      using opt = fs::directory_options;
//      fs::recursive_directory_iterator(p, opt::skip_permission_denied);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：建立樹狀結構並全列
//   * Demo 2：跳過特定資料夾（disable_recursion_pending）
//   * Demo 3：累計目錄總大小（du-like）
// =============================================================================

/*
補充筆記：std::recursive_iterator
  - std::recursive_iterator 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - recursive_directory_iterator 會深入子目錄，遇到權限問題可能丟例外或需用 options 跳過。
  - 遞迴掃描大型目錄要注意效能、符號連結循環與最大深度控制。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】recursive_directory_iterator
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. recursive_directory_iterator 比 directory_iterator 多了什麼？
//     答：它會深度優先遞迴進入子目錄，並額外提供 depth()（目前深度）、
//     disable_recursion_pending()（跳過即將進入的那個目錄）、pop()（跳出目前這一層），
//     以及 directory_options 參數。其餘性質與 directory_iterator 相同：InputIterator、
//     順序未指定、不含 . 與 .. 。
//     追問：怎麼跳過某棵子樹？（在該目錄「被進入之前」呼叫 it.disable_recursion_pending()
//     ——時機是關鍵，進去之後再呼叫就沒用了）
//
// 🔥 Q2. 它預設會跟隨目錄 symlink 嗎？為什麼這個預設值很重要？
//     答：預設不跟隨，除非明確給 directory_options::follow_directory_symlink。這個預設
//     值防止了 symlink 迴圈造成的無窮遞迴（例如目錄內有個 symlink 指回自己的祖先），
//     也避免掃描範圍意外擴散到整個檔案系統。要開啟跟隨時，必須自己負責處理迴圈偵測。
//
// ⚠️ 陷阱. 掃描整個目錄樹時，遇到一個沒有權限的子目錄會怎樣？
//     答：預設會拋 filesystem_error，整個迴圈就此中斷——已經掃到的結果還在，但剩下的
//     完全沒掃。對「掃描使用者主目錄」這種必然會遇到權限問題的工作，正解是建構時給
//     directory_options::skip_permission_denied，或改用接受 error_code 的形式並在
//     迴圈中逐項檢查。
//     為什麼會錯：以為權限不足只會讓「那一項」被跳過，實際上例外會直接終止整個遍歷。
// ═══════════════════════════════════════════════════════════════════════════

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：cleanup_old_files
//   工作中常用：清掉 build 目錄裡所有 .o / .tmp 檔案。
//   回傳「被刪掉的位元組總數」，方便寫進 log。
// ─────────────────────────────────────────────────────────
static std::uintmax_t cleanup_by_extensions(
    const fs::path& root,
    const std::vector<std::string>& exts) {
    std::uintmax_t freed = 0;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(root, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec) || ec) continue;
        std::string e = it->path().extension().string();
        for (auto& x : exts) {
            if (e == x) {
                auto sz = it->file_size(ec);
                fs::remove(it->path(), ec);
                if (!ec) freed += sz;
                break;
            }
        }
    }
    return freed;
}

static void demo_practical_cleanup() {
    std::cout << "[Practical] cleanup_by_extensions\n";
    const fs::path root{"tmp_clean_dir"};
    fs::remove_all(root);
    fs::create_directories(root / "build");
    std::ofstream{root / "build/a.o"}   << std::string(100, 'x');
    std::ofstream{root / "build/b.o"}   << std::string(200, 'y');
    std::ofstream{root / "build/c.tmp"} << std::string(50,  'z');
    std::ofstream{root / "keep.cpp"}    << "// keep\n";
    auto freed = cleanup_by_extensions(root, {".o", ".tmp"});
    std::cout << "  freed = " << freed << " bytes\n";
    std::cout << "  remaining files:\n";
    for (auto& e : fs::recursive_directory_iterator(root))
        if (e.is_regular_file())
            std::cout << "    " << e.path().filename() << '\n';
    fs::remove_all(root);
}

int main() {
    const fs::path root = "tmp_fs_dir";
    fs::remove_all(root);

    // 建一棵樹
    fs::create_directories(root / "src/feature");
    fs::create_directories(root / "src/util");
    fs::create_directories(root / "build");
    fs::create_directories(root / ".git/objects");          // 模擬 .git

    auto write = [](const fs::path& p, const std::string& s) {
        std::ofstream{p} << s;
    };
    write(root / "README.md",                 "hello world\n");
    write(root / "src/main.cpp",              "int main(){}\n");
    write(root / "src/feature/feat.cpp",      "// feat\n");
    write(root / "src/util/util.cpp",         "// util\n");
    write(root / "build/main.o",              "binary..\n");
    write(root / ".git/objects/abc",          "blob\n");

    // ─────────────────────────────────────────────────────────
    // Demo 1：列出整棵樹（含 depth 縮排）
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo1] full tree:\n";
    for (auto it = fs::recursive_directory_iterator(root);
         it != fs::recursive_directory_iterator(); ++it)
    {
        std::cout << std::string(it.depth() * 2, ' ')
                  << it->path().filename();
        if (it->is_directory()) std::cout << '/';
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：跳過 .git 與 build
    //   遇到要進去的子目錄前，先 disable_recursion_pending
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo2] tree (skipping .git and build):\n";
    for (auto it = fs::recursive_directory_iterator(root);
         it != fs::recursive_directory_iterator(); ++it)
    {
        const auto name = it->path().filename().string();
        if (it->is_directory() && (name == ".git" || name == "build")) {
            it.disable_recursion_pending();         // 不要進去
        }
        std::cout << std::string(it.depth() * 2, ' ')
                  << name;
        if (it->is_directory()) std::cout << '/';
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 3：算整顆樹的 byte 總和（du-like）
    // ─────────────────────────────────────────────────────────
    std::uintmax_t total = 0;
    for (auto& e : fs::recursive_directory_iterator(root)) {
        if (e.is_regular_file()) total += e.file_size();
    }
    std::cout << "[Demo3] total bytes under " << root << " = " << total << '\n';

    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼有 disable_recursion_pending 而不是「直接跳過 entry」？
    //    A：「跳過」常見有兩種意思：(a) 不列這條 entry (b) 不進去這個目
    //       錄但仍列出它本身。disable_recursion_pending 是 (b) — 列出但
    //       不進入；要做 (a) 自己 if continue 即可。
    //
    //  Q2：iteration 中怎麼處理 symlink loop？
    //    A：預設 recursive_directory_iterator 不跟隨 symlink；要跟隨用
    //       directory_options::follow_directory_symlink，但要小心循環。
    //
    //  Q3：怎麼像 `find . -name "*.cpp"`？
    //    A：自己 if entry.path().extension() == ".cpp" 即可。或結合
    //       std::regex 對 filename 匹配。
    //
    demo_practical_cleanup();
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 06_recursive_iterator.cpp -o 06_recursive_iterator

// === 預期輸出 (節錄) ===
// [Demo1] full tree:
// "README.md"
// ".git"/
//   "objects"/
//     "abc"
// "build"/
//   "main.o"
// "src"/
//   "main.cpp"
//   "util"/
//     "util.cpp"
//   "feature"/
//     "feat.cpp"
// [Demo2] tree (skipping .git and build):
// README.md
// .git/
// build/
// src/
//   main.cpp
//   util/
// …（後略，完整輸出共 28 行）
