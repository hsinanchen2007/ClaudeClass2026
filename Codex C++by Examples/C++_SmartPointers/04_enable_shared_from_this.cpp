// ============================================================================
// 課題 4：std::enable_shared_from_this
// ============================================================================
//
// class 繼承 enable_shared_from_this<T> 後，可在 member 取得「共用同一 control block」的
// shared_ptr (`shared_from_this`)。不要寫 `shared_ptr<T>(this)`，那會建立第二 control
// block 並 double delete。
//
// 前置條件：object 已由 shared_ptr 擁有。constructor 中或 stack/new raw object 上呼叫
// shared_from_this 會丟 bad_weak_ptr。常用 private constructor + static create/make_shared
// pattern（若 constructor private，make_shared access 需 enabler 技巧）。C++17 的
// weak_from_this 可取得不丟例外的 weak observer。
// ============================================================================

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Session : public std::enable_shared_from_this<Session> {
public:
    static std::shared_ptr<Session> create(std::string id)
    {
        struct Enabler final : Session {
            explicit Enabler(std::string value) : Session(std::move(value)) {}
        };
        return std::make_shared<Enabler>(std::move(id));
    }

    std::function<std::string()> callback()
    {
        const std::weak_ptr<Session> weak = weak_from_this();
        return [weak] {
            const auto session = weak.lock();
            return session == nullptr ? std::string("expired") : "session:" + session->id_;
        };
    }

private:
    explicit Session(std::string id) : id_(std::move(id)) {}
    std::string id_;
};

void basic_example()
{
    auto session = Session::create("42");
    auto callback = session->callback();
    assert(callback() == "session:42");
    session.reset();
    assert(callback() == "expired"); // callback weakly observes，不延長 session。
    std::cout << "[基礎] weak callback safely handles destroyed Session\n";
}

// LeetCode 1603：ParkingSystem。addCar 本身完整實作題目契約；再額外加入 deferred callback，
// 示範 event 只在 object 仍活時呼叫，物件死後不 dereference dangling this。
class ParkingSystem : public std::enable_shared_from_this<ParkingSystem> {
public:
    ParkingSystem(int big, int medium, int small) : spaces_{big, medium, small} {}
    bool addCar(int type)
    {
        int& space = spaces_.at(static_cast<std::size_t>(type - 1));
        if (space == 0) return false;
        --space;
        return true;
    }
    std::function<bool()> deferred_add(int type)
    {
        const std::weak_ptr<ParkingSystem> weak = weak_from_this();
        return [weak, type] {
            const auto parking = weak.lock();
            return parking != nullptr && parking->addCar(type);
        };
    }
private:
    std::array<int, 3> spaces_;
};

void leetcode_1603_example()
{
    auto parking = std::make_shared<ParkingSystem>(1, 0, 0);
    assert(!parking->addCar(2));
    auto add_big = parking->deferred_add(1);
    assert(add_big());
    assert(!add_big());
    parking.reset();
    assert(!add_big());
    std::cout << "[LeetCode 1603] addCar 契約完整，deferred callback 另防 dangling this\n";
}

// 實務：async work 常 capture shared_from_this 延長 lifetime，或 weak_from_this 允許取消。
void practical_example()
{
    auto session = Session::create("build");
    auto callback = session->callback();
    assert(callback() == "session:build");
    std::cout << "[實務] async-style callback uses existing control block\n";
}

int main()
{
    basic_example();
    leetcode_1603_example();
    practical_example();
}

// 易錯與面試：只能在 object 已由 shared_ptr control block 管理後呼叫 shared_from_this；
// constructor 內呼叫通常丟 bad_weak_ptr。也不能用 `shared_ptr(this)` 建第二個 control block。

// 練習：比較 callback capture shared_from_this 與 weak_from_this 對 lifetime/cancellation。
// 複雜度：shared_from_this/weak_from_this 是 O(1) control-block 操作，不會複製整個 object。
// 生命週期：constructor 期間尚未接上 shared owner；此時呼叫 shared_from_this 會丟 bad_weak_ptr。
