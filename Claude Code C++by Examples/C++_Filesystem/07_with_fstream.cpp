// =============================================================================
//  07_with_fstream.cpp  —  filesystem + fstream 配合
// =============================================================================
//  參考：
//    - https://en.cppreference.com/w/cpp/filesystem/path
//    - https://en.cppreference.com/w/cpp/io/basic_fstream
//    - https://en.cppreference.com/w/cpp/filesystem/file_size
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 一、fstream 直接吃 path                                    │
//  └────────────────────────────────────────────────────────────┘
//
//  C++17 起 fstream 的建構子接受 std::filesystem::path —— 不必再 .string()：
//
//      fs::path p = workDir / "log.txt";
//      std::ofstream out{p};
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 二、常見組合招式                                           │
//  └────────────────────────────────────────────────────────────┘
//
//   1. 確保「父目錄存在」再寫檔（避免「open 失敗」）
//          fs::create_directories(p.parent_path());
//          std::ofstream out{p};
//
//   2. 寫到「臨時檔」之後 rename 進正式位置（atomic-ish 寫入）
//          auto tmp = p; tmp += ".tmp";
//          { std::ofstream out{tmp}; out << ...; }
//          fs::rename(tmp, p);
//
//   3. 把整檔讀成 std::string
//          std::ifstream in{p, std::ios::binary};
//          std::ostringstream buf; buf << in.rdbuf();
//          std::string content = buf.str();
//
//      或用 file_size 預先 reserve：
//          auto sz = fs::file_size(p);
//          std::string content(sz, '\0');
//          std::ifstream in{p, std::ios::binary};
//          in.read(content.data(), sz);
//
//  ┌────────────────────────────────────────────────────────────┐
//  │ 三、本檔示範                                               │
//  └────────────────────────────────────────────────────────────┘
//
//   * Demo 1：建立深層目錄並寫檔
//   * Demo 2：atomic-style write（tmp + rename）
//   * Demo 3：把檔案讀成 string
// =============================================================================

