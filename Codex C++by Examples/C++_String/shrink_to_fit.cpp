/*
 * shrink_to_fit()：非強制的「希望釋放多餘容量」請求
 *
 * 呼叫不改內容與 size；實作可以降低 capacity，也可以完全忽略。若發生重新配置，
 * iterator/reference/pointer 失效。不要在熱迴圈頻繁呼叫，因為之後再成長可能又配置。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text(1000U, 'x');
    text.resize(10U);
    const std::size_t before = text.capacity();
    text.shrink_to_fit();
    assert(text == std::string(10U, 'x'));
    assert(text.capacity() >= text.size());
    // 不能 assert capacity 變小；before 只供觀察。
    static_cast<void>(before);
}

// LeetCode 2390（Removing Stars From a String）。
std::string leetcode_remove_stars(const std::string& input) {
    std::string stack;
    stack.reserve(input.size());
    for (const char ch : input) {
        if (ch == '*') {
            assert(!stack.empty());  // 題目保證星號前有字元。
            stack.pop_back();
        } else {
            stack.push_back(ch);
        }
    }
    stack.shrink_to_fit();  // 可選；正確性不能依賴它真的縮。
    return stack;
}

// 實務：完成一次大型匯入後，長期保存的小摘要可提出縮減請求。
void practical_retain_summary(std::string& buffer, const std::size_t keep) {
    if (buffer.size() > keep) {
        buffer.resize(keep);
    }
    buffer.shrink_to_fit();
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_remove_stars("leet**cod*e") == "lecoe");
    std::string buffer(4096U, 'a');
    practical_retain_summary(buffer, 16U);
    assert(buffer.size() == 16U);
    std::cout << "shrink_to_fit: tests passed\n";
}

/*
 * 【面試】shrink_to_fit 是否保證 capacity==size？不保證，它是 non-binding request。
 * 【陷阱】保存 c_str 指標後 shrink_to_fit，再讀舊指標可能 UB。
 * 【練習】比較「swap with a temporary」與 shrink_to_fit 的意圖、例外與可讀性。
 */

/*
 * 【使用準則】只在大型暫存工作結束、字串將長期保持很小且記憶體壓力可量測時考慮。
 * 呼叫後應重新取得 data/c_str/iterator；即使 capacity 沒變，也不要寫依賴特定實作的測試。
 * 【面試題】為何是 non-binding？實作可能有 SSO、allocator bucket 或保留策略，強制縮小
 * 反而不一定更有效率。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'shrink_to_fit.cpp' -o '/tmp/codex_cpp_C_String_shrink_to_fit' && '/tmp/codex_cpp_C_String_shrink_to_fit'
//
// === 預期輸出（節錄）===
// shrink_to_fit: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
