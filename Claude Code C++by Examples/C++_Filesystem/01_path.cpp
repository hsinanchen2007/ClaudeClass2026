// =============================================================================
//  01_path.cpp  —  std::filesystem::path 基礎
// =============================================================================
//  參考：https://en.cppreference.com/w/cpp/filesystem/path
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、為什麼有 path？                                        │
//  └────────────────────────────────────────────────────────────┘
//
//  路徑字串看起來「就是 std::string」，但「跨平台」一旦考慮就麻煩：
//
//   * Windows 用 "\\"，Linux / macOS 用 "/"
//   * Windows 路徑大小寫不敏感、Linux 敏感
//   * Windows 有「磁碟代號」(C:\)
//   * UNC 路徑、URL-like 路徑、UTF-8 / UTF-16 編碼差異
//
//  std::filesystem::path 把「路徑」抽象成型別，自動處理上述差異；不必
//  自己拼接 "/" 或 "\\"，不必自己 split。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、建立 path                                             │
//  └────────────────────────────────────────────────────────────┘
//
//      namespace fs = std::filesystem;
//
//      fs::path p1{"/usr/local/bin/ls"};          // 從字串
//      fs::path p2 = "data" / fs::path{"a.txt"};  // 用 / 拼接
//      fs::path p3 = fs::current_path();          // 程式當前工作目錄
//      fs::path p4 = fs::temp_directory_path();   // 系統暫存目錄
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、轉字串（要看用途）                                     │
//  └────────────────────────────────────────────────────────────┘
//
//      p.string()       平台原生 narrow string（Linux 上是 UTF-8 慣例）
//      p.u8string()     UTF-8 (C++17 type 為 std::string；C++20 改為 u8string)
//      p.generic_string() 把 \\ 換成 / 的標準寫法
//      p.native()       平台原生型別 (Linux: std::string; Windows: std::wstring)
//      p.c_str()        C-string view
//
//  cout << path 預設加引號，方便閱讀；想要原始字串就手動 string()。
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 四、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：四種建立方式
//   * Demo 2：operator/ 連接
//   * Demo 3：cout vs string()
// =============================================================================

