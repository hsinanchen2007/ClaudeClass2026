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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 2390. Removing Stars From a String（移除星號）
// 題目：每個星號刪除自己與左側最近未刪字元；"leet**cod*e" 最後得到 "lecoe"。
// 為何使用本章主題：演算法完成後呼叫 shrink_to_fit 只是可選的縮容請求，不影響答案；
//       它展示 API 契約，但競賽提交通常應省略，避免多一次可能的配置與搬移。
// 思路：1. 用 string 當 stack；2. 非星號 push；3. 星號 pop；4. 可選提出縮容後回傳。
// 複雜度：演算法時間 O(N)，shrink 若重配再 O(N)；額外空間 O(N)，N 是 input 長度。
// 易錯點：不能 assert capacity==size；星號前需有字元，且 shrink 後所有舊 pointer/view 要重取。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】大型匯入後保留短摘要
// 情境：buffer 曾容納數千 bytes，匯入完成只需長期保存前 keep 個字元，希望降低閒置容量。
// 為何使用本章主題：先 resize 刪除不需內容，再以 shrink_to_fit 提出釋放多餘配置的請求；
//       相較每輪都縮容，此做法只放在工作階段邊界。
// 設計：1. size 大於 keep 才縮內容；2. 呼叫 shrink_to_fit；3. 正確性只檢查 size/內容。
// 成本：最壞時間 O(N)、可能配置與搬移 O(keep)，額外配置成本由實作決定；N 是舊長度。
// 上線注意：縮容非強制且可能增加延遲；呼叫後重取所有 handle，敏感資料也不因縮容而保證抹除。
// -----------------------------------------------------------------------------
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
