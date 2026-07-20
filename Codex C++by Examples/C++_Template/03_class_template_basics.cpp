/*
 * 第 03 章：類別模板基礎
 *
 * 類別模板用一套資料結構支援多種元素型別。與函式模板不同，建立物件通常要寫
 * Stack<int>；C++17 的 CTAD 可在有合適 deduction guide 時省略部分型別。
 * 本章的 Stack 使用 vector，因此 push 平均 O(1)、top O(1)、pop O(1)。
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

template <typename T>
class Stack {
public:
    void push(const T& value) { data_.push_back(value); }
    void push(T&& value) { data_.push_back(std::move(value)); }

    void pop() {
        if (data_.empty()) {
            throw std::underflow_error("Stack::pop on empty stack");
        }
        data_.pop_back();
    }

    T& top() {
        if (data_.empty()) {
            throw std::underflow_error("Stack::top on empty stack");
        }
        return data_.back();
    }

    const T& top() const {
        if (data_.empty()) {
            throw std::underflow_error("Stack::top on empty stack");
        }
        return data_.back();
    }

    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }

private:
    std::vector<T> data_;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 155. Min Stack（最小棧）
// 題目：設計支援 push、pop、top、getMin 的 stack，且每項皆 O(1)；push -2,0,-3 後最小值為 -3。
// 為何使用本章主題：MinStack<T> 以類別模板重用「值與當下最小值成對保存」的結構，
// 原題只需 int；泛化後 T 必須可複製、移動並支援 std::min 所需比較。
// 思路：push 時比較新值與前一層 minimum；把 value/minimum 成對入棧；pop 會一併還原前一層最小值。
// 複雜度：每項操作時間 O(1)，額外空間 O(N)，N 是 stack 元素數，每層多存一份 T。
// 易錯點：pop/top/get_min 只在非空時合法；minimum 複製可能昂貴，浮點 NaN 也會破壞直覺排序。
// -----------------------------------------------------------------------------
template <typename T>
class MinStack {
public:
    void push(T value) {
        const T minimum = data_.empty() ? value : std::min(value, data_.back().second);
        data_.emplace_back(std::move(value), minimum);
    }

    void pop() { data_.pop_back(); }
    const T& top() const { return data_.back().first; }
    const T& get_min() const { return data_.back().second; }

private:
    std::vector<std::pair<T, T>> data_;
};

// -----------------------------------------------------------------------------
// 【日常實務範例】部署命令復原堆疊
// 情境：部署工具依執行順序保存 service/version 命令，復原時先取出最近一次操作。
// 為何使用本章主題：Stack<DeployCommand> 直接重用通用容器模板，不需讓 Stack 認識部署欄位；
// 相較 void* 或 any，元素型別與 top 回傳值都在編譯期確定。
// 設計：依序 push 兩筆命令；top 取得最新命令；pop 後前一筆重新成為棧頂。
// 成本：push 攤銷 O(1)、top/pop O(1)，空間 O(N)，N 是尚可復原的命令數。
// 上線注意：空棧操作會丟例外；top 參考在 pop、重新配置或 Stack 銷毀後失效，命令也需持久化。
// -----------------------------------------------------------------------------
struct DeployCommand {
    std::string service;
    int version{};
};

void leetcode_min_stack_test() {
    MinStack<int> mins;
    mins.push(-2);
    mins.push(0);
    mins.push(-3);
    assert(mins.get_min() == -3);
    mins.pop();
    assert(mins.top() == 0);
    assert(mins.get_min() == -2);
}

void practical_undo_stack_test() {
    Stack<DeployCommand> undo;
    undo.push(DeployCommand{"billing", 42});
    undo.push(DeployCommand{"search", 18});
    assert(undo.top().service == "search");
    undo.pop();
    assert(undo.top().version == 42);
}

int main() {
    // 基礎。
    Stack<std::string> pages;
    pages.push("home");
    pages.push("settings");
    assert(pages.top() == "settings");
    pages.pop();
    assert(pages.size() == 1U);

    // LeetCode 式測試。
    leetcode_min_stack_test();

    // 實務：部署命令的 undo stack；模板不需要知道命令欄位。
    practical_undo_stack_test();

    std::cout << "類別模板測試完成\n";
}

/*
 * 【生命週期】top() 回傳容器內元素 reference；pop、重新配置或 Stack 銷毀後不可再用。
 * 【例外安全】vector::push_back 若配置失敗，原 vector 通常維持不變（strong guarantee）。
 * 【陷阱】MinStack::pop 沒做空檢查，是配合題目「呼叫必合法」的契約；正式 API 應防護。
 * 【面試】為何成員函式定義通常要放 header？編譯器在實體化點需要看見完整定義。
 * 【練習】加入 emplace(Args&&...) 並用 perfect forwarding 直接建構 T。
 */

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_class_template_basics.cpp' -o '/tmp/codex_cpp_C_Template_03_class_template_basics' && '/tmp/codex_cpp_C_Template_03_class_template_basics'
//
// === 預期輸出（節錄）===
// 類別模板測試完成
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
