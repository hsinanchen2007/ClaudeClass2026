// =============================================================
// 05_condition_variable.cpp  --  以條件變數實作生產者/消費者
// =============================================================
//
// 本課目標:
//   1. 理解 std::condition_variable 為何存在:
//      讓一個執行緒 *睡著*,直到另一個執行緒告訴它
//      「狀態變了,你再去看看」。
//   2. 建立經典的生產者/消費者佇列。
//   3. 學會三條規則 —— 它們合在一起讓條件變數正確 (而初學者
//      永遠都會違反某一條):
//        (a) 條件變數一定要搭配 mutex。
//        (b) 一定要用 predicate (述詞) 形式的 wait
//            (才能應付假醒 spurious wakeup)。
//        (c) 一定要在 *持鎖* 的狀態下改變共享狀態,然後再 notify。
//   4. 學會如何乾淨地關閉一個工作執行緒。
//
// 編譯方式:
//     g++ -std=c++17 -O2 -pthread 05_condition_variable.cpp \
//         -o 05_condition_variable
//
// 執行方式:
//     ./05_condition_variable
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     生產者/消費者模式,用 mutex+CV 等狀態變化
// 前置課程: lesson 03
// 觀念詞彙:
//   - condition variable ── 「睡到狀態改變才醒」的同步原語
//   - predicate          ── 用來檢查「我要的條件成立了嗎?」
//   - spurious wakeup    ── 標準允許 wait 無故醒,要再檢查 predicate
//   - lost wakeup        ── notify 在 wait 之前發生,被遺漏
// 新介紹 API:
//   std::condition_variable    必須與 std::unique_lock 搭配
//   cv.wait(lk, predicate)     原子地解鎖+睡;醒來重新鎖
//   cv.notify_one()            喚醒一個等待者
//   cv.notify_all()            喚醒所有等待者
//   std::unique_lock           可解鎖再上鎖的 RAII (lock_guard 不行)
//   std::condition_variable_any  可與任何 Lockable (含 shared_lock) 搭配
//   std::notify_all_at_thread_exit(cv, lk)
//                              C++11:在 thread 結束、thread_local 解構
//                              都跑完後,自動 unlock 並 notify_all
// 何時使用:
//   - 生產者-消費者佇列、限流、等共享狀態變化
// 何時不要用:
//   - 共享狀態就是一個 atomic 值 → std::atomic::wait (lesson 12)
//   - 一次性的「等 N 件事完成」 → std::latch (lesson 12)
// 常見錯誤:
//   - 用 cv.wait(lk) 沒 predicate → 假醒導致邏輯錯誤
//   - 沒在持鎖時 mutate 共享狀態 → race
//   - 持鎖時 notify → 喚醒方馬上又被擋住,浪費
//   - notify 前忘了把資料推進 queue → 喚醒方看到空 queue
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── condition_variable 的兩個經典坑
// =============================================================
//
// 1. 為什麼 cv 必須配 mutex?── lost wakeup 問題
//    想像一個簡化版 (沒有 mutex):
//        // consumer
//        if (queue.empty())   // step 1: 看到空
//            cv.wait();        // step 2: 進入睡眠
//        // producer
//        queue.push(x);       // 在 1 與 2 之間發生!
//        cv.notify_one();     // 此時還沒人在 wait,notify 撞空
//    consumer 永遠醒不來。問題:check 與 wait 不是原子的。
//
//    cv.wait(lk) 的精妙:它在 *釋放 mutex 與進入 sleep 之間是原子的*。
//    所以正確流程是:
//        unique_lock lk(mu);
//        cv.wait(lk, [&]{ return !queue.empty(); });   // wait 期間鎖被放掉
//        // 醒來時鎖自動重新拿好
//    notify_one() 必須在「拿著鎖改完狀態」之後才呼叫,確保 consumer
//    看到一致狀態。
//
// 2. spurious wakeup ── 為什麼一定要 predicate
//    POSIX 與 C++ 都明文允許 cv.wait *無故醒來*。原因:
//      - futex 介面在 signal 中斷後可能提前返回
//      - 為了實作簡化,廣播一次叫醒多人後並非每個都能「拿到工作」
//    所以 *不能* 寫成 `if (queue.empty()) cv.wait(lk);` ── 醒來不代表
//    條件成立,要 *while* 重檢:
//        while (queue.empty()) cv.wait(lk);
//    或直接用 predicate 版本 `cv.wait(lk, pred)`,標準幫你包好 while。
//
// 3. notify_one vs notify_all
//    - notify_one:叫醒一條 (任一條,無公平保證)。
//    - notify_all:叫醒全部正在 wait 的 thread。
//    用 notify_all 的時機:狀態改變影響「多個不同條件」(例如 producer
//    放了 5 個元素,5 個 consumer 都該醒)。但若條件唯一 (例如「queue
//    非空」),用 notify_one 即可,避免 thundering herd ── 全員衝去搶
//    一把鎖,只有一個贏,其餘白醒一場再睡回去。
//
// 4. 持鎖時 notify 還是釋鎖後 notify?
//    都正確,但有微差:
//      - 持鎖時 notify:喚醒方醒來立刻又被 mutex 擋住,要等你 unlock。
//        多 1 次 context switch 的代價。但保證「notify 那刻狀態還在那」。
//      - 釋鎖後 notify:喚醒方一醒就能拿鎖。但若 notify 與 unlock 之間
//        被打斷,理論上可能有少數 race-y 邊界 case (例如 cv 物件本身被
//        銷毀)。一般 producer/consumer 模式採「先 unlock 再 notify」更快。
//    POSIX 文件保證兩種寫法皆正確;選哪個看效能重要還是程式碼清爽。
//
// 5. cv.wait_for / wait_until 的回傳值
//    回傳 std::cv_status::no_timeout (有人 notify 了) 或 timeout。
//    *仍可能 spurious wakeup* → 還是要檢查 predicate 才知道條件是否真成立。
//    用 predicate 版本最簡:回 false 代表 timeout 且條件未滿足。
//
// 6. 與 atomic::wait/notify (C++20, lesson 12) 比較
//    atomic::wait 是 cv 的「無 mutex」版本:直接對 atomic 變數的「值」
//    做 futex 等待。優點:更便宜 (一個 atomic + 一次 syscall);缺點:
//    只能等「值是否相等」,不能等「複合條件」。複雜條件還是 cv。
//
// 7. condition_variable vs condition_variable_any
//    std::condition_variable 只接受 std::unique_lock<std::mutex> ── 有些
//    平台 (例如 Linux glibc) 可以走更快的 fast path,因為它能直接拿到
//    pthread_mutex_t 的 native handle。這是「為了效能犧牲一點通用性」。
//    若你想等的鎖不是 std::mutex (例如 std::shared_mutex 的 shared_lock、
//    std::recursive_mutex、自定義的 Lockable),只能用
//    std::condition_variable_any。它對「任何符合 BasicLockable 的鎖」
//    都能 wait,代價是內部多一層自己的 mutex+cv,微速一點。
//
//    補充:C++20 的 stop_token 整合 (jthread + interruptible wait) 只在
//    cv_any.wait(lk, stop_token, pred) 上提供 ── 想做「可被取消的 cv 等
//    候」一定要用 cv_any (見 lesson 07)。
//
// 8. notify_all_at_thread_exit ── thread 收尾的同步信號
//    std::notify_all_at_thread_exit(cv, std::move(lk)) 是 C++11 的「fire
//    and forget」工具:
//      - lk 必須是 std::unique_lock<std::mutex>,持有狀態,呼叫後所有權
//        被 *偷走* (移到內部),你自己持鎖區段在這行 *之後* 結束時鎖仍
//        被持著。
//      - 待 thread 真正退場 ── *thread_local 物件解構也都跑完後* ── 標準
//        函式庫才幫你 unlock(lk) 並 cv.notify_all()。
//      - 跟「在函式末尾自己 unlock + notify_all」相比,差別是它能保證
//        thread_local 的解構順序 *早於* 喚醒方看到「我結束了」的旗標。
//        典型用途:thread_local 大物件 (例如 connection、log buffer) 在
//        thread 結束時要被別處看到「資源已歸還」,沒這個保證 race 就會
//        在「thread_local 還在 destruct」與「main 看 done==true」間發生。
//    cppreference: https://en.cppreference.com/w/cpp/thread/notify_all_at_thread_exit
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目 (cv 是 LC concurrency 標籤最頻繁出現的工具):
//   ✓ LC 1115  Print FooBar Alternately
//     - 兩個 thread 用 turn flag + cv 交替印 ── cv 教科書範例。
//     - 完整解答 → lesson 30 Q2 (cv 版 + semaphore 版兩種)
//   ✓ LC 1116  Print Zero Even Odd
//     - 三 thread 三狀態 turn,cv.wait predicate 比對自己該不該動。
//     - 完整解答 → lesson 30 Q3 (用 binary_semaphore 解,更簡潔)
//   ✓ LC 1188  Design Bounded Blocking Queue
//     - mutex + 兩個 cv (notFull / notEmpty),經典 producer/consumer。
//     - 完整解答 → lesson 30 Q5
//   ✓ LC 1195  Fizz Buzz Multithreaded
//     - 4 thread 共用 i_ + cv,各自 wait predicate 判斷 mod 3/5。
//     - 完整解答 → lesson 30 Q6
//   ✓ LC 1242  Web Crawler Multithreaded
//     - cv.wait 在 queue 為空時等候新工作,搭配 in_flight termination。
//     - 完整解答 → lesson 30 Q8
//
// 主要 API 對照 (cppreference):
//   - std::condition_variable           https://en.cppreference.com/w/cpp/thread/condition_variable
//   - cv::wait                          https://en.cppreference.com/w/cpp/thread/condition_variable/wait
//   - cv::wait_for / wait_until         https://en.cppreference.com/w/cpp/thread/condition_variable/wait_for
//   - cv::notify_one / notify_all       https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one
//   - std::cv_status                    https://en.cppreference.com/w/cpp/thread/cv_status
//   - std::condition_variable_any       https://en.cppreference.com/w/cpp/thread/condition_variable_any
//   - std::notify_all_at_thread_exit    https://en.cppreference.com/w/cpp/thread/notify_all_at_thread_exit
//
// 練習建議:
//   讀完本課,馬上去 lesson 30 把 Q2 / Q5 / Q6 三題 cv 解法看完,你會
//   發現 90% 的 LC concurrency 題就是 cv + predicate 的組合。
// =============================================================

