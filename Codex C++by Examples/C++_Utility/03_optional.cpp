/*
 * 第 03 章：std::optional
 *
 * optional<T> 表示「可能有一個 T，也可能沒有」，取代魔法 sentinel（-1、空字串）與
 * 裸 pointer 的部分用途。它在物件內直接保存 T 的 storage，通常不做 heap allocation。
 * 適合「缺值是正常狀態」；若需要錯誤原因，C++23 std::expected<T,E> 更合適。
 */

#include <cassert>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

std::optional<int> basic_parse_positive(const std::string& text) {
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (value > 0 && consumed == text.size()) {
            return value; // 隱式建構 optional<int>
        }
    } catch (const std::exception&) {
        // 本教學將格式錯誤與非正值都表示為 no value。
    }
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 278. First Bad Version（第一個錯誤的版本）
// 題目：版本 1..n 從某版起全部為 bad，找出第一版；例如 n=10 且 4 起為 bad，答案是 4。
// 為何使用本章主題：原題保證存在 bad 版本；此泛化介面用 optional 額外表達 count 無效或範圍內
// 完全沒有 bad，避免以 -1 當 magic sentinel。
// 思路：在 1..count 二分；bad 時縮右界，good 時移左界；收斂後再驗證候選是否真的 bad。
// 複雜度：時間 O(log N)、額外空間 O(1)，N 是 count，另有一次最終 is_bad 查詢。
// 易錯點：count<=0 要回 nullopt；is_bad 必須單調且 std::function 不可為空，否則會丟例外。
// -----------------------------------------------------------------------------
std::optional<int> leetcode_first_bad_version(int count,
                                              const std::function<bool(int)>& is_bad) {
    int left = 1;
    int right = count;
    while (left < right) {
        const int mid = left + (right - left) / 2;
        if (is_bad(mid)) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    if (count <= 0 || !is_bad(left)) {
        return std::nullopt;
    }
    return left;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可缺少的環境設定查詢
// 情境：服務從環境設定表查 MODE、PORT 等鍵，未設定是可由呼叫端套預設值的正常狀態。
// 為何使用本章主題：optional 明確區分「鍵不存在」與「存在但值是空字串」；相較例外或特殊字串，
// 呼叫端可用 has_value/value_or 直接處理缺值分支。
// 設計：在 unordered_map 查 key；未命中回 nullopt；命中則複製字串到 engaged optional。
// 成本：平均查找 O(1)、回傳複製 O(L)，L 是設定值長度；惡劣碰撞時查找可到 O(N)。
// 上線注意：敏感值不可寫入 log；需區分缺值、空值與格式錯誤，熱更新時還要同步環境快照。
// -----------------------------------------------------------------------------
using Environment = std::unordered_map<std::string, std::string>;

std::optional<std::string> practical_find_config(const Environment& environment,
                                                 const std::string& key) {
    const auto found = environment.find(key);
    if (found == environment.end()) {
        return std::nullopt;
    }
    return found->second;
}

int main() {
    const auto parsed = basic_parse_positive("42");
    assert(parsed.has_value());
    assert(*parsed == 42); // operator* 不檢查；先確認 engaged
    assert(!basic_parse_positive("bad"));
    assert(!basic_parse_positive("42ms")); // stoi 可部分解析，必須核對 consumed
    assert(basic_parse_positive("-2").value_or(10) == 10);

    const auto bad = leetcode_first_bad_version(10, [](int version) { return version >= 4; });
    assert(bad == 4);
    assert(!leetcode_first_bad_version(5, [](int) { return false; }));

    const Environment environment{{"MODE", "production"}, {"PORT", "8080"}};
    const auto mode = practical_find_config(environment, "MODE");
    assert(mode && *mode == "production");
    assert(practical_find_config(environment, "TOKEN").value_or("missing") == "missing");

    std::optional<std::string> reusable;
    reusable.emplace(3U, 'x'); // 就地建構 "xxx"
    assert(reusable == "xxx");
    reusable.reset();          // 銷毀內含 string，回到 disengaged
    assert(!reusable);

    std::cout << "optional 測試完成\n";
}

/*
 * 【常見陷阱】
 * - value() 在無值時丟 bad_optional_access；operator* 在無值時是 UB。
 * - optional<bool>{false} 是「有值且值為 false」，`if(optional)` 檢查的是有沒有值。
 * - `optional<T&>` 不存在於 C++20；可用 optional<reference_wrapper<T>>，但要管理 referent 壽命。
 * - optional<unique_ptr<T>> 有三態（無 optional、有空 ptr、有非空 ptr），通常過度複雜。
 *
 * 【面試段落】optional 是否配置 heap？通常 T 直接內嵌，sizeof 會包含 T 加狀態/對齊成本。
 * 【練習】把 parse 改為 expected<int,string>，保留 invalid_argument/out_of_range 原因。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_optional.cpp' -o '/tmp/codex_cpp_C_Utility_03_optional' && '/tmp/codex_cpp_C_Utility_03_optional'
//
// === 預期輸出（節錄）===
// optional 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
