// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定4.cpp  —  隨機退避（jitter）與活鎖
// =============================================================================
//
// 【主題資訊 Information】
//   std::random_device                 // C++11，<random>，非決定性種子來源
//   std::mt19937                       // C++11，Mersenne Twister 引擎
//   std::uniform_int_distribution<>    // C++11，均勻整數分布
//   bool std::mutex::try_lock();       // C++11，<mutex>
//   標頭檔：<mutex>、<random>、<thread>、<chrono>
//   本檔模式：try-and-back-off + 【隨機】等待 + 最大嘗試次數上限
//
// 【詳細解釋 Explanation】
//
// 【1. 活鎖（livelock）：沒卡住，但也沒進展】
//   死結是「所有人都停住不動」；活鎖是「所有人都很忙，卻沒有人前進」。
//   典型畫面：兩個人在走廊迎面相遇，同時往左讓、又同時往右讓，
//   反覆禮讓卻始終過不去。
//   在本檔的場景裡：
//       T1: 鎖住 mutex1 → try_lock(mutex2) 失敗 → 放掉 → 等 5ms → 重來
//       T2: 鎖住 mutex2 → try_lock(mutex1) 失敗 → 放掉 → 等 5ms → 重來
//   如果兩者的等待【一模一樣】，它們就會以相同節奏同步地重試，
//   每一輪都精準地再次撞在一起。
//   ⚠️ 與死結的關鍵差異：活鎖的執行緒都是 runnable 的，
//   所以現象是 CPU 使用率【很高】卻毫無產出；
//   死結則是 CPU 使用率接近 0。這是線上診斷時最快的區分方式。
//
// 【2. 隨機退避為什麼有效】
//   活鎖的成因是「節奏同步」。隨機化就是要打散這個同步：
//       int waitTime = dis(gen);   // 1~10 毫秒隨機
//   兩條執行緒等待不同長度，下一輪就會錯開，其中一條先進場、拿到兩把鎖、
//   完成後釋放，另一條隨即成功。
//   只要有【任何】隨機性，持續碰撞的機率就隨輪數指數下降：
//   若每輪錯開的機率是 p，連續 n 輪都撞上的機率是 (1-p)^n → 迅速趨近 0。
//   → 這也是為什麼「活鎖」在加了 jitter 之後幾乎不再是實務問題。
//
// 【3. 為什麼是隨機，而不是「讓其中一個優先」】
//   另一種解法是給執行緒排定優先順序（例如 id 小的優先），
//   但那會造成【飢餓】（starvation）：id 大的執行緒可能長期搶不到。
//   隨機退避的好處是統計上公平——長期而言每條執行緒的期望等待相同。
//   實務系統（TCP 的 exponential backoff、乙太網路的 CSMA/CD、
//   AWS SDK 的重試策略）全都採用「指數退避 + 隨機 jitter」，
//   正是這個道理。
//
// 【4. 隨機數產生器為什麼要放在【執行緒內部】】
//   本檔的三行：
//       std::random_device rd;
//       std::mt19937 gen(rd());
//       std::uniform_int_distribution<> dis(1, 10);
//   全部宣告在 robustOperation() 內，是【每條執行緒各有一份】。
//   這非常重要：
//     * std::mt19937 【不是】thread-safe。多條執行緒共用同一個引擎
//       而不加鎖，就是 data race（UB）。
//     * 若做成全域再加鎖保護，等於在「避免鎖競爭」的程式碼裡
//       又引入一把鎖，本末倒置。
//   → 慣用做法是 thread_local，或像本檔一樣宣告成區域變數。
//   ⚠️ 但要注意成本：std::random_device 在 Linux 上可能讀
//   /dev/urandom 或用 RDRAND 指令，初始化不便宜。
//   若這個函式會被高頻呼叫，應改用 thread_local 只初始化一次。
//
// 【概念補充 Concept Deep Dive】
//   * 三種「卡住」的完整區分：
//       死結（deadlock）  ：互相等待，全部停止，CPU ≈ 0%
//       活鎖（livelock）  ：互相禮讓，都在跑但無進展，CPU 很高
//       飢餓（starvation）：整體有進展，但【某些】執行緒長期搶不到資源
//     三者的診斷方式與解法都不同，面試很愛考它們的差異。
//   * 本檔的 maxAttempts = 100 是必要的安全閥。
//     沒有上限的重試迴圈遇到病態競爭時會永遠轉下去，
//     而且完全沒有任何訊號讓維運知道出事了。
//     有上限就能在超過時記錄告警、走降級路徑。
//   * 老實說：這個手寫模式在需要「同時取得多把鎖」時，
//     一律應該用 std::scoped_lock（C++17）取代——
//     它內部的死結避免演算法已經處理好這些細節。
//     本檔的價值在於理解「為什麼需要隨機性」，
//     以及在無法一次取得所有鎖的場景下該怎麼寫。
//
// 【注意事項 Pay Attention】
//   1. 隨機數引擎【不是 thread-safe】，必須每執行緒一份
//      （區域變數或 thread_local），絕不可多執行緒共用。
//   2. 重試迴圈【必須】有次數或時間上限，否則病態情況下永不返回。
//   3. 活鎖的徵狀是 CPU 高但無產出；死結是 CPU 低且完全停止。
//   4. 固定間隔的退避【不保證】會活鎖，隨機退避也【不保證】不會——
//      兩者都是機率問題。隨機化只是把碰撞機率壓到實務上可忽略。
//   5. 需要同時持有多把鎖時，優先用 std::scoped_lock 而非手寫重試。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】活鎖與隨機退避
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 死結、活鎖、飢餓有什麼差別？線上遇到時怎麼快速區分？
//     答：死結＝互相等待、全體停止，執行緒都在 blocked 狀態，CPU ≈ 0%；
//         活鎖＝互相禮讓、都在跑但沒進展，執行緒都是 runnable，CPU 很高；
//         飢餓＝系統整體有進展，但某些執行緒長期搶不到資源。
//         快速區分：先看 CPU 使用率——接近 0 就往死結查（取堆疊會看到
//         全部停在 pthread_mutex_lock），很高卻沒產出就往活鎖查。
//     追問：飢餓怎麼辦？→ 換成公平鎖（FIFO 排隊）、
//           或加入優先度老化（aging），讓等久的逐漸提高優先度。
//
// 🔥 Q2. 為什麼退避要「隨機」？固定間隔不行嗎？
//     答：活鎖的成因是【節奏同步】。固定間隔會讓所有競爭者以相同節拍
//         重試，每一輪都精準地再次撞在一起。
//         加入隨機 jitter 之後，每輪錯開的機率是 p，
//         連續 n 輪都撞上的機率是 (1-p)^n，隨輪數指數衰減，
//         實務上幾乎不可能持續碰撞。
//     追問：為什麼不直接讓 id 小的執行緒優先？→ 那會造成飢餓，
//           id 大的可能長期搶不到。隨機退避在統計上是公平的。
//
// ⚠️ 陷阱. 把 std::mt19937 gen 宣告成全域變數讓所有執行緒共用，
//        「反正只是拿來算等待時間，不精確也沒關係」——這樣可以嗎？
//     答：不可以，這是 data race → 未定義行為。
//         mt19937 每次產生亂數都會【修改自己的內部狀態】（624 個字的陣列
//         加上索引），多條執行緒同時讀寫就是典型的 data race。
//         它的後果不只是「拿到不夠隨機的數字」，而是整個程式的行為
//         都不再有定義——標準不保證任何事。
//     為什麼會錯：把 UB 的嚴重性誤判成「結果不精確」。
//         「這個值算錯一點也沒差」是對【邏輯】的判斷，
//         但 data race 是對【語言規則】的違反，兩者不是同一個層次。
//         正解：宣告成區域變數或 thread_local，每條執行緒各有一份。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）只用單一同步點，
//   不需要同時持有多把鎖，因此不會發生活鎖，也用不到隨機退避。
//   更根本的是：那些題目要求【確定的】輸出順序，
//   而本檔的主題正是「引入隨機性」——兩者的目標互相矛盾。
//   硬掛一題會給出完全錯誤的示範，故從缺。

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