/*
補充筆記：std::condition_variable
  - condition_variable::wait 必須搭配 mutex，並用 predicate 重新檢查條件。
  - spurious wakeup 是標準允許的；wait(lock, pred) 能把 while 檢查寫正確。
  - notify_one/notify_all 不保存通知，若條件狀態沒有被保護好，等待者可能永遠睡著。
  - std::condition_variable 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】condition_variable：predicate、lost wakeup、生產者/消費者
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼 wait() 一定要帶 predicate？
//     答：兩個原因。(1) Spurious wakeup：標準允許 wait() 在沒有任何 notify 的情況下
//         返回。(2) 搶跑：被喚醒後要重新競爭 mutex，等真正拿到鎖時條件可能已被另一個
//         先醒的 thread 消費掉。wait(lk, pred) 等價於 while (!pred()) wait(lk); ——
//         用迴圈而非 if，把兩者都變成無害的重試。
//     追問：wait() 內部做了什麼？（原子地「解鎖 mutex + 進入等待」，返回前重新上鎖，
//           這個原子性正是必須把 lock 傳進去的原因）
//
// 🔥 Q2. 什麼是 lost wakeup？如何避免？
//     答：CV 沒有記憶——notify_one() 發出時若沒有任何 thread 在等待，這個通知就永久
//         消失。典型錯誤：consumer 檢查條件為 false、還沒進入 wait，producer 就生產
//         並 notify，consumer 才進 wait → 永遠等不到。避免方式：條件的修改與檢查都
//         必須在同一把 mutex 保護下，使「檢查→等待」相對於「修改→通知」是原子的。
//     追問：notify 該在鎖內還是鎖外？（都正確；鎖內較易推理，鎖外可避免被喚醒者立刻
//           撞鎖，但要確保狀態已改完）
//
// 🔥 Q3. notify_one() 與 notify_all() 怎麼選？
//     答：notify_one() 開銷小；notify_all() 可能造成 thundering herd（一堆 thread 醒來
//         搶同一把鎖）。規則：等待者同質、且一次事件只能滿足一個等待者（單一 queue 的
//         多個 consumer）用 notify_one()；等待者在等不同條件、或一次事件可滿足多個
//         （廣播 shutdown、各自 predicate 不同）必須用 notify_all()，否則被喚醒的可能
//         是 predicate 仍為 false 的那個，其他人餓死。
//     追問：shutdown 旗標為什麼一律用 notify_all？（要叫醒全部等待者，一個都不能漏）
//
// 🔥 Q4. 手寫有界阻塞佇列的要點？
//     答：mutex + 兩個 CV（not_full / not_empty）+ 緩衝區。Producer：wait 直到
//         size < cap 或 stop → push → 解鎖 → notify not_empty；consumer 對稱。用兩個
//         CV 而非一個，可避免 producer 喚醒 producer 的無效喚醒。stop 旗標必須納入
//         每個 predicate，否則無法優雅關閉。
//     追問：如何做 push 逾時放棄？（wait_for 帶 predicate，回傳值即 predicate 最終值）
//
// ⚠️ 陷阱. 把 while 換成 if：if (q.empty()) cv.wait(lk); 會怎樣？
//     答：if 只檢查一次，被 spurious wakeup 或搶跑喚醒後就當作條件成立往下走，對空
//         queue 呼叫 pop() → UB 或崩潰。必須寫 while (q.empty()) cv.wait(lk);，或
//         直接用帶 predicate 的多載（內部就是 while）。
//     為什麼會錯：直覺認為「被通知了就代表條件成立」，但 CV 通知的語意只是「狀態可能
//         變了，你自己再去看一次」。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>           // std::shared_mutex / shared_lock,給 cv_any 範例
#include <condition_variable>
#include <queue>
#include <chrono>
#include <vector>
#include <string>

// -------------------------------------------------------------
// 共享狀態
//
// 一個由生產者與消費者共用的整數佇列。
// 三樣東西要一起出現,且 *永遠* 在同一把 mutex 之下被觸碰:
//   - 佇列本身
//   - 「done」旗標 (生產者結束、不會再有新項目)
//   - 用來通知等待者的條件變數
// -------------------------------------------------------------
std::queue<int>         q;
std::mutex              q_mtx;
std::condition_variable q_cv;
bool                    done = false;

// -------------------------------------------------------------
// 為什麼不直接用一個旗標 + while 迴圈?
//
//   while (q.empty()) { /* 自旋 */ }
//
// 這樣是會運作,但會把一顆 CPU 核心吃滿到 100% 卻什麼都
// 沒做。在靠電池供電的裝置上,幾分鐘就會把電耗光。
//
// std::condition_variable 讓消費者執行緒去 *睡覺* —— 作業
// 系統把它從 CPU 排程下去 —— 直到生產者呼叫 notify_one()。
// 然後 OS 才把它喚醒。等待時 CPU 用量為零,喚醒延遲只有
// 微秒等級。
// -------------------------------------------------------------

