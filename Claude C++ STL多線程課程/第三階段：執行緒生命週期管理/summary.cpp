/*
 * ============================================================
 * 【第三階段：執行緒生命週期管理】總複習 summary.cpp
 * ============================================================
 *
 * 本階段涵蓋課程：
 *   - 課程 3.1：RAII 與執行緒管理
 *   - 課程 3.2：執行緒守衛類別設計
 *   - 課程 3.3：std::jthread (C++20)
 *   - 課程 3.4：執行緒例外處理
 *   - 課程 3.5：執行緒本地儲存
 *   - 課程 3.6：執行緒安全的初始化
 *
 * ============================================================
 * 重點摘要：
 *
 *  3.1  RAII 原則：建構取得資源、解構釋放資源。
 *       std::thread 若在 joinable 狀態下被解構，會呼叫
 *       std::terminate() 使程式崩潰。RAII 包裝類別讓
 *       執行緒無論正常返回或拋出例外都能被安全回收。
 *
 *  3.2  完整的執行緒守衛類別需支援移動語意（move）、
 *       完美轉發（perfect forwarding），並禁止複製。
 *       移動賦值前須先對自身的 joinable 執行緒呼叫 join()。
 *       可使用編譯期模板參數（if constexpr）選擇 join/detach。
 *
 *  3.3  std::jthread（C++20）是標準化的 RAII 執行緒：
 *         - 解構時自動呼叫 request_stop() + join()
 *         - 內建協作式取消機制：stop_token / stop_source
 *         - stop_callback 可在停止時觸發回調
 *
 *  3.4  執行緒中拋出的例外不會自動跨執行緒傳遞。
 *       解決方案：
 *         a) 在執行緒內部捕獲（最簡單，但無法傳回呼叫者）
 *         b) std::exception_ptr + current_exception()
 *            + rethrow_exception()（可跨執行緒傳遞）
 *         c) std::future / std::async（推薦，自動處理）
 *         d) std::promise::set_exception()（彈性控制）
 *
 *  3.5  thread_local 關鍵字讓每個執行緒擁有獨立的變數副本，
 *       無需同步。初始化發生在執行緒首次存取時，銷毀在
 *       執行緒結束時。常見用途：錯誤碼、快取、隨機數產生器。
 *
 *  3.6  std::call_once + std::once_flag 確保初始化函式只
 *       被執行一次，即使多個執行緒同時競爭。若初始化函式
 *       拋出例外，flag 不會被設定，下一個執行緒會繼續嘗試。
 *       C++11 起，函式內的 static 區域變數初始化本身也是
 *       執行緒安全的，可作為單例模式的最簡實作。
 *
 * ============================================================
 * 編譯指令（C++17，含 C++20 的 jthread 部分需 C++20）：
 *
 *   g++ -std=c++17 -pthread -o summary summary.cpp
 *
 * 若編譯器支援 C++20：
 *   g++ -std=c++20 -pthread -o summary summary.cpp
 *
 * ============================================================
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <exception>
#include <stdexcept>
#include <atomic>
#include <random>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <chrono>


// ============================================================
// ===== 課程 3.1：RAII 與執行緒管理 =====
// ============================================================
//
// 【核心問題】
//   std::thread 在 joinable 狀態（即執行緒已啟動但尚未 join/detach）
//   下被解構時，C++ 標準規定直接呼叫 std::terminate()，
//   造成程式崩潰（abort）。
//
//   以下是典型的危險情境：
//     void riskyFunction() {
//         std::thread t([]{ std::cout << "工作中\n"; });
//         throw std::runtime_error("錯誤！");
//         t.join();  // 永遠不會執行！
//     }  // t 仍是 joinable → std::terminate() 被呼叫！
//
// 【RAII 原則】
//   Resource Acquisition Is Initialization（資源獲取即初始化）
//   - 建構函式：獲取資源（例如：接管執行緒）
//   - 解構函式：釋放資源（例如：join 或 detach 執行緒）
//   - 優點：無論正常返回或拋出例外，解構函式都一定被呼叫，
//           確保資源被正確釋放，不會洩漏。
//
// ------------------------------------------------------------
// 【方案一】ThreadGuard：持有 std::thread 的「引用」
//
//   優點：設計簡單，適合在現有 std::thread 上加上 RAII 保護。
//   缺點：必須確保被引用的 std::thread 物件的生命週期比
//         ThreadGuard 長，不適合放入容器。
// ------------------------------------------------------------

class ThreadGuard {
    std::thread& t;  // 持有引用（非擁有）

public:
    // explicit 防止隱式轉換
    explicit ThreadGuard(std::thread& thread) : t(thread) {}

    ~ThreadGuard() {
        // 若執行緒仍是 joinable，就 join 它
        // joinable() 為 false 的情況：已被 join、已被 detach、
        // 或是預設建構的空執行緒
        if (t.joinable()) {
            t.join();
        }
    }

    // 禁止複製——複製後兩個 guard 都會嘗試 join 同一個執行緒，
    // 第二次 join 會因為不再 joinable 而拋出例外
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

// 使用示範
void demo_ThreadGuard() {
    std::thread t([]() {
        std::cout << "[ThreadGuard] 執行緒工作完成" << std::endl;
    });

    ThreadGuard guard(t);  // 保證 t 會被 join

    // 即使以下這行拋出例外，guard 的解構函式仍會執行並 join
    // throw std::runtime_error("測試例外");

    // 正常返回：guard 解構 → 自動 join t
}


// ------------------------------------------------------------
// 【方案二】ScopedThread：擁有（owns）std::thread
//
//   移動語意：透過 std::move 接管 std::thread 的所有權。
//   優點：執行緒與 ScopedThread 綁定，不會有懸空引用問題。
//         建構時立即驗證執行緒是否有效（joinable），
//         若無效就早失敗（fail fast），避免後續問題。
//   缺點：禁止複製，但可以加上移動支援使其放入容器。
// ------------------------------------------------------------

class ScopedThread {
    std::thread t;  // 擁有執行緒（不是引用）

public:
    // 接受一個右值 std::thread，移動進來
    explicit ScopedThread(std::thread thread)
        : t(std::move(thread))
    {
        // 立即檢查：如果傳入的是空執行緒（未關聯任何執行緒），
        // 則提前拋出例外，讓問題在建構時暴露
        if (!t.joinable()) {
            throw std::logic_error("ScopedThread：傳入的執行緒無效（not joinable）");
        }
    }

    ~ScopedThread() {
        // 因建構時已驗證，此處一定是 joinable，直接 join
        t.join();
    }

    // 禁止複製（std::thread 本身就不可複製）
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;
};

void demo_ScopedThread() {
    // 直接在建構時傳入匿名執行緒，省去手動管理
    ScopedThread st(std::thread([]() {
        std::cout << "[ScopedThread] 安全的執行緒，自動 join" << std::endl;
    }));
    // 離開此函式時，st 解構 → 自動 join
}


// ------------------------------------------------------------
// 【方案三】FlexibleThread：可選擇 join 或 detach
//
//   某些情況下，我們希望在作用域結束時自動 detach（讓執行緒
//   在背景繼續執行），而不是 join（等待完成）。
//   FlexibleThread 讓使用者在建構時指定行為。
// ------------------------------------------------------------

class FlexibleThread {
public:
    // 使用強型別列舉（enum class）避免整數隱式轉換的混淆
    enum class Action { join, detach };

private:
    std::thread t;
    Action action;

public:
    FlexibleThread(std::thread thread, Action a)
        : t(std::move(thread)), action(a) {}

    ~FlexibleThread() {
        if (t.joinable()) {
            if (action == Action::join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }

    FlexibleThread(const FlexibleThread&) = delete;
    FlexibleThread& operator=(const FlexibleThread&) = delete;
};

void demo_FlexibleThread() {
    // 這個執行緒在作用域結束時會被 join（等待完成）
    FlexibleThread ft1(
        std::thread([]() { std::cout << "[FlexibleThread] join 我" << std::endl; }),
        FlexibleThread::Action::join
    );

    // 這個執行緒在作用域結束時會被 detach（背景繼續）
    FlexibleThread ft2(
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // 注意：detach 後若主程式已退出，此輸出可能看不到
        }),
        FlexibleThread::Action::detach
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


// ============================================================
// ===== 課程 3.2：執行緒守衛類別設計 =====
// ============================================================
//
// 【設計目標】
//   一個完善的執行緒守衛類別應具備：
//     1. 自動在解構時 join（或 detach）
//     2. 支援移動語意——可以轉移所有權，可放入容器
//     3. 禁止複製——避免雙重 join
//     4. 完美轉發——直接在建構時傳入可呼叫物件與參數
//     5. 提供 join()、detach()、joinable()、get_id() 等工具方法
//
// 【完美轉發（Perfect Forwarding）】
//   template<typename Func, typename... Args>
//   JoiningThread(Func&& f, Args&&... args)
//       : t(std::forward<Func>(f), std::forward<Args>(args)...)
//   {}
//
//   std::forward 保留參數的值類別（左值/右值），
//   避免不必要的複製，並將參數「完美地」傳遞給 std::thread 的建構子。
//
// 【移動賦值的注意事項】
//   JoiningThread& operator=(JoiningThread&& other) noexcept {
//       if (this != &other) {
//           if (t.joinable()) {
//               t.join();  // ← 先處理自身管理的執行緒！
//           }              //   否則被覆蓋時就會 terminate
//           t = std::move(other.t);
//       }
//       return *this;
//   }
// ------------------------------------------------------------

class JoiningThread {
    std::thread t;

public:
    // 預設建構：不管理任何執行緒（t 是空的）
    JoiningThread() noexcept = default;

    // 從一個已存在的 std::thread 移動建構（接管所有權）
    explicit JoiningThread(std::thread thread) noexcept
        : t(std::move(thread)) {}

    // 直接建構執行緒（完美轉發可呼叫物件與所有參數）
    template<typename Func, typename... Args>
    explicit JoiningThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 移動建構：從另一個 JoiningThread 搬走執行緒
    JoiningThread(JoiningThread&& other) noexcept
        : t(std::move(other.t)) {}

    // 移動賦值：先 join 自己的執行緒，再接管別人的
    JoiningThread& operator=(JoiningThread&& other) noexcept {
        if (this != &other) {
            if (t.joinable()) {
                t.join();  // 重要：先結束自己管理的執行緒
            }
            t = std::move(other.t);
        }
        return *this;
    }

    // 禁止複製
    JoiningThread(const JoiningThread&) = delete;
    JoiningThread& operator=(const JoiningThread&) = delete;

    // 解構：自動 join
    ~JoiningThread() {
        if (t.joinable()) {
            t.join();
        }
    }

    // --- 工具方法（委派給底層 std::thread）---
    bool joinable() const noexcept { return t.joinable(); }
    std::thread::id get_id() const noexcept { return t.get_id(); }
    void join() { t.join(); }
    void detach() { t.detach(); }
    std::thread& get() noexcept { return t; }
};

void demo_JoiningThread() {
    // 方式一：直接在建構時傳入 lambda（完美轉發）
    JoiningThread jt1([]() {
        std::cout << "[JoiningThread] 方式一：直接建構" << std::endl;
    });

    // 方式二：從已存在的 std::thread 移動
    std::thread raw([]() {
        std::cout << "[JoiningThread] 方式二：從 std::thread 移動" << std::endl;
    });
    JoiningThread jt2(std::move(raw));
    // 注意：raw 移動後成為空執行緒，不可再使用

    // 方式三：帶參數
    JoiningThread jt3([](int x, const std::string& s) {
        std::cout << "[JoiningThread] 方式三：帶參數 x=" << x
                  << " s=" << s << std::endl;
    }, 42, std::string("hello"));

    // 所有 JoiningThread 在此函式結束時自動 join
}

void demo_JoiningThread_in_vector() {
    // 因為 JoiningThread 支援移動語意，可以放入 vector
    std::vector<JoiningThread> threads;

    for (int i = 0; i < 4; ++i) {
        // emplace_back 使用完美轉發，直接在 vector 內部建構
        threads.emplace_back([i]() {
            std::cout << "[Vector] Worker " << i << " 執行中" << std::endl;
        });
    }

    std::cout << "[Vector] 所有執行緒已建立，等待結束..." << std::endl;
    // vector 解構時，每個 JoiningThread 的解構函式自動 join
}


// ------------------------------------------------------------
// 【DetachingThread】：解構時自動 detach 的變體
//
//   適合「發射後不管」（fire-and-forget）的背景任務。
//   注意：detach 後若主程式退出，未完成的執行緒會被強制結束，
//   可能導致資源未釋放，使用時需謹慎。
// ------------------------------------------------------------

class DetachingThread {
    std::thread t;

public:
    template<typename Func, typename... Args>
    explicit DetachingThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    // 允許移動
    DetachingThread(DetachingThread&&) = default;
    DetachingThread& operator=(DetachingThread&&) = default;

    DetachingThread(const DetachingThread&) = delete;
    DetachingThread& operator=(const DetachingThread&) = delete;

    ~DetachingThread() {
        if (t.joinable()) {
            t.detach();  // 解構時自動 detach，不等待
        }
    }
};


// ------------------------------------------------------------
// 【ManagedThread】：以模板參數在編譯期決定行為
//
//   使用 if constexpr（C++17）在編譯期選擇分支，
//   沒有任何執行期開銷。使用型別別名增加可讀性。
// ------------------------------------------------------------

enum class ThreadAction { Join, Detach };

template<ThreadAction action>
class ManagedThread {
    std::thread t;

public:
    template<typename Func, typename... Args>
    explicit ManagedThread(Func&& f, Args&&... args)
        : t(std::forward<Func>(f), std::forward<Args>(args)...) {}

    ManagedThread(ManagedThread&&) = default;
    ManagedThread& operator=(ManagedThread&&) = default;

    ManagedThread(const ManagedThread&) = delete;
    ManagedThread& operator=(const ManagedThread&) = delete;

    ~ManagedThread() {
        if (t.joinable()) {
            if constexpr (action == ThreadAction::Join) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
};

// 型別別名：讓使用者不用每次寫完整的模板語法
using AutoJoinThread   = ManagedThread<ThreadAction::Join>;
using AutoDetachThread = ManagedThread<ThreadAction::Detach>;

void demo_ManagedThread() {
    AutoJoinThread jt([]() {
        std::cout << "[ManagedThread] AutoJoinThread：離開作用域時自動 join" << std::endl;
    });
    // jt 解構時自動 join
}


// ============================================================
// ===== 課程 3.3：std::jthread (C++20) =====
// ============================================================
//
// 【std::jthread 是什麼？】
//   C++20 引入的 std::jthread（joining thread）是 std::thread
//   的改良版，內建兩大功能：
//     1. RAII 自動 join：解構時自動呼叫 request_stop() + join()
//     2. 協作式取消機制：內建 stop_token / stop_source 系統
//
// 【std::thread vs std::jthread 對比】
//
//   std::thread                    std::jthread
//   ─────────────────────          ─────────────────────────────
//   必須手動呼叫 join/detach        解構時自動 join（永不忘記）
//   忘記 join → std::terminate()   不會有這個問題
//   無內建取消機制                 內建 stop_token 協作式取消
//   C++11 起可用                   C++20 起可用
//
// 【stop_token 機制三元件】
//   std::stop_source  → 持有者，可以發出停止請求
//   std::stop_token   → 觀察者，檢查是否已收到停止請求
//   std::stop_callback → 當停止被請求時，自動執行指定的回調函式
//
// 【jthread 的完整介面（概念性）】
//   class jthread {
//   public:
//       jthread() noexcept;
//       template<typename F, typename... Args>
//       explicit jthread(F&& f, Args&&... args);
//       ~jthread();  // 自動 request_stop() + join()
//       jthread(jthread&&) noexcept;
//       jthread& operator=(jthread&&) noexcept;
//       stop_source get_stop_source() noexcept;
//       stop_token get_stop_token() const noexcept;
//       bool request_stop() noexcept;
//       bool joinable() const noexcept;
//       void join();
//       void detach();
//       id get_id() const noexcept;
//       static unsigned int hardware_concurrency() noexcept;
//   };
// ------------------------------------------------------------

// 【jthread 基本用法示範】
// （此函式使用 std::jthread，需 C++20 編譯）
//
// void demo_jthread_basic() {
//     std::jthread jt([]() {
//         std::cout << "[jthread] Hello from jthread！" << std::endl;
//     });
//     // 不需要呼叫 join()！
//     // 離開作用域時，jt 解構 → 自動 join
// }

// 【jthread 例外安全示範】
// 即使函式拋出例外，jthread 也能正確 join
//
// void demo_jthread_exception_safe() {
//     std::jthread jt([]() {
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         std::cout << "[jthread] 執行緒完成（即使發生例外）" << std::endl;
//     });
//     throw std::runtime_error("例外！");
//     // jt 解構時會先等待執行緒結束，再傳播例外
// }

// 【stop_token 協作式取消示範】
// 執行緒函式的第一個參數若為 std::stop_token，
// jthread 會自動注入它的 stop_token
//
// void demo_jthread_stop_token() {
//     std::jthread jt([](std::stop_token stoken) {
//         int count = 0;
//         while (!stoken.stop_requested()) {
//             std::cout << "[jthread] 工作中... " << ++count << std::endl;
//             std::this_thread::sleep_for(std::chrono::milliseconds(200));
//         }
//         std::cout << "[jthread] 收到停止請求，結束" << std::endl;
//     });
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     jt.request_stop();  // 請求停止
//     // jt 解構時先確認執行緒已結束再返回
// }

// 【stop_callback 示範】
// 停止被請求時，自動執行回調（不需要執行緒主動輪詢）
//
// void demo_jthread_stop_callback() {
//     std::jthread jt([](std::stop_token stoken) {
//         // 當停止被請求時，callback 會被立即觸發
//         std::stop_callback cb(stoken, []() {
//             std::cout << "[jthread] 停止回調：正在清理資源..." << std::endl;
//         });
//
//         while (!stoken.stop_requested()) {
//             std::this_thread::sleep_for(std::chrono::milliseconds(300));
//         }
//         std::cout << "[jthread] 執行緒結束" << std::endl;
//     });
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     jt.request_stop();
// }

// 【jthread 存放於 vector 的示範】
// void demo_jthread_vector() {
//     std::vector<std::jthread> workers;
//
//     for (int i = 0; i < 3; ++i) {
//         workers.emplace_back([](std::stop_token stoken, int id) {
//             std::cout << "Worker " << id << " 啟動" << std::endl;
//             int count = 0;
//             while (!stoken.stop_requested()) {
//                 ++count;
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             }
//             std::cout << "Worker " << id << " 結束，執行了 " << count << " 次" << std::endl;
//         }, i);
//     }
//
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//
//     // 逐一請求停止
//     for (auto& w : workers) {
//         w.request_stop();
//     }
//     // vector 解構時，每個 jthread 自動 join
// }

// 【何時用 jthread vs thread】
// C++20 可用時    → 優先使用 jthread（更安全、功能更豐富）
// 需要取消機制    → 使用 jthread（內建 stop_token）
// 需要 detach     → 使用 thread（jthread 不支援後台 detach）
// 舊版本相容性    → 使用 thread + 自訂守衛類別（如 JoiningThread）


// ============================================================
// ===== 課程 3.4：執行緒例外處理 =====
// ============================================================
//
// 【核心問題】
//   執行緒是獨立的執行環境，每個執行緒有自己的呼叫堆疊。
//   在執行緒 A 中拋出的例外，不會自動傳遞到執行緒 B。
//   若例外在執行緒函式內未被捕獲，C++ 會呼叫 std::terminate()，
//   導致整個程式崩潰（不只是那個執行緒）。
//
//   【錯誤示範】
//     void worker() { throw std::runtime_error("錯誤！"); }
//     int main() {
//         std::thread t(worker);
//         try {
//             t.join();
//         } catch (...) { /* 捕獲不到！*/ }
//     }
//     → 程式因未捕獲例外而 abort
//
// 【解決方案 A】執行緒內部捕獲（最簡單）
//   適合：錯誤在執行緒內部可以完全處理的情況
//   缺點：呼叫端無法得知執行緒是否發生錯誤
// ------------------------------------------------------------

