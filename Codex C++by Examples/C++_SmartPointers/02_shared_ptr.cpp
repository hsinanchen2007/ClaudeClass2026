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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 133. Clone Graph（複製圖）
// 題目：深複製含 cycle 的連通圖；例如節點 1、2 互連時，新圖關係相同但所有節點地址不同。
// 為何使用本章主題：本教學版以 shared_ptr edges 簡化遞迴傳遞，會形成 ownership cycle，測試後必須拆除；實務宜 arena/weak edges。
// 思路：null 回空；map 命中回既有 clone；先建立並登記；再遞迴複製每條 neighbor edge。
// 複雜度：時間 O(V+E)、額外空間 O(V)，V/E 為節點與邊數，遞迴 stack 最壞 O(V)。
// 易錯點：走鄰點前必須登記以終止 cycle；來源與 clone 不可共用節點；strong cycle 若不拆會永久洩漏。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】推論請求共享唯讀模型權重
// 情境：多個 Request 同時使用 llama 權重，模型載入昂貴且任一請求結束都不應提早釋放。
// 為何使用本章主題：shared_ptr<const Model> 讓請求共同決定 lifetime，const pointee 也限制意外 mutation。
// 設計：make_shared 建立單一模型；每個 Request copy owner；execute 透過共享物件讀 name。
// 成本：owner copy/reset 為常數 refcount 成本，模型只存一份；execute 字串組合 O(M)。
// 上線注意：refcount 安全不代表模型可變 cache thread-safe；避免 cycle，模型熱更新要以版本化 owner 原子發布。
// -----------------------------------------------------------------------------
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
