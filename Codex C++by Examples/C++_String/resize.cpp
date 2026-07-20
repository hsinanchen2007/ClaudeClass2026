/*
 * resize(n[, fill])：改變 size，而不只是 capacity
 *
 * n 較小時刪除尾端；n 較大時新增 value-initialized '\0' 或指定 fill 字元。成長可能
 * 重新配置並使 iterator/pointer 失效。resize 後 [0,n) 都是合法元素；這與 reserve
 * 只預留空間完全不同。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "abcdef";
    text.resize(3U);
    assert(text == "abc");
    text.resize(5U, '.');
    assert(text == "abc..");
}

// LeetCode 1598（Crawler Log Folder）：用 string 模擬最後產生的 canonical depth 標記。
int leetcode_min_operations(const std::initializer_list<std::string> logs) {
    int depth = 0;
    for (const std::string& entry : logs) {
        if (entry == "../") {
            if (depth > 0) --depth;
        } else if (entry != "./") {
            ++depth;
        }
    }
    return depth;
}

// 實務：讀固定長度 frame；先 resize 成可寫元素，再由 fake API 寫入。
std::string practical_make_padded_id(std::string id, const std::size_t width) {
    if (id.size() > width) {
        id.resize(width);
    } else {
        id.resize(width, '0');
    }
    return id;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_min_operations({"d1/", "d2/", "../", "d21/", "./"}) == 2);
    assert(practical_make_padded_id("A7", 5U) == "A7000");
    assert(practical_make_padded_id("TOOLONG", 3U) == "TOO");
    std::cout << "resize: tests passed\n";
}

/*
 * 【陷阱】resize(n) 新字元是 '\0'，cout 看起來可能像沒增加，但 size 已增加。
 * 【面試】reserve(100) 與 resize(100)？前者 size 不變；後者建立 100 個可存取元素。
 * 【練習】改寫 practical_make_padded_id，左側補零而不是右側補零。
 */

/*
 * 【resize 決策表】
 * - 縮小：尾端元素被刪，size 變小；capacity 不保證縮。
 * - 放大無 fill：新 char value-initialize 為 '\0'。
 * - 放大有 fill：新增元素都是指定 char。
 * - 給 C API 寫 buffer：先 resize 到容量，呼叫後再 resize 到實際長度。
 * - 只想避免重配但不建立元素：reserve，不是 resize。
 * 【面試題】resize 是否保留前 min(old,new) 個元素？會；其後依縮/增規則處理。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'resize.cpp' -o '/tmp/codex_cpp_C_String_resize' && '/tmp/codex_cpp_C_String_resize'
//
// === 預期輸出（節錄）===
// resize: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