void demo_exception_internal_catch() {
    auto worker = []() {
        try {
            throw std::runtime_error("執行緒內的錯誤！");
        } catch (const std::exception& e) {
            // 在執行緒內部處理，外部無法感知
            std::cout << "[方案A] 執行緒內捕獲：" << e.what() << std::endl;
        }
    };

    std::thread t(worker);
    t.join();
    std::cout << "[方案A] 主執行緒：程式正常繼續" << std::endl;
}


// ------------------------------------------------------------
// 【解決方案 B】std::exception_ptr 跨執行緒傳遞例外
//
//   std::exception_ptr 是一個可以儲存任何例外的智慧指標。
//   關鍵函式：
//     std::current_exception()  → 在 catch 區塊中，捕獲當前例外
//                                  並回傳 exception_ptr
//     std::rethrow_exception()  → 重新拋出 exception_ptr 所儲存的例外
//     std::make_exception_ptr() → 直接從例外物件建立 exception_ptr
//
//   流程：
//     1. 執行緒內部：catch → current_exception() → 存入共享變數
//     2. 主執行緒：join 後 → 檢查共享變數 → rethrow_exception()
// ------------------------------------------------------------

void demo_exception_ptr() {
    std::exception_ptr exPtr = nullptr;  // 共享的例外儲存位置

    auto worker = [&exPtr]() {
        try {
            throw std::runtime_error("來自工作執行緒的錯誤！");
        } catch (...) {
            // 捕獲所有例外，儲存到 exPtr
            exPtr = std::current_exception();
        }
    };

    std::thread t(worker);
    t.join();  // 等待執行緒完成

    // 現在可以安全地在主執行緒中處理例外
    if (exPtr) {
        try {
            std::rethrow_exception(exPtr);  // 重新拋出例外
        } catch (const std::exception& e) {
            std::cout << "[方案B] 主執行緒捕獲例外：" << e.what() << std::endl;
        }
    }
}


