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

// LeetCode 1768（Merge Strings Alternately）。
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

// 實務：逐段建立 URL；此例假設輸入已做 URL encoding。
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
