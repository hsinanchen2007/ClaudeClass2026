// =============================================================================
//  課程 1.2：為什麼需要多執行緒 — 第 2 部分：響應性改善（Responsiveness）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>、<atomic>、<chrono>、<string>
//   標準版本：C++11（std::thread 與 std::atomic 皆為 C++11 引入）
//
//     std::atomic<bool> flag{false};
//     flag.store(true);                    // 預設 memory_order_seq_cst
//     bool v = flag.load();                // 預設 memory_order_seq_cst
//     flag.store(true, std::memory_order_release);
//     bool v = flag.load(std::memory_order_acquire);
//
//   複雜度：atomic 的 load/store 在 x86-64 上對齊的 4/8 byte 型別是單一指令；
//           seq_cst 的 store 需要額外的記憶體屏障（x86 上是 XCHG 或 MFENCE）。
//
// 【詳細解釋 Explanation】
//
// 【1. 響應性與吞吐量是兩件不同的事】
//   第 1 部分談的是【吞吐量／效能】：同樣的工作用更短的總時間做完。
//   本檔談的是【響應性／延遲】：程式對使用者操作的反應有多快。
//   兩者可以完全獨立：
//     * 把下載搬到背景執行緒，總下載時間【一點也沒變快】，
//       但 UI 從「凍結 5 秒」變成「全程可互動」——
//       使用者感受到的品質天差地遠，而吞吐量毫無改善。
//   這是多執行緒的第二個核心價值，在桌面應用、行動 App、
//   遊戲與任何有互動介面的系統上，它的重要性往往高於吞吐量。
//
//   本檔情境一（阻塞式）與情境二（多執行緒）的總時間幾乎相同，
//   差別只在「這 5 秒之內使用者能不能做別的事」。
//
// 【2. 為什麼一定要用 std::atomic 而不是普通 bool】
//   下載執行緒寫 downloadComplete、UI 執行緒讀它 —— 這是典型的
//   「一寫一讀共享變數」。若用普通 bool，就是 data race，
//   而 data race 在 C++ 中是【未定義行為】，不是「偶爾讀到舊值」而已。
//   實際會發生的災難有兩種：
//     (a) 編譯器最佳化：看到 while (!done) 迴圈內沒有修改 done，
//         編譯器可以合法地把它提到迴圈外只讀一次，變成
//         if (!done) while (true) {...} —— 無窮迴圈，而且【完全合法】。
//     (b) CPU 快取／重排：即使編譯器沒動手腳，寫入可能停在另一核心的
//         store buffer 裡，讀取端遲遲看不到。
//   std::atomic 同時解決這兩層：它禁止編譯器把讀取提出迴圈，
//   也在需要時插入記憶體屏障確保跨核心可見性。
//
//   ⚠️ 常見誤解：用 volatile 可以解決。【不行】。
//      C++ 的 volatile 只承諾「不省略此讀寫、不與其他 volatile 重排」，
//      它【不提供】原子性、跨執行緒可見性、也不建立 happens-before 關係。
//      volatile 是給 MMIO（記憶體映射 I/O）用的，不是同步原語。
//      Java/C# 的 volatile 語意比 C++ 強（含 acquire/release），
//      這是跨語言遷移時最常見的誤植來源。
//
// 【3. memory_order：本檔為什麼用預設的 seq_cst 就夠】
//   atomic 操作預設是 std::memory_order_seq_cst（循序一致），
//   它保證所有執行緒看到的所有 seq_cst 操作有一個【一致的全域順序】，
//   最直觀也最保險，但在某些架構上成本最高。
//   本檔的旗標更新頻率極低（每 500ms 一次），
//   seq_cst 的額外成本完全可以忽略，優先選擇「最不容易寫錯」的預設值。
//
//   若是高頻的生產者-消費者，才值得放寬成 acquire/release：
//       producer:  data = 42;  ready.store(true, std::memory_order_release);
//       consumer:  if (ready.load(std::memory_order_acquire)) use(data);
//   release/acquire 配對建立了 happens-before：release 之前的所有寫入，
//   對「看到該值的 acquire」之後的讀取都可見。
//   ⚠️ 注意 memory_order_relaxed 只保證【該變數本身】的原子性，
//      不建立任何跨變數的順序關係，用來做旗標同步是錯的。
//
// 【4. 協作式取消：cancelRequested 的設計】
//   情境三示範的取消機制與 C++20 的 stop_token（課程 3.3）本質相同：
//   主執行緒設旗標，工作執行緒【自願】在安全的檢查點查看並收工。
//   關鍵是檢查點的位置 —— 本檔放在「每個下載區塊的邊界」，
//   所以取消後不會留下寫到一半的區塊。
//   ⚠️ 這也代表取消【不是立即的】：旗標設下後，最壞要等當前這個
//      500ms 的 sleep 結束才會被看到。輸出中「使用者點擊取消」到
//      「停止下載」之間的延遲就是這個原因，不是 bug。
//
// 【5. 為什麼不能用強制中止】
//   沒有 std::thread::kill()，這是刻意的。強制中止會在任意指令邊界
//   砍掉執行緒，導致它持有的鎖不解、配置的記憶體不放、解構子不執行、
//   檔案寫到一半。Java 的 Thread.stop() 與 POSIX 的 pthread_cancel()
//   都因為這些問題被視為設計失誤。協作式取消是唯一安全的做法。
//
// 【概念補充 Concept Deep Dive】
//   * atomic 不一定是無鎖的：std::atomic<T> 對過大或非平凡的 T
//     會退化成內部加鎖實作。用 is_lock_free() 查詢（本檔會實際印出）。
//     bool / int / 指標在主流平台上都是無鎖的。
//     ⚠️ is_lock_free() 的結果是【實作定義】的，不可跨平台假設。
//
//   * atomic<bool> 的大小：通常等同 bool（1 byte），但對齊要求可能更嚴格。
//     這是實作定義的，本檔會印出實測值。
//
//   * 為什麼 UI 執行緒要 sleep 50ms：真實的 UI 執行緒是阻塞在
//     事件佇列上（等待作業系統送來滑鼠／鍵盤事件），不是忙碌輪詢。
//     這裡用 sleep 模擬「處理完事件後等待下一個事件」的空檔。
//     ⚠️ 若真的寫成不 sleep 的忙碌輪詢（busy-wait），會吃滿一整顆核心，
//        反而拖慢背景工作 —— 這是新手實作進度條時的經典錯誤。
//
//   * 本檔的 UI 更新次數為什麼每次都不同：它取決於 UI 迴圈實際跑了幾圈，
//     而每圈的 sleep(50ms) 只保證【至少】睡 50ms，實際受排程器影響。
//     所以這個數字是非決定性的，不可拿來做斷言。
//
// 【注意事項 Pay Attention】
//   1. 跨執行緒共享的旗標【必須】用 std::atomic。普通 bool 是 data race（UB），
//      編譯器可以合法地把 while (!done) 最佳化成無窮迴圈。
//   2. volatile 【不是】同步原語，不能取代 atomic。
//   3. 協作式取消【不是立即的】，延遲取決於檢查點的間隔。
//   4. 所有時間數字、UI 更新次數、輸出交錯順序【每次執行都不同】。
//   5. std::cout 保證無 data race，但【不保證整行原子性】；
//      本檔多條執行緒同時輸出時可能交錯，這是預期現象。
//   6. 本檔總執行時間約 9 秒（2.5 + 5 + 1.5 秒的模擬等待）。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】響應性與 atomic 旗標
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 用一個普通的 bool 當作跨執行緒的結束旗標，會有什麼問題？
//     答：那是 data race，屬於未定義行為。最典型的災難是編譯器最佳化：
//         while (!done) { ... } 迴圈內若沒有修改 done，編譯器可以合法地
//         把讀取提到迴圈外只讀一次，程式就變成無窮迴圈 —— 而且這個
//         最佳化完全符合標準。此外還有 CPU 快取與重排造成的可見性問題。
//         必須改用 std::atomic<bool>。
//     追問：那加 volatile 可以嗎？
//         → 不行。C++ 的 volatile 只承諾不省略讀寫、不與其他 volatile 重排，
//           它不提供原子性、不保證跨核心可見性、也不建立 happens-before。
//           它是給 MMIO 用的。Java/C# 的 volatile 有 acquire/release 語意，
//           跟 C++ 不同，這是跨語言遷移最常見的誤植。
//
// 🔥 Q2. 響應性（responsiveness）和吞吐量（throughput）差在哪？
//     答：吞吐量是「單位時間能完成多少工作」，響應性是「對使用者操作
//         多快做出反應」。兩者可以完全獨立 —— 本檔把下載搬到背景執行緒後，
//         總下載時間【一點也沒變快】（吞吐量不變），但 UI 從凍結 5 秒
//         變成全程可互動（響應性大幅改善）。在有互動介面的系統上，
//         響應性往往比吞吐量更決定使用者的感受。
//     追問：所以單核心機器上做多執行緒有意義嗎？
//         → 非常有意義。單核心無法提升吞吐量，但透過時間分片仍能讓
//           UI 保持響應；而且 I/O 密集任務在等待時會讓出 CPU，
//           單核心上照樣能得到顯著的整體加速（見第 3 部分）。
//
// ⚠️ 陷阱. 既然 atomic 保證了原子性，下面這段為什麼仍然是錯的？
//         if (!cancelRequested.load()) {      // 檢查
//             doExpensiveWork();              // 使用
//         }
//     答：因為這是 check-then-act，屬於【race condition】而非 data race。
//         atomic 保證的是「每一次 load/store 本身不可分割」，
//         但保證不了「檢查」與「使用」之間沒有其他執行緒插入。
//         另一條執行緒完全可能在 load 之後、doExpensiveWork 之前設下取消旗標，
//         於是明明已經取消了，昂貴的工作還是整個做完。
//     為什麼會錯：把 data race 和 race condition 混為一談。
//         * data race      —— 無同步的並行存取，是【未定義行為】，atomic 能消除。
//         * race condition —— 邏輯依賴時序的漏洞，即使完全沒有 data race
//                             也可能存在，atomic【消除不了】。
//         本檔的用法之所以正確，是因為檢查點被放在每個區塊的邊界、
//         且「多做一個區塊才停」是可接受的語意；
//         若要求嚴格的「檢查後立即停」，就得靠更細的檢查粒度，
//         或用 mutex 把檢查與動作framed成一個不可分割的臨界區。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔主題是「把耗時工作移出主執行緒以保持介面響應」，
//   本質上是使用者體驗與系統結構的議題，衡量標準是延遲感受。
//   LeetCode 並行題（1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded）
//   全部是「強制多執行緒依特定順序輸出」的同步練習，
//   既沒有互動介面，也沒有響應性的概念，兩者沒有真實交集。故從缺。
//   （atomic 旗標真正對應的 LeetCode 場景要到「協作式取消」，
//     而 LeetCode 並行題不涉及取消。）

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

