/*
 * capacity()：目前配置可容納多少 char 而「不必重新配置」
 *
 * 永遠有 capacity() >= size()。capacity 不含對外可見的終止 NUL，也不是可寫元素數；
 * 只有 [0,size()) 可讀寫。成長策略與 small-string optimization 都是實作細節，不能
 * 依賴特定數值或倍增規則。
 */

#include <cassert>
#include <iostream>
#include <string>

namespace {

void basic_demo() {
    std::string text = "abc";
    assert(text.capacity() >= text.size());
    const std::size_t before = text.capacity();
    text.push_back('d');
    assert(text.capacity() >= text.size());
    // before 是否改變不可攜；只驗證標準保證。
    static_cast<void>(before);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1768. Merge Strings Alternately（交錯合併字串）
// 題目：交替取 a、b 的同索引字元，最後接上較長輸入的剩餘字元；"ab"、"XYZ" 得 "aXbYZ"。
// 為何使用本章主題：答案長度確定為 N+M，先 reserve 後可用 capacity() 驗證儲存區至少
//       能容納目前答案；正確性不依賴實作實際給了多少額外容量。
// 思路：1. 預留 N+M；2. 索引逐輪前進；3. 對仍未結束的輸入各附加一字元。
// 複雜度：時間 O(N+M)、額外空間 O(N+M)，N、M 是兩輸入長度，空間為輸出字串。
// 易錯點：capacity 不是可直接寫入的元素數；只有 [0,size()) 合法，且 N+M 應防無號溢位。
// -----------------------------------------------------------------------------
std::string leetcode_merge_alternately(const std::string& a, const std::string& b) {
    std::string answer;
    answer.reserve(a.size() + b.size());
    for (std::size_t i = 0U; i < a.size() || i < b.size(); ++i) {
        if (i < a.size()) {
            answer.push_back(a[i]);
        }
        if (i < b.size()) {
            answer.push_back(b[i]);
        }
    }
    assert(answer.capacity() >= answer.size());
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】追加前的重新配置預判
// 情境：低延遲 log builder 已有內容，追加 incoming bytes 前要判斷目前容量是否足夠。
// 為何使用本章主題：capacity()-size() 是現有配置尚可容納的元素數，可做延遲或指標失效風險
//       的提示；相較猜測 SSO 或成長倍數，此判斷只依標準保證。
// 設計：1. 讀目前 size 與 capacity；2. 計算剩餘容量；3. incoming 不超過剩餘量才回 true。
// 成本：時間 O(1)、額外空間 O(1)，不配置也不修改 buffer。
// 上線注意：結果只代表「此刻下一次追加」；其他執行緒修改或追加量改變就失效，且即使會重配
//       也不代表操作失敗，只代表舊 pointer／iterator 可能失效。
// -----------------------------------------------------------------------------
bool practical_next_append_fits_without_reallocation(const std::string& buffer,
                                           const std::size_t incoming) {
    return incoming <= buffer.capacity() - buffer.size();
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_merge_alternately("ab", "XYZ") == "aXbYZ");
    std::string buffer;
    buffer.reserve(32U);
    buffer = "log";
    assert(practical_next_append_fits_without_reallocation(buffer, 20U));
    std::cout << "capacity: tests passed\n";
}

/*
 * 【陷阱】對 data()+size() 到 data()+capacity() 寫入是 UB；capacity 不是 size。
 * 【面試】SSO 大小是多少？標準沒規定，不能寫死 15/22 等數字。
 * 【練習】記錄逐字元 append 時 capacity 變化，只當觀察，不寫依賴結果的 assertion。
 */

/*
 * 【size/capacity/max_size 三分法】
 * - size：目前存在、可存取的元素數。
 * - capacity：目前配置在不重配前至少能容納的元素數。
 * - max_size：allocator/實作的理論上限，不是 RAM 保證。
 * `capacity-size` 可估下一次 append 是否重配，但不能拿來當可直接寫的元素。
 * clear 常保留 capacity；shrink_to_fit 只提出縮減請求；reserve 只保證下限。
 * 【面試題】為何標準不規定 growth factor？讓實作依 allocator 與平台調整。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'capacity.cpp' -o '/tmp/codex_cpp_C_String_capacity' && '/tmp/codex_cpp_C_String_capacity'
//
// === 預期輸出（節錄）===
// capacity: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
