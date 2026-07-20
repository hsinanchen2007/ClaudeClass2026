// =============================================================
// 01_hello_thread.cpp  --  你的第一支 C++ 多執行緒程式
// =============================================================
//
// 本課目標:
//   1. 學會如何啟動第二個執行緒。
//   2. 觀察第二個執行緒會與 main() *並行* 執行。
//   3. 理解為何在執行緒物件被銷毀前必須呼叫 .join()。
//
// 編譯方式 (Linux / WSL, g++):
//     g++ -std=c++17 -pthread 01_hello_thread.cpp -o 01_hello_thread
//
// 執行方式:
//     ./01_hello_thread
//
// 你會看到「main」與「worker」的輸出交錯出現。每次執行的
// 順序都不一樣 —— 這正是執行緒的重點:它們各自獨立執行,
// 由作業系統決定誰先誰後。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     啟動一條執行緒,並等它結束
// 前置課程: 無 (本課是入門)
// 觀念詞彙:
//   - thread       ── 一條獨立的執行流;OS 排程器決定何時跑
//   - joinable     ── std::thread 物件還沒被 join/detach 的狀態
//   - interleaving ── 兩條 thread 的指令穿插出現的順序
// 新介紹 API:
//   std::thread(fn, args...)         建立並 *立即啟動* 一條執行緒
//   t.join()                         阻塞,等 t 結束
//   t.detach()                       放生 t,自行了斷生命
//   t.joinable()                     是否還沒被 join/detach
//   std::this_thread::sleep_for(d)   睡 *相對* 時間量
//   std::this_thread::sleep_until(t) 睡到 *絕對* 時間點 (週期排程不漂移)
// 何時使用:
//   - 想把工作丟到背景跑 (UI 不卡、I/O 不卡其他事)
// 何時不要用:
//   - 只是想拿到回傳值 → std::async + future (lesson 06)
//   - 太短的工作 (< 100 µs) → 開 thread 的 overhead 比工作本身貴
// 常見錯誤:
//   - 忘了 join 或 detach → 解構子呼叫 std::terminate
//   - 共享變數沒同步 → data race (lesson 02)
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── thread 在 OS 層做了什麼
// =============================================================
//
// 1. thread vs process
//    process 有獨立 virtual address space + page table。thread 共享同一個
//    address space (heap、global、code segment),但各自擁有 stack
//    (Linux 預設 8 MB) 與 register state。建立 thread 比 fork() 便宜 ~10×
//    (~10 µs vs ~100 µs);context switch 也便宜,因為不必切 page table、
//    不必 flush TLB。
//
// 2. 1:1 vs M:N 模型
//    Linux NPTL/glibc 是 *1:1*:每個 std::thread 對應一個 OS kernel
//    thread (透過 clone(CLONE_THREAD)),由 kernel scheduler 排程。
//    Go 的 goroutine 是 *M:N*:M 個用戶層 thread 跑在 N 個 OS thread 上。
//    C++ 標準沒指定,但所有主流平台 (Linux/Windows/macOS) 都是 1:1。
//
// 3. Context switch 真實成本
//    user→user 約 1-3 µs (Linux 5.x,KPTI 開啟更貴)。每次切換要存/載
//    register、可能切 mm_struct、可能 flush TLB。
//    → thread 數量 ≫ CPU 核數時,context switch 會主宰 overhead。
//    這就是 lesson 08 thread pool 出現的理由:把 N 個 task 排到固定 K
//    條 thread 上 (K ≈ 核數),避免重複建立 + 過度切換。
//
// 4. 為什麼 std::thread 解構子要 terminate?
//    委員會的兩難:default join 會掩蓋忘記同步的 bug、拖慢解構;default
//    detach 會讓 thread 在物件死後仍存取已銷毀的捕獲 → 隱蔽 UB。
//    最安全的選擇是「強迫使用者明確選擇」 → terminate。C++20 的
//    std::jthread (lesson 07) 改成「解構 = request_stop + join」,
//    把這層心智負擔自動化。
//
// 5. join() 的內部
//    Linux 上等同 pthread_join → futex 等 thread 的 exit flag。若 thread
//    已結束,join() 立刻返回;否則 caller 阻塞。同一個 std::thread 只能
//    join 一次,第二次是 UB (joinable() 已是 false)。
//
// 6. detach() 的隱藏代價
//    detach 後 thread 變 daemon。但它仍 share address space,捕獲的物件
//    若在主流程銷毀,detached thread 存取就是 UB。main() 結束時整個 process
//    被 SIGKILL,thread 內 RAII 解構子 *不會* 跑 → 檔案 buffer 沒 flush、
//    socket 沒關。實務上 99% 場景該選 join 或 jthread。
//
// 7. 啟動 vs 排程
//    `std::thread t(fn)` *建構後* thread 已是 runnable 狀態交給 OS。
//    它何時真的拿到 CPU 由 scheduler 決定 (可能下一奈秒、也可能 100 µs
//    後)。所以「啟動順序」不等於「執行順序」── 這是 lesson 02 race
//    condition 的根源之一。
//
// 8. sleep_for vs sleep_until ── 為什麼週期任務該用 sleep_until
//    sleep_for(50ms) 在每個 tick 都會 *漂移*:
//        t0 = now;
//        for (i...) {
//            do_work();              // 假設花 7 ms
//            sleep_for(50ms);        // 實際從 do_work 結束算起
//        }
//      第 1 次 tick 真實時間 ≈ t0 + 7 + 50 = t0 + 57 ms
//      第 2 次 tick           ≈ t0 + 7 + 50 + 7 + 50 = t0 + 114 ms
//      跑 1 小時後可能漂掉幾秒。
//
//    sleep_until(t0 + i*50ms) 用 *絕對時間點*,do_work 用了多久不影響
//    下一個 tick 的目標 ── 沒有累積漂移。OS 排程器仍可能延遲幾微秒,
//    但 *誤差不會累積*。實作上 Linux 對 sleep_until 走的就是 timerfd /
//    futex_wait_bitset 的 absolute time 介面,跟 sleep_for 是同一條
//    syscall 路徑,只差一個「時間點 vs 時間長度」參數。
//
//    經驗法則:
//      - 「睡個一下,順便讓出 CPU」          → sleep_for / yield
//      - 「我每 50 ms 要心跳一次,長期不漂」 → sleep_until + 預設目標
//      - 「睡到絕對時間 (例如下一個整點)」    → sleep_until
//
//    跟 cv.wait_until 同源:也是絕對時間點,週期 + 早醒友善。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ 本課 (純 std::thread 啟動 / join) 在 LC Concurrency 標籤 *無直接
//     對應題目*。LC 上的題目都假設 thread 已被啟動,要你解同步邏輯。
//   → 但所有 lesson 30 的 LC 解答都依賴本課這個基礎 ── 沒能正確 join,
//     什麼題目都做不完。
//
// 主要 API 對照 (cppreference):
//   - std::thread                       https://en.cppreference.com/w/cpp/thread/thread
//   - std::thread::join                 https://en.cppreference.com/w/cpp/thread/thread/join
//   - std::thread::detach               https://en.cppreference.com/w/cpp/thread/thread/detach
//   - std::thread::joinable             https://en.cppreference.com/w/cpp/thread/thread/joinable
//   - std::this_thread::sleep_for       https://en.cppreference.com/w/cpp/thread/sleep_for
//   - std::this_thread::sleep_until     https://en.cppreference.com/w/cpp/thread/sleep_until
//   - std::this_thread::get_id          https://en.cppreference.com/w/cpp/thread/get_id
//
// 練習建議:
//   讀完本課,去 lesson 07 看 std::jthread (C++20) 如何把「忘記 join」
//   的坑自動修好,再去 lesson 30 任一題看 thread 如何被組合成同步邏輯。
// =============================================================