/*
補充筆記：std::with_fstream
  - std::with_fstream 使用 std::filesystem；path 只代表路徑文字與平台規則，不保證檔案真的存在。
  - exists/status/is_regular_file/is_directory 會查檔案系統狀態，可能因權限、競態或路徑不存在失敗。
  - 檔案系統操作有 TOCTOU 問題：先檢查 exists 再操作之間，檔案可能已被別的程式改掉。
  - remove/remove_all/create_directory/rename/copy 都可能丟 filesystem_error；工作程式要決定使用例外版或 error_code 版。
  - directory_iterator 不保證順序；若輸出需要穩定順序，收集後自行排序。
  - path 的字元編碼和平台有關；跨平台程式要避免假設 Windows 與 Unix 路徑分隔和編碼完全相同。
  - filesystem::path 可直接交給 fstream 開檔，避免手動轉字串造成平台編碼問題。
  - 開檔失敗時 fstream 設定 failbit；filesystem 路徑正確不代表檔案可讀寫。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】filesystem 與 fstream 配合
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. C++17 起 fstream 可以直接吃 path 嗎？為什麼這件事很重要？
//     答：可以——fstream 家族的建構子與 open() 從 C++17 起接受 const fs::path&。
//     這很重要是因為在 C++17 之前只能寫 ifs.open(p.string())，而 .string() 在 Windows
//     上會走本地碼頁轉換，遇到非 ASCII 檔名就壞掉。直接傳 path 讓實作使用平台的原生
//     字元型別（Windows 上是 wchar_t），這正是那組 path 多載存在的主要理由。
//     追問：MSVC 有 ifstream(const wchar_t*) 擴充，為什麼不夠？（不可攜；傳 path 才是
//     標準解）
//
// 🔥 Q2. 要讀一個檔案，該先 fs::exists 檢查嗎？
//     答：不該。直接開檔並檢查結果就好：std::ifstream ifs(p); if (!ifs) { ... }。
//     先檢查再開不僅多一次 syscall，還引入 TOCTOU 競態——檢查通過之後檔案仍可能被刪除
//     或被 symlink 取代。而且 exists() 回 true 也不代表你開得起來（可能沒有讀取權限），
//     所以那個檢查連「省下錯誤處理」都做不到。
//
// ⚠️ 陷阱. 開檔失敗時，怎麼知道失敗的原因？
//     答：fstream 只會設定 failbit，本身不提供錯誤原因。要區分「檔案不存在」「權限
//     不足」「是個目錄」等情況，得另外用 <filesystem> 的 error_code 版 API 查詢，或在
//     POSIX 上檢視 errno。所以實務上的順序是「先直接開、失敗之後才去查原因」——而不是
//     反過來先查再開。
//     為什麼會錯：期待 stream 像 filesystem API 一樣帶有 error_code，但它的錯誤模型
//     只有幾個狀態位元。
// ═══════════════════════════════════════════════════════════════════════════

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 額外實用工具：read_lines (把檔案讀成 vector<string>)
//   工作中超常見：要把設定檔/日誌一行一行讀進來處理。
//   把 fs::path 與 fstream 接好，加上空檔保護。
// ─────────────────────────────────────────────────────────
static std::vector<std::string> read_lines(const fs::path& p) {
    std::vector<std::string> out;
    std::ifstream in{p};
    if (!in) return out;                              // 開不起來就回空
    std::string line;
    while (std::getline(in, line)) out.push_back(line);
    return out;
}

static void demo_practical_read_lines() {
    std::cout << "[Practical] read_lines\n";
    fs::path tmp{"tmp_read_lines.txt"};
    {
        std::ofstream out{tmp};
        out << "host=127.0.0.1\n";
        out << "port=8080\n";
        out << "app=demo\n";
    }
    auto lines = read_lines(tmp);
    std::cout << "  讀到 " << lines.size() << " 行：\n";
    for (auto& l : lines) std::cout << "    [" << l << "]\n";
    fs::remove(tmp);
}

int main() {
    const fs::path root = "tmp_fs_dir";
    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // Demo 1：建立深層目錄並寫檔
    // ─────────────────────────────────────────────────────────
    fs::path log = root / "logs/app/2026/05/05.log";
    fs::create_directories(log.parent_path());
    {
        std::ofstream out{log};
        out << "started at noon\n";
        out << "request handled\n";
    }
    std::cout << "[Demo1] wrote " << log << " (size="
              << fs::file_size(log) << " bytes)\n";

    // ─────────────────────────────────────────────────────────
    // Demo 2：atomic-style write — 寫到 tmp 再 rename
    //   這樣中途 crash 不會留下「半寫」的檔
    // ─────────────────────────────────────────────────────────
    fs::path target = root / "data.json";
    fs::path tmp = target;
    tmp += ".tmp";

    {
        std::ofstream out{tmp};
        out << "{ \"version\": 1, \"items\": [] }\n";
    }
    fs::rename(tmp, target);
    std::cout << "[Demo2] atomic-write " << target << '\n';

    // ─────────────────────────────────────────────────────────
    // Demo 3：把整檔讀成 string（兩種寫法）
    // ─────────────────────────────────────────────────────────

    // 寫法 A：rdbuf
    {
        std::ifstream in{target};
        std::ostringstream buf;
        buf << in.rdbuf();
        std::string content = buf.str();
        std::cout << "[Demo3-A] (rdbuf) size=" << content.size()
                  << " preview: " << content.substr(0, 30) << "...\n";
    }

    // 寫法 B：file_size + reserve + read（適合大檔，省一次 buffer）
    {
        auto sz = fs::file_size(target);
        std::string content(sz, '\0');
        std::ifstream in{target, std::ios::binary};
        in.read(content.data(), static_cast<std::streamsize>(sz));
        std::cout << "[Demo3-B] (read) size=" << content.size() << '\n';
    }

    fs::remove_all(root);

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：atomic-write 真的 atomic 嗎？
    //    A：rename 在「同一檔案系統」內是 atomic（POSIX 保證）。但 OS
    //       buffer 與 disk fsync 是另一層 — 要絕對落盤需 fsync(fd) 之
    //       後再 rename。要做 production-grade atomic file write 通常
    //       配 fsync 一起。
    //
    //  Q2：fs::file_size 對「正在寫入的檔」可靠嗎？
    //    A：拿到的是「呼叫當下 OS 給的數值」 — 對方還在寫的話之後會變。
    //       atomic 讀取要靠檔案系統屬性（鎖、版本控制）保證。
    //
    //  Q3：寫入大檔最佳姿勢？
    //    A：(1) 開 binary 模式
    //       (2) 用 write(buf, n) 而非 << 一個 char 一個 char
    //       (3) 對「一次性產生」的大檔，先建臨時 + rename
    //       (4) 若要極限速度可考慮 mmap (POSIX) — 但跨平台複雜
    //
    demo_practical_read_lines();
    return 0;
}
