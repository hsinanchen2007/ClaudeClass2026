// ============================================================================
// 課題 21：std::shared_ptr、std::weak_ptr 與共享 ownership
// ============================================================================
//
// shared_ptr 的 control block 保存 strong/weak reference counts；最後一個 strong owner
// 消失時 object 解構。copy shared_ptr 增加 strong count，move 不增加。make_shared 通常
// 把 object/control block 一次配置，較有效率且 exception-safe。
//
// weak_ptr 是不延長生命的 observer。`lock()` 原子地取得 shared_ptr；若 object 已死回
// empty。雙向圖若 edges 全用 shared_ptr 會形成 cycle，reference count 永遠不歸零；
// back edge/observer 應用 weak_ptr，或重新設計單一 owner。
//
// 【面試】shared_ptr control block 操作 thread-safe，不代表 pointee 本身 thread-safe。
// 【陷阱】不可用同一 raw pointer 分別建立兩個 shared_ptr，會有兩個 control blocks、
// 最後 double delete。需要從 this 取得 owner 時研究 enable_shared_from_this。
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
    std::weak_ptr<Model> observer;
    {
        auto owner = std::make_shared<Model>("resnet50");
        std::shared_ptr<Model> second_owner = owner;
        observer = owner;
        assert(owner.use_count() == 2 && observer.lock()->name() == "resnet50");
    }
    assert(observer.expired());
    std::cout << "[基礎] 最後 strong owner 消失後 weak_ptr expired\n";
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 133. Clone Graph（複製圖）
// 題目：深複製連通無向圖，包含 cycle；例如節點 1、2 互連時，clone 也互連但地址全不同。
// 為何使用本章主題：本例改以 shared owners 加 weak edges 建模 cycle；clones map 同時防重複遞迴並持有新節點。
// 思路：null 直接回；已複製就回 map 值；先建立並登記 clone；再 lock 每個鄰點並遞迴接邊。
// 複雜度：時間 O(V+E)、額外空間 O(V)，V/E 為可達節點與邊數，另有遞迴 stack O(V)。
// 易錯點：必須在走 neighbors 前登記；weak edge 需外部 owner 保活；clones map 若銷毀，只回 root 不足以擁有其餘節點。
// -----------------------------------------------------------------------------
struct GraphNode {
    explicit GraphNode(int node_value) : value(node_value) {}
    int value;
    std::vector<std::weak_ptr<GraphNode>> neighbors;
};

std::shared_ptr<GraphNode> clone_graph(
    const std::shared_ptr<GraphNode>& source,
    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>>& clones)
{
    if (source == nullptr) return nullptr;
    const auto found = clones.find(source.get());
    if (found != clones.end()) return found->second;

    auto clone = std::make_shared<GraphNode>(source->value);
    clones.emplace(source.get(), clone); // 先登記，cycle 遞迴時才會終止。
    for (const std::weak_ptr<GraphNode>& weak_neighbor : source->neighbors) {
        if (const auto neighbor = weak_neighbor.lock()) {
            clone->neighbors.push_back(clone_graph(neighbor, clones));
        }
    }
    return clone;
}

void leetcode_133_example()
{
    auto first = std::make_shared<GraphNode>(1);
    auto second = std::make_shared<GraphNode>(2);
    first->neighbors.push_back(second);
    second->neighbors.push_back(first);

    std::unordered_map<const GraphNode*, std::shared_ptr<GraphNode>> owners;
    const auto clone = clone_graph(first, owners);
    const auto clone_neighbor = clone->neighbors.front().lock();
    assert(clone.get() != first.get());
    assert(clone->value == 1 && clone_neighbor->value == 2);
    assert(clone_neighbor->neighbors.front().lock().get() == clone.get());
    std::cout << "[LeetCode 133] cyclic graph deep clone 完成且 edges 不擁有\n";
}

// -----------------------------------------------------------------------------
// 【日常實務範例】多個推論請求共享唯讀模型權重
// 情境：兩個 InferenceRequest 同時使用 llama 模型，權重昂貴且應只載入一份，請求結束順序不固定。
// 為何使用本章主題：shared_ptr<const Model> 表達真正共同 lifetime，const pointee 阻止請求意外修改模型。
// 設計：make_shared 建模型與 control block；每個 request copy 一個 owner；run 透過共享 owner 讀 name。
// 成本：每次 owner copy/reset 為常數 reference-count 成本，模型空間一份；run 字串配置 O(M)。
// 上線注意：control block thread-safe 不等於模型內部可變 cache 安全；避免 ownership cycle，熱換模型需版本與同步策略。
// -----------------------------------------------------------------------------
class InferenceRequest {
public:
    explicit InferenceRequest(std::shared_ptr<const Model> model) : model_(std::move(model)) {}
    std::string run() const { return "using:" + model_->name(); }
private:
    std::shared_ptr<const Model> model_;
};

void practical_example()
{
    auto model = std::make_shared<const Model>("llama");
    InferenceRequest first(model);
    InferenceRequest second(model);
    assert(model.use_count() == 3);
    assert(first.run() == "using:llama" && second.run() == "using:llama");
    std::cout << "[實務] 兩 requests 共享 immutable llama model\n";
}

int main()
{
    basic_example();
    leetcode_133_example();
    practical_example();
}

// 練習：把 GraphNode edges 改 shared_ptr，說明為何清空外部 owners 後 nodes 仍不解構。
// 複雜度：shared_ptr copy/reset 通常 O(1) control-block 操作；graph clone 為 O(V+E)。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '21_SharedPtr.cpp' -o '/tmp/codex_cpp_C_OOP_21_SharedPtr' && '/tmp/codex_cpp_C_OOP_21_SharedPtr'
//
// === 預期輸出（節錄）===
// [實務] 兩 requests 共享 immutable llama model
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
