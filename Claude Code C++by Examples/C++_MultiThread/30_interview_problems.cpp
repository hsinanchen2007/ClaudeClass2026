// =====================================================================
// 30_interview_problems.cpp
//
// 主題:經典多執行緒面試題集 (interview reference)
//
// 為什麼補這課:
//   前 29 課把 primitives (mutex / cv / atomic / semaphore / barrier /
//   shared_mutex / latch / async / thread_pool / lock-free 結構) 走過
//   一遍。但實際面試常考的是 *把 primitives 兜出指定的同步行為*。
//   這課把幾個最常被考的題目集中在一個檔案,每題附:
//
//     - 題意
//     - 可用的解法 (通常 ≥1 種)
//     - 為什麼這樣解、坑在哪
//
// 收錄題目 (LeetCode Concurrency 標籤全 9 題):
//   Q1. Print in Order                  (LC 1114) ── 3 thread 嚴格依序
//   Q2. Print FooBar Alternately        (LC 1115) ── 2 thread 交替
//   Q3. Print Zero Even Odd             (LC 1116) ── 3 thread 三角輪轉
//   Q4. Building H2O                    (LC 1117) ── 每 2H+1O 才放行
//   Q5. Bounded Blocking Queue          (LC 1188) ── 經典 P/C
//   Q6. Fizz Buzz Multithreaded         (LC 1195) ── 4 thread 依 mod 3/5 輪
//   Q7. Dining Philosophers             (LC 1226) ── 死鎖預防
//   Q8. Web Crawler Multithreaded       (LC 1242) ── thread pool + 終止偵測
//   Q9. Traffic Light Intersection      (LC 1279) ── 交叉路口互斥
//
// 解法 primitives 對照:
//   Q1 → 兩個 binary_semaphore
//   Q2 → mutex + cv (turn flag)  /  雙 binary_semaphore
//   Q3 → 三個 binary_semaphore (zero 推 odd / even,odd / even 推 zero)
//   Q4 → counting_semaphore + barrier (C++20 漂亮解)
//   Q5 → mutex + 兩個 cv (notFull / notEmpty)
//   Q6 → mutex + cv,wait predicate 用 mod 3/5 判斷
//   Q7 → scoped_lock (std::lock 一口氣鎖兩個,內含 deadlock-avoidance)
//   Q8 → thread pool + 共享 mutex 保護 visited set + queue,
//        in-flight counter 做 termination detection
//   Q9 → 一把 mutex + currentRoad 狀態變數 (面試陷阱:不需要更複雜)
//
// Build:  g++ -std=c++20 -O2 -pthread 30_interview_problems.cpp -o 30_interview_problems
// =====================================================================

// =====================================================================
// 課程資訊 (Class Info)
// =====================================================================
// 主題:     LeetCode Concurrency 標籤全 9 題集中解 ── 把前 29 課的
//           primitives 兜成題目要的同步行為
// 前置課程:
//   lesson 03 std::mutex          (Q5 / Q6 / Q7 / Q9)
//   lesson 04 std::atomic         (Q8 in-flight counter)
//   lesson 05 condition_variable  (Q2 / Q3 / Q5 / Q6)
//   lesson 08 thread pool         (Q8 web crawler)
//   lesson 12 binary / counting semaphore + barrier (Q1 / Q2 / Q3 / Q4)
//   lesson 21 scoped_lock         (Q7 dining philosophers)
// 觀念詞彙:
//   - sequencing            ── N 條 thread 必須依特定順序前進 (Q1/Q2/Q3/Q6)
//   - grouping / barrier    ── 湊齊特定組合才放行 (Q4 H2O = 2H+1O)
//   - resource sharing      ── 多 thread 搶有限資源 + 避免 deadlock (Q5/Q7/Q9)
//   - termination detection ── 知道「沒有任何待處理工作」何時結束 (Q8)
//   - turn variable         ── 抽象「下一個輪到誰」的整數或 enum
//   - lost wakeup           ── notify 在 wait 之前發出 → 訊號遺失 (cv 必配 mutex 的根本理由)
// 新介紹 API:  本課不引入新 API,而是把前面 29 課的 API 兜成解
//   std::binary_semaphore               Q1 / Q2 / Q3 配對推進
//   std::counting_semaphore             Q4 限制每類元素入場數
//   std::barrier (C++20)                Q4 湊齊一組才放行 + completion lambda
//   std::condition_variable             Q5 / Q6 等待狀態 predicate 成立
//   std::scoped_lock                    Q7 一口氣鎖兩根筷子 (deadlock-free)
//   std::atomic<int>                    Q8 in-flight counter 偵測結束
// 何時使用 (這課的價值所在):
//   - 面試前快速複習 9 題標準解法
//   - 想看「同一個問題用 cv 解 vs 用 semaphore 解」的對比
//   - 想看 primitive 怎麼組合成 producer/consumer / barrier / 終止偵測
// 何時不要用:
//   - 不要把這課的 sleep / spin 寫法搬去生產環境 (面試題追求簡短不一定追求 fairness)
//   - 不要照抄 Q7 dining philosophers 的 scoped_lock 解到「每筆鎖延遲 < 100 ns」
//     的高頻場景 (見 lesson 21 三種解法的取捨)
// 常見錯誤 (面試時最常被挑的):
//   - 用 sleep 代替 cv.wait → 不確定性同步,換 CPU 就壞
//   - cv.wait 沒包 while + predicate → spurious wakeup 漏接
//   - 兩把以上 mutex 不同順序 lock → deadlock (Q7 必考)
//   - mutex 跨 thread unlock → 只能鎖它的 thread 解 (Q1 為何用 semaphore 而非 mutex)
//   - 試圖把 shared_mutex 從 reader 升級成 writer → 標準不支援 (lesson 10)
//   - 結束條件用 sleep_for 等死 → 用 jthread + stop_token (lesson 07) 才乾淨
// =====================================================================