// ------------------------------------------------------------
// 【SafeThread】：封裝例外傳遞的 RAII 類別
//
//   將「例外儲存」的邏輯封裝在類別中，讓使用者透過 join()
//   自動感知執行緒中的例外，使用方式接近同步函式呼叫。
// ------------------------------------------------------------

class SafeThread {
    std::thread t;
    std::exception_ptr exPtr = nullptr;

public:
    template<typename Func, typename... Args>
    explicit SafeThread(Func&& f, Args&&... args) {
        // 包裝原本的函式，在外層加上例外捕獲
        t = std::thread([this,
                         func = std::forward<Func>(f)]() mutable {
            try {
                func();
            } catch (...) {
                exPtr = std::current_exception();
            }
        });
    }

    // join 後若有例外，重新拋出（讓呼叫端感知）
    void join() {
        t.join();
        if (exPtr) {
            std::rethrow_exception(exPtr);
        }
    }

    ~SafeThread() {
        if (t.joinable()) {
            t.join();  // RAII：確保執行緒被 join
        }
    }
};

void demo_SafeThread() {
    try {
        SafeThread st([]() {
            throw std::runtime_error("SafeThread 內的錯誤！");
        });
        st.join();  // 例外在此被重新拋出
    } catch (const std::exception& e) {
        std::cout << "[SafeThread] 主執行緒捕獲：" << e.what() << std::endl;
    }
}


