// ============================================================================
// 課題 2：Path operations - append、replace、lexical relation
// ============================================================================
//
// `base / child` 若 child 是 absolute，通常結果改以 child 為主；不可把不可信 absolute
// input 直接接到 sandbox root，否則可能逃逸。replace_filename/replace_extension 修改 path
// object，不動磁碟。lexically_relative/proximate 只做字串路徑計算，不 resolve symlink。
//
// absolute() 依 current working directory；canonical() 要全部存在並解析 symlink；
// weakly_canonical() 可容許尾端不存在。安全 sandbox 還需處理 symlink race，單純 lexical
// `..` 檢查不是完整 security boundary。
// ============================================================================

#include <cassert>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

void basic_example()
{
    fs::path output = fs::path("build") / "artifact.tmp";
    output.replace_extension(".bin");
    assert(output == fs::path("build/artifact.bin"));
    output.replace_filename("model.onnx");
    assert(output == fs::path("build/model.onnx"));
    assert(output.lexically_relative("build") == "model.onnx");
    std::cout << "[基礎] replace operations only change path value\n";
}

// LeetCode 71：標準庫可完成 lexical normalize，但題目另要求刪除非 root 的 trailing `/`。
std::string simplify_path(const std::string& input)
{
    std::string result = fs::path(input).lexically_normal().generic_string();
    while (result.size() > 1U && result.back() == '/') result.pop_back();
    return result.empty() ? "/" : result;
}

// 同一觀念的實務延伸：限制結果必須留在 lexical sandbox。先拒 absolute，再 normalize，
// 並拒絕第一個 component 是 ..；這仍不是抵抗 symlink race 的完整 security sandbox。
fs::path lexical_sandbox_path(const fs::path& root, const fs::path& user_relative)
{
    if (user_relative.is_absolute()) throw std::invalid_argument("absolute input rejected");
    const fs::path normalized = user_relative.lexically_normal();
    if (!normalized.empty() && *normalized.begin() == "..") {
        throw std::invalid_argument("parent traversal rejected");
    }
    return root / normalized;
}

void leetcode_71_example()
{
    assert(simplify_path("/home/") == "/home");
    assert(simplify_path("/../") == "/");
    assert(simplify_path("/a/./b/../../c/") == "/c");
    assert(lexical_sandbox_path("/srv/data", "a/../b.txt") == "/srv/data/b.txt");
    bool rejected = false;
    try { (void)lexical_sandbox_path("/srv/data", "../../etc/passwd"); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    std::cout << "[LeetCode 71] canonical path outputs match official contract\n";
}

// 實務：同一 input 產生 sidecar checksum path。
fs::path checksum_path(fs::path artifact)
{
    artifact += ".sha256";
    return artifact;
}

void practical_example()
{
    assert(checksum_path("model.bin") == "model.bin.sha256");
    std::cout << "[實務] artifact sidecar=model.bin.sha256\n";
}

int main()
{
    basic_example();
    leetcode_71_example();
    practical_example();
}

// 成本與生命週期：replace/lexically_normal 只處理 path 內容，成本約隨路徑長度或 component
// 數量成長；canonical/weakly_canonical 需要 filesystem 查詢，成本取決於磁碟與 component。
// 本例回傳 fs::path value，結果自行擁有資料；若改回傳 local path 的 reference 就會懸空。
// 面試快問：lexically_normal 後確認沒有 `..` 是否足以當 security sandbox？不夠，symlink
// 及 check-then-use race 仍可使實際解析位置逃出 root。
// 易錯：右側 absolute path 的 operator/ 語意可能捨棄 root；接外部輸入前先驗 is_absolute。
// 練習：比較 `/`、`concat`/`+=`；前者加入 path separator，後者直接串字元。
// 複雜度：lexical path 操作約隨 component/字元數線性成長；真正 canonical 查詢另受 I/O 影響。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_path_ops.cpp' -o '/tmp/codex_cpp_C_Filesystem_02_path_ops' && '/tmp/codex_cpp_C_Filesystem_02_path_ops'
//
// === 預期輸出（節錄）===
// [實務] artifact sidecar=model.bin.sha256
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