// =====================================================================
// 深入解析 (Deep Dive) ── 面試題的「套路骨架」
// =====================================================================
//
// 1. 多執行緒題目的三大類型
//    A. *Sequencing* (順序控制)
//       N 條 thread 必須按某種順序印/執行。
//       例:LC 1114 print in order、LC 1116 zero-even-odd、LC 1115 foobar、
//          LC 1195 fizzbuzz multithreaded。
//       套路:把「下一個輪到誰」抽象成一個 turn 變數 (int 或 enum)。
//       primitive 選擇:
//          - 兩條 thread 交替 → 一對 binary_semaphore (互推) 或 cv + bool
//          - 多條 thread 環狀 → cv + 一個 turn enum,wait predicate 比對
//
//    B. *Grouping / Barrier-style* (組隊 / 湊齊)
//       要湊齊「特定組合」才能放行。
//       例:LC 1117 H2O (2H+1O)、production line (一份原料 + 一份包裝)。
//       套路:counting_semaphore 限制每類元素數量 + barrier 湊齊放行 +
//             completion lambda 補回 semaphore slot 開放下一輪。
//
//    C. *Resource sharing* (爭搶資源)
//       N 條 thread 共用 K 個資源,要避免 deadlock / starvation。
//       例:LC 1226 dining philosophers、bank 轉帳 (lesson 21)、producer/
//          consumer (LC 1188 bounded blocking queue)。
//       套路:
//          - 多 mutex → scoped_lock 一次拿全部 (lesson 21)
//          - bounded buffer → mutex + 兩個 cv (notFull / notEmpty)
//          - reader/writer → shared_mutex 或 RCU
//
// 2. 為什麼這些題目都是「同步原語的組合題」
//    面試考官想看你是否理解 *primitive 的語意* + *組合能力*。所以:
//      - 不要重新發明同步 (例如自己用 sleep + while 等) → 一定錯
//      - 用最小的 primitive 解 → 越少 lock 越少 cv 越好,代表你懂語意
//      - 解釋 *為什麼這個 primitive*,不只是「能跑」
//
// 3. 解題框架 (對任何同步題都適用)
//    Step 1: *畫狀態機*
//      把問題狀態列出來:有哪些「同步點」?哪幾條 thread 在競爭?
//      conditions:什麼時候允許前進?
//
//    Step 2: *指派 primitive*
//      每個「等待條件」用一個 cv 或 semaphore 或 atomic.wait。
//      每個「互斥資源」用一把 mutex 或 atomic CAS。
//      這層映射就決定了 80% 的程式碼。
//
//    Step 3: *寫 wait + notify 配對*
//      找出每個 wait 對應的 notify (誰在何時 release / notify_one)。
//      若有 wait 找不到對應 notify → 可能 deadlock,設計有缺。
//
//    Step 4: *死鎖 / starvation 自我審查*
//      - 兩個鎖以不同順序拿過嗎? (deadlock)
//      - 有 thread 永遠搶不到嗎? (starvation)
//      - 條件 spurious wake 有 while 重檢嗎?
//
// 4. 常見陷阱 (面試官會挑這些)
//    A. 「為什麼用 mutex 不用 atomic?」
//       答:狀態 *複合*、操作 *跨多步*。atomic 只能保證單一變數的單步。
//
//    B. 「為什麼 cv 要配 mutex?」
//       答:check + wait 必須原子,否則 lost wakeup (lesson 05 #1)。
//
//    C. 「為什麼不用 sleep?」
//       答:sleep 是非確定性,不是同步。靠 sleep 解的程式換 CPU 就壞。
//
//    D. 「Spurious wakeup 怎麼辦?」
//       答:while + predicate,或用 cv.wait(lk, pred) 標準形式。
//
//    E. 「能不能升級鎖?」
//       答:shared → unique 標準不支援 (lesson 10 #3),要先放再拿。
//
//    F. 「會不會 deadlock?」
//       答:看你怎麼鎖兩把以上。scoped_lock 或全域排序避免 (lesson 21)。
//
// 5. 記憶口訣 (面試現場用)
//    順序  → semaphore 互推 (簡) 或 cv + turn (彈性)
//    組隊  → semaphore 限類 + barrier 放行
//    互斥  → mutex (單) / scoped_lock (多) / shared_mutex (讀重)
//    生產者消費者 → mutex + 雙 cv (notFull / notEmpty)
//    fairness 嚴格 → 自己 queue + cv;不嚴格 → semaphore 即可
//    結束通知 → atomic<bool> + cv.wait_for + token (lesson 07 jthread)
//
// 6. LeetCode concurrency 題庫對照 (本檔覆蓋全部 9 題)
//    1114 Print in Order              → Q1, 套路 A (sequencing)
//    1115 Print FooBar Alternately    → Q2, 套路 A (alternation)
//    1116 Print Zero Even Odd         → Q3, 套路 A (3-way alternation)
//    1117 Building H2O                → Q4, 套路 B (grouping)
//    1188 Bounded Blocking Queue      → Q5, 套路 C (producer/consumer)
//    1195 Fizz Buzz Multithreaded     → Q6, 套路 A (4-way modulo)
//    1226 The Dining Philosophers     → Q7, 套路 C (resource ordering)
//    1242 Web Crawler Multithreaded   → Q8, 套路 C + thread pool + termination
//    1279 Traffic Light Intersection  → Q9, 套路 C 的退化版 (一把 mutex + 狀態)
// =====================================================================

// =====================================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =====================================================================
//
// 對應 LeetCode 題目 (本檔本身就是 LC Concurrency 標籤全 9 題的解集):
//   Q1  LC 1114  Print in Order                   ─ binary_semaphore
//   Q2  LC 1115  Print FooBar Alternately         ─ mutex + cv (turn flag)
//   Q3  LC 1116  Print Zero Even Odd              ─ 三個 binary_semaphore
//   Q4  LC 1117  Building H2O                     ─ counting_semaphore + barrier
//   Q5  LC 1188  Design Bounded Blocking Queue    ─ mutex + 雙 cv (notFull/notEmpty)
//   Q6  LC 1195  Fizz Buzz Multithreaded          ─ mutex + cv (predicate mod 3/5)
//   Q7  LC 1226  The Dining Philosophers          ─ scoped_lock (一口氣兩根筷子)
//   Q8  LC 1242  Web Crawler Multithreaded        ─ thread pool + 共享 visited 集合
//                                                   + atomic in-flight counter
//   Q9  LC 1279  Traffic Light Controlled         ─ 一把 std::mutex +
//                Intersection                       currentRoad 狀態變數
//
// 主要 API 對照 (cppreference) ── 本檔用到的同步元件:
//
//   Mutex 系列:
//     std::mutex                       https://en.cppreference.com/w/cpp/thread/mutex
//     std::lock_guard                  https://en.cppreference.com/w/cpp/thread/lock_guard
//     std::unique_lock                 https://en.cppreference.com/w/cpp/thread/unique_lock
//     std::scoped_lock                 https://en.cppreference.com/w/cpp/thread/scoped_lock
//
//   Condition variable:
//     std::condition_variable          https://en.cppreference.com/w/cpp/thread/condition_variable
//     cv::wait (with predicate)        https://en.cppreference.com/w/cpp/thread/condition_variable/wait
//     cv::notify_one / notify_all      https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one
//
//   Semaphore / barrier (C++20):
//     std::binary_semaphore            https://en.cppreference.com/w/cpp/thread/counting_semaphore
//     std::counting_semaphore          https://en.cppreference.com/w/cpp/thread/counting_semaphore
//     std::barrier                     https://en.cppreference.com/w/cpp/thread/barrier
//
//   Atomic (Q8 in-flight counter):
//     std::atomic<T>                   https://en.cppreference.com/w/cpp/atomic/atomic
//     std::memory_order                https://en.cppreference.com/w/cpp/atomic/memory_order
//
//   Thread:
//     std::thread                      https://en.cppreference.com/w/cpp/thread/thread
//     std::jthread (lesson 07 補充)    https://en.cppreference.com/w/cpp/thread/jthread
//
// 練習建議:
//   - 先依序看 lesson 03 / 05 / 12 / 21 把 primitive 的語意吃熟,再回來看
//     本檔解法 ── 你會發現每題的 wait / notify 配對都直接對應 primitive 的
//     基本用法,沒有任何「魔法」。
//   - 把 Q5 BoundedBlockingQueue 的雙 cv 改寫成 *單* cv + notify_all 版本,
//     觀察 wakeup 過剩問題。理解後再改寫回雙 cv,體會雙 cv 的意義。
//   - 把 Q1 / Q2 從 semaphore 換成 cv + bool flag 的版本,比較程式碼長度,
//     理解 semaphore 在「無持有者語意」時的簡潔性。
// =====================================================================

