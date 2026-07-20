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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 242. Valid Anagram（有效的字母異位詞）
// 題目：判斷兩字串能否由相同字元重新排列而成；anagram/nagaram 為 true，rat/car 為 false。
// 為何使用本章主題：Normalize<Character> 主模板保持原值，Normalize<char> 完整特化為小寫化；
// 原題輸入為小寫英文，本例刻意擴成 char 大小寫不敏感的教學行為。
// 思路：長度不同先失敗；正規化 left 後累加次數；正規化 right 後遞減，任何負值立即失敗。
// 複雜度：平均時間 O(N)、額外空間 O(K)，N 是字串長度、K 是不同正規化字元數。
// 易錯點：tolower 前要轉 unsigned char；目前受 C locale 影響，寬字元特化也沒有 Unicode 正規化。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】異質設定欄位編碼
// 情境：設定 payload 需要以相同 make_field 介面編碼數字、布林與字串欄位。
// 為何使用本章主題：TextEncoder<T> 主模板處理可串流型別，bool/string 完整特化符合各自格式；
// 相較在 make_field 堆疊 runtime type 判斷，編譯期直接選定 encoder。
// 設計：make_field 產生 key 前綴；依 T 選 TextEncoder；呼叫端串接多個欄位成物件文字。
// 成本：時間與空間 O(K+V)，K/V 是 key 與編碼值長度，字串串接可能多次配置。
// 上線注意：目前 key/string 未做引號與控制字元 escaping，不能當完整 JSON serializer，數值格式也需規範。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_template_specialization_full.cpp' -o '/tmp/codex_cpp_C_Template_05_template_specialization_full' && '/tmp/codex_cpp_C_Template_05_template_specialization_full'
//
// === 預期輸出（節錄）===
// {"enabled":true,"name":"cuda"}
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
