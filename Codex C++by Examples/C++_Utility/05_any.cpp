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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1. Two Sum（兩數之和）
// 題目：在整數陣列中找兩個相異索引，使其值相加為 target；此例 [2,"skip",7,11]、9 得 [0,2]。
// 為何使用本章主題：本函式刻意把輸入改為 vector<any>，以 pointer any_cast 略過非 int；
// 正式題目只有 int，不應使用 any，這是展示執行期型別檢查代價的教學改寫。
// 思路：逐項嘗試 any_cast<int>；成功才查互補值；命中回兩個原始索引，否則記錄目前整數。
// 複雜度：平均時間 O(N)、額外空間 O(K)，N 是 values 數量、K 是其中 int 的數量。
// 易錯點：型別必須精確是 int，long 不會自動轉換；非 int 被靜默略過，且減法仍可能溢位。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】外掛 metadata 顯示邊界
// 情境：外掛可附加未知欄位，顯示層只支援 int、string、bool，並區分缺少與不支援型別。
// 為何使用本章主題：any 容許外掛在不修改中央型別清單下加入值；相較 variant 更開放，
// 但所有 any_cast 集中在邊界函式，避免執行期型別契約散落整個系統。
// 設計：先查 key；依序用 pointer any_cast 探測三種支援型別；無匹配則回 unsupported。
// 成本：平均查找 O(1)，三次以內型別探測 O(1)，字串輸出成本 O(L) 且 any 可能使用 heap。
// 上線注意：要限制 metadata 大小與允許型別，避免敏感欄位外洩；錯誤最好回結構化狀態而非字串。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '05_any.cpp' -o '/tmp/codex_cpp_C_Utility_05_any' && '/tmp/codex_cpp_C_Utility_05_any'
//
// === 預期輸出（節錄）===
// any 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