/*
補充筆記：interview_problems
  - interview_problems 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - interview_problems 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】LeetCode 併發題的通用解法套路
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 這類「指定執行順序」的題目有哪些通用工具？
//     答：三套。(1) binary_semaphore 鏈：最直觀，「A 做完就 release 給 B」，適合嚴格
//         輪轉（Print in Order、FooBar、Zero-Even-Odd）。(2) mutex + CV + 狀態變數
//         （turn / counter）：最通用，predicate 直接表達「輪到我了嗎」，任何複雜條件
//         都寫得出來。(3) atomic + wait/notify（C++20）：最輕量，適合條件單純的情況。
//     追問：為什麼不用 atomic 旗標 busy-wait？（可行但持續燒 CPU；面試要答得出阻塞式
//           解法，這才是真實系統會用的）
//
// 🔥 Q2. 哲學家用餐怎麼解？
//     答：本質是死鎖題，破壞四個必要條件之一即可：(1) 破壞環路等待——資源排序，例如
//         規定所有人先拿編號小的叉子，或讓其中一位反向取；(2) 破壞佔有並等待——用
//         scoped_lock 一次取得兩支叉子，取不到就全部放開；(3) 限制同時進餐人數為 N-1
//         （counting_semaphore），讓至少一人一定能湊齊兩支。
//     追問：這三種解法的差別？（資源排序最省、無額外原語；semaphore 最好懂但多一層
//           限流；scoped_lock 最短但需要能同時拿到兩把鎖的介面）
//
// 🔥 Q3. 這類題目最常見的扣分點是什麼？
//     答：三個。(1) 用 if 而不是 while／不帶 predicate 呼叫 wait，忽略 spurious wakeup。
//         (2) 只考慮「跑一輪」，沒處理「重複呼叫多輪」時狀態要能循環回去。(3) 沒有把
//         終止條件納入設計，導致程式最後卡在等待、無法結束。面試官通常會在你寫完後
//         直接追問這三點。
//     追問：如何自我驗證？（把 sleep 插在不同位置強制改變交錯順序，或用 TSan 跑；
//           正確的同步邏輯不該因為插入延遲就改變輸出）
// ═══════════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <atomic>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <semaphore>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;

// =====================================================================
// Q1. Print in Order  (LeetCode 1114)
//
// 題意:
//   有三個方法 first() / second() / third(),三個 thread 分別會去呼叫
//   其中一個。你不能控制誰先被呼叫,也不能控制何時。請設計同步,讓
//   印出的順序「保證」是 first → second → third。
//
// 套路:
//   兩個 binary_semaphore (= 一次性 gate)。
//     - sem_12:first 印完才 release,second 必須先 acquire 才能印。
//     - sem_23:second 印完才 release,third 必須先 acquire 才能印。
//
//   也可以用 mutex + cv + 兩個 bool flag。但 binary_semaphore 在這個
//   題目最簡潔 —— 概念上就是「一個一次性的紅綠燈」。
//
// 重要陷阱:
//   不能用「兩把 mutex 由 first lock,second/third 試 lock」。因為 mutex
//   *只能由 lock 它的執行緒 unlock*,first 執行緒結束時其他人解不開。
//   semaphore 沒有「持有者」概念,任何 thread 都可以 release,所以剛好
//   合用。
// =====================================================================
class Foo_PrintInOrder {
public:
    Foo_PrintInOrder() : sem_12_(0), sem_23_(0) {}

    void first(std::function<void()> printFirst) {
        printFirst();
        sem_12_.release();   // 解開 second 的閘
    }
    void second(std::function<void()> printSecond) {
        sem_12_.acquire();   // 等 first 印完
        printSecond();
        sem_23_.release();   // 解開 third 的閘
    }
    void third(std::function<void()> printThird) {
        sem_23_.acquire();   // 等 second 印完
        printThird();
    }
private:
    std::binary_semaphore sem_12_;
    std::binary_semaphore sem_23_;
};

void demo_Q1_print_in_order() {
    std::cout << "\n--- Q1. Print in Order (LC 1114) ---\n";
    Foo_PrintInOrder foo;

    // 故意把 thread 啟動順序倒過來,證明真正的順序由同步決定,
    // 不是由「誰先被 std::thread() 呼叫」決定。
    std::thread t3([&]{ foo.third (  []{ std::cout << "third\n";  } ); });
    std::thread t1([&]{ foo.first (  []{ std::cout << "first\n";  } ); });
    std::thread t2([&]{ foo.second(  []{ std::cout << "second\n"; } ); });
    t1.join(); t2.join(); t3.join();
}

// =====================================================================
// Q2. Print FooBar Alternately  (LeetCode 1115)
//
// 題意:
//   一個 thread 重複呼叫 foo() n 次,另一個 thread 重複呼叫 bar() n 次。
//   無論交給作業系統如何排,印出的結果必須是 "foobarfoobarfoobar..."。
//
// 套路:
//   一個 turn flag (true = 該 foo,false = 該 bar)。每邊用 cv.wait
//   等到輪到自己,印完後翻轉 flag 並 notify_one。
//
//   也可以用兩個 binary_semaphore 互推:foo 印完 release sem_b,
//   bar 印完 release sem_a。常見且更短。下方兩種都示範。
//
// 重要陷阱:
//   - 別用 sleep 達成「交替」 —— 那不是同步,只是運氣好。
//   - cv 一定要搭 mutex,wait 時釋放鎖、醒來重新拿。
// =====================================================================
class FooBar_CV {
public:
    explicit FooBar_CV(int n) : n_(n) {}

    void foo(std::function<void()> printFoo) {
        for (int i = 0; i < n_; ++i) {
            std::unique_lock lk(mu_);
            cv_.wait(lk, [&]{ return foo_turn_; });
            printFoo();
            foo_turn_ = false;
            cv_.notify_one();
        }
    }
    void bar(std::function<void()> printBar) {
        for (int i = 0; i < n_; ++i) {
            std::unique_lock lk(mu_);
            cv_.wait(lk, [&]{ return !foo_turn_; });
            printBar();
            foo_turn_ = true;
            cv_.notify_one();
        }
    }
private:
    int n_;
    std::mutex mu_;
    std::condition_variable cv_;
    bool foo_turn_ = true;     // 一開始輪 foo
};

class FooBar_Sem {
public:
    explicit FooBar_Sem(int n) : n_(n), sem_foo_(1), sem_bar_(0) {}

    void foo(std::function<void()> printFoo) {
        for (int i = 0; i < n_; ++i) {
            sem_foo_.acquire();
            printFoo();
            sem_bar_.release();
        }
    }
    void bar(std::function<void()> printBar) {
        for (int i = 0; i < n_; ++i) {
            sem_bar_.acquire();
            printBar();
            sem_foo_.release();
        }
    }
private:
    int n_;
    std::binary_semaphore sem_foo_;   // 初值 1 → foo 先跑
    std::binary_semaphore sem_bar_;   // 初值 0 → bar 等
};

void demo_Q2_foobar() {
    std::cout << "\n--- Q2. FooBar Alternately (LC 1115) ---\n";
    constexpr int N = 5;

    {
        FooBar_CV fb(N);
        std::cout << "[cv version]   ";
        std::thread tA([&]{ fb.foo([]{ std::cout << "foo"; }); });
        std::thread tB([&]{ fb.bar([]{ std::cout << "bar"; }); });
        tA.join(); tB.join();
        std::cout << "\n";
    }
    {
        FooBar_Sem fb(N);
        std::cout << "[sem version]  ";
        std::thread tA([&]{ fb.foo([]{ std::cout << "foo"; }); });
        std::thread tB([&]{ fb.bar([]{ std::cout << "bar"; }); });
        tA.join(); tB.join();
        std::cout << "\n";
    }
}

// =====================================================================
// Q3. Print Zero Even Odd  (LeetCode 1116)
//
// 題意:
//   三個 thread 共用一個 ZeroEvenOdd 物件,各自無限重複呼叫:
//     thread A → zero(printNumber)
//     thread B → even(printNumber)
//     thread C → odd(printNumber)
//   要保證輸出是「0 1 0 2 0 3 0 4 ... 0 n」 (n 之內合法 1-9)。
//   也就是 zero 在每個數字前各印一次,odd 印 1,3,5...,even 印 2,4,6...。
//
// 套路:
//   三個 binary_semaphore 形成「zero → odd → zero → even → zero ...」的環。
//     - sem_z 起始 1 (zero 先跑),sem_o 起始 0,sem_e 起始 0
//     - zero 印完 0 後,根據「下一個要印的數字」決定 release sem_o 或 sem_e
//     - odd / even 印完後 release sem_z 給 zero
//   這比用 cv 寫三狀態的 turn 變數更簡潔,因為每條 thread 只關心
//   「我自己的閘何時開」。
//
// 重要陷阱:
//   - 若用單一 cv + turn enum,wait predicate 比較囉嗦。semaphore 解
//     乾淨許多。
//   - 別用 atomic + sleep 假同步 ── 沒用對 primitive 一定會 race。
// =====================================================================
class ZeroEvenOdd {
public:
    explicit ZeroEvenOdd(int n)
        : n_(n), sem_z_(1), sem_o_(0), sem_e_(0) {}

    void zero(std::function<void(int)> printNumber) {
        for (int i = 1; i <= n_; ++i) {
            sem_z_.acquire();
            printNumber(0);
            // 下一個要印的數字是 i ── 奇數推 odd 閘,偶數推 even 閘
            if (i % 2 == 1) sem_o_.release();
            else            sem_e_.release();
        }
    }
    void odd(std::function<void(int)> printNumber) {
        for (int i = 1; i <= n_; i += 2) {
            sem_o_.acquire();
            printNumber(i);
            sem_z_.release();
        }
    }
    void even(std::function<void(int)> printNumber) {
        for (int i = 2; i <= n_; i += 2) {
            sem_e_.acquire();
            printNumber(i);
            sem_z_.release();
        }
    }
private:
    int n_;
    std::binary_semaphore sem_z_;
    std::binary_semaphore sem_o_;
    std::binary_semaphore sem_e_;
};

void demo_Q3_zero_even_odd() {
    std::cout << "\n--- Q3. Print Zero Even Odd (LC 1116) ---\n";
    constexpr int N = 7;
    ZeroEvenOdd zeo(N);

    std::mutex io_mu;
    auto print = [&](int x){
        std::lock_guard lk(io_mu);
        std::cout << x;
    };

    std::thread tz([&]{ zeo.zero(print); });
    std::thread to([&]{ zeo.odd (print); });
    std::thread te([&]{ zeo.even(print); });
    tz.join(); to.join(); te.join();
    std::cout << "  (expected 0102030405060708... up to 0" << N << ")\n";
}

// =====================================================================
// Q4. Building H2O  (LeetCode 1117)
//
// 題意:
//   有任意數量的 thread 各自代表一個 H 或 O 原子,呼叫 hydrogen() 或
//   oxygen()。每三個原子 (兩個 H + 一個 O) 必須一起被釋放,才合成一個
//   水分子。多個 H/O 不能跨分子混在一起 (例如不能讓四個 H 連續通過再
//   來兩個 O,必須是 HHO 或 HOH 或 OHH 為單位地組成)。
//
// 套路 (C++20):
//   - counting_semaphore<2> 限制同一分子內最多 2 個 H 通過
//   - counting_semaphore<1> 限制同一分子內最多 1 個 O 通過
//   - std::barrier(3) 把 2H+1O 卡在閘口、湊齊三個才一起放行;
//     barrier 的 completion lambda 在最後一個 arrive 的 thread 上
//     執行,正好用來「一次釋放 2 個 H slot + 1 個 O slot」,讓
//     下一組分子可以進來。
//
// 為什麼這樣解最乾淨:
//   barrier 的「湊齊 N 個才放行」語意,正好契合「2H+1O 一起組分子」。
//   三條線在 barrier.arrive_and_wait() 等到第三個到位才一起繼續,
//   而 completion lambda 確保 *只有湊齊一次* 後才補 slot,維持
//   分子間互斥。
// =====================================================================
class H2O {
public:
    H2O()
      : sem_h_(2), sem_o_(1),
        bar_(3, [this]{
            // 一個分子組好後,在 barrier completion 內補回 slots,
            // 讓下一輪可以進來。completion 只會被執行一次/輪。
            sem_h_.release(2);
            sem_o_.release(1);
        })
    {}

    void hydrogen(std::function<void()> releaseH) {
        sem_h_.acquire();         // 同分子內最多 2 個 H
        releaseH();
        bar_.arrive_and_wait();   // 等齊 3 個 (2H+1O)
    }
    void oxygen(std::function<void()> releaseO) {
        sem_o_.acquire();         // 同分子內最多 1 個 O
        releaseO();
        bar_.arrive_and_wait();
    }
private:
    std::counting_semaphore<2> sem_h_;
    std::counting_semaphore<1> sem_o_;
    std::barrier<std::function<void()>> bar_;
};

void demo_Q4_h2o() {
    std::cout << "\n--- Q4. Building H2O (LC 1117) ---\n";
    H2O plant;
    constexpr int MOLECULES = 4;

    std::mutex io_mu;
    auto print = [&](char c){
        std::lock_guard lk(io_mu);
        std::cout << c;
    };

    std::vector<std::thread> threads;
    threads.reserve(3 * MOLECULES);

    // 故意打散 H / O 啟動順序,看同步是否真能維持「每 3 個必為 2H1O」。
    for (int i = 0; i < MOLECULES; ++i) {
        threads.emplace_back([&]{ plant.oxygen   ([&]{ print('O'); }); });
        threads.emplace_back([&]{ plant.hydrogen ([&]{ print('H'); }); });
        threads.emplace_back([&]{ plant.hydrogen ([&]{ print('H'); }); });
    }
    for (auto& t : threads) t.join();
    std::cout << "  (each consecutive 3 chars must be 2H+1O)\n";
}

// =====================================================================
// Q5. Bounded Blocking Queue  (LeetCode 1188)
//
// 題意:
//   實作一個 fixed-capacity blocking queue,具備:
//     - enqueue(x):滿 → 阻塞直到有空位
//     - dequeue() :空 → 阻塞直到有元素
//     - size():    現在元素個數 (執行緒安全)
//
// 套路:
//   一把 mutex + 兩個 cv。「滿 / 空」是兩個獨立條件,各自用一個 cv,
//   notify 時可以只叫起該叫的人,避免 thundering herd。
//
//   為什麼 *不要* 共用一個 cv?共用也可以,但 producer 與 consumer 任
//   一邊呼叫 notify_one() 時,可能叫到「等錯條件」的人;那人重新 wait,
//   多浪費一輪。兩 cv 就沒這問題。
//
// 注意:
//   - 使用 cv.wait(lk, pred) (有 predicate 版本) 來防 spurious wake。
//   - notify_one 在 unlock *之後* 或之前都行,但我選擇先 unlock 再
//     notify,可以省掉「醒來立刻又被 mutex 卡住」的小成本 (lost
//     wakeup-style 效益)。
// =====================================================================
template <class T>
class BoundedBlockingQueue {
public:
    explicit BoundedBlockingQueue(std::size_t cap) : cap_(cap) {}

    void enqueue(T v) {
        std::unique_lock lk(mu_);
        not_full_.wait(lk, [&]{ return q_.size() < cap_; });
        q_.push_back(std::move(v));
        lk.unlock();
        not_empty_.notify_one();
    }
    T dequeue() {
        std::unique_lock lk(mu_);
        not_empty_.wait(lk, [&]{ return !q_.empty(); });
        T v = std::move(q_.front());
        q_.pop_front();
        lk.unlock();
        not_full_.notify_one();
        return v;
    }
    std::size_t size() {
        std::lock_guard lk(mu_);
        return q_.size();
    }
private:
    std::size_t cap_;
    std::deque<T> q_;
    std::mutex mu_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};

void demo_Q5_bounded_queue() {
    std::cout << "\n--- Q5. Bounded Blocking Queue (LC 1188) ---\n";
    BoundedBlockingQueue<int> q(/*cap=*/3);

    constexpr int N = 8;
    std::atomic<int> sum{0};

    std::thread prod([&]{
        for (int i = 1; i <= N; ++i) {
            q.enqueue(i);
            // 慢寫快讀 → consumer 經常會卡在 dequeue 上等 producer
        }
    });
    std::thread cons([&]{
        for (int i = 0; i < N; ++i) {
            sum += q.dequeue();
        }
    });
    prod.join(); cons.join();

    int expected = N * (N + 1) / 2;
    std::cout << "consumed sum = " << sum.load()
              << " (expected " << expected << ")\n";
}

