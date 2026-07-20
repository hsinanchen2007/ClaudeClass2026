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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1614. Maximum Nesting Depth of the Parentheses（括號最大巢狀深度）
// 題目：輸入合法括號字串，回傳任一位置的最大開括號深度；`(1+(2*3)+((8)/4))+1` 回 3。
// 為何使用本章主題：此演算法只掃描字元，沒有使用 max_size；它是對照案例，說明理論容量上限
//       與括號深度邏輯無關，不能為了題目預先配置到 max_size。
// 思路：1. depth/best 歸零；2. 遇 '(' 增加並更新 best；3. 遇 ')' 減少；4. 回 best。
// 複雜度：時間 O(N)、額外空間 O(1)，N 是 text 長度。
// 易錯點：原題保證括號有效；一般 parser 要拒絕 depth 負值與結束時非零，且不可 reserve(max_size)。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】訊息追加大小閘門
// 情境：現有訊息要追加 incoming bytes，既不能超過 string 可表示上限，也不能超過較小的業務上限。
// 為何使用本章主題：max_size()-size() 先以減法檢查容器上限，避免 `size+incoming` 無號溢位；
//       再套 domain_limit，而不是把 max_size 誤當可用 RAM 或安全輸入上限。
// 設計：1. 先判 incoming 是否超過容器剩餘表示範圍；2. 安全後才相加；3. 與 domain limit 比較。
// 成本：時間 O(1)、額外空間 O(1)，不配置也不修改 current。
// 上線注意：domain_limit 若小於 current.size() 也會拒絕；通過只代表長度可接受，後續配置仍可能 bad_alloc。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'max_size.cpp' -o '/tmp/codex_cpp_C_String_max_size' && '/tmp/codex_cpp_C_String_max_size'
//
// === 預期輸出（節錄）===
// max_size: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
