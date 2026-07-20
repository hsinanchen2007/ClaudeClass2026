// ============================================================================
// 課題 1：std::filesystem::path 是路徑語法，不等於檔案
// ============================================================================
//
// path 保存平台原生路徑表示，能拆 root/parent/filename/stem/extension；建立 path 不做 I/O，
// 不代表目標存在。`string()` 是 native-ish encoding；`generic_string()` 用 `/` 分隔，適合
// 顯示/portable metadata，但 Unicode encoding 仍需平台政策。
//
// extension 只看最後一段：archive.tar.gz 的 stem 是 archive.tar、extension 是 .gz。
// 隱藏檔 `.bashrc` 通常 stem=.bashrc、extension empty。不要用手刻 `/` 與 substr 解析。
// ============================================================================

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void basic_example()
{
    const fs::path path = fs::path("reports") / "2026" / "daily.log";
    assert(path.filename() == "daily.log");
    assert(path.stem() == "daily");
    assert(path.extension() == ".log");
    assert(path.parent_path().filename() == "2026");
    std::cout << "[基礎] generic path=" << path.generic_string() << '\n';
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 71. Simplify Path（簡化路徑）
// 題目：將 POSIX absolute path 的重複斜線、`.`、`..` 正規化；"/home//foo/" 回傳 "/home/foo"。
// 為何使用本章主題：fs::path::lexically_normal 純做 component 語法運算，不查磁碟；再補上題目不允許尾斜線的契約。
// 思路：1. 建立 path 並 lexical normalize；2. 除 root 外移除尾端 `/`；3. 空結果映射成 root。
// 複雜度：L 為路徑字元數；時間與結果空間 O(L)。
// 易錯點：這是 POSIX 題目但 std::filesystem::path 具平台語意；lexical normalize 不解析 symlink，也不是安全 sandbox。
// -----------------------------------------------------------------------------
std::string simplify_path(const std::string& input)
{
    std::string result = fs::path(input).lexically_normal().generic_string();
    while (result.size() > 1U && result.back() == '/') result.pop_back();
    return result.empty() ? "/" : result;
}

void leetcode_71_example()
{
    assert(simplify_path("/home/") == "/home");
    assert(simplify_path("/home//foo/") == "/home/foo");
    assert(simplify_path("/home/user/Documents/../Pictures") == "/home/user/Pictures");
    assert(simplify_path("/../") == "/");
    std::cout << "[LeetCode 71] trailing slash 與 dot segments 均符合題目契約\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】每日報表輸出路徑組合
// 情境：依部署 root 與日期建立 `<root>/reports/<date>.json`，root 可能有或沒有尾 separator。
// 為何使用本章主題：path::operator/ 依平台 component 規則組合，不需手刻 `/` 或猜現有尾字元。
// 設計：1. 接收 root path value；2. 接上 reports component；3. 將日期加 .json 後作為 filename。
// 成本：L 為結果路徑長度；字串與 path 組合時間、結果空間皆 O(L)。
// 上線注意：date 要驗格式與路徑分隔字元；若 child 可由外部控制，先拒 absolute/root-name 以免捨棄 root。
// -----------------------------------------------------------------------------
fs::path report_path(const fs::path& root, const std::string& date)
{
    return root / "reports" / (date + ".json");
}

void practical_example()
{
    assert(report_path("/srv/app", "2026-07-19") == "/srv/app/reports/2026-07-19.json");
    std::cout << "[實務] path composition avoids manual slash bugs\n";
}

int main()
{
    basic_example();
    leetcode_71_example();
    practical_example();
}

// 易錯與面試：path 是平台語法的結構，不是「永遠用 / 的 string」。operator/ 若右側是
// absolute path，組合語意可能捨棄前綴；接受不可信 child path 時要先拒 absolute/root-name。
// lexical normalization 只改文字，並不存取磁碟，也不能阻止 symlink traversal。安全邊界
// 需 canonical/weakly_canonical 後確認仍在 root，且仍要考慮 check/use 間的 TOCTOU。
//
// extension/stem 對 dotfiles、多重副檔名有既定語意，不能假設最後一個 dot 永遠是副檔名。
//
// API 與成本速記：
//   * filename/parent_path/stem/extension 是 path value 的語法拆解，不會發出 system call。
//   * operator/ 建立新 path，成本至少與要複製/串接的路徑字元數成正比，不是 O(1)。
//   * exists/status/canonical 才會觀察 filesystem；其結果可能在下一刻因其他程序而失效。
//   * path 自己擁有字串資料；由 temporary path 取得後長期保存的 iterator/reference 會懸空。
//
// 面試快問：`path("a/b")` 成功能否證明檔案存在？不能，constructor 只建立路徑值。
// 練習：測試 archive.tar.gz、.bashrc、name. 的 stem/extension。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '01_path.cpp' -o '/tmp/codex_cpp_C_Filesystem_01_path' && '/tmp/codex_cpp_C_Filesystem_01_path'
//
// === 預期輸出（節錄）===
// [實務] path composition avoids manual slash bugs
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
