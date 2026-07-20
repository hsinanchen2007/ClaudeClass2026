// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定1.cpp  —  try_lock() 的基本使用方式
// =============================================================================
//
// 【主題資訊 Information】
//   bool std::mutex::try_lock();                                    // C++11，<mutex>
//   回傳值：true  = 已取得鎖（呼叫端【有責任】在用完後 unlock()）
//           false = 沒取得，立即返回，【不會】阻塞
//   複雜度：一次原子的 compare-and-swap，不進核心，成本與無競爭的 lock() 相當。
//   ⚠️ 標準允許 try_lock() 【偽失敗】（spurious failure）：
//      即使鎖當下沒有被任何人持有，它仍【被允許】回傳 false。
//
// 【詳細解釋 Explanation】
//
// 【1. try_lock() 的存在意義：把「等待」變成一個可以選擇的選項】
//   lock() 的語意是「我一定要拿到，拿不到就在這裡等」。
//   但很多真實情境裡，「等」是最糟的選擇：
//     * GUI 的繪製執行緒：寧可這一幀畫舊資料，也不能卡住讓畫面凍結；
//     * 監控/統計的取樣：這次取不到就跳過，下次再取，絕不能拖慢主流程；
//     * 有替代路徑可走：拿不到快取鎖就直接去問資料庫。
//   try_lock() 把「取得鎖」從一個阻塞操作變成一個【查詢】，
//   讓呼叫端能自己決定「拿不到的話要做什麼」。
//
// 【2. 成功之後的責任完全等同 lock()】
//   try_lock() 回傳 true 就代表【你持有這把鎖】，
//   跟 lock() 成功之後沒有任何差別——包括「必須 unlock()」這個義務。
//   這裡有個特別容易漏的陷阱：因為 try_lock 常寫在 if 裡面，
//   而 if 區塊裡往往有 return / break / continue，
//   一不小心就會有某條路徑沒解鎖。
//   → 正解是用 std::unique_lock(mtx, std::try_to_lock)，
//     讓 RAII 接手（本課後面的檔案會專門示範）。
//
// 【3. 「偽失敗」是標準明文允許的，不是實作的 bug】
//   標準對 try_lock 的規定是：「允許失敗並回傳 false，即使 mutex
//   目前並未被任何執行緒鎖住」。
//   原因跟 compare_exchange_weak 一樣——在 LL/SC 架構上，
//   store-conditional 可能因為中斷或無關的快取行活動而失敗，
//   要求實作必須內部重試會讓 try_lock 失去「絕不阻塞」的保證。
//   → 實務結論：【永遠不要】用 try_lock() 的回傳值去推論
//     「鎖現在有沒有被持有」。它只能回答「這一次我拿到了沒有」。
//   想做「偵測是否有人持有」的邏輯，代表設計本身就錯了。
//
// 【4. try_lock 不解決死結，只是給你逃生門】
//   常見的誤解是「用 try_lock 就不會死結」。
//   準確的說法是：try_lock 讓你有機會【偵測到搶不到】並主動退讓，
//   但退讓的策略要自己寫對，否則會從死結變成【活鎖】（livelock）——
//   兩條執行緒不斷地「拿到 A → 搶不到 B → 放掉 A → 重試」，
//   誰都沒卡住（不是死結），卻誰也沒進展。
//   解法是退讓時加入隨機退避（本課後面的檔案會示範）。
//
// 【概念補充 Concept Deep Dive】
//   * try_lock() 與 lock() 的成本在【無競爭】時幾乎相同——
//     兩者都只是一次原子操作。差別完全在「失敗時做什麼」：
//     lock() 進入等待（可能自旋後睡眠），try_lock() 直接返回 false。
//   * 這也是為什麼「用 try_lock 迴圈模擬 lock」是個壞主意：
//       while (!mtx.try_lock()) { }     // ← 別這樣寫
//     它把一個會讓出 CPU 的等待，變成了燒 CPU 的自旋，
//     而且失去了作業系統的 futex 排隊機制，還可能因為不公平而餓死某條執行緒。
//   * C++11 同時提供了 std::timed_mutex::try_lock_for / try_lock_until，
//     那才是「等一段時間、逾時就放棄」的正規工具，
//     不需要自己用 try_lock 加迴圈土炮（本課後面的檔案會對照）。
//   * 本檔加上了一道「同時起跑」的 barrier。若不加，三條執行緒
//     可能自然錯開（建立執行緒本身就要數十微秒），
//     第一條早就做完並解鎖了，後面兩條也各自成功 —— 那就完全看不到
//     try_lock 失敗的情境。加了 barrier 才能穩定重現要示範的行為。
//
// 【注意事項 Pay Attention】
//   1. try_lock() 回傳 true 之後【必須】unlock()，責任與 lock() 完全相同。
//   2. 標準允許【偽失敗】：false 不代表「鎖被別人持有」。
//   3. 【不可】用 try_lock 的結果來判斷鎖的狀態或做任何推論。
//   4. 【不要】寫 while (!try_lock()) 來模擬 lock()——那是在燒 CPU。
//   5. 對已被【自己】持有的 std::mutex 呼叫 try_lock() 是【未定義行為】
//      （與重複 lock() 相同，std::mutex 不可重入）。
//   6. try_lock 不會自動避免死結，退讓策略寫錯會變成活鎖。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】try_lock()
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. try_lock() 回傳 false，是不是代表這把鎖正被別的執行緒持有？
//     答：不能這樣推論。標準【明文允許】try_lock() 偽失敗——
//         即使鎖當下沒有任何人持有，它仍被允許回傳 false
//         （原因與 compare_exchange_weak 相同：LL/SC 架構上的
//          store-conditional 可能因中斷或快取活動而失敗）。
//         try_lock 只能回答「這一次我有沒有拿到」，
//         不能回答「鎖現在是什麼狀態」。
//     追問：那要怎麼寫「偵測鎖是否被持有」的邏輯？→ 不要寫。
//           有這種需求通常代表設計有問題；狀態應該由被保護的資料自己表達。
//
// 🔥 Q2. 用 while (!mtx.try_lock()) {} 取代 mtx.lock()，有什麼問題？
//     答：三個問題。(1) 把「阻塞等待」變成「燒 CPU 自旋」，
//         在等待較久或執行緒數超過核心數時會嚴重拖垮整台機器；
//         (2) 繞過了作業系統的 futex 等待佇列，失去排隊與喚醒機制；
//         (3) 純自旋沒有任何公平性，某條執行緒可能長期搶不到而被餓死。
//     追問：那如果我就是想「等一段時間就放棄」呢？
//           → 用 std::timed_mutex 的 try_lock_for() / try_lock_until()，
//             那是標準為此設計的工具，內部會正確地睡眠而非空轉。
//
// ⚠️ 陷阱. if (mtx.try_lock()) { ... if (err) return; ... mtx.unlock(); }
//        —— 這段程式碼有什麼問題？
//     答：中間那條 return 會跳過 mtx.unlock()，鎖從此永遠不會被釋放。
//         之後所有對這把鎖呼叫 lock() 的執行緒都會【永久阻塞】
//         （注意：這是標準保證的阻塞，不是 UB）。
//     為什麼會錯：try_lock 常被寫在 if 條件裡，讓人下意識覺得
//         「這是一段輕量的嘗試」，而忘記它一旦成功，
//         責任就跟 lock() 一模一樣。
//         正解是 std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
//         再用 lock.owns_lock() 判斷——所有離開路徑都由 RAII 保證解鎖。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）都要求
//   「一定要正確輸出全部結果」，沒有任何一題允許「這次拿不到就放棄」。
//   try_lock 的價值恰恰在於【可以放棄】——這個語意在那些題目裡
//   不但用不上，用了還會直接答錯（漏輸出）。
//   硬掛一題會傳達完全錯誤的使用時機，故從缺。
//   （本檔改以「最佳努力取樣」的實務範例呈現 try_lock 的正確用途。）

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;

