// =============================================================
// 14_thread_sanitizer.cpp  --  用 ThreadSanitizer (TSan) 抓 race
// =============================================================
//
// 本課目標:
//   1. 學會 ThreadSanitizer 是什麼、怎麼啟用、怎麼讀它的報告。
//   2. 看到 TSan 把 lesson 02 那種在 x86 上 *看似* 漏失幾筆
//      但答案 *像是* 正確的 race,直接抓出來;以及把 lesson 11
//      中 release/acquire 配對被破壞的問題也抓出來 —— 兩者
//      都是普通測試與正式編譯抓不到的。
//   3. 培養一個習慣:任何多執行緒程式,CI 都要至少跑一次
//      TSan 版本。
//
// 兩種編譯方式 (這支程式 *故意* 留了 race,要兩種方式都跑):
//
//   (A) 一般編譯 (給人看「答案大致正確」的假象):
//       g++ -std=c++17 -O2 -pthread 14_thread_sanitizer.cpp \
//           -o 14_thread_sanitizer
//       ./14_thread_sanitizer
//
//   (B) TSan 編譯 (讓 TSan 把 race 抓出來):
//       g++ -std=c++17 -g -O1 -fsanitize=thread -pthread \
//           14_thread_sanitizer.cpp -o 14_thread_sanitizer_tsan
//       ./14_thread_sanitizer_tsan
//
//       若在 WSL2 / 較新核心上看到:
//         "FATAL: ThreadSanitizer: unexpected memory mapping"
//       原因是 ASLR (mmap_rnd_bits) 給的隨機區段超出 TSan
//       影子記憶體佈局所允許的範圍。最簡單的修法是用
//       setarch -R 把該行程的 ASLR 關掉:
//         setarch -R ./14_thread_sanitizer_tsan
//       或者調整 sysctl: sudo sysctl -w vm.mmap_rnd_bits=28
//
// 注意:
//   - TSan 開了會慢 5–15 倍、記憶體用 5–10 倍。它是給 CI /
//     開發環境用的,不是給生產環境。
//   - -fsanitize=thread 不能與 -fsanitize=address 同時用。
//   - 開 -O0 跑 TSan 也可以,但 -O1 通常已經足夠且比較快。
//   - 報告中的 "data race" 才是你要修的;如果有 "lock-order
//     inversion" / "potential deadlock" 也是 TSan 提供的好
//     東西,但本課先聚焦在 race。
// =============================================================

// =============================================================
// 課程資訊 (Class Info)
// =============================================================
// 主題:     ThreadSanitizer (TSan) ── 動態 race 偵測工具
// 前置課程: lesson 02, 11
// 觀念詞彙:
//   - dynamic instrumentation ── 執行期插入觀測,不靠靜態分析
//   - happens-before graph    ── TSan 維護的執行順序關係圖
//   - shadow memory           ── TSan 為每個 byte 額外存的 metadata
// 新介紹工具/旗標:
//   -fsanitize=thread             gcc/clang 開啟 TSan
//   -g                            保留 debug info,看得到行號
//   setarch -R ./prog             WSL2 上關 ASLR,避開 TSan crash
//   sysctl vm.mmap_rnd_bits=28    系統級的 ASLR 縮減
//   __tsan_acquire / __tsan_release  人為標註自製同步給 TSan 看
// TSan 抓什麼:
//   - data race (任何兩 thread 對同一 byte 至少一寫且無同步)
//   - deadlock / lock order inversion (⚠️ 僅 LLVM TSan,且預設關閉,見下方 5.B)
//   - 雙重 unlock、unlock 別人的鎖
//   - condition_variable 用法錯誤
// TSan 抓不到:
//   - 跨 process 共享記憶體的 race
//   - asm 內聯、特殊 syscall 的競爭
// 常見誤區:
//   - 認為「測試在 x86 過了就沒事」→ x86 把 bug 藏起來,TSan 揭露
//   - 在生產環境跑 TSan → 慢 5-15x、記憶體 5-10x,只用於 CI/dev
//   - 與 -fsanitize=address 同時用 → 不相容,擇一
// =============================================================

