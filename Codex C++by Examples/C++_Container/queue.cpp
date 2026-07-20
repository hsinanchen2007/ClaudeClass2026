// ============================================================================
// queue：FIFO 容器轉接器
// ============================================================================
// 【基本模型】queue 是 FIFO 容器轉接器，只允許從 back 加入、從 front 觀察與移除。
// 【模板】std::queue<T, Container> 預設以 deque<T> 儲存；底層需提供 front/back/push_back/pop_front。
// 【API】push 有 const T& 與 T&& overload；emplace 直接建構尾端元素；pop 不回傳被刪元素。
// 【存取】front/back 回傳底層元素 reference；empty/size 只查詢狀態，queue 本身沒有 iterator。
// 【選型】純 FIFO 用 queue；需要巡覽或刪中間用 deque；按權重取出則用 priority_queue。
// 【複雜度】複雜度繼承底層容器；預設 deque 的兩端操作通常為常數時間。
// 【生命週期】pop 會銷毀 front；所有指向該元素的 pointer/reference 立即失效。
// 【失效】push 對既有 reference/iterator 的影響也由底層容器決定，轉接器不增加保證。
// 【前置條件】空 queue 上呼叫 front/back/pop 是 undefined behavior，必須先檢查 empty。
// 【例外安全】push/emplace 或取值的 copy/move 都可能拋例外；應先成功取得值，再呼叫 pop。

#include <cassert>
#include <cstddef>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void basic_demo()
{
    std::queue<int> requests;
    requests.push(100);
    requests.emplace(200);
    assert(requests.front() == 100 && requests.back() == 200);
    requests.pop();
    assert(requests.front() == 200 && requests.size() == 1U);
    requests.pop();
    assert(requests.empty());
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1700. Number of Students Unable to Eat Lunch（無法吃午餐的學生數量）
// 題目：學生依序只拿偏好的 0/1 三明治，拒絕者排到隊尾；例如 [1,1,1,0,0,1] 配給後剩 3 人。
// 為何使用本章主題：queue 直接表達學生 FIFO 輪轉與 front sandwich 的消耗順序；這是直觀模擬而非最佳計數解。
// 思路：學生全部入隊；匹配就 pop 並前進 sandwich；不匹配就移到尾端；完整一輪無匹配即停止。
// 複雜度：最壞時間 O(N^2)、額外空間 O(N)，N 為學生數；按偏好計數可降為 O(N)。
// 易錯點：兩陣列需等長且值域為 0/1；refusals 在成功後清零；空 queue 不可讀 front。
// -----------------------------------------------------------------------------
std::size_t unable_to_eat(const std::vector<int>& students,
                          const std::vector<int>& sandwiches)
{
    if (students.size() != sandwiches.size()) {
        throw std::invalid_argument("students and sandwiches must have equal sizes");
    }

    std::queue<int> line;
    for (const int preference : students) {
        line.push(preference);
    }

    std::size_t sandwich = 0;
    std::size_t refusals = 0;
    while (!line.empty() && refusals < line.size()) {
        if (line.front() == sandwiches.at(sandwich)) {
            line.pop();
            ++sandwich;
            refusals = 0;
        } else {
            const int student = line.front();
            line.pop();
            line.push(student);
            ++refusals;
        }
    }
    return line.size();
}

void leetcode_demo()
{
    assert(unable_to_eat({1, 1, 0, 0}, {0, 1, 0, 1}) == 0U);
    assert(unable_to_eat({1, 1, 1, 0, 0, 1}, {1, 0, 0, 0, 1, 1}) == 3U);
    assert(unable_to_eat({}, {}) == 0U);

    bool mismatched_sizes_rejected = false;
    try {
        static_cast<void>(unable_to_eat({1}, {}));
    } catch (const std::invalid_argument&) {
        mismatched_sizes_rejected = true;
    }
    assert(mismatched_sizes_rejected);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】依提交順序處理匯入工作
// 情境：parse、index、publish 等匯入工作必須依 FIFO 順序執行，並拒絕空 payload。
// 為何使用本章主題：queue 將只從尾端提交、前端取件的契約鎖進介面，避免任意重排或中間刪除。
// 設計：函式按值取得工作 ownership；驗證 front payload；先 move 出 Job 再 pop；記錄完成 id。
// 成本：時間 O(J)、額外空間 O(J)，J 為工作數；按值傳 lvalue 另有整隊複製成本，move 則可轉移。
// 上線注意：驗證失敗時輸入副本可能已部分消耗；真正 worker 需定義重試、dead-letter、同步與 graceful shutdown。
// -----------------------------------------------------------------------------
struct Job {
    int id;
    std::string payload;
};

std::vector<int> process_all(std::queue<Job> jobs)
{
    std::vector<int> order;
    order.reserve(jobs.size());
    while (!jobs.empty()) {
        if (jobs.front().payload.empty()) {
            throw std::invalid_argument("job payload must not be empty");
        }
        Job current = std::move(jobs.front());
        jobs.pop();
        order.push_back(current.id);
    }
    return order;
}

void practical_demo()
{
    std::queue<Job> jobs;
    jobs.push({7, "parse"});
    jobs.push({8, "index"});
    jobs.push({9, "publish"});
    const std::vector<int> processed_ids = process_all(std::move(jobs));
    assert((processed_ids == std::vector<int>{7, 8, 9}));

    std::queue<Job> invalid_jobs;
    invalid_jobs.push({10, ""});
    bool empty_payload_rejected = false;
    try {
        static_cast<void>(process_all(std::move(invalid_jobs)));
    } catch (const std::invalid_argument&) {
        empty_payload_rejected = true;
    }
    assert(empty_payload_rejected);
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "queue：FIFO、輪轉模擬與工作處理測試通過\n";
}

// 【LeetCode 契約】兩陣列等長且值為 0/1；不等長明確丟 invalid_argument，空輸入回零。
// 【LeetCode 成本】輪轉模擬最壞 O(n^2)、queue 額外空間 O(n)；計數法可降為 O(n)。
// 【實務契約】process_all 取得 queue 副本或移入的 ownership，依 FIFO 回傳每個非空工作 ID。
// 【實務例外安全】傳 lvalue 時 caller 不變；傳 move 後 caller 為有效但未指定，無效 payload 被拒絕。
// 【取值陷阱】先 pop 再讀會遺失元素；先 move 若拋例外，front 可能仍在但已部分 moved-from。
// 【並行陷阱】queue 非 thread-safe；多 producer/consumer 需 mutex、condition_variable 與關閉協定。
// 【面試追問】BFS 為何用 queue？FIFO 可按距離層級維持先發現先處理。
// 【面試追問】為何 pop 不回值？把可能拋例外的取值與移除分開，避免元素已刪卻拿不到結果。
// 【面試追問】能否用 vector 當底層？不行，vector 沒有 queue 所需的 pop_front。
// 【練習】用 queue 實作二元樹 level-order traversal。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread 'queue.cpp' -o '/tmp/codex_cpp_C_Container_queue' && '/tmp/codex_cpp_C_Container_queue'
//
// === 預期輸出（節錄）===
// queue：FIFO、輪轉模擬與工作處理測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
