/*
 * C++23 string range modifiers：append_range / insert_range / replace_with_range
 *
 * 這些 API 接受 ranges，讓 vector<char>、array<char,N>、view 等不必手動寫 begin/end。
 * 它們不是 C++20；本檔在 C++20 用 iterator overload 做等價 fallback，並以 feature-test
 * macro 保留真正 C++23 路徑。複雜度取決於輸入長度與搬移/重新配置。
 *
 * 易錯陷阱：position/range 不合法不能只靠 assert；release build 也必須拒絕。
 * single-pass range 只能走訪一次；fallback 先 materialize 也讓 self-overlap 有明確快照。
 * 真正 C++23 API 的 iterator 必須屬於被修改的 string，修改後舊 iterator 可能失效。
 */

#include <array>
#include <cassert>
#include <iostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

template <typename Range>
std::string materialize_chars(Range&& range) {
    std::string temporary;
    for (auto&& value : range) {
        temporary.push_back(static_cast<char>(value));
    }
    return temporary;
}

template <typename Range>
void append_char_range(std::string& output, Range&& range) {
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    output.append_range(std::forward<Range>(range));
#else
    // 先 materialize，才能支援 non-common、single-pass input range，也能安全處理
    // range 引用 output 本身的 self-overlap。
    const std::string temporary = materialize_chars(std::forward<Range>(range));
    output.append(temporary);
#endif
}

template <typename Range>
void insert_char_range(std::string& output, std::size_t position, Range&& range) {
    if (position > output.size()) {
        throw std::out_of_range("insert position");
    }
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    output.insert_range(output.cbegin() + static_cast<std::ptrdiff_t>(position),
                        std::forward<Range>(range));
#else
    const std::string temporary = materialize_chars(std::forward<Range>(range));
    output.insert(position, temporary);
#endif
}

template <typename Range>
void replace_char_range(std::string& output, std::size_t first, std::size_t last,
                        Range&& range) {
    if (first > last || last > output.size()) {
        throw std::out_of_range("replace range");
    }
#if defined(__cpp_lib_containers_ranges) && __cpp_lib_containers_ranges >= 202202L
    output.replace_with_range(
        output.cbegin() + static_cast<std::ptrdiff_t>(first),
        output.cbegin() + static_cast<std::ptrdiff_t>(last),
        std::forward<Range>(range));
#else
    const std::string temporary = materialize_chars(std::forward<Range>(range));
    output.replace(first, last - first, temporary);
#endif
}

void basic_demo() {
    std::string text = "C";
    const std::array<char, 2U> suffix{'+', '+'};
    append_char_range(text, suffix);
    assert(text == "C++");

    const std::array<char, 2U> brackets{'[', ']'};
    insert_char_range(text, 0U, std::views::counted(brackets.data(), 1));
    insert_char_range(text, text.size(), std::views::counted(brackets.data() + 1, 1));
    assert(text == "[C++]");

    const std::array<char, 3U> language{'C', 'P', 'P'};
    replace_char_range(text, 1U, 4U, language);
    assert(text == "[CPP]");

    std::istringstream input("A B");
    auto input_chars = std::ranges::istream_view<char>(input);
    std::string streamed;
    append_char_range(streamed, input_chars);
    assert(streamed == "AB");

    std::string overlap = "ab";
    append_char_range(overlap, std::string_view(overlap));
    assert(overlap == "abab");

    bool invalid_range_rejected = false;
    try {
        insert_char_range(overlap, overlap.size() + 1U, suffix);
    } catch (const std::out_of_range&) {
        invalid_range_rejected = true;
    }
    assert(invalid_range_rejected);
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1768. Merge Strings Alternately（交錯合併字串）
// 題目：從 a 開始交替取兩字串字元，某一輸入耗盡後接上另一輸入剩餘部分；"ab"、"XYZ" 得 "aXbYZ"。
// 為何使用本章主題：每個字元包成一元素 array，再由 append_char_range 接受 range；這是刻意
//       展示 C++23 range modifier/fallback 的教學寫法，實際解題直接 push_back 更簡單。
// 思路：1. 索引走到兩輸入皆結束；2. a 尚有字元就建立一元素 range 並附加；3. b 同理。
// 複雜度：時間 O(N+M)、額外空間 O(N+M)，N、M 是輸入長度；一元素 temporary 為常數空間。
// 易錯點：這不是最精簡提交；C++20 fallback 會 materialize，每個來源 range 也不得在修改中失效。
// -----------------------------------------------------------------------------
std::string leetcode_merge_alternately(const std::string& a, const std::string& b) {
    std::string result;
    for (std::size_t i = 0U; i < a.size() || i < b.size(); ++i) {
        if (i < a.size()) {
            const std::array<char, 1U> one{a[i]};
            append_char_range(result, one);
        }
        if (i < b.size()) {
            const std::array<char, 1U> one{b[i]};
            append_char_range(result, one);
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】網路 byte chunks 訊息組裝
// 情境：接收層交付多個 vector<char> 區塊，要依到達順序合成一份完整 HTTP2 文字標記。
// 為何使用本章主題：append_range 可直接接受每個 char range，不需 caller 先轉成 string；
//       C++20 fallback 先 materialize，亦給 single-pass/self-overlap 明確快照語意。
// 設計：1. 建立空 result；2. 依序走 chunks；3. 將每段完整 range 附加到尾端。
// 成本：時間 O(T)、額外答案空間 O(T)，T 是所有 chunk bytes 總和；fallback 另有單段 temporary。
// 上線注意：應先檢查總長度溢位並 reserve；若資料是任意 binary，string 可保存 NUL，但後續不可當 C 字串。
// -----------------------------------------------------------------------------
std::string practical_assemble_chunks(const std::vector<std::vector<char>>& chunks) {
    std::string result;
    for (const auto& chunk : chunks) append_char_range(result, chunk);
    return result;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_merge_alternately("ab", "XYZ") == "aXbYZ");
    assert(practical_assemble_chunks({{'H', 'T'}, {'T', 'P'}, {'2'}}) == "HTTP2");
    std::cout << "range modifiers: tests passed\n";
}

/*
 * 【面試】range modifier 的主要價值？直接接受 range，降低 iterator pair 配錯的機會。
 * 【self-overlap】標準 range modifier 的 effects 先由 range 建立 temporary string，再修改
 * 目標，因此單純引用同一 string 有明確語意；fallback 也刻意先 materialize。自行手寫
 * 「邊走訪、邊 append」則仍可能因 reallocation 讓來源 iterator 失效。
 * 【練習】加入只提供 sentinel end 的自訂 input_range，驗證 fallback 不要求 common range。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'range_modifiers.cpp' -o '/tmp/codex_cpp_C_String_range_modifiers' && '/tmp/codex_cpp_C_String_range_modifiers'
//
// === 預期輸出（節錄）===
// range modifiers: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
