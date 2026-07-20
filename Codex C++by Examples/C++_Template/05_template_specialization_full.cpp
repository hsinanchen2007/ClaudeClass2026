/*
 * 第 05 章：完整特化（full specialization）
 *
 * 主模板描述一般規則；完整特化為「某一組確切模板引數」提供不同實作。
 * 語法是 template <> class Name<ExactType>。完整特化不是繼承，兩者是獨立類別，
 * 所以主模板新增成員時，特化不會自動取得。
 */

#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

template <typename T>
struct TextEncoder {
    static std::string encode(const T& value) {
        std::ostringstream output;
        output << value;
        return output.str();
    }
};

// bool 若走一般串流會得到 0/1；API 規格要求 true/false，因此完整特化。
template <>
struct TextEncoder<bool> {
    static std::string encode(bool value) { return value ? "true" : "false"; }
};

template <>
struct TextEncoder<std::string> {
    static std::string encode(const std::string& value) { return '"' + value + '"'; }
};

// LeetCode 242：Valid Anagram。此處把字元正規化規則做成策略型別。
template <typename Character>
struct Normalize {
    static Character apply(Character value) { return value; }
};

template <>
struct Normalize<char> {
    static char apply(char value) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
    }
};

template <typename Character>
bool leetcode_is_anagram(const std::basic_string<Character>& left,
                         const std::basic_string<Character>& right) {
    if (left.size() != right.size()) {
        return false;
    }
    // 用 Character 當 key，避免 wchar_t/char16_t 被截成 unsigned char。
    std::unordered_map<Character, int> counts;
    counts.reserve(left.size());
    for (Character ch : left) {
        const auto normalized = Normalize<Character>::apply(ch);
        ++counts[normalized];
    }
    for (Character ch : right) {
        const auto normalized = Normalize<Character>::apply(ch);
        if (--counts[normalized] < 0) {
            return false;
        }
    }
    return true;
}

// 實務：組出小型設定 payload；不同欄位型別走相同呼叫介面。
template <typename T>
std::string practical_make_field(const std::string& key, const T& value) {
    return '"' + key + "\":" + TextEncoder<T>::encode(value);
}

int main() {
    assert(TextEncoder<int>::encode(42) == "42");
    assert(TextEncoder<bool>::encode(true) == "true");
    assert(TextEncoder<std::string>::encode("gpu") == "\"gpu\"");

    assert(leetcode_is_anagram(std::string{"Listen"}, std::string{"Silent"}));
    assert(!leetcode_is_anagram(std::string{"rat"}, std::string{"car"}));

    const std::string payload = "{" + practical_make_field("enabled", true) + "," +
                                practical_make_field("name", std::string{"cuda"}) + "}";
    assert(payload == "{\"enabled\":true,\"name\":\"cuda\"}");

    std::cout << payload << '\n';
}

/*
 * 【陷阱】
 * - 函式模板可以完整特化，但不能偏特化；需要偏特化時通常改用類別模板或 overload。
 * - 特化必須在首次會造成實體化之前可見，否則不同 translation unit 可能行為不一致。
 * - std 命名空間通常不可自行加入特化；只有標準明確允許的 user-defined type 情況例外。
 * 【面試】overload 與 specialization 的選擇？函式行為差異通常優先 overload，規則更直觀。
 * 【練習】新增 TextEncoder<vector<int>>，輸出 [1,2,3]。
 */
