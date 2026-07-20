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

// =============================================================================
//  第一階段總複習 — 並行程式設計基礎概念（教科書級完整規格）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔與對應主題：
//     <thread>              std::thread / this_thread / hardware_concurrency  (C++11)
//     <mutex>               std::mutex / lock_guard / unique_lock / scoped_lock
//                           （scoped_lock 是 C++17）
//     <atomic>              std::atomic<T> / atomic_flag                      (C++11)
//     <future>              std::async / future / promise / packaged_task     (C++11)
//     <condition_variable>  condition_variable / condition_variable_any       (C++11)
//     <chrono>              steady_clock（量測經過時間的唯一正確選擇）        (C++11)
//
//   複雜度速記：
//     建立一條執行緒        ≈ 20–80 微秒（Linux clone(2) + 堆疊配置）
//     未競爭的 mutex 加解鎖 ≈ 20–50 奈秒（快速路徑不進核心）
//     競爭到的 mutex        ≈ 微秒等級（futex 系統呼叫 + 排程）
//     atomic fetch_add      ≈ 5–20 奈秒（x86 的 lock 前綴指令）
//   ⚠️ 以上皆為【實作與硬體相關】的量級參考，不是標準保證的數值。
//
// 【詳細解釋 Explanation】
//
// 【1. 本階段的主線：從「為什麼」到「有什麼代價」】
//   六堂課其實在回答四個遞進的問題：
//     1.1  並發與並行是什麼？    → 釐清概念，避免用錯詞、想錯模型
//     1.2  為什麼要用多執行緒？  → 三大價值：效能、響應性、資源利用
//     1.3  執行緒與程序差在哪？  → 共享記憶體是威力來源，也是所有麻煩的根源
//     1.4  會遇到什麼問題？      → 競爭條件、死結、飢餓
//     1.5–1.6 標準提供了什麼工具？
//   關鍵在於 1.3 與 1.4 的因果關係：【正因為執行緒共享記憶體，
//   才有 1.2 的低成本通訊優勢，也才有 1.4 的全部問題】。
//   這兩件事是同一枚硬幣的兩面，不能只要好處。
//
// 【2. data race 與 race condition —— 全階段最重要的區分】
//   這兩個詞中文都常被譯成「競爭」，但它們是【不同層次】的概念，
//   混用會導致完全錯誤的除錯方向：
//
//   * data race（資料競爭）：
//       定義（[intro.races]）：兩個執行緒【並行存取同一記憶體位置】，
//       其中【至少一個是寫入】，且兩者之間【沒有 happens-before 關係】。
//       後果：【未定義行為（UB）】—— 標準允許任何結果，
//       包含讀到撕裂的值、編譯器把迴圈最佳化掉、甚至 crash。
//       消除方式：mutex、atomic、或其他建立 happens-before 的同步機制。
//
//   * race condition（競爭條件）：
//       定義：程式的正確性依賴於「不受控的事件先後順序」。
//       後果：邏輯錯誤，但【不一定是 UB】。
//       典型是 check-then-act：
//           if (!map.count(k)) map[k] = compute();   // 兩步之間可能被插隊
//       即使每一步都用 mutex 保護（沒有 data race），
//       整體仍可能出錯 —— 因為臨界區的【邊界劃錯了】。
//
//   兩者的關係：
//       有 data race        → 一定是 bug（UB）
//       沒有 data race      → 【不代表】沒有 race condition
//       修掉所有 data race  → 只是拿到了「討論正確性」的入場券
//   本檔 demo_1_3 的 g_sharedCounter 是【data race】（未加任何同步的 ++），
//   demo_1_4 的死結示範則是【race condition】家族的問題。
//
// 【3. 為什麼 ++counter 不是原子操作】
//   在 x86-64 上，未最佳化的 ++counter 展開成三個步驟：
//       mov  rax, [counter]     ; load
//       add  rax, 1             ; modify
//       mov  [counter], rax     ; store
//   兩條執行緒可能同時 load 到相同的值，各自 +1 再寫回，
//   於是兩次遞增只生效一次 —— 這叫 lost update。
//   ⚠️ 即使編譯器最佳化成單一指令 inc [counter]，在硬體上【仍非原子】：
//      沒有 lock 前綴的話，其他核心仍可能在讀-改-寫之間插入。
//      只有 lock inc（也就是 std::atomic::fetch_add 產生的指令）
//      才會鎖住該條 cache line，保證整個讀-改-寫不可分割。
//
// 【4. 為什麼 demo_1_3 的結果「有時候剛好是 20000」】
//   這是本階段最危險的陷阱。data race 是 UB，
//   【不保證一定出錯，也不保證一定出對】。實測上：
//     * 迴圈次數少、執行緒啟動有時間差 → 可能剛好不重疊，答案「正確」；
//     * 開了 -O2，編譯器可能把整個迴圈摺疊成一次加法 → 看起來更「正確」；
//     * 換 ARM 等弱記憶體模型的平台 → 錯得更明顯。
//   ⚠️ 所以【絕對不能】用「跑起來答案對」來證明沒有 data race。
//      正確的驗證工具是 ThreadSanitizer（g++ -fsanitize=thread），
//      它偵測的是「有沒有違反 happens-before」，而不是「答案對不對」。
//
// 【概念補充 Concept Deep Dive】
//   * happens-before 的三個常見來源（本階段全部用到）：
//       (a) 同一條執行緒內的程式順序（sequenced-before）；
//       (b) mutex：解鎖 synchronizes-with 後續對同一 mutex 的加鎖；
//       (c) thread::join()：執行緒的完成 synchronizes-with join() 的返回。
//     (c) 特別實用 —— 它讓「worker 寫、main 在 join 後讀」不需要任何額外同步。
//
//   * lock_guard vs unique_lock vs scoped_lock：
//       lock_guard   最輕量，建構即鎖、解構即解，中途不能放開。
//       unique_lock  可延遲鎖定/中途 unlock/移動，condition_variable 必須用它。
//       scoped_lock  C++17，可一次鎖多個 mutex 且【內建死結避免演算法】，
//                    是同時鎖兩把以上時的正確選擇。
//
//   * 為什麼 mutex 比 atomic 慢：未競爭時兩者差距不大（mutex 快速路徑
//     也只是一次 atomic 操作）；一旦競爭，mutex 會走 futex 系統呼叫讓
//     執行緒進入睡眠，成本跳到微秒等級。atomic 則是自旋重試，
//     沒有系統呼叫，但高競爭下會浪費 CPU 並造成 cache line 乒乓。
//
//   * 執行緒不是免費的：每條執行緒預設堆疊在 Linux 上通常是 8 MB
//     【虛擬】位址空間（實體記憶體按需配置）。開幾千條執行緒會嚴重
//     消耗位址空間與排程器資源，這是要用執行緒池而非「每任務一執行緒」的原因。
//
// 【注意事項 Pay Attention】
//   1. data race ≠ race condition。前者是 UB、可用 atomic/mutex 消除；
//      後者是邏輯錯誤、即使無 data race 也可能存在。
//   2. 【不可】用「執行結果看起來正確」證明沒有 data race。UB 不保證出錯。
//      要驗證請用 g++ -fsanitize=thread。
//   3. demo_1_3 的 g_sharedCounter 是【刻意保留】的 data race 示範，
//      是教學用的錯誤示範，【請勿】當成可以照抄的寫法。
//   4. 本檔所有毫秒數、競爭條件的實際數值、執行緒交錯順序
//      【每次執行都不同】，且在不同機器上差異更大。
//   5. std::cout 保證無 data race，但【不保證整行輸出的原子性】。
//   6. hardware_concurrency() 可能回傳 0，且在容器中不反映 cgroup 配額。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】並行基礎概念總複習
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. data race 和 race condition 有什麼不同？
//     答：data race 是「兩條執行緒並行存取同一記憶體位置、至少一個是寫、
//         且無 happens-before 關係」，後果是【未定義行為】，
//         可用 mutex 或 atomic 消除。race condition 是「正確性依賴於
//         不受控的事件順序」，屬於邏輯錯誤，【即使完全沒有 data race
//         也可能存在】—— 例如全程用 mutex 保護的 check-then-act。
//         有 data race 一定是 bug；沒有 data race 只是及格線，不代表正確。
//     追問：舉一個沒有 data race 但有 race condition 的例子。
//         → if (!cache.contains(k)) cache.insert(k, compute());
//           兩個操作各自加鎖所以沒有 data race，但兩者之間可能被插隊，
//           導致 compute() 被執行兩次或覆蓋別人的結果。
//           修法是把「檢查與動作」放進【同一個】臨界區。
//
// 🔥 Q2. 為什麼 ++counter 在多執行緒下不安全？加了 volatile 能解決嗎？
//     答：++counter 是讀-改-寫三個步驟，兩條執行緒可能同時讀到相同的值，
//         各自 +1 再寫回，於是兩次遞增只生效一次（lost update）。
//         即使編譯器最佳化成單一 inc 指令，硬體上仍非原子 ——
//         必須有 lock 前綴才會鎖住 cache line。
//         volatile 【不能】解決：C++ 的 volatile 只承諾不省略讀寫、
//         不與其他 volatile 重排，它不提供原子性、不保證跨核心可見性、
//         也不建立 happens-before。正解是 std::atomic<int> 或 mutex。
//     追問：那 Java 的 volatile 呢？
//         → Java/C# 的 volatile 有 acquire/release 語意，比 C++ 強得多。
//           這是跨語言遷移時最常見的誤植來源，三家不能互相套用。
//
// 🔥 Q3. 並發與並行的差別？單核心機器上寫多執行緒有意義嗎？
//     答：並發是【程式的結構】（把程式組織成多個可獨立推進的任務，與硬體無關），
//         並行是【執行的現象】（多個任務同一時刻真的同時跑，需要多核心）。
//         單核心上仍非常有意義：(a) 可以讓 UI 保持響應；
//         (b) I/O 密集任務在等待時讓出 CPU，照樣能得到接近執行緒數的加速。
//         只有【CPU 密集】任務的吞吐量才真正需要多核心。
//     追問：那怎麼決定要開幾條執行緒？
//         → CPU 密集：約等於核心數（再多只會增加 context switch）。
//           I/O 密集：可以遠多於核心數，因為多數時間在等待。
//           實務上用執行緒池並依量測結果調整，不要憑感覺。
//
// 🔥 Q4. std::thread 解構時如果還 joinable，會發生什麼事？
//     答：呼叫 std::terminate()，程式立刻中止。這是標準【保證】的行為，
//         不是未定義行為。設計理由是解構時自動 join 會讓解構子阻塞任意久、
//         自動 detach 又會造成 dangling reference，委員會選擇讓錯誤立刻可見。
//         解法是用 RAII 包裝（自寫 ThreadGuard，或 C++20 的 std::jthread）。
//     追問：jthread 為什麼就敢自動 join？
//         → 因為它同時提供 stop_token，解構子先 request_stop() 再 join()，
//           讓「阻塞很久」這個缺點有了標準化的解法。
//
// ⚠️ 陷阱. demo_1_3 的競爭條件示範，我跑了好幾次結果都剛好是 20000，
//         是不是代表這段程式其實是安全的？
//     答：完全不是。data race 是【未定義行為】——標準允許任何結果，
//         【包括剛好正確】。看起來對的原因可能是：迴圈太短、
//         兩條執行緒的啟動有時間差而幾乎沒重疊、或編譯器把整個迴圈
//         摺疊成一次加法。換一台機器、換一個最佳化等級、
//         或在真實負載下，它隨時會出錯。
//     為什麼會錯：把「測試通過」當成「正確性證明」。
//         對 UB 而言，測試【原理上】就無法證明安全性 ——
//         你只是沒觸發到而已。唯一可靠的做法是用
//         ThreadSanitizer（g++ -fsanitize=thread）檢查，
//         它偵測的是「有沒有違反 happens-before 規則」這個【結構性】問題，
//         而不是「這次的答案對不對」。本檔的 demo 用 TSan 一跑就會報 race。
// ═══════════════════════════════════════════════════════════════════════════

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <queue>
#include <string>
#include <thread>
#include <vector>

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

    // 預期 20000；因為存在 data race，結果通常小於 20000（lost update）
    std::cout << "預期結果: 20000\n";
    std::cout << "實際結果: " << g_sharedCounter << "\n";
    std::cout << "（這是【data race】—— 未定義行為，不只是「結果不確定」。\n";
    std::cout << "  它甚至可能剛好印出 20000，但那不代表程式是安全的；\n";
    std::cout << "  用 g++ -fsanitize=thread 編譯執行，TSan 會直接報出這個 race。）\n";
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
// ============================================================
// ===== LeetCode 實戰範例 =====
// ============================================================

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 1】LeetCode 1114. Print in Order
//   題目：三條執行緒分別呼叫 first()/second()/third()，呼叫順序【任意】，
//         但輸出必須永遠是 "firstsecondthird"。
//   為什麼用到本階段主題：這題把課程 1.1（並發≠並行）與 1.4（同步的必要性）
//         綁在一起。三條執行緒結構上獨立（並發），可任意交錯執行，
//         題目卻要求確定的順序 —— 這只能靠【主動建立 happens-before 關係】
//         來達成，正是本階段 1.4 的核心結論。
//   ⚠️ wait 必須帶述詞：condition_variable 允許【虛假喚醒】，
//         沒有人 notify 也可能醒來。述詞版會在醒來後重新檢查條件。
// -----------------------------------------------------------------------------
class PrintInOrder {
public:
    void first(const std::function<void()>& printFirst) {
        std::unique_lock<std::mutex> lock(mtx_);
        printFirst();
        step_ = 2;
        cv_.notify_all();
    }
    void second(const std::function<void()>& printSecond) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return step_ == 2; });
        printSecond();
        step_ = 3;
        cv_.notify_all();
    }
    void third(const std::function<void()>& printThird) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return step_ == 3; });
        printThird();
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int step_ = 1;
};

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例 2】LeetCode 1116. Print Zero Even Odd
//   題目：三條執行緒協作輸出 "0102030405..."（n=5 時）。
//         zero() 印 n 個 0（每個數字前一個）、even() 印偶數、odd() 印奇數。
//   為什麼用到本階段主題：比 1114 更進一階 —— 它不是「一次性排序」，
//         而是【反覆輪流】的循環協調，需要一個能表達「現在輪到誰」的狀態機。
//         這正好示範課程 1.4 說的「同步不只是加鎖，更是設計狀態轉移」。
//   狀態設計：turn_ = 0 → 該印 0；1 → 該印奇數；2 → 該印偶數。
//         zero() 印完後依 counter 的奇偶決定下一棒交給誰，避免多餘的喚醒。
// -----------------------------------------------------------------------------
class ZeroEvenOdd {
public:
    explicit ZeroEvenOdd(int n) : n_(n) {}

