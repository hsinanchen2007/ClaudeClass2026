// =============================================================================
//  05_directory_iterator.cpp  —  列目錄
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/directory_iterator
//    - https://en.cppreference.com/w/cpp/filesystem/directory_entry
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、directory_iterator 是什麼？                            │
//  └────────────────────────────────────────────────────────────┘
//
//  迭代「一個目錄裡」的所有 entry（檔案 + 子目錄），不遞迴進子目錄。
//  類似 Python 的 os.scandir 或 ls 不加 -R。
//
//  每個 element 是 fs::directory_entry，可呼叫：
//   * .path()              取 fs::path
//   * .is_regular_file()
//   * .is_directory()
//   * .file_size()
//   * .last_write_time()
//
//  好處：directory_entry 內部有「快取 stat」 — 你呼叫 is_regular_file 時
//  不會再 stat 一次 OS，比 fs::is_regular_file(p) 一次次呼叫快。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、語法                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//      for (auto& e : fs::directory_iterator(p)) {
//          std::cout << e.path() << '\n';
//      }
//
//  順序：未指定（依 OS 提供）。要排序自己 collect 後 sort。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：建一個小目錄、列出內容
//   * Demo 2：分類成 file / dir
//   * Demo 3：依檔名排序輸出
// =============================================================================

