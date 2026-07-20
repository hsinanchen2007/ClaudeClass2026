/*
 * std::string::assign：以新內容「整批取代」舊內容
 *
 * assign 與建構子有相似 overload：另一個 string、substring、C 字串、
 * pointer+count、count+char、iterator range、initializer_list。差別是物件已存在。
 * 回傳值為 *this，因此可以串接，但實務上分行通常更容易讀。
 *
 * 複雜度約 O(新字串長度)；若既有 capacity 足夠，實作可重用配置，但標準不保證
 * 一定不配置。assign 可能使原本指向字串的 iterator/reference/pointer 失效。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::string text = "old";
    text.assign(4U, 'x');
    assert(text == "xxxx");

    const std::string source = "prefix:value:suffix";
    text.assign(source, 7U, 5U);
    assert(text == "value");

    const std::vector<char> bytes{'A', 'P', 'I'};
    text.assign(bytes.begin(), bytes.end());
    assert(text == "API");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：原題要求原地反轉 vector<char>，不可另建同長度陣列；例如 hello 變成 olleh。
// 為何使用本章主題：本檔把容器教學改為 std::string，但反轉函式本身不呼叫 assign；
//       assign 的整批覆寫角色留給下方緩衝區案例，這裡只對照既有字串的就地修改。
// 思路：1. left 指向開頭、right 先設為 size；2. 先遞減 right；3. 兩端交換後向內縮。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 value 的字元數。
// 易錯點：原題簽名是 vector<char>；空字串時不可先算 size()-1，本實作用 right=size 避免下溢。
// -----------------------------------------------------------------------------
void leetcode_reverse_string(std::string& value) {
    std::size_t left = 0U;
    std::size_t right = value.size();
    while (left < right) {
        --right;
        if (left >= right) {
            break;
        }
        const char saved = value[left];
        value[left] = value[right];
        value[right] = saved;
        ++left;
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】重載 HTTP 請求本文緩衝區
// 情境：同一工作字串先含舊請求資料，下一筆收到 pointer 與 byte 長度後必須整批取代。
// 為何使用本章主題：assign(data, size) 表達「舊內容全部作廢」，可重用既有容量，且不像
//       assign(data) 依賴 NUL 結尾，因此能保存二進位本文中的內嵌 NUL。
// 設計：1. null pointer 只接受零長度並清空；2. 其他情況依明確 size 複製；3. 不保留來源指標。
// 成本：時間 O(S)、結果空間 O(S)，S 是本文 bytes；capacity 不足時另有配置成本。
// 上線注意：assert 在 release 會消失，正式 API 必須拒絕 nullptr 搭配非零 size、限制本文上限，
//       並假設 assign 後所有舊 view／iterator 已失效。
// -----------------------------------------------------------------------------
void practical_load_request_body(std::string& buffer, const char* data, const std::size_t size) {
    if (data == nullptr) {
        assert(size == 0U);
        buffer.clear();
        return;
    }
    buffer.assign(data, size);
}

}  // namespace

int main() {
    basic_demo();

    std::string word = "hello";
    leetcode_reverse_string(word);
    assert(word == "olleh");

    std::string request = "stale data";
    const char payload[] = {'a', '\0', 'b'};
    practical_load_request_body(request, payload, sizeof(payload));
    assert(request.size() == 3U && request[1] == '\0');

    std::cout << "assign: tests passed\n";
}

/*
 * 【易錯】assign(ptr) 要求 ptr 指向有效 NUL 結尾字串；二進位資料請用 assign(ptr,n)。
 * 【面試】assign 與 operator= 怎麼選？單一完整來源用 = 最自然；要 substring、重複
 * 字元或 iterator 範圍時 assign 的意圖較精準。
 * 【練習】用 assign(source, pos, count) 取出 URL 的 host，並處理 pos 超界例外。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'assign.cpp' -o '/tmp/codex_cpp_C_String_assign' && '/tmp/codex_cpp_C_String_assign'
//
// === 預期輸出（節錄）===
// assign: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
