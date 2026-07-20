// ============================================================================
// 課題 2：std::shared_ptr - 明確的共享 ownership
// ============================================================================
//
// shared_ptr copies 共用 control block；最後一個 strong owner 消失才解構 object。
// make_shared 通常一次 allocation 放 object+control block，效能/exception safety 較好。
// `use_count()` 主要供 diagnostics，不可用它做 synchronization 決策，值隨時可能變。
//
// 只有真實需求是「多方共同決定生命週期」才用 shared_ptr；不確定 owner 時盲用會隱藏
// architecture。shared_ptr 本身的不同 copies 可由多 threads 操作，但 pointee 仍需同步。
// 由同一 raw pointer 分別建兩個 shared_ptr 會形成兩個 control blocks，最後 double delete。
// ============================================================================

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Model {
public:
    explicit Model(std::string name) : name_(std::move(name)) {}
    const std::string& name() const { return name_; }
private:
    std::string name_;
};

void basic_example()
{
    auto model = std::make_shared<const Model>("resnet50");
    std::shared_ptr<const Model> worker_a = model;
    std::shared_ptr<const Model> worker_b = model;
    assert(model.use_count() == 3);
    assert(worker_a->name() == "resnet50" && worker_b->name() == "resnet50");
    std::cout << "[基礎] three owners share immutable model\n";
}

// LeetCode 133：Clone Graph。clones map 必須在遞迴 neighbors 前先登記新節點，否則遇到
// cycle 會無限遞迴。為聚焦演算法，本例 edges 使用 shared_ptr；測試結尾會明確拆 cycle。
// Production graph 通常由 arena/container 擁有 nodes，edges 用 raw/weak observer 避免環狀 ownership。
struct GraphNode {
    explicit GraphNode(int node_value) : value(node_value) {}
    int value;
    std::vector<std::shared_ptr<GraphNode>> neighbors;
};

std::shared_ptr<GraphNode> clone_graph(
    const std::shared_ptr<GraphNode>& node,
    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>>& clones)
{
    if (node == nullptr) return nullptr;
    const auto found = clones.find(node.get());
    if (found != clones.end()) return found->second;
    auto clone = std::make_shared<GraphNode>(node->value);
    clones.emplace(node.get(), clone);
    for (const auto& neighbor : node->neighbors) {
        clone->neighbors.push_back(clone_graph(neighbor, clones));
    }
    return clone;
}

void leetcode_133_example()
{
    auto one = std::make_shared<GraphNode>(1);
    auto two = std::make_shared<GraphNode>(2);
    one->neighbors.push_back(two);
    two->neighbors.push_back(one); // 真正測試 LeetCode graph 的 cycle。
    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>> clones;
    const auto copy = clone_graph(one, clones);
    const auto copy_two = copy->neighbors.front();
    assert(copy.get() != one.get());
    assert(copy->value == 1 && copy_two->value == 2);
    assert(copy_two.get() != two.get());
    assert(copy_two->neighbors.front().get() == copy.get());

    // shared_ptr edges 只為教材示範，必須拆環；下一課用 weak_ptr 建模 observer edge。
    one->neighbors.clear();
    two->neighbors.clear();
    copy->neighbors.clear();
    copy_two->neighbors.clear();
    std::cout << "[LeetCode 133] 兩節點 cycle 完整複製且 clone 與來源獨立\n";
}

// 實務：多 requests 共用 read-only weights；const pointee 限制意外 mutation。
class Request {
public:
    explicit Request(std::shared_ptr<const Model> model) : model_(std::move(model)) {}
    std::string execute() const { return "infer:" + model_->name(); }
private:
    std::shared_ptr<const Model> model_;
};

void practical_example()
{
    auto weights = std::make_shared<const Model>("llama");
    Request first(weights);
    Request second(weights);
    assert(first.execute() == "infer:llama" && second.execute() == "infer:llama");
    std::cout << "[實務] two requests share const llama weights\n";
}

int main()
{
    basic_example();
    leetcode_133_example();
    practical_example();
}

// 練習：讓 graph 加 2->1 cycle，觀察 strong cycle；下一課改 weak_ptr。
// 複雜度：copy/reset 通常是 O(1) control-block 計數操作；最後釋放另含 pointee destructor。
// 生命週期：strong count 歸零才解構 pointee，weak count 歸零後 control block 才可回收。

/*
【本課面試問答】
Q1：`shared_ptr` 本身 thread-safe 嗎？
A：不同 `shared_ptr` 物件若共享同一 control block，可在不同 threads 複製/銷毀，引用計數同步是安全的；
但同一個 `shared_ptr` object 的非 const 操作，以及 pointee 的資料，都不因此自動安全。前者可用
`atomic<shared_ptr<T>>`，後者仍需物件自己的同步策略。

Q2：`make_shared` 為何通常較快？何時反而不用？
A：典型實作把 control block 與 `T` 合併成一次配置，改善配置成本與 locality。若 weak owners 長期
存在，合併配置會讓包含 `T` 儲存空間的整塊記憶體延後回收；需要自訂 deleter/allocator 或特殊
生命週期時，分開配置可能更合適。

Q3：為何不能對同一個 raw pointer 分別建兩個 `shared_ptr`？
A：兩者會有獨立 control blocks，最後各自 delete 同一物件，形成 double delete/UB。要增加 owner
必須複製既有 `shared_ptr`；需要指向子物件但共享 ownership 時用 aliasing constructor。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '02_shared_ptr.cpp' -o '/tmp/codex_cpp_C_SmartPointers_02_shared_ptr' && '/tmp/codex_cpp_C_SmartPointers_02_shared_ptr'
//
// === 預期輸出（節錄）===
// [實務] two requests share const llama weights
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