void producer()
{
    for (int i = 1; i <= 5; ++i) {
        // 假裝產生這個項目是個慢工作。
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // -------------------------------------------------
        // 規則 (c): 在 *持鎖* 的狀態下改動共享狀態,
        //           *然後再* notify。順序就是這樣。
        // -------------------------------------------------
        {
            std::lock_guard<std::mutex> lk(q_mtx);
            q.push(i);
            std::cout << "[producer] pushed " << i << '\n';
        }   // <-- 在 notify 之前 *先* 釋放 mutex。
            //     如果 notify 時還持著鎖,被喚醒的消費者
            //     會立刻又因為要拿這把鎖而被阻塞。

        // notify_one() 喚醒 *一個* 在 q_cv 上等待的執行緒。
        // 如果有多個消費者可能對同一個事件感興趣,就用
        // notify_all()。
        q_cv.notify_one();
    }

    // 告訴消費者「不會再有新的項目了」。
    // 同樣的模式:在持鎖的狀態下改狀態,然後 notify。
    {
        std::lock_guard<std::mutex> lk(q_mtx);
        done = true;
        std::cout << "[producer] done, signalling shutdown\n";
    }
    q_cv.notify_all();   // 喚醒所有消費者讓它們離開
}

void consumer()
{
    while (true) {
        // ---------------------------------------------------------
        // 這裡用 std::unique_lock,*不是* std::lock_guard。
        // 為什麼?因為 cv.wait() 在睡覺時必須先 *解鎖* mutex,
        // 醒來時再 *重新上鎖*。unique_lock 支援這個操作;
        // lock_guard 不支援。每當有條件變數出現時,就用
        // 這個略為厚重一點的 RAII 包裝器。
        // ---------------------------------------------------------
        std::unique_lock<std::mutex> lk(q_mtx);

        // -----------------------------------------------------
        // 規則 (b): *永遠* 用 predicate 形式的 wait。
        //
        // 這個 lambda 會被檢查:
        //   - 進入睡眠之前 (避免在已經有工作時還傻傻地睡著
        //     —— 這預防了「lost wakeup 漏掉的喚醒」),
        //   - 以及每次被喚醒時 (包括「假醒 spurious wakeup」)。
        //
        // 「假醒」= 作業系統可以毫無理由地把執行緒叫醒。
        // 標準允許這種行為,而且實際上真的會發生。predicate
        // 讓你的程式碼免疫於假醒:只有在你真正關心的條件成立
        // 時,你才會繼續往下走。
        //
        // 這一行的等效行為是:
        //
        //   while (!(predicate)) {
        //       atomically: unlock(lk); 睡到被通知;
        //       醒來時: lock(lk);
        //   }
        // -----------------------------------------------------
        q_cv.wait(lk, []{ return !q.empty() || done; });

        // 走到這裡時我們又持有 mutex。決定要做什麼。

        // 在檢查 `done` 之前,把佇列裡剩下的東西全部處理完,
        // 以免和「shutdown」一起送來的項目被丟掉。
        while (!q.empty()) {
            int v = q.front();
            q.pop();

            // 在我們「處理」項目時把鎖釋放掉,這樣生產者
            // 可以繼續生產。臨界區應該保持很 *短* —— 這是
            // lesson 03 的規則 2。
            lk.unlock();

            std::cout << "  [consumer] got " << v << '\n';
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            lk.lock();   // 進入下一次迴圈前重新上鎖
        }

        // 佇列空了。我們是在關機嗎?
        if (done) {
            std::cout << "  [consumer] queue empty and done -> exit\n";
            return;
        }
        // 否則回到上面繼續等更多項目。
    }
}

