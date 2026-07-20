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

// LeetCode 232：Implement Queue using Stacks。
// Sequence 模板可在 vector 與 deque 間替換，不改 queue 演算法。
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

// 實務：測試環境改用 list 以觀察不同配置特性；公開介面不暴露底層容器。
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