std::mutex mutex1;
std::mutex mutex2;

std::atomic<int> successCount{0};
std::atomic<int> giveUpCount{0};

void robustOperation(int id) {
    (void)id;   // 訊息改由主執行緒統一輸出，避免多執行緒 cout 交錯

    // 隨機數產生器
    // ⚠️ 這三行【必須】在函式內（每執行緒一份）：
    //    std::mt19937 不是 thread-safe，共用同一個引擎就是 data race。
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10);

    int attempts = 0;
    const int maxAttempts = 100;

    while (attempts < maxAttempts) {
        ++attempts;

        mutex1.lock();

        if (mutex2.try_lock()) {
            // 成功獲得兩個鎖

            // 執行操作
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            mutex2.unlock();
            mutex1.unlock();
            successCount.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        // 釋放並等待隨機時間（指數退避的簡化版）
        // ⚠️ 順序關鍵：先 unlock 再 sleep，握著鎖去睡等於沒解決問題
        mutex1.unlock();

        int waitTime = dis(gen);  // 1-10 毫秒的隨機等待
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }

    // 達到最大嘗試次數 —— 安全閥，避免病態情況下永遠轉下去
    giveUpCount.fetch_add(1, std::memory_order_relaxed);
}

// -----------------------------------------------------------------------------
// 【日常實務範例】快取重建的驚群效應（thundering herd / cache stampede）
//   情境：一個熱門的快取項目過期了。同一瞬間有 32 條請求執行緒發現快取失效，
//         全部衝去搶「重建鎖」準備回源查資料庫。
//   若沒有 jitter：32 條執行緒以完全相同的節奏重試，
//         每一輪都同時撞上，且每次失敗都在燒 CPU。
//   正解（本例）：
//     (1) 只有搶到重建鎖的那一條真的去回源（其餘不重複打資料庫）；
//     (2) 沒搶到的執行緒【隨機】退避後再檢查快取，錯開重試節奏；
//     (3) 有重試上限，超過就走降級路徑（回傳過期資料或錯誤）。
//   驗證：資料庫只會被查詢【一次】，而所有請求最終都拿到資料。
// -----------------------------------------------------------------------------
class CacheWithStampedeProtection {
private:
    std::mutex        dataMtx_;
    std::string       cachedValue_;
    bool              valid_ = false;

