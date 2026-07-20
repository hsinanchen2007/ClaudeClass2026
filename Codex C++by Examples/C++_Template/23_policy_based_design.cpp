/*
 * 第 23 章：Policy-based design
 *
 * Policy 是注入行為的小型型別。主類別負責流程，policy 負責可替換決策，例如排序、
 * 重試、記錄、配置。相較大量 bool 旗標，policy 名稱清楚且可在編譯期消除不用的路徑；
 * 相較 virtual，沒有執行期 dispatch，但更換 policy 會產生不同型別。
 */

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct LowestFirst {
    template <typename T>
    bool operator()(const T& left, const T& right) const { return left < right; }
};

struct HighestFirst {
    template <typename T>
    bool operator()(const T& left, const T& right) const { return left > right; }
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 215. Kth Largest Element in an Array（陣列中的第 K 個最大元素）
// 題目：找排序後第 k 大值；[3,2,1,5,6,4]、k=2 的答案是 5。
// 為何使用本章主題：HighestFirst policy 讓 std::sort 產生降冪結果，k-1 即第 k 大；
// LowestFirst 展示相同流程可改成第 k 小，但不是原題需求，也不是線性時間最佳解。
// 思路：按值取得輸入副本；依 order policy 排序；回傳索引 k-1 的元素。
// 複雜度：時間 O(N log N)、輸入副本空間 O(N)，N 是 values 長度；sort 另用實作定義堆疊空間。
// 易錯點：必須有 1<=k<=N，release 移除 assert 後無保護；policy 方向決定第 k 大或小。
// -----------------------------------------------------------------------------
template <typename OrderPolicy>
int leetcode_kth(std::vector<int> values, std::size_t k, OrderPolicy order = {}) {
    assert(k >= 1U && k <= values.size());
    std::sort(values.begin(), values.end(), order);
    return values[k - 1U];
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可組合的請求重試與記錄策略
// 情境：網路操作要獨立選擇嘗試次數與觀測方式，測試中不實際 sleep 或連線。
// 為何使用本章主題：RetryPolicy 與 LogPolicy 各封裝單一決策，RequestRunner 在編譯期組合；
// 相較多個 bool/runtime if，型別名稱直接表達組態且空 logger 可受 EBO 最佳化。
// 設計：每輪先通知 logger；呼叫 operation(attempt)；optional 有值立即回傳，耗盡則 nullopt。
// 成本：最多 O(A) 次操作與記錄、額外空間 O(1)，A 是 policy 的 max_attempts；I/O 成本主導。
// 上線注意：需加入 backoff、timeout、取消與冪等性；CountingLog 的裸指標必須有效且並行時需同步。
// -----------------------------------------------------------------------------
struct OneAttempt {
    static constexpr int max_attempts = 1;
};

struct ThreeAttempts {
    static constexpr int max_attempts = 3;
};

struct SilentLog {
    void attempt(int) const noexcept {}
};

struct CountingLog {
    int* counter{};
    void attempt(int) const { ++(*counter); }
};

template <typename RetryPolicy, typename LogPolicy>
class RequestRunner : private LogPolicy {
public:
    explicit RequestRunner(LogPolicy logger = {}) : LogPolicy(std::move(logger)) {}

    template <typename Operation>
    std::optional<std::string> run(Operation operation) {
        for (int attempt = 1; attempt <= RetryPolicy::max_attempts; ++attempt) {
            LogPolicy::attempt(attempt);
            if (auto result = operation(attempt)) {
                return result;
            }
        }
        return std::nullopt;
    }
};

void practical_retry_test() {
    int logged = 0;
    RequestRunner<ThreeAttempts, CountingLog> runner(CountingLog{&logged});
    const auto result = runner.run([](int attempt) -> std::optional<std::string> {
        if (attempt == 3) {
            return "remote-ok";
        }
        return std::nullopt;
    });
    assert(result == "remote-ok");
    assert(logged == 3);
}

int main() {
    assert(leetcode_kth({3, 2, 1, 5, 6, 4}, 2U, HighestFirst{}) == 5);
    assert(leetcode_kth({3, 2, 1, 5, 6, 4}, 2U, LowestFirst{}) == 2);

    practical_retry_test();

    RequestRunner<OneAttempt, SilentLog> once;
    assert(!once.run([](int) -> std::optional<std::string> { return std::nullopt; }));

    std::cout << "policy-based design 測試完成\n";
}

/*
 * 【設計準則】policy 應聚焦單一決策、介面小、最好是無狀態；主類別記錄流程不變量。
 * 【EBO】空 policy 以 private inheritance 儲存時可享 empty base optimization，不占額外 byte（非絕對保證）。
 * 【陷阱】policy 組合數過多會增加實體/code size；真正執行期可變的設定不宜全模板化。
 * 【面試】Strategy pattern 與 policy 差異？前者常用執行期多型，後者通常編譯期組合。
 * 【練習】新增 ExponentialBackoff policy，但測試中不要真的 sleep，改注入 clock/sleeper。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '23_policy_based_design.cpp' -o '/tmp/codex_cpp_C_Template_23_policy_based_design' && '/tmp/codex_cpp_C_Template_23_policy_based_design'
//
// === 預期輸出（節錄）===
// policy-based design 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