// 為了讓示範可驗證，三條執行緒的結果先寫進各自的槽位，最後由主執行緒統一輸出。
// （直接在執行緒裡 cout 會交錯，且順序每次不同。）
struct WorkerResult {
    bool        acquired = false;
    std::string message;
};

std::vector<WorkerResult> results(3);
std::atomic<int> arrived{0};

void worker(int id) {
    // 同時起跑：確保三條執行緒真的在同一瞬間搶鎖。
    // 若沒有這道 barrier，建立執行緒的開銷（數十微秒）就足以讓它們自然錯開，
    // 第一條早已完成並解鎖，後面兩條也會各自成功，就看不到 try_lock 失敗了。
    arrived.fetch_add(1, std::memory_order_acq_rel);
    while (arrived.load(std::memory_order_acquire) < 3) {
        std::this_thread::yield();
    }

    // 嘗試獲取鎖
    if (mtx.try_lock()) {
        // 成功獲取鎖
        results[id - 1].acquired = true;
        results[id - 1].message  = "成功獲取鎖，開始工作...";

        // 模擬工作（持有 100ms，確保另外兩條一定撲空）
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        results[id - 1].message += " → 工作完成，釋放鎖";
        mtx.unlock();  // 記得解鎖！
    } else {
        // 獲取失敗，立即返回
        results[id - 1].acquired = false;
        results[id - 1].message  = "無法獲取鎖，執行其他任務";
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】最佳努力的指標取樣（best-effort metrics sampling）
//   情境：高吞吐的交易系統，每筆交易處理完想順手更新一份統計資料
//         （總筆數、最大金額、各幣別筆數…）。
//   關鍵取捨：統計【不是】業務正確性的一部分，少算幾筆完全可以接受；
//         但絕對不能因為統計而拖慢交易主流程。
//   → 正是 try_lock 的教科書用途：拿得到就更新，拿不到就跳過，
//     並記錄「跳過了幾次」讓維運知道取樣覆蓋率。
//   ⚠️ 對照組：如果這裡改用 lock()，高競爭時每筆交易都要排隊等統計鎖，
//      統計反而成了整個系統的瓶頸——這是真實系統中很常見的效能事故。
// -----------------------------------------------------------------------------
class BestEffortMetrics {
private:
    std::mutex mtx_;
    long  samples_ = 0;
    long  maxAmount_ = 0;
    std::atomic<long> skipped_{0};   // 因為鎖忙碌而跳過的次數

public:
    // 回傳 true 表示這次成功取樣
    bool tryRecord(long amount) {
        if (!mtx_.try_lock()) {
            skipped_.fetch_add(1, std::memory_order_relaxed);
            return false;            // 拿不到就放棄，主流程繼續跑
        }

        // ⚠️ 從這裡到 unlock 之間絕不可以有 return / throw，
        //    否則鎖就洩漏了。這段刻意寫得極短，只有兩行。
        ++samples_;
        if (amount > maxAmount_) maxAmount_ = amount;
        mtx_.unlock();
        return true;
    }

    long samples()   { std::lock_guard<std::mutex> lk(mtx_); return samples_; }
    long maxAmount() { std::lock_guard<std::mutex> lk(mtx_); return maxAmount_; }
    long skipped()   { return skipped_.load(); }
};

int main() {
    std::cout << "=== 課程示範: 三條執行緒同時 try_lock ===" << std::endl;
    {
        std::thread t1(worker, 1);
        std::thread t2(worker, 2);
        std::thread t3(worker, 3);

        t1.join();
        t2.join();
        t3.join();

        int acquiredCount = 0;
        for (const auto& r : results) {
            if (r.acquired) ++acquiredCount;
        }

        std::cout << "成功取得鎖的執行緒數: " << acquiredCount
                  << "  (必須恰好 1——贏家持有 100ms，另外兩條必然撲空)" << std::endl;
        std::cout << "try_lock 失敗的執行緒數: " << (3 - acquiredCount)
                  << "  (它們沒有阻塞，立即改做別的事)" << std::endl;
        std::cout << "註: 「哪一條執行緒是贏家」由排程決定，每次執行都不同，"
                     "故不列出 id" << std::endl;
    }

    std::cout << "\n=== try_lock 成功後，責任與 lock() 完全相同 ===" << std::endl;
    {
        std::mutex demo;
        if (demo.try_lock()) {
            std::cout << "第一次 try_lock: 成功（現在我持有這把鎖）" << std::endl;
            demo.unlock();
            std::cout << "已 unlock" << std::endl;
        }
        // 解鎖之後，另一次 try_lock 才可能成功
        if (demo.try_lock()) {
            std::cout << "解鎖後再 try_lock: 成功" << std::endl;
            demo.unlock();
        }
        std::cout << "⚠️ 注意: 對【自己已持有】的 std::mutex 呼叫 try_lock() "
                     "是未定義行為，本檔刻意不示範" << std::endl;
    }

    std::cout << "\n=== 日常實務: 最佳努力的指標取樣 ===" << std::endl;
    {
        BestEffortMetrics metrics;
        const int threads   = 8;
        const int perThread = 20000;

        std::vector<std::thread> workers;
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&metrics, perThread, t]() {
                for (int i = 1; i <= perThread; ++i) {
                    // 模擬「處理一筆交易」——這才是主流程，不能被統計拖累
                    const long amount = static_cast<long>(t + 1) * i;
                    metrics.tryRecord(amount);
                }
            });
        }
        for (auto& t : workers) t.join();

        const long recorded = metrics.samples();
        const long skipped  = metrics.skipped();

        std::cout << "交易總筆數: " << (threads * perThread) << std::endl;
        std::cout << "成功取樣 + 跳過 = " << (recorded + skipped)
                  << "  (必須等於交易總筆數，一筆都不會遺漏統計機會)" << std::endl;
        std::cout << "主流程完全沒有被阻塞: 是（try_lock 失敗即返回）" << std::endl;
        std::cout << "註: 實際「成功取樣」與「跳過」各佔多少完全取決於排程，"
                     "每次執行都不同，故不列出數值" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定1.cpp' -o try_lock_basic

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 「哪一條執行緒搶到鎖」、「成功取樣 vs 跳過的比例」都由排程決定，
//      【每次執行都不同】，故本檔只輸出可穩定驗證的不變量，不印出這些數值。
//   2. 「成功取得鎖的執行緒數 = 1」是穩定的：barrier 確保三條同時起跑，
//      贏家持有鎖 100ms，遠長於另外兩條 try_lock 所需的時間。
//   3. 「成功取樣 + 跳過 = 總筆數」是【確定】的：每一筆交易都恰好
//      走過其中一條路徑。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 三條執行緒同時 try_lock ===
// 成功取得鎖的執行緒數: 1  (必須恰好 1——贏家持有 100ms，另外兩條必然撲空)
// try_lock 失敗的執行緒數: 2  (它們沒有阻塞，立即改做別的事)
// 註: 「哪一條執行緒是贏家」由排程決定，每次執行都不同，故不列出 id
//
// === try_lock 成功後，責任與 lock() 完全相同 ===
// 第一次 try_lock: 成功（現在我持有這把鎖）
// 已 unlock
// 解鎖後再 try_lock: 成功
// ⚠️ 注意: 對【自己已持有】的 std::mutex 呼叫 try_lock() 是未定義行為，本檔刻意不示範
//
// === 日常實務: 最佳努力的指標取樣 ===
// 交易總筆數: 160000
// 成功取樣 + 跳過 = 160000  (必須等於交易總筆數，一筆都不會遺漏統計機會)
// 主流程完全沒有被阻塞: 是（try_lock 失敗即返回）
// 註: 實際「成功取樣」與「跳過」各佔多少完全取決於排程，每次執行都不同，故不列出數值