/*
補充筆記：std::directory_iterator
  - std::directory_iterator 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - directory_iterator 只走一層，且順序未指定；需要穩定輸出要自行排序。
  - 迭代中檔案可能被新增刪除，檔案系統掃描要能接受競態。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】directory_iterator 列目錄
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. directory_iterator 有哪些必須知道的性質？
//     答：它只列出該目錄的「直接」內容、不進入子目錄；它是 InputIterator（單向、
//     single-pass）；預設建構的迭代器就是 end iterator，所以可以直接用在 range-based
//     for 裡。兩個常被忽略的重點：① 列舉順序未指定，不保證字典序，需要排序就自己收集
//     後 std::sort ② 結果不包含 . 與 .. 。
//     追問：遇到沒有讀取權限的子目錄會怎樣？（預設拋 filesystem_error 中斷整個迴圈；
//     用 directory_options::skip_permission_denied 或 error_code 版建構才能繼續）
//
// 🔥 Q2. directory_entry 為什麼比對 path 重新查詢更高效？
//     答：因為 directory_entry 快取了作業系統在列舉目錄時「順便」回傳的 metadata
//     （Linux 的 readdir 會給 d_type）。所以迴圈裡寫 e.is_regular_file()、e.file_size()
//     有機會完全不需要額外的 syscall；而寫成 fs::is_regular_file(e.path()) 則每次都要
//     再做一次 stat，掃描大目錄時差距非常明顯。
//     追問：什麼時候快取一定不夠？（symlink 需要真的 stat 才知道目標型別；某些檔案系統
//     的 readdir 會回傳 DT_UNKNOWN。此外快取可能過期，需要最新狀態要呼叫 e.refresh()）
//
// ⚠️ 陷阱. directory_entry 的快取讓程式變快，有什麼代價？
//     答：快取本身就是一種 TOCTOU——它反映的是「列舉當下」的狀態，你在迴圈裡讀到的
//     size 或型別，可能在你真正去開檔時已經改變了。所以「用快取加速掃描」與「操作前
//     不要相信快取」要同時成立：統計、報表可以用快取；真正要開檔或刪檔時，仍然要直接
//     操作並處理失敗，而不是依賴先前快取的判斷。
//     為什麼會錯：把快取當成「同一份事實的便宜副本」，忽略檔案系統是會被別人改動的
//     共享狀態。
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：list_files_matching
//   工作上常需要做「找出某目錄下，副檔名是 X 的所有檔案，依檔名排序」。
//   類似 `ls *.cpp | sort` 的功能。
// ─────────────────────────────────────────────────────────
static std::vector<fs::path>
list_files_matching(const fs::path& root, const std::string& ext) {
    std::vector<fs::path> out;
    for (auto& e : fs::directory_iterator(root)) {
        if (e.is_regular_file() && e.path().extension() == ext) {
            out.push_back(e.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

static void demo_practical_list_matching() {
    std::cout << "[Practical] list_files_matching .cpp\n";
    const fs::path root{"tmp_match_dir"};
    fs::remove_all(root);
    fs::create_directories(root);
    // 建若干測試檔
    std::ofstream{root / "z.cpp"} << "z";
    std::ofstream{root / "a.cpp"} << "a";
    std::ofstream{root / "b.h"}   << "b";
    std::ofstream{root / "m.cpp"} << "m";

    auto cpps = list_files_matching(root, ".cpp");
    for (auto& p : cpps) std::cout << "  " << p.filename() << '\n';
    fs::remove_all(root);
}

int main() {
    const fs::path root = "tmp_fs_dir";
    fs::remove_all(root);
    fs::create_directories(root / "subA");
    fs::create_directories(root / "subB");

    // 寫幾個小檔
    {
        std::ofstream{root / "alpha.txt"} << "1\n";
        std::ofstream{root / "beta.txt"}  << "22\n";
        std::ofstream{root / "gamma.log"} << "333\n";
        std::ofstream{root / "subA/inside.txt"} << "x\n";  // 子目錄裡的不被 demo1 看到
    }

    // ─────────────────────────────────────────────────────────
    // Demo 1：列當前目錄內容（不進子目錄）
    // ─────────────────────────────────────────────────────────
    std::cout << "[Demo1] entries in " << root << ":\n";
    for (auto& e : fs::directory_iterator(root)) {
        std::cout << "  " << e.path().filename();
        if (e.is_directory()) std::cout << " [DIR]";
        else if (e.is_regular_file()) std::cout << "  (" << e.file_size() << " bytes)";
        std::cout << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // Demo 2：把 file 與 dir 分開印
    // ─────────────────────────────────────────────────────────
    std::vector<fs::path> files, dirs;
    for (auto& e : fs::directory_iterator(root)) {
        if (e.is_directory())   dirs.push_back(e.path());
        else                    files.push_back(e.path());
    }
    std::cout << "[Demo2] dirs:";  for (auto& d : dirs)  std::cout << ' ' << d.filename();
    std::cout << "\n[Demo2] files:"; for (auto& f : files) std::cout << ' ' << f.filename();
    std::cout << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：排序輸出（按檔名）
    // ─────────────────────────────────────────────────────────
    std::vector<fs::path> all;
    for (auto& e : fs::directory_iterator(root)) all.push_back(e.path());
    std::sort(all.begin(), all.end(),
              [](const fs::path& a, const fs::path& b) {
                  return a.filename() < b.filename();
              });
    std::cout << "[Demo3] sorted:";
    for (auto& p : all) std::cout << ' ' << p.filename();
    std::cout << '\n';

    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：為什麼每次都用 e 而不是 e.path()？
    //    A：directory_entry 內快取了「entry 的 stat 資訊」 — 用 e.is_xxx /
    //       e.file_size 比每次 fs::is_xxx(e.path()) 少一次 syscall。
    //       Linux readdir() 對某些 fs 還會直接告訴你 d_type（不必 stat）。
    //
    //  Q2：列大目錄會卡嗎？
    //    A：底層每次 readdir() 拿一批 entry，疊加 stat 開銷。10 萬個 entry
    //       會耗掉幾百 ms 是正常的。需要快可以用 OS-specific API（getdents、
    //       FindFirstFileEx），但喪失可攜性。
    //
    //  Q3：directory_iterator 與 entry 是 stable 的嗎？
    //    A：標準規定的是「迭代開始後才新增／刪除的檔案，這次迭代是否會看到」
    //       屬於 unspecified（未指定），【不是】未定義行為 —— 程式不會因此
    //       壞掉，但你拿到的清單可能包含、也可能不包含那些變動。要結果穩定，
    //       先把 iterator 結果 collect 進 vector，再做後續操作。
    //
    demo_practical_list_matching();
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 05_directory_iterator.cpp -o 05_directory_iterator

// === 預期輸出 ===
// [Demo1] entries in "tmp_fs_dir":
//   "gamma.log"  (4 bytes)
//   "beta.txt"  (3 bytes)
//   "alpha.txt"  (2 bytes)
//   "subB" [DIR]
//   "subA" [DIR]
// [Demo2] dirs: "subB" "subA"
// [Demo2] files: "gamma.log" "beta.txt" "alpha.txt"
// [Demo3] sorted: "alpha.txt" "beta.txt" "gamma.log" "subA" "subB"
// [Practical] list_files_matching .cpp
//   "a.cpp"
//   "m.cpp"
//   "z.cpp"
