// 檔案：lesson_5_4_unlock_unowned.cpp
// 說明：解鎖未鎖定或不屬於自己的互斥鎖
//
// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤8.cpp  —  unlock 一把你沒有持有的鎖
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   wrongUnlock() 與 wrongThread() 都是【未定義行為 (UB)】，
//   所以它們在 main 裡被註解掉。本檔【不會】宣稱「一定會 crash」或
//   「一定會印出某個錯誤訊息」——UB 沒有固定結果。
//   本檔在保留原始錯誤示範的同時，補上「為什麼標準要這樣規定」
//   以及可安全執行的正確作法對照。
//
// 【主題資訊 Information】
//   主題：    std::mutex 的所有權 (ownership) 語意
//   標準規定：[thread.mutex.requirements.mutex] 對 unlock() 的前置條件是
//             「呼叫端執行緒必須【持有】這把 mutex」；違反則為未定義行為
//   標準版本：std::mutex 為 C++11
//   標頭檔：  <mutex>
//   關鍵概念：mutex 有「擁有者」的概念，而 semaphore 沒有 —— 這是兩者的本質差異
//
// 【詳細解釋 Explanation】
//
// 【1. mutex 是「有主人」的鎖】
//   mutex = MUTual EXclusion，它的完整語意是：
//   【誰鎖的，誰才能解】。這條規則叫做 ownership（所有權）。
//   兩個錯誤示範各違反它一次：
//     * wrongUnlock()：根本沒有人鎖過，卻呼叫 unlock() → 違反「必須持有」
//     * wrongThread()：A 執行緒鎖、B 執行緒解 → 違反「誰鎖的誰解」
//   標準把這兩種情形都定義為【未定義行為】，而不是「丟例外」或「回傳錯誤」。
//
// 【2. 為什麼標準選擇 UB 而不是丟例外】
//   關鍵是成本。若要在 unlock() 時檢查「呼叫者是不是持有者」，
//   就必須在鎖裡額外儲存「目前擁有者的執行緒 id」，
//   而且每次 unlock 都要比對一次。
//   對一個「無競爭時只有一次原子 CAS、約 13~25 ns」（本機實測範圍，見 5.6-1）
//   的操作來說，這個檢查是巨大的相對開銷。
//   C++ 的核心原則是 zero-overhead abstraction：
//   【不為你沒用到的東西付費】。所以標準把責任交給使用者，
//   讓正確的程式跑得最快，而不是讓所有人為錯誤的程式付出檢查成本。
//   （對照：std::recursive_mutex 就【必須】記錄擁有者與遞迴計數，
//     所以它比 std::mutex 重，這就是那個成本的具體證據。）
//
// 【3. 為什麼「跨執行緒解鎖」特別危險】
//   從 mutex 的用途想就很清楚：鎖的目的是保護一段臨界區段。
//   若 B 可以解開 A 持有的鎖，那麼 A 還在臨界區段裡執行，
//   保護就已經消失了 —— 第三條執行緒 C 隨時可以進來。
//   這種錯誤比「忘記解鎖」更難查：忘記解鎖會卡住（很明顯），
//   跨執行緒解鎖則是【保護悄悄失效】，程式繼續跑，錯誤在別的地方浮現。
//
// 【4. 那「A 鎖、B 解」的需求該用什麼】
//   如果你真的需要「一條執行緒發出許可、另一條執行緒取用」的語意，
//   那你要的不是 mutex，是【號誌 (semaphore)】：
//     * std::counting_semaphore / std::binary_semaphore（C++20）
//       沒有擁有者概念，任何執行緒都可以 release()。
//     * 或用 std::condition_variable + mutex 表達「等待某個條件成立」。
//   → 選錯同步原語是這類 bug 的根源。
//     判準：要保護「一段程式碼」用 mutex；要傳遞「一個訊號」用 semaphore / cv。
//
// 【5. 正確作法：永遠用 RAII，讓錯誤寫不出來】
//   本檔的兩個錯誤，只要改用 std::lock_guard 就【在語法層面不可能發生】：
//     * lock_guard 一定是「建構時鎖、解構時解」，成對出現。
//     * 它不可複製、不可移動，無法跨執行緒傳遞。
//     * 它是自動儲存期物件，解構必定發生在【建立它的那條執行緒】上。
//   → 這是 C++ 的核心設計哲學：
//     好的介面應該讓錯誤的用法「寫不出來」，而不是「寫得出來但會被檢查」。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 本機 std::mutex 的實作與這兩個 UB 的實際後果
//   Linux/glibc 上 std::mutex 是 pthread_mutex_t 的薄封裝，
//   預設型別 PTHREAD_MUTEX_TIMED_NP（非 error-checking）。
//   對這種 mutex 解一把沒鎖的鎖，glibc 通常不會檢查、直接把狀態改成「未鎖定」，
//   後果是【下一個 lock() 的人與現在的持有者同時進入臨界區段】——
//   保護靜默失效，而不是立刻崩潰。這正是它難查的原因。
//   （但這是本平台的實作行為，不是標準保證；不可據此推論任何「一定會…」。）
//
// (B) 為什麼 unlock() 是 noexcept
//   標準把 unlock() 宣告為 noexcept，正是因為它的前置條件由使用者保證，
//   實作不需要（也不該）在此檢查與拋出。
//   這也反過來說明了：不要期待它會用例外告訴你用錯了。
//
// (C) 除錯手段：把 mutex 換成 error-checking 型別
//   POSIX 提供 PTHREAD_MUTEX_ERRORCHECK，
//   對這兩種誤用會回傳 EPERM 而不是靜默損壞。
//   std::mutex 沒有暴露這個選項，但除錯時可以：
//     * 用 ThreadSanitizer（會報 "unlock of an unlocked mutex"）
//     * 或暫時改用 pthread_mutex_t 並設定 ERRORCHECK 屬性
//
// 【注意事項 Pay Attention】
// 1. unlock() 的前置條件是「呼叫端必須持有」；違反是 UB，不是例外。
// 2. 「誰鎖的誰解」—— 跨執行緒解鎖同樣是 UB，且後果是保護靜默失效。
// 3. 標準選擇 UB 是為了 zero-overhead；別期待它會幫你檢查。
// 4. 需要「A 發訊號、B 取用」請改用 semaphore（C++20）或 condition_variable。
// 5. 一律使用 lock_guard / unique_lock / scoped_lock，
//    讓這兩種錯誤在語法層面就寫不出來。
// 6. 本檔的錯誤函式在 main 中【刻意保持註解】，取消註解會進入 UB。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔講的是 std::mutex 的所有權語意與 API 誤用，屬於語言規則層面；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都是在「正確使用同步原語」的前提下解題，沒有一題在考「解錯鎖」。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （RAII 封裝、以 semaphore 表達跨執行緒許可）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】std::mutex 的所有權語意
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 對一把沒有鎖定的 std::mutex 呼叫 unlock() 會發生什麼事?
//     答：未定義行為。標準規定 unlock() 的前置條件是「呼叫端執行緒必須持有
//         這把 mutex」，違反時標準不再保證任何結果 ——
//         它不會丟例外，也不會回傳錯誤碼（unlock 甚至是 noexcept）。
//     追問：為什麼標準不做檢查、直接丟例外就好?
//         → zero-overhead 原則。要檢查就得儲存擁有者 id 並每次比對，
//           對一個無競爭時只要一次原子 CAS（本機約 13~25 ns）的操作
//           是很大的相對成本。標準選擇讓正確的程式跑最快。
//           std::recursive_mutex 因為語意上必須記錄擁有者與計數，所以比較重。
//
// 🔥 Q2. 可以在 A 執行緒 lock、在 B 執行緒 unlock 嗎?
//     答：不可以，同樣是 UB。mutex 有 ownership 語意：誰鎖的誰解。
//         而且危險的不只是規則本身 —— 若 B 解開了 A 的鎖，
//         A 還在臨界區段裡，保護就已經失效，第三條執行緒隨時可以進來。
//     追問：那如果我真的需要「A 發出許可、B 取用」呢?
//         → 那你要的不是 mutex 而是 semaphore
//           （C++20 的 std::counting_semaphore / binary_semaphore，
//           沒有擁有者概念，任何執行緒都能 release），
//           或用 condition_variable 表達「等待條件成立」。
//           選錯同步原語才是這類 bug 的根源。
//
// ⚠️ 陷阱. 「解一把沒鎖的 mutex，頂多就是沒作用而已吧?」——錯在哪?
//     答：它是 UB，標準不保證任何結果。而本機 glibc 的實際行為更糟：
//         預設的 mutex 型別不做檢查，會直接把狀態改成「未鎖定」——
//         於是【正在臨界區段裡的那條執行緒失去了保護】，
//         另一條執行緒立刻可以進來。程式不會崩潰，錯誤會在完全不相干的
//         地方以資料損毀的形式浮現。
//     為什麼會錯：把 mutex 想成一個「狀態旗標」，覺得重複設成 false 無害。
//         但 mutex 的語意是「所有權」而非「布林狀態」；
//         擅自解鎖等於把別人正在使用的保護拆掉，
//         這比「沒作用」嚴重得多，也難查得多。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <stdexcept>

