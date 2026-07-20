/*
 * operator+=：把內容附加到既有 std::string
 *
 * 【基本模型】operator+= 保留既有 prefix，將右側內容接到尾端，並回傳同一個 string&。
 * 【overload】可附加 string、C 字串、單一 char、initializer_list，以及合格的 string_view-like 型別。
 * 【選型】簡單串接用 +=；需要 substring、pointer+count 或 iterator range 時用 append。
 * 【選型】混合多種型別格式化時，format/stream 通常比手動逐段轉字串清楚。
 * 【複雜度】附加 n 個字元需 O(n) 複製；反覆單字元成長通常具有攤銷 O(1) 行為。
 * 【容量】已知最終大小時先 reserve 可減少重新配置，但不要每輪只 reserve 下一個大小。
 * 【失效】若重新配置，舊 iterator/reference/pointer（含 c_str/data）全部失效並須重取。
 * 【例外安全】單次 += 可能丟 length_error/bad_alloc；多次 += 並不構成整體交易。
 * 【易錯】C 字串 overload 依 NUL 判斷結尾；二進位資料應使用帶長度的 append。
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

void basic_demo() {
    std::string line;
    line.reserve(16U);
    line += "status";
    line += '=';
    line += {'o', 'k'};
    assert(line == "status=ok");
    std::string& same_object = (line += '!');
    assert(&same_object == &line && line == "status=ok!");
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1768. Merge Strings Alternately（交錯合併字串）
// 題目：從 first 開始輪流取兩字串同索引字元，較長輸入剩餘部分接到尾端；"ab"、"pqrs" 得 "apbqrs"。
// 為何使用本章主題：operator+= char 直接擴張同一答案字串，先 reserve N+M 後避免連鎖 operator+
//       產生不必要的中間字串。
// 思路：1. 預留總長；2. 走到兩者最大長度；3. 各自索引合法時依 first、second 順序 +=。
// 複雜度：時間 O(N+M)、額外空間 O(N+M)，N、M 是兩輸入長度。
// 易錯點：兩個長度 guard 必須分開；總長相加需考慮 size overflow，且 += 可能使舊 pointer 失效。
// -----------------------------------------------------------------------------
std::string leetcode_merge_alternately(const std::string& first, const std::string& second) {
    std::string answer;
    answer.reserve(first.size() + second.size());
    const std::size_t longest = first.size() > second.size() ? first.size() : second.size();
    for (std::size_t i = 0U; i < longest; ++i) {
        if (i < first.size()) {
            answer += first[i];
        }
        if (i < second.size()) {
            answer += second[i];
        }
    }
    return answer;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】查詢 URL 逐欄組裝
// 情境：base 尚未含 query，fields 已完成 URL encoding，要產生 `/search?q=cpp&page=2`。
// 為何使用本章主題：operator+= 可依欄位逐段附加 separator、key、`=`、value；相較每輪 operator+
//       建立暫存字串，更適合長度逐步成長的 builder。
// 設計：1. 複製 base；2. 首欄用 `?`、其後用 `&`；3. 依序附加 key/value；4. 空 fields 原樣回傳。
// 成本：時間 O(L)、額外空間 O(L)，L 是最終 URL 長度；未預估容量時可能多次重新配置。
// 上線注意：函式不做 URL encoding，也未處理 base 已有 `?`、空 key、重複欄位或長度上限。
// -----------------------------------------------------------------------------
std::string practical_make_query_url(const std::string& base,
                                     const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string result = base;
    char separator = '?';
    for (const auto& [key, value] : fields) {
        result += separator;
        result += key;
        result += '=';
        result += value;
        separator = '&';
    }
    return result;
}

}  // namespace

int main() {
    basic_demo();
    assert(leetcode_merge_alternately("abc", "pqr") == "apbqcr");
    assert(leetcode_merge_alternately("ab", "pqrs") == "apbqrs");
    assert(leetcode_merge_alternately("", "xy") == "xy");
    assert(practical_make_query_url("/search", {{"q", "cpp"}, {"page", "2"}}) ==
           "/search?q=cpp&page=2");
    assert(practical_make_query_url("/health", {}) == "/health");
    std::cout << "operator+=: tests passed\n";
}

/*
 * 【LeetCode 契約】兩輸入可為空且保持不變；輸出依索引交錯，總時間與空間 O(n+m)。
 * 【實務契約】base 尚未含 query，fields 已完成 URL encoding；空 fields 原樣回傳 base。
 * 【實務易錯】+= 不會自動 URL encode，也不會替數字轉字串，分隔符規則由 caller 定義。
 * 【生命週期】本例回傳擁有資料的新 string，不回傳指向 base 或 fields 的 view。
 * 【例外邊界】practical_make_query_url 在區域 result 上建構；失敗不會修改輸入參數。
 * 【面試追問】+= 與 append 差異？+= 語法短；append 提供 substring/count/range 等精細形式。
 * 【面試追問】reserve 後 pointer 永遠有效嗎？只有沒有後續重新配置且原字串仍存活才成立。
 * 【練習】加入 URL encoding，並定義 base 已含 query 時要接 '&' 還是拒絕輸入。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'operator_plus_eq.cpp' -o '/tmp/codex_cpp_C_String_operator_plus_eq' && '/tmp/codex_cpp_C_String_operator_plus_eq'
//
// === 預期輸出（節錄）===
// operator+=: tests passed
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
