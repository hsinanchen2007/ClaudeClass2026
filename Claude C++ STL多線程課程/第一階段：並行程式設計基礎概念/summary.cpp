/*
 * ============================================================
 * 【第一階段：並行程式設計基礎概念】總複習 summary.cpp
 * ============================================================
 * 本階段涵蓋課程：
 * - 課程 1.1：什麼是並行與並發
 * - 課程 1.2：為什麼需要多執行緒（效能、響應性、資源利用）
 * - 課程 1.3：執行緒 vs 程序（共享記憶體示範）
 * - 課程 1.4：多執行緒的挑戰（競爭條件、死結、飢餓）
 * - 課程 1.5：C++ 多執行緒發展史（C++11 到 C++20）
 * - 課程 1.6：標準函式庫總覽
 *
 * 重點摘要：
 * 1. 並發（Concurrency）是程式設計結構概念；並行（Parallelism）是物理執行概念
 * 2. 多執行緒三大核心價值：效能提升、響應性改善、資源利用
 * 3. 執行緒共享同一程序的記憶體空間，程序之間彼此隔離
 * 4. 多執行緒三大挑戰：競爭條件（Race Condition）、死結（Deadlock）、飢餓（Starvation）
 * 5. C++11 首次將多執行緒納入語言標準，C++14/17/20 持續擴展
 * 6. 核心標頭檔：<thread>、<mutex>、<condition_variable>、<future>、<atomic>
 *
 * 編譯指令：g++ -std=c++17 -pthread summary.cpp -o summary
 * ============================================================
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <vector>
#include <chrono>
#include <string>
#include <iomanip>
#include <numeric>

// ============================================================
// ===== 課程 1.1：什麼是並行與並發 =====
// ============================================================
//
// 【核心概念】
// ─────────────────────────────────────────────────────────────
// 並發（Concurrency）：
//   - 程式的「結構化方式」，強調同時處理多個任務的「能力」
//   - 強調程式的設計與結構，與硬體無關
//   - 多個任務在「邏輯上」同時進行（可能在單核心上輪流執行）
//
// 並行（Parallelism）：
//   - 多個任務在同一時刻「真正地」同時執行
//   - 強調執行與效能，需要多核心 CPU 硬體支援
//   - 並行一定是並發的，但並發不一定是並行的
//
// 生活比喻：
//   - 一位廚師同時處理三道菜（來回切換）= 並發，非並行
//   - 三位廚師各自處理一道菜（真正同步）= 並行
//
// 四種組合：
//   1. 非並發、非並行：[任務A] → [任務B] → [任務C]（循序）
//   2. 並發、非並行（單核）：[A1][B1][A2][C1]...（時間分片）
//   3. 並行、非並發：多核同時跑完全獨立的任務
//   4. 並發且並行（多核）：多個相關任務在多核上同時且交錯執行
//
// Context Switch（上下文切換）：
//   - 由作業系統自動處理，程式設計師不需要手動管理
//   - OS 保存執行緒的暫存器、程式計數器、堆疊指標到 TCB（執行緒控制區塊）
//   - TCB 存放在共享記憶體中，任何 CPU 都能存取
//   - 程式設計師只需要關心共享資料的同步（mutex、atomic 等）
//
// 【std::thread::hardware_concurrency()】
//   - 回傳系統支援的並行執行緒數量（通常等於 CPU 核心數 + 超執行緒）
//   - 可協助決定應建立多少執行緒
// ─────────────────────────────────────────────────────────────

// 課程 1.1 示範函式：模擬一個需要時間的任務
void performTask_1_1(const std::string& taskName, int iterations) {
    for (int i = 1; i <= iterations; ++i) {
        std::cout << "[" << taskName << "] 執行第 " << i
                  << " 次（執行緒 ID: " << std::this_thread::get_id() << "）\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[" << taskName << "] 完成！\n";
}

void demo_1_1_concurrency_vs_parallelism() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.1】並發 vs 並行 示範\n";
    std::cout << "========================================\n";
    std::cout << "主執行緒 ID: " << std::this_thread::get_id() << "\n";
    std::cout << "硬體支援的執行緒數量: " << std::thread::hardware_concurrency() << "\n";

    // 方式一：循序執行（非並發）—— 所有任務在同一執行緒中依序完成
    std::cout << "\n--- 循序執行（非並發）---\n";
    auto seqStart = std::chrono::steady_clock::now();
    performTask_1_1("任務A", 2);
    performTask_1_1("任務B", 2);
    auto seqEnd = std::chrono::steady_clock::now();
    auto seqMs = std::chrono::duration_cast<std::chrono::milliseconds>(seqEnd - seqStart).count();
    std::cout << "循序耗時: " << seqMs << " ms\n";

    // 方式二：並發執行 —— 三個任務同時在不同執行緒中進行
    std::cout << "\n--- 並發執行 ---\n";
    auto concStart = std::chrono::steady_clock::now();

    std::thread tA(performTask_1_1, "任務A", 2);
    std::thread tB(performTask_1_1, "任務B", 2);

    // join() 阻塞主執行緒，直到兩個子執行緒都完成
    tA.join();
    tB.join();

    auto concEnd = std::chrono::steady_clock::now();
    auto concMs = std::chrono::duration_cast<std::chrono::milliseconds>(concEnd - concStart).count();
    std::cout << "並發耗時: " << concMs << " ms\n";
    std::cout << "加速比: " << std::fixed << std::setprecision(2)
              << static_cast<double>(seqMs) / concMs << " 倍\n";
}


// ============================================================
// ===== 課程 1.2：為什麼需要多執行緒 =====
// ============================================================
//
// 【三大核心價值】
// ─────────────────────────────────────────────────────────────
// 1. 效能提升（Performance）
//    - 運算密集型任務（CPU-bound）：利用多核心加速
//      例：圖像處理、科學計算、密碼學、遊戲物理
//    - 多執行緒版本可接近 N 倍加速（N = 核心數）
//    - 工作分配：每個執行緒處理資料的子範圍
//    - 需使用 std::ref 傳遞引用以收集各執行緒結果
//
// 2. 響應性改善（Responsiveness）
//    - I/O 密集型任務（I/O-bound）：在等待時繼續做其他事
//      例：網路請求、檔案讀寫、資料庫查詢
//    - 將耗時操作移至背景執行緒，UI 執行緒保持暢通
//    - 協作式取消（Cooperative Cancellation）：
//      工作執行緒定期檢查 atomic<bool> cancelFlag
//
// 3. 資源利用（Resource Utilization）
//    - I/O 等待期間 CPU 閒置，利用多執行緒填補空白
//    - 並行獲取多個資源時：總時間 ≈ 最慢單個操作的時間
//      （vs 循序：總時間 = 所有操作時間的總和）
//    - 即使單核心也能從 I/O 密集型多執行緒中受益
//
// 【何時不該用多執行緒】
//   - 任務太小：執行緒建立開銷超過任務本身執行時間
//   - 強烈資料相依性：每步依賴前一步結果，無法並行化
//   - 大量同步需求：頻繁鎖競爭抵消並行化收益
//   - 記憶體頻寬瓶頸：問題不在 CPU 計算速度
//
// 【阿姆達爾定律（Amdahl's Law）】
//   加速比 = 1 / ((1 - P) + P/N)
//   P = 可並行化比例，N = 處理器數量
//   即使無限核心，若僅 50% 可並行化，最大加速比也只有 2 倍
//
// 【CPU-bound vs I/O-bound 最佳執行緒數】
//   CPU-bound：最佳執行緒數 ≈ CPU 核心數
//   I/O-bound：最佳執行緒數 >> CPU 核心數（等待期間 CPU 可用）
// ─────────────────────────────────────────────────────────────

// 使用 atomic 標誌示範「響應性」與「協作式取消」
std::atomic<bool> g_downloadComplete{false};
std::atomic<int>  g_downloadProgress{0};
std::atomic<bool> g_cancelRequested{false};

// 模擬背景下載執行緒
void simulateDownload(const std::string& filename) {
    std::cout << "[下載] 開始下載: " << filename << "\n";
    const int steps = 5;
    for (int i = 1; i <= steps; ++i) {
        if (g_cancelRequested.load()) {
            std::cout << "[下載] 收到取消請求，停止\n";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        g_downloadProgress.store(i * 100 / steps);
        std::cout << "[下載] 進度: " << g_downloadProgress.load() << "%\n";
    }
    g_downloadComplete.store(true);
    std::cout << "[下載] 下載完成！\n";
}

void demo_1_2_responsiveness() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.2】響應性改善 示範（協作式取消）\n";
    std::cout << "========================================\n";

    // 重設狀態
    g_downloadComplete.store(false);
    g_downloadProgress.store(0);
    g_cancelRequested.store(false);

    // 在獨立執行緒中執行下載
    std::thread downloadThread(simulateDownload, "example.zip");

    // 主執行緒（模擬 UI）繼續處理其他事
    std::cout << "[UI] 下載開始中，UI 保持響應...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[UI] 使用者點擊取消按鈕！\n";
    g_cancelRequested.store(true);

    downloadThread.join();
    std::cout << "[UI] 操作結束\n";
}

void demo_1_2_performance() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.2】效能提升 示範（質數計算）\n";
    std::cout << "========================================\n";

    const long long RANGE = 500000;
    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 2;

    // 輔助函式：判斷質數
    auto isPrime = [](long long n) -> bool {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        for (long long i = 3; i * i <= n; i += 2)
            if (n % i == 0) return false;
        return true;
    };

    // 計算範圍內質數數量
    auto countPrimes = [&](long long start, long long end) -> long long {
        long long cnt = 0;
        for (long long i = start; i <= end; ++i)
            if (isPrime(i)) ++cnt;
        return cnt;
    };

    // 單執行緒
    auto t1 = std::chrono::steady_clock::now();
    long long singleResult = countPrimes(1, RANGE);
    auto t2 = std::chrono::steady_clock::now();
    long long singleMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "單執行緒: " << singleResult << " 個質數，耗時 " << singleMs << " ms\n";

    // 多執行緒（工作分配）
    std::vector<std::thread> threads;
    std::vector<long long>   results(numCores, 0);
    long long rangeSize = RANGE / numCores;

    auto t3 = std::chrono::steady_clock::now();
    for (unsigned int i = 0; i < numCores; ++i) {
        long long start = 1 + i * rangeSize;
        long long end   = (i == numCores - 1) ? RANGE : start + rangeSize - 1;
        // std::ref 傳遞引用，讓執行緒直接寫回 results[i]
        threads.emplace_back([start, end, &results, i, &countPrimes]() {
            results[i] = countPrimes(start, end);
        });
    }
    for (auto& t : threads) t.join();
    auto t4 = std::chrono::steady_clock::now();
    long long multiMs     = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    long long multiResult = 0;
    for (auto r : results) multiResult += r;

    std::cout << "多執行緒(" << numCores << " 核): " << multiResult
              << " 個質數，耗時 " << multiMs << " ms\n";
    std::cout << "加速比: " << std::fixed << std::setprecision(2)
              << (singleMs > 0 ? static_cast<double>(singleMs) / multiMs : 1.0) << " 倍\n";
    std::cout << (singleResult == multiResult ? "結果驗證：一致\n" : "結果驗證：不一致！\n");
}


// ============================================================
// ===== 課程 1.3：執行緒 vs 程序 =====
// ============================================================
//
// 【程序（Process）】
//   - 執行中的程式實例
//   - 每個程序有獨立的記憶體空間（程式碼段、資料段、堆疊、堆積）
//   - 程序間通訊（IPC）需要特殊機制：管道、Socket、共享記憶體等
//   - 一個程序崩潰不影響其他程序
//   - 建立開銷大，切換開銷大
//
// 【執行緒（Thread）】
//   - 程式執行的最小單位，存在於程序內部
//   - 同一程序內的所有執行緒「共享」：
//       * 全域變數、static 變數
//       * 堆積（Heap）上的動態配置記憶體
//       * 程式碼（Text Segment）
//       * 開啟的檔案描述符
//   - 每個執行緒「獨有」：
//       * 堆疊（Stack）—— 區域變數、函式呼叫堆疊
//       * 程式計數器（Program Counter）
//       * CPU 暫存器狀態
//   - 建立開銷小，切換開銷小
//
// 【共享記憶體的雙面刃】
//   優點：執行緒間通訊方便，可直接讀寫共享變數
//   缺點：必須謹慎處理同步問題，否則會有競爭條件
//
// 【課程 1.3 關鍵示範】
//   下面的程式展示：同一程序的兩個執行緒共享 sharedCounter 全域變數，
//   但因為未加保護，導致結果不正確（競爭條件）。
// ─────────────────────────────────────────────────────────────

int g_sharedCounter = 0;  // 全域變數，所有執行緒共享

void incrementShared() {
    // 警告：這裡沒有保護，展示競爭條件
    for (int i = 0; i < 10000; ++i) {
        ++g_sharedCounter;  // 非原子操作！實際上是三個步驟：讀-改-寫
    }
}

void demo_1_3_shared_memory() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.3】執行緒共享記憶體 示範\n";
    std::cout << "========================================\n";

    g_sharedCounter = 0;
    std::thread t1(incrementShared);
    std::thread t2(incrementShared);
    t1.join();
    t2.join();

    // 預期 20000，但因競爭條件，結果通常不等於 20000
    std::cout << "預期結果: 20000\n";
    std::cout << "實際結果: " << g_sharedCounter << "\n";
    std::cout << "（結果不確定，這就是競爭條件！）\n";
}


// ============================================================
// ===== 課程 1.4：多執行緒的挑戰 =====
// ============================================================
//
// 【三大挑戰】
// ─────────────────────────────────────────────────────────────
//
// 1. 競爭條件（Race Condition）
//    定義：多個執行緒同時存取共享資料，且至少一個執行緒進行寫入，
//          結果取決於執行緒的執行順序。
//
//    根本原因：++counter 看似一行，實際上是三個步驟：
//      a. 讀取 counter 的值到暫存器
//      b. 暫存器的值 +1
//      c. 將結果寫回 counter
//    如果兩個執行緒在步驟 a/b 之間被切換，就會「遺失一次更新」。
//
//    解決方案：互斥鎖（std::mutex）、原子操作（std::atomic）
//
// 2. 死結（Deadlock）
//    定義：兩個或多個執行緒互相等待對方釋放資源，導致全部卡住。
//
//    四個必要條件（Coffman Conditions，必須同時滿足才會發生）：
//      a. 互斥（Mutual Exclusion）：資源一次只能被一個執行緒使用
//      b. 持有並等待（Hold and Wait）：持有資源的同時等待其他資源
//      c. 不可搶占（No Preemption）：已分配的資源不能被強制取走
//      d. 循環等待（Circular Wait）：存在執行緒的循環等待鏈
//
//    打破任一條件即可避免死結。
//
//    死結示意：
//      執行緒1 持有 mutexA，等待 mutexB
//      執行緒2 持有 mutexB，等待 mutexA
//      → 互相等待，永遠無法繼續！
//
//    解決方案：
//      - 固定鎖的獲取順序（所有執行緒都先鎖 A 再鎖 B）
//      - 使用 std::scoped_lock（C++17）同時鎖定多個 mutex
//      - 使用超時機制（std::timed_mutex）
//
// 3. 飢餓（Starvation）
//    定義：某個執行緒長時間無法獲得所需資源，導致無法執行。
//
//    常見原因：
//      - 優先權不公平（高優先權執行緒總是搶先）
//      - 鎖的獲取不公平（某些執行緒總是先搶到）
//      - 資源分配策略問題（某執行緒持有資源時間過長）
//
//    解決方案：公平鎖、優先權調整
//
// 【三大問題對照表】
//   問題      | 原因                 | 症狀           | 解決方向
//   ─────────────────────────────────────────────────────────
//   競爭條件  | 非同步存取共享資料   | 結果不可預測   | mutex、atomic
//   死結      | 循環等待資源         | 程式卡住不動   | 鎖順序、scoped_lock
//   飢餓      | 資源分配不公平       | 某執行緒無法執行 | 公平鎖
// ─────────────────────────────────────────────────────────────

// 使用 mutex 修正競爭條件
std::mutex g_counterMutex;
int        g_safeCounter = 0;

void incrementSafe() {
    for (int i = 0; i < 10000; ++i) {
        std::lock_guard<std::mutex> lock(g_counterMutex);  // RAII 鎖：自動加鎖、自動解鎖
        ++g_safeCounter;
    }
}

// 使用 atomic 修正競爭條件（更高效）
std::atomic<int> g_atomicCounter{0};

void incrementAtomic() {
    for (int i = 0; i < 10000; ++i) {
        ++g_atomicCounter;  // atomic 的 ++ 是原子操作，不會競爭
    }
}

void demo_1_4_challenges() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.4】多執行緒挑戰 示範\n";
    std::cout << "========================================\n";

    // 示範 1：競爭條件（Race Condition）
    std::cout << "\n--- 競爭條件示範 ---\n";
    g_sharedCounter = 0;
    {
        std::thread t1(incrementShared);
        std::thread t2(incrementShared);
        t1.join();
        t2.join();
    }
    std::cout << "無保護計數器（預期20000）: " << g_sharedCounter << "\n";

    // 示範 2：使用 mutex 修正
    std::cout << "\n--- 使用 mutex 修正競爭條件 ---\n";
    g_safeCounter = 0;
    {
        std::thread t1(incrementSafe);
        std::thread t2(incrementSafe);
        t1.join();
        t2.join();
    }
    std::cout << "mutex 保護計數器（預期20000）: " << g_safeCounter << "\n";

    // 示範 3：使用 atomic 修正（更高效）
    std::cout << "\n--- 使用 atomic 修正競爭條件 ---\n";
    g_atomicCounter.store(0);
    {
        std::thread t1(incrementAtomic);
        std::thread t2(incrementAtomic);
        t1.join();
        t2.join();
    }
    std::cout << "atomic 計數器（預期20000）: " << g_atomicCounter.load() << "\n";

    // 死結說明（不實際執行，因為會卡住）
    std::cout << "\n--- 死結說明 ---\n";
    std::cout << "死結情境：\n";
    std::cout << "  執行緒1 鎖定 mutexA → 等待 mutexB\n";
    std::cout << "  執行緒2 鎖定 mutexB → 等待 mutexA\n";
    std::cout << "  兩者互相等待，永遠無法繼續！\n";
    std::cout << "解決方法：固定鎖順序，或使用 std::scoped_lock（C++17）\n";
}


// ============================================================
// ===== 課程 1.5：C++ 多執行緒發展史 =====
// ============================================================
//
// 【C++11 之前的黑暗時代】
// ─────────────────────────────────────────────────────────────
// 不同平台有各自的 API，程式碼無法跨平台：
//
//   Windows:                    POSIX (Linux/macOS/Unix):
//   CreateThread()              pthread_create()
//   WaitForSingleObject()       pthread_join()
//   EnterCriticalSection()      pthread_mutex_lock()
//   LeaveCriticalSection()      pthread_mutex_unlock()
//
// POSIX 舊式寫法缺點：
//   - 需要手動管理記憶體與資源
//   - 型別不安全（使用 void*）
//   - 無法跨平台
//   - 容易出錯
//
// 【C++11：標準多執行緒的誕生（2011）】
//   新增標頭檔：
//   <thread>              → std::thread（跨平台、型別安全）
//   <mutex>               → std::mutex、std::lock_guard（RAII 風格）
//   <condition_variable>  → std::condition_variable
//   <future>              → std::future、std::promise、std::async
//   <atomic>              → std::atomic（無鎖程式設計）
//
//   優點：型別安全、跨平台、支援 Lambda、RAII 資源管理
//
// 【C++14（2014）—— 小幅改進】
//   - std::shared_timed_mutex（讀寫鎖）
//   - std::shared_lock
//
// 【C++17（2017）—— 重要擴展】
//   - std::shared_mutex（簡化版讀寫鎖）
//   - std::scoped_lock（同時鎖定多個 mutex，避免死結）
//   - 平行演算法（Parallel Algorithms）：std::sort(std::execution::par, ...)
//
// 【C++20（2020）—— 大幅增強】
//   - std::jthread（自動 join 的執行緒，解構時自動呼叫 join）
//   - std::counting_semaphore、std::binary_semaphore（信號量）
//   - std::latch（一次性倒數閂鎖）
//   - std::barrier（可重複使用的屏障）
//   - std::stop_token（協作式取消機制）
//   - 協程（Coroutines）
//
// 【C++23（2023）—— 持續完善】
//   - 更多原子操作改進
//   - std::expected（錯誤處理）
//
// 【編譯器支援參考】
//   功能               | GCC  | Clang | MSVC
//   C++11 多執行緒      | 4.8+ | 3.3+  | 2012+
//   C++17 平行演算法   | 9+   | 10+   | 2017+
//   C++20 jthread      | 10+  | 12+   | 2019+
// ─────────────────────────────────────────────────────────────

// 示範 C++11 現代寫法（對比 POSIX 舊式寫法）
void threadFunctionModern(int value) {
    // C++11：型別安全，直接接收 int
    std::cout << "現代 C++ 執行緒收到值: " << value << "\n";
}

// POSIX 舊式寫法的概念對比（這裡以注釋形式說明）
/*
void* threadFunctionPosix(void* arg) {      // POSIX：型別不安全，使用 void*
    int* value = (int*)arg;                  // 需要手動轉型
    printf("POSIX 執行緒收到值: %d\n", *value);
    return NULL;
}
int main_posix() {
    pthread_t thread;
    int value = 42;
    pthread_create(&thread, NULL, threadFunctionPosix, &value);
    pthread_join(thread, NULL);
    return 0;
}
*/

