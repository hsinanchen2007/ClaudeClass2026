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

// LeetCode 1108（Defanging an IP Address）：由左至右替換每個點。
std::string leetcode_defang_ip(std::string address) {
    std::size_t position = 0U;
    while ((position = address.find('.', position)) != std::string::npos) {
        address.replace(position, 1U, "[.]");
        position += 3U;  // 跳過剛插入的內容，避免再次找到其中的點。
    }
    return address;
}

// 實務：替換所有模板 token；空 token 必須拒絕，否則迴圈不前進。
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