// =============================================================
// 深入解析 (Deep Dive) ── TSan 怎麼運作 + 抓得到/不到的東西
// =============================================================
//
// 1. shadow memory 模型
//    TSan 為 *每個* user byte 維護一個 shadow word (~8 byte)。每次 load/
//    store 編譯器插入 instrumentation:更新該 byte 的 shadow,記錄
//    「最近哪條 thread 在哪個 vector clock 寫了它」。讀寫時對照 shadow
//    判斷:
//      - 兩條不同 thread 沒有 happens-before 關係的存取,且至少一條是寫
//        → 報 race。
//    所以 TSan 不是「跑很多次、賭運氣抓到」的工具 ── race 只要在這次執行
//    真的發生過,通常當場就會被報出來,不必靠重跑碰運氣。
//    ⚠️ 但別把它講成「一定報」:TSan 每個 8-byte granule 只保留固定數量的
//    shadow cell,滿了就隨機淘汰,所以對「同一位置被大量 thread 反覆存取」
//    的熱點,偵測仍帶機率性。這與下方第 7 點「TSan 並不能保證沒 race」一致。
//
// 2. happens-before 怎麼建
//    TSan 識別所有同步原語:mutex.lock/unlock、atomic 的 release/acquire、
//    cv.wait/notify、thread.create/join、各種 fence。每呼叫一次,就更新
//    對應 thread 的 vector clock,讓 happens-before 關係沿著鎖/原子變數
//    傳遞下去。
//    → 用了 mutex 但 *沒包住所有寫入路徑* → race 仍會被發現。
//
// 3. 成本量化
//    runtime 慢 5-15x (依存取頻率)。
//    記憶體 5-10x (shadow 之外還有 metadata)。
//    啟動慢:每個 syscall 也要記。
//    → CI 上跑「能跑就好」測試,別在 production 開。
//
// 4. TSan 抓得到的 race
//    A. 普通變數 race  (data race UB)
//    B. atomic 用錯 memory order (例如 release 卻寫 relaxed → acquire 端
//       看不到先前的寫,雖然都是 atomic、TSan 仍會抓出 happens-before
//       缺失)
//    C. 漏鎖某個寫入路徑
//    D. mutex 持鎖期間呼叫的 callback 又 lock 同一把 → 不算 race,但
//       deadlock detector (TSan + 額外旗標) 也會抓
//
// 5. TSan 抓不到的東西
//    A. 邏輯 race condition (例如 check-then-act 雖各用 atomic,但兩步驟
//       中間別人插隊改了狀態)。TSan 看的是 happens-before,不是高層邏輯。
//    B. 死鎖。預設不抓,而且開關【不是編譯旗標】,是執行期環境變數:
//       TSAN_OPTIONS=detect_deadlocks=1 ./prog —— 且【只有 LLVM 的 TSan】
//       支援。GCC 的 TSan 對死鎖是靜默的(本課程 21_deadlock.cpp 的重現已
//       確認會直接 hang,不會有任何報告)。要查死鎖也可改用 Helgrind。
//    C. 不通過記憶體共享的競爭 (例如 file system race)。
//    D. 沒被觸發的程式分支裡的 race。
//
// 6. WSL2 / 容器 / KASLR 上的「unexpected memory mapping」
//    TSan 把 shadow 放在固定虛擬位址範圍 (例如 ~7000 GB)。新 Linux kernel
//    啟用 KASLR (kernel address layout randomization) 或 5-level page table,
//    user space 的 mmap 可能落在 TSan 想要的範圍 → 啟動就 abort。
//    解法:setarch -R ./prog 關 ASLR 給該 process,或用 docker run 的
//    --cap-add SYS_PTRACE 與舊一點的 image。
//
// 7. 如何 *最小化* race 觀察成本
//    TSan 並不能「保證沒 race」── 它只能保證「這次跑的程式碼路徑沒 race」。
//    要逼近覆蓋率:
//      A. 跑所有 unit test + integration test 在 TSan 下
//      B. fuzz / property test 多輪
//      C. 真實負載 stress test
//    工業作法:CI 開一個專門 TSan job,把所有 test 都跑一次。慢一點沒關係。
//
// 8. 與其他工具
//    Helgrind (valgrind)  ── 不需要重編,但慢更多 (50-100x)。對 atomic
//                            支援差,當代 C++ 程式不太準。
//    DRD (valgrind)        ── 類似 Helgrind,演算法略不同。
//    rr / record-replay    ── 可以重現偶發 race,但需配合事後 GDB。
//    建議首選 TSan,其次 rr 重現,Helgrind/DRD 留給沒 TSan 的舊 toolchain。
// =============================================================