    std::mutex        rebuildMtx_;      // 只有一條執行緒能持有它去回源
    std::atomic<int>  dbQueries_{0};    // 資料庫被查了幾次

public:
    // 回傳取得的值；取不到回傳空字串（降級）
    std::string get(int maxAttempts) {
        // 每執行緒各自的隨機引擎
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> jitter(1, 5);

        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            // (1) 先看快取有沒有效
            {
                std::lock_guard<std::mutex> lk(dataMtx_);
                if (valid_) return cachedValue_;
            }

            // (2) 快取失效 → 嘗試搶「重建」的資格
            if (rebuildMtx_.try_lock()) {
                // 搶到了：由我負責回源。但要再確認一次——
                // 可能在我搶鎖的空檔，別人已經重建好了（double-checked locking）
                bool needRebuild = false;
                {
                    std::lock_guard<std::mutex> lk(dataMtx_);
                    needRebuild = !valid_;
                }

                if (needRebuild) {
                    dbQueries_.fetch_add(1, std::memory_order_relaxed);
                    // 模擬回源查資料庫的耗時
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));

                    std::lock_guard<std::mutex> lk(dataMtx_);
                    cachedValue_ = "user:42:profile";
                    valid_ = true;
                }
                rebuildMtx_.unlock();
                continue;   // 下一輪就會從快取讀到
            }

            // (3) 沒搶到重建資格：隨機退避後再看快取，避免整群同步重試
            std::this_thread::sleep_for(std::chrono::milliseconds(jitter(gen)));
        }
        return "";   // 降級：超過重試上限
    }

    int dbQueries() const { return dbQueries_.load(); }
};

