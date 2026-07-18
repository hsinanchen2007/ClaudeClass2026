// =============================================================================
//  02_path_ops.cpp  —  path 拆解、副檔名與正規化
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/path
//    - https://en.cppreference.com/w/cpp/filesystem/path/extension
//    - https://en.cppreference.com/w/cpp/filesystem/path/lexically_normal
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 路徑成員函式速查                                           │
//  └────────────────────────────────────────────────────────────┘
//
//  以 p = "/home/alice/code/main.cpp.bak" 為例：
//
//      p.parent_path()       "/home/alice/code"
//      p.filename()          "main.cpp.bak"
//      p.stem()              "main.cpp"   ← 移除「最右一個 .」之後的 ext
//      p.extension()         ".bak"        ← 「最右一個 .」開始的部分
//      p.is_absolute()       true
//      p.is_relative()       false
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 修改類                                                     │
//  └────────────────────────────────────────────────────────────┘
//
//      p.replace_extension(".log")       → "/home/alice/code/main.cpp.log"
//      p.replace_filename("other.cpp")   → "/home/alice/code/other.cpp"
//      p.remove_filename()               → "/home/alice/code/"
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 正規化 / 相對化                                            │
//  └────────────────────────────────────────────────────────────┘
//
//      p.lexically_normal()              把 "a/./b/../c" 變成 "a/c"（只看字串）
//      fs::canonical(p)                  正規化 + 解掉 symlink + 確認存在
//      fs::weakly_canonical(p)           同上但允許「不存在」
//      fs::relative(p, base)             相對 base 的路徑
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 本檔示範                                                   │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：拆解 path
//   * Demo 2：替換副檔名 / 檔名
//   * Demo 3：lexically_normal
//   * Demo 4：用副檔名做分流（C++ / Python / 圖片）
// =============================================================================

/*
補充筆記：std::path_ops
  - std::path_ops 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - filename、stem、extension 都只是路徑字串分解，不會碰檔案系統。
  - relative、absolute、canonical 的語意不同；canonical 需要路徑存在並可能丟例外。
*/
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具 1：批次重命名「.cpp → .cc」
//   工作中常見：要把一批檔案的副檔名「換掉」(不真的動檔，只生成新檔名)。
//   用 path.replace_extension 一個函式搞定，免去自己 split 字串。
// ─────────────────────────────────────────────────────────
static std::vector<fs::path> rename_extensions(
    const std::vector<fs::path>& src,
    const std::string& new_ext) {
    std::vector<fs::path> out;
    out.reserve(src.size());
    for (auto& p : src) {
        fs::path q = p;
        q.replace_extension(new_ext);                // ".cpp" → ".cc"
        out.push_back(q);
    }
    return out;
}

// ─────────────────────────────────────────────────────────
// 額外實用工具 2：依副檔名分桶
//   常見於資產整理腳本：把一堆檔案路徑依「副檔名」分到 map 桶裡。
// ─────────────────────────────────────────────────────────
static std::map<std::string, std::vector<fs::path>>
bucketize_by_extension(const std::vector<fs::path>& files) {
    std::map<std::string, std::vector<fs::path>> buckets;
    for (auto& f : files) {
        std::string ext = f.extension().string();
        if (ext.empty()) ext = "<no-ext>";
        buckets[ext].push_back(f);
    }
    return buckets;
}

static void demo_practical_rename_and_bucket() {
    std::cout << "[Practical] rename + bucketize\n";
    std::vector<fs::path> src{"main.cpp", "util.cpp", "feature.cpp"};
    auto renamed = rename_extensions(src, ".cc");
    for (size_t i = 0; i < src.size(); ++i)
        std::cout << "  " << src[i] << " → " << renamed[i] << '\n';

    std::vector<fs::path> assets{
        "logo.png", "main.cpp", "doc.md", "util.h", "icon.png", "Makefile"};
    auto buckets = bucketize_by_extension(assets);
    for (auto& [ext, list] : buckets) {
        std::cout << "  bucket " << ext << " :";
        for (auto& p : list) std::cout << ' ' << p.filename();
        std::cout << '\n';
    }
}

int main() {
    // ─────────────────────────────────────────────────────────
    // Demo 1：拆解
    // ─────────────────────────────────────────────────────────
    fs::path p{"/home/alice/code/main.cpp"};
    std::cout << "[Demo1] full        = " << p << '\n';
    std::cout << "[Demo1] parent_path = " << p.parent_path() << '\n';
    std::cout << "[Demo1] filename    = " << p.filename() << '\n';
    std::cout << "[Demo1] stem        = " << p.stem() << '\n';
    std::cout << "[Demo1] extension   = " << p.extension() << '\n';
    std::cout << "[Demo1] is_absolute = " << std::boolalpha << p.is_absolute() << '\n';

    // 多重副檔名：extension 只回最後一個
    fs::path q{"data.tar.gz"};
    std::cout << "[Demo1] data.tar.gz: stem=" << q.stem()
              << " ext=" << q.extension() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 2：替換
    // ─────────────────────────────────────────────────────────
    fs::path p2 = p;
    p2.replace_extension(".o");
    std::cout << "[Demo2] replace_ext   = " << p2 << '\n';

    fs::path p3 = p;
    p3.replace_filename("other.cpp");
    std::cout << "[Demo2] replace_name  = " << p3 << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：lexically_normal — 只「字串級」消除 . / ..
    // ─────────────────────────────────────────────────────────
    fs::path messy{"a/./b/../c/./d.txt"};
    std::cout << "[Demo3] before  = " << messy << '\n';
    std::cout << "[Demo3] normal  = " << messy.lexically_normal() << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 4：用 extension 做分流
    // ─────────────────────────────────────────────────────────
    std::vector<fs::path> files{
        "main.cpp", "util.h", "build.sh", "icon.png", "data.tar.gz", "Makefile"
    };
    for (auto& f : files) {
        std::string ext = f.extension().string();
        const char* tag;
        if      (ext == ".cpp" || ext == ".h" || ext == ".hpp") tag = "C++";
        else if (ext == ".png" || ext == ".jpg")                tag = "image";
        else if (ext == ".sh")                                  tag = "shell";
        else if (ext == ".gz" || ext == ".zip")                 tag = "archive";
        else                                                    tag = "other";
        std::cout << "[Demo4] " << f << " → " << tag << '\n';
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：lexically_normal 跟 canonical 有什麼差？
    //    A：lexically_normal 不碰真實檔案系統，純字串處理；canonical 會
    //       (a) 解析 symbolic link、(b) 相對 → 絕對、(c) 要求路徑存在。
    //       不需要訪問檔案系統就用 lexically_normal，速度快、無例外。
    //
    //  Q2：extension 對「.cpp.bak」回傳什麼？
    //    A：".bak"（最右一個 .）。要拿到 ".cpp.bak"，要自己用 stem() 拆兩次：
    //         while (p.has_extension()) {
    //             ext = p.extension().string() + ext;
    //             p = p.stem();
    //         }
    //
    //  Q3：對「/foo/bar/.」(結尾是斜線)，filename 是什麼？
    //    A：filename 在這種路徑下是 "."；POSIX 慣例如此。lexically_normal
    //       後變 "/foo/bar"。
    //
    demo_practical_rename_and_bucket();
    return 0;
}
