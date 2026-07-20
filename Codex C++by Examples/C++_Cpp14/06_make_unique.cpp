/*
 * C++14 教科書：std::make_unique
 *
 * make_unique<T>(args...) 配置 T 並立即包進 unique_ptr，表達單一 ownership。相較
 * `unique_ptr<T>(new T(...))` 更短。在本章採用的 C++14 規則下，它也避免
 * `consume(unique_ptr<T>(new T), may_throw())` 的引數評估交錯，使 raw pointer 尚未
 * 被 unique_ptr 接管前另一引數就拋例外。C++17 起，各函式引數的求值彼此
 * indeterminately sequenced，已消除這個特定交錯漏洞；make_unique 仍因簡潔、單一
 * ownership 與避免重複型別名稱而是首選。C++14 同時支援動態陣列 T[]。
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
// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 104. Maximum Depth of Binary Tree（二元樹最大深度）
// 題目：輸入二元樹根節點，回傳根到最深葉節點的節點數；[3,9,20,null,null,15,7] 深度為 3。
// 為何使用本章主題：TreeNode 以 unique_ptr 唯一擁有子樹，make_unique 建測試樹並自動遞迴釋放；演算法只借用 const raw pointer。
// 思路：1. nullptr 深度為 0；2. 遞迴求左右子樹深度；3. 取較大值再加目前節點 1。
// 複雜度：N 為節點數、H 為樹高；時間 O(N)、遞迴堆疊 O(H)，樹本身 ownership 空間 O(N)。
// 易錯點：observer pointer 不可 delete；極度偏斜樹可能耗盡 call stack，Base 多型擁有時還要 virtual destructor。
// -----------------------------------------------------------------------------
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

namespace practical {
// -----------------------------------------------------------------------------
// 【日常實務範例】可組合的資料處理 pipeline
// 情境：服務依序執行多個 Stage，pipeline 容器要明確擁有不同 concrete stage 並在離開 scope 時釋放。
// 為何使用本章主題：vector<unique_ptr<Stage>> 表達唯一 ownership，make_unique 建立 ScaleStage，無需裸 new/delete。
// 設計：1. 定義具 virtual destructor 的 Stage；2. 以 make_unique 加入倍率 stage；3. 依序把前一步輸出餵給下一步。
// 成本：S 為 stage 數；執行時間 O(S) 加各 stage 成本，持有空間 O(S)，每個 stage 各一次 allocation。
// 上線注意：pipeline 不可複製但可移動；需定義 stage 例外策略、輸入溢位與是否允許跨執行緒共用。
// -----------------------------------------------------------------------------
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

// 【延伸練習】加入會在 constructor 拋例外的 Stage，驗證已建立物件仍由 RAII 正確釋放。

/*
 * 【教科書補充：make_unique 的陣列與 deleter 邊界】
 * - `make_unique<T[]>(n)` 配置動態陣列並 value-initialize n 個元素；固定長度 `T[N]` overload 被刪除。
 * - 它不能在呼叫點指定 custom deleter；需特殊釋放策略時明確建構 unique_ptr<T,Deleter>。
 * - make_unique 只處理 ownership，不改變 Base 必須有 virtual destructor 的多型刪除契約。
 * - C++17 修掉的是特定引數交錯洩漏，並沒有指定不同函式引數誰先求值。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++14 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '06_make_unique.cpp' -o '/tmp/codex_cpp_C_Cpp14_06_make_unique' && '/tmp/codex_cpp_C_Cpp14_06_make_unique'
//
// === 預期輸出（節錄）===
// make_unique：ownership、tree depth、pipeline 測試通過
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
