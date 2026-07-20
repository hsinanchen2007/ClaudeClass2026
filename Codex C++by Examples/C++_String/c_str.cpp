/*
 * c_str()：與要求 NUL 結尾 const char* 的 C API 溝通
 *
 * 回傳指標指向 string 內部連續儲存，保證 data()[size()] == '\0'。取得指標是 O(1)，
 * 不會配置副本。指標只在字串未做可能重新配置/修改的操作期間有效；string 銷毀後
 * 也立即失效。C API 若要修改 buffer，不能把 c_str() 強制 const_cast。
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>

namespace {

std::size_t legacy_length(const char* text) {
    assert(text != nullptr);
    return std::strlen(text);
}

void basic_demo() {
    const std::string value = "hello";
    const char* pointer = value.c_str();
    assert(legacy_length(pointer) == value.size());
    assert(pointer[value.size()] == '\0');
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 709. To Lower Case（轉成小寫）
// 題目：輸入含英文字母的字串，將大寫 ASCII 字母改成小寫；例如 "HeLLo" 得到 "hello"。
// 為何使用本章主題：題解直接走訪 owning string，並不需要 c_str()；這是刻意對照，說明純
//       C++ 轉換不應為了使用 C 指標而繞路，c_str() 真正角色在下方 legacy API 邊界。
// 思路：1. 複製輸入；2. 掃描每個 byte；3. 只對 'A'..'Z' 做固定 ASCII 位移。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 text 長度，空間來自按值回傳的副本。
// 易錯點：本版只承諾 ASCII，不是 Unicode case folding；不要把 c_str() const_cast 後修改。
// -----------------------------------------------------------------------------
std::string leetcode_to_ascii_lower(std::string text) {
    for (char& ch : text) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return text;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】舊式 C 設定鍵前綴檢查
// 情境：既有函式只接受 NUL 結尾 const char*，新程式要判斷 std::string 是否以 "cfg." 開頭。
// 為何使用本章主題：c_str() O(1) 提供符合 C ABI 的唯讀指標，不需配置第二份 NUL 結尾 buffer。
// 設計：1. wrapper 取得 value.c_str()；2. legacy helper 量測固定 prefix；3. 只在呼叫期間比較。
// 成本：c_str() O(1)，strncmp 最多 O(P)，P 是 prefix 長度；額外空間 O(1)。
// 上線注意：legacy helper 若保存指標，owner 必須持續存活且不可修改；輸入含內嵌 NUL 時
//       C 與 C++ 看到的內容不同，安全敏感設定應先拒絕。
// -----------------------------------------------------------------------------
bool legacy_has_prefix(const char* text, const char* prefix) {
    assert(text != nullptr && prefix != nullptr);
    return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

bool practical_is_config_key(const std::string& value) {
    return legacy_has_prefix(value.c_str(), "cfg.");
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_to_ascii_lower("HeLLo123") == "hello123");
    assert(practical_is_config_key("cfg.timeout"));
    assert(!practical_is_config_key("user.name"));
    std::cout << "c_str: tests passed\n";
}

/*
 * 【陷阱】內嵌 '\0' 之後的內容仍在 string，C API 卻通常在第一個 '\0' 停止。
 * 【面試】c_str() 指標能保存多久？最多到字串下一次可能失效指標的非 const 操作或銷毀。
 * 【練習】呼叫 std::fopen(path.c_str(), "rb") 前，先說明路徑內嵌 NUL 的安全風險。
 */

/*
 * 【C interop 清單】
 * 1. API 是否只在呼叫期間讀取？若保存 pointer，owner 必須活得更久且不修改。
 * 2. API 是否會寫入？會寫就不能傳 c_str；改用 resize+data 或獨立 buffer。
 * 3. API 是否接受 length？有 length 優先傳 data()+size，避免內嵌 NUL 截斷。
 * 4. 修改 string 前，不要保留先前取得的 pointer。
 * 5. 安全敏感輸入應拒絕內嵌 NUL，避免 C/C++ 看到不同內容。
 * 【面試題】c_str 與 const data 在 C++17 後用途相近；c_str 更明確表示呼叫端需要
 * C-style NUL termination。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'c_str.cpp' -o '/tmp/codex_cpp_C_String_c_str' && '/tmp/codex_cpp_C_String_c_str'
//
// === 預期輸出（節錄）===
// c_str: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