    void zero(const std::function<void(int)>& printNumber) {
        for (int i = 1; i <= n_; ++i) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return turn_ == 0; });
            printNumber(0);
            turn_ = (i % 2 == 1) ? 1 : 2;   // 奇數棒交給 odd，偶數棒交給 even
            cv_.notify_all();
        }
    }

    void even(const std::function<void(int)>& printNumber) {
        for (int i = 2; i <= n_; i += 2) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return turn_ == 2; });
            printNumber(i);
            turn_ = 0;
            cv_.notify_all();
        }
    }

    void odd(const std::function<void(int)>& printNumber) {
        for (int i = 1; i <= n_; i += 2) {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return turn_ == 1; });
            printNumber(i);
            turn_ = 0;
            cv_.notify_all();
        }
    }

private:
    int n_;
    int turn_ = 0;
    std::mutex mtx_;
    std::condition_variable cv_;
};

void demo_leetcode() {
    std::cout << "\n========================================\n";
    std::cout << "【LeetCode 實戰】並行題實作\n";
    std::cout << "========================================\n";

    // ---- 1114. Print in Order ----
    {
        PrintInOrder foo;
        std::string out;
        std::mutex m;
        auto emit = [&](const char* s) {
            std::lock_guard<std::mutex> lk(m);
            out += s;
        };
        // 故意用相反順序建立執行緒，證明結果與建立順序無關
        std::thread t3([&] { foo.third([&] { emit("third"); }); });
        std::thread t2([&] { foo.second([&] { emit("second"); }); });
        std::thread t1([&] { foo.first([&] { emit("first"); }); });
        t3.join(); t2.join(); t1.join();
        std::cout << "1114 Print in Order（建立順序故意顛倒為 3→2→1）\n";
        std::cout << "  輸出: " << out << "\n";
    }

    // ---- 1116. Print Zero Even Odd ----
    {
        const int n = 5;
        ZeroEvenOdd zeo(n);
        std::string out;
        std::mutex m;
        auto emit = [&](int v) {
            std::lock_guard<std::mutex> lk(m);
            out += std::to_string(v);
        };
        std::thread tz([&] { zeo.zero(emit); });
        std::thread te([&] { zeo.even(emit); });
        std::thread to([&] { zeo.odd(emit); });
        tz.join(); te.join(); to.join();
        std::cout << "1116 Print Zero Even Odd（n = " << n << "）\n";
        std::cout << "  輸出: " << out << "\n";
    }
    std::cout << "  ← 兩題的輸出都是【確定的】：這正是同步原語存在的意義。\n";
}

