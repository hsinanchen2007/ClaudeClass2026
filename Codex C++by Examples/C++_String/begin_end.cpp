/*
 * begin()/end()：把 std::string 當一般容器使用
 *
 * [begin,end) 是半開區間：begin 指第一個元素，end 指最後元素「之後」，不可解參考。
 * 空字串 begin()==end()。iterator 為 random-access iterator，因此可 +n、相減、排序。
 * const 物件取得 const_iterator；也可明確用 cbegin/cend。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "dcba";
    std::sort(text.begin(), text.end());
    assert(text == "abcd");
    const std::string fixed = text;
    assert(std::distance(fixed.cbegin(), fixed.cend()) == 4);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 344. Reverse String（反轉字串）
// 題目：原地反轉字元序列且只用常數額外空間；例如 hello 變成 olleh。
// 為何使用本章主題：begin()/end() 提供完整半開區間，讓 std::reverse 直接修改 string；
//       本檔是把原題 vector<char> 容器等價改成 string 的教學版本。
// 思路：1. 取得 [begin,end)；2. 由標準演算法成對交換首尾元素；3. 奇數中央字元保持不動。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 的字元數。
// 易錯點：end() 是尾後哨兵不可解參考；原題簽名不同，但 iterator 解法可直接套回 vector。
// -----------------------------------------------------------------------------
void leetcode_reverse_string(std::string& text) {
    std::reverse(text.begin(), text.end());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】紀錄行數字遮蔽
// 情境：在送出 log 前，把帳號或識別碼中的所有 ASCII 數字原地改成 '*'，避免輸出原值。
// 為何使用本章主題：可寫的 begin()/end() 讓 std::replace_if 走訪同一 buffer，不必另建字串。
// 設計：1. 以半開區間覆蓋全行；2. predicate 只辨認 '0' 到 '9'；3. 命中就原地替換。
// 成本：時間 O(N)、額外空間 O(1)，N 是 log_line 的 byte 數。
// 上線注意：這只遮 ASCII digits，不識別 Unicode 數字或欄位語意，也不是完整個資去識別；
//       多執行緒不得同時讀寫同一字串。
// -----------------------------------------------------------------------------
void practical_redact_digits(std::string& log_line) {
    std::replace_if(log_line.begin(), log_line.end(),
                    [](const char ch) { return ch >= '0' && ch <= '9'; }, '*');
}

}  // namespace

int main() {
    basic_demo();
    std::string word = "hello";
    leetcode_reverse_string(word);
    assert(word == "olleh");

    std::string log = "card=1234 user=42";
    practical_redact_digits(log);
    assert(log == "card=**** user=**");
    std::cout << "begin/end: tests passed\n";
}

/*
 * 【失效規則】basic_string 對修改操作允許的失效範圍比「只有重新配置才失效」更廣；
 * 可攜程式在 insert/append/erase/replace/reserve 後，應把舊 iterator、pointer 與
 * reference 都視為不可再用，重新由 string 取得。不要套用 vector 的較細規則猜測。
 * 【面試】end 能否解參考？不能，它只是哨兵。
 * 【練習】用 std::transform 將 ASCII 小寫轉大寫，正確處理 char signedness。
 */

/*
 * 【iterator 速查表】
 * - begin/end：可修改 iterator（若 string 非 const）。
 * - cbegin/cend：明確唯讀，避免意外修改。
 * - rbegin/rend：反向走訪；base() 有「下一個正向位置」語意。
 * - distance(begin,end) 對 string 為 O(1)，因它是 random-access iterator。
 * - end 是 half-open sentinel，永遠不可 `*end`。
 *
 * 【修改期間的安全迴圈】
 * erase iterator 區間後，接住 erase 回傳的新 iterator；不要遞增已交給 erase 的舊值。
 * insert/append/replace/reserve 後，最安全是從 string 重新取得所有 view/iterator。
 *
 * 【面試題】為何 STL 使用 [first,last)？空範圍自然 first==last、長度可相減，
 * 相鄰範圍 `[a,b)` 與 `[b,c)` 不重疊，組合演算法較一致。
 * 【練習】用 iterator 寫只保留 ASCII printable bytes 的 filter。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'begin_end.cpp' -o '/tmp/codex_cpp_C_String_begin_end' && '/tmp/codex_cpp_C_String_begin_end'
//
// === 預期輸出（節錄）===
// begin/end: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