void demo_1_5_history() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.5】C++ 多執行緒發展史 示範\n";
    std::cout << "========================================\n";

    std::cout << "--- C++11 現代寫法（型別安全）---\n";
    // C++11：直接傳遞型別安全的參數
    std::thread t(threadFunctionModern, 42);
    t.join();

    std::cout << "--- C++17 scoped_lock（同時鎖定多個 mutex）---\n";
    std::mutex m1, m2;
    {
        // scoped_lock 同時鎖定 m1 和 m2，避免死結，解構時自動解鎖
        std::scoped_lock lock(m1, m2);
        std::cout << "同時持有 m1 和 m2，不會發生死結\n";
    }  // 自動解鎖

    std::cout << "--- 各版本功能展示 ---\n";
    std::cout << "C++11: thread, mutex, future, atomic, condition_variable\n";
    std::cout << "C++14: shared_timed_mutex, shared_lock\n";
    std::cout << "C++17: shared_mutex, scoped_lock, parallel algorithms\n";
    std::cout << "C++20: jthread, semaphore, latch, barrier, stop_token\n";
}


// ============================================================
// ===== 課程 1.6：標準函式庫總覽 =====
// ============================================================
//
// 【各標頭檔及核心功能】
// ─────────────────────────────────────────────────────────────
//
// <thread>  —— 執行緒管理
//   std::thread              執行緒類別（建構即執行）
//   std::this_thread         當前執行緒的操作命名空間
//     ::get_id()               取得執行緒 ID
//     ::sleep_for(duration)    休眠指定時間
//     ::sleep_until(time)      休眠到指定時間點
//     ::yield()                讓出 CPU 時間給其他執行緒
//   std::thread::hardware_concurrency()  查詢 CPU 核心數
//   std::jthread             自動 join 的執行緒 (C++20)
//
// <mutex>   —— 互斥鎖
//   互斥鎖類型：
//     std::mutex                 基本互斥鎖
//     std::timed_mutex           支援超時的互斥鎖
//     std::recursive_mutex       可遞迴鎖定
//     std::recursive_timed_mutex 遞迴 + 超時
//     std::shared_mutex          讀寫鎖 (C++17)
//   鎖管理器（RAII 風格，自動管理鎖的生命週期）：
//     std::lock_guard<M>         簡單 RAII 鎖（不可提前解鎖）
//     std::unique_lock<M>        靈活 RAII 鎖（可提前解鎖、可轉移）
//     std::scoped_lock<M...>     多重鎖 (C++17)
//     std::shared_lock<M>        共享（讀取）鎖 (C++14)
//   輔助：
//     std::call_once + std::once_flag  確保函式只執行一次
//
// <condition_variable>  —— 條件變數（執行緒間的等待與通知）
//   std::condition_variable      搭配 unique_lock<mutex>
//   std::condition_variable_any  搭配任意 BasicLockable
//   主要操作：
//     .wait(lock, predicate)     等待通知（會自動釋放鎖再等待）
//     .wait_for(lock, duration)  等待指定時間
//     .notify_one()              喚醒一個等待者
//     .notify_all()              喚醒所有等待者
//
// <future>  —— 非同步操作
//   std::future<T>       非同步結果的接收端（.get() 阻塞直到結果就緒）
//   std::promise<T>      非同步結果的發送端（.set_value() 傳遞結果）
//   std::shared_future<T> 可複製的 future（多個接收者）
//   std::packaged_task<F> 包裝可呼叫物件為非同步任務
//   std::async(policy, f, args...) 高階非同步執行函式
//     std::launch::async    強制在新執行緒中立即執行
//     std::launch::deferred 延遲執行（呼叫 .get() 時才執行）
//
// <atomic>  —— 原子操作（無鎖程式設計）
//   std::atomic<T>        原子類型模板（對 T 的所有操作都是原子的）
//   std::atomic_flag      最基本的原子布林（無鎖保證）
//   std::atomic_ref       對既有物件的原子操作 (C++20)
//   記憶體順序（memory_order）：
//     memory_order_relaxed  最寬鬆，僅保證原子性，無順序約束
//     memory_order_acquire  讀取操作：確保後續讀取不會被重排到此操作前
//     memory_order_release  寫入操作：確保前面的寫入不會被重排到此後
//     memory_order_acq_rel  同時具備 acquire 和 release 語意
//     memory_order_seq_cst  最嚴格，全序一致（預設值）
//
// <semaphore>  (C++20)
//   std::counting_semaphore<MaxVal>  計數信號量
//   std::binary_semaphore            二元信號量（等同 counting_semaphore<1>）
//
// <latch>  (C++20)
//   std::latch  一次性倒數閂鎖（倒數到 0 後開放所有等待者）
//
// <barrier>  (C++20)
//   std::barrier  可重複使用的屏障（所有執行緒到達後同時繼續）
//
// <stop_token>  (C++20)
//   std::stop_token    協作式取消令牌
//   std::stop_source   取消信號的發起者
//
// 【快速選擇指南】
//   我想要...              | 使用...                          | 標頭檔
//   建立執行緒             | std::thread                      | <thread>
//   保護共享資料           | std::mutex + std::lock_guard     | <mutex>
//   等待某個條件           | std::condition_variable          | <condition_variable>
//   執行非同步任務取得結果 | std::async + std::future         | <future>
//   無鎖計數器             | std::atomic<int>                 | <atomic>
//   限制並發數量           | std::counting_semaphore          | <semaphore>
//   等待多執行緒到達同一點 | std::barrier                     | <barrier>
// ─────────────────────────────────────────────────────────────