// =============================================================
// 📚 LeetCode 對照 + 📖 cppreference 連結
// =============================================================
//
// 對應 LeetCode 題目:
//   ✗ LC Concurrency 標籤 *無直接對應題目* ── TSan 是工具,不是題目。
//   → 但實戰用法:把 lesson 30 任一題故意拔掉一處同步 (例如 Q5
//     BoundedBlockingQueue 把 push 端的 mutex 拿掉),用 -fsanitize=
//     thread 編譯跑跑,看 TSan 怎麼指出 race 位置。LC 上不會幫你抓,
//     但工作面試後寫真實程式必備。
//
// 主要 API 對照 (cppreference):
//   - C++ 規格不直接定義 TSan ── 但與「data race 是 UB」緊密相關:
//   - 多執行緒記憶體模型               https://en.cppreference.com/w/cpp/language/multithread
//   - C++ memory_order                 https://en.cppreference.com/w/cpp/atomic/memory_order
//   - 工具文件 (gcc/clang):
//     -fsanitize=thread                https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
//                                      https://clang.llvm.org/docs/ThreadSanitizer.html
//
// 練習建議:
//   - 把 lesson 30 任一題去掉一處同步,跑 TSan 看會抓到什麼。
//   - 進階:把所有 lesson 30 題目納入 CI,用 TSan 跑一輪當 regression test。
// =============================================================

/*
補充筆記：thread_sanitizer
  - thread_sanitizer 範例應先畫出共享資料與執行緒生命週期，再討論語法。
  - 任何跨執行緒讀寫都需要明確同步，例如 mutex、atomic、future 或條件變數。
  - thread 何時開始、何時停止、誰 join，是多執行緒範例的核心。
  - thread_sanitizer 屬於多執行緒主題；先判斷哪些資料被多個 thread 同時存取，哪些操作需要建立 happens-before 關係。
  - data race 是 C++ 中的未定義行為；只要一個 thread 寫、另一個 thread 同時讀或寫同一位置，且沒有同步，就不是單純偶發 bug。
  - mutex 保護的是共享資料的不變條件，不只是保護某一行程式；lock 的範圍要涵蓋檢查與修改的完整臨界區。
  - atomic 適合簡單共享狀態或 lock-free building block，但 memory_order 不應隨意填；初學先用預設 sequentially consistent 理解正確性。
  - condition_variable 必須搭配 predicate 迴圈，因為可能 spurious wakeup，也可能通知先於等待發生。
  - thread 生命週期要明確 join 或 detach；std::jthread 以 RAII 方式在解構時 request_stop 並 join，較不容易漏掉。
  - 效能問題如 false sharing、contention、過度建立 thread，通常在正確性之後才調整；先寫對，再量測。
*/

// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】ThreadSanitizer 與並行程式的驗證
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. TSan 是什麼？能抓到什麼、抓不到什麼？
//     答：-fsanitize=thread 在執行期以 happens-before 演算法追蹤每個記憶體位置的存取
//         與同步事件，能可靠報出「實際執行到的」data race（即使該次執行結果看起來正確），
//         也能報部分死鎖與 lock order inversion。限制：只能發現實際走到的路徑，覆蓋率
//         不足就漏；執行變慢數倍、記憶體用量大增；不報 race condition（邏輯錯），
//         只報 data race。
//     追問：能和 AddressSanitizer 一起開嗎？（不行，兩者不能同時使用）
//
// 🔥 Q2. 為什麼「跑很多次都答案正確」不能證明沒有 data race？
//     答：因為 data race 是 UB，不是機率性的錯誤數值。在 x86 上，強記憶體模型與特定
//         的優化決策可能讓有 race 的程式長期表現正常，直到換編譯器版本、換優化等級、
//         或換到 ARM 才爆炸。而且測試只覆蓋了實際發生的交錯順序，可能的交錯數量是
//         組合爆炸的。
//     追問：正確的驗收方式？（CI 至少跑一次 TSan 版本，加上對同步關係的 code review）
//
// ⚠️ 陷阱. 把所有共享變數都改成 std::atomic 之後 TSan 不報了，代表程式對了嗎？
//     答：不代表。TSan 之後不會報 data race（因為 atomic 存取依定義不構成 data race），
//         但「memory_order 選錯造成的邏輯錯」它抓不到——例如該用 acquire/release 卻寫
//         relaxed，happens-before 根本沒建立，TSan 仍視為合法的原子操作。這是無鎖程式
//         最危險之處。
//     為什麼會錯：把 TSan 當成「並行正確性的完整驗證器」。它只驗證 data race 這一個
//         性質；無鎖演算法的正確性需要模型檢查工具或人工推理 happens-before 鏈。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>

// =============================================================
// 故意留下的兩個錯誤
// =============================================================

// -------------------------------------------------------------
// 錯誤 1 —— 純粹的 race:counter++ 沒有任何同步。
//
// 在 x86 + -O2 下,counter 最後可能看起來「答案差不多」
// (lesson 02 已經示範過這個假象)。但 TSan 會立刻指出問題。
// -------------------------------------------------------------
long long bare_counter = 0;

void racy_increment(int n)
{
    for (int i = 0; i < n; ++i) {
        ++bare_counter;          // <<< TSan 會抓這一行
    }
}

// -------------------------------------------------------------
// 錯誤 2 —— release/acquire 被改成 relaxed (lesson 11 的樣子)。
//
// publisher 寫 payload 後用 *relaxed* store 設旗標。
// subscriber 用 *relaxed* load 看旗標,然後讀 payload。
//
// 在 x86 上,subscriber 幾乎永遠看到正確的 payload —— 因為
// x86 的 TSO 模型不會對這幾種寫做重排。但這在標準層面就是
// 一個 data race (沒有 happens-before 關係),TSan 會抓出來,
// 而在 ARM / Apple silicon 上才會真的炸。
// -------------------------------------------------------------
struct Payload {
    int    a = 0;
    int    b = 0;
};

Payload          payload;
std::atomic<bool> ready{false};

void publisher()
{
    payload.a = 7;
    payload.b = 42;
    // 應該是 std::memory_order_release。這裡故意寫錯。
    ready.store(true, std::memory_order_relaxed);   // <<< TSan 會抓這條配對
}

void subscriber()
{
    while (!ready.load(std::memory_order_relaxed)) { /* spin */ }
    // 這兩個讀沒有與 publisher 那邊建立 synchronizes-with 關係,
    // 因此這裡讀到 payload.a / payload.b 的值是 race。
    volatile int sink = payload.a + payload.b;
    (void)sink;
}