// 原子變數用於執行緒間的安全通訊
std::atomic<bool> downloadComplete{false};
std::atomic<int> downloadProgress{0};
std::atomic<bool> cancelRequested{false};

// 模擬一個耗時的下載操作
void simulateDownload(const std::string& filename) {
    std::cout << "[下載執行緒] 開始下載: " << filename << std::endl;

    const int totalSteps = 10;
    for (int i = 1; i <= totalSteps; ++i) {
        // 檢查是否收到取消請求（檢查點放在區塊邊界，不會留下半個區塊）
        if (cancelRequested.load()) {
            std::cout << "[下載執行緒] 收到取消請求，停止下載" << std::endl;
            return;
        }

        // 模擬下載一個區塊
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 更新進度
        downloadProgress.store(i * 100 / totalSteps);
        std::cout << "[下載執行緒] 進度: " << downloadProgress.load() << "%" << std::endl;
    }

    downloadComplete.store(true);
    std::cout << "[下載執行緒] 下載完成！" << std::endl;
}

// 模擬使用者介面執行緒
void simulateUIThread() {
    std::cout << "[UI 執行緒] 使用者介面已啟動" << std::endl;

    int lastProgress = -1;
    int uiUpdateCount = 0;

    while (!downloadComplete.load()) {
        // 模擬 UI 更新（例如動畫、時鐘更新等）
        ++uiUpdateCount;

        // 如果進度有變化，顯示更新
        int currentProgress = downloadProgress.load();
        if (currentProgress != lastProgress) {
            std::cout << "[UI 執行緒] 更新進度條顯示: "
                      << currentProgress << "%" << std::endl;
            lastProgress = currentProgress;
        }

        // 模擬其他 UI 工作
        if (uiUpdateCount % 10 == 0) {
            std::cout << "[UI 執行緒] 處理使用者輸入事件 #"
                      << uiUpdateCount / 10 << std::endl;
        }

        // UI 執行緒不應該佔用過多 CPU
        // ⚠️ 真實 UI 是阻塞在事件佇列上；這裡用 sleep 模擬等待下一個事件的空檔。
        //    若寫成不 sleep 的忙碌輪詢，會吃滿一整顆核心並拖慢背景工作。
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[UI 執行緒] 下載完成，更新介面狀態" << std::endl;
    std::cout << "[UI 執行緒] 共處理了 " << uiUpdateCount << " 次 UI 更新" << std::endl;
}

void demonstrateBlockingUI() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境一】單執行緒（阻塞式）" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "模擬在單執行緒中執行下載..." << std::endl;
    std::cout << "(注意：下載期間 UI 完全無法回應)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    auto start = std::chrono::steady_clock::now();

    // 模擬阻塞式下載
    std::cout << "[主執行緒] 開始下載..." << std::endl;
    for (int i = 1; i <= 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "[主執行緒] 下載進度: " << i * 20 << "%" << std::endl;
        // 在這期間，UI 完全凍結！
    }
    std::cout << "[主執行緒] 下載完成！" << std::endl;
    std::cout << "[主執行緒] ⚠️ 在下載的 2.5 秒內，UI 完全無法回應！" << std::endl;

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    std::cout << "總時間: " << duration << " 毫秒" << std::endl;
}

