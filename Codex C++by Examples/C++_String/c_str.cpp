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

// LeetCode 709（To Lower Case）：實作限制在 ASCII，避免把 char 負值交給 cctype。
std::string leetcode_to_ascii_lower(std::string text) {
    for (char& ch : text) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return text;
}

// 實務：模擬只讀 legacy API。指標只在呼叫期間使用，不保存到全域。
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