// ============================================================
// ===== 日常實務範例 =====
// ============================================================

// -----------------------------------------------------------------------------
// 【日常實務範例】有界阻塞佇列 + 生產者/消費者（本階段所有概念的集大成）
//   情境：日誌收集器。多條業務執行緒（生產者）產生日誌事件，
//         少數幾條寫入執行緒（消費者）批次寫進磁碟或送往遠端。
//         佇列必須【有上限】—— 否則下游變慢時記憶體會被無限撐爆，
//         這是生產環境真實發生過的事故模式（unbounded queue 導致 OOM）。
//   本階段概念如何被用上：
//     * 課程 1.2 資源利用：生產者不必等磁碟 I/O，寫入與業務邏輯重疊進行；
//     * 課程 1.3 共享記憶體：佇列就是共享狀態，必須同步；
//     * 課程 1.4 競爭條件：用 mutex 保護佇列、用 cv 表達「滿 / 空」兩種等待；
//     * happens-before：消費者透過 mutex 看到生產者寫入的完整資料。
//   ⚠️ 兩個 condition_variable 的必要性：
//         notFull_ 給生產者等（佇列滿時），notEmpty_ 給消費者等（佇列空時）。
//         若共用一個 cv，notify 會喚醒錯誤的一方造成無謂的自旋，
//         極端情況下甚至可能所有人一起睡死（lost wakeup）。
//   ⚠️ 關閉協定（shutdown）：光設旗標不夠，必須 notify_all() 把
//         正在 wait 的消費者叫醒，否則它們永遠等不到，join() 就會卡死。
// -----------------------------------------------------------------------------
template <typename T>
class BoundedBlockingQueue {
public:
    explicit BoundedBlockingQueue(std::size_t cap) : cap_(cap) {}