// ------------------------------------------------------------
// 【解決方案 C】std::future / std::async（推薦方式）
//
//   std::async 自動在另一個執行緒執行函式，並將結果（或例外）
//   儲存在 std::future 中。呼叫 future.get() 時：
//     - 若成功：返回結果值
//     - 若例外：重新拋出例外
//   整個過程自動完成，無需手動管理 exception_ptr。
//
//   std::launch::async → 強制在新執行緒執行（不延遲）
//   std::launch::deferred → 延遲執行（直到 get() 被呼叫）
// ------------------------------------------------------------

int task_that_may_fail(int id) {
    if (id == 2) {
        throw std::runtime_error("Task 2 失敗（模擬錯誤）");
    }
    return id * 10;
}

void demo_future_exception() {
    std::vector<std::future<int>> futures;

    // 啟動多個非同步任務
    for (int i = 0; i < 5; ++i) {
        futures.push_back(
            std::async(std::launch::async, task_that_may_fail, i)
        );
    }

    // 收集結果（例外在 get() 時被重新拋出）
    for (int i = 0; i < 5; ++i) {
        try {
            int result = futures[i].get();
            std::cout << "[future] Task " << i << " 結果：" << result << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[future] Task " << i << " 例外：" << e.what() << std::endl;
        }
    }
}


