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

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】path 拆解、正規化與拼接
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. absolute、canonical、weakly_canonical、relative、lexically_normal 差在哪？
//     答：關鍵是三個維度——是否需要路徑存在、是否解析 symlink、是否移除 . 與 ..：
//     absolute(p) 不需存在、不解析 symlink、也不移除 ..（它只是在前面接上 current_path，
//     所以 absolute("a/../b") 仍然含有 ..，這點常被誤會）；canonical(p) 需要路徑存在
//     （不存在會拋例外）、會解析 symlink、會移除 . 與 ..；weakly_canonical(p) 不需存在，
//     對存在的前綴部分會解析；relative(p, base) 與 lexically_normal() 都是純字面計算，
//     完全不碰檔案系統。
//     追問：什麼時候該用 weakly_canonical？（要正規化一個「即將建立、目前還不存在」的
//     路徑）為什麼 relative() 可能給出錯誤結果？（它不解析 symlink，若路徑中有 symlink，
//     字面上的 .. 與實際位置不一致）
//
// 🔥 Q2. operator/ 和 operator+= / concat() 差在哪？
//     答：operator/=（append）會在需要時插入分隔符：path("a") / "b" 得到 "a/b"；
//     operator+= 與 concat() 是純字串串接、不插分隔符：path("a") += "b" 得到 "ab"。
//     加副檔名時該用後者（p += ".txt"），改副檔名則用 p.replace_extension(".log")。
//
// ⚠️ 陷阱. canonical 和 lexically_normal 都能「正規化路徑」，可以互換嗎？
//     答：不行，兩者的代價與風險完全不同。canonical 會實際碰檔案系統——有 I/O 成本、
//     路徑不存在會拋例外、而且結果只反映「查詢當下」的狀態，隨後檔案可能就被改掉了
//     （TOCTOU）。lexically_normal 是純字串運算，不碰磁碟、不會失敗，但它不知道
//     symlink 的存在，所以 a/symlink/.. 的字面化簡結果可能與實際位置不同。
//     為什麼會錯：把兩者都當成「把路徑整理乾淨」的同義工具，忽略一個查磁碟、一個不查。
// ═══════════════════════════════════════════════════════════════════════════

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

// 編譯: g++ -std=c++20 -Wall -Wextra 02_path_ops.cpp -o 02_path_ops

// === 預期輸出 (節錄) ===
// [Demo1] full        = "/home/alice/code/main.cpp"
// [Demo1] parent_path = "/home/alice/code"
// [Demo1] filename    = "main.cpp"
// [Demo1] stem        = "main"
// [Demo1] extension   = ".cpp"
// [Demo1] is_absolute = true
// [Demo1] data.tar.gz: stem="data.tar" ext=".gz"
// [Demo2] replace_ext   = "/home/alice/code/main.o"
// [Demo2] replace_name  = "/home/alice/code/other.cpp"
// [Demo3] before  = "a/./b/../c/./d.txt"
// [Demo3] normal  = "a/c/d.txt"
// [Demo4] "main.cpp" → C++
// [Demo4] "util.h" → C++
// [Demo4] "build.sh" → shell
// [Demo4] "icon.png" → image
// [Demo4] "data.tar.gz" → archive
// [Demo4] "Makefile" → other
// [Practical] rename + bucketize
//   "main.cpp" → "main.cc"
//   "util.cpp" → "util.cc"
// …（後略，完整輸出共 26 行）