// =============================================================
// 補充 1 ── std::condition_variable_any 配 std::shared_lock
//
// 主課的 cv 只能搭配 std::unique_lock<std::mutex>。如果你的同步
// 用的是 std::shared_mutex (lesson 10 的 reader/writer 鎖),要在
// shared_lock 上 wait 就只能用 cv_any。底下示範:讀者持 shared_lock
// 等「configuration version 大於我目前看到的」── 一個經典「讀
// 配置等熱重載」場景。
//
// 為什麼不直接用 cv:cv 內部會把 unique_lock<mutex> 的底層 mutex
// 解鎖再上鎖,只認得這一種型態。shared_lock<shared_mutex> 是另一
// 個型態 → 編譯不過。cv_any 接 *任何* 滿足 BasicLockable 的鎖。
// =============================================================
struct ConfigStore {
    std::shared_mutex            sm;
    std::condition_variable_any  cv_any;
    int                          version = 0;
    std::string                  payload = "v0";
};

static void config_reader(ConfigStore& cs, int my_seen, int reader_id)
{
    std::shared_lock<std::shared_mutex> sl(cs.sm);   // 讀者鎖
    // 用 cv_any 在 shared_lock 上 wait。標準 cv 做不到這件事。
    cs.cv_any.wait(sl, [&]{ return cs.version > my_seen; });
    std::cout << "  [reader " << reader_id << "] saw v" << cs.version
              << " payload=\"" << cs.payload << "\"\n";
}

