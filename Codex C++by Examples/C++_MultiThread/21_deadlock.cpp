// ============================================================================
// Deadlock：四條件、鎖順序與避免在持鎖時呼叫外部程式
// ============================================================================
// Coffman 四條件：mutual exclusion、hold-and-wait、no preemption、circular wait。
// 打破任一條即可避免 deadlock。最常用策略：全系統固定 lock ordering，或用
// std::scoped_lock/std::lock 一次取得多把鎖。try_lock+退避可避免永久等待，但可能
// livelock/starvation；不是萬靈丹。
//
// 危險 AB/BA（只示意、不執行）：thread A 鎖 a 再 b；thread B 鎖 b 再 a。
// 除了 mutex，future.get、join、callback、queue capacity 也可形成 wait-for cycle。

#include <array>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct Account {
    std::mutex mutex;
    int balance;
};

bool transfer(Account& from, Account& to, int amount)
{
    // 同一個 Account 會對應同一把 non-recursive mutex；不可把它重複交給 scoped_lock。
    // 用 runtime check 固定 release/debug 都相同的契約：alias 與負金額皆回 false。
    if (&from == &to || amount < 0) return false;

    std::scoped_lock lock(from.mutex, to.mutex);
    if (from.balance < amount) return false;
    from.balance -= amount;
    to.balance += amount;
    return true;
}

void basic_demo()
{
    Account first{{}, 100};
    Account second{{}, 100};
    bool first_succeeded = false;
    bool second_succeeded = false;
    std::thread a([&] { first_succeeded = transfer(first, second, 10); });
    std::thread b([&] { second_succeeded = transfer(second, first, 20); });
    a.join();
    b.join();
    expect(first_succeeded && second_succeeded, "valid transfer was rejected");
    expect(!transfer(first, first, 1), "alias transfer must be rejected");
    {
        const std::scoped_lock lock(first.mutex, second.mutex);
        expect(first.balance + second.balance == 200,
               "transfer broke total-balance invariant");
    }
}

// ----------------------------------------------------------------------------
// LeetCode 1226：The Dining Philosophers（簡化一次進餐）
// ----------------------------------------------------------------------------
// scoped_lock 對左右兩叉使用 deadlock-avoidance。真題要處理五 philosopher 多輪；
// 本函式保留相同資源取得問題並驗證每位都完成一次。
void dine_once(int philosopher,
               std::array<std::mutex, 5>& forks,
               std::array<int, 5>& meals)
{
    const std::size_t left = static_cast<std::size_t>(philosopher);
    const std::size_t right = (left + 1U) % forks.size();
    std::scoped_lock lock(forks[left], forks[right]);
    ++meals[left];  // 每個 philosopher 只有自己的 slot writer。
}

void leetcode_demo()
{
    std::array<std::mutex, 5> forks;
    std::array<int, 5> meals{};
    std::array<std::thread, 5> philosophers{
        std::thread(dine_once, 0, std::ref(forks), std::ref(meals)),
        std::thread(dine_once, 1, std::ref(forks), std::ref(meals)),
        std::thread(dine_once, 2, std::ref(forks), std::ref(meals)),
        std::thread(dine_once, 3, std::ref(forks), std::ref(meals)),
        std::thread(dine_once, 4, std::ref(forks), std::ref(meals))};
    for (std::thread& philosopher : philosophers) philosopher.join();
    expect((meals == std::array<int, 5>{1, 1, 1, 1, 1}),
           "not every philosopher completed");
}

// ----------------------------------------------------------------------------
// 實務：跨兩資源更新；callback 放到解鎖後，避免 unknown code 反向取鎖
// ----------------------------------------------------------------------------
void update_both(Account& first,
                 Account& second,
                 const std::function<void()>& after_commit)
{
    // 這裡的語意是更新「兩個不同資源」。若 alias，丟出明確錯誤，不能把同一 mutex
    // 傳兩次給 scoped_lock；也不猜測究竟應該加一次或兩次。
    if (&first == &second) {
        throw std::invalid_argument("update_both requires two distinct accounts");
    }
    {
        std::scoped_lock lock(first.mutex, second.mutex);
        ++first.balance;
        ++second.balance;
    }
    after_commit();
}

void practical_demo()
{
    Account a{{}, 1};
    Account b{{}, 2};
    bool callback_ran = false;
    update_both(a, b, [&] { callback_ran = true; });
    expect(callback_ran && a.balance == 2 && b.balance == 3,
           "post-commit callback or account update failed");

    bool alias_rejected = false;
    try {
        update_both(a, a, [] {});
    } catch (const std::invalid_argument&) {
        alias_rejected = true;
    }
    expect(alias_rejected, "update_both must reject aliased resources");
}

int main()
{
    basic_demo();
    leetcode_demo();
    practical_demo();
    std::cout << "deadlock：scoped_lock 與 callback 邊界測試通過\n";
}

// 【陷阱】「每把 mutex 都有 RAII」仍可能 deadlock；RAII 只保證釋放，不保證順序。
// 【陷阱】assert 在 -DNDEBUG 會消失；不可用它執行 transfer 或守住 alias 前置條件。
// 【陷阱】持鎖 join 另一 thread，而對方需同鎖才能結束，是常見 shutdown deadlock。
// 【面試】deadlock、livelock、starvation：不動、一直退讓卻沒進度、長期拿不到資源。
// 【練習】畫 transfer 的 wait-for graph，說明 scoped_lock 如何移除 circular wait。