// ------------------------------------------------------------
// 【解決方案 D】std::promise 精確控制例外傳遞
//
//   std::promise 允許在任意時間點設定值或例外，
//   搭配 std::future 可以靈活地在執行緒間傳遞結果。
//
//   prom.set_value(42)              → 設定成功結果
//   prom.set_exception(exPtr)       → 設定例外（當 future.get()
//                                      時會重新拋出）
// ------------------------------------------------------------

void promise_worker(std::promise<int> prom) {
    try {
        throw std::runtime_error("Promise worker 發生錯誤！");
        prom.set_value(42);  // 若無例外，設定結果
    } catch (...) {
        // 將例外傳遞給 future
        prom.set_exception(std::current_exception());
    }
}

void demo_promise_exception() {
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();

    std::thread t(promise_worker, std::move(prom));

    try {
        int value = fut.get();  // 等待並取得結果（或重新拋出例外）
        std::cout << "[promise] 結果：" << value << std::endl;
    } catch (const std::exception& e) {
        std::cout << "[promise] 主執行緒捕獲：" << e.what() << std::endl;
    }

    t.join();
}


// ============================================================
// ===== 課程 3.5：執行緒本地儲存 =====
// ============================================================
//
// 【什麼是 thread_local？】
//   thread_local 關鍵字（C++11 起）使每個執行緒擁有獨立的
//   變數副本。各執行緒對自己的副本進行讀寫，互不干擾，
//   因此不需要任何同步（mutex/atomic）。
//
// 【變數儲存類型比較】
//
//   全域/static 變數         thread_local 變數
//   ────────────────────    ────────────────────────────
//   所有執行緒共享一份        每個執行緒各有一份
//   需要 mutex 等同步保護     不需要同步（各自獨立）
//   程式啟動時初始化          首次存取時（執行緒開始後）初始化
//   程式結束時銷毀            執行緒結束時銷毀（呼叫解構函式）
//
// 【thread_local 可用的三個位置】
//   1. 全域變數：thread_local int g_tl = 0;
//   2. 函式內的 static 變數：thread_local static int f_tl = 0;
//   3. 類別的 static 成員：thread_local static int cls_tl;
//
// 【生命週期】
//   執行緒啟動
//     → 首次存取時初始化（若為複雜物件，呼叫建構函式）
//     → 執行緒執行期間持續存在
//     → 執行緒結束時銷毀（呼叫解構函式，注意析構順序）
// ------------------------------------------------------------

// 全域 thread_local 變數：每個執行緒有獨立的 counter
thread_local int tl_counter = 0;

void demo_thread_local_basic() {
    auto increment = [](const std::string& name) {
        for (int i = 0; i < 3; ++i) {
            ++tl_counter;
            std::cout << "[thread_local] " << name
                      << ": counter = " << tl_counter << std::endl;
        }
    };

    std::thread t1(increment, "Thread A");
    std::thread t2(increment, "Thread B");

    t1.join();
    t2.join();

    // 兩個執行緒的 counter 各自從 0 開始，互不影響
    // 預期輸出：A 和 B 各自計數到 3（但輸出順序不定）
}


