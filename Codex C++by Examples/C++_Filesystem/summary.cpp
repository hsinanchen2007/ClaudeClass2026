// ============================================================================
// C++17 <filesystem> 總複習：path、狀態、巡覽、檔案發布與安全邊界
// ============================================================================
//
// 【本章地圖：對應 01～08】
//   path / path operations / exists+status / create+remove / directory_iterator /
//   recursive_directory_iterator / fstream 整合 / practical tools
//
// 【path 是語法值，不等於檔案已存在】
//   p / "child"             依平台規則串接 component，優於手寫 '/'
//   filename/stem/extension  純 lexical 查詢，不做 I/O
//   parent_path/root_path     拆解路徑；相對/絕對仍依平台語法
//   lexically_normal          純字串層處理 . 和 ..，不跟 symlink、不需存在
//   absolute                 產生 absolute path，但不保證存在/唯一
//   canonical                需要路徑存在，解析 symlink，可能 throw
//   weakly_canonical         對存在前綴 canonical，再處理不存在尾端
//   relative/proximate       相對於 base 計算，可能碰 filesystem
//
// 【status 與 symlink_status】
//   status(path)          通常跟隨 symlink，問 target 是什麼。
//   symlink_status(path)  問 link 本身是什麼。
//   exists/is_regular_file/is_directory/is_symlink 可收 path 或 file_status。
//   若同一路徑查多個性質，先取一次 status，減少 syscall 與 TOCTOU 窗口但不消除競態。
//
// 【建立/複製/刪除 API】
//   create_directory       只建一層；已存在時回 false（不一定是錯）。
//   create_directories     建完整 parent chain。
//   copy_file              單一 regular file，options 決定 skip/overwrite/update。
//   copy                   可遞迴，但 options/symlink policy 必須明訂。
//   rename                 同 filesystem 常為 atomic visibility；跨 filesystem 可能失敗。
//   remove                 一個 file 或空 dir，回是否有刪除。
//   remove_all             遞迴刪除並回數量；高風險，需 allowlist/marker/dry-run。
//
// 【iterator 選型與複雜度】
//   directory_iterator            一層；每 entry amortized O(1)，順序未指定。
//   recursive_directory_iterator  DFS-like 遞迴；可 disable_recursion_pending 跳目錄。
//   recursive options             skip_permission_denied、follow_directory_symlink 要慎選；
//                                 follow 可能形成 cycle。
//   directory_entry               可 cache 某些 status 資訊，但 filesystem 隨時可能變。
//   若輸出/測試需 deterministic，收集 path 後 sort；不要依賴 OS 枚舉順序。
//
// 【throwing vs error_code overload】
//   throwing overload：單一步驟失敗即中止最清楚，catch filesystem_error 可讀 path1/path2/code。
//   error_code overload：best-effort 掃描、destructor cleanup、需逐項繼續時使用；每次呼叫後檢查 ec。
//   不可把 error_code 忽略成成功；也不要同一段混兩種模型而遺漏錯誤。
//
// 【fstream 整合與發布政策】
//   filesystem 只管名稱/metadata；內容讀寫仍用 ifstream/ofstream。
//   open、write、flush/close 都可能失敗。temp + close-check + rename 可避免讀者看見半檔，
//   但不等於斷電 durability；要求 durable 時還需 OS fsync file 與 parent directory。
//   temp 必須與 destination 同目錄，才能避免跨 filesystem rename；名稱也必須唯一，
//   才不會讓兩個 writer 共用同一 `.new`。既有目的要明訂 reject 或 replace，不能靠
//   各平台碰巧的 rename 行為。標準 C++ 沒有可攜的「atomic no-replace」primitive；若
//   reject policy 還要抵抗惡意競態，需用 renameat2/linkat 等平台 API。
//
// 【安全與 lifetime】
//   - exists 後到 open 前檔案可被換掉（TOCTOU）；security boundary 用 OS descriptor API。
//   - lexical prefix check 無法阻止 symlink escape；canonical 也仍有檢查後被換掉的競態。
//   - current_path 是 process-global 狀態；library 不應偷偷修改，多執行緒更危險。
//   - iterator/reference 只在 iterator 狀態與目錄未被不相容修改時使用；先複製 path value。
//   - 跨平台不要假設 separator、case sensitivity、permissions、Unicode normalization 相同。
//
// 【面試快問快答】
//   Q: rename 原子就代表資料不會因斷電遺失嗎？ A: 否；atomic visibility != durable storage。
//   Q: canonical 與 lexically_normal 差在哪？ A: 前者讀真 FS 並解析 symlink；後者只改語法。
//   Q: 如何安全列出目錄？ A: 明訂 symlink/permission policy、收 error、需要穩定輸出就 sort。
// ============================================================================

