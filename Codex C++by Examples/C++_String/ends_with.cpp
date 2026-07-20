/*
 * C++20 ends_with：檢查後綴，回傳 bool
 *
 * overload 接 string_view 或 char；先檢查長度，再比較尾端，不配置。複雜度 O(suffix.size())。
 * 空 suffix 永遠成立。它只比較 bytes/code units，不會自動做副檔名大小寫正規化。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void basic_demo() {
    const std::string filename = "report.csv";
    assert(filename.ends_with(".csv"));
    assert(filename.ends_with('v'));
    assert(filename.ends_with(""));
    assert(!filename.ends_with(".CSV"));
}

// LeetCode-style：計算有幾個單字以指定 suffix 結尾。
int leetcode_count_suffix(const std::vector<std::string>& words,
                          const std::string_view suffix) {
    int count = 0;
    for (const std::string& word : words) {
        if (word.ends_with(suffix)) ++count;
    }
    return count;
}

// 實務：只允許明確白名單副檔名；真正安全檢查還要看內容與 MIME。
bool practical_allowed_upload(const std::string_view filename) {
    return filename.ends_with(".txt") || filename.ends_with(".md") ||
           filename.ends_with(".csv");
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_count_suffix({"running", "king", "run"}, "ing") == 2);
    assert(leetcode_count_suffix({"a", "b"}, "") == 2);
    assert(practical_allowed_upload("notes.md"));
    assert(!practical_allowed_upload("payload.exe"));
    assert(!practical_allowed_upload("NOTES.MD"));
    std::cout << "ends_with: tests passed\n";
}

/*
 * 【安全陷阱】檔名後綴不是內容驗證；`image.jpg.exe`、大小寫、Unicode 混淆都需政策處理。
 * 【面試】手寫等價式：s.size()>=suffix.size() && s.compare(s.size()-suffix.size(),... )==0。
 * 【選擇】ends_with 比 rfind 更能直接表達「必須位於結尾」。
 * 【練習】建立 ASCII case-insensitive extension checker，先正規化副檔名。
 */

/*
 * 【後綴檢查決策】
 * - 精確後綴：ends_with（C++20）。
 * - 需要最後匹配位置：rfind，並確認 `pos + suffix.size() == size()`。
 * - 找最後一個集合字元：find_last_of，語意不同。
 * - 檔案類型安全：後綴只能當第一層 UX 篩選，仍需 magic bytes/MIME/解碼器驗證。
 *
 * 【複雜度】最多比較 suffix 長度，不建立 substr。suffix 比字串長時直接 false。
 * 【大小寫】標準比較區分大小寫；跨平台副檔名政策要自行正規化。
 * 【面試題】空 suffix 為何 true？空序列是任何序列的前綴與後綴。
 * 【練習】處理 `.tar.gz` 複合副檔名白名單，不只看最後一個點。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'ends_with.cpp' -o '/tmp/codex_cpp_C_String_ends_with' && '/tmp/codex_cpp_C_String_ends_with'
//
// === 預期輸出（節錄）===
// ends_with: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