// 綜合示範：在一個程式中使用 thread、mutex、future、atomic
std::mutex g_printMutex;  // 保護 cout 輸出，避免輸出混雜

void demo_1_6_library_overview() {
    std::cout << "\n========================================\n";
    std::cout << "【課程 1.6】標準函式庫總覽 綜合示範\n";
    std::cout << "========================================\n";

    // 1. std::thread —— 建立執行緒
    std::cout << "\n[1] std::thread - 建立執行緒\n";
    std::thread t1([]() {
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::cout << "    Hello from thread! ID: " << std::this_thread::get_id() << "\n";
    });

    // 2. std::async + std::future —— 非同步執行，取得返回值
    std::cout << "[2] std::async - 非同步執行\n";
    auto future1 = std::async(std::launch::async, []() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 42;
    });

    // 3. std::atomic —— 原子計數器（無鎖）
    std::cout << "[3] std::atomic - 原子操作\n";
    std::atomic<int> counter{0};
    std::thread t2([&counter]() { ++counter; });
    std::thread t3([&counter]() { ++counter; });

    // 等待所有任務完成
    t1.join();
    t2.join();
    t3.join();

    // 取得 future 的結果（若還未完成則阻塞等待）
    int asyncResult = future1.get();
    std::cout << "    async 結果: " << asyncResult << "\n";
    std::cout << "    atomic 計數器: " << counter.load() << "\n";

    // 4. std::condition_variable —— 執行緒間等待與通知
    std::cout << "[4] std::condition_variable - 等待與通知\n";
    std::mutex cv_mutex;
    std::condition_variable cv;
    bool ready = false;
    std::string data;

    std::thread producer([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        {
            std::lock_guard<std::mutex> lock(cv_mutex);
            data = "資料已就緒";
            ready = true;
        }
        cv.notify_one();  // 通知等待者
    });

    std::thread consumer([&]() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        cv.wait(lock, [&ready]() { return ready; });  // 等待直到 ready == true
        std::cout << "    消費者收到: " << data << "\n";
    });

    producer.join();
    consumer.join();

    std::cout << "\n[總結] 標準函式庫各部分均運作正常\n";
}


