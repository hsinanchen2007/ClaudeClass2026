// =============================================================================
//  課程 3.5：執行緒本地儲存 2  —  thread_local vs 共享全域變數
// =============================================================================
//
// 【主題資訊 Information】
//   thread_local int counter = 0;      // 每條執行緒各有一份獨立實體
//   int globalCounter = 0;             // 全程序共用一份
//
//   標準版本：C++11(thread_local 是 C++11 新增的儲存期指定符)
//   標頭檔  ：不需要(語言關鍵字);本檔另用 <atomic>、<mutex> 做對照
//   儲存期  ：thread storage duration —— 在執行緒開始時建構、結束時解構
//
//   四種儲存期的完整對照:
//       automatic  —— 區域變數,離開作用域即銷毀
//       static     —— 靜態儲存期,程式啟動到結束(全程序一份)
//       thread     —— 執行緒儲存期,每條執行緒一份(thread_local)
//       dynamic    —— new/delete 手動控制
//
// 【詳細解釋 Explanation】
//
// 【1. thread_local 解決什麼問題】
//   多執行緒程式最大的麻煩來源是【共享可變狀態】。只要兩條執行緒
//   同時存取同一個物件、其中至少一方是寫入,而且沒有任何同步,
//   就構成 data race —— 這是未定義行為。
//
//   面對共享狀態,你其實只有三個選擇:
//     (a) 加鎖(mutex)          → 正確,但有競爭、有死結風險、會變慢
//     (b) 改用原子操作(atomic)→ 正確,但只適用於簡單型別與操作
//     (c) 【根本不要共享】       → 最快、最安全,因為問題被消滅了
//   thread_local 就是 (c) 的語言層支援:每條執行緒擁有自己的副本,
//   彼此在記憶體上是完全不同的物件,自然不可能有 data race。
//
//   這是並行程式設計裡最重要的一句話:
//   【最好的同步,就是不需要同步】。
//
// 【2. 本檔示範的核心對照】
//       int          globalCounter;   // 三條執行緒搶同一個 → 需要同步
//       thread_local int localCounter;// 三條執行緒各一份   → 天生安全
//
//   ⚠️ 課文原始碼直接對 globalCounter 做 ++,那是一個【真正的 data race】:
//   ++x 不是原子操作,它是「讀 → 加一 → 寫回」三步,兩條執行緒交錯執行
//   就會遺失更新。而 data race 在 C++ 裡是【未定義行為】,不是「結果可能
//   不準」那麼簡單 —— 標準對整個程式的行為不做任何保證。
//
//   因此本檔把共享計數器改成 std::atomic<int>(消除 UB、保留「共享」的
//   語意對照),並另外用 -DDEMONSTRATE_UB 隔離出原始的 racy 版本,
//   讓你可以親自觀察「遺失更新」現象,而不會污染預設的示範輸出。
//
// 【3. data race 與 race condition 是兩回事】
//   這兩個詞經常被混用,但意義完全不同,面試也很愛問:
//
//     * data race(資料競爭)—— 這是【語言層】的概念,有精確定義:
//       兩個執行緒對同一記憶體位置的存取,至少一個是寫入,
//       而且它們之間沒有 happens-before 關係。
//       後果:【未定義行為】。編譯器可以假設它不存在並據此最佳化。
//
//     * race condition(競態條件)—— 這是【邏輯層】的概念:
//       程式的正確性取決於事件發生的相對時序。
//       它【不一定】是 data race。例如兩條執行緒都用 mutex 保護地做
//       「檢查餘額 → 扣款」,沒有 data race(全程有鎖),
//       但若檢查和扣款不在同一個臨界區內,仍然會超扣 —— 這是
//       race condition。
//
//   一句話總結:data race 是「同時碰同一塊記憶體」,
//   race condition 是「順序不對導致邏輯錯」。
//   thread_local 能徹底消滅前者,但無法解決後者。
//
// 【4. thread_local 變數什麼時候建構、什麼時候解構?】
//   * 建構:在該執行緒【第一次使用它】之前完成(允許 lazy 初始化)。
//     對 main 執行緒而言,具有常數初始化的 thread_local 在程式啟動時就緒。
//   * 解構:在該執行緒【正常結束時】,以建構的相反順序解構。
//   ⚠️ 兩個重要的例外:
//     (a) 被 detach 的執行緒若在 main() 結束後才被作業系統終止,
//         其 thread_local 物件的解構【不保證】會發生。
//     (b) 呼叫 std::exit() 或 std::abort() 結束程式時,
//         其他執行緒的 thread_local 解構函式不會被執行。
//   所以 thread_local 物件不適合持有「必須被釋放」的關鍵資源。
//
// 【概念補充 Concept Deep Dive】
//
// (A) thread_local 在底層怎麼實作?
//   在 Linux/ELF x86-64 上,thread_local 變數放在每條執行緒的
//   TLS(Thread-Local Storage)區塊,由 %fs 段暫存器指向的
//   Thread Control Block 定位。存取一個 thread_local 變數大致是:
//       mov %fs:offset, %eax
//   也就是一次帶段前綴的記憶體存取 —— 比存取一般全域變數多一點點成本,
//   但【遠比取一次鎖便宜】(鎖在無競爭時約數十個週期,有競爭時是
//   系統呼叫等級的成本)。
//
//   需要動態初始化(建構函式不是 constexpr)的 thread_local,
//   每次存取還會多一個「是否已初始化」的檢查(TLS wrapper 函式),
//   所以熱路徑上盡量用可常數初始化的型別。
//
// (B) thread_local 與 static 的關係
//   在函式內寫 thread_local static int x; 時,static 是多餘的 ——
//   thread_local 本身就隱含靜態儲存期(區域 thread_local 變數的
//   生命週期涵蓋整條執行緒,不隨函式返回而銷毀)。
//   寫出來只是強調,不影響語意。
//
// (C) 為什麼 thread_local 不能用在非靜態的類別成員上?
//   thread_local 是【儲存期】指定符,而非靜態成員的生命週期由所屬物件
//   決定,兩者衝突。只有靜態資料成員可以是 thread_local ——
//   此時它的意思是「每條執行緒都有一份這個類別的靜態成員」。
//
// (D) 每條執行緒一份,代價是什麼?
//   記憶體。若一個 thread_local 物件佔 1 MB,開 100 條執行緒就是 100 MB。
//   而且 TLS 區塊在執行緒建立時就要配置,會拉長建立執行緒的時間。
//   所以 thread_local 適合放【小而常用】的東西(緩衝區索引、
//   隨機數種子、當前請求 id),不適合放大型資料結構。
//
// 【注意事項 Pay Attention】
//   1. 對非 atomic 的共享變數做 ++ 是 data race → 未定義行為,
//      不可描述成「結果會少算」這種固定結果(雖然實務上常觀察到遺失更新)。
//   2. 多條執行緒同時對 std::cout 輸出,行的【順序不確定】,
//      而且同一行可能被切開交錯 —— std::cout 的執行緒安全只保證
//      stream 物件不被毀損,不保證輸出不交錯。
//   3. thread_local 物件在 detached thread 或 std::exit() 的情況下
//      解構函式【不保證】被呼叫。
//   4. 每條執行緒一份 → 記憶體成本與執行緒數成正比,不適合放大物件。
//   5. 課文原始碼使用了 std::string 卻只 include <thread>,
//      能編譯是標頭傳遞依賴的巧合,本檔已補上 <string>。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 與 data race
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data race 和 race condition 有什麼不同?
//     答：data race 是【語言層】的精確定義 —— 兩條執行緒存取同一記憶體
//         位置、至少一方是寫入、且無 happens-before 關係,後果是
//         【未定義行為】。race condition 是【邏輯層】的概念 ——
//         程式正確性取決於事件的相對時序,它不一定涉及 data race。
//         例如全程持鎖但把「檢查」和「扣款」分成兩個臨界區,
//         沒有 data race 卻仍會超扣,那是 race condition。
//     追問：thread_local 能解決哪一種?
//         → 只能解決 data race(把共享變成不共享)。race condition 是
//           邏輯問題,要靠把相關操作放進同一個臨界區、或重新設計協定
//           來解決,換儲存期是沒有用的。
//
// 🔥 Q2. thread_local 變數什麼時候解構?有沒有不會解構的情況?
//     答：在該執行緒【正常結束時】,以建構的相反順序解構。
//         但有兩個重要例外:(a) detached thread 若在 main() 結束後才被
//         作業系統終止,解構不保證發生;(b) 呼叫 std::exit() 或
//         std::abort() 時,其他執行緒的 thread_local 解構不會執行。
//     追問：那 thread_local 適合放什麼?
//         → 小而常用、且「沒被釋放也不會出事」的東西:隨機數產生器的
//           狀態、當前請求的 trace id、暫存緩衝區。不要放必須關閉的
//           檔案句柄、必須歸還的連線、必須 flush 的緩衝 ——
//           那些要用明確的生命週期管理。
//
// ⚠️ 陷阱 1. 「++globalCounter 只是一個指令,應該是原子的吧?」
//     答：不是。++x 在機器層是「載入 → 加一 → 寫回」三個步驟
//         (即使 x86 能編成單一 inc 指令,那也不是原子的,除非加 lock 前綴)。
//         兩條執行緒交錯執行就會遺失更新。而且更關鍵的是:
//         這在 C++ 裡是 data race → 【未定義行為】,不只是「數字少算」——
//         編譯器有權基於「不存在 data race」的假設做最佳化。
//     為什麼會錯：把 C++ 的一行敘述等同於一個 CPU 指令,
//         又把「單一 CPU 指令」等同於「原子操作」。兩層假設都不成立。
//
// ⚠️ 陷阱 2. 「thread_local 變數在每條執行緒裡的初始值,會不會被前一條
//              執行緒改過的值影響?」
//     答：不會。每條執行緒的 thread_local 是【全新的獨立物件】,
//         按照宣告的初始化式重新初始化。執行緒 A 把它加到 100,
//         之後建立的執行緒 B 看到的仍然是初始值 0。
//     為什麼會錯：把 thread_local 想成「static 但加了鎖」。
//         它不是共享物件的同步存取,而是【複數個不同的物件】——
//         連記憶體位址都不一樣。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// 對照組:共享 vs 執行緒本地
//
//   課文原始碼是 int globalCounter = 0; 並直接 ++ —— 那是 data race(UB)。
//   這裡改成 std::atomic<int>,保留「全程序共用一份」的語意對照,
//   同時消除未定義行為。原始的 racy 版本另以 -DDEMONSTRATE_UB 示範。
// -----------------------------------------------------------------------------
std::atomic<int> globalCounter{0};   // 共享:全程序一份
thread_local int localCounter = 0;   // 各自獨立:每條執行緒一份