static void config_writer(ConfigStore& cs)
{
    for (int i = 1; i <= 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        {
            // 寫者用 unique_lock<shared_mutex>。改完狀態,解鎖,再廣播。
            std::unique_lock<std::shared_mutex> ul(cs.sm);
            cs.version  = i;
            cs.payload  = "v" + std::to_string(i);
            std::cout << "[writer] published v" << i << '\n';
        }
        cs.cv_any.notify_all();   // 喚醒所有持 shared_lock 等的 reader
    }
}

// =============================================================
// 補充 2 ── std::notify_all_at_thread_exit
//
// thread 結束 *並且* 它所有 thread_local 物件都解構完之後,自動
// unlock(lk) 並 cv.notify_all()。它解的是這個 race:
//
//    worker:                     main:
//      thread_local Big big;       cv.wait(lk, [&]{ return done; });
//      ...                         // 醒來時, big 還在解構中 → use-after-free
//      done = true;
//      cv.notify_all();
//
// 把通知 *延後到 thread_local 解構完*,確保「main 醒來」與「worker
// 的 thread_local 已解構完成」之間有 happens-before 關係。
// =============================================================
struct ThreadLocalProbe {
    int id;
    explicit ThreadLocalProbe(int x) : id(x) {
        std::cout << "  [tl ctor " << id << "]\n";
    }
    ~ThreadLocalProbe() {
        std::cout << "  [tl dtor " << id << "] (this fires BEFORE main wakes)\n";
    }
};