// ============================================================
// ===== main() —— 執行所有示範 =====
// ============================================================
int main() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║  第一階段：並行程式設計基礎概念 — 總複習 summary.cpp ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";

    // 課程 1.1：並發 vs 並行
    demo_1_1_concurrency_vs_parallelism();

    // 課程 1.2：為什麼需要多執行緒
    demo_1_2_responsiveness();
    demo_1_2_performance();

    // 課程 1.3：執行緒共享記憶體（展示競爭條件）
    demo_1_3_shared_memory();

    // 課程 1.4：多執行緒的挑戰（競爭條件、死結、飢餓）
    demo_1_4_challenges();

    // 課程 1.5：C++ 多執行緒發展史
    demo_1_5_history();

    // 課程 1.6：標準函式庫總覽
    demo_1_6_library_overview();

    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║           第一階段複習完成！                          ║\n";
    std::cout << "╠══════════════════════════════════════════════════════╣\n";
    std::cout << "║  核心重點：                                           ║\n";
    std::cout << "║  1. 並發是設計概念，並行是執行現象                   ║\n";
    std::cout << "║  2. 多執行緒：效能、響應性、資源利用                 ║\n";
    std::cout << "║  3. 執行緒共享程序記憶體，需謹慎同步                 ║\n";
    std::cout << "║  4. 競爭條件→mutex/atomic，死結→固定鎖順序           ║\n";
    std::cout << "║  5. C++11 開始標準化，C++20 大幅增強                ║\n";
    std::cout << "║  6. 六大標頭檔：thread/mutex/cv/future/atomic/...    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";

    return 0;
}
