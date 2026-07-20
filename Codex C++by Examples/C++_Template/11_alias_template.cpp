/*
 * 第 11 章：別名模板（alias template）
 *
 * using Name = ComplexType 可替型別取別名；加上 template 後，就能建立一族簡潔名稱。
 * 它不建立新型別：UserIds 與原本 vector<int> 完全相同。若需要強型別隔離，應建立 class。
 */

/*
 * 【離線教材：模型與取捨】
 * 1. alias template 是編譯期名稱替換；List<int> 與 std::vector<int> 是同一型別而非子類別。
 * 2. 基本語法是 `template<class T> using Name = Pattern<T>`，右側仍可含多層模板運算。
 * 3. using 比 typedef 更能直接表達參數化別名，也讓 `typename Trait<T>::type` 集中成 `_t` 名稱。
 * 4. 別名適合隱藏重複拼字與表達領域角色；若必須阻止 UserId/OrderId 混用，應用 wrapper。
 * 5. 公開 API 若暴露 vector/unordered_map 別名，仍把容器選擇、失效規則與 ABI 一起暴露出去。
 * 6. Alias 本身沒有 runtime 成本，也不會額外配置；真正成本完全來自右側所代表的型別。
 * 7. 它也不會減少模板實體化或 code size；List<int> 仍會實體化所需的 vector<int> 成員。
 * 8. 短名稱可降低解析噪音，但過度包裝會讓錯誤訊息同時出現別名與展開後型別。
 * 9. Lookup 使用 unordered_map：平均查找 O(1)、最壞 O(n)，rehash 會使 iterator 失效。
 * 10. Two Sum 平均時間 O(n)、額外空間 O(n)；碰撞攻擊下 unordered_map 最壞可到 O(n^2)。
 * 11. 本例用 long long 計算 target-value，避免 int 邊界相減先發生 signed overflow。
 * 12. LeetCode 契約是回傳兩個不同索引；無解時以 {size,size} 表示，呼叫端必須檢查。
 * 13. 重複值採「先查再插」，所以 [3,3], target=6 能找到兩個不同位置。
 * 14. List 擁有 Job；practical_count_enabled 只借用 const reference，不延長 map 或字串生命週期。
 * 15. Predicate<Job> 是 std::function，可能配置 heap 並經間接呼叫；固定 lambda 通常更小更快。
 * 16. 因此別名不能把成本洗掉：命名 Predicate 不代表它等同零成本的函式模板參數。
 * 17. Alias template 不能完整或偏特化；需要分類選型時先特化 helper class，再暴露 helper_t。
 * 18. 診斷時先展開 alias 看真正型別，尤其 allocator、hash 與 comparator 是否符合預期。
 * 19. 面試追問：using 能否建立強型別？不能；隱式轉換與 overload resolution 都看底層型別。
 * 20. 面試追問：為何仍保留 std::function alias？需要儲存不同 callable 的統一 owning 介面時才選。
 */

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
using List = std::vector<T>;

template <typename Key, typename Value>
using Lookup = std::unordered_map<Key, Value>;

template <typename T>
using Predicate = std::function<bool(const T&)>;

// LeetCode 1：Two Sum。別名縮短簽名，但演算法仍是平均 O(n)、空間 O(n)。
using IndexPair = std::pair<std::size_t, std::size_t>;

IndexPair leetcode_two_sum(const List<int>& values, int target) {
    Lookup<long long, std::size_t> index_by_value;
    for (std::size_t i = 0; i < values.size(); ++i) {
        const long long wanted =
            static_cast<long long>(target) - static_cast<long long>(values[i]);
        const auto found = index_by_value.find(wanted);
        if (found != index_by_value.end()) {
            return {found->second, i};
        }
        index_by_value.emplace(static_cast<long long>(values[i]), i);
    }
    return {values.size(), values.size()};
}

struct Job {
    std::string id;
    bool enabled{};
};

// 實務：將容易讀錯的巢狀模板集中命名。
using JobsByOwner = Lookup<std::string, List<Job>>;

std::size_t practical_count_enabled(const JobsByOwner& jobs, const std::string& owner) {
    const auto found = jobs.find(owner);
    if (found == jobs.end()) {
        return 0;
    }
    const Predicate<Job> enabled = [](const Job& job) { return job.enabled; };
    std::size_t count = 0;
    for (const Job& job : found->second) {
        count += enabled(job) ? 1U : 0U;
    }
    return count;
}

int main() {
    static_assert(std::is_same_v<List<int>, std::vector<int>>);

    [[maybe_unused]] const auto answer = leetcode_two_sum({2, 7, 11, 15}, 9);
    assert(answer == IndexPair(0U, 1U));
    assert(leetcode_two_sum({3, 3}, 6) == IndexPair(0U, 1U));
    assert(leetcode_two_sum({std::numeric_limits<int>::min(),
                             std::numeric_limits<int>::max()},
                            -1) == IndexPair(0U, 1U));
    assert(leetcode_two_sum({1, 2}, 99) == IndexPair(2U, 2U));

    const JobsByOwner jobs{{"ops", {{"backup", true}, {"cleanup", false}}}};
    assert(practical_count_enabled(jobs, "ops") == 1U);
    assert(practical_count_enabled(jobs, "missing") == 0U);

    std::cout << "alias template 測試完成\n";
}

/*
 * 【限制】alias template 不能直接完整或偏特化；可先特化 helper struct，
 *         再以 `using result_t = typename helper<T>::type` 暴露。
 * 【陷阱】`using UserId = int` 不防止把 OrderId 傳進去；強型別需 wrapper class。
 * 【面試】typedef 與 using 差異？一般別名皆可；只有 using 能自然表達 alias template。
 * 【練習】建立 Result<T> = variant<T, Error>，並思考它是否該是 alias 或正式 class。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '11_alias_template.cpp' -o '/tmp/codex_cpp_C_Template_11_alias_template' && '/tmp/codex_cpp_C_Template_11_alias_template'
//
// === 預期輸出（節錄）===
// alias template 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
