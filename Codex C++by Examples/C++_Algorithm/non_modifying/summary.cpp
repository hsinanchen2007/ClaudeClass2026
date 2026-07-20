/*
 * Non-modifying Algorithms：面試前完整速查章
 * ===========================================
 *
 * 一、分類與選擇表
 * find/find_if        找第一個值/條件，O(N)
 * adjacent_find       找第一對相鄰關係，O(N)
 * find_first_of       第一範圍中第一個屬於候選集合，最壞 O(N*M)
 * search              第一個子序列，generic 最壞 O(N*M)
 * find_end            最後一個子序列，generic 最壞 O(N*M)
 * search_n            第一段連續 count 個值，O(N)
 * count/count_if      計數，固定掃完整範圍 O(N)
 * all/any/none_of     量詞判斷，可短路，最壞 O(N)
 * equal               同位置逐項相等，可短路
 * mismatch            第一個不同位置，可短路
 * for_each/_n         對每項呼叫，不能 early break
 *
 * 二、共同契約
 * 這些演算法不改範圍元素（除非 for_each callable 刻意接非 const reference），也不
 * 保存 iterator。回傳 end 是正常「找不到」，解參考前必須比較。predicate 應純粹，
 * 不依賴呼叫次數、短路位置或平行執行順序。
 *
 * 三、空範圍語意
 * find/search_n 等找不到回 end；search 的空 needle 回 begin；find_end 空 needle 回 end。
 * all_of(empty)=true、any_of(empty)=false、none_of(empty)=true。這些數學語意未必符合
 * 業務「沒有資料=未知」，API 層要先處理空值。
 *
 * 四、範圍長度安全
 * 優先使用同時接兩個 end 的 equal/mismatch overload。classic binary transform、
 * 三 iterator equal/mismatch 不知道第二範圍長度，呼叫者若給短範圍會越界。
 * for_each_n 同樣不知道 end，外部 count 先驗證。
 *
 * 五、複雜度陷阱
 * 線性 find/count 放進 N 次 loop 會成 O(N^2)。大量 key 查詢改建 hash/tree index；
 * 已排序資料使用 binary search。大 pattern 搜尋考慮 KMP/Boyer-Moore/searcher。
 * 一次要很多 count 指標時，一個 loop 聚合可減少 memory bandwidth。
 *
 * 六、字串與 byte 語意
 * std::string 演算法比較 byte；UTF-8 大小寫/正規化/字素需 Unicode library。
 * std::tolower 參數先轉 unsigned char。protocol magic 搜尋不是完整 framing/security。
 * 密碼學秘密比較不能用會短路的 equal，需要 constant-time primitive。
 *
 * 七、易錯陷阱
 * - end iterator 解參考；把 size sentinel 當合法 index。
 * - 忘記 empty pattern 的特殊結果。
 * - find_first_of 兩範圍角色看反。
 * - 以為 adjacent_find 能找未排序全域重複。
 * - 用 stateful side effect predicate，卻忘了演算法可能短路。
 * - 將 O(N^2) 教學版直接帶進大資料正式環境。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// LeetCode 28：第一個 substring index；保留為整合 demo 的搜尋核心。
int leetcode_first_occurrence(const std::string& text,
                              const std::string& pattern) {
    const auto it = std::search(text.begin(), text.end(),
                                pattern.begin(), pattern.end());
    return it == text.end()
               ? -1
               : static_cast<int>(std::distance(text.begin(), it));
}

struct LogLine {
    int status;
    std::string message;
};

struct AuditResult {
    std::size_t errors;
    std::optional<std::size_t> first_secret_line;
    bool all_status_valid;
};

// 實務整合：一次示範 count_if、find_if、all_of，輸入保持不變。
AuditResult practical_audit_logs(const std::vector<LogLine>& logs) {
    const auto errors = std::count_if(logs.begin(), logs.end(),
                                      [](const LogLine& line) {
                                          return line.status >= 500;
                                      });
    const auto secret = std::find_if(logs.begin(), logs.end(),
                                     [](const LogLine& line) {
                                         return line.message.find("SECRET") !=
                                                std::string::npos;
                                     });
    const bool valid = std::all_of(logs.begin(), logs.end(),
                                   [](const LogLine& line) {
                                       return line.status >= 100 && line.status <= 599;
                                   });
    std::optional<std::size_t> secret_index;
    if (secret != logs.end()) {
        secret_index = static_cast<std::size_t>(std::distance(logs.begin(), secret));
    }
    return {static_cast<std::size_t>(errors),
            secret_index,
            valid};
}

int main() {
    assert(leetcode_first_occurrence("sadbutsad", "sad") == 0);
    assert(leetcode_first_occurrence("leetcode", "leeto") == -1);
    assert(leetcode_first_occurrence("abc", "") == 0);

    const std::vector<LogLine> logs{{200, "ok"},
                                    {503, "retry"},
                                    {200, "SECRET token"}};
    const auto result = practical_audit_logs(logs);
    assert(result.errors == 1U);
    assert(result.first_secret_line.has_value() && *result.first_secret_line == 2U);
    assert(result.all_status_valid);
    assert(logs[0].message == "ok");

    const auto at_first = practical_audit_logs({{200, "SECRET at index zero"}});
    assert(at_first.first_secret_line.has_value() &&
           *at_first.first_secret_line == 0U);

    const auto absent = practical_audit_logs({{200, "ordinary entry"}});
    assert(!absent.first_secret_line.has_value());

    const auto empty = practical_audit_logs({});
    assert(empty.errors == 0U && !empty.first_secret_line.has_value());
    assert(empty.all_status_valid);  // vacuous truth；業務層可另標 unknown。
    std::cout << "Non-modifying algorithms 整合複習完成\n";
}

/*
 * 八、面試 Q&A
 * Q: find 與 binary_search 怎麼選？
 * A: 任意未排序範圍 find O(N)；已排序且多次查詢可 binary search O(log N)。
 * Q: search 與 find_first_of 差別？
 * A: search 要整段連續 pattern；find_first_of 只要命中候選集合任一元素。
 * Q: equal 與 mismatch 關係？
 * A: mismatch 找第一差異；若兩邊同時到 end 才完整 equal。
 * Q: all_of 空範圍為何 true？
 * A: 沒有反例；這叫 vacuous truth。業務 unknown 需另建模。
 * Q: for_each 能 break 嗎？
 * A: 不能正常 early break；需要 find_if/loop，靠丟例外控制流程是壞設計。
 * Q: count 回什麼型別？
 * A: iterator difference_type，不保證是 int。
 *
 * 九、LeetCode 解題模式
 * - duplicate：find 前綴是 O(N^2) baseline，hash set 平均 O(N)。
 * - anagram：sort+equal O(N log N)，固定 alphabet frequency O(N)。
 * - substring：std::search 可快速交付，面試可能要求 KMP。
 * - Buddy Strings：mismatch 找差異位置，完全相同另檢查重複字元。
 * - consecutive run：search_n 可問存在性，最大長度用單趟 current/best。
 *
 * 十、實務選型
 * 單次小資料線性演算法最清楚；高頻查詢先建 index。資料從 stream/input iterator
 * 而來時不能任意重掃，應一次聚合。監控 predicate 要避免昂貴 regex 重複編譯；
 * 將 regex/searcher 建在 loop 外。安全敏感比對使用專門函式。
 *
 * 十一、iterator 與 lifetime
 * 回傳 iterator 指向原容器位置，不擁有資料；原容器銷毀後懸空。vector reallocation
 * 後失效。若跨 API 邊界，回 optional<size_t>、optional value 或 owning copy 通常
 * 更安全；裸 size_t 無法同時讓 0 表示合法首項，又表示「未找到」。
 * string_view/temporary range 也要特別防止 view/iterator 比來源活得久。
 *
 * 十二、測試清單
 * 空範圍、單值、命中頭/中/尾、完全找不到、pattern 比來源長、空 pattern、重複匹配、
 * 大小寫/UTF-8、第二範圍較短、predicate 短路、副作用禁止、size sentinel 都要測。
 * property test 可將 std 演算法結果與簡單 reference loop 比較。
 *
 * 十三、練習
 * 1. 用 KMP 重寫 leetcode_first_occurrence，保留同一組測試。
 * 2. practical_audit_logs 改成單 loop 同時算三項，量測大資料差異。
 * 3. 將 AuditResult 改成 expected，區分「未命中」與「輸入/解析失敗」。
 * 4. 寫 constant-time byte equality，並說明不能自行發明 production crypto。
 * 5. 用 ranges::find_if 回 borrowed iterator，研究 temporary range 的 dangling 型別。
 *
 * 最後速記：先問「找值、找集合成員、找子序列、找連續 run、計數、量詞、差異」；
 * 再確認空範圍、第二範圍長度與複雜度，幾乎就能選對 non-modifying algorithm。
 */
