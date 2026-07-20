// ============================================================================
// 課題 17：Abstract class、pure virtual function 與介面
// ============================================================================
//
// 至少一個 pure virtual (`= 0`) 的 class 是 abstract class，不能直接建立 object；
// 它定義契約，derived concrete class 必須實作。介面通常只有 public virtual operations
// 與 virtual destructor，不暴露 writable data。
//
// abstract class 可有 constructor、data、已實作 member，pure virtual 甚至可有定義；
// 但越接近穩定 interface，derived implementation 越不易受 base representation 變動。
//
// 【設計】呼叫端依賴 abstraction，測試可注入 fake；這是 dependency inversion。
// 【陷阱】介面加入新的 pure virtual 會讓所有 implementations 同時編譯失敗，是 API
// breaking change；可新增有合理預設實作的 virtual，或建立新版本介面。
// ============================================================================

#include <cassert>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Rectangle final : public Shape {
public:
    Rectangle(double width, double height) : width_(width), height_(height) {}
    double area() const override { return width_ * height_; }

private:
    double width_;
    double height_;
};

void basic_example()
{
    const Rectangle rectangle(4.0, 3.0);
    const Shape& shape = rectangle;
    assert(shape.area() == 12.0);
    std::cout << "[基礎] abstract Shape contract area=12\n";
}

// LeetCode 278：First Bad Version。
// 線上 judge 提供 isBadVersion；用 abstract oracle 表達外部依賴，測試注入 fake oracle。
class VersionOracle {
public:
    virtual ~VersionOracle() = default;
    virtual bool is_bad(int version) const = 0;
};

class FixedBadVersionOracle final : public VersionOracle {
public:
    explicit FixedBadVersionOracle(int first_bad) : first_bad_(first_bad) {}
    bool is_bad(int version) const override { return version >= first_bad_; }

private:
    int first_bad_;
};

int first_bad_version(int versions, const VersionOracle& oracle)
{
    int left = 1;
    int right = versions;
    while (left < right) {
        const int middle = left + (right - left) / 2; // 避免 left+right overflow。
        if (oracle.is_bad(middle)) right = middle;
        else left = middle + 1;
    }
    return left;
}

void leetcode_278_example()
{
    const FixedBadVersionOracle oracle(4);
    assert(first_bad_version(5, oracle) == 4);
    std::cout << "[LeetCode 278] injected oracle 找到 first bad=4\n";
}

// 實務案例：Storage 是穩定邊界；MemoryStorage 可供本機與 unit test 使用。
class Storage {
public:
    virtual ~Storage() = default;
    virtual void put(std::string key, std::string value) = 0;
    virtual std::string get(const std::string& key) const = 0;
};

class MemoryStorage final : public Storage {
public:
    void put(std::string key, std::string value) override
    {
        values_[std::move(key)] = std::move(value);
    }
    std::string get(const std::string& key) const override
    {
        const auto found = values_.find(key);
        if (found == values_.end()) throw std::out_of_range("missing key");
        return found->second;
    }

private:
    std::map<std::string, std::string> values_;
};

void practical_example()
{
    MemoryStorage memory;
    Storage& storage = memory;
    storage.put("model", "resnet50");
    assert(storage.get("model") == "resnet50");
    std::cout << "[實務] Storage abstraction 讀回 resnet50\n";
}

int main()
{
    basic_example();
    leetcode_278_example();
    practical_example();
}

// 練習：新增 FileStorage stub；呼叫 first_bad_version 時改用另一 fake oracle 測邊界 1。
// 複雜度：interface/abstract class 不改演算法；first_bad_version 仍 O(log N) calls。
// 生命週期：以 unique_ptr<Base> 擁有 implementation 時需 virtual destructor，borrowed interface 則由 caller 保活。

/*
【本課面試問答】
Q1：含 pure virtual function 的類別一定不能有實作嗎？
A：不是。pure virtual function 仍可在類別外提供定義，derived 可明確呼叫；pure virtual destructor
甚至必須有定義，因為銷毀 derived 時仍會執行 base destructor。抽象只代表不能直接建立該類別。

Q2：interface 的 destructor 為何通常是 `virtual`？
A：若允許 caller 透過 `Base*`/`unique_ptr<Base>` 刪除 concrete object，非 virtual destructor 會造成 UB。
若明確禁止 polymorphic deletion，也可把 destructor 設 protected non-virtual，但 API 必須一致表達。

Q3：abstract class 與 type erasure/templates 怎麼選？
A：virtual interface 適合 runtime 可替換、穩定 ABI 邊界；templates 提供 compile-time polymorphism，
可 inline 但會擴大編譯依賴；type erasure（如 function）可保留 value-like API。先依替換時機與 ownership 選擇。
*/