// =============================================================
// MAIN
// =============================================================
int main()
{
    // ---- 錯誤 1:跑一個 race 計數器 ----
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < 4; ++i) ts.emplace_back(racy_increment, 100'000);
        for (auto& t : ts) t.join();
        std::cout << "[race1] bare_counter = " << bare_counter
                  << "  (expected 400000)\n";
    }

    // ---- 錯誤 2:跑一個沒有 release/acquire 的發布訂閱 ----
    {
        std::thread p(publisher);
        std::thread s(subscriber);
        p.join();
        s.join();
        std::cout << "[race2] payload.a=" << payload.a
                  << " payload.b=" << payload.b << '\n';
    }

    // ---- 實戰 1:TSan 抓鎖順序不一致 (lock order inversion) ----
    //
    // 兩條 thread 拿同一對鎖, 但順序相反, 在某次 interleaving
    // 下會 deadlock。一般測試不會 100% 重現 (要時機剛好), TSan
    // 一次就抓得到「potential deadlock 鎖順序不一致」。
    //
    // 正解請見 lesson 21 ── 用 std::scoped_lock(m1, m2) 一次拿
    // 兩把就不會 deadlock。這裡只示範 TSan 能抓到的「錯誤模式」。
    {
        std::cout << "[race3] lock-order inversion (TSan 會警告)\n";
        std::mutex m1, m2;
        // worker 1: 先 m1 後 m2
        std::thread t1([&]{
            std::lock_guard<std::mutex> a(m1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> b(m2);
        });
        // worker 2: 先 m2 後 m1 (相反!)
        std::thread t2([&]{
            std::lock_guard<std::mutex> a(m2);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::lock_guard<std::mutex> b(m1);
        });
        t1.join(); t2.join();
        std::cout << "  (本次 run 沒卡住 != 沒問題, TSan 仍會印警告)\n";
    }

    // ---- 實戰 2:錯誤的 double-checked locking (TSan 抓得到) ----
    //
    // 經典反例: 雙重檢查鎖定的 plain bool 版。第一次 check 沒持
    // 鎖, 但 init_done 是 plain bool, 跟「init 完成」之間沒
    // happens-before → race。正解請見 lesson 15 ── 用 std::
    // call_once 或 Meyer's singleton。這裡示範 TSan 抓到的錯版。
    {
        std::cout << "[race4] broken double-checked init (TSan 會抓)\n";
        static bool init_done = false;     // 應該是 atomic<bool>!
        static int  data      = 0;
        static std::mutex init_mtx;

        auto try_init = [&]{
            if (!init_done) {              // race: plain bool 讀
                std::lock_guard<std::mutex> lk(init_mtx);
                if (!init_done) {
                    data = 42;
                    init_done = true;      // race: plain bool 寫
                }
            }
            // 讀 data: TSan 認為與另一條 thread 的 data 寫無同步
            volatile int sink = data; (void)sink;
        };
        std::thread t1(try_init), t2(try_init);
        t1.join(); t2.join();
        std::cout << "  data = " << data << " (lesson 15 教你正確做法)\n";
    }

    // ─────────────────────────────────────────────────────────
    // 課堂知識補充
    // ─────────────────────────────────────────────────────────
    //  Q1：TSan 抓 race 的原理是什麼? 跟一般測試差在哪?
    //    A：TSan 在每個 load/store/lock 操作上插入 instrumentation, 維
    //       護一個「shadow memory + happens-before 圖」。每次存取都查
    //       是否與另一條 thread 的存取無 happens-before 關係, 有就是
    //       race。所以即使 race 在執行時「沒造成錯誤輸出」, TSan 仍會
    //       在精確的源碼行號標出問題。一般 unit test 只能看「結果對
    //       不對」, 抓不到「結果剛好對但 UB」。
    //
    //  Q2：TSan 開銷多大? 能跟 ASan 一起用嗎?
    //    A：TSan 大約讓程式慢 5-15× 且記憶體用量增加 5-10×, 不適合生
    //       產, 但 CI 跑得起。重要: TSan 跟 ASan 不能同時開
    //       (-fsanitize=thread,address 會衝突), 要分兩個 build。WSL2 上
    //       常見「unexpected memory mapping」錯誤是 ASLR 跟 TSan shadow
    //       layout 衝突, 用 setarch -R ./prog 跑就解。
    //
    //  Q3：TSan 對 release/acquire 的配對錯誤怎麼抓?
    //    A：TSan 把 atomic 操作的 memory order 也納入 happens-before
    //       圖。如果你 store 用 relaxed 而對應 load 用 acquire, TSan
    //       認為兩者沒建立同步關係, 後續存取若依賴這個 publication 就
    //       被標 race。這是它比 valgrind/helgrind 強的關鍵 ── lesson
    //       11 那種 memory order 寫錯的 bug, 一般測試永遠抓不到, TSan
    //       直接打臉。
    //
    return 0;
}

