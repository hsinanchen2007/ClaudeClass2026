/*
================================================================================
【C++_MultiThread/summary.cpp】

本目錄主題：多執行緒（thread/mutex/atomic/condition_variable/future）

多執行緒的核心難點不是 API，而是「共享狀態」：
  - data race（資料競爭）會讓程式進入未定義行為（UB）
  - 正確的同步：mutex/atomic/condition_variable
  - 避免 deadlock：固定鎖順序、用 std::lock/std::scoped_lock

本 summary 原則：
  - 不加入 題庫 類範例
  - 以 C++17 可編譯；C++20 的 jthread/latch/barrier 等只做提示

編譯：
  g++ -std=c++17 -pthread -Wall -Wextra summary.cpp -o summary && ./summary
（Windows 的 g++/clang++ 也需要對應的 thread 支援；若用 MSVC 則不需要 -pthread）
================================================================================
*/

/*
補充筆記：C++_MultiThread/C++_MultiThread summary
  - 如果兩個範例看起來都能完成同一件事，優先比較它們是否擁有資料、是否配置記憶體、是否改變輸入。
  - C++_MultiThread/C++_MultiThread summary 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
  - 這個 summary.cpp 只做章節整理，不新增題庫題解；需要實作練習時回到各主題檔。
  - C++_MultiThread/C++_MultiThread summary 的複習方式是把 API 依用途分組，再比較輸入條件、輸出語意、失敗狀態和複雜度。
  - 初學複習 summary 時，不要只背函式名稱；要能說出何時該用、何時不該用、和相近工具差在哪裡。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多執行緒總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 面試官只給你三十秒：多執行緒程式最核心的三個原則是什麼？
//     答：(1) 先確認「哪些資料被多個 thread 存取、其中有沒有寫」——沒有共享寫入就
//         沒有同步問題。(2) 有的話，替它建立 happens-before：用 mutex 保護完整的不變式，
//         或用 atomic 的 release/acquire 配對。(3) 先寫對再談快：false sharing、
//         contention、lock-free 都是正確性成立之後才處理的事。
//     追問：什麼情況下可以完全不同步？（資料不可變、或每個 thread 各自擁有獨立副本，
//           最後才合併——這也是最省事、最不會出錯的設計）
//
// 🔥 Q2. mutex、atomic、condition_variable 各自解決什麼問題？
//     答：mutex 保護「一段臨界區的不變式」，涵蓋多個變數；atomic 保護「單一變數的
//         單一操作」，適合計數器與旗標，不需系統呼叫；condition_variable 解決的則是
//         「等待某個條件成立」——它本身不保護任何資料，必須搭配 mutex 與 predicate
//         迴圈使用。三者常一起出現：mutex 保狀態、CV 等狀態改變、atomic 做旗標。
//     追問：CV 為什麼一定要搭配 mutex？（wait 需要原子地「解鎖 + 進入等待」，否則
//           「檢查條件」與「進入等待」之間的空窗會造成 lost wakeup）
//
// ⚠️ 陷阱. 「我測了一萬次都正確，所以這段並行程式沒問題」——錯在哪？
//     答：data race 是 UB，不是機率性的錯值。在 x86 這種強記憶體模型上，錯誤的
//         memory_order 或缺少的同步經常長期表現正常，直到換編譯器版本、換優化等級或
//         換到 ARM 才爆炸；而且測試只覆蓋了實際發生過的交錯順序。
//     為什麼會錯：把並行 bug 當成一般 bug——以為「可重現才是 bug、測不出來就沒事」。
//         並行正確性要靠推理 happens-before 加上 TSan 這類工具，不能靠次數堆砌。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

// -----------------------------------------------------------------------------
// 【重點 1】thread 基本：建立、join（不要忘記 join/detach）
// -----------------------------------------------------------------------------
static void demo_thread_basic() {
    std::cout << "\n[demo_thread_basic]\n";
    std::thread t([] {
        std::cout << "  hello from worker thread\n";
    });
    t.join();
}

// -----------------------------------------------------------------------------
// 【重點 2】mutex：保護共享資料（避免 data race）
// -----------------------------------------------------------------------------
static void demo_mutex_counter() {
    std::cout << "\n[demo_mutex_counter]\n";

    int counter = 0;
    std::mutex m;

    auto work = [&] {
        for (int i = 0; i < 10000; ++i) {
            std::lock_guard<std::mutex> lk(m);
            ++counter;
        }
    };

    std::thread t1(work), t2(work);
    t1.join();
    t2.join();
    std::cout << "  counter=" << counter << " (expected 20000)\n";
}

// -----------------------------------------------------------------------------
// 【重點 3】atomic：對簡單數值做無鎖同步（仍需理解 memory ordering）
// -----------------------------------------------------------------------------
static void demo_atomic_counter() {
    std::cout << "\n[demo_atomic_counter]\n";

    std::atomic<int> counter{0};
    auto work = [&] {
        for (int i = 0; i < 10000; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
        }
    };
    std::thread t1(work), t2(work);
    t1.join();
    t2.join();
    std::cout << "  atomic counter=" << counter.load() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 4】condition_variable：生產者/消費者（等待條件成立）
// -----------------------------------------------------------------------------
static void demo_condition_variable() {
    std::cout << "\n[demo_condition_variable]\n";

    std::mutex m;
    std::condition_variable cv;
    std::queue<int> q;
    bool done = false;

    std::thread producer([&] {
        for (int i = 1; i <= 5; ++i) {
            {
                std::lock_guard<std::mutex> lk(m);
                q.push(i);
            }
            cv.notify_one();
        }
        {
            std::lock_guard<std::mutex> lk(m);
            done = true;
        }
        cv.notify_one();
    });

    std::thread consumer([&] {
        while (true) {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&] { return done || !q.empty(); }); // ★ 用 predicate 避免假喚醒

            while (!q.empty()) {
                int x = q.front();
                q.pop();
                lk.unlock(); // 處理資料時可先放鎖，縮短臨界區
                std::cout << "  consume " << x << "\n";
                lk.lock();
            }
            if (done) break;
        }
    });

    producer.join();
    consumer.join();
}

// -----------------------------------------------------------------------------
// 【重點 5】future/async：把工作丟到背景，取回結果（簡化 join + 例外傳遞）
// -----------------------------------------------------------------------------
static void demo_future_async() {
    std::cout << "\n[demo_future_async]\n";

    auto fut = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 7 * 6;
    });
    std::cout << "  async result=" << fut.get() << "\n";
}

// -----------------------------------------------------------------------------
// 【重點 6】deadlock 避免：scoped_lock / std::lock（一次鎖多把）
// -----------------------------------------------------------------------------
static void demo_scoped_lock() {
    std::cout << "\n[demo_scoped_lock]\n";

    std::mutex m1, m2;
    int a = 0, b = 0;

    auto work = [&] {
        // C++17 其實也有 scoped_lock，但有些靜態分析/工具鏈可能誤判；
        // 這裡用 std::lock + adopt_lock 展示「一次鎖多把」的標準寫法。
        std::lock(m1, m2);
        std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
        std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);
        ++a;
        ++b;
    };

    std::thread t1(work), t2(work);
    t1.join();
    t2.join();
    std::cout << "  a=" << a << ", b=" << b << "\n";
}

static void demo_cpp20_note() {
    std::cout << "\n[demo_cpp20_note]\n";
#if __cplusplus >= 202002L
    std::cout << "  C++20: jthread/stop_token, latch, barrier, semaphore 等同步工具更完整。\n";
#else
    std::cout << "  (C++17：用 thread/mutex/cv/future 為主；更高階需求可用 thread pool/任務系統)\n";
#endif
}

int main() {
    demo_thread_basic();
    demo_mutex_counter();
    demo_atomic_counter();
    demo_condition_variable();
    demo_future_async();
    demo_scoped_lock();
    demo_cpp20_note();

    std::cout << "\n[done]\n";
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra summary.cpp -o summary

// === 預期輸出 (節錄) ===
//
// [demo_thread_basic]
//   hello from worker thread
//
// [demo_mutex_counter]
//   counter=20000 (expected 20000)
//
// [demo_atomic_counter]
//   atomic counter=20000
//
// [demo_condition_variable]
//   consume 1
//   consume 2
//   consume 3
//   consume 4
//   consume 5
//
// [demo_future_async]
//   async result=42
//
// …（後略，完整輸出共 27 行）
