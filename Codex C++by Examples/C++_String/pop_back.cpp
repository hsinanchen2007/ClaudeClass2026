/*
 * pop_back()：刪除最後一個字元
 *
 * 非空前提下為 O(1)，沒有回傳值。若需要刪除前的值，先讀 back() 再 pop_back()。
 * 空字串呼叫是未定義行為。被刪元素的 reference/iterator 失效；不要保存再使用。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "done!";
    assert(text.back() == '!');
    text.pop_back();
    assert(text == "done");
}

// LeetCode 2390（Removing Stars From a String）；題目保證每顆星前有可刪字元。
std::string leetcode_remove_stars(const std::string& input) {
    std::string output;
    for (const char ch : input) {
        if (ch == '*') {
            assert(!output.empty());
            output.pop_back();
        } else {
            output.push_back(ch);
        }
    }
    return output;
}

// 實務：去掉 URL/path 尾端多餘 slash，但保留根目錄單一 slash。
void practical_trim_trailing_slashes(std::string& path) {
    while (path.size() > 1U && path.back() == '/') {
        path.pop_back();
    }
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_stars("leet**cod*e") == "lecoe");
    std::string path = "/srv/data///";
    practical_trim_trailing_slashes(path);
    assert(path == "/srv/data");
    std::string root = "/";
    practical_trim_trailing_slashes(root);
    assert(root == "/");
    std::cout << "pop_back: tests passed\n";
}

/*
 * 【面試】為何 pop_back 不回傳 char？容器 modifier 通常只負責修改；先 back 可明確控制。
 * 【陷阱】`while (s.back()=='/')` 沒有 empty/size guard，刪到空時 UB。
 * 【練習】處理空路徑與相對路徑 "abc///"，寫出規格與測試。
 */

/*
 * 【考前速查】
 * - 前置條件 `!empty()`，違反是 UB，不是丟例外。
 * - O(1)、size 減一、capacity 通常不變。
 * - 沒有回傳值；若需字元先 copy back()。
 * - 刪掉的 element reference 立即失效，end iterator 也改變。
 * - 用 string 當 char stack 時，push_back/back/pop_back 是核心三件組。
 * 【面試題】為何不自動在空時 no-op？標準容器偏向由 caller 維護明確前置條件。
 */
