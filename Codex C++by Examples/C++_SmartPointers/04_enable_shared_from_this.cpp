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

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】LeetCode 1603. Design Parking System（設計停車系統）
// 題目：大中小車依 type 有剩餘空位才停入；例如容量 (1,0,0) 時第一次大車成功、第二次失敗。
// 為何使用本章主題：addCar 完整解題；deferred_add 是額外教學擴充，以 weak_from_this 防 callback 使用 dangling this。
// 思路：addCar 取 type 對應 slot並遞減；deferred callback 保存 weak owner；觸發時 lock，存活才呼叫 addCar。
// 複雜度：addCar 與 callback 觸發皆 O(1)，物件空間 O(1)，另有 control block/callback 常數成本。
// 易錯點：題目保證 type 1..3 與非負容量，本類未自行驗證；物件必須先由 shared_ptr 管理才有有效 weak_from_this。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// 【日常實務範例】可在 session 消失後取消的非同步 callback
// 情境：排程工作可能晚於 Session 解構才執行；存活時回 session:build，已銷毀時應安全回 expired。
// 為何使用本章主題：Session::callback 由 weak_from_this 取得既有 control block observer，不延長 lifetime 也不建立第二 owner block。
// 設計：factory 先建立 shared-owned Session；callback capture weak；執行時 lock；依成功與否讀 id 或回 expired。
// 成本：建立 callback 與每次 lock 為常數 control-block 成本，回傳字串 O(I)。
// 上線注意：constructor 內不能 shared_from_this；callback 若需保證完成可 capture strong owner，但要檢查 cycle 與取消政策。
// -----------------------------------------------------------------------------
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

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '04_enable_shared_from_this.cpp' -o '/tmp/codex_cpp_C_SmartPointers_04_enable_shared_from_this' && '/tmp/codex_cpp_C_SmartPointers_04_enable_shared_from_this'
//
// === 預期輸出（節錄）===
// [實務] async-style callback uses existing control block
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