// =====================================================================
// Q6. Fizz Buzz Multithreaded  (LeetCode 1195)
//
// 題意:
//   四個 thread 共用一個 FizzBuzz 物件,各自呼叫:
//     fizz()       對 i 是 3 的倍數 (但非 15) 印 "fizz"
//     buzz()       對 i 是 5 的倍數 (但非 15) 印 "buzz"
//     fizzbuzz()   對 i 是 15 的倍數 印 "fizzbuzz"
//     number(int)  對其他 i 印該數字
//   i 從 1 跑到 n,要保證輸出與「單一 thread 跑 1..n 印 fizzbuzz」相同。
//
// 套路:
//   一個共用 i_ (current 數),一把 mutex,一個 cv。
//   每條 thread 在 cv.wait 內檢查 *自己是否該動手*:
//     fizz:    i_ <= n && i_ % 3 == 0 && i_ % 5 != 0
//     buzz:    i_ <= n && i_ % 5 == 0 && i_ % 3 != 0
//     fizzbuzz: i_ <= n && i_ % 15 == 0
//     number:  i_ <= n && i_ % 3 != 0 && i_ % 5 != 0
//   印完 i_++,然後 notify_all。所有人重檢,只有一條會通過 predicate。
//   結束條件 (i_ > n) 要讓所有 thread 都能醒來退出,因此 wait predicate
//   要寫成「該我了 OR 已結束」。
//
// 重要陷阱:
//   - 結束時若只 notify 該動手的人,其他三條會永遠卡 wait → 用 notify_all。
//   - notify_all 雖然有 thundering herd,但每條 thread 只會搶 1 次 (印 + i_++),
//     成本極低。對 LC 規模沒問題;真高頻情境可改 turn-encoded 方案。
// =====================================================================
class FizzBuzzMT {
public:
    explicit FizzBuzzMT(int n) : n_(n) {}

