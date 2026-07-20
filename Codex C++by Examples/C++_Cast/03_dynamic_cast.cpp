// ============================================================================
// 課題 3：dynamic_cast - polymorphic hierarchy 的 runtime check
// ============================================================================
//
// Base 至少有一個 virtual function 才是 polymorphic。`dynamic_cast<Derived*>(base)` 成功
// 回 pointer、失敗回 nullptr；reference 版本失敗丟 std::bad_cast。upcast 不需要它，
// normal implicit conversion 即安全。
//
// 大量 dynamic_cast 往往表示介面缺少 virtual operation，或呼叫端過度依賴 concrete
// types。它適合 plugin/UI tree/heterogeneous framework 中「可選的 concrete capability」，
// 不適合每個 hot-loop element 都靠 RTTI 分支。
//
// 【安全】永遠檢查 pointer 結果；不可假設 factory 回來一定是某 derived type。
// ============================================================================

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Event {
public:
    virtual ~Event() = default;
    virtual std::string kind() const = 0;
};

class ErrorEvent final : public Event {
public:
    explicit ErrorEvent(int code) : code_(code) {}
    std::string kind() const override { return "error"; }
    int code() const { return code_; }
private:
    int code_;
};

class InfoEvent final : public Event {
public:
    std::string kind() const override { return "info"; }
};

void basic_example()
{
    std::unique_ptr<Event> event = std::make_unique<ErrorEvent>(404);
    const auto* error = dynamic_cast<const ErrorEvent*>(event.get());
    assert(error != nullptr && error->code() == 404);
    assert(dynamic_cast<const InfoEvent*>(event.get()) == nullptr);
    std::cout << "[基礎] dynamic_cast identifies ErrorEvent code=404\n";
}

// LeetCode 341：Flatten Nested List Iterator。官方 NestedInteger 已提供 isInteger/getList，
// 通常不需要 RTTI；本例以 abstract Item + dynamic_cast 建立等價資料模型，並實作題目要求的
// next()/hasNext() API。constructor eager flatten，next/hasNext 均為 O(1)。
class Item {
public:
    virtual ~Item() = default;
};

class IntegerItem final : public Item {
public:
    explicit IntegerItem(int value) : value_(value) {}
    int value() const { return value_; }
private:
    int value_;
};

class ListItem final : public Item {
public:
    void add(std::unique_ptr<Item> item) { items_.push_back(std::move(item)); }
    const std::vector<std::unique_ptr<Item>>& items() const { return items_; }
private:
    std::vector<std::unique_ptr<Item>> items_;
};

void flatten(const Item& item, std::vector<int>& output)
{
    if (const auto* integer = dynamic_cast<const IntegerItem*>(&item)) {
        output.push_back(integer->value());
        return;
    }
    if (const auto* list = dynamic_cast<const ListItem*>(&item)) {
        for (const auto& child : list->items()) flatten(*child, output);
    }
}

class NestedIterator {
public:
    explicit NestedIterator(const std::vector<std::unique_ptr<Item>>& nested_list)
    {
        for (const auto& item : nested_list) flatten(*item, values_);
    }

    int next()
    {
        if (!hasNext()) throw std::out_of_range("NestedIterator exhausted");
        return values_.at(index_++);
    }

    bool hasNext() const noexcept { return index_ < values_.size(); }

private:
    std::vector<int> values_;
    std::size_t index_ = 0U;
};

void leetcode_341_example()
{
    // 官方型態案例 [1,[4,[6]]]，預期依序輸出 1,4,6。
    std::vector<std::unique_ptr<Item>> nested_list;
    nested_list.push_back(std::make_unique<IntegerItem>(1));
    auto outer = std::make_unique<ListItem>();
    outer->add(std::make_unique<IntegerItem>(4));
    auto inner = std::make_unique<ListItem>();
    inner->add(std::make_unique<IntegerItem>(6));
    outer->add(std::move(inner));
    nested_list.push_back(std::move(outer));

    NestedIterator iterator(nested_list);
    std::vector<int> flat;
    while (iterator.hasNext()) flat.push_back(iterator.next());
    assert((flat == std::vector<int>{1, 4, 6}));
    assert(!iterator.hasNext());
    std::cout << "[LeetCode 341] next/hasNext flatten [1,[4,[6]]] -> 1,4,6\n";
}

// 實務：只有某些 plugins 支援 reload；dynamic_cast 檢查 optional capability。
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual std::string name() const = 0;
};
class ReloadablePlugin final : public Plugin {
public:
    std::string name() const override { return "config"; }
    void reload() { ++reloads_; }
    int reloads() const { return reloads_; }
private:
    int reloads_ = 0;
};

void practical_example()
{
    std::unique_ptr<Plugin> plugin = std::make_unique<ReloadablePlugin>();
    auto* reloadable = dynamic_cast<ReloadablePlugin*>(plugin.get());
    assert(reloadable != nullptr);
    reloadable->reload();
    assert(reloadable->reloads() == 1);
    std::cout << "[實務] optional reload capability invoked once\n";
}

int main()
{
    basic_example();
    leetcode_341_example();
    practical_example();
}

// 練習：新增 virtual `accept(Visitor&)`，比較 Visitor 與重複 dynamic_cast 的取捨。
// 複雜度：dynamic_cast 的搜尋成本由 ABI/hierarchy 實作決定，標準不保證 O(1)。
// 生命週期：成功回傳的 pointer/reference 只是同一 complete object 的別名，完全不擁有它。

/*
【本課面試問答】
Q1：`dynamic_cast` downcast 的前提與失敗結果？
A：來源通常必須指向 polymorphic class（至少一個 virtual member）。pointer cast 失敗回 nullptr；reference
cast 失敗丟 `std::bad_cast`。成功只產生 alias，不延長 object lifetime。

Q2：能否一律用 `static_cast<Derived*>` 取代以省 RTTI 成本？
A：只有程式已由其他不變式證明 dynamic type 正確時才可；判斷錯誤後使用結果是 UB。dynamic_cast 的
成本與 ABI/hierarchy 有關，應先 profile；不能用未證明的 downcast 換取臆測效能。

Q3：大量 `dynamic_cast` chain 暗示什麼？
A：可能表示 base interface 缺少行為、需要 Visitor、variant 或 capability interface。本課 optional
capability 的單點檢查可合理；若每個 operation 都 switch concrete type，應重審多型邊界。
*/

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '03_dynamic_cast.cpp' -o '/tmp/codex_cpp_C_Cast_03_dynamic_cast' && '/tmp/codex_cpp_C_Cast_03_dynamic_cast'
//
// === 預期輸出（節錄）===
// [實務] optional reload capability invoked once
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
