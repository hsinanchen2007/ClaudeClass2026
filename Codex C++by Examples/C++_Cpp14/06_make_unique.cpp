/*
 * C++14 教科書：std::make_unique
 *
 * make_unique<T>(args...) 配置 T 並立即包進 unique_ptr，表達單一 ownership。相較
 * `unique_ptr<T>(new T(...))` 更短，也避免在複雜 function-call argument evaluation 中
 * raw pointer 尚未被接管就遇到 exception 的風險。C++14 同時支援動態陣列 T[]。
 *
 * 【ownership】unique_ptr 不可複製、可 move；離開 scope 自動 delete。observer 用 T* 或 T&。
 * 【polymorphism】unique_ptr<Derived> 可 move 成 unique_ptr<Base>，Base destructor 要 virtual。
 * 【選擇】需要共享 lifetime 才考慮 shared_ptr；不要為了方便把所有東西都 shared。
 * 【陷阱】`ptr.release()` 放棄 ownership 且不 delete；多半應用 reset() 或自然離開 scope。
 * 【面試題】make_unique 與 make_shared 的 allocation/control-block 差異。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace basic {
struct Record {
    explicit Record(std::string initial_text) : text(std::move(initial_text)) {}
    std::string text;
};

void demo() {
    std::unique_ptr<Record> record = std::make_unique<Record>("owned");
    assert(record->text == "owned");
    std::unique_ptr<Record> moved = std::move(record);
    assert(!record && moved->text == "owned");
}
}  // namespace basic

namespace leetcode {
// LeetCode 104：Maximum Depth of Binary Tree。unique_ptr 建立測試樹並自動遞迴釋放。
struct TreeNode {
    explicit TreeNode(int initial_value) : value(initial_value) {}
    int value;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};

int leetcode_max_depth(const TreeNode* node) {
    if (node == nullptr) return 0;
    return 1 + std::max(leetcode_max_depth(node->left.get()),
                        leetcode_max_depth(node->right.get()));
}

void leetcode_test() {
    std::unique_ptr<TreeNode> root = std::make_unique<TreeNode>(3);
    root->left = std::make_unique<TreeNode>(9);
    root->right = std::make_unique<TreeNode>(20);
    root->right->left = std::make_unique<TreeNode>(15);
    root->right->right = std::make_unique<TreeNode>(7);
    assert(leetcode_max_depth(root.get()) == 3);
}
}  // namespace leetcode

// 實務案例：下列 practical_* 函式與測試展示工作場景。
namespace practical {
class Stage {
public:
    virtual ~Stage() {}
    virtual int run(int input) const = 0;
};

class ScaleStage : public Stage {
public:
    explicit ScaleStage(int factor) : factor_(factor) {}
    int run(int input) const override { return input * factor_; }
private:
    int factor_;
};

int practical_pipeline(const std::vector<std::unique_ptr<Stage> >& stages, int value) {
    for (std::vector<std::unique_ptr<Stage> >::const_reference stage : stages) {
        value = stage->run(value);
    }
    return value;
}

void practical_test() {
    std::vector<std::unique_ptr<Stage> > stages;
    stages.push_back(std::make_unique<ScaleStage>(2));
    stages.push_back(std::make_unique<ScaleStage>(3));
    assert(practical_pipeline(stages, 5) == 30);
}
}  // namespace practical

int main() {
    basic::demo();
    leetcode::leetcode_test();
    practical::practical_test();
    std::cout << "make_unique：ownership、tree depth、pipeline 測試通過\n";
}
