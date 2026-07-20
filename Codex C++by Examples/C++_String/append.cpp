/*
 * std::string::append：把一段內容附加到尾端
 *
 * append 比 += 提供更多精確 overload：整個 string、substring、C 字串、pointer+count、
 * count+char、iterator range。回傳 *this，可串接。附加 n 個字元約 O(n)，若重新配置，
 * 所有舊 iterator/reference/pointer 都會失效。已知總長度時先 reserve。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::string text = "error";
    text.append(2U, ':').append(" timeout", 8U);
    assert(text == "error:: timeout");

    const std::string source = "prefix-VALUE-suffix";
    text.assign("key=").append(source, 7U, 5U);
    assert(text == "key=VALUE");
}

// LeetCode 1768（Merge Strings Alternately）。
std::string leetcode_merge_alternately(const std::string& a, const std::string& b) {
    std::string result;
    result.reserve(a.size() + b.size());
    for (std::size_t i = 0U; i < a.size() || i < b.size(); ++i) {
        if (i < a.size()) result.append(1U, a[i]);
        if (i < b.size()) result.append(1U, b[i]);
    }
    return result;
}

// 實務：建立批次 log；append(pointer,count) 能保存切片，無須先做 substr 副本。
std::string practical_join_log_fields(const std::vector<std::string>& fields) {
    std::string line;
    for (const std::string& field : fields) {
        if (!line.empty()) line.append(" | ");
        line.append(field.data(), field.size());
    }
    return line;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_merge_alternately("abc", "pq") == "apbqc");
    assert(practical_join_log_fields({"INFO", "job=7", "done"}) == "INFO | job=7 | done");
    std::cout << "append: tests passed\n";
}

/*
 * 【陷阱】append(ptr,count) 要求 ptr 指向至少 count 個可讀 char；count 錯誤就是越界讀取。
 * 【面試】append(s,pos,count) 相較 append(s.substr(...)) 的優點？避免暫時 substring 配置。
 * 【練習】為 practical_join_log_fields 預先計算總容量，再驗證輸出相同。
 */

/*
 * 【考前速查】
 * - append(s) 加完整內容；append(s,pos,count) 直接取來源片段。
 * - append(ptr,count) 可含 NUL；append(ptr) 則依賴 NUL 結尾。
 * - append(count,ch) 適合 padding；iterator overload 適合其他 char container。
 * - 回傳 string&，雖可鏈式呼叫，但除錯時分行較清楚。
 * - 自我附加與來源區間重疊要依 overload 前置條件，不用裸 pointer 猜行為。
 * - 迴圈大量附加前先算總長度 reserve，正確性仍不可依賴 capacity 精確值。
 * 【面試延伸】如何證明 builder 不會 size overflow？先檢查新增長度 <= max_size-size。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'append.cpp' -o '/tmp/codex_cpp_C_String_append' && '/tmp/codex_cpp_C_String_append'
//
// === 預期輸出（節錄）===
// append: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
