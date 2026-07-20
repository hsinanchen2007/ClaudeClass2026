/*
 * max_size()：容器理論上可表示的最大元素數，不是「目前可配置記憶體」
 *
 * max_size 受 allocator、difference_type 與實作限制影響，通常非常大。它是上限資訊，
 * 不是配置承諾；即使要求遠小於 max_size，仍可能因 RAM/位址空間而丟 bad_alloc。
 * 真實系統必須另設「領域上限」，例如 HTTP body 8 MiB，而非相信 max_size。
 */

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>

namespace {

void basic_demo() {
    const std::string text = "small";
    assert(text.size() <= text.max_size());
    assert(text.max_size() > 0U);
}

// LeetCode 1614（Maximum Nesting Depth）：先用 size 檢查索引資料是否合理。
int leetcode_max_parenthesis_depth(const std::string& text) {
    int depth = 0;
    int best = 0;
    for (const char ch : text) {
        if (ch == '(') {
            ++depth;
            if (depth > best) {
                best = depth;
            }
        } else if (ch == ')') {
            --depth;
        }
    }
    return best;
}

// 實務：檢查相加是否 overflow，並套用真正業務上限。
bool practical_can_append(const std::string& current, const std::size_t incoming,
                const std::size_t domain_limit) {
    if (incoming > current.max_size() - current.size()) {
        return false;
    }
    return current.size() + incoming <= domain_limit;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_max_parenthesis_depth("(1+(2*3)+((8)/4))+1") == 3);
    const std::string current = "1234";
    assert(practical_can_append(current, 4U, 8U));
    assert(!practical_can_append(current, 5U, 8U));
    assert(!practical_can_append(current, std::numeric_limits<std::size_t>::max(), 8U));
    std::cout << "max_size: tests passed\n";
}

/*
 * 【面試】max_size 與 capacity 差別？max_size 是理論上限；capacity 是目前不重配可容納量。
 * 【陷阱】`size()+incoming` 先加再比較可能 overflow；先以減法形式檢查。
 * 【練習】將 practical_can_append 套到讀檔迴圈，拒絕超過 1 MiB 的輸入。
 */

/*
 * 【面試速查】max_size 是容器/allocator 可表示上限，不是可用 RAM、磁碟或業務限制。
 * 真實輸入先套用較小的 domain limit，並用減法防 `size+incoming` overflow。超過
 * max_size 的操作通常丟 length_error；未超過仍可能因配置失敗丟 bad_alloc。
 * 不可用 max_size 直接 reserve 做壓力測試，這可能耗盡程序或系統資源。
 */
