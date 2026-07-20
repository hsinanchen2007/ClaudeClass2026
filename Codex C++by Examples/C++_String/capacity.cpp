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

// LeetCode 1768（Merge Strings Alternately）；reserve 後 capacity 至少容納答案。
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

// 實務：觀察是否會重配，但不把特定 allocator 行為當正確性條件。
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
