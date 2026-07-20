/*
 * Permutation / lexicographical algorithms 面試速查
 * =================================================
 * 同目錄四個 API：is_permutation、lexicographical_compare、next_permutation、
 * prev_permutation。前者比較 multiset，第二個比較字典序，後兩個在排列空間移動。
 *
 * 【選擇表】
 * 忽略順序但次數必須相同                    is_permutation
 * 依第一差異與 prefix 規則比較兩序列         lexicographical_compare
 * 找下一個字典序排列 / 正向枚舉              next_permutation
 * 找上一個字典序排列 / 反向枚舉              prev_permutation
 *
 * 【複雜度】
 * is_permutation 一般最壞 O(N^2)；sort+equal O(N log N)；hash count 平均 O(N)。
 * lexicographical_compare O(min(N,M))。
 * next/prev 每一步 O(N)、空間 O(1)；列出全部為 O(N!*N)，輸出本身已階乘爆炸。
 *
 * 【生命週期】
 * 比較演算法不保存 iterator；排列演算法原地交換值、不改容器 size。iterator 通常
 * 沒失效，但所指位置的值改變。自訂 comparator/predicate capture 必須活到呼叫結束。
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

// 基礎整合：產生所有 unique 排列並確認每個都是原輸入的 permutation。
std::vector<std::string> basic_unique_permutations(std::string input) {
    const std::string original = input;
    std::sort(input.begin(), input.end());
    std::vector<std::string> output;
    do {
        assert(std::is_permutation(input.begin(), input.end(),
                                   original.begin(), original.end()));
        if (!output.empty()) {
            assert(std::lexicographical_compare(
                output.back().begin(), output.back().end(), input.begin(), input.end()));
        }
        output.push_back(input);
    } while (std::next_permutation(input.begin(), input.end()));
    return output;
}

// LeetCode 567：Permutation in String；256 格 byte frequency 做 O(N) sliding window。
// 契約：空 pattern 是每個 text（包含空 text）在 index 0 的排列子字串。
bool leetcode_check_inclusion(const std::string& pattern,
                              const std::string& text) {
    if (pattern.empty()) {
        return true;
    }
    if (pattern.size() > text.size()) {
        return false;
    }
    std::array<std::size_t, 256> need{};
    std::array<std::size_t, 256> window{};
    const auto byte_index = [](char ch) {
        return static_cast<std::size_t>(static_cast<unsigned char>(ch));
    };
    for (const char ch : pattern) {
        ++need[byte_index(ch)];
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        ++window[byte_index(text[i])];
        if (i >= pattern.size()) {
            --window[byte_index(text[i - pattern.size()])];
        }
        if (window == need) {
            return true;
        }
    }
    return false;
}

struct Version {
    std::vector<int> fields;
    std::string label;
};

// 實務：排序 release；數字段優先，完全相同再以 label 字典序決勝。
std::vector<Version> practical_sort_versions(std::vector<Version> versions) {
    std::sort(versions.begin(), versions.end(), [](const Version& lhs,
                                                   const Version& rhs) {
        if (lhs.fields != rhs.fields) {
            return std::lexicographical_compare(
                lhs.fields.begin(), lhs.fields.end(),
                rhs.fields.begin(), rhs.fields.end());
        }
        return lhs.label < rhs.label;
    });
    return versions;
}

int main() {
    const auto permutations = basic_unique_permutations("AAB");
    assert((permutations == std::vector<std::string>{"AAB", "ABA", "BAA"}));
    assert(leetcode_check_inclusion("ab", "eidbaooo"));
    assert(!leetcode_check_inclusion("ab", "eidboaoo"));
    assert(leetcode_check_inclusion("", ""));
    assert(leetcode_check_inclusion("", "non-empty"));

    const std::string byte_pattern{static_cast<char>(0xFF), '\0'};
    const std::string byte_text{'x', '\0', static_cast<char>(0xFF), 'y'};
    assert(leetcode_check_inclusion(byte_pattern, byte_text));
    assert(!leetcode_check_inclusion(byte_pattern, std::string{'x', '\0', 'y'}));

    const auto versions = practical_sort_versions(
        {{{1, 10}, "stable"}, {{1, 2}, "stable"}, {{1, 10}, "beta"}});
    assert(versions[0].fields == std::vector<int>({1, 2}));
    assert(versions[1].label == "beta");
    assert(versions[2].label == "stable");

    std::vector<int> wrap{3, 2, 1};
    assert(!std::next_permutation(wrap.begin(), wrap.end()));
    assert((wrap == std::vector<int>{1, 2, 3}));
    assert(!std::prev_permutation(wrap.begin(), wrap.end()));
    assert((wrap == std::vector<int>{3, 2, 1}));

    std::cout << "Permutation summary：比較、枚舉與滑動視窗測試通過\n";
}

/*
 * 【陷阱總表】
 * 1. is_permutation 比出現次數，不只 membership。
 * 2. 四 iterator overload 才能驗兩邊長度；舊三 iterator 版假設第二邊足夠長。
 * 3. 自訂 equality 必須是等價關係；近似浮點 epsilon 常不具傳遞性。
 * 4. lexicographical_compare 回 false 可能是相等，也可能 lhs 較大；要反向再比較
 *    或使用 C++20 three-way 才能分三種結果。
 * 5. prefix 全相同時短者較小；"app" < "apple"。
 * 6. 數字字串與版本不可直接字典比較；"10" < "2" 是字典序的合法結果。
 * 7. next/prev 回 false 時仍修改範圍，分別 wrap 到最小/最大排列。
 * 8. 完整正向枚舉前要先 sort ascending；反向則 descending。
 * 9. 重複元素會自然只列 unique 字典序排列，但總量仍可能巨大。
 * 10. comparator 必須 strict weak ordering，並與起始排序使用同一規則。
 * 11. 排列不改 size，iterator 不失效；但位置上的 value identity 已改。
 * 12. locale-aware 人名排序不是單純 code-unit lexicographical comparison。
 * 13. 空 pattern 需明定語意；本檔比照空子字串規則回 true，不能靠 loop 偶然命中。
 * 14. char 可能有號；拿任意 byte 當陣列索引時先轉 unsigned char，再轉 size_t。
 *
 * 【面試快問快答】
 * Q: 如何手寫 next_permutation？
 * A: 由右找非遞增 suffix、找 pivot、與 suffix 中剛大於它的元素交換、反轉 suffix。
 * Q: 為何最後反轉 suffix？
 * A: suffix 原為最大降序；交換後要改成最小升序，才能得到「緊鄰下一個」。
 * Q: LC242 用 is_permutation 最佳嗎？
 * A: generic 且簡潔；alphabet 已知時 frequency O(N) 且更可預測。只接受 a-z 可用
 *    26 格；本例 API 接一般 std::string，故用 unsigned-char 的完整 256 格。
 * Q: LC567 為何不用每個 window 呼叫 is_permutation？
 * A: 每窗重新比較會增加成本；滑動 frequency 每步 O(1) 更新。
 * Q: LC1053 可直接 prev_permutation 嗎？
 * A: 不可，題目限定一次 swap；prev_permutation 可能反轉整個 suffix。
 *
 * 【選型與實務】
 * - 驗證重試 payload 忽略順序：is_permutation 或 canonical sort/hash。
 * - 版本/路徑/tuple 順序：先 parse 正確 token，再 lexicographical compare。
 * - 小型 exhaustive test：next_permutation；可邊生成邊測，不必全部存入 vector。
 * - constraint search：backtracking + pruning 通常勝過先列 N! 再過濾。
 * - 安全/可重現系統：明確固定 comparator、locale 與 normalization。
 *
 * 【面試前自問】
 * - 我能說出 wrap-around 後容器內容嗎？
 * - 我能解釋 duplicate permutation 為何不重複嗎？
 * - 我能比較 O(N^2)、sort、hash 三種 permutation 驗證嗎？
 * - 我能區分 sequence equality、set equality、multiset equality 嗎？
 * - 我能指出版本字串比較的 parse 邊界嗎？
 *
 * 練習：
 * 1. 手寫 next/prev，對所有長度 <=8 的多重集合與標準函式交叉驗證。
 * 2. 實作 LC1053 一次 swap，使用 prev_permutation 當小資料 oracle。
 * 3. 加入 Unicode normalization 後比較使用者名稱；不要把 byte order 當語言排序。
 * 4. 以 mismatch-counter 優化 check_inclusion，避免每步比較完整 256 格陣列。
 * 5. 實作 rank/unrank permutation，直接跳到第 k 個排列而不枚舉前 k-1 個。
 */