std::mutex mtx;

// -----------------------------------------------------------------------------
// 【錯誤示範 1】解一把沒有鎖定的 mutex → UB
// -----------------------------------------------------------------------------
void wrongUnlock() {
    // 沒有 lock()
    mtx.unlock();  // 💀 未定義行為！
}

// -----------------------------------------------------------------------------
// 【錯誤示範 2】跨執行緒解鎖 → UB，且保護會靜默失效
// -----------------------------------------------------------------------------
void wrongThread() {
    mtx.lock();
    
    std::thread t([&]() {
        mtx.unlock();  // 💀 在不同執行緒解鎖！未定義行為！
    });
    t.join();
}

// -----------------------------------------------------------------------------
// 【正確作法 1】RAII：讓這兩種錯誤在語法層面就寫不出來
//   lock_guard 不可複製、不可移動，且是自動儲存期物件 ——
//   它的解構必定發生在建立它的那條執行緒上，不可能跨執行緒解鎖，
//   也不可能「解一把沒鎖的鎖」。
// -----------------------------------------------------------------------------
std::mutex safeMtx;
long safeCounter = 0;

void safeIncrement(int times) {
    for (int i = 0; i < times; ++i) {
        std::lock_guard<std::mutex> lock(safeMtx);   // 建構即鎖
        ++safeCounter;
    }                                                // 解構即解，且必定在本執行緒
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】把「必須成對」的資源一律包成 RAII
//   情境：專案裡除了 mutex，還有很多「取得 → 必須釋放」的資源：
//         檔案句柄、資料庫交易、GPU context、效能計時區段。
//         只要是手動配對，就一定會有人在某條 return 路徑上漏掉。
//   做法：寫一個小小的 RAII 包裝，讓釋放變成「不可能忘記」。
//         本例示範一個「資源租借」的守衛：它同時解決
//         ① 忘記歸還 ② 在錯的執行緒歸還 兩個問題，
//         因為解構一定發生在持有它的那個作用域裡。
// -----------------------------------------------------------------------------
class ResourceTracker {
private:
    mutable std::mutex mtx;
    int inUse = 0;
    int peak = 0;
    long totalAcquired = 0;

public:
    void acquire() {
        std::lock_guard<std::mutex> lock(mtx);
        ++inUse;
        ++totalAcquired;
        if (inUse > peak) peak = inUse;
    }
    void release() {
        std::lock_guard<std::mutex> lock(mtx);
        --inUse;
    }
    int currentInUse() const { std::lock_guard<std::mutex> lock(mtx); return inUse; }
    int peakInUse() const    { std::lock_guard<std::mutex> lock(mtx); return peak; }
    long total() const       { std::lock_guard<std::mutex> lock(mtx); return totalAcquired; }
};

// RAII 守衛：建構時借出、解構時必定歸還（即使中途 return 或拋例外）
class ResourceLease {
private:
    ResourceTracker& tracker;

public:
    explicit ResourceLease(ResourceTracker& t) : tracker(t) { tracker.acquire(); }
    ~ResourceLease() { tracker.release(); }

    // 禁止複製與移動 —— 與 lock_guard 完全相同的設計，
    // 確保「歸還」只會發生一次，而且一定在借出的那個作用域
    ResourceLease(const ResourceLease&) = delete;
    ResourceLease& operator=(const ResourceLease&) = delete;
};

// 即使這個函式有三個離開點（含拋例外），租借都必定被歸還
bool useResource(ResourceTracker& tracker, int value) {
    ResourceLease lease(tracker);          // 借出
    if (value < 0) return false;           // 離開點 1 → 解構歸還
    if (value % 13 == 0) {
        throw std::runtime_error("unlucky");   // 離開點 2 → 堆疊展開時歸還
    }
    return true;                           // 離開點 3 → 解構歸還
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】「A 發許可、B 取用」該用 semaphore 而不是 mutex
//   情境：生產者算完一批資料，通知消費者可以取用。
//         這是「傳遞訊號」而不是「保護程式碼」，用 mutex 的 lock/unlock
//         去模擬會直接掉進本檔的跨執行緒解鎖 UB。
//   正解：用 condition_variable + mutex（C++11 就有，相容性最好），
//         或 C++20 的 std::counting_semaphore。
//         本例用 cv 實作，因為本檔以 C++17 編譯。
//   關鍵區別：mutex 保護「臨界區段」，cv / semaphore 傳遞「事件」。
// -----------------------------------------------------------------------------
class WorkQueue {
private:
    std::mutex mtx;                     // 保護 queue 本身（這才是 mutex 的職責）
    std::condition_variable cv;         // 傳遞「有工作了」的訊號
    std::queue<int> items;
    bool done = false;

public:
    void push(int v) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            items.push(v);
        }
        cv.notify_one();                // 發訊號：任何執行緒都可以呼叫，沒有所有權問題
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            done = true;
        }
        cv.notify_all();
    }

    // 取一件工作；沒工作就等待。回傳 false 表示已收工且沒有剩餘工作。
    bool pop(int& out) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !items.empty() || done; });
        if (items.empty()) return false;    // done 且已取空
        out = items.front();
        items.pop();
        return true;
    }
};

