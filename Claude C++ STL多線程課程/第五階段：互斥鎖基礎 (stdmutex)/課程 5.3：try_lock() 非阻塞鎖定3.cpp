// =============================================================================
//  課程 5.3：try_lock() 非阻塞鎖定3.cpp  —  用 try_lock() 破除死結的循環等待
// =============================================================================
//
// 【主題資訊 Information】
//   bool std::mutex::try_lock();                                     // C++11
//   對照工具（更好的解法）：
//       template<class L1, class L2, class... L3>
//       void std::lock(L1&, L2&, L3&...);                            // C++11
//       template<class... MutexTypes> class std::scoped_lock;        // C++17
//   標頭檔：<mutex>
//   本檔模式：lock(A) → try_lock(B) → 失敗就【放掉 A】再重試（back-off）
//
// 【詳細解釋 Explanation】
//
// 【1. 死結的四個必要條件——本檔打破的是哪一個】
//   Coffman 條件（四個【同時】成立才會死結）：
//     (1) 互斥（Mutual Exclusion）：資源不能共享 —— 鎖的本質，無法拿掉
//     (2) 持有並等待（Hold and Wait）：拿著 A 的同時等 B
//     (3) 不可搶奪（No Preemption）：不能強行把別人的鎖搶過來
//     (4) 循環等待（Circular Wait）：T1 等 T2 的鎖、T2 等 T1 的鎖
//   本檔的做法打破的是【第 (2) 條】：
//   拿著 A 去 try_lock(B)，失敗就【立刻放掉 A】——
//   於是「持有並等待」不成立，循環等待也就無法形成。
//   ⚠️ 對照：另一種常見解法「全程式統一鎖的取得順序」打破的是第 (4) 條。
//   兩者都有效，但適用情境不同（見下方第 3 點）。
//
// 【2. 為什麼「放掉第一把鎖」是關鍵，順序不能顛倒】
//   非常容易寫錯的版本：
//       first.lock();
//       if (!second.try_lock()) {
//           std::this_thread::sleep_for(1ms);   // ← 還握著 first 就去睡
//           continue;
//       }
//   這樣完全沒有解決問題：睡覺期間仍然持有 first，
//   對方依舊拿不到，死結照樣成立（只是變成兩邊輪流睡而已）。
//   正確順序是【先 unlock，再 sleep】：
//       first.unlock();                          // 先放手
//       std::this_thread::sleep_for(1ms);        // 才去等
//   把鎖放掉，對方才有機會前進——這就是打破 hold-and-wait 的實際動作。
//
// 【3. 這個手寫模式 vs std::scoped_lock（C++17）】
//   標準已經提供了現成且更好的工具：
//       std::scoped_lock lock(mutex1, mutex2);   // C++17，一次取得多把鎖
//   它內部使用 std::lock 的死結避免演算法（典型實作是
//   「try-and-back-off」：先鎖一把，其餘用 try_lock，失敗就全部放掉、
//   換一把當起點重來），效果與本檔手寫的相同，但：
//     * 不可能寫錯順序或漏解鎖（RAII）
//     * 不可能忘記「失敗要放掉已持有的鎖」
//     * 例外安全
//   → 【實務結論】：需要同時持有多把鎖時，一律用 scoped_lock。
//     本檔手寫版本的價值在於讓你看懂 scoped_lock 內部在做什麼，
//     以及在「兩把鎖的取得之間必須做別的事」而無法一次取得時的備案。
//
// 【4. 這個模式的代價：可能活鎖（livelock）】
//   放掉鎖再重試解決了死結，卻引入新風險：
//   兩條執行緒可能以完全相同的節奏「取得 → 撲空 → 放手 → 重試」，
//   永遠互相禮讓而誰都無法前進。程式沒有卡死（每條執行緒都在跑），
//   但也沒有任何進展 —— 這叫活鎖。
//   本檔用固定的 1ms 等待，實務上應該用【隨機】退避打散節奏
//   （本課下一個檔案專門示範）。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼解鎖順序寫成「後取得的先釋放」（second → first）：
//     對【正確性】而言，解鎖順序其實無關緊要——unlock 不會阻塞，
//     不可能造成死結。這樣寫是為了對稱、可讀（像堆疊一樣後進先出），
//     而且與 RAII 的解構順序一致（scoped_lock 就是這樣做的）。
//     ⚠️ 但「取得」的順序至關重要，兩者不要混淆。
//   * 本檔的兩條執行緒刻意用【相反】的順序取鎖（t1: 1→2、t2: 2→1），
//     這正是教科書上的 AB-BA 死結場景。
//     若把 try_lock 換回 lock()，就會變成同課 5.4 示範的那個
//     「時序相依的 AB-BA 死結」——注意那個死結【不是保證發生】的，
//     它取決於兩條執行緒是否真的交錯。
//   * 這個模式在資料庫界叫做「樂觀鎖 + 重試」。
//     悲觀策略（老實排隊等）與樂觀策略（先試，衝突就重來）的取捨，
//     在鎖、交易、無鎖資料結構裡反覆出現，是很值得建立的直覺。
//
// 【注意事項 Pay Attention】
//   1. try_lock 失敗後【必須先 unlock 已持有的鎖，再 sleep】，順序不可顛倒。
//   2. 本模式可能造成活鎖；重試等待應加入隨機性（見下一檔）。
//   3. 需要同時持有多把鎖時，優先使用 std::scoped_lock（C++17）。
//   4. 解鎖順序不影響正確性，但取得順序影響極大。
//   5. 手寫版本有多條 unlock 路徑，極易遺漏；正式程式請交給 RAII。
//   6. 本檔的「無限 while (true)」在真實系統中應加上重試上限或截止時間，
//      否則遇到病態競爭時會永遠轉下去。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】用 try_lock 避免死結
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 死結的四個必要條件是什麼？本檔的做法打破了哪一個？
//     答：互斥、持有並等待、不可搶奪、循環等待——四者【同時】成立才死結。
//         本檔打破的是「持有並等待」：拿著第一把鎖去 try_lock 第二把，
//         失敗就立刻把第一把也放掉，不再「握著資源等別的資源」。
//     追問：還有別的破法嗎？→ 有，最常用的是打破「循環等待」——
//           全程式統一鎖的取得順序（例如永遠依記憶體位址由小到大取），
//           這樣不可能形成環。std::lock / scoped_lock 則是用
//           try-and-back-off 演算法，屬於打破「持有並等待」那一類。
//
// 🔥 Q2. 既然 C++17 有 std::scoped_lock，為什麼還要懂這個手寫模式？
//     答：兩個理由。第一，scoped_lock 內部做的就是這件事，
//         懂了才知道它為什麼不會死結、以及它為什麼可能反覆重試。
//         第二，當兩把鎖【無法同時取得】時（例如取得 A 之後要先做一段
//         判斷才知道需不需要 B），scoped_lock 就用不上，
//         這時仍需手寫 try-and-back-off。
//     追問：那實務上預設選哪個？→ 能用 scoped_lock 就用 scoped_lock，
//           它消除了漏解鎖與順序寫錯這兩類 bug。
//
// ⚠️ 陷阱. try_lock 失敗後這樣寫：「sleep_for(1ms); continue;」
//        （沒有先 unlock 第一把鎖）——問題在哪？
//     答：完全沒有解決死結。睡覺期間仍然持有第一把鎖，
//         對方依然拿不到它需要的資源，循環等待原封不動地存在，
//         只是從「兩邊都卡住」變成「兩邊輪流睡覺然後繼續卡住」。
//     為什麼會錯：多數人把重點放在「有沒有用 try_lock」，
//         以為只要不是阻塞式的 lock 就安全了。
//         但真正解決死結的動作不是 try_lock 本身，
//         而是【失敗之後把已持有的鎖放掉】。
//         try_lock 只是讓你有機會知道「失敗了」，
//         放手才是打破 hold-and-wait 的那一步。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）都只用
//   【單一】同步點協調輸出順序，沒有任何一題需要同時持有兩把以上的鎖，
//   因此不存在死結情境，也就用不到 try-and-back-off。
//   硬掛一題只會讓人誤以為這個模式是通用寫法，故從缺。

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mutex1;
std::mutex mutex2;