// =============================================================
// 要記住的事
//
// 1. TSan = ThreadSanitizer,gcc 與 clang 都內建。它在執行期
//    動態觀察 *每一筆* 記憶體存取,維護一個 happens-before 圖,
//    在偵測到「兩個執行緒對同一 byte 至少一個寫,且沒有同步
//    關係」時印出一段 WARNING。
//
// 2. 開法:加 -fsanitize=thread (連結時也要加),建議再加
//    -g 才看得到原始碼行號。-O1 通常 OK,-O0 也行只是更慢。
//
// 3. TSan 報告長什麼樣 (節錄):
//
//      WARNING: ThreadSanitizer: data race (pid=12345)
//        Write of size 8 at 0x... by thread T2:
//          #0 racy_increment(int) 14_thread_sanitizer.cpp:LINE
//          #1 ...
//        Previous write of size 8 at 0x... by thread T1:
//          #0 racy_increment(int) 14_thread_sanitizer.cpp:LINE
//          #1 ...
//        Location is global 'bare_counter' of size 8 at 0x...
//
//    讀法:第一段是 *當下* 出問題的存取,第二段是 *先前* 那
//    個與它衝突的存取。兩者中間沒有任何 happens-before 關係。
//
// 4. TSan 抓不到的東西:
//      - 純單執行緒 bug、邏輯錯誤
//      - misuse of pthread API 在 std API 之外的部分有時會漏
//      - asm 內聯、跨進程共享記憶體 (shm) 的 race
//      - 與 signal handler 的競爭
//    抓得到的東西:
//      - 普通 data race (本課示範)
//      - mutex 解鎖另一條鎖、雙重 unlock
//      - 鎖獲取順序不一致 (lock order inversion / deadlock 警告)
//      - condition_variable 用法錯誤
//      - thread leaks
//
// 5. False positive 幾乎沒有。如果 TSan 抱怨,絕大多數時候
//    它真的有理。例外:你寫了「正確但 TSan 看不懂」的
//    custom 同步 (例如 hazard pointer、自製 RCU);這些情況
//    可以用 __tsan_acquire/__tsan_release 註解告訴它。
//
// 6. 把 TSan 放進 CI。理想的 CI 矩陣至少有三條:
//      - Release build,跑功能測試
//      - Debug build with TSan,跑同一批測試
//      - Debug build with ASan + UBSan (互相相容)
//    這三條合起來,大多數 C++ 後端能避免絕大部分的「上線
//    才壞」型 bug。
//
// 7. TSan 在 Apple silicon (M1/M2/M3) 上效果一樣好,且因為
//    那些晶片的記憶體模型較弱,你會發現本機就抓得到 lesson 11
//    那種「relaxed 偽裝成 release」的 bug —— 而它在 x86 上
//    可能要靠 TSan 才看得見。
// =============================================================

// 編譯: g++ -std=c++20 -Wall -Wextra 14_thread_sanitizer.cpp -o 14_thread_sanitizer
//
// ⚠️ 本檔是【刻意保留 data race】的示範，內含 spin loop，正常執行不會自己結束。
//    請用 ThreadSanitizer 觀察它報出的 race：
//    g++ -std=c++20 -fsanitize=thread -g 14_thread_sanitizer.cpp -o 14_thread_sanitizer && ./14_thread_sanitizer

// === 預期輸出 ===
