// ============================================================================
// 課題 3：Encapsulation（封裝）與 class invariant
// ============================================================================
//
// 封裝不是「把欄位改 private 就完成」，而是讓所有 state transition 經過可驗證的
// public operations，維持 class invariant（物件任何可觀察時刻都必須成立的規則）。
// private 是編譯期 access control，不是加密；它阻止一般呼叫端直接破壞內部表示。
//
// getter 若回 non-const reference，呼叫端仍可繞過驗證；對小值可回 value，大型唯讀
// 資料可回 const reference，但要說明 reference 生命週期/後續修改失效規則。
// setter 不應只是 `x_=x`；若沒有規則，public struct 可能更誠實。封裝讓日後更換
// 內部表示而不改呼叫端，是 API boundary 的價值。
//
// 【面試】protected 不是「更安全 private」；它讓所有 derived class 依賴內部表示，
// 常增加耦合。優先 private + protected/public behavior。
// ============================================================================

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

class Account {
public:
    Account(std::string owner, int initial_balance)
        : owner_(std::move(owner)), balance_(initial_balance)
    {
        if (owner_.empty() || balance_ < 0) {
            throw std::invalid_argument("invalid account");
        }
    }

    bool withdraw(int amount)
    {
        if (amount <= 0 || amount > balance_) {
            return false;
        }
        balance_ -= amount;
        return true;
    }

    int balance() const { return balance_; }

private:
    std::string owner_;
    int balance_;  // invariant：永遠 >= 0。
};

void basic_example()
{
    Account account("Ada", 100);
    const bool withdrew_40 = account.withdraw(40);
    const bool overdraft_rejected = !account.withdraw(100);
    assert(withdrew_40);
    assert(overdraft_rejected);
    assert(account.balance() == 60);
    std::cout << "[基礎] 封裝阻止餘額成為負數\n";
}

// LeetCode 1603：Parking System。剩餘車位只能由 addCar 合法遞減。
class ParkingSystem {
public:
    ParkingSystem(int big, int medium, int small) : spaces_{big, medium, small}
    {
        if (big < 0 || medium < 0 || small < 0) {
            throw std::invalid_argument("negative capacity");
        }
    }

    bool add_car(int car_type)
    {
        if (car_type < 1 || car_type > 3) {
            return false;
        }
        int& remaining = spaces_.at(static_cast<std::size_t>(car_type - 1));
        if (remaining == 0) {
            return false;
        }
        --remaining;
        return true;
    }

private:
    std::array<int, 3> spaces_;
};

void leetcode_1603_example()
{
    ParkingSystem parking(1, 1, 0);
    const bool big_accepted = parking.add_car(1);
    const bool medium_accepted = parking.add_car(2);
    const bool small_rejected = !parking.add_car(3);
    const bool second_big_rejected = !parking.add_car(1);
    assert(big_accepted);
    assert(medium_accepted);
    assert(small_rejected);
    assert(second_big_rejected);
    std::cout << "[LeetCode 1603] 車位 invariant 全由 add_car 維護\n";
}

// 工作案例：ThreadPoolConfig 建構後永遠保證 threads/queue 合理。
class ThreadPoolConfig {
public:
    static ThreadPoolConfig create(int threads, int queue_capacity)
    {
        if (threads < 1 || threads > 64 || queue_capacity < threads) {
            throw std::invalid_argument("invalid pool config");
        }
        return ThreadPoolConfig(threads, queue_capacity);
    }

    int threads() const { return threads_; }
    int queue_capacity() const { return queue_capacity_; }

private:
    ThreadPoolConfig(int threads, int queue_capacity)
        : threads_(threads), queue_capacity_(queue_capacity) {}
    int threads_;
    int queue_capacity_;
};

void practical_example()
{
    const auto config = ThreadPoolConfig::create(8, 100);
    assert(config.threads() == 8 && config.queue_capacity() == 100);
    std::cout << "[實務] pool threads=8 queue=100\n";
}

int main()
{
    basic_example();
    leetcode_1603_example();
    practical_example();
}

// 實務提醒與易錯：private 不只是隱藏，而是縮小合法狀態集合；回傳 non-const reference
// 會讓 caller 繞過驗證，等於把 invariant 的控制權重新公開。
// 練習：新增 deposit，拒絕 <=0；不要提供可修改 balance reference。
// 複雜度：private/public 是 compile-time access policy、無 runtime 成本；操作成本仍由資料結構決定。
// 生命週期：封裝不自動解決 borrow；若 getter 回 reference，caller 不得比 Account 活得更久。

// ================================================================================
// 編譯與執行（請先 cd 到本檔所在目錄）:
// g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -pthread '3_Encapsulation.cpp' -o '/tmp/codex_cpp_C_OOP_3_Encapsulation' && '/tmp/codex_cpp_C_OOP_3_Encapsulation'
//
// === 預期輸出（節錄）===
// [實務] pool threads=8 queue=100
// 程式正常結束（exit code 0）代表所有 assert／內建檢查均通過。
// ================================================================================
