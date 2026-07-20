/*
 * std::string::replace：以新內容替換指定區段
 *
 * `replace(pos,count,replacement)` 可同時刪除 count 個舊字元並插入新字元；也有
 * iterator range、substring、pointer+count、count+char overload。最壞 O(size+new_count)，
 * pos 超界丟 out_of_range。替換可能重配並使所有 iterator/pointer 失效。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "Hello NAME";
    text.replace(6U, 4U, "Ada");
    assert(text == "Hello Ada");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1108. Defanging an IP Address（IP 位址去敏化）
// 題目：將合法 IPv4 中每個點換成 `[.]`；例如 255.100.50.0 得 255[.]100[.]50[.]0。
// 為何使用本章主題：replace(position,1,"[.]") 一次表達刪一字元、插三字元；修改後游標
//       跳過新片段，避免再次匹配剛插入內容中的點。
// 思路：1. 從 position 0 找點；2. 替換該一字元；3. 游標加 3；4. 到 npos 為止。
// 複雜度：IPv4 固定三次替換時時間 O(N)、額外空間 O(N)；一般 P 個點的中段搬移可達 O(P*N)。
// 易錯點：替換後不能只前進 1；一般輸入應先驗 IPv4，且每次 replace 可能使舊 iterator 失效。
// -----------------------------------------------------------------------------
std::string leetcode_defang_ip(std::string address) {
    std::size_t position = 0U;
    while ((position = address.find('.', position)) != std::string::npos) {
        address.replace(position, 1U, "[.]");
        position += 3U;  // 跳過剛插入的內容，避免再次找到其中的點。
    }
    return address;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】非遞迴模板 token 替換
// 情境：把文字中的每個 `{{name}}` 替換成值；新插入 value 即使含相同 token 也不再遞迴展開。
// 為何使用本章主題：replace 可在找到位置後一次調整長度與內容；相較 erase+insert 分兩步，
//       游標與意圖更集中。
// 設計：1. 空 token 原樣返回；2. 從游標 find；3. replace 匹配；4. 依 value.size 跳過新內容。
// 成本：最壞時間 O(R*N)、額外空間 O(N+growth)，R 是匹配數，N 是動態文字長度，因中段需搬移。
// 上線注意：必須設定最大輸出與匹配次數，並明定遞迴語意；模板替換不會自動做 HTML/SQL escaping。
// -----------------------------------------------------------------------------
std::string practical_replace_all(std::string text, const std::string& token,
                        const std::string& value) {
    if (token.empty()) return text;
    std::size_t position = 0U;
    while ((position = text.find(token, position)) != std::string::npos) {
        text.replace(position, token.size(), value);
        position += value.size();
    }
    return text;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_defang_ip("1.1.1.1") == "1[.]1[.]1[.]1");
    assert(practical_replace_all("Hello, {{name}}!", "{{name}}", "Ada") == "Hello, Ada!");
    assert(practical_replace_all("aaaa", "aa", "b") == "bb");
    std::cout << "replace: tests passed\n";
}

/*
 * 【陷阱】替換後仍用舊 token 長度前進，可能漏掉或重複處理；應按需求定義重疊語意。
 * 【面試】replace 與 erase+insert？replace 一次表達原子意圖，通常更清楚。
 * 【練習】定義 replacement 內含 token 時是否遞迴展開，並避免無限迴圈。
 */

/*
 * 【replace overload 速查】
 * - replace(pos,count,string)：最常用索引版。
 * - replace(first,last,string)：iterator 半開區間。
 * - replacement 可來自 substring、pointer+count、count 個 char 或 iterator range。
 * - 新舊長度不同時尾端要搬移；capacity 不足還會重配。
 *
 * 【重疊與游標】
 * replace_all 要先決定是否處理新插入內容。本例跳過 replacement，避免 replacement
 * 含 token 時無限展開。若要求遞迴模板，需設定最大展開次數/輸出大小。
 *
 * 【面試題】pos 合法但 count 過長？count 自動截到尾端；pos>size 才 out_of_range。
 * 【安全】模板替換不等於 HTML/SQL/shell escaping，輸出邊界仍要專用 encoder。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'replace.cpp' -o '/tmp/codex_cpp_C_String_replace' && '/tmp/codex_cpp_C_String_replace'
//
// === 預期輸出（節錄）===
// replace: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