/*
補充筆記：std::hello_thread
  - std::thread 建立後會立即開始執行 callable。
  - thread 物件在 destructor 前必須 join 或 detach，否則會呼叫 std::terminate。
  - 傳參數給 thread 預設會複製；要傳參考需使用 std::ref。
  - std::hello_thread 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::thread 基礎：建立、join/detach、參數傳遞
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. Thread 與 Process 的差別？
//     答：Process 是資源配置單位，擁有獨立虛擬位址空間、page table、fd table；
//         Thread 是 CPU 排程單位，同一 process 內共享 code/data/heap/fd，但各自
//         擁有 stack、暫存器與 thread_local。所以 thread 間通訊直接讀寫共享記憶體
//         （快但需同步），process 間必須走 IPC。Linux 底層兩者都是 task_struct。
//     追問：為什麼一個 thread segfault 會拖垮整個 process？
//
// 🔥 Q2. join() 與 detach() 差在哪？兩個都不呼叫會怎樣？
//     答：join() 阻塞當前 thread 直到目標結束並回收資源；detach() 讓它在背景獨立
//         執行，std::thread 物件與其脫鉤、變成 non-joinable。兩個都不呼叫：解構時
//         若仍 joinable()，會呼叫 std::terminate() 直接中止程式。這是刻意設計——
//         隱式 join 會讓解構點意外阻塞，靜默 detach 則導致懸空存取，都比崩潰危險。
//     追問：joinable() 何時回 false？（預設建構、已 join、已 detach、已被 move 走）
//
// 🔥 Q3. detach() 最大的風險是什麼？
//     答：生命週期不再受控。detached thread 若捕捉了區域變數的參考／指標，而建立它
//         的函式已返回，就是懸空存取（UB）；更隱蔽的是 main() 返回後 static 物件
//         開始解構、runtime 開始收尾，detached thread 仍在跑。標準沒有任何機制讓你
//         等待 detached thread。
//     追問：真要 fire-and-forget 怎麼做？（改用 thread pool，或 jthread + stop_token
//           並在 shutdown 時明確等待）
//
// ⚠️ 陷阱. void f(int&); 為什麼 std::thread t(f, x); 編譯不過？
//     答：std::thread 建構子對每個參數做 decay-copy（decay 後複製／移動到執行緒自己
//         的儲存空間），再以「右值」傳給可呼叫物件——右值綁不到非 const 左值參考。
//         必須寫 std::ref(x) 包成 reference_wrapper，並自行保證 x 活得比 thread 久。
//     為什麼會錯：多數人腦中的模型是「參數照函式簽章原樣轉發」，實際上是先值複製
//         一份。同理把 const char* 傳給收 std::string 的函式，字串轉換發生在「新
//         執行緒中」，若原字串已銷毀就是 UB —— 應直接傳 std::string。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>   // std::cout
#include <thread>     // std::thread, C++11 引入的執行緒類別
#include <chrono>     // std::chrono::milliseconds, 給 sleep_for 使用
#include <vector>     // std::vector (給並行下載 demo 用)
#include <string>     // std::string (給並行下載 demo 用)

// -------------------------------------------------------------
// 這是新執行緒要執行的函式。
// 「執行緒函式」就是一個普通函式 —— 簽章沒有任何特別之處。
// 當我們把它交給 std::thread,標準函式庫會安排它在一個與
// main() *不同* 的作業系統執行緒上執行。
// -------------------------------------------------------------
void worker(int id)
{
    // 印出 5 則訊息,每次之間 sleep 100 毫秒,讓 main()
    // 有機會在中間印出自己的訊息。如果不 sleep,這個執行緒
    // 可能在 main() 印出任何東西之前就結束了,我們想觀察
    // 的交錯效果就看不到。
    for (int i = 0; i < 5; ++i) {
        std::cout << "  [worker " << id << "] step " << i << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// -------------------------------------------------------------
// 週期心跳:示範 sleep_for 會漂移、sleep_until 不會。
//
// 兩條 worker 都在每個 tick 之間「假裝做了 7 ms 工作」,然後
// 想要每 30 ms 心跳一次。我們把每次 tick 的真實 wall-clock
// 偏移 (相對於程式起點) 印出來;sleep_until 版本能保持 30/60/90
// ms 整齊間隔,sleep_for 版本則因為加上 work 時間,漂到 37/74/...。
// -------------------------------------------------------------
void heartbeat_sleep_for(std::chrono::steady_clock::time_point t0)
{
    using namespace std::chrono;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(milliseconds(7));   // 假裝做事
        auto elapsed = duration_cast<milliseconds>(
                           steady_clock::now() - t0).count();
        std::cout << "  [for  ] tick " << i << " at +" << elapsed << " ms\n";
        std::this_thread::sleep_for(milliseconds(30));  // 漂移會累積
    }
}

void heartbeat_sleep_until(std::chrono::steady_clock::time_point t0)
{
    using namespace std::chrono;
    auto target = t0;
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(milliseconds(7));   // 同樣的工作
        auto elapsed = duration_cast<milliseconds>(
                           steady_clock::now() - t0).count();
        std::cout << "  [until] tick " << i << " at +" << elapsed << " ms\n";
        target += milliseconds(30);                     // 絕對目標
        std::this_thread::sleep_until(target);          // 不漂移
    }
}

// -------------------------------------------------------------
// 實戰範例: 並行下載 / 並行 I/O
// -------------------------------------------------------------
// 真實情境: 我們要從 3 個慢的 endpoint「下載」資料 (這裡用
// sleep_for 模擬網路延遲)。如果序列做要花 3 × 200ms = 600ms,
// 開 3 條 thread 並行做只需要 ~200ms。這是 std::thread 最基本
// 也最常見的工作場景: I/O 重疊。
//
// 注意: 我們把每個 thread 的結果寫到 *自己獨佔* 的 vector slot
// (results[i]), 不需要 mutex ── 因為不同 index 是不同位址,
// 沒有 race。這比 push_back 同一個 vector 更乾淨。
// -------------------------------------------------------------
void fetch_url(int idx, const std::string& url, std::vector<std::string>& results)
{
    // 假裝這是 HTTP request: 200 ms 等回應
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    results[idx] = "fetched-" + url;   // 寫到自己 slot, 無 race
}

void demo_parallel_download()
{
    std::cout << "\n[main] parallel download demo\n";
    std::vector<std::string> urls = {"api.foo.com", "api.bar.com", "api.baz.com"};
    std::vector<std::string> results(urls.size());

    auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> ts;
    for (size_t i = 0; i < urls.size(); ++i)
        ts.emplace_back(fetch_url, (int)i, urls[i], std::ref(results));
    for (auto& t : ts) t.join();   // 等全部下載完

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();
    std::cout << "  parallel total = " << ms << " ms (序列要 ~600ms)\n";
    for (auto& r : results) std::cout << "  got: " << r << '\n';
}

int main()
{
    std::cout << "[main] program starting\n";

    // ---------------------------------------------------------
    // 建立一個執行緒。
    //
    // 第一個參數是要執行的函式。
    // 其餘參數會被轉發為該函式的引數 —— 這裡我們把整數 1
    // 當作 `id` 傳進去。
    //
    // 這一行一旦執行完畢,`worker(1)` *已經在某處跑起來了*。
    // main() 並不會等它。
    // ---------------------------------------------------------
    std::thread t(worker, 1);

    // 在 worker 忙碌的同時,main() 也並行做自己的工作。
    for (int i = 0; i < 5; ++i) {
        std::cout << "[main] step " << i << '\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ---------------------------------------------------------
    // join() 會等待執行緒 `t` 結束。
    //
    // 規則:每個 std::thread 物件在被銷毀之前,必須呼叫過
    // join() 或 detach() 其中之一。如果你忘了,解構子會呼叫
    // std::terminate(),整個程式就掛掉。
    //
    // 你可以把 join() 想成「我會在這裡等你做完」。
    // ---------------------------------------------------------
    t.join();

    std::cout << "[main] worker has finished\n";

    // ---------------------------------------------------------
    // 對照 sleep_for vs sleep_until 的累積漂移行為。
    // ---------------------------------------------------------
    std::cout << "\n[main] heartbeat: sleep_for vs sleep_until\n";
    auto t0 = std::chrono::steady_clock::now();
    std::thread th_for  (heartbeat_sleep_for,   t0);
    th_for.join();
    auto t1 = std::chrono::steady_clock::now();
    std::thread th_until(heartbeat_sleep_until, t1);
    th_until.join();

    // ---------------------------------------------------------
    // 實戰範例: 並行下載
    // ---------------------------------------------------------
    demo_parallel_download();

    std::cout << "[main] exiting\n";

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：std::thread 解構時若仍 joinable, 為什麼是呼叫 std::terminate
    //       而不是 default join?
    //    A：default join 會把「忘了同步」的 bug 偷偷藏起來, 而且讓解構
    //       時間不可預期 (可能阻塞數秒)。default detach 又會讓 thread
    //       存取已銷毀的捕獲變數 (UB)。委員會選擇 terminate, 強迫程式
    //       員明確選邊。C++20 的 jthread (lesson 07) 改成自動 join +
    //       stop_token, 才把這層心智負擔自動化。
    //
    //  Q2：為什麼週期任務該用 sleep_until 而不是 sleep_for?
    //    A：sleep_for(50ms) 是「從現在開始睡 50ms」, 把工作時間也算進
    //       下一輪, 會逐輪累積漂移 (跑一小時可能漂掉幾秒)。sleep_until
    //       (target += 50ms) 鎖定「絕對時間點」, 工作快或慢都不影響下
    //       一個 tick 的目標, 誤差不累積。心跳、心律檢查、固定 FPS 都
    //       該用 sleep_until。
    //
    //  Q3：std::thread::hardware_concurrency() 是什麼? 可以拿來開幾條
    //       thread 嗎?
    //    A：它回傳「平台建議的並行 thread 數」(通常 = 邏輯核數), 但
    //       標準允許回傳 0 (代表無法判斷)。實務上要用 std::max(1u,
    //       hardware_concurrency()) 防呆, 而且 thread pool 的 worker
    //       數通常 = 核數 (CPU bound) 或更多 (I/O bound), 不要直接拿
    //       來當 thread 數開很多條。
    //
    return 0;
}