int main() {
    std::cout << "=== 課程示範: 隨機退避的 try-and-back-off ===" << std::endl;
    {
        std::thread t1(robustOperation, 1);
        std::thread t2(robustOperation, 2);

        t1.join();
        t2.join();

        std::cout << "成功完成的執行緒數: " << successCount.load()
                  << "  (必須是 2)" << std::endl;
        std::cout << "達到上限而放棄的執行緒數: " << giveUpCount.load()
                  << "  (必須是 0：100 次上限遠遠足夠)" << std::endl;
        std::cout << "註: 各執行緒實際嘗試了幾次、等待了多久都是隨機的，"
                     "每次執行都不同，故不列出" << std::endl;
    }

    std::cout << "\n=== 三種「卡住」的區分 ===" << std::endl;
    std::cout << "死結 deadlock   : 互相等待，全體停止，CPU 使用率 ≈ 0%" << std::endl;
    std::cout << "活鎖 livelock   : 互相禮讓，都在跑但無進展，CPU 使用率很高" << std::endl;
    std::cout << "飢餓 starvation : 整體有進展，但某些執行緒長期搶不到資源" << std::endl;

    std::cout << "\n=== 日常實務: 防止快取重建的驚群效應 ===" << std::endl;
    {
        CacheWithStampedeProtection cache;
        const int numRequests = 32;

        std::vector<std::thread> requests;
        std::vector<std::string> results(numRequests);

        // 32 條請求同時發現快取失效
        for (int i = 0; i < numRequests; ++i) {
            requests.emplace_back([&cache, &results, i]() {
                results[i] = cache.get(200);
            });
        }
        for (auto& t : requests) t.join();

        int served = 0;
        for (const auto& r : results) {
            if (!r.empty()) ++served;
        }

        std::cout << "同時湧入的請求數: " << numRequests << std::endl;
        std::cout << "成功取得資料的請求數: " << served
                  << "  (必須是 " << numRequests << "，一個都不能少)" << std::endl;
        std::cout << "資料庫實際被查詢次數: " << cache.dbQueries()
                  << "  (必須是 1——這就是防驚群的價值)" << std::endl;
        std::cout << "說明: 沒搶到重建資格的執行緒隨機退避，"
                     "避免整群以相同節奏反覆撞擊" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定4.cpp' -o avoid_livelock

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 本檔刻意【不輸出】任何嘗試次數與等待時間——它們本來就是隨機的，
//      每次執行必然不同。只驗證可穩定成立的不變量。
//   2. 「成功 2 / 放棄 0」是穩定的：100 次嘗試的預算遠大於實際所需
//      （對方最多持有兩把鎖 10ms）。
//   3. 「資料庫只被查 1 次」是【確定】的：重建鎖保證只有一條執行緒回源，
//      且它在回源前用 double-checked locking 再確認一次快取仍然失效。
//   4. 本檔【無法】穩定重現真正的活鎖——活鎖本來就是機率事件，
//      固定間隔不保證發生、隨機退避也不保證不發生。
//      這一點在「注意事項」中已明確說明，不以虛構的輸出假裝重現。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 隨機退避的 try-and-back-off ===
// 成功完成的執行緒數: 2  (必須是 2)
// 達到上限而放棄的執行緒數: 0  (必須是 0：100 次上限遠遠足夠)
// 註: 各執行緒實際嘗試了幾次、等待了多久都是隨機的，每次執行都不同，故不列出
//
// === 三種「卡住」的區分 ===
// 死結 deadlock   : 互相等待，全體停止，CPU 使用率 ≈ 0%
// 活鎖 livelock   : 互相禮讓，都在跑但無進展，CPU 使用率很高
// 飢餓 starvation : 整體有進展，但某些執行緒長期搶不到資源
//
// === 日常實務: 防止快取重建的驚群效應 ===
// 同時湧入的請求數: 32
// 成功取得資料的請求數: 32  (必須是 32，一個都不能少)
// 資料庫實際被查詢次數: 1  (必須是 1——這就是防驚群的價值)
// 說明: 沒搶到重建資格的執行緒隨機退避，避免整群以相同節奏反覆撞擊