/*
補充筆記：std::path
  - std::path 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - std::filesystem::path 保存平台路徑語意，operator/ 可安全組合路徑片段。
  - path 可存在但檔案不存在；路徑物件和檔案系統狀態要分開理解。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::filesystem::path 基礎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. path 和 std::string 差在哪？為什麼需要一個專門的型別？
//     答：四個理由：① 跨平台分隔符——path 內部使用 native 格式，operator/ 會自動處理
//     拼接，不必手寫 "a" + "/" + "b" ② 原生字元型別——Windows 上 path::value_type 是
//     wchar_t，POSIX 上是 char；用 std::string 存路徑在 Windows 上處理非 ASCII 檔名
//     會壞掉 ③ 結構化存取——filename()、stem()、extension()、parent_path()，而且路徑
//     元素本身可迭代 ④ 比較與正規化——lexically_normal()、compare()。
//     追問：怎麼轉回字串？（.string() 可能牽涉轉換；.native() 零成本但型別依平台；
//     .generic_string() 一律用 /；.u8string() 在 C++20 起回傳 std::u8string，這是相對
//     C++17 的 breaking change，原本接 std::string 的程式碼會編譯失敗）
//
// 🔥 Q2. /a/b/c.txt 的 filename()、stem()、extension()、parent_path() 各是什麼？
//     答：依序是 c.txt、c、.txt、/a/b。注意 extension() 是「包含那個點」的（.txt 而不是
//     txt）。三個必考的邊界：① /a/b/ 的 filename() 是空字串（尾端有隱含的空元素），
//     想取目錄名 b 要寫 p.parent_path().filename() ② 開頭的點不算副檔名——.bashrc 的
//     stem() 是 .bashrc、extension() 是空，has_extension() 回 false ③ 只取最後一個
//     副檔名——archive.tar.gz 的 stem() 是 archive.tar、extension() 是 .gz。
//
// ⚠️ 陷阱. path("/home/user") / "/etc" 的結果是什麼？
//     答：是 "/etc"，不是 "/home/user/etc"。operator/ 遇到絕對路徑的右運算元會丟棄左邊
//     （與 Python 的 os.path.join 行為一致）。這在拼接使用者輸入時是路徑穿越漏洞的來源。
//     安全的做法是先 lexically_normal()，再檢查結果是否仍以 base 為前綴；或直接拒絕
//     含 .. 或以 / 開頭的輸入。
//     為什麼會錯：把 operator/ 想成單純的字串接合，忽略它有「絕對路徑重設」的語意。
// ═══════════════════════════════════════════════════════════════════════════

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：path_helpers
//   工作中常見的兩個「用 path 解析」的小工具，沒有 Leetcode 對應題目
//   (filesystem 主題 Leetcode 通常不會考)，所以改成「立刻能用」的範例。
//   工具 1：把多段「設定路徑」用 operator/ 安全拼起來，並輸出 generic 樣式。
//   工具 2：對一串字串組成的相對路徑，回傳對應的「絕對路徑表」。
// ─────────────────────────────────────────────────────────
static fs::path join_config_path(std::initializer_list<std::string> segs) {
    fs::path p;                                      // 起始空 path
    for (auto& s : segs) p /= s;                     // 逐段 append
    return p;
}

static std::vector<fs::path> to_absolute_list(const std::vector<std::string>& rels) {
    std::vector<fs::path> out;
    out.reserve(rels.size());
    fs::path base = fs::current_path();              // 以目前工作目錄為基準
    for (auto& r : rels) out.push_back(base / r);    // 相對 → 絕對
    return out;
}

static void demo_practical_path_helpers() {
    std::cout << "[Practical] path_helpers\n";
    // 工具 1：把 (etc, myapp, settings.json) 安全拼成跨平台路徑
    fs::path cfg = join_config_path({"etc", "myapp", "settings.json"});
    std::cout << "  join_config_path = " << cfg.generic_string() << '\n';

    // 工具 2：把多個相對路徑批次轉絕對
    auto abs_list = to_absolute_list({"a.txt", "sub/b.txt"});
    for (auto& p : abs_list) std::cout << "  abs => " << p.generic_string() << '\n';
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：建立 path
    // ─────────────────────────────────────────────────────────
    fs::path p1{"/usr/local/bin"};
    fs::path p2{"data/items.csv"};
    fs::path cwd = fs::current_path();
    fs::path tmp = fs::temp_directory_path();

    std::cout << "[Demo1] p1   = " << p1   << '\n';
    std::cout << "[Demo1] p2   = " << p2   << '\n';
    std::cout << "[Demo1] cwd  = " << cwd  << '\n';
    std::cout << "[Demo1] tmp  = " << tmp  << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：用 operator/ 連接 — 不用煩惱分隔符
    // ─────────────────────────────────────────────────────────
    fs::path p = cwd / "build" / "out" / "log.txt";
    std::cout << "[Demo2] " << p << '\n';

    // 也可以原地 append
    p /= "subdir";
    p /= "another.txt";
    std::cout << "[Demo2] after /= " << p << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：印出方式
    //   * cout << path 會加雙引號（Demo1 已看到）
    //   * .string() 取「乾淨」字串，但內容跟原 path 一樣
    // ─────────────────────────────────────────────────────────
    fs::path q{"/foo bar/baz.txt"};
    std::cout << "[Demo3] cout: " << q << '\n';
    std::cout << "[Demo3] str : " << q.string() << '\n';

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：path 比 std::string 好在哪？
    //    A：(1) 自動處理 separator (跨平台)
    //       (2) 內建一堆成員函式 (parent_path、filename、extension...)
    //       (3) 跟其他 fs API 直接接，不必反覆 c_str() 互轉
    //
    //  Q2：path 是值還是 ref 語意？
    //    A：值語意 — 拷貝是 deep copy（內含字串）。傳給函式建議
    //       const fs::path& 避免拷貝。
    //
    //  Q3：path 跟「實際檔案」是兩回事嗎？
    //    A：對 — path 只是「字串」描述，不保證對應的東西真的存在。
    //       要確認存在才 fs::exists(p) 等查詢。
    //
    demo_practical_path_helpers();
    return 0;
}
