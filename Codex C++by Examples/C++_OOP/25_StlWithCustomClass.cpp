// ============================================================================
// 課題 25：把 custom class 放進 STL containers/algorithms
// ============================================================================
//
// vector 通常只要求 element 可 move/copy；sort 需要 strict weak ordering；set/map 用
// ordering 判「等價」，unordered containers 則需要 hash 與 equality 一致：若 a==b，
// hash(a) 必須等於 hash(b)。C++20 ranges/projection 可減少為每種排序目的重載 operator<。
//
// container key 在容器中不可被修改到影響 ordering/hash，否則內部位置與新值不一致。
// map/set 的 key 是 const；unordered key 也不可原地改，應 erase 後 reinsert。
//
// 【面試】operator< 不必與「所有欄位相等」同義，但同一 container 的 comparator 必須
// 穩定。floating NaN 不符合一般 total ordering，拿來當 key 要特別設計。
// ============================================================================

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Employee {
public:
    Employee(int id, std::string name) : id_(id), name_(std::move(name)) {}
    int id() const { return id_; }
    const std::string& name() const { return name_; }
    bool operator<(const Employee& other) const { return id_ < other.id_; }

private:
    int id_;
    std::string name_;
};

void basic_example()
{
    std::vector<Employee> employees{{3, "Carol"}, {1, "Ada"}, {2, "Bjarne"}};
    std::sort(employees.begin(), employees.end());
    assert(employees.front().id() == 1 && employees.back().id() == 3);
    const auto found = std::find_if(employees.begin(), employees.end(),
        [](const Employee& employee) { return employee.name() == "Bjarne"; });
    assert(found != employees.end() && found->id() == 2);
    std::cout << "[基礎] custom Employee 可 sort 並依 name find_if\n";
}

// LeetCode 1：Two Sum。
// ValueIndex 把 value 與原始 index 綁成一個 value object；operator< 讓 sort 依 value，
// two-pointer 找到總和後仍能回原 index。
class ValueIndex {
public:
    ValueIndex(int value, int index) : value_(value), index_(index) {}
    int value() const { return value_; }
    int index() const { return index_; }
    bool operator<(const ValueIndex& other) const { return value_ < other.value_; }

private:
    int value_;
    int index_;
};

std::pair<int, int> two_sum(const std::vector<int>& nums, int target)
{
    if (nums.size() < 2U) return {-1, -1};
    std::vector<ValueIndex> indexed;
    indexed.reserve(nums.size());
    for (std::size_t index = 0U; index < nums.size(); ++index) {
        indexed.emplace_back(nums.at(index), static_cast<int>(index));
    }
    std::sort(indexed.begin(), indexed.end());
    std::size_t left = 0U;
    std::size_t right = indexed.size() - 1U;
    while (left < right) {
        const long long sum = static_cast<long long>(indexed.at(left).value()) +
                              indexed.at(right).value();
        if (sum == target) return {indexed.at(left).index(), indexed.at(right).index()};
        if (sum < target) ++left;
        else --right;
    }
    return {-1, -1};
}

void leetcode_1_example()
{
    const auto answer = two_sum({2, 7, 11, 15}, 9);
    assert(answer.first == 0 && answer.second == 1);
    std::cout << "[LeetCode 1] custom ValueIndex 找到 indices 0,1\n";
}

// 實務案例：strong AssetId 避免把任意 string 誤傳；自訂 hash 支援 unordered_map。
class AssetId {
public:
    explicit AssetId(std::string value) : value_(std::move(value)) {}
    const std::string& value() const { return value_; }
    bool operator==(const AssetId& other) const { return value_ == other.value_; }
private:
    std::string value_;
};

struct AssetIdHash {
    std::size_t operator()(const AssetId& id) const noexcept
    {
        return std::hash<std::string>{}(id.value());
    }
};

void practical_example()
{
    std::unordered_map<AssetId, std::size_t, AssetIdHash> sizes;
    sizes.emplace(AssetId("model.bin"), 4'096U);
    assert(sizes.at(AssetId("model.bin")) == 4'096U);
    std::cout << "[實務] unordered_map<AssetId,size> lookup=4096\n";
}

int main()
{
    basic_example();
    leetcode_1_example();
    practical_example();
}

// 練習：不用 operator<，改傳 lambda 讓 Employee 依 name 排序。
// 複雜度：sort O(N log N)、unordered lookup 平均 O(1)；custom class 不改 container guarantee。
// 生命週期：container 按值擁有 elements；iterator/reference 仍受 reallocation、erase 規則約束。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '25_StlWithCustomClass.cpp' -o '/tmp/codex_cpp_C_OOP_25_StlWithCustomClass' && '/tmp/codex_cpp_C_OOP_25_StlWithCustomClass'
//
// === 預期輸出（節錄）===
// [實務] unordered_map<AssetId,size> lookup=4096
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
