/*
 * 第 05 章：std::any
 *
 * any 可保存任意 CopyConstructible 型別，型別集合不必事先列出；這是完全 type erasure。
 * 優點是彈性，缺點是契約移到執行期：取值者必須知道真正型別，錯誤 any_cast 會丟例外。
 * 核心演算法若充滿 any_cast，通常表示資料模型設計太弱；它較適合外掛邊界/稀疏 metadata。
 */

#include <any>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::any basic_dynamic_value(bool use_text) {
    if (use_text) {
        return std::string{"cuda"};
    }
    return 42;
}

// LeetCode 1 的動態輸入版本：先驗證每個 any 確實是 int，再執行 Two Sum。
// 正式 LeetCode 不會用 any；此處是為了展示 runtime type check。
std::pair<std::size_t, std::size_t>
leetcode_two_sum_any(const std::vector<std::any>& values, int target) {
    std::unordered_map<int, std::size_t> seen;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const int* current = std::any_cast<int>(&values[i]); // pointer 版失敗回 nullptr，不丟例外
        if (current == nullptr) {
            continue;
        }
        if (const auto found = seen.find(target - *current); found != seen.end()) {
            return {found->second, i};
        }
        seen.emplace(*current, i);
    }
    return {values.size(), values.size()};
}

// 【實務情境】外掛 metadata 的欄位型別開放，邊界層集中做 any_cast 驗證。
using Metadata = std::unordered_map<std::string, std::any>;

std::string practical_render_metadata(const Metadata& metadata, const std::string& key) {
    const auto found = metadata.find(key);
    if (found == metadata.end()) {
        return "missing";
    }
    if (const auto* value = std::any_cast<int>(&found->second)) {
        return std::to_string(*value);
    }
    if (const auto* value = std::any_cast<std::string>(&found->second)) {
        return *value;
    }
    if (const auto* value = std::any_cast<bool>(&found->second)) {
        return *value ? "true" : "false";
    }
    return "unsupported";
}

int main() {
    std::any value = basic_dynamic_value(true);
    assert(value.has_value());
    assert(value.type() == typeid(std::string));
    assert(std::any_cast<std::string>(value) == "cuda");

    value.emplace<int>(7);
    assert(std::any_cast<int>(value) == 7);
    value.reset();
    assert(!value.has_value());

    const auto answer = leetcode_two_sum_any({2, std::string{"skip"}, 7, 11}, 9);
    assert(answer.first == 0U && answer.second == 2U);

    const Metadata metadata{{"retries", 3}, {"owner", std::string{"ops"}}, {"enabled", true}};
    assert(practical_render_metadata(metadata, "owner") == "ops");
    assert(practical_render_metadata(metadata, "enabled") == "true");
    assert(practical_render_metadata(metadata, "unknown") == "missing");

    std::cout << "any 測試完成\n";
}

/*
 * 【常見陷阱】
 * - any_cast<T>(value) 型別不符丟 bad_any_cast；any_cast<T>(&value) 回 nullptr，適合探測。
 * - `const char*` 與 std::string 是不同型別；存入 "text" 後 any_cast<string> 不會自動轉換。
 * - C++20 any 無法直接保存 move-only unique_ptr，因被保存型別須 CopyConstructible。
 * - 是否 heap allocation 是實作品質與物件大小問題，不應假定一定/一定不配置。
 *
 * 【面試段落】any、variant、optional：any=未知任意型；variant=已知多選一；optional=零或一個 T。
 * 【練習】把 Metadata 改成 variant<int,string,bool>，比較 exhaustive handling 與錯誤時機。
 */
