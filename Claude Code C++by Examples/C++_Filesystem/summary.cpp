/*
================================================================================
【C++_Filesystem/summary.cpp】

本目錄主題：C++17 <filesystem>（路徑、檔案狀態、遍歷、建立/刪除）

<filesystem> 是「跨平台」檔案系統 API（但仍要尊重 OS 差異）：
  - std::filesystem::path：路徑型別（自動處理分隔符、編碼等）
  - exists/status/is_*：查狀態
  - create_directories/remove/rename/copy：檔案操作
  - directory_iterator / recursive_directory_iterator：遍歷

重要提醒：
  - 檔案系統操作常常會丟例外（permission、路徑不存在、鎖定等）
    需要的話可用帶 error_code 參數的 overload 走「不丟例外」路線。

本 summary 原則：
  - 不加入 題庫 類範例
  - C++17 可編譯（Windows/MSVC 有些情況需要額外連結 stdc++fs，這裡以 g++ C++17 為主）

編譯：
  g++ -std=c++17 -Wall -Wextra summary.cpp -o summary && ./summary
================================================================================
*/

// ⚠️ 本檔曾為了「讓 clangd 靜態分析看得懂」而改用 <experimental/filesystem>，
//    結果反而製造出文件與實作不一致的問題：
//      ・標題與編譯命令都寫 C++17 <filesystem>
//      ・實作卻是 experimental 版
//      ・而 experimental::filesystem 在 GCC/libstdc++ 上【需要額外連結 -lstdc++fs】，
//        所以文件給的 g++ -std=c++17 ... 命令會 link 失敗（undefined reference）
//    正解是直接用標準的 <filesystem>：GCC 9+ / C++17 起不需要任何額外連結旗標。
//    （若 IDE 仍判定不可用，那是 IDE 的 C++ 標準設定要調，不該遷就它改壞程式碼。）
#ifndef _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#endif
/*
補充筆記：C++_Filesystem/C++_Filesystem summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_Filesystem/C++_Filesystem summary 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_Filesystem/C++_Filesystem summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>
#include <iostream>
#include <string>

static void demo_path_basics() {
    std::cout << "\n[demo_path_basics]\n";

    fs::path p = fs::current_path();
    std::cout << "  current_path = " << p.string() << "\n";

    fs::path file = p / "tmp_fs_demo" / "hello.txt";
    std::cout << "  joined path  = " << file.string() << "\n";
    std::cout << "  parent_path  = " << file.parent_path().string() << "\n";
    std::cout << "  filename     = " << file.filename().string() << "\n";
    std::cout << "  stem         = " << file.stem().string() << "\n";
    std::cout << "  extension    = " << file.extension().string() << "\n";
}

static void demo_create_write_read_remove() {
    std::cout << "\n[demo_create_write_read_remove]\n";

    fs::path dir = fs::current_path() / "tmp_fs_demo";
    fs::path file = dir / "hello.txt";

    // (1) 建資料夾（存在也 OK）
    fs::create_directories(dir);
    std::cout << "  created dir: " << dir.string() << "\n";

    // (2) 寫檔
    {
        std::ofstream out(file);
        out << "hello filesystem\n";
    }
    std::cout << "  wrote file: " << file.string() << "\n";

    // (3) exists / file_size
    std::cout << "  exists? " << fs::exists(file) << ", size=" << fs::file_size(file) << "\n";

    // (4) 讀檔（示範 with fstream）
    {
        std::ifstream in(file);
        std::string line;
        std::getline(in, line);
        std::cout << "  read line: " << line << "\n";
    }

    // (5) 刪檔、刪資料夾（remove_all 遞迴刪除，請小心）
    fs::remove(file);
    fs::remove_all(dir);
    std::cout << "  removed tmp_fs_demo\n";
}

static void demo_iterators() {
    std::cout << "\n[demo_iterators]\n";

    // 這裡遍歷目前工作目錄的第一層（避免輸出太多）
    fs::path p = fs::current_path();
    std::cout << "  list current_path entries (first 10):\n";

    int shown = 0;
    for (const auto& entry : fs::directory_iterator(p)) {
        std::cout << "    - " << entry.path().filename().string();
        // experimental::filesystem 的 directory_entry 可能沒有 is_directory() 成員函式，
        // 用 fs::is_directory(...) 這種 free function 最保險。
        if (fs::is_directory(entry.path())) std::cout << " [dir]";
        else if (fs::is_regular_file(entry.path())) std::cout << " [file]";
        std::cout << "\n";
        if (++shown >= 10) break;
    }

    std::cout << "  NOTE: recursive_directory_iterator 可遞迴遍歷整棵樹（輸出量會很大）\n";
}

static void demo_error_code_style_note() {
    std::cout << "\n[demo_error_code_style_note]\n";
    std::cout << "  filesystem API 多數都有 (path, error_code&) overload，可避免例外。\n";
}

int main() {
    demo_path_basics();
    demo_create_write_read_remove();
    demo_iterators();
    demo_error_code_style_note();

    std::cout << "\n[done]\n";
    return 0;
}

