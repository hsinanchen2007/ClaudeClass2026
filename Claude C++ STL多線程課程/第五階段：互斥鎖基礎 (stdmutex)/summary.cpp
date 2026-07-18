/*
 * ================================================================
 * 【第五階段：互斥鎖基礎 (std::mutex)】總複習 summary.cpp
 * ================================================================
 * 編譯方式：g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 本階段涵蓋課程：
 * - 課程 5.1：std::mutex 基本操作（lock/unlock）
 * - 課程 5.2：互斥鎖的工作原理
 * - 課程 5.3：try_lock() 非阻塞鎖定
 * - 課程 5.4：互斥鎖的常見錯誤
 * - 課程 5.5：保護共享資料實作
 * - 課程 5.6：互斥鎖的效能考量
 *
 * 重點摘要：
 * 1. std::mutex —— 最基本的互斥鎖
 * 2. lock_guard  —— RAII 鎖，自動 unlock，推薦使用
 * 3. unique_lock —— 靈活鎖，支援延遲鎖定、手動 unlock
 * 4. try_lock()  —— 非阻塞嘗試，失敗立即返回
 * 5. 死鎖（Deadlock）成因與防範
 * 6. 執行緒安全的類別設計模式
 * 7. 效能考量：細粒度鎖定
 * ================================================================
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <string>
#include <atomic>
using namespace std;

// ================================================================
// 課程 5.1：std::mutex 基本操作
// ================================================================
// 互斥鎖（Mutex）= Mutual Exclusion（互相排斥）
// 保證同一時刻只有一個執行緒可以進入「臨界區（Critical Section）」
//
// 基本操作：
//   mutex.lock()   —— 阻塞等待，取得鎖
//   mutex.unlock() —— 釋放鎖，讓其他執行緒可以進入
//
// 問題：沒有互斥鎖的計數器

namespace Lesson51 {

// 【錯誤示範】沒有鎖保護的計數器
int unsafe_counter = 0;
mutex counter_mutex;

void unsafe_increment() {
    for (int i = 0; i < 10000; ++i) {
        ++unsafe_counter;  // 非原子操作！讀-改-寫 可能被打斷
    }
}

// 【正確做法】用 mutex 保護
int safe_counter = 0;

void safe_increment() {
    for (int i = 0; i < 10000; ++i) {
        counter_mutex.lock();    // 鎖定（其他執行緒在此阻塞）
        ++safe_counter;          // 臨界區：只有一個執行緒可以執行
        counter_mutex.unlock();  // 釋放鎖
    }
}

void demo() {
    cout << "\n【5.1 基本 mutex 操作】" << endl;

    // 無鎖：結果不確定（通常小於 20000）
    unsafe_counter = 0;
    thread t1(unsafe_increment);
    thread t2(unsafe_increment);
    t1.join(); t2.join();
    cout << "  無鎖計數（預期 20000，實際）: " << unsafe_counter << endl;

    // 有鎖：結果正確（等於 20000）
    safe_counter = 0;
    thread t3(safe_increment);
    thread t4(safe_increment);
    t3.join(); t4.join();
    cout << "  有鎖計數（正確）: " << safe_counter << endl;
}

} // namespace Lesson51

// ================================================================
// 課程 5.2：lock_guard —— RAII 自動鎖（推薦）
// ================================================================
// 直接用 lock/unlock 的問題：若例外發生，可能永遠不會 unlock！
// lock_guard<mutex> 使用 RAII：
//   - 建構時 lock()
//   - 離開作用域時自動 unlock()（即使發生例外）
// lock_guard 是最推薦的鎖定方式

namespace Lesson52 {

mutex mtx;
int counter = 0;

void safe_increment_with_guard() {
    for (int i = 0; i < 10000; ++i) {
        lock_guard<mutex> guard(mtx);  // 建構時 lock()
        ++counter;
        // guard 離開作用域時自動 unlock()
    }
}

// 比較：忘記 unlock 的危險
void buggy_increment() {
    // mtx.lock();
    // ++counter;
    // 如果這裡拋出例外，mtx 永遠不會 unlock() → 死鎖！
    // mtx.unlock();
}

void demo() {
    cout << "\n【5.2 lock_guard RAII 鎖】" << endl;

    counter = 0;
    thread t1(safe_increment_with_guard);
    thread t2(safe_increment_with_guard);
    t1.join(); t2.join();
    cout << "  lock_guard 計數（正確）: " << counter << endl;
    cout << "  原理：建構=lock，解構=unlock，例外安全" << endl;
}

} // namespace Lesson52

// ================================================================
// 課程 5.3：try_lock() —— 非阻塞鎖定
// ================================================================
// mutex.lock()     —— 阻塞等待（直到取得鎖）
// mutex.try_lock() —— 非阻塞嘗試（立即返回 true/false）
//
// 用途：不想讓執行緒阻塞，失敗時執行其他任務

namespace Lesson53 {

mutex resource_mutex;

void worker_with_trylock(int id) {
    if (resource_mutex.try_lock()) {
        // 成功取得鎖
        cout << "  執行緒 " << id << "：取得鎖，執行工作" << endl;
        this_thread::sleep_for(chrono::milliseconds(50));
        cout << "  執行緒 " << id << "：工作完成，釋放鎖" << endl;
        resource_mutex.unlock();  // try_lock 成功後必須手動 unlock！
    } else {
        // 取鎖失敗，執行替代任務
        cout << "  執行緒 " << id << "：鎖定失敗，執行其他任務" << endl;
    }
}

void demo() {
    cout << "\n【5.3 try_lock 非阻塞鎖定】" << endl;

    thread t1(worker_with_trylock, 1);
    thread t2(worker_with_trylock, 2);
    thread t3(worker_with_trylock, 3);
    t1.join(); t2.join(); t3.join();
}

} // namespace Lesson53

// ================================================================
// 課程 5.4：互斥鎖常見錯誤與解決方案
// ================================================================
// 錯誤一：忘記 unlock（導致死鎖）→ 用 lock_guard 解決
// 錯誤二：死鎖（Deadlock）—— 兩個執行緒互相等待
// 錯誤三：重複 lock（同一執行緒）—— 用 recursive_mutex
// 錯誤四：lock 後忘記 unlock 又 lock → 用 unique_lock

namespace Lesson54 {

// 死鎖示範（避免在實際程式中這樣寫）
mutex m1, m2;

// 執行緒 A：先鎖 m1，再鎖 m2
// 執行緒 B：先鎖 m2，再鎖 m1
// → 可能死鎖！

// 解決死鎖：std::lock() 同時鎖定多個 mutex（無死鎖保證）
void safe_dual_lock() {
    // std::lock 保證無死鎖地同時鎖定 m1 和 m2
    lock(m1, m2);
    // adopt_lock 表示「已持有鎖，只負責解鎖」
    lock_guard<mutex> guard1(m1, adopt_lock);
    lock_guard<mutex> guard2(m2, adopt_lock);

    // 臨界區：持有 m1 和 m2
    cout << "  安全持有兩個鎖" << endl;
}  // guard1, guard2 析構，自動釋放 m1, m2

// scoped_lock（C++17）：更簡潔的多鎖方式
void safe_dual_lock_cpp17() {
    scoped_lock guard(m1, m2);  // C++17：同時鎖定，RAII，無死鎖
    cout << "  scoped_lock（C++17）：更簡潔的多鎖" << endl;
}

// recursive_mutex：允許同一執行緒重複 lock
recursive_mutex rmtx;
void recursive_function(int depth) {
    lock_guard<recursive_mutex> guard(rmtx);
    if (depth > 0) recursive_function(depth - 1);  // 遞迴鎖定，不死鎖
}

void demo() {
    cout << "\n【5.4 常見錯誤與解決方案】" << endl;

    // 安全的雙重鎖定
    thread t1(safe_dual_lock);
    thread t2(safe_dual_lock_cpp17);
    t1.join(); t2.join();

    // recursive_mutex 示範
    recursive_function(3);
    cout << "  recursive_mutex：遞迴鎖定成功" << endl;

    cout << "  死鎖防範原則：" << endl;
    cout << "    1. 固定加鎖順序" << endl;
    cout << "    2. 使用 std::lock() 同時鎖定" << endl;
    cout << "    3. 使用 scoped_lock（C++17）" << endl;
}

} // namespace Lesson54

// ================================================================
// 課程 5.5：執行緒安全的類別設計
// ================================================================
// 設計原則：封裝 mutex 在類別內部，讓外部使用者無需關心鎖

namespace Lesson55 {

class ThreadSafeCounter {
private:
    mutable mutex mtx;  // mutable：允許在 const 方法中鎖定
    int count = 0;

public:
    // 禁止複製（mutex 不可複製）
    ThreadSafeCounter() = default;
    ThreadSafeCounter(const ThreadSafeCounter&) = delete;
    ThreadSafeCounter& operator=(const ThreadSafeCounter&) = delete;

    void increment() {
        lock_guard<mutex> lock(mtx);
        ++count;
    }

    void decrement() {
        lock_guard<mutex> lock(mtx);
        --count;
    }

    void add(int value) {
        lock_guard<mutex> lock(mtx);
        count += value;
    }

    int get() const {
        lock_guard<mutex> lock(mtx);  // mutable mutex 在 const 中可用
        return count;
    }

    // 原子性的「讀取並重置」
    int reset() {
        lock_guard<mutex> lock(mtx);
        int old = count;
        count = 0;
        return old;
    }
};

void demo() {
    cout << "\n【5.5 執行緒安全類別設計】" << endl;

    ThreadSafeCounter counter;

    // 多個執行緒同時操作
    vector<thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; ++j) {
                counter.increment();
            }
        });
    }
    for (auto& t : threads) t.join();

    cout << "  10 個執行緒各加 1000 次，結果: " << counter.get()
         << "（期望 10000）" << endl;
}

} // namespace Lesson55

// ================================================================
// 課程 5.6：unique_lock —— 靈活鎖定
// ================================================================
// unique_lock 比 lock_guard 更靈活：
//   - 可延遲鎖定（defer_lock）
//   - 可手動 lock/unlock
//   - 可轉移所有權
//   - 配合 condition_variable 使用
// 代價：比 lock_guard 稍微有額外開銷

namespace Lesson56 {

mutex mtx;

void demo_unique_lock() {
    cout << "\n【5.6 unique_lock 靈活鎖定】" << endl;

    // 立即鎖定（等同 lock_guard）
    {
        unique_lock<mutex> lock(mtx);
        cout << "  unique_lock：立即鎖定" << endl;
    }  // 自動 unlock

    // 延遲鎖定（defer_lock）
    {
        unique_lock<mutex> lock(mtx, defer_lock);  // 建構但不鎖定
        cout << "  defer_lock：尚未鎖定" << endl;
        // ... 做些其他事情 ...
        lock.lock();   // 手動鎖定
        cout << "  手動 lock() 後鎖定" << endl;
        lock.unlock(); // 手動解鎖（可以多次 lock/unlock）
        cout << "  手動 unlock() 後解鎖" << endl;
        lock.lock();   // 再次鎖定
        cout << "  再次 lock()" << endl;
    }  // 自動 unlock

    // try_to_lock（非阻塞）
    {
        unique_lock<mutex> lock(mtx, try_to_lock);
        if (lock.owns_lock()) {
            cout << "  try_to_lock：成功取得鎖" << endl;
        } else {
            cout << "  try_to_lock：取鎖失敗" << endl;
        }
    }
}

} // namespace Lesson56

// ================================================================
// 效能考量總結
// ================================================================
//
// ┌──────────────────┬─────────────────────────────────────────────┐
// │ 鎖的類型          │ 適用場景                                    │
// ├──────────────────┼─────────────────────────────────────────────┤
// │ lock_guard       │ 簡單的作用域鎖定（推薦優先使用）            │
// │ scoped_lock(C++17)│ 同時鎖定多個 mutex（推薦）                 │
// │ unique_lock      │ 需要延遲鎖定、手動控制、配合 CV 使用        │
// │ shared_lock(C++17)│ 讀寫鎖的「讀取」部分                       │
// │ try_lock         │ 非阻塞嘗試，失敗時執行其他任務              │
// │ recursive_mutex  │ 同一執行緒需要遞迴鎖定                      │
// └──────────────────┴─────────────────────────────────────────────┘
//
// 效能原則：
// 1. 鎖定粒度越小越好（只鎖必要的最小範圍）
// 2. 持有鎖的時間越短越好
// 3. 優先用 atomic 替代簡單計數（效能更好，無鎖）
// 4. 讀多寫少時考慮 shared_mutex（讀寫鎖）

int main() {
    cout << "=============================================" << endl;
    cout << "   第五階段：互斥鎖基礎 (std::mutex) 總複習" << endl;
    cout << "=============================================" << endl;

    Lesson51::demo();
    Lesson52::demo();
    Lesson53::demo();
    Lesson54::demo();
    Lesson55::demo();
    Lesson56::demo_unique_lock();

    cout << "\n==============================================" << endl;
    cout << " 核心原則：" << endl;
    cout << " 1. 優先用 lock_guard（自動 RAII）" << endl;
    cout << " 2. 多個 mutex 用 scoped_lock（C++17）防死鎖" << endl;
    cout << " 3. 鎖住最小範圍，持有最短時間" << endl;
    cout << " 4. 封裝 mutex 在類別內，不讓外部處理鎖" << endl;
    cout << "==============================================" << endl;

    return 0;
}