static std::mutex              exit_mtx;
static std::condition_variable exit_cv;
static bool                    worker_done = false;

static void exit_worker()
{
    // thread_local:每條 thread 各有一份;thread 結束時自動解構。
    thread_local ThreadLocalProbe probe(42);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::unique_lock<std::mutex> lk(exit_mtx);
    worker_done = true;

    // 把鎖的所有權「捐」給 std 的 thread-exit 機制。我們的這個
    // function 結束、thread_local 解構也跑完之後,才會 unlock(lk)
    // 並 cv.notify_all()。lk 在這行之後仍持鎖,但你 *不該* 再用它
    // (已被 move 走)。
    std::notify_all_at_thread_exit(exit_cv, std::move(lk));

    // 注意:這裡 *沒有* 自己 cv.notify_all()、*沒有* 自己 unlock。
    // 函式 return 時 → thread_local probe 解構 → std 完成 unlock+
    // notify_all。順序保證:tl dtor 在 main 看到 worker_done == true
    // *之前* 跑完。
}

// =============================================================
// LeetCode 1188  Design Bounded Blocking Queue
// 難度: medium
// =============================================================
// 題意: 實作一個 thread-safe 的有界佇列, 支援:
//   - enqueue(x): 若滿了, 阻塞直到有空間
//   - dequeue():  若空了, 阻塞直到有元素 (回傳該元素)
//   - size():     回傳目前元素數
//
// 解題思路: 這就是 producer/consumer 的「物件化封裝」。
//   - 一把 mutex 保護內部 deque + 兩個 cv:
//       not_full_  : 給 enqueue 等「有空間」
//       not_empty_ : 給 dequeue 等「有元素」
//   - enqueue: while (q.size() == cap) wait(not_full_); push; notify_one(not_empty_);
//   - dequeue: while (q.empty())       wait(not_empty_); pop;  notify_one(not_full_);
//   - 兩個 cv 分開, 不會出現 producer 喚醒 producer 的浪費。
//
// 複雜度: enqueue/dequeue 平均 O(1), 受 mutex 爭用影響。
// 邊界: cap >= 1; LC 1188 不要求 close(), 不必處理 done 旗標。
// =============================================================
class BoundedBlockingQueue {
    std::mutex mtx_;
    std::condition_variable not_full_, not_empty_;
    std::queue<int> q_;
    size_t cap_;
public:
    explicit BoundedBlockingQueue(size_t capacity) : cap_(capacity) {}