std::mutex coutMutex;                // 保護輸出,避免整行被切開交錯

void work(const std::string& name) {
    int g = globalCounter.fetch_add(1, std::memory_order_relaxed) + 1;
    ++localCounter;   // 完全不需要同步:這是本執行緒獨有的物件

    std::lock_guard<std::mutex> lk(coutMutex);
    std::cout << "  " << name
              << " global=" << g
              << " local=" << localCounter << "\n";
}

// -----------------------------------------------------------------------------
// 示範 1:課文原型(輸出順序不確定,故僅示範不納入確定性驗證)
// -----------------------------------------------------------------------------
void demoOriginal() {
    std::thread t1(work, "A");
    std::thread t2(work, "B");
    std::thread t3(work, "C");

    t1.join();
    t2.join();
    t3.join();

    std::cout << "  三條執行緒結束後 globalCounter = " << globalCounter.load()
              << "(共享,累積到 3)\n";
    std::cout << "  main 執行緒的 localCounter = " << localCounter
              << "(main 自己那一份,從未被子執行緒碰過)\n";
}

// -----------------------------------------------------------------------------
// 示範 2:確定性驗證 —— 每條執行緒的 thread_local 都從初始值開始
//
//   每條執行緒把自己的 localCounter 累加 N 次,再回報最終值。
//   因為互不干擾,每條的結果必定都是 N —— 這是可驗證的確定結果。
// -----------------------------------------------------------------------------
void demoIsolation() {
    const int N = 1000;
    const int T = 4;
    std::vector<int>         results(T, -1);
    std::vector<std::thread> threads;
    threads.reserve(T);

    for (int i = 0; i < T; ++i) {
        threads.emplace_back([i, N, &results]() {
            for (int k = 0; k < N; ++k) ++localCounter;
            results[i] = localCounter;   // 各寫各的槽位
        });
    }
    for (auto& t : threads) t.join();

    bool allEqualN = true;
    for (int r : results) if (r != N) allEqualN = false;

    std::cout << "  " << T << " 條執行緒各自累加 " << N << " 次\n";
    std::cout << "  每條的 localCounter 最終值:";
    for (std::size_t i = 0; i < results.size(); ++i) {
        std::cout << (i ? ", " : " ") << results[i];
    }
    std::cout << "\n";
    std::cout << "  全部等於 " << N << ": " << std::boolalpha << allEqualN
              << "(互不干擾,無需任何同步)\n";
    std::cout << "  main 的 localCounter 仍為 " << localCounter
              << "(不受任何子執行緒影響)\n";
}