    // 生產者：佇列滿就阻塞等待
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mtx_);
        notFull_.wait(lock, [this] { return q_.size() < cap_ || closed_; });
        if (closed_) return false;              // 已關閉，拒絕新資料
        q_.push(std::move(item));
        lock.unlock();                          // 先解鎖再通知，減少無謂的喚醒後阻塞
        notEmpty_.notify_one();
        return true;
    }

    // 消費者：佇列空就阻塞等待；已關閉且清空則回傳 false 讓消費者收工
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(mtx_);
        notEmpty_.wait(lock, [this] { return !q_.empty() || closed_; });
        if (q_.empty()) return false;           // 只有「已關閉且已清空」會走到這
        out = std::move(q_.front());
        q_.pop();
        lock.unlock();
        notFull_.notify_one();
        return true;
    }

    // 關閉：設旗標 + 叫醒【所有】等待者，否則它們會永遠睡著
    void close() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            closed_ = true;
        }
        notFull_.notify_all();
        notEmpty_.notify_all();
    }

private:
    std::size_t cap_;
    std::queue<T> q_;
    std::mutex mtx_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    bool closed_ = false;
};

void demo_practical_producer_consumer() {
    std::cout << "\n========================================\n";
    std::cout << "【日常實務】有界阻塞佇列：日誌收集器\n";
    std::cout << "========================================\n";

    BoundedBlockingQueue<std::string> queue(4);   // 刻意設小，讓「佇列滿」真的發生
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::mutex coutMtx;                            // 保護輸出，避免整行被撕裂

    auto say = [&coutMtx](const std::string& s) {
        std::lock_guard<std::mutex> lk(coutMtx);
        std::cout << s << "\n";
    };

    // 2 條消費者（模擬批次寫檔）
    std::vector<std::thread> consumers;
    for (int c = 0; c < 2; ++c) {
        consumers.emplace_back([&, c] {
            std::string item;
            while (queue.pop(item)) {
                consumed.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::milliseconds(30));  // 模擬 I/O
                say("  [consumer " + std::to_string(c) + "] 寫入 " + item);
            }
            say("  [consumer " + std::to_string(c) + "] 佇列已關閉且清空，收工");
        });
    }

    // 3 條生產者（模擬業務執行緒）
    std::vector<std::thread> producers;
    for (int p = 0; p < 3; ++p) {
        producers.emplace_back([&, p] {
            for (int i = 0; i < 4; ++i) {
                queue.push("log-" + std::to_string(p) + "-" + std::to_string(i));
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : producers) t.join();
    queue.close();                                 // 生產結束 → 關閉並喚醒消費者
    for (auto& t : consumers) t.join();

    std::cout << "  生產 " << produced.load() << " 筆、消費 " << consumed.load()
              << " 筆（兩者相等代表沒有遺失，佇列容量僅 4）\n";
    std::cout << "  ← 佇列有上限：下游變慢時生產者會被擋住（背壓 back-pressure），\n";
    std::cout << "    而不是無限堆積記憶體直到 OOM。\n";
}

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

    // LeetCode 實戰：1114 Print in Order、1116 Print Zero Even Odd
    demo_leetcode();

    // 日常實務：有界阻塞佇列（生產者/消費者）
    demo_practical_producer_consumer();

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
// 編譯: g++ -std=c++17 -Wall -Wextra -pthread summary.cpp -o summary
//   本檔只用到 C++11 起就有的功能（lock_guard/atomic/future/condition_variable），
//   已用 -std=c++17 -pedantic-errors 驗證通過，0 警告。
//
//   ★ 驗證 data race 的正確方式（強烈建議親自跑一次）：
//       g++ -std=c++17 -fsanitize=thread -g -pthread summary.cpp -o summary_tsan
//       ./summary_tsan
//     ThreadSanitizer 會明確報出 demo_1_3 的 incrementShared() 有 data race。
//     實測結果：偵測到 2 筆 data race 警告，程式以 exit code 66 結束。
//     這證明了「答案有時看起來正確」【完全無法】用來證明沒有 race。

// 註:
//   ⚠️ 非決定性內容，【每次執行都不同】：
//   * 所有執行緒 ID（實作定義的數值，執行緒結束後還可能被重複使用）；
//   * 所有毫秒數與加速比（受機器、核心數、當下負載影響）；
//   * 【課程 1.3 / 1.4 的無保護計數器數值】—— 那是 data race（未定義行為），
//   標準允許任何結果，包含剛好等於 20000；
//   * 多執行緒區段的輸出交錯順序。
//   【確定不變】的是：mutex 版與 atomic 版永遠是 20000；
//   LeetCode 1114 永遠輸出 firstsecondthird、1116 永遠輸出 0102030405；
//   生產者/消費者的生產數與消費數永遠相等（12 = 12，無遺失）。
//   以下為本機（16 邏輯核心）某一次的實際執行結果，重複區段以 ... 省略。

// === 預期輸出 ===
// ╔══════════════════════════════════════════════════════╗
// ║  第一階段：並行程式設計基礎概念 — 總複習 summary.cpp ║
// ╚══════════════════════════════════════════════════════╝
//
// ========================================
// 【課程 1.1】並發 vs 並行 示範
// ========================================
// 主執行緒 ID: 135125134903232
// 硬體支援的執行緒數量: 16
//
// --- 循序執行（非並發）---
// [任務A] 執行第 1 次（執行緒 ID: 135125134903232）
// [任務A] 執行第 2 次（執行緒 ID: 135125134903232）
// [任務A] 完成！
// [任務B] 執行第 1 次（執行緒 ID: 135125134903232）
// [任務B] 執行第 2 次（執行緒 ID: 135125134903232）
// [任務B] 完成！
// 循序耗時: 201 ms
//
// --- 並發執行 ---
// [任務A] 執行第 1 次（執行緒 ID: 135125127919296）
// [任務B] 執行第 1 次（執行緒 ID: 135125119526592）
// [任務A] 執行第 2 次（執行緒 ID: 135125127919296[任務B] 執行第 2 次（執行緒 ID: 135125119526592）
// ）
// [任務B] 完成！
// [任務A] 完成！
// 並發耗時: 100 ms
// 加速比: 2.01 倍
//
// ========================================
// 【課程 1.2】響應性改善 示範（協作式取消）
// ========================================
// [UI] 下載開始中，UI 保持響應...
// [下載] 開始下載: example.zip
// [下載] 進度: 20%
// [下載] 進度: 40%
// [UI] 使用者點擊取消按鈕！
// [下載] 進度: 60%
// [下載] 收到取消請求，停止
// [UI] 操作結束
//
// ========================================
// 【課程 1.2】效能提升 示範（質數計算）
// ========================================
// 單執行緒: 41538 個質數，耗時 127 ms
// 多執行緒(16 核): 41538 個質數，耗時 11 ms
// 加速比: 11.55 倍
// 結果驗證：一致
//
// ========================================
// 【課程 1.3】執行緒共享記憶體 示範
// ========================================
// 預期結果: 20000
// 實際結果: 19675
// （這是【data race】—— 未定義行為，不只是「結果不確定」。
//   它甚至可能剛好印出 20000，但那不代表程式是安全的；
//   用 g++ -fsanitize=thread 編譯執行，TSan 會直接報出這個 race。）
//
// ========================================
// 【課程 1.4】多執行緒挑戰 示範
// ========================================
//
// --- 競爭條件示範 ---
// 無保護計數器（預期20000）: 16281
//
// --- 使用 mutex 修正競爭條件 ---
// mutex 保護計數器（預期20000）: 20000
//
// --- 使用 atomic 修正競爭條件 ---
// atomic 計數器（預期20000）: 20000
//
// --- 死結說明 ---
// 死結情境：
//   執行緒1 鎖定 mutexA → 等待 mutexB
//   執行緒2 鎖定 mutexB → 等待 mutexA
//   兩者互相等待，永遠無法繼續！
// 解決方法：固定鎖順序，或使用 std::scoped_lock（C++17）
//
// ========================================
// 【課程 1.5】C++ 多執行緒發展史 示範
// ========================================
// --- C++11 現代寫法（型別安全）---
// 現代 C++ 執行緒收到值: 42
// --- C++17 scoped_lock（同時鎖定多個 mutex）---
// 同時持有 m1 和 m2，不會發生死結
// --- 各版本功能展示 ---
// C++11: thread, mutex, future, atomic, condition_variable
// C++14: shared_timed_mutex, shared_lock
// C++17: shared_mutex, scoped_lock, parallel algorithms
// C++20: jthread, semaphore, latch, barrier, stop_token
//
// ========================================
// 【課程 1.6】標準函式庫總覽 綜合示範
// ========================================
//
// [1] std::thread - 建立執行緒
// [2] std::async - 非同步執行
//     Hello from thread! ID: 135124855264960[3] std::atomic - 原子操作
//
//     async 結果: 42
//     atomic 計數器: 2
// [4] std::condition_variable - 等待與通知
//     消費者收到: 資料已就緒
//
// [總結] 標準函式庫各部分均運作正常
//
// ========================================
// 【LeetCode 實戰】並行題實作
// ========================================
// 1114 Print in Order（建立順序故意顛倒為 3→2→1）
//   輸出: firstsecondthird
// 1116 Print Zero Even Odd（n = 5）
//   輸出: 0102030405
//   ← 兩題的輸出都是【確定的】：這正是同步原語存在的意義。
//
// ========================================
// 【日常實務】有界阻塞佇列：日誌收集器
// ========================================
//   [consumer 0] 寫入 log-0-0
//   [consumer 1] 寫入 log-0-1
//   [consumer 0] 寫入 log-0-2
//   [consumer 1] 寫入 log-0-3
//   [consumer 0] 寫入 log-2-0
//   [consumer 1] 寫入 log-2-1
//   [consumer 0] 寫入 log-2-2
//   [consumer 1] 寫入 log-1-0
//   [consumer 0] 寫入 log-2-3
//   [consumer 1] 寫入 log-1-1
//   [consumer 0] 寫入 log-1-2
//   [consumer 0] 佇列已關閉且清空，收工
//   [consumer 1] 寫入 log-1-3
//   [consumer 1] 佇列已關閉且清空，收工
//   生產 12 筆、消費 12 筆（兩者相等代表沒有遺失，佇列容量僅 4）
//   ← 佇列有上限：下游變慢時生產者會被擋住（背壓 back-pressure），
//     而不是無限堆積記憶體直到 OOM。
//
// ╔══════════════════════════════════════════════════════╗
// ║           第一階段複習完成！                          ║
// ╠══════════════════════════════════════════════════════╣
// ║  核心重點：                                           ║
// ║  1. 並發是設計概念，並行是執行現象                   ║
// ║  2. 多執行緒：效能、響應性、資源利用                 ║
// ║  3. 執行緒共享程序記憶體，需謹慎同步                 ║
// ║  4. 競爭條件→mutex/atomic，死結→固定鎖順序           ║
// ║  5. C++11 開始標準化，C++20 大幅增強                ║
// ║  6. 六大標頭檔：thread/mutex/cv/future/atomic/...    ║
// ╚══════════════════════════════════════════════════════╝