int main() {
    // 這些都是錯誤的用法
    // wrongUnlock();
    // wrongThread();
    
    std::cout << "這些函式都有問題，不要這樣做！" << std::endl;
    std::cout << "  wrongUnlock() ：解一把沒鎖的 mutex        → 未定義行為\n";
    std::cout << "  wrongThread() ：A 執行緒鎖、B 執行緒解    → 未定義行為\n";
    std::cout << "  兩者在 main 中刻意保持註解；取消註解即進入 UB。\n";

    std::cout << "\n=== 正確作法：RAII 讓這兩種錯誤寫不出來 ===\n";
    {
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) ths.emplace_back(safeIncrement, 50000);
        for (auto& t : ths) t.join();
        std::cout << "8 執行緒 × 50000 次遞增\n";
        std::cout << "safeCounter = " << safeCounter << " (必定為 400000)\n";
        std::cout << "lock_guard 不可複製/移動，且解構必定發生在本執行緒 →\n";
        std::cout << "  不可能跨執行緒解鎖，也不可能解一把沒鎖的鎖。\n";
    }

    std::cout << "\n=== 日常實務 1：RAII 守衛保證資源必定歸還 ===\n";
    {
        ResourceTracker tracker;
        std::vector<std::thread> ths;

        for (int i = 0; i < 4; ++i) {
            ths.emplace_back([&tracker, i] {
                for (int k = 0; k < 1000; ++k) {
                    try {
                        useResource(tracker, i * 1000 + k - 100);  // 含負數與 13 的倍數
                    } catch (const std::exception&) {
                        // 即使拋例外，ResourceLease 的解構仍已歸還資源
                    }
                }
            });
        }
        for (auto& t : ths) t.join();

        std::cout << "共借出 " << tracker.total() << " 次"
                  << "（其中含 return false 與拋例外的路徑）\n";
        std::cout << "目前未歸還: " << tracker.currentInUse()
                  << " (必定為 0 —— RAII 保證每條路徑都歸還)\n";
        std::cout << "峰值同時使用: " << tracker.peakInUse() << " (≤ 4，每執行緒最多 1)\n";
    }

    std::cout << "\n=== 日常實務 2：傳遞訊號用 condition_variable，不是 mutex ===\n";
    {
        WorkQueue q;
        std::atomic<int> consumed{0};
        std::atomic<long> sum{0};

        std::vector<std::thread> consumers;
        for (int i = 0; i < 3; ++i) {
            consumers.emplace_back([&q, &consumed, &sum] {
                int v;
                while (q.pop(v)) {
                    consumed.fetch_add(1);
                    sum.fetch_add(v);
                }
            });
        }

        std::thread producer([&q] {
            for (int i = 1; i <= 3000; ++i) q.push(i);
            q.finish();
        });

        producer.join();
        for (auto& t : consumers) t.join();

        std::cout << "生產 3000 件，3 個消費者\n";
        std::cout << "消費件數: " << consumed.load() << " (必定為 3000)\n";
        std::cout << "總和: " << sum.load() << " (必定為 4501500 = 1+2+...+3000)\n";
        std::cout << "→ notify 由生產者呼叫、wait 由消費者呼叫，完全沒有所有權問題；\n";
        std::cout << "  mutex 只用來保護 queue 本身，各司其職。\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤8.cpp' -o unlock_unowned
//
// 想觀察 UB（自行承擔風險，取消 main 中的註解後）:
//   g++ -std=c++17 -g -fsanitize=thread -pthread '課程 5.4：互斥鎖的常見錯誤8.cpp' -o unlock_tsan
//   → ThreadSanitizer 會明確報出 "unlock of an unlocked mutex"，
//     比等它剛好崩潰可靠得多。

// 註：兩個 UB 示範（wrongUnlock / wrongThread）在 main 中【刻意保持註解】，
// 因此本檔實際執行的部分完全沒有 UB，輸出為確定值。
// 唯一例外是「峰值同時使用」——它取決於 4 條執行緒的重疊程度，
// 每次執行可能是 1~4 之間的任何值（本機連續三次實測皆為 4）；
// 有保證的是「目前未歸還」必定為 0，這正是 RAII 要證明的事。

// === 預期輸出 ===
// 這些函式都有問題，不要這樣做！
//   wrongUnlock() ：解一把沒鎖的 mutex        → 未定義行為
//   wrongThread() ：A 執行緒鎖、B 執行緒解    → 未定義行為
//   兩者在 main 中刻意保持註解；取消註解即進入 UB。
//
// === 正確作法：RAII 讓這兩種錯誤寫不出來 ===
// 8 執行緒 × 50000 次遞增
// safeCounter = 400000 (必定為 400000)
// lock_guard 不可複製/移動，且解構必定發生在本執行緒 →
//   不可能跨執行緒解鎖，也不可能解一把沒鎖的鎖。
//
// === 日常實務 1：RAII 守衛保證資源必定歸還 ===
// 共借出 4000 次（其中含 return false 與拋例外的路徑）
// 目前未歸還: 0 (必定為 0 —— RAII 保證每條路徑都歸還)
// 峰值同時使用: 4 (≤ 4，每執行緒最多 1)
//
// === 日常實務 2：傳遞訊號用 condition_variable，不是 mutex ===
// 生產 3000 件，3 個消費者
// 消費件數: 3000 (必定為 3000)
// 總和: 4501500 (必定為 4501500 = 1+2+...+3000)
// → notify 由生產者呼叫、wait 由消費者呼叫，完全沒有所有權問題；
//   mutex 只用來保護 queue 本身，各司其職。