/*
==============================================================================
【面試深挖：std::filesystem】

FS1｜`path / child` 與字串 `+=` 的差別？
答：operator/ 依 path 語意插入 separator/處理 absolute component；concat/+= 只是拼接
native path 字元。組目錄層級應用 /，組副檔名片段才可能 concat。

FS2｜先 `exists(path)` 再 open 是否安全？
答：有 TOCTOU race，兩步間檔案可被替換/刪除，安全邊界甚至可能遭 symlink attack。
真正操作並處理結果；涉及安全時用 OS-level descriptor-relative API 與適當 flags。

FS3｜exception overload 與 `error_code` overload 怎麼選？
答：例外適合失敗即中止目前高階操作；error_code 適合掃目錄時逐項降級、destructor/noexcept
路徑。不能忽略 ec；每次呼叫後依該 API 契約檢查，避免沿用舊錯誤。

FS4｜`status` 與 `symlink_status` 差在哪？
答：status 跟隨 symlink 看 target；symlink_status 看 link 本身。做刪除、權限或 sandbox
判斷時選錯會對錯誤物件操作，且檢查後仍有 race。

FS5｜`canonical` 與 `weakly_canonical`？
答：canonical 通常要求整條路徑存在並消除 dot/symlink；weakly_canonical 可處理不存在的
尾段。lexically_normal 只做文字正規化，不查 filesystem，也不能當安全 canonicalization。

FS6｜directory_iterator 順序有保證嗎？
答：沒有。測試或輸出需要 deterministic order 時先收集 path 再明確排序。
iteration 期間目錄發生變更，是否觀察到新舊項目也不可依賴。

FS7｜recursive_directory_iterator 遇到權限或 symlink cycle？
答：可用 directory_options::skip_permission_denied；預設不跟 directory symlink，
若啟用 follow 必須防 cycle。錯誤策略要決定「中止」或「報告後繼續」。

FS8｜`rename` 是否總是原子且可跨檔案系統？
答：跨 filesystem 常失敗；同 filesystem 的 atomicity 與 replace semantics 仍受平台影響。
可靠發布通常在同目錄寫 temp、fsync 必要資料、再 rename，並處理 Windows/POSIX 差異。

FS9｜`remove_all` 的最大風險？
答：遞迴刪除且回刪除數；路徑算錯會放大損害。要驗證 root/marker、拒絕空或根路徑、
處理 symlink policy，並先 dry-run 顯示 resolved target。

FS10｜`space_info.free` 與 `available` 為何不同？
答：free 是檔案系統全部空間，available 是一般呼叫者可用空間，可能扣除保留 blocks/quota。
應用判斷「我能否寫入」應看 available，並容許查詢失敗回特殊值。

FS11｜C++20 對 UTF-8 path 有何 breaking change？
答：char8_t 導致 u8string 回傳型別與舊 const char API 不再直接相容，u8path 也進入不同
版本狀態。path native encoding 受平台影響；跨平台程式要把 encoding conversion 當明確邊界。

FS12｜`file_size` 後配置同等 buffer 安全嗎？
答：檔案可在兩步間改變，且特殊檔案可能不支援 size。讀取 loop 必須以實際 read result
為準並設上限，不能把 metadata snapshot 當 immutable contract。
==============================================================================
*/

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