    void enqueue(int x) {
        std::unique_lock<std::mutex> lk(mtx_);
        not_full_.wait(lk, [&]{ return q_.size() < cap_; });  // predicate 形式
        q_.push(x);
        lk.unlock();             // 先解鎖再 notify, 減少 hurry-up-and-wait
        not_empty_.notify_one();
    }
    int dequeue() {
        std::unique_lock<std::mutex> lk(mtx_);
        not_empty_.wait(lk, [&]{ return !q_.empty(); });
        int v = q_.front(); q_.pop();
        lk.unlock();
        not_full_.notify_one();
        return v;
    }
    size_t size() {
        std::lock_guard<std::mutex> lk(mtx_);
        return q_.size();
    }
};

void demo_bounded_queue()
{
    std::cout << "\n--- LC 1188 Bounded Blocking Queue ---\n";
    BoundedBlockingQueue bq(3);   // 容量 3
    // producer: push 6 個元素 (容量 3 → 一定要阻塞兩輪)
    std::thread prod([&]{
        for (int i = 1; i <= 6; ++i) {
            bq.enqueue(i);
            std::cout << "  [prod] enq " << i << " (size=" << bq.size() << ")\n";
        }
    });
    // consumer: 慢慢取, 把 producer 卡住一下
    std::thread cons([&]{
        for (int i = 0; i < 6; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
            int v = bq.dequeue();
            std::cout << "  [cons] deq " << v << '\n';
        }
    });
    prod.join(); cons.join();
}


