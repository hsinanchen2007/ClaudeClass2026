/*
 * <cstring>：C 字串與 raw byte 記憶體 API
 *
 * C 字串函式 strlen/strcmp/strchr/strstr 依賴 NUL 終止；memcpy/memmove/memcmp 則按
 * 明確 byte count。memcpy 的來源與目的不可重疊，重疊要用 memmove。strcpy/strcat
 * 不知道目的容量，現代 C++ 優先 std::string；只有 ABI/系統 interop 才用低階 API。
 */

#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    const char text[] = "hello";
    assert(std::strlen(text) == 5U);
    const char same_first[] = "abc";
    const char same_second[] = "abc";
    const char later[] = "abd";
    assert(std::strcmp(same_first, same_second) == 0);
    assert(std::strcmp(same_first, later) < 0);
    assert(std::strchr(text, 'l') == text + 2);

    std::array<char, 6U> copied{};
    std::memcpy(copied.data(), text, sizeof(text));
    assert(std::strcmp(copied.data(), "hello") == 0);
}

// LeetCode 28：以 strstr 示範 C API；只接受無內嵌 NUL 的 std::string。
int leetcode_str_str_c_api(const std::string& haystack, const std::string& needle) {
    const char* found = std::strstr(haystack.c_str(), needle.c_str());
    if (found == nullptr) return -1;
    return static_cast<int>(found - haystack.c_str());
}

// 實務：封包欄位固定 8 bytes，使用 memcpy；先驗容量，不把它當 C 字串。
std::array<std::byte, 8U> practical_encode_tag(const std::string& tag) {
    std::array<std::byte, 8U> output{};
    const std::size_t count = tag.size() < output.size() ? tag.size() : output.size();
    std::memcpy(output.data(), tag.data(), count);
    return output;
}

void practical_overlap_demo() {
    std::array<char, 7U> bytes{'a', 'b', 'c', 'd', 'e', 'f', '\0'};
    // 來源 [0,4) 與目的 [2,6) 重疊，必須 memmove，不能 memcpy。
    std::memmove(bytes.data() + 2, bytes.data(), 4U);
    assert(std::string(bytes.data()) == "ababcd");
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_str_str_c_api("sadbutsad", "but") == 3);
    assert(leetcode_str_str_c_api("abc", "x") == -1);
    const auto encoded = practical_encode_tag("ABCDEFGH-more");
    assert(static_cast<char>(encoded[0]) == 'A');
    assert(static_cast<char>(encoded[7]) == 'H');
    practical_overlap_demo();
    std::cout << "cstring: tests passed\n";
}

/*
 * 【陷阱與面試】
 * - strlen 是 O(n)，且越過缺少 NUL 的 buffer 讀取是 UB。
 * - strcmp 只保證結果負/零/正，不保證恰為 -1/0/1。
 * - memcmp 比 raw bytes，不適合含 padding 的 struct，也不是文字 locale 比較。
 * - strncpy 不是通用安全替代：截斷時可能不補 NUL，且會填滿剩餘空間。
 * - std::string 可含內嵌 NUL；交給 strstr/strlen 會在第一個 NUL 提前停止。
 *
 * 【練習】寫一個接收 `span<const byte>` 的 packet prefix 比較，取代 memcmp 裸指標介面。
 */