// 對比：普通全域變數（共享）vs thread_local（獨立）
int global_counter = 0;              // 所有執行緒共享，有競爭條件
thread_local int local_tl_counter = 0;  // 每個執行緒獨立

void demo_global_vs_thread_local() {
    auto work = [](const std::string& name) {
        ++global_counter;   // 非原子操作，有競爭條件（僅示範用）
        ++local_tl_counter;

        std::cout << "[對比] " << name
                  << " global=" << global_counter
                  << " local=" << local_tl_counter << std::endl;
    };

    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");

    t1.join();
    t2.join();
    t3.join();

    // global_counter 會是 3（三個執行緒各加一次，但有競爭）
    // local_tl_counter 每個執行緒都是 1（各自獨立）
}


// 【用途一】執行緒專屬錯誤碼（類似 POSIX errno）
//   errno 就是用 thread_local 實作的，確保每個執行緒
//   設定和讀取自己的錯誤碼，不互相干擾。
thread_local int tl_last_error = 0;

void setThreadError(int code) { tl_last_error = code; }
int  getThreadError() { return tl_last_error; }

void demo_thread_local_error_code() {
    auto worker = [](int id) {
        setThreadError(id * 100);
        std::cout << "[errno] Thread " << id
                  << " error: " << getThreadError() << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
    // Thread 1 → error: 100，Thread 2 → error: 200，互不影響
}


// 【用途二】執行緒專屬快取
//   每個執行緒有自己的快取，避免快取競爭（cache contention）
//   和同步開銷，可以顯著提高效能。
thread_local std::string tl_cache;

std::string getComputedResult(int thread_id) {
    if (tl_cache.empty()) {
        // 模擬耗時計算（每個執行緒只計算一次）
        tl_cache = "Thread-" + std::to_string(thread_id) + "-result";
        std::cout << "[cache] Thread " << thread_id << " 首次計算..." << std::endl;
    }
    return tl_cache;  // 後續呼叫直接從快取返回
}

void demo_thread_local_cache() {
    auto worker = [](int id) {
        // 呼叫兩次：第一次計算，第二次從快取取
        std::cout << "[cache] " << getComputedResult(id) << std::endl;
        std::cout << "[cache] " << getComputedResult(id) << " (快取)" << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
}


// 【用途三】執行緒專屬隨機數產生器
//   std::mt19937 不是執行緒安全的（且使用 mutex 共享一個產生器
//   會成為效能瓶頸）。使用 thread_local 讓每個執行緒有自己的
//   隨機數引擎，既安全又高效。
thread_local std::mt19937 tl_rng{std::random_device{}()};

int threadSafeRandInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(tl_rng);  // 每個執行緒使用自己的 rng，無需同步
}

void demo_thread_local_rng() {
    auto worker = [](int id) {
        std::cout << "[rng] Thread " << id << ": "
                  << threadSafeRandInt(1, 100) << ", "
                  << threadSafeRandInt(1, 100) << ", "
                  << threadSafeRandInt(1, 100) << std::endl;
    };

    std::thread t1(worker, 1);
    std::thread t2(worker, 2);

    t1.join();
    t2.join();
}


// 【thread_local 物件的生命週期展示】
class TlResource {
    int id;
public:
    TlResource(int i) : id(i) {
        std::cout << "[TlResource] Thread " << id << " Resource 建構" << std::endl;
    }
    ~TlResource() {
        std::cout << "[TlResource] Thread " << id << " Resource 銷毀" << std::endl;
    }
};

// 注意：thread_local 物件在執行緒首次存取時建構，執行緒結束時銷毀
// thread_local TlResource tl_res{0};  // 每個執行緒有自己的 TlResource
// （此處注釋掉以避免在主執行緒啟動時就建構）


// ============================================================
// ===== 課程 3.6：執行緒安全的初始化 =====
// ============================================================
//
// 【核心問題：競爭條件（Race Condition）下的初始化】
//   當多個執行緒同時執行 "if (ptr == nullptr) { ptr = new X(); }"
//   時，可能有多個執行緒都通過了 nullptr 檢查，導致物件被
//   初始化多次，造成記憶體洩漏或未定義行為。
//
//   這是「雙重檢查鎖定（Double-Checked Locking）」的問題根源。
//   在 C++11 記憶體模型出現前，即使加了鎖，DCL 也可能因
//   編譯器/CPU 重排而失效。
//
// 【解決方案一】std::call_once + std::once_flag
//
//   std::once_flag flag;
//   void initDB() { db = new Database(); }
//   void use() {
//       std::call_once(flag, initDB);  // 保證只執行一次
//       db->query();
//   }
//
//   工作原理：
//     - 第一個到達的執行緒執行初始化函式
//     - 其他執行緒等待直到初始化函式完成
//     - 之後所有呼叫直接略過（flag 已設定）
//     - 若初始化函式拋出例外：
//         * flag 不會被設定
//         * 例外傳播給當前執行緒
//         * 下一個執行緒會再次嘗試初始化
//
// 【解決方案二】函式內 static 區域變數（C++11 保證安全）
//   C++11 標準明確規定：static 區域變數的初始化是執行緒安全的。
//   編譯器/runtime 會自動確保只有一個執行緒執行初始化。
// ------------------------------------------------------------

// --- 不安全的初始化（示範問題，請勿在生產環境使用）---
// Database* db = nullptr;
// void unsafe_init() {
//     if (db == nullptr) {      // 多個執行緒可能同時通過此檢查！
//         db = new Database();  // 可能被初始化多次！
//     }
// }


// --- call_once 示範 ---

class Database {
    int id;
public:
    Database(int i = 0) : id(i) {
        std::cout << "[Database] 初始化（id=" << id << "）" << std::endl;
    }
    void query(int thread_id) const {
        std::cout << "[Database] Thread " << thread_id
                  << " 查詢中（db id=" << id << "）" << std::endl;
    }
};

Database* g_db = nullptr;
std::once_flag g_db_init_flag;

void demo_call_once_basic() {
    auto initDatabase = []() {
        g_db = new Database(42);
    };

    auto initAndUse = [&](int thread_id) {
        // 無論多少執行緒同時呼叫，initDatabase 只會被執行一次
        std::call_once(g_db_init_flag, initDatabase);
        g_db->query(thread_id);
    };

    std::thread t1(initAndUse, 1);
    std::thread t2(initAndUse, 2);
    std::thread t3(initAndUse, 3);

    t1.join();
    t2.join();
    t3.join();

    delete g_db;
    g_db = nullptr;
}


// --- call_once 與例外：失敗後重試 ---

std::once_flag retry_flag;
std::atomic<int> attempt_count{0};

void mayFailInit() {
    int current = ++attempt_count;
    std::cout << "[call_once] 嘗試 #" << current << std::endl;

    if (current < 3) {
        throw std::runtime_error("初始化失敗（模擬）");
    }

    std::cout << "[call_once] 初始化成功！" << std::endl;
}

void demo_call_once_with_exception() {
    auto worker = []() {
        try {
            // 若 mayFailInit 拋出例外，flag 不會被設定，
            // 下一個執行緒會再次嘗試
            std::call_once(retry_flag, mayFailInit);
            std::cout << "[call_once] 繼續執行..." << std::endl;
        } catch (const std::exception& e) {
            std::cout << "[call_once] 捕獲例外：" << e.what() << std::endl;
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);
    std::thread t4(worker);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}


// --- 執行緒安全的單例模式（Singleton）---
// 【方法一】使用 call_once（明確控制初始化）

class SingletonV1 {
    static SingletonV1* instance;
    static std::once_flag init_flag;

    SingletonV1() {
        std::cout << "[Singleton v1] 建立（call_once 版本）" << std::endl;
    }

public:
    SingletonV1(const SingletonV1&) = delete;
    SingletonV1& operator=(const SingletonV1&) = delete;

    static SingletonV1& getInstance() {
        std::call_once(init_flag, []() {
            instance = new SingletonV1();
        });
        return *instance;
    }

    void doWork() {
        std::cout << "[Singleton v1] 執行工作" << std::endl;
    }
};

SingletonV1* SingletonV1::instance = nullptr;
std::once_flag SingletonV1::init_flag;


// 【方法二】使用 static 區域變數（推薦，C++11 保證執行緒安全）
//   C++11 標準 §6.7 規定：若多個執行緒同時進入 static 區域變數
//   的宣告，初始化只會由一個執行緒執行，其他執行緒等待完成。

class SingletonV2 {
    SingletonV2() {
        std::cout << "[Singleton v2] 建立（static 局部變數版本）" << std::endl;
    }

public:
    SingletonV2(const SingletonV2&) = delete;
    SingletonV2& operator=(const SingletonV2&) = delete;

    static SingletonV2& getInstance() {
        static SingletonV2 instance;  // C++11 保證執行緒安全！
        return instance;
    }

    void doWork() {
        std::cout << "[Singleton v2] 執行工作" << std::endl;
    }
};


// 【延遲初始化（Lazy Initialization）封裝示範】
//   使用 call_once 實作成員的延遲初始化——只在第一次需要時才建構，
//   並且保證多執行緒環境下只建構一次。

class ServiceWithLazyCache {
    mutable std::once_flag cache_flag;
    mutable std::unique_ptr<std::string> cache;

    void initCache() const {
        std::cout << "[LazyCache] 正在初始化快取..." << std::endl;
        cache = std::make_unique<std::string>("快取資料（只初始化一次）");
    }

public:
    // const 方法：邏輯上不修改物件，但允許延遲初始化
    // 使用 mutable 讓 const 方法能修改 cache_flag 和 cache
    const std::string& getCache() const {
        std::call_once(cache_flag, &ServiceWithLazyCache::initCache, this);
        return *cache;
    }
};

void demo_lazy_init() {
    ServiceWithLazyCache service;

    std::thread t1([&]() {
        std::cout << "[LazyCache] Thread 1: " << service.getCache() << std::endl;
    });
    std::thread t2([&]() {
        std::cout << "[LazyCache] Thread 2: " << service.getCache() << std::endl;
    });

    t1.join();
    t2.join();
    // initCache() 只被呼叫一次，兩個執行緒共用同一份快取
}

void demo_singleton() {
    // 方法一：call_once 版本
    std::thread ta([]() { SingletonV1::getInstance().doWork(); });
    std::thread tb([]() { SingletonV1::getInstance().doWork(); });
    ta.join();
    tb.join();

    // 方法二：static 區域變數版本（更簡潔）
    std::thread tc([]() { SingletonV2::getInstance().doWork(); });
    std::thread td([]() { SingletonV2::getInstance().doWork(); });
    tc.join();
    td.join();
}


// ============================================================
// ===== main()：示範本階段所有關鍵概念 =====
// ============================================================

int main() {
    std::cout << "\n"
              << "============================================================\n"
              << " 第三階段：執行緒生命週期管理 - 總複習示範\n"
              << "============================================================\n"
              << std::endl;

    // --- 3.1 RAII 與執行緒管理 ---
    std::cout << "--- 課程 3.1：RAII 與執行緒管理 ---\n" << std::endl;

    std::cout << "[3.1.1] ThreadGuard（引用版 RAII）：\n";
    demo_ThreadGuard();

    std::cout << "\n[3.1.2] ScopedThread（擁有版 RAII）：\n";
    demo_ScopedThread();

    std::cout << "\n[3.1.3] FlexibleThread（可選 join/detach）：\n";
    demo_FlexibleThread();

    // --- 3.2 執行緒守衛類別設計 ---
    std::cout << "\n--- 課程 3.2：執行緒守衛類別設計 ---\n" << std::endl;

    std::cout << "[3.2.1] JoiningThread（完整守衛類別）：\n";
    demo_JoiningThread();

    std::cout << "\n[3.2.2] JoiningThread 放入 vector：\n";
    demo_JoiningThread_in_vector();

    std::cout << "\n[3.2.3] ManagedThread（編譯期決定行為）：\n";
    demo_ManagedThread();

    // --- 3.3 std::jthread (C++20) ---
    std::cout << "\n--- 課程 3.3：std::jthread (C++20) ---\n" << std::endl;
    std::cout << "[3.3] std::jthread 需要 C++20，請參閱上方已注釋的示範函式：\n"
              << "      demo_jthread_basic()          → 基本使用\n"
              << "      demo_jthread_exception_safe() → 例外安全\n"
              << "      demo_jthread_stop_token()     → stop_token 取消\n"
              << "      demo_jthread_stop_callback()  → stop_callback 回調\n"
              << "      demo_jthread_vector()         → vector 中的 jthread\n"
              << std::endl;

    // --- 3.4 執行緒例外處理 ---
    std::cout << "--- 課程 3.4：執行緒例外處理 ---\n" << std::endl;

    std::cout << "[3.4.A] 執行緒內部捕獲：\n";
    demo_exception_internal_catch();

    std::cout << "\n[3.4.B] std::exception_ptr 跨執行緒傳遞：\n";
    demo_exception_ptr();

    std::cout << "\n[3.4.C] SafeThread RAII 包裝：\n";
    demo_SafeThread();

    std::cout << "\n[3.4.D] std::future / std::async（推薦）：\n";
    demo_future_exception();

    std::cout << "\n[3.4.E] std::promise 精確控制：\n";
    demo_promise_exception();

    // --- 3.5 執行緒本地儲存 ---
    std::cout << "\n--- 課程 3.5：執行緒本地儲存 ---\n" << std::endl;

    std::cout << "[3.5.1] thread_local 基本示範（各自獨立計數）：\n";
    demo_thread_local_basic();

    std::cout << "\n[3.5.2] 全域變數 vs thread_local 對比：\n";
    demo_global_vs_thread_local();

    std::cout << "\n[3.5.3] 用途一：執行緒專屬錯誤碼：\n";
    demo_thread_local_error_code();

    std::cout << "\n[3.5.4] 用途二：執行緒專屬快取：\n";
    demo_thread_local_cache();

    std::cout << "\n[3.5.5] 用途三：執行緒專屬隨機數產生器：\n";
    demo_thread_local_rng();

    // --- 3.6 執行緒安全的初始化 ---
    std::cout << "\n--- 課程 3.6：執行緒安全的初始化 ---\n" << std::endl;

    std::cout << "[3.6.1] std::call_once 基本示範：\n";
    demo_call_once_basic();

    std::cout << "\n[3.6.2] call_once 與例外（失敗後重試）：\n";
    demo_call_once_with_exception();

    std::cout << "\n[3.6.3] 執行緒安全的單例模式：\n";
    demo_singleton();

    std::cout << "\n[3.6.4] 延遲初始化（Lazy Init）：\n";
    demo_lazy_init();

    std::cout << "\n"
              << "============================================================\n"
              << " 第三階段所有示範執行完成！\n"
              << "============================================================\n"
              << std::endl;

    return 0;
}

/*
 * ============================================================
 * 本階段核心觀念速查表
 * ============================================================
 *
 * 【RAII 執行緒管理】
 *   ┌─────────────────────────────────────────────────────┐
 *   │ 問題                    解決方案                     │
 *   │ std::thread 未 join →   使用 RAII 包裝類別          │
 *   │ 例外導致 join 被跳過 →   RAII 確保解構時 join        │
 *   │ 程式崩潰（terminate） →  永遠不讓 joinable 的 thread │
 *   │                          在未 join/detach 下解構     │
 *   └─────────────────────────────────────────────────────┘
 *
 * 【執行緒守衛比較】
 *   ThreadGuard     → 持有引用，簡單，不可移動
 *   ScopedThread    → 持有所有權，建構時驗證
 *   JoiningThread   → 完整版，支援移動，可入容器
 *   ManagedThread   → 模板版，編譯期決定行為
 *   std::jthread    → C++20 標準版，內建取消機制
 *
 * 【例外處理方案比較】
 *   執行緒內部捕獲     → 最簡單，但呼叫端感知不到
 *   exception_ptr     → 可跨執行緒傳遞任意例外
 *   std::future       → 推薦！自動傳遞，語法最簡潔
 *   std::promise      → 最靈活，可精確控制時間點
 *
 * 【thread_local 使用場景】
 *   - errno 風格的執行緒專屬錯誤碼
 *   - 避免鎖競爭的執行緒專屬快取
 *   - 每個執行緒獨立的隨機數產生器
 *   - 執行緒 ID、名稱等執行緒元資料
 *
 * 【執行緒安全初始化比較】
 *   call_once + once_flag → 適合複雜初始化邏輯，可重試
 *   static 區域變數       → 單例模式首選，最簡潔
 *   簡單 mutex 保護       → 效能較差，不推薦用於初始化
 *
 * ============================================================
 */