int main()
{
    // ---------- 主課 ── producer / consumer ----------
    std::thread tp(producer);
    std::thread tc(consumer);
    tp.join();
    tc.join();

    // ---------- 補充 1 ── condition_variable_any ----------
    std::cout << "\n--- condition_variable_any + shared_lock ---\n";
    ConfigStore cs;
    std::vector<std::thread> readers;
    for (int i = 0; i < 3; ++i)
        readers.emplace_back(config_reader, std::ref(cs), 0, i);
    std::thread w(config_writer, std::ref(cs));
    w.join();
    for (auto& r : readers) r.join();

    // ---------- 補充 2 ── notify_all_at_thread_exit ----------
    std::cout << "\n--- notify_all_at_thread_exit ---\n";
    {
        std::thread t(exit_worker);
        t.detach();   // 故意 detach,讓 thread_local 解構由 std 收尾
        // (實務上一般不 detach,這裡僅為突顯「main 不負責 join」也有保證)

        std::unique_lock<std::mutex> lk(exit_mtx);
        exit_cv.wait(lk, []{ return worker_done; });
        std::cout << "[main] saw worker_done == true (tl dtor already ran)\n";
    }

    // ---------- LC 1188 ── Bounded Blocking Queue ----------
    demo_bounded_queue();

    std::cout << "[main] all threads finished\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：什麼是 spurious wakeup? 為什麼 wait 一定要寫 predicate 形式?
    //    A：spurious wakeup 指「沒人 notify, wait 也可能自己醒過來」
    //       (POSIX/futex 在訊號、CPU 排程時可能發生)。所以單純 wait()
    //       醒來後不能假設條件已成立, 必須 while 迴圈再檢查條件。
    //       cv.wait(lk, pred) 形式內部就是 while(!pred()) wait();, 寫法
    //       簡潔又絕對正確。永遠用這個形式。
    //
    //  Q2：notify_one() vs notify_all() 怎麼選? 用錯會怎樣?
    //    A：當「每個事件只配對一個 waiter, 而 waiter 都等同樣條件」用
    //       notify_one, 省掉 thundering herd。當「條件改變後可能多個
    //       waiter 都應醒來 (例如多個 producer/consumer 等同一個旗標
    //       關閉)」必須用 notify_all, 否則會漏喚醒造成死等。誤用
    //       notify_one 在 broadcast 場景是典型的「測試會過、生產會掛」
    //       bug。
    //
    //  Q3：notify 在持鎖時呼叫還是放鎖後? 對效能的影響?
    //    A：標準允許兩種, 邏輯都正確。但在持鎖時 notify, 被叫醒的 waiter
    //       會立刻嘗試 acquire 同一把鎖, 結果還是要等 notifier 釋放,
    //       多一次 context switch (「hurry up and wait」)。最佳實踐是
    //       「在 lock_guard scope 內改狀態, scope 外才 notify」, 把鎖
    //       釋放跟 wakeup 拆開, 減少不必要的 thread 切換。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. 沒有 mutex 的條件變數毫無用處。
//    mutex 保護共享狀態;CV 通知該狀態的 *變化*。
//    它們永遠是一對。
//
// 2. wait(lk, predicate) —— 永遠用 predicate 形式。
//    雖然有純 wait(lk) 形式,但你必須自己處理假醒,而且
//    很容易出錯。所以就別用了。
//
// 3. notify_one() vs notify_all():
//      - notify_one(): 這次變化最多滿足 *一個* 等待者
//        (例如 push 一個項目 -> 一個消費者可以前進)。
//      - notify_all(): 這次變化可能滿足 *所有* 等待者
//        (例如關機,或「設定檔已重新載入」)。
//    猶豫時,notify_all() 一定正確,只是稍貴一點點。
//
// 4. 在 *持鎖* 的狀態下改狀態,*釋放* 鎖,*然後再* notify。
//    持鎖時 notify 並非錯誤,只是比較沒效率而已。
//
// 5. 跟 CV 搭配時用 unique_lock,其他地方用 lock_guard。
//
// 6. 這個生產者/消費者模式是你以後會寫到或用到的 *每一個*
//    執行緒池、任務佇列、工作系統的骨架。把它內化吧。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 05_condition_variable.cpp -o 05_condition_variable

// === 預期輸出 (節錄) ===
// [producer] pushed 1
//   [consumer] got 1
// [producer] pushed 2
//   [consumer] got 2
// [producer] pushed 3
//   [consumer] got 3
// [producer] pushed 4
//   [consumer] got 4
// [producer] pushed 5
// [producer] done, signalling shutdown
//   [consumer] got 5
//   [consumer] queue empty and done -> exit
//
// --- condition_variable_any + shared_lock ---
// [writer] published v1
//   [reader 2] saw v1 payload="v1"
//   [reader 1] saw v1 payload="v1"
//   [reader 0] saw v1 payload="v1"
// [writer] published v2
// [writer] published v3
// …（後略，完整輸出共 40 行）