// -----------------------------------------------------------------------------
// 示範 3:原始的 racy 版本 —— 預設不執行
//
//   對非 atomic 的 int 做 ++ 是 data race,屬於【未定義行為】。
//   實務上最常觀察到的症狀是「遺失更新」(最終值小於預期),
//   但標準不保證任何特定結果 —— 也可能剛好等於預期值。
//
//   觀察方式:
//     g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.5：執行緒本地儲存2.cpp" -o tls2_ub
//     ./tls2_ub
//   建議搭配 ThreadSanitizer 觀察它如何被偵測出來:
//     g++ -std=c++17 -fsanitize=thread -g -pthread -DDEMONSTRATE_UB ... && ./a.out
// -----------------------------------------------------------------------------
#ifdef DEMONSTRATE_UB
int racyCounter = 0;   // 刻意不加保護
#endif

void demoDataRace() {
#ifdef DEMONSTRATE_UB
    const int N = 100000;
    const int T = 4;
    std::vector<std::thread> threads;
    threads.reserve(T);
    for (int i = 0; i < T; ++i) {
        threads.emplace_back([N]() {
            for (int k = 0; k < N; ++k) ++racyCounter;  // ⚠️ data race → UB
        });
    }
    for (auto& t : threads) t.join();

    std::cout << "  預期值(若無競爭)= " << (N * T) << "\n";
    std::cout << "  本次實際觀察值   = " << racyCounter << "\n";
    std::cout << "  ⚠️ 這是未定義行為,本次觀察值不保證重現,也不保證會小於預期值。\n";
#else
    std::cout << "  已略過(預設不執行:對非 atomic 變數做 ++ 是未定義行為)。\n";
    std::cout << "  要親自觀察遺失更新,請加上 -DDEMONSTRATE_UB 重新編譯。\n";
#endif
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔【刻意不提供】
//
//   說明:LeetCode 的多執行緒題(1114 Print in Order、1115 Print FooBar
//   Alternately、1116 Print Zero Even Odd、1117 Building H2O、
//   1195 Fizz Buzz Multithreaded)的核心需求是【執行緒之間必須互相協調】,
//   而 thread_local 的作用恰好相反 —— 它讓執行緒【不共享任何東西】。
//   用 thread_local 解那些題目在方向上就是錯的(各執行緒看不到彼此的
//   狀態,根本無法排序)。兩者無交集,故從缺。
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 【日常實務範例】每執行緒的請求追蹤 ID(trace id)與統計
//
//   情境:微服務要能把一次請求在各層產生的 log 串起來 ——
//         同一個請求的所有 log 都帶同一個 trace id。
//         但 log 函式散布在數十個模組裡,不可能每個函式都多傳一個參數。
//
//   為何用 thread_local:
//     * 一條請求由一條 worker 執行緒從頭處理到尾,
//       所以「當前 trace id」天然就是【執行緒範圍】的狀態。
//     * 存成 thread_local 之後,任何深度的函式都能直接讀到,
//       不需要修改函式簽名(這是 log 框架、APM 工具的標準做法)。
//     * 完全沒有共享 → 沒有鎖 → 沒有競爭 → 熱路徑上幾乎零成本。
//
//   同時示範另一個常見用法:每執行緒累積統計,最後才歸併 ——
//   避免每次計數都去搶同一個 atomic(高競爭下 atomic 也會很慢)。
// -----------------------------------------------------------------------------
thread_local std::string g_traceId  = "-";
thread_local int         g_logCount = 0;

// 模擬「深處的某個函式」:不必知道 trace id 從哪來
std::string formatLogLine(const std::string& msg) {
    ++g_logCount;
    return "[trace=" + g_traceId + "] " + msg;
}

struct RequestStat {
    std::string traceId;
    int         logLines;
};

RequestStat handleRequest(const std::string& traceId) {
    g_traceId  = traceId;   // 進入點設定一次
    g_logCount = 0;

    // 底下這些函式完全不知道 trace id 的存在,卻都帶得上
    formatLogLine("auth ok");
    formatLogLine("db query start");
    formatLogLine("db query done");
    std::string last = formatLogLine("response 200");

    return {traceId, g_logCount};
}

int main() {
    std::cout << "=== 示範 1:課文原型(共享 vs thread_local)===\n";
    demoOriginal();

    std::cout << "\n=== 示範 2:確定性驗證 —— 執行緒間完全隔離 ===\n";
    demoIsolation();

    std::cout << "\n=== 示範 3:原始 racy 版本(data race → UB)===\n";
    demoDataRace();

    std::cout << "\n=== 日常實務:每執行緒 trace id ===\n";
    {
        std::vector<std::string> ids{"req-a1", "req-b2", "req-c3", "req-d4"};
        std::vector<RequestStat> stats(ids.size());
        std::vector<std::thread> workers;
        workers.reserve(ids.size());

        for (std::size_t i = 0; i < ids.size(); ++i) {
            workers.emplace_back([i, &ids, &stats]() {
                stats[i] = handleRequest(ids[i]);   // 各寫各的槽位
            });
        }
        for (auto& w : workers) w.join();

        for (const auto& s : stats) {
            std::cout << "  " << s.traceId << " 產生 " << s.logLines << " 行 log\n";
        }
        std::cout << "  每條 worker 的 trace id 互不干擾,log 函式不必多傳參數\n";
        std::cout << "  main 執行緒的 g_traceId 仍為 \"" << g_traceId
                  << "\"(未被任何 worker 影響)\n";
    }

    std::cout << "\n=== 全部示範結束 ===\n";
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存2.cpp" -o tls2
// 觀察 data race(未定義行為,結果不保證重現):
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB "課程 3.5：執行緒本地儲存2.cpp" -o tls2_ub

// 註:示範 1 中 A / B / C 三行的【先後順序不確定】,每次執行都可能不同
//     (輸出有 mutex 保護,所以不會被切開交錯,但誰先搶到鎖是排程決定的);
//     其 global= 的值也因此可能是 1/2/3 的任意排列。
//     示範 2、示範 4 的數值則是確定的:各執行緒互不干擾,結果與排程無關。

// === 預期輸出 ===
// === 示範 1:課文原型(共享 vs thread_local)===
//   A global=1 local=1
//   B global=2 local=1
//   C global=3 local=1
//   三條執行緒結束後 globalCounter = 3(共享,累積到 3)
//   main 執行緒的 localCounter = 0(main 自己那一份,從未被子執行緒碰過)
//
// === 示範 2:確定性驗證 —— 執行緒間完全隔離 ===
//   4 條執行緒各自累加 1000 次
//   每條的 localCounter 最終值: 1000, 1000, 1000, 1000
//   全部等於 1000: true(互不干擾,無需任何同步)
//   main 的 localCounter 仍為 0(不受任何子執行緒影響)
//
// === 示範 3:原始 racy 版本(data race → UB)===
//   已略過(預設不執行:對非 atomic 變數做 ++ 是未定義行為)。
//   要親自觀察遺失更新,請加上 -DDEMONSTRATE_UB 重新編譯。
//
// === 日常實務:每執行緒 trace id ===
//   req-a1 產生 4 行 log
//   req-b2 產生 4 行 log
//   req-c3 產生 4 行 log
//   req-d4 產生 4 行 log
//   每條 worker 的 trace id 互不干擾,log 函式不必多傳參數
//   main 執行緒的 g_traceId 仍為 "-"(未被任何 worker 影響)
//
// === 全部示範結束 ===