    void fizz(std::function<void()> printFizz) {
        loop_(printFizz, [](int i){ return i % 3 == 0 && i % 5 != 0; });
    }
    void buzz(std::function<void()> printBuzz) {
        loop_(printBuzz, [](int i){ return i % 5 == 0 && i % 3 != 0; });
    }
    void fizzbuzz(std::function<void()> printFizzBuzz) {
        loop_(printFizzBuzz, [](int i){ return i % 15 == 0; });
    }
    void number(std::function<void(int)> printNumber) {
        // number 的回呼簽名不同 (吃 int),走自己的迴圈
        while (true) {
            std::unique_lock lk(mu_);
            cv_.wait(lk, [&]{
                return i_ > n_ || (i_ % 3 != 0 && i_ % 5 != 0);
            });
            if (i_ > n_) return;
            printNumber(i_);
            ++i_;
            cv_.notify_all();
        }
    }
private:
    template <class Print, class Pred>
    void loop_(Print printOne, Pred shouldFire) {
        while (true) {
            std::unique_lock lk(mu_);
            cv_.wait(lk, [&]{ return i_ > n_ || shouldFire(i_); });
            if (i_ > n_) return;
            printOne();
            ++i_;
            cv_.notify_all();
        }
    }

    const int n_;
    int i_ = 1;
    std::mutex mu_;
    std::condition_variable cv_;
};

void demo_Q6_fizzbuzz() {
    std::cout << "\n--- Q6. Fizz Buzz Multithreaded (LC 1195) ---\n";
    constexpr int N = 15;
    FizzBuzzMT fb(N);

    std::mutex io_mu;
    auto sep = [&](const char* s){
        std::lock_guard lk(io_mu);
        std::cout << s << ' ';
    };
    auto sep_n = [&](int x){
        std::lock_guard lk(io_mu);
        std::cout << x << ' ';
    };

    std::thread tF ([&]{ fb.fizz    ([&]{ sep("fizz");      }); });
    std::thread tB ([&]{ fb.buzz    ([&]{ sep("buzz");      }); });
    std::thread tFB([&]{ fb.fizzbuzz([&]{ sep("fizzbuzz");  }); });
    std::thread tN ([&]{ fb.number  (sep_n); });
    tF.join(); tB.join(); tFB.join(); tN.join();
    std::cout << "\n  (expected: 1 2 fizz 4 buzz fizz 7 8 fizz buzz 11 fizz 13 14 fizzbuzz)\n";
}

