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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 28. Find the Index of the First Occurrence in a String（尋找首次出現位置）
// 題目：回傳 needle 在 haystack 的第一個起始索引，找不到回 -1；例如 "sadbutsad"、"but" 回 3。
// 為何使用本章主題：本版刻意用 C 的 strstr 示範 interop，再以指標差換成索引；這是教學改寫，
//       一般 C++ 解答優先使用 string::find，因其能正確處理明確長度與內嵌 NUL。
// 思路：1. 取得兩個 NUL 結尾指標；2. strstr 搜尋；3. null 回 -1，否則以 found-begin 算位置。
// 複雜度：直觀最壞時間 O(N*K)、額外空間 O(1)，N、K 是兩個 C 字串在首個 NUL 前的長度。
// 易錯點：含內嵌 NUL 時程式行為不符合 std::string 全長；指標差轉 int 前也要驗範圍。
// -----------------------------------------------------------------------------
int leetcode_str_str_c_api(const std::string& haystack, const std::string& needle) {
    const char* found = std::strstr(haystack.c_str(), needle.c_str());
    if (found == nullptr) return -1;
    return static_cast<int>(found - haystack.c_str());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】八位元封包標籤編碼
// 情境：wire record 有固定 8-byte tag 欄位，來源可能較短或較長，結果按 bytes 傳輸而非 C 字串。
// 為何使用本章主題：memcpy 對已驗證的連續範圍做低階 byte 複製，不掃描 NUL；相較 strcpy，
//       它能處理固定寬度且不會假設目的地有終止字元。
// 設計：1. 零初始化 8-byte array；2. 取來源與欄寬較小值；3. memcpy 精確複製該數量。
// 成本：固定欄寬時時間與空間 O(1)；一般寬度 W 的複製成本 O(min(N,W))。
// 上線注意：要明定過長值是錯誤還是截斷；解碼端按固定寬度處理，不能把 output 當 NUL 字串。
// -----------------------------------------------------------------------------
std::array<std::byte, 8U> practical_encode_tag(const std::string& tag) {
    std::array<std::byte, 8U> output{};
    const std::size_t count = tag.size() < output.size() ? tag.size() : output.size();
    std::memcpy(output.data(), tag.data(), count);
    return output;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】重疊 buffer 區段右移
// 情境：同一個 char array 內要把前四 bytes 搬到位置 2，來源 [0,4) 與目的 [2,6) 彼此重疊。
// 為何使用本章主題：memmove 明確允許來源與目的重疊；memcpy 在此情境前置條件不成立，
//       即使某次測試看似成功也屬未定義行為。
// 設計：1. 保留足夠 array 容量；2. 指定目的起點與來源起點；3. 搬移恰好四 bytes 後驗證內容。
// 成本：時間 O(K)、額外空間由實作決定但語意上 O(K) 搬移，K 是搬移 byte 數。
// 上線注意：呼叫前仍須驗證兩個範圍都在同一有效配置內；memmove 不懂物件生命週期或文字編碼。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'cstring_functions.cpp' -o '/tmp/codex_cpp_C_String_cstring_functions' && '/tmp/codex_cpp_C_String_cstring_functions'
//
// === 預期輸出（節錄）===
// cstring: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
