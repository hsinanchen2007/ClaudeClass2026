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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2390. Removing Stars From a String（移除星號）
// 題目：每個 `*` 會刪除自己與左側最近尚未刪除的字元；"leet**cod*e" 得 "lecoe"。
// 為何使用本章主題：把 output 當 char stack，遇星號以 pop_back O(1) 刪除尾端；一般字元 push_back。
// 思路：1. 從左掃 input；2. 非星號入 stack；3. 星號刪除 stack 尾；4. 最終 stack 即答案。
// 複雜度：時間 O(N)、額外空間 O(N)，N 是 input 長度。
// 易錯點：原題保證每顆星前有可刪字元；一般輸入不能只靠 assert，空 stack 呼叫 pop_back 是 UB。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】路徑尾端斜線正規化
// 情境：將 `/srv/data///` 正規化為 `/srv/data`，但根目錄 `/` 必須保留。
// 為何使用本章主題：back() 檢查目前最後字元、pop_back() 原地刪一格，適合未知數量的尾端重複字元，
//       不需重新建立整條路徑。
// 設計：1. 以 size>1 保護根目錄與非空條件；2. 尾端為 `/` 就 pop；3. 直到條件不成立。
// 成本：時間 O(K)、額外空間 O(1)，K 是被移除的尾斜線數。
// 上線注意：這不是完整 path canonicalization，不處理空字串、`//` 特殊語意、`.`/`..` 或 Windows 分隔符。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'pop_back.cpp' -o '/tmp/codex_cpp_C_String_pop_back' && '/tmp/codex_cpp_C_String_pop_back'
//
// === 預期輸出（節錄）===
// pop_back: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