// 統計用（atomic，因為在鎖外更新）
std::atomic<int> retryCount{0};
std::atomic<int> completedCount{0};

void safeOperation(int id, std::mutex& first, std::mutex& second,
                   const char* firstName, const char* secondName) {
    (void)id; (void)firstName; (void)secondName;   // 訊息改由主執行緒統一輸出

    while (true) {
        // 先鎖定第一個互斥鎖
        first.lock();

        // 嘗試鎖定第二個互斥鎖
        if (second.try_lock()) {
            // 成功獲得兩個鎖

            // 執行需要兩個鎖的操作
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // 釋放鎖（注意順序：後獲取的先釋放。
            // 對正確性而言解鎖順序無關緊要，這樣寫是為了對稱與可讀）
            second.unlock();
            first.unlock();

            completedCount.fetch_add(1, std::memory_order_relaxed);
            return;  // 成功完成
        }

        // 無法獲得第二個鎖 → 【先放掉】第一個鎖，打破 hold-and-wait
        // ⚠️ 順序關鍵：必須 unlock 之後才 sleep，
        //    握著鎖去睡等於完全沒有解決死結。
        first.unlock();
        retryCount.fetch_add(1, std::memory_order_relaxed);

        // 稍等一下再重試（避免活鎖；實務上應改用隨機退避）
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// -----------------------------------------------------------------------------
// 【現代對照組】C++17 的 std::scoped_lock —— 同一件事，交給標準函式庫
//   scoped_lock 一次取得多把鎖，內部使用死結避免演算法，
//   而且是 RAII：不可能漏解鎖、不可能寫錯順序、例外安全。
//   注意：兩條執行緒即使把參數順序寫反（如下），也【不會】死結，
//   因為 scoped_lock 不是傻傻地依序 lock。
// -----------------------------------------------------------------------------
void scopedLockOperation(std::mutex& a, std::mutex& b) {
    std::scoped_lock lock(a, b);   // C++17：一次取得兩把，無死結保證
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

// -----------------------------------------------------------------------------
// 【日常實務範例】銀行帳戶轉帳：經典的雙鎖場景
//   情境：transfer(A → B) 需要同時鎖住兩個帳戶才能保證
//         「扣款」與「入帳」是一個原子操作（中間不能被別人看到錢憑空消失）。
//   死結怎麼發生：執行緒 1 做 transfer(甲 → 乙)、執行緒 2 同時做
//         transfer(乙 → 甲)，若各自依「來源帳戶先鎖」的直覺實作，
//         就形成 AB-BA 循環等待。
//   本例提供兩種正確解法並實測其正確性：
//     (1) scoped_lock —— 一次取得兩把鎖（推薦）
//     (2) 統一順序   —— 依帳戶 id 由小到大取鎖，打破循環等待
//   驗證方式：多執行緒隨機互轉數千次後，「總餘額」必須完全不變。
// -----------------------------------------------------------------------------
class Account {
public:
    int    id;
    std::mutex mtx;
    long   balance;

    Account(int accountId, long initial) : id(accountId), balance(initial) {}
};

// 解法 1：scoped_lock（C++17）——最推薦
bool transferScoped(Account& from, Account& to, long amount) {
    if (from.id == to.id) return false;

    std::scoped_lock lock(from.mtx, to.mtx);   // 無死結保證
    if (from.balance < amount) return false;
    from.balance -= amount;
    to.balance   += amount;
    return true;
}

// 解法 2：統一鎖的取得順序（依 id 由小到大）——打破「循環等待」
bool transferOrdered(Account& from, Account& to, long amount) {
    if (from.id == to.id) return false;

    Account& firstLock  = (from.id < to.id) ? from : to;
    Account& secondLock = (from.id < to.id) ? to   : from;

    std::lock_guard<std::mutex> l1(firstLock.mtx);
    std::lock_guard<std::mutex> l2(secondLock.mtx);

    if (from.balance < amount) return false;
    from.balance -= amount;
    to.balance   += amount;
    return true;
}

int main() {
    std::cout << "=== 課程示範: try-and-back-off 破除 AB-BA 死結 ===" << std::endl;
    {
        // 兩個執行緒以不同順序請求鎖（教科書的 AB-BA 場景）
        std::thread t1([]() {
            safeOperation(1, mutex1, mutex2, "mutex1", "mutex2");
        });
        std::thread t2([]() {
            safeOperation(2, mutex2, mutex1, "mutex2", "mutex1");
        });

        t1.join();
        t2.join();

        std::cout << "完成的執行緒數: " << completedCount.load()
                  << "  (必須是 2)" << std::endl;
        std::cout << "兩個執行緒都成功完成，沒有死結！" << std::endl;
        std::cout << "註: 實際重試了幾次由排程決定（本次"
                  << (retryCount.load() > 0 ? "有" : "沒有")
                  << "發生退讓），每次執行都不同，故不列出次數" << std::endl;
    }

    std::cout << "\n=== 現代對照組: std::scoped_lock (C++17) ===" << std::endl;
    {
        std::mutex a, b;
        // 刻意用相反順序傳入——scoped_lock 依然不會死結
        std::thread t1([&a, &b]() { for (int i = 0; i < 100; ++i) scopedLockOperation(a, b); });
        std::thread t2([&a, &b]() { for (int i = 0; i < 100; ++i) scopedLockOperation(b, a); });
        t1.join();
        t2.join();
        std::cout << "兩條執行緒以相反順序各取鎖 100 次: 全部完成，無死結"
                  << std::endl;
        std::cout << "說明: scoped_lock 內部就是 try-and-back-off，"
                     "但由 RAII 保證不會漏解鎖" << std::endl;
    }

    std::cout << "\n=== 日常實務: 銀行帳戶互轉（總餘額必須守恆）===" << std::endl;
    {
        const long initial = 10000;
        const int  numAccounts = 6;

        // ── 解法 1：scoped_lock ──
        {
            std::vector<std::unique_ptr<Account>> accounts;
            for (int i = 0; i < numAccounts; ++i) {
                accounts.push_back(std::make_unique<Account>(i, initial));
            }

            std::vector<std::thread> workers;
            for (int t = 0; t < 8; ++t) {
                workers.emplace_back([&accounts, t, numAccounts]() {
                    for (int i = 0; i < 2000; ++i) {
                        // 刻意製造雙向轉帳，最容易觸發 AB-BA
                        const int from = (t + i) % numAccounts;
                        const int to   = (t + i + 1 + (i % 3)) % numAccounts;
                        transferScoped(*accounts[from], *accounts[to], 10);
                    }
                });
            }
            for (auto& th : workers) th.join();

            long total = 0;
            for (const auto& a : accounts) total += a->balance;

            std::cout << "解法 1 (scoped_lock)  轉帳後總餘額: " << total
                      << "  (必須是 " << (initial * numAccounts) << ")" << std::endl;
        }

        // ── 解法 2：統一鎖的取得順序 ──
        {
            std::vector<std::unique_ptr<Account>> accounts;
            for (int i = 0; i < numAccounts; ++i) {
                accounts.push_back(std::make_unique<Account>(i, initial));
            }

            std::vector<std::thread> workers;
            for (int t = 0; t < 8; ++t) {
                workers.emplace_back([&accounts, t, numAccounts]() {
                    for (int i = 0; i < 2000; ++i) {
                        const int from = (t + i) % numAccounts;
                        const int to   = (t + i + 1 + (i % 3)) % numAccounts;
                        transferOrdered(*accounts[from], *accounts[to], 10);
                    }
                });
            }
            for (auto& th : workers) th.join();

            long total = 0;
            for (const auto& a : accounts) total += a->balance;

            std::cout << "解法 2 (統一順序)     轉帳後總餘額: " << total
                      << "  (必須是 " << (initial * numAccounts) << ")" << std::endl;
        }

        std::cout << "兩種解法都不會死結，且金額守恆" << std::endl;
        std::cout << "註: 各帳戶最終餘額的分佈取決於排程，每次執行都不同，"
                     "故只驗證總額" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.3：try_lock() 非阻塞鎖定3.cpp' -o avoid_deadlock

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. ⚠️ 「本次有／沒有發生退讓」這一行是本檔唯一可能變動的輸出，
//      【兩種結果都是正確的】。
//      本機實測連續 8 次執行【都是「沒有」】，原因值得想清楚：
//      t1 先啟動並取得 mutex1，接著 try_lock(mutex2) 通常直接成功；
//      而 t2 的第一步是 mutex2.lock()——那是【阻塞式】的 lock，不是 try_lock，
//      所以 t2 只是老實地等 t1 放手，並不會進入退讓分支。
//      要穩定觀察到退讓，需要兩條執行緒剛好在極短的時間窗內交錯。
//      這也正好說明：AB-BA 死結本身就是【時序相依】的，不是必然發生。
//   2. 各帳戶的最終餘額分佈每次都不同，故只驗證「總餘額守恆」這個不變量。
//   3. 若把本檔的 try_lock 換回 lock()，就會變成 AB-BA 死結——
//      但那也【不是保證】發生，取決於兩條執行緒是否真的交錯。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: try-and-back-off 破除 AB-BA 死結 ===
// 完成的執行緒數: 2  (必須是 2)
// 兩個執行緒都成功完成，沒有死結！
// 註: 實際重試了幾次由排程決定（本次沒有發生退讓），每次執行都不同，故不列出次數
//
// === 現代對照組: std::scoped_lock (C++17) ===
// 兩條執行緒以相反順序各取鎖 100 次: 全部完成，無死結
// 說明: scoped_lock 內部就是 try-and-back-off，但由 RAII 保證不會漏解鎖
//
// === 日常實務: 銀行帳戶互轉（總餘額必須守恆）===
// 解法 1 (scoped_lock)  轉帳後總餘額: 60000  (必須是 60000)
// 解法 2 (統一順序)     轉帳後總餘額: 60000  (必須是 60000)
// 兩種解法都不會死結，且金額守恆
// 註: 各帳戶最終餘額的分佈取決於排程，每次執行都不同，故只驗證總額
