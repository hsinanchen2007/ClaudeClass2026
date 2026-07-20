/*
 * C++20 starts_with：檢查前綴，回傳 bool
 *
 * overload 接 string_view 或 char；語意等同「長度足夠且前段相等」，不配置 substring。
 * 複雜度 O(prefix.size())。空 prefix 永遠成立。比較區分大小寫並按 char code units。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void basic_demo() {
    const std::string url = "https://example.test";
    assert(url.starts_with("https://"));
    assert(url.starts_with('h'));
    assert(url.starts_with(""));
    assert(!url.starts_with("http://"));
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1961. Check If String Is a Prefix of Array（字串是否為陣列前綴）
// 題目：判斷 target 是否恰由 words 的前 k 個完整單字串接而成；iloveleetcode 對 i/love/leetcode... 為 true。
// 為何使用本章主題：每加入一個 word 後以 target.starts_with(built) 及早拒絕已偏離 target 的前綴，
//       避免繼續無效串接；命中完整 target 時立即成功。
// 思路：1. built 從空開始；2. 逐 word 附加；3. 完全相等回 true；4. 不再是 target 前綴就回 false。
// 複雜度：此實作最壞時間 O(T^2)、額外空間 O(T)，T 是累積字元數，反覆前綴比較造成平方上界。
// 易錯點：只能在完整 word 邊界判成功；built 超過 target 時 starts_with 會失敗，空 target 規格需另訂。
// -----------------------------------------------------------------------------
bool leetcode_is_prefix_string(const std::string& target,
                               const std::vector<std::string>& words) {
    std::string built;
    for (const std::string& word : words) {
        built += word;
        if (built == target) return true;
        if (!target.starts_with(built)) return false;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】HTTP 路徑前綴路由
// 情境：`/api/admin/` 應送 admin，其他 `/api/` 送 api，`/static/` 送 static，剩餘 not-found。
// 為何使用本章主題：starts_with 對固定 literal 做零配置前綴判斷；相較 regex 成本與語意更直接。
// 設計：1. 先檢查最具體 admin 前綴；2. 再檢查一般 api；3. 再 static；4. 否則 fallback。
// 成本：固定三規則下時間與空間 O(1)；一般 R 條平均前綴 P 時為 O(R*P)。
// 上線注意：規則順序是契約；需處理 URL decode、大小寫、重複斜線與 path traversal，不能只看 raw prefix。
// -----------------------------------------------------------------------------
std::string_view practical_route(const std::string_view path) {
    if (path.starts_with("/api/admin/")) return "admin";
    if (path.starts_with("/api/")) return "api";
    if (path.starts_with("/static/")) return "static";
    return "not-found";
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_is_prefix_string("iloveleetcode", {"i", "love", "leetcode", "apples"}));
    assert(!leetcode_is_prefix_string("a", {"aa", "aaaa"}));
    assert(practical_route("/api/admin/users") == "admin");
    assert(practical_route("/api/items") == "api");
    assert(practical_route("/x") == "not-found");
    std::cout << "starts_with: tests passed\n";
}

/*
 * 【陷阱】starts_with("GET") 也會接受 "GETTING"；若判 token，還要檢查邊界字元或長度。
 * 【面試】相較 `find(prefix)==0`，starts_with 更直接且不混淆「搜尋」與「前綴」。
 * 【生命週期】傳入 string_view 只在呼叫內使用，沒有保存，因此 temporary literal 安全。
 * 【練習】實作 `starts_with_word(text,prefix)`，要求下一字元為空白或已到結尾。
 */

/*
 * 【前綴檢查決策】
 * - C++20：starts_with，零配置且語意直接。
 * - 舊標準：size guard + compare(0,prefix.size(),prefix)==0。
 * - 需要前綴位置以外的匹配：find。
 * - protocol routing：先比最具體/最長前綴，避免一般規則遮蔽特例。
 * 空 prefix 永遠 true；若 API 把空 prefix 當設定錯誤，應在呼叫前驗證。
 * 【面試題】starts_with 是不是 regex anchor `^`？效果類似簡單 literal 前綴，但沒有
 * regex 語法、capture 或 character class，成本與風險更低。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'starts_with.cpp' -o '/tmp/codex_cpp_C_String_starts_with' && '/tmp/codex_cpp_C_String_starts_with'
//
// === 預期輸出（節錄）===
// starts_with: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