// =====================================================================
// Q7. Dining Philosophers  (LeetCode 1226)
//
// 題意:
//   5 位哲學家圍坐圓桌,每兩人共用一根筷子 (共 5 根)。哲學家要思考 →
//   想吃飯時必須同時拿到「左右兩根筷子」才能吃,吃完放回。請設計同步
//   讓:
//     1. 不會 deadlock
//     2. 不會餓死 (long-term fairness 不要差太多)
//
// 套路 (面試最常被接受的兩個):
//
//   方案 A:全域排序 (resource hierarchy)
//     強制每個 philosopher 先拿「編號小」的那根。
//     證明無 deadlock:circular wait 的「環」一定有人要從大到小拿,
//     違反規則 → 不可能成環。
//
//   方案 B:std::scoped_lock (C++17)
//     std::lock(a, b) 內部用 deadlock-avoidance 演算法 (try-and-back-
//     off) 一口氣鎖兩個,失敗就放掉重來。scoped_lock 把這個包成 RAII。
//     不需要思考排序,std 幫你解。最現代的寫法。
//
//   方案 C:奇偶不對稱
//     偶數編號先左後右,奇數編號先右後左。也能破環。較少寫,知道概念
//     就好。
//
// 下方示範方案 B (scoped_lock),最簡潔且不需改鎖的順序語意。
// =====================================================================
class DiningPhilosophers {
public:
    DiningPhilosophers() : forks_(5) {}

    // 介面遵循 LC 1226:
    //   philosopher 編號 0..4,各自的左右筷子編號為 i 與 (i+1)%5。
    void wantsToEat(int philosopher,
                    std::function<void()> pickLeftFork,
                    std::function<void()> pickRightFork,
                    std::function<void()> eat,
                    std::function<void()> putLeftFork,
                    std::function<void()> putRightFork)
    {
        int L = philosopher;
        int R = (philosopher + 1) % 5;

        // scoped_lock = std::lock(...) + 兩個 unique_lock 的 RAII 包裝。
        // 內部會嘗試 lock 兩把,任一把失敗就 release 已拿到的、回頭重試,
        // 確保不會形成 hold-and-wait → 不會 deadlock。
        std::scoped_lock both(forks_[L], forks_[R]);

        // 為了符合題目介面,先後呼叫 pick*。實際上鎖在 scoped_lock 建構
        // 時就已經拿到了 —— 這幾個 callback 只是「印出動作」用。
        pickLeftFork();
        pickRightFork();
        eat();
        putRightFork();
        putLeftFork();
        // scoped_lock 出作用域 → 兩把 mutex 一起釋放
    }

private:
    std::vector<std::mutex> forks_;
};

void demo_Q7_dining_philosophers() {
    std::cout << "\n--- Q7. Dining Philosophers (LC 1226) ---\n";
    DiningPhilosophers table;

    constexpr int ROUNDS = 200;
    std::array<std::atomic<int>, 5> meals{};
    std::mutex io_mu;
    bool verbose = false;   // 想看每口飯的軌跡可以打開

    auto philosopher = [&](int id){
        std::mt19937 rng(id);
        std::uniform_int_distribution<int> think(50, 200);
        for (int r = 0; r < ROUNDS; ++r) {
            std::this_thread::sleep_for(std::chrono::microseconds(think(rng)));
            table.wantsToEat(
                id,
                [&]{ if (verbose) { std::lock_guard lk(io_mu); std::cout << id << "L "; } },
                [&]{ if (verbose) { std::lock_guard lk(io_mu); std::cout << id << "R "; } },
                [&]{ ++meals[id]; },
                [&]{},
                [&]{}
            );
        }
    };

    std::vector<std::thread> ph;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 5; ++i) ph.emplace_back(philosopher, i);
    for (auto& t : ph) t.join();
    auto dt = std::chrono::steady_clock::now() - t0;

    std::cout << "elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count()
              << " ms,  meals per philosopher: ";
    for (int i = 0; i < 5; ++i) std::cout << meals[i].load() << ' ';
    std::cout << "(no deadlock if this prints at all)\n";
}

// =====================================================================
// Q8. Web Crawler Multithreaded  (LeetCode 1242)
//
// 題意:
//   給一個 startUrl 與一個 HtmlParser (有 getUrls(url) 方法回傳該頁所有
//   超連結)。請用多執行緒爬出所有與 startUrl *同 hostname* 的可達 URL。
//   不可重複爬同一個 URL,結果順序不限。getUrls 模擬網路延遲 (慢)。
//
// 套路:
//   經典「平行 BFS + termination detection」。
//     - 一把 mutex 保護:visited set、待爬 queue、in_flight 計數
//     - 一個 cv 通知 worker 「queue 有東西了」或「全部結束了」
//     - in_flight 表示「目前有多少 worker 正在 getUrls (不在 queue 裡的)」
//     - 結束條件:queue 空 *且* in_flight == 0
//
// 為什麼結束偵測這麼容易錯:
//   常見 bug:「queue 空就退出」。錯。可能 worker A 剛 pop 了一個 URL 正在
//   getUrls,還沒回來推新 URL;worker B 看到 queue 空就退,結果把 A 的
//   後續工作全棄。所以必須同時看 in_flight。
//
//   增/減 in_flight 必須在 *同一把 mutex* 裡與 queue.size 一併判斷,
//   否則會有 race window:
//      thread A: queue.pop → 釋放鎖 → ... → ++in_flight (來不及)
//      thread B: 拿鎖 → 看到 queue 空 + in_flight==0 → 誤判結束
//   修法:pop 與 ++in_flight 在同一個 critical section 內完成。
//
// 重要陷阱:
//   - hostname 比對要同樣 case-insensitive、忽略 port (LC 題目簡化版本不需)。
//   - getUrls 可能回傳同一個 URL 多次 → visited.insert 的回傳值是「是否
//     新插入」,只在新插入時推 queue。
//   - 若爬出的圖有環 → visited 會自然剪掉,不會死迴圈。
// =====================================================================

class HtmlParser {
public:
    virtual std::vector<std::string> getUrls(const std::string& url) = 0;
    virtual ~HtmlParser() = default;
};

// 教學用的 mock,模擬一個小型網站圖。
class MockHtmlParser : public HtmlParser {
public:
    explicit MockHtmlParser(
        std::unordered_map<std::string, std::vector<std::string>> g)
        : graph_(std::move(g)) {}

    std::vector<std::string> getUrls(const std::string& url) override {
        // 模擬 ~20 ms 網路延遲,讓多 thread 真的有平行效益
        std::this_thread::sleep_for(20ms);
        auto it = graph_.find(url);
        return it == graph_.end() ? std::vector<std::string>{} : it->second;
    }
private:
    std::unordered_map<std::string, std::vector<std::string>> graph_;
};

