/*
 * data()：取得連續儲存區；C++17 起非 const string 可取得 char*
 *
 * const string::data() 與 c_str() 都提供唯讀 NUL 結尾資料。非 const data() 可讓 C API
 * 在既有 size 範圍內寫入；不可寫超過 size，也不可改寫 data()[size()] 的終止 NUL。
 * 若 API 需要更大 buffer，先 resize，再傳 data()，最後依實際長度縮回。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "abcd";
    char* bytes = text.data();
    bytes[0] = 'A';
    assert(text == "Abcd");
    assert(text.data()[text.size()] == '\0');
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：原地反轉字元序列，只能使用常數額外空間；例如 stressed 變成 desserts。
// 為何使用本章主題：C++17 的 non-const data() 提供首元素 pointer，與 data()+size() 組成
//       合法半開區間交給 std::reverse；本檔把原題 vector<char> 改成 string 展示連續儲存。
// 思路：1. 取得可寫 data pointer；2. 以 size 算尾後位置；3. 標準演算法原地交換兩端。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：尾端必須是 data()+size()，不可把 capacity 當元素數；原題容器簽名與本教學版不同。
// -----------------------------------------------------------------------------
void leetcode_reverse_in_place(std::string& text) {
    std::reverse(text.data(), text.data() + static_cast<std::ptrdiff_t>(text.size()));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】C API 主機名稱讀取
// 情境：舊 API 由 caller 提供可寫 buffer，成功時寫入 NUL 結尾 hostname 並回傳不含 NUL 的長度。
// 為何使用本章主題：先建立 64 個實際字元再傳 data()，可讓 C API 合法寫入；相較 reserve，
//       resize/建構才真正擴大 [0,size()) 可寫範圍。
// 設計：1. 建立固定大小 NUL buffer；2. API 驗證容量後寫入；3. 失敗回空；4. 成功 resize 到實際長度。
// 成本：本例最多處理 64 bytes，視為 O(1)；一般容量 C 時初始化與縮整體為 O(C) 空間與時間。
// 上線注意：要區分空 hostname 與容量不足，驗證 API 回傳長度不超過 buffer，且不能在 resize 後
//       繼續使用先前取得的 data pointer。
// -----------------------------------------------------------------------------
std::size_t fake_read_hostname(char* output, const std::size_t capacity) {
    constexpr char hostname[] = "build-node";
    constexpr std::size_t length = sizeof(hostname) - 1U;
    if (capacity < length + 1U) {
        return 0U;
    }
    std::copy_n(hostname, length + 1U, output);
    return length;
}

std::string practical_read_hostname() {
    std::string buffer(64U, '\0');
    const std::size_t written = fake_read_hostname(buffer.data(), buffer.size());
    if (written == 0U) {
        return {};
    }
    buffer.resize(written);
    return buffer;
}

}  // namespace

int main() {
    basic_demo();
    std::string word = "stressed";
    leetcode_reverse_in_place(word);
    assert(word == "desserts");
    assert(practical_read_hostname() == "build-node");
    std::cout << "data: tests passed\n";
}

/*
 * 【陷阱】reserve(100) 只改 capacity，不讓 data()[0..99] 成為可寫元素；必須 resize。
 * 【面試】為何不能讓 C API 更新 string 的 size？C API 不知道 string 物件；呼叫後由
 * C++ 端依回傳長度 resize。
 * 【練習】讓 fake API 在空間不足時回報所需容量，實作「查大小→配置→再呼叫」。
 */

/*
 * 【面試速查】non-const data 自 C++17 可寫，但只限現有 [0,size()) 元素；若 C API
 * 會改長度，必須由回傳值同步 resize。任何可能重配的 string 操作都使舊 data pointer
 * 失效。若 API 只讀且要求 NUL，c_str 的意圖通常比 data 更清楚。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'data.cpp' -o '/tmp/codex_cpp_C_String_data' && '/tmp/codex_cpp_C_String_data'
//
// === 預期輸出（節錄）===
// data: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
