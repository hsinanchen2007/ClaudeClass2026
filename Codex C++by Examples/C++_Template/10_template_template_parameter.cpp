/*
 * 第 10 章：template-template parameter
 *
 * 當演算法不只要接收「型別」，而要接收「產生型別的模板」時，可用
 * template <typename, typename> class Container。它常見於 policy/container 注入。
 * 參數形狀必須相容；標準容器通常還有 allocator 參數，因此宣告要把它納入。
 */

#include <cassert>
#include <deque>
#include <iostream>
#include <list>
#include <string>
#include <utility>
#include <vector>

template <typename T,
          template <typename, typename> class Container = std::vector,
          typename Allocator = std::allocator<T>>
class Bag {
public:
    void add(T value) { values_.push_back(std::move(value)); }
    [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
    const T& first() const { return values_.front(); }

private:
    Container<T, Allocator> values_;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 232. Implement Queue using Stacks（用棧實作佇列）
// 題目：只用 stack 式操作實作 FIFO 的 push、pop、peek、empty；push 1,2 後先 pop 1。
// 為何使用本章主題：Sequence 是 template-template parameter，可在 vector/deque 間替換，
// incoming/outgoing 的兩棧演算法不變；正式題目不要求底層容器可注入。
// 思路：push 追加 incoming；outgoing 空時把 incoming 由尾端逐一搬過去；pop/peek 讀 outgoing 尾端。
// 複雜度：單次 refill 最壞 O(N)，每元素最多搬一次，push/pop/peek 攤銷 O(1)，空間 O(N)。
// 易錯點：pop/peek 只在非空時合法；搬移中拋例外會留下部分轉移，peek 參考也可能失效。
// -----------------------------------------------------------------------------
template <typename T, template <typename, typename> class Sequence = std::vector>
class TwoStackQueue {
public:
    void push(T value) { incoming_.push_back(std::move(value)); }

    T pop() {
        refill();
        T value = std::move(outgoing_.back());
        outgoing_.pop_back();
        return value;
    }

    const T& peek() {
        refill();
        return outgoing_.back();
    }

    bool empty() const noexcept { return incoming_.empty() && outgoing_.empty(); }

private:
    void refill() {
        if (outgoing_.empty()) {
            while (!incoming_.empty()) {
                outgoing_.push_back(std::move(incoming_.back()));
                incoming_.pop_back();
            }
        }
    }

    Sequence<T, std::allocator<T>> incoming_;
    Sequence<T, std::allocator<T>> outgoing_;
};

void leetcode_queue_test() {
    TwoStackQueue<int, std::deque> queue;
    queue.push(1);
    queue.push(2);
    // peek 會按需 refill、pop 會移除元素；NDEBUG 會移除整個 assert expression，
    // 因此操作必須先執行並保存結果，assert 只負責驗證。
    [[maybe_unused]] const int first_peek = queue.peek();
    assert(first_peek == 1);
    [[maybe_unused]] const int popped = queue.pop();
    assert(popped == 1);
    [[maybe_unused]] const int second_peek = queue.peek();
    assert(second_peek == 2);
    assert(!queue.empty());
}

// -----------------------------------------------------------------------------
// 【日常實務範例】可替換儲存容器的工作清單
// 情境：工作載入流程只需 add/size，測試時想改用 list 觀察逐節點配置而不改公開 Bag 介面。
// 為何使用本章主題：Storage 以未實體化模板傳入，practical_load_job_count 可建立不同 Bag；
// 相較把 vector/list 寫成兩個函式，流程只保留一份，但容器參數形狀必須相容。
// 設計：依 Storage 建立 Bag<string>；加入 index 與 backup；回傳容器報告的工作數。
// 成本：兩次插入與 size 查詢；一般化為 N 筆時 O(N) 建立、O(N) 空間，配置模式依 Storage 而異。
// 上線注意：只因語法相容不代表 iterator/配置成本相同；空 Bag 的 first 也沒有防護。
// -----------------------------------------------------------------------------
template <template <typename, typename> class Storage>
std::size_t practical_load_job_count() {
    Bag<std::string, Storage> jobs;
    jobs.add("index");
    jobs.add("backup");
    return jobs.size();
}

int main() {
    Bag<int> vector_bag;
    vector_bag.add(7);
    assert(vector_bag.first() == 7);

    Bag<std::string, std::list> list_bag;
    list_bag.add("task");
    assert(list_bag.size() == 1U);

    leetcode_queue_test();

    [[maybe_unused]] const std::size_t job_count = practical_load_job_count<std::list>();
    assert(job_count == 2U);
    std::cout << "template-template parameter 測試完成\n";
}

/*
 * 【複雜度】TwoStackQueue 每個元素最多搬移一次，單次最壞 O(n)，攤銷 O(1)。
 * 【陷阱】不同模板的參數數量/種類不相容會直接編譯失敗，concept 可改善診斷。
 * 【設計】若只需 range 行為，通常直接接收 range 物件比接模板更簡單。
 * 【面試】template-template parameter 接的是什麼？未實體化的模板，而不是 vector<int> 型別。
 * 【練習】加入 Container policy，讓 Bag 支援 reserve（僅對有 reserve 的容器呼叫）。
 */

/*
 * 【教科書補充：容器 policy 的隱藏契約】
 * - Bag::first、TwoStackQueue::pop/peek 都要求非空；本例由測試先 push，正式 API 應持續驗證。
 * - Sequence 必須提供 push_back/back/pop_back/empty，且 allocator 參數形狀相容；名稱相容不代表語意相容。
 * - refill 搬移一半時若 T move/配置拋出，incoming/outgoing 可能是部分轉移狀態，只能承諾有限保證。
 * - peek 回傳的 reference 在後續 pop/refill 或底層容器操作後可能失效，caller 不可長期保存。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '10_template_template_parameter.cpp' -o '/tmp/codex_cpp_C_Template_10_template_template_parameter' && '/tmp/codex_cpp_C_Template_10_template_template_parameter'
//
// === 預期輸出（節錄）===
// template-template parameter 測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