class WebCrawlerMT {
public:
    std::vector<std::string> crawl(const std::string& startUrl,
                                   HtmlParser& parser,
                                   int num_workers = 4)
    {
        const std::string host = hostname_(startUrl);

        std::mutex mu;
        std::condition_variable cv;
        std::unordered_set<std::string> visited{startUrl};
        std::deque<std::string> queue{startUrl};
        int in_flight = 0;
        bool done = false;

        auto worker = [&]{
            while (true) {
                std::string url;
                {
                    std::unique_lock lk(mu);
                    cv.wait(lk, [&]{ return !queue.empty() || done; });
                    if (queue.empty()) return;        // done == true 才會走到
                    url = std::move(queue.front());
                    queue.pop_front();
                    ++in_flight;                      // 與 pop 同 critical section
                }

                // 真正的「慢工作」放在鎖外,讓其他 worker 並行
                auto urls = parser.getUrls(url);

                {
                    std::lock_guard lk(mu);
                    for (auto& u : urls) {
                        if (hostname_(u) != host) continue;
                        if (visited.insert(u).second) queue.push_back(u);
                    }
                    --in_flight;
                    if (queue.empty() && in_flight == 0) {
                        done = true;
                        cv.notify_all();              // 所有 worker 醒來退出
                    } else if (!urls.empty()) {
                        cv.notify_all();              // 有新 URL 給其他 worker
                    }
                }
            }
        };

        std::vector<std::thread> ws;
        ws.reserve(num_workers);
        for (int i = 0; i < num_workers; ++i) ws.emplace_back(worker);
        for (auto& t : ws) t.join();

        return std::vector<std::string>(visited.begin(), visited.end());
    }

private:
    static std::string hostname_(const std::string& url) {
        // 假設形如 "http://x.com/path";LC 題目保證以 http:// 開頭
        constexpr std::string_view scheme = "http://";
        if (url.compare(0, scheme.size(), scheme) != 0) return url;
        auto start = scheme.size();
        auto end   = url.find('/', start);
        return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
    }
};

void demo_Q8_web_crawler() {
    std::cout << "\n--- Q8. Web Crawler Multithreaded (LC 1242) ---\n";

    // 模擬一個網站:news.com 內部互連 + 外鏈到 other.com (應被過濾掉)
    MockHtmlParser parser({
        {"http://news.com/",          {"http://news.com/a", "http://news.com/b", "http://other.com/x"}},
        {"http://news.com/a",         {"http://news.com/b", "http://news.com/c"}},
        {"http://news.com/b",         {"http://news.com/a", "http://news.com/d"}},
        {"http://news.com/c",         {"http://news.com/d"}},
        {"http://news.com/d",         {"http://news.com/"}},                    // 環,visited 剪掉
        {"http://other.com/x",        {"http://other.com/y"}},                  // 不會走到
    });

    WebCrawlerMT crawler;
    auto t0 = std::chrono::steady_clock::now();
    auto result = crawler.crawl("http://news.com/", parser, /*num_workers=*/4);
    auto dt = std::chrono::steady_clock::now() - t0;
    std::sort(result.begin(), result.end());

    std::cout << "elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count()
              << " ms,  visited " << result.size() << " URLs:\n";
    for (auto& u : result) std::cout << "  " << u << '\n';
    std::cout << "  (expected: 5 news.com URLs, no other.com)\n";
}

// =====================================================================
// Q9. Traffic Light Controlled Intersection  (LeetCode 1279)
//
// 題意:
//   兩條路 (road 1 / road 2) 在十字路口交錯,各自方向會有車到 carArrived。
//   全域只有一盞紅綠燈,初始 road 1 綠。當車到達:
//     - 若燈已是該車所在道路 → 直接呼叫 crossCar()
//     - 若是另一條路 → 先 turnGreen() 把燈翻過來,再 crossCar()
//   保證任意時刻 *最多一條路上的車在通過* (燈不會同綠)。
//
// 套路:
//   *陷阱題*。看似要 RW lock 之類複雜結構,實際只需:
//     - 一把 mutex 保護 currentRoad_ 與 turnGreen/crossCar 的互斥執行
//   一條 critical section 從進來到 cross 結束都持鎖。同時最多一車通過。
//
//   為什麼這麼樸素就對?因為題目沒要求並行通過 (即使同一條路也是序列化)。
//   面試時若直覺想用 shared_mutex 或多 mutex,代表 *過度設計*。
//
// 重要陷阱:
//   - turnGreen 只在「真的要切換」時呼叫,別每次都呼叫 (LC judge 會驗
//     turnGreen 與 currentRoad 的一致性)。
//   - mutex 持鎖跨 turnGreen+crossCar 兩個 callback 是必要的,否則別車
//     可能在切燈與 cross 之間插隊。
// =====================================================================
class TrafficLight {
public:
    void carArrived(
        int /*carId*/,
        int roadId,                              // 1 or 2
        int /*direction*/,                       // 題目允許忽略
        std::function<void()> turnGreen,
        std::function<void()> crossCar)
    {
        std::lock_guard lk(mu_);
        if (currentRoad_ != roadId) {
            turnGreen();
            currentRoad_ = roadId;
        }
        crossCar();
    }
private:
    std::mutex mu_;
    int currentRoad_ = 1;     // 初始 road 1 綠
};

void demo_Q9_traffic_light() {
    std::cout << "\n--- Q9. Traffic Light Intersection (LC 1279) ---\n";
    TrafficLight light;

    std::mutex io_mu;
    std::atomic<int> green_road{1};
    std::atomic<int> max_concurrent{0};
    std::atomic<int> in_cross{0};

    auto turnGreen = [&](int newRoad){
        return [&, newRoad]{
            std::lock_guard lk(io_mu);
            std::cout << "[turn green road " << newRoad << "] ";
            green_road = newRoad;
        };
    };
    auto crossCar = [&](int carId, int roadId){
        return [&, carId, roadId]{
            int now = ++in_cross;
            int prev = max_concurrent.load();
            while (now > prev && !max_concurrent.compare_exchange_weak(prev, now)) {}
            // 不變式:燈應該已經是我這條路
            if (green_road.load() != roadId) {
                std::lock_guard lk(io_mu);
                std::cout << "\n[INVARIANT VIOLATED] car " << carId << " road " << roadId << "\n";
            }
            {
                std::lock_guard lk(io_mu);
                std::cout << "car" << carId << "(R" << roadId << ") ";
            }
            std::this_thread::sleep_for(2ms);
            --in_cross;
        };
    };

    // 30 輛車隨機從兩條路來
    std::vector<std::thread> cars;
    std::mt19937 rng(123);
    for (int i = 1; i <= 30; ++i) {
        int roadId = (rng() % 2) + 1;
        int dir    = (rng() % 2) + 1;
        cars.emplace_back([&, i, roadId, dir]{
            std::this_thread::sleep_for(std::chrono::microseconds(rng() % 5000));
            light.carArrived(i, roadId, dir, turnGreen(roadId), crossCar(i, roadId));
        });
    }
    for (auto& t : cars) t.join();
    std::cout << "\n  max concurrent crossings: " << max_concurrent.load()
              << " (expected 1)\n";
}