class TempDirectory {
public:
    TempDirectory()
        : path_(fs::temp_directory_path()
                / ("codex_fs_summary_"
                   + std::to_string(std::chrono::steady_clock::now()
                                        .time_since_epoch().count())))
    {
        if (!fs::create_directory(path_)) {
            throw std::runtime_error("cannot create temporary directory");
        }
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;

    ~TempDirectory()
    {
        std::error_code error;
        fs::remove_all(path_, error); // destructor 不丟例外
    }

    [[nodiscard]] const fs::path& path() const noexcept { return path_; }

private:
    fs::path path_;
};

void write_checked(const fs::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("open failed: " + path.string());
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    output.close();
    if (!output) throw std::runtime_error("write/close failed: " + path.string());
}

void basic_path_and_iteration_demo()
{
    TempDirectory temp;
    const fs::path nested = temp.path() / "logs" / "2026";
    // 不把必要副作用放進 assert；NDEBUG 移除 assert 時，目錄仍必須建立。
    const bool directories_created = fs::create_directories(nested);
    if (!directories_created) {
        throw std::runtime_error("fresh nested directory was not created");
    }
    write_checked(nested / "b.log", "B\n");
    write_checked(nested / "a.log", "A\n");

    assert((nested / "../2026/./a.log").lexically_normal() == nested / "a.log");
    assert(fs::is_regular_file(nested / "a.log"));

    std::vector<fs::path> names;
    for (const fs::directory_entry& entry : fs::directory_iterator(nested)) {
        if (entry.is_regular_file()) names.push_back(entry.path().filename());
    }
    std::sort(names.begin(), names.end());
    assert((names == std::vector<fs::path>{"a.log", "b.log"}));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 71. Simplify Path（簡化路徑）
// 題目：簡化 POSIX absolute path；"/a/./b/../../c/" 回傳 "/c"。
// 為何使用本章主題：這是 lexical stack 題，刻意不查 filesystem；不存在的 component 也必須依文字契約處理。
// 思路：1. 依 `/` 讀 components；2. `..` pop、`.`/空字串略過、名稱 push；3. 以單一 `/` 重組。
// 複雜度：N 為輸入字元數；時間 O(N)、額外 components 與結果空間 O(N)。
// 易錯點：先要求 absolute path；root 上的 `..` 不可再向上，substr 遇 npos 的長度語意也要正確。
// -----------------------------------------------------------------------------
std::string simplify_path(const std::string& path)
{
    if (path.empty() || path.front() != '/') {
        throw std::invalid_argument("absolute path required");
    }
    std::vector<std::string> components;
    std::size_t begin = 1U;
    while (begin <= path.size()) {
        const std::size_t end = path.find('/', begin);
        const std::string component = path.substr(begin, end - begin);
        if (component == "..") {
            if (!components.empty()) components.pop_back();
        } else if (!component.empty() && component != ".") {
            components.push_back(component);
        }
        if (end == std::string::npos) break;
        begin = end + 1U;
    }

    std::string result;
    for (const std::string& component : components) result += "/" + component;
    return result.empty() ? "/" : result;
}

void leetcode_demo()
{
    assert(simplify_path("/home/") == "/home");
    assert(simplify_path("/../") == "/");
    assert(simplify_path("/home//foo/") == "/home/foo");
    assert(simplify_path("/a/./b/../../c/") == "/c");
}

// -----------------------------------------------------------------------------
// 【日常實務範例】同目錄 temporary 的設定發布
// 情境：發布 config.txt 時支援拒絕既有檔或替換政策，失敗要自動清掉 temporary。
// 為何使用本章主題：path、create_directories、fstream close-check、RAII cleanup 與 rename 組成原子可見發布流程。
// 設計：1. 驗 parent 並選唯一 sibling temp；2. 寫完且 close 成功；3. 套 existing policy 後 rename，成功才 commit cleanup。
// 成本：B 為內容大小；寫入 O(B)、暫存空間 O(B)，其餘為多次 metadata/rename I/O。
// 上線注意：reject 的先查再 rename 有 TOCTOU；跨平台 replace 語意不同，atomic visibility 也不等於 fsync durability。
// -----------------------------------------------------------------------------
enum class ExistingDestinationPolicy {
    reject_existing,
    replace_existing
};

fs::path publication_parent(const fs::path& destination)
{
    if (destination.empty() || destination.filename().empty()) {
        throw std::invalid_argument("destination must name a file");
    }
    const fs::path parent = destination.parent_path();
    return parent.empty() ? fs::path{"."} : parent;
}

void ensure_directory(const fs::path& directory)
{
    std::error_code error;
    static_cast<void>(fs::create_directories(directory, error));
    if (error) throw fs::filesystem_error("create publication directory", directory, error);

    const bool is_directory = fs::is_directory(directory, error);
    if (error) throw fs::filesystem_error("inspect publication directory", directory, error);
    if (!is_directory) {
        throw fs::filesystem_error(
            "publication parent is not a directory", directory,
            std::make_error_code(std::errc::not_a_directory));
    }
}

fs::path unique_sibling_path(const fs::path& destination)
{
    static std::atomic<unsigned long long> sequence{0ULL};
    const fs::path parent = publication_parent(destination);
    const auto clock_tick = std::chrono::steady_clock::now().time_since_epoch().count();

    for (unsigned int attempt = 0U; attempt < 128U; ++attempt) {
        const unsigned long long ticket =
            sequence.fetch_add(1ULL, std::memory_order_relaxed);
        const std::string name = "." + destination.filename().string() + ".tmp."
                               + std::to_string(clock_tick) + "."
                               + std::to_string(ticket);
        const fs::path candidate = parent / name;
        std::error_code error;
        const bool already_exists = fs::exists(fs::symlink_status(candidate, error));
        if (error && error != std::errc::no_such_file_or_directory) {
            throw fs::filesystem_error("inspect temporary path", candidate, error);
        }
        if (!already_exists) return candidate;
    }
    throw std::runtime_error("cannot choose a unique sibling temporary path");
}

class RemoveUnlessCommitted {
public:
    explicit RemoveUnlessCommitted(fs::path path) : path_(std::move(path)) {}
    RemoveUnlessCommitted(const RemoveUnlessCommitted&) = delete;
    RemoveUnlessCommitted& operator=(const RemoveUnlessCommitted&) = delete;

    ~RemoveUnlessCommitted()
    {
        if (!committed_) {
            std::error_code error;
            static_cast<void>(fs::remove(path_, error));
        }
    }

    void commit() noexcept { committed_ = true; }

private:
    fs::path path_;
    bool committed_{false};
};

void publish_atomically(const fs::path& destination,
                        const std::string& content,
                        const ExistingDestinationPolicy policy)
{
    const fs::path parent = publication_parent(destination); // 空 parent 明確視為目前目錄。
    ensure_directory(parent);

    const fs::path temporary = unique_sibling_path(destination);
    RemoveUnlessCommitted cleanup(temporary);
    write_checked(temporary, content);

    if (policy == ExistingDestinationPolicy::reject_existing) {
        std::error_code error;
        const bool destination_exists = fs::exists(fs::symlink_status(destination, error));
        if (error && error != std::errc::no_such_file_or_directory) {
            throw fs::filesystem_error("inspect destination", destination, error);
        }
        if (destination_exists) {
            throw fs::filesystem_error(
                "destination already exists", destination,
                std::make_error_code(std::errc::file_exists));
        }
    }

    std::error_code error;
    fs::rename(temporary, destination, error);
    if (error) {
        // 不以 remove(destination)+rename 退讓：那會破壞原子可見性與舊檔保留保證。
        throw fs::filesystem_error("publish temporary file", temporary, destination, error);
    }
    cleanup.commit();
}

class CurrentPathGuard {
public:
    explicit CurrentPathGuard(const fs::path& next) : previous_(fs::current_path())
    {
        fs::current_path(next);
    }

    CurrentPathGuard(const CurrentPathGuard&) = delete;
    CurrentPathGuard& operator=(const CurrentPathGuard&) = delete;

    ~CurrentPathGuard()
    {
        if (active_) {
            std::error_code error;
            fs::current_path(previous_, error);
        }
    }

    void restore()
    {
        if (active_) {
            fs::current_path(previous_);
            active_ = false;
        }
    }

private:
    fs::path previous_;
    bool active_{true};
};

// -----------------------------------------------------------------------------
// 【日常實務範例】遞迴 regular file inventory
// 情境：掃描發布目錄，產生 relative generic path 到 file size 的穩定排序清單。
// 為何使用本章主題：recursive_directory_iterator 遞迴列舉，error_code 逐步檢查，std::map 讓結果按 path 決定性排序。
// 設計：1. 以 skip_permission_denied 建 iterator；2. regular file 取 relative path 與 size；3. 每次 increment/error 都驗證。
// 成本：E 為 entries；走訪與 metadata I/O O(E)，結果路徑空間 O(K*L)，map 插入 O(K log K)。
// 上線注意：skip_permission_denied 可能使清單不完整；file_size 是瞬間 snapshot，不是內容 hash 或後續存在保證。
// -----------------------------------------------------------------------------
std::map<std::string, std::uintmax_t> regular_file_inventory(const fs::path& root)
{
    std::map<std::string, std::uintmax_t> result; // map 讓輸出按 key 穩定排序
    std::error_code error;
    fs::recursive_directory_iterator iterator(
        root, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    if (error) throw fs::filesystem_error("open inventory", root, error);

    while (iterator != end) {
        const fs::directory_entry entry = *iterator;
        if (entry.is_regular_file(error)) {
            if (error) throw fs::filesystem_error("status", entry.path(), error);
            const auto size = entry.file_size(error);
            if (error) throw fs::filesystem_error("size", entry.path(), error);
            result.emplace(fs::relative(entry.path(), root).generic_string(), size);
        } else if (error) {
            throw fs::filesystem_error("status", entry.path(), error);
        }
        iterator.increment(error);
        if (error) throw fs::filesystem_error("iterate", root, error);
    }
    return result;
}

void practical_demo()
{
    TempDirectory temp;
    const fs::path config = temp.path() / "config.txt";

    // 以隔離的 temporary directory 當 cwd，實際覆蓋 destination.parent_path()==empty 的路徑。
    // current_path 是 process-global；正式 library 不應這樣切換，本測試是單執行緒且會還原。
    CurrentPathGuard current_path(temp.path());
    publish_atomically("config.txt", "threads=4\n",
                       ExistingDestinationPolicy::reject_existing);

    bool rejected_existing = false;
    try {
        publish_atomically("config.txt", "must-not-win\n",
                           ExistingDestinationPolicy::reject_existing);
    } catch (const fs::filesystem_error& error) {
        rejected_existing = error.code() == std::errc::file_exists;
    }
    if (!rejected_existing) {
        throw std::runtime_error("reject_existing policy did not reject the destination");
    }

    // reject 發生在 temp 寫完後；目錄仍只有舊目的檔，證明 failure cleanup 已執行。
    const auto after_rejection = regular_file_inventory(".");
    assert(after_rejection.size() == 1U);
    assert(after_rejection.at("config.txt") == 10U);

    publish_atomically("config.txt", "threads=8\n",
                       ExistingDestinationPolicy::replace_existing);
    current_path.restore();
    write_checked(temp.path() / "README.md", "offline\n");

    const auto inventory = regular_file_inventory(temp.path());
    assert(inventory.at("README.md") == 8U);
    assert(inventory.at("config.txt") == 10U);

    std::ifstream input(config);
    std::string line;
    const bool line_read = static_cast<bool>(std::getline(input, line));
    if (!line_read) throw std::runtime_error("cannot read published configuration");
    assert(line == "threads=8");
}

int main()
{
    basic_path_and_iteration_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "Filesystem summary: all checks passed\n";
}

// 【章末自測】寫安全原子更新流程：同目錄暫存、flush/close、rename、錯誤清理與權限策略。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'summary.cpp' -o '/tmp/codex_cpp_C_Filesystem_summary' && '/tmp/codex_cpp_C_Filesystem_summary'
//
// === 預期輸出（節錄）===
// Filesystem summary: all checks passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