void demonstrateResponsiveUI() {
    // 重設狀態
    downloadComplete.store(false);
    downloadProgress.store(0);
    cancelRequested.store(false);

    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境二】多執行緒（響應式）" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "使用獨立執行緒進行下載，UI 保持響應..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    auto start = std::chrono::steady_clock::now();

    // 在獨立執行緒中執行下載
    std::thread downloadThread(simulateDownload, "large_file.zip");

    // 主執行緒繼續處理 UI
    simulateUIThread();

    // 等待下載執行緒完成
    downloadThread.join();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "總時間: " << duration << " 毫秒" << std::endl;
    std::cout << "✓ 在下載期間，UI 持續保持響應！" << std::endl;
    std::cout << "（注意：總時間與情境一相當 —— 吞吐量沒有變快，"
                 "改善的是【響應性】）" << std::endl;
}

void demonstrateCancellation() {
    // 重設狀態
    downloadComplete.store(false);
    downloadProgress.store(0);
    cancelRequested.store(false);

    std::cout << "\n========================================" << std::endl;
    std::cout << "【情境三】支援取消操作" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "示範使用者可以在下載中途取消操作..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 在獨立執行緒中執行下載
    std::thread downloadThread(simulateDownload, "another_file.zip");

    // 主執行緒等待一段時間後發送取消請求
    std::cout << "[主執行緒] 等待 1.5 秒後將發送取消請求..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    std::cout << "[主執行緒] 使用者點擊了取消按鈕！" << std::endl;
    cancelRequested.store(true);

    // 等待下載執行緒結束
    downloadThread.join();

    std::cout << "----------------------------------------" << std::endl;
    std::cout << "✓ 下載已被成功取消！" << std::endl;
    std::cout << "（注意：取消【不是立即的】—— 工作執行緒要等當前這個 500ms "
                 "區塊結束後，回到檢查點才會發現）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】atomic 的實作特性（實作定義的數值）
// -----------------------------------------------------------------------------
void demoAtomicProperties() {
    std::atomic<bool> ab{false};
    std::atomic<int> ai{0};
    std::atomic<long long> all{0};

    std::cout << "  sizeof(bool)              = " << sizeof(bool) << std::endl;
    std::cout << "  sizeof(std::atomic<bool>) = " << sizeof(ab) << std::endl;
    std::cout << "  atomic<bool>      無鎖? " << std::boolalpha
              << ab.is_lock_free() << std::endl;
    std::cout << "  atomic<int>       無鎖? " << std::boolalpha
              << ai.is_lock_free() << std::endl;
    std::cout << "  atomic<long long> 無鎖? " << std::boolalpha
              << all.is_lock_free() << std::endl;
    std::cout << "  （以上全為【實作定義】的數值，不同平台/編譯器可能不同）"
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】編輯器的背景自動存檔（autosave）
//   情境：文字編輯器／IDE 必須在使用者持續打字的同時，定期把內容寫進
//         暫存檔以防當機遺失。存檔牽涉磁碟 I/O，可能耗時數十到數百毫秒；
//         若在主執行緒做，使用者打字就會卡頓 —— 這是編輯器最不能忍受的體驗。
//   為何用背景執行緒 + atomic：
//     * 存檔搬到背景執行緒 → 打字輸入完全不受影響（響應性）；
//     * dirty 旗標用 atomic：主執行緒（打字時）設為 true，
//       存檔執行緒讀到 true 才真的寫磁碟，沒有變更就跳過，省下無謂 I/O；
//     * 關閉文件時用 stopRequested 旗標請求背景執行緒收工，
//       並在退出前做最後一次存檔 —— 確保使用者最後的編輯不遺失。
//   這正是情境二（響應性）與情境三（協作式取消）在真實產品中的組合應用。
// -----------------------------------------------------------------------------
class AutoSaver {
public:
    // 主執行緒（UI）在每次按鍵時呼叫
    void onTextChanged(const std::string& text) {
        {
            // ⚠️ content_ 是 std::string，由 UI 執行緒寫、存檔執行緒讀。
            //    atomic 只能保護 dirty_ 這個旗標，【保護不了字串本身】——
            //    std::string 不是 atomic 型別，並行讀寫就是 data race（UB）。
            //    所以這裡必須用 mutex。這是初學者最常見的誤解：
            //    「我已經用 atomic 了」只代表旗標安全，不代表它保護的資料安全。
            std::lock_guard<std::mutex> lock(mtx_);
            content_ = text;
        }
        dirty_.store(true);           // 標記「有未存檔的變更」
    }

    void run(int intervalMs) {
        while (!stopRequested_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

            if (dirty_.exchange(false)) {   // 讀取並同時清除，一步完成
                doSave();
            } else {
                std::cout << "    [autosave] 無變更，跳過本次存檔" << std::endl;
            }
        }
        // 收工前做最後一次存檔，確保最後的編輯不遺失
        if (dirty_.load()) {
            std::cout << "    [autosave] 關閉前執行最後一次存檔" << std::endl;
            doSave();
        }
        std::cout << "    [autosave] 背景存檔執行緒結束（共存檔 "
                  << saveCount_ << " 次）" << std::endl;
    }

    void requestStop() { stopRequested_.store(true); }
    int saveCount() const { return saveCount_; }

private:
    void doSave() {
        // 先在鎖內取一份快照，再放開鎖去做慢速 I/O ——
        // 這樣打字的 UI 執行緒不會被 40ms 的磁碟寫入卡住（持鎖時間最小化）。
        std::string snapshot;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            snapshot = content_;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(40));  // 模擬磁碟 I/O
        ++saveCount_;   // 只有本執行緒寫，不需保護
        std::cout << "    [autosave] 已存檔（" << snapshot.size()
                  << " 字元）" << std::endl;
    }

    std::mutex mtx_;              // 保護 content_
    std::string content_;
    std::atomic<bool> dirty_{false};
    std::atomic<bool> stopRequested_{false};
    int saveCount_ = 0;
};

void demoAutoSave() {
    AutoSaver saver;
    std::thread bg(&AutoSaver::run, &saver, 100);

    // 模擬使用者打字：主執行緒全程保持響應，不會被存檔的磁碟 I/O 卡住
    const char* keystrokes[] = {"H", "He", "Hel", "Hell", "Hello",
                                "Hello ", "Hello w", "Hello wo"};
    for (const char* s : keystrokes) {
        saver.onTextChanged(s);
        std::cout << "    [UI] 使用者輸入 → \"" << s << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(45));
    }

    std::cout << "    [UI] 使用者停止打字，等待一段時間（觀察跳過存檔）" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    std::cout << "    [UI] 關閉文件" << std::endl;
    saver.requestStop();
    bg.join();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    多執行緒響應性改善示範" << std::endl;
    std::cout << "========================================" << std::endl;

    // 情境一：展示阻塞式 UI 的問題
    demonstrateBlockingUI();

    // 情境二：展示響應式 UI 的優勢
    demonstrateResponsiveUI();

    // 情境三：展示取消操作的能力
    demonstrateCancellation();

    std::cout << "\n========================================" << std::endl;
    std::cout << "atomic 的實作特性（實作定義）" << std::endl;
    std::cout << "========================================" << std::endl;
    demoAtomicProperties();

    std::cout << "\n========================================" << std::endl;
    std::cout << "日常實務：編輯器的背景自動存檔" << std::endl;
    std::cout << "========================================" << std::endl;
    demoAutoSave();

    std::cout << "\n========================================" << std::endl;
    std::cout << "示範結束" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.2：為什麼需要多執行緒2.cpp" -o resp2
//   本檔只用到 C++11 起就有的功能，以 -std=c++17 -pedantic-errors 驗證通過。
//   本檔總執行時間約 9 秒（含 2.5 + 5 + 1.5 秒的模擬等待）。

// 註:
//   ⚠️ 以下內容有大量【非決定性】成分，每次執行都不同：
//   * 所有毫秒數；
//   * 「UI 執行緒」與「下載執行緒」兩者輸出的【交錯順序】；
//   * UI 更新次數、處理使用者輸入事件的編號上限；
//   * 自動存檔的次數與「跳過存檔」出現的位置；
//   * sizeof / is_lock_free 為【實作定義】的數值。
//   穩定不變的是【結構】：情境一總時間 ≈ 情境二總時間（吞吐量沒變快），
//   情境三會在收到取消後的下一個檢查點停止。
//   以下為本機（16 邏輯核心）某一次的實際執行結果（中間重複的進度行以 ... 省略）。

// === 預期輸出 ===
// ========================================
//     多執行緒響應性改善示範
// ========================================
//
// ========================================
// 【情境一】單執行緒（阻塞式）
// ========================================
// 模擬在單執行緒中執行下載...
// (注意：下載期間 UI 完全無法回應)
// ----------------------------------------
// [主執行緒] 開始下載...
// [主執行緒] 下載進度: 20%
// [主執行緒] 下載進度: 40%
// [主執行緒] 下載進度: 60%
// [主執行緒] 下載進度: 80%
// [主執行緒] 下載進度: 100%
// [主執行緒] 下載完成！
// [主執行緒] ⚠️ 在下載的 2.5 秒內，UI 完全無法回應！
// 總時間: 2500 毫秒
//
// ========================================
// 【情境二】多執行緒（響應式）
// ========================================
// 使用獨立執行緒進行下載，UI 保持響應...
// ----------------------------------------
// [UI 執行緒] 使用者介面已啟動
// [UI 執行緒] 更新進度條顯示: 0%
// [下載執行緒] 開始下載: large_file.zip
// [UI 執行緒] 處理使用者輸入事件 #1
// [下載執行緒] 進度: 10%
// [UI 執行緒] 更新進度條顯示: 10%
// [UI 執行緒] 處理使用者輸入事件 #2
// [下載執行緒] 進度: 20%
// [UI 執行緒] 更新進度條顯示: 20%
// [UI 執行緒] 處理使用者輸入事件 #3
// [下載執行緒] 進度: 30%
// [UI 執行緒] 更新進度條顯示: 30%
// [UI 執行緒] 處理使用者輸入事件 #4
// [下載執行緒] 進度: 40%
// [UI 執行緒] 更新進度條顯示: 40%
// [UI 執行緒] 處理使用者輸入事件 #5
// [下載執行緒] 進度: 50%
// [UI 執行緒] 更新進度條顯示: 50%
// [UI 執行緒] 處理使用者輸入事件 #6
// [下載執行緒] 進度: 60%
// [UI 執行緒] 更新進度條顯示: 60%
// [UI 執行緒] 處理使用者輸入事件 #7
// [下載執行緒] 進度: 70%
// [UI 執行緒] 更新進度條顯示: 70%
// [UI 執行緒] 處理使用者輸入事件 #8
// [下載執行緒] 進度: 80%
// [UI 執行緒] 更新進度條顯示: 80%
// [UI 執行緒] 處理使用者輸入事件 #9
// [下載執行緒] 進度: 90%
// [UI 執行緒] 更新進度條顯示: 90%
// [UI 執行緒] 處理使用者輸入事件 #10
// [下載執行緒] 進度: 100%
// [下載執行緒] 下載完成！
// [UI 執行緒] 下載完成，更新介面狀態
// [UI 執行緒] 共處理了 100 次 UI 更新
// ----------------------------------------
// 總時間: 5013 毫秒
// ✓ 在下載期間，UI 持續保持響應！
// （注意：總時間與情境一相當 —— 吞吐量沒有變快，改善的是【響應性】）
//
// ========================================
// 【情境三】支援取消操作
// ========================================
// 示範使用者可以在下載中途取消操作...
// ----------------------------------------
// [主執行緒] 等待 1.5 秒後將發送取消請求...
// [下載執行緒] 開始下載: another_file.zip
// [下載執行緒] 進度: 10%
// [下載執行緒] 進度: 20%
// [主執行緒] 使用者點擊了取消按鈕！
// [下載執行緒] 進度: 30%
// [下載執行緒] 收到取消請求，停止下載
// ----------------------------------------
// ✓ 下載已被成功取消！
// （注意：取消【不是立即的】—— 工作執行緒要等當前這個 500ms 區塊結束後，回到檢查點才會發現）
//
// ========================================
// atomic 的實作特性（實作定義）
// ========================================
//   sizeof(bool)              = 1
//   sizeof(std::atomic<bool>) = 1
//   atomic<bool>      無鎖? true
//   atomic<int>       無鎖? true
//   atomic<long long> 無鎖? true
//   （以上全為【實作定義】的數值，不同平台/編譯器可能不同）
//
// ========================================
// 日常實務：編輯器的背景自動存檔
// ========================================
//     [UI] 使用者輸入 → "H"
//     [UI] 使用者輸入 → "He"
//     [UI] 使用者輸入 → "Hel"
//     [UI] 使用者輸入 → "Hell"
//     [autosave] 已存檔（3 字元）
//     [UI] 使用者輸入 → "Hello"
//     [UI] 使用者輸入 → "Hello "
//     [UI] 使用者輸入 → "Hello w"
//     [autosave] 已存檔（6 字元）
//     [UI] 使用者輸入 → "Hello wo"
//     [UI] 使用者停止打字，等待一段時間（觀察跳過存檔）
//     [autosave] 已存檔（8 字元）
//     [autosave] 無變更，跳過本次存檔
//     [UI] 關閉文件
//     [autosave] 無變更，跳過本次存檔
//     [autosave] 背景存檔執行緒結束（共存檔 3 次）
//
// ========================================
// 示範結束
// ========================================