// =====================================================================
// 實戰範例: 自己實作 token-bucket rate limiter
// =====================================================================
// 應用場景: 面試常見「設計 rate limiter」── 限制每秒最多 N 個
// 請求通過, 多餘的等下一個視窗。Token bucket 是最常見實作:
//   - 桶子裡有 N 個 token
//   - 每秒補 N 個 (上限不超過 N)
//   - 請求要 take 1 token; 沒 token 就 wait
//
// 用 mutex + cv + 一個 worker thread 補 token 即可實作。
// 這是面試常被要求「跟 Q5 不同, 寫一個基於時間的同步原語」的
// 典型題, 也是線上服務 (API gateway, 防 DDoS) 的真實 building
// block。
// =====================================================================
class TokenBucket {
    std::mutex mtx_;
    std::condition_variable cv_;
    int tokens_;
    const int cap_;
public:
    explicit TokenBucket(int cap) : tokens_(cap), cap_(cap) {}
    // 取一個 token, 沒有就等到有
    void take() {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [&]{ return tokens_ > 0; });
        --tokens_;
    }
    // 補 token 用 (由 background timer 呼叫)
    void refill(int n) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            tokens_ = std::min(cap_, tokens_ + n);
        }
        cv_.notify_all();
    }
};

void demo_token_bucket() {
    std::cout << "\n--- Bonus: Token Bucket rate limiter ---\n";
    TokenBucket tb(3);          // 容量 3
    std::atomic<bool> stop{false};
    // 補 token thread: 每 20 ms 補 1 個 (= 50 tokens/sec)
    std::thread refiller([&]{
        while (!stop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            tb.refill(1);
        }
    });
    // 5 條 worker 各想要 4 個 token
    std::vector<std::thread> ws;
    std::atomic<int> done{0};
    for (int i = 0; i < 5; ++i) {
        ws.emplace_back([&, i]{
            for (int k = 0; k < 4; ++k) {
                tb.take();
                std::cout << "  worker " << i << " got token #" << (k+1) << '\n';
            }
            done.fetch_add(1);
        });
    }
    for (auto& t : ws) t.join();
    stop.store(true);
    tb.refill(1);      // 叫醒 refiller 退出
    refiller.join();
    std::cout << "  all " << done.load() << " workers got their tokens\n";
}

// =====================================================================
// 已全部覆蓋 LeetCode Concurrency 標籤 9 題。
// 額外延伸 (本檔不實作,但常被問):
//
//   - 自己實作一個 ReadWriteLock ── lesson 10 已示範 std::shared_mutex,
//     面試常要求「徒手用 mutex+cv 實作」。Q5 變體:writers/readers
//     兩個 cv,加上 reader_count 與 writer_waiting 兩個計數來避免
//     writer starvation。
//
//   - 自己實作一個 Thread Pool ── 看 lesson 08 即可。
//
//   - 自己實作一個 Semaphore (用 mutex + cv) ── count 計數 + cv.wait
//     當 count == 0,release 時 ++count 與 notify_one。
//
//   - 自己實作一個 CountDownLatch / Barrier ── lesson 12 已示範,面試
//     常要求徒手復刻。CountDownLatch 用 atomic + cv,Barrier 多一個
//     generation 計數。
// =====================================================================

int main()
{
    demo_Q1_print_in_order();
    demo_Q2_foobar();
    demo_Q3_zero_even_odd();
    demo_Q4_h2o();
    demo_Q5_bounded_queue();
    demo_Q6_fizzbuzz();
    demo_Q7_dining_philosophers();
    demo_Q8_web_crawler();
    demo_Q9_traffic_light();
    demo_token_bucket();
    std::cout << "\n[done]\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：LeetCode concurrency 題目主要考哪幾類同步模型?
    //    A：三大類:(A) Sequencing (順序控制) ── Q1/Q2/Q3/Q6,把「下一個
    //       輪到誰」抽象成 turn 變數,用 binary_semaphore 互推或 cv +
    //       predicate;(B) Grouping/Barrier (湊齊組合) ── Q4 H2O 用
    //       counting_semaphore + barrier + completion lambda;(C) Resource
    //       sharing (爭搶資源) ── Q5/Q7/Q9,bounded buffer 用 mutex + 雙
    //       cv,多鎖用 scoped_lock,終止偵測用 atomic in-flight counter。
    //       看到題目先分類再選 primitive,90% 解題思路就定了。
    //
    //  Q2：Q5 BoundedBlockingQueue 為什麼用「兩個 cv」而非「一個 cv +
    //      notify_all」?
    //    A：共用一個 cv 也能跑對,但 producer 與 consumer 隨意呼叫
    //       notify_one() 時,可能叫醒「等錯條件」的人 ── 那人重檢
    //       predicate 失敗只好重新 wait,白白浪費一輪 (lost wakeup
    //       的近親 thundering herd)。雙 cv (notFull / notEmpty) 各自
    //       對應一個條件:producer push 完只 notify notEmpty 給 consumer,
    //       consumer pop 完只 notify notFull 給 producer ── 叫醒的人一
    //       定是該動的人,wakeup 成本最低。生產級 BlockingQueue 一律雙 cv。
    //
    //  Q3：Q1 Print in Order 為什麼用 binary_semaphore 而不能用 mutex?
    //    A：std::mutex spec 規定「lock 它的 thread 必須自己 unlock」,跨
    //       thread unlock 是 UB。Q1 想做的是「first 印完 → 開閘讓 second
    //       進」,等同跨 thread 喚醒 ── mutex 做不到 (first 結束後其他
    //       thread 解不開它鎖過的 mutex)。binary_semaphore 沒有「持有者」
    //       概念,任何 thread 都能 release,正好契合「一次性 gate / 紅綠
    //       燈」語意。同理 Q3 三個 binary_semaphore 形成環狀互推,也是
    //       semaphore 比 cv 簡潔的場景 ── 沒有「等什麼條件」只有「該你了」。
    //
    return 0;
}

// 編譯: g++ -std=c++20 -Wall -Wextra 30_interview_problems.cpp -o 30_interview_problems

// === 預期輸出 (節錄) ===
//
// --- Q1. Print in Order (LC 1114) ---
// first
// second
// third
//
// --- Q2. FooBar Alternately (LC 1115) ---
// [cv version]   foobarfoobarfoobarfoobarfoobar
// [sem version]  foobarfoobarfoobarfoobarfoobar
//
// --- Q3. Print Zero Even Odd (LC 1116) ---
// 01020304050607  (expected 0102030405060708... up to 07)
//
// --- Q4. Building H2O (LC 1117) ---
// HOHHOHHHOHHO  (each consecutive 3 chars must be 2H+1O)
//
// --- Q5. Bounded Blocking Queue (LC 1188) ---
// consumed sum = 36 (expected 36)
//
// --- Q6. Fizz Buzz Multithreaded (LC 1195) ---
// …（後略，完整輸出共 63 行）
