// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤12.cpp  —  保護範圍太小：check-then-act / TOCTOU
// =============================================================================
//
// 【主題資訊 Information】
//   本檔談的不是某個 API，而是一類【邏輯錯誤】：
//       TOCTOU = Time Of Check To Time Of Use（檢查時機 vs 使用時機）
//   徵狀：每一次個別存取都有鎖保護（所以【不是 data race】、【不是 UB】），
//         但「檢查」與「依據檢查結果採取的行動」被拆進兩個臨界區段，
//         中間的空隙讓其他執行緒插進來，使不變量被打破。
//   標頭檔：<mutex>、<vector>、<thread>、<atomic>
//
// 【詳細解釋 Explanation】
//
// 【1. 這個錯誤最危險的地方：它看起來完全正確】
//   看看這段程式碼——每一次碰 data 都有鎖：
//       mtx.lock();
//       bool shouldAppend = (data.size() < 10);   // 檢查
//       mtx.unlock();
//                                                  // ← 空隙！
//       if (shouldAppend) {
//           mtx.lock();
//           data.push_back(value);                 // 動作
//           mtx.unlock();
//       }
//   用 ThreadSanitizer 掃描：【乾淨，零警告】。因為它真的沒有 data race。
//   靜態分析工具也找不出來——每個共享存取都被正確保護了。
//   但 shouldAppend 這個布林值在離開臨界區段的那一刻就【過期】了：
//   它描述的是「過去某個時間點的世界」，而程式卻拿它去決定「現在」該做什麼。
//
// 【2. 為什麼「每個操作都是原子的」不等於「整體是原子的」】
//   這是並行程式設計最重要的觀念之一，值得反覆強調：
//       原子性【不能組合】（atomicity does not compose）。
//   兩個原子操作接在一起，得到的不是一個原子操作，而是
//   「兩個原子操作，中間有一個任何事都可能發生的空隙」。
//   同樣的道理適用於所有 thread-safe 容器：
//       if (!q.empty()) { auto x = q.pop(); }   // 💀 即使 q 每個方法都有鎖
//       if (!m.count(k)) { m.insert(k, v); }    // 💀 同上
//   → 這也是為什麼 Java 的 ConcurrentHashMap 要提供 putIfAbsent()、
//     C++20 要提供 std::atomic 的 compare_exchange——
//     它們把「檢查 + 動作」打包成一個不可分割的操作。
//
// 【3. 正解：讓臨界區段涵蓋整個「不變量的生命週期」】
//       std::lock_guard<std::mutex> lock(mtx);
//       if (data.size() < 10) {     // 檢查
//           data.push_back(value);  // 動作 —— 兩者之間鎖沒有放開
//       }
//   判準不是「哪幾行碰了共享資料」，而是
//   【從讀取狀態、到根據該狀態做完決定，這整段期間狀態不可以變】。
//   換句話說：臨界區段的邊界由【不變量】決定，不是由「變數存取」決定。
//
// 【4. 這個錯誤在真實世界的樣貌】
//   它有很多化身，全都是同一個錯誤：
//     * 【超賣】：檢查庫存 > 0 → （空隙）→ 扣庫存 → 變成 -1
//     * 【重複扣款】：檢查未付款 → （空隙）→ 扣款 → 扣了兩次
//     * 【檔案競態】：access() 檢查權限 → （空隙）→ open() 開檔
//       （這是經典的資安漏洞：攻擊者在空隙中把檔案換成 symlink）
//     * 【重複初始化】：檢查未初始化 → （空隙）→ 初始化 → 做了兩次
//   最後一項的正解是 std::call_once / std::once_flag。
//
// 【概念補充 Concept Deep Dive】
//   * 本檔的示範需要一道「同時起跑」的閘門才能穩定重現。
//     若只是依序建立 32 條執行緒，光是建立執行緒的開銷（數十微秒）
//     就足以讓它們自然錯開，空隙撞不到，問題就看不出來——
//     這正是這類 bug 在測試環境「重現不了」、上線後才爆的原因。
//   * 為什麼這種 bug 特別難抓：它需要兩條執行緒在【非常短的空隙】內
//     同時到達。機率可能是萬分之一，但在每秒數千次請求的系統上，
//     萬分之一意味著每分鐘發生數次。
//   * ⚠️ 檢查與動作之間即使只隔一個 unlock/lock（本檔的情況），
//     空隙依然存在。有些人以為「馬上就重新鎖回來了，應該沒事」——
//     作業系統完全可以在那一瞬間切換執行緒，而且
//     std::mutex 不保證公平，被喚醒的可能正好是競爭者。
//
// 【注意事項 Pay Attention】
//   1. 本檔的錯誤【不是 data race、不是 UB】，ThreadSanitizer 掃不出來。
//      它是邏輯錯誤，只能靠設計與程式碼審查避免。
//   2. 原子性不能組合：兩個原子操作串起來不是原子操作。
//   3. 臨界區段的邊界由【不變量】決定，不是由「哪幾行碰共享資料」決定。
//   4. thread-safe 容器不會讓使用它的程式碼自動變 thread-safe。
//   5. 正解是提供「檢查 + 動作」合一的介面（如 tryPush / putIfAbsent），
//      而不是要求呼叫端自己在外面加鎖。
//   6. 本檔的違規【不保證每次都發生】——它是機率事件，
//      這正是它難以測試的原因。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】check-then-act / TOCTOU
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 下面這段程式碼每次存取都有鎖，為什麼還是錯的？
//            mtx.lock(); bool ok = (v.size() < 10); mtx.unlock();
//            if (ok) { mtx.lock(); v.push_back(x); mtx.unlock(); }
//     答：ok 這個布林值在 unlock 的那一刻就過期了——它描述的是過去的狀態。
//         兩個臨界區段之間有空隙，別的執行緒可以在那裡 push_back，
//         等這條執行緒回來時 size 早就不是它檢查時的值了。
//         結果是「最多 10 筆」這個不變量被打破。
//         注意這【不是 data race】：ThreadSanitizer 掃不出來，
//         它是純粹的邏輯錯誤。
//     追問：怎麼修？→ 把檢查與動作放進【同一個】臨界區段：
//           lock_guard 一次包住 if 判斷與 push_back。
//
// 🔥 Q2. 一個 thread-safe 的佇列，每個方法都有鎖。
//        寫 if (!q.empty()) { auto x = q.pop(); } 安全嗎？
//     答：不安全。empty() 與 pop() 各自是原子的，但兩者之間有空隙：
//         另一條執行緒可能在中間把最後一個元素取走，
//         於是 pop() 對空佇列執行（丟例外或 UB，看實作）。
//         這就是「原子性不能組合」——
//         把兩個原子操作串起來，得到的是兩個原子操作加一個空隙。
//     追問：正確的介面該長什麼樣？
//           → 提供合一的操作，例如 bool tryPop(T& out)，
//             把「檢查是否為空」與「取出」放在同一個臨界區段內。
//             這也是為什麼 Java 有 putIfAbsent()、
//             C++ 有 compare_exchange——都是同一個設計思路。
//
// ⚠️ 陷阱. 「我用 ThreadSanitizer 掃過了，零警告，所以我的並行程式碼沒問題。」
//     答：TSan 偵測的是【data race】——「兩條執行緒同時存取同一記憶體位址，
//         至少一個是寫入，且沒有同步關係」。
//         本檔的錯誤完全不符合這個定義：每次存取都有鎖保護、
//         都有正確的 happens-before 關係。TSan 會回報乾淨。
//         但程式的不變量照樣被打破了。
//     為什麼會錯：把「沒有 data race」等同於「並行正確」。
//         data race 是【語言層級】的規則違反（會導致 UB）；
//         而 TOCTOU 是【應用層級】的邏輯錯誤——
//         程式完全符合 C++ 標準，只是做錯了事。
//         工具能抓前者，後者只能靠設計與審查。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）由評測框架
//   固定呼叫順序與執行緒數，考的是「如何建立順序」，
//   不存在「檢查完再動作」的複合不變量，因此無法示範 TOCTOU。
//   本檔改以「秒殺庫存超賣」這個真實事故場景呈現，
//   那才是這個錯誤在生產環境的實際樣貌。

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;
std::vector<int> data;

void unsafeAppend(int value) {
    // 錯誤：檢查和操作分開保護

    mtx.lock();
    bool shouldAppend = (data.size() < 10);
    mtx.unlock();

    // 💀 此時其他執行緒可能已經改變了 data.size()！
    //    shouldAppend 描述的是「過去」的狀態，已經過期了。

    if (shouldAppend) {
        mtx.lock();
        data.push_back(value);  // 💀 可能超過限制！
        mtx.unlock();
    }
}

void safeAppend(int value) {
    // 正確：整個操作在同一個臨界區段
    std::lock_guard<std::mutex> lock(mtx);

    if (data.size() < 10) {
        data.push_back(value);  // ✓ 安全：檢查與動作之間鎖沒有放開
    }
}

// 用 32 個執行緒同時搶著新增,放大「檢查與動作之間的空隙」。
// 注意：這裡用一個 atomic 閘門讓所有執行緒「同時起跑」——
// 若只是依序 create thread,建立執行緒的成本就足以讓它們自然錯開,
// 空隙撞不到,反而看不出問題。
// ⚠️ 這正是這類 bug 在測試環境重現不了、上線後才爆的原因。
static std::size_t runWith(void (*appendFn)(int)) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        data.clear();
    }

    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    threads.reserve(32);
    for (int i = 0; i < 32; ++i) {
        threads.emplace_back([&go, appendFn, i]() {
            while (!go.load(std::memory_order_acquire)) {
                // 等待起跑訊號
            }
            appendFn(i);
        });
    }
    go.store(true, std::memory_order_release);  // 同時放行

    for (auto& t : threads) {
        t.join();
    }

    std::lock_guard<std::mutex> lock(mtx);
    return data.size();
}

// -----------------------------------------------------------------------------
// 【日常實務範例】秒殺活動的庫存超賣事故
//   情境：限量 100 件的商品，開賣瞬間湧入大量請求。
//   ❌ 錯誤實作（tryBuyBroken）：
//        「查詢剩餘庫存」與「扣減庫存」是兩次獨立的加鎖操作。
//        兩條執行緒可能同時讀到「剩 1 件」，然後雙雙扣減 → 庫存變成 -1，
//        也就是【超賣】。這是電商系統最經典的事故之一。
//   ✅ 正確實作（tryBuySafe）：
//        檢查與扣減在同一個臨界區段內完成，庫存永遠不會變成負數。
//
//   本例用「同時起跑」的閘門讓兩種實作在相同壓力下對比，
//   並驗證：安全版的「售出數 + 剩餘庫存」永遠等於初始庫存。
// -----------------------------------------------------------------------------
class FlashSale {
private:
    std::mutex mtx_;
    int stock_;
    std::atomic<int> sold_{0};

public:
    explicit FlashSale(int stock) : stock_(stock) {}

    // ❌ 檢查與扣減分開 → 可能超賣
    bool tryBuyBroken() {
        int remaining;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            remaining = stock_;                 // 檢查
        }                                       // ← 空隙

        if (remaining <= 0) return false;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            --stock_;                           // 動作：可能扣成負數
        }
        sold_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // ✅ 檢查與扣減在同一個臨界區段
    bool tryBuySafe() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (stock_ <= 0) return false;      // 檢查
            --stock_;                           // 動作：中間鎖沒放開
        }
        sold_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    int stock() { std::lock_guard<std::mutex> lk(mtx_); return stock_; }
    int sold()  { return sold_.load(); }
};

static void runFlashSale(FlashSale& sale, bool useSafe, int threads, int perThread) {
    std::atomic<bool> go{false};
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));

    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&sale, &go, useSafe, perThread]() {
            while (!go.load(std::memory_order_acquire)) { }
            for (int i = 0; i < perThread; ++i) {
                if (useSafe) sale.tryBuySafe();
                else         sale.tryBuyBroken();
            }
        });
    }
    go.store(true, std::memory_order_release);
    for (auto& t : workers) t.join();
}

int main() {
    std::cout << "=== 課程示範: 保護範圍太小（check-then-act）===" << std::endl;
    {
        // 跑多輪，觀察不變量是否曾被打破
        const int rounds = 200;
        std::size_t maxUnsafe = 0;
        int violations = 0;

        for (int r = 0; r < rounds; ++r) {
            const std::size_t n = runWith(unsafeAppend);
            if (n > maxUnsafe) maxUnsafe = n;
            if (n > 10) ++violations;
        }

        std::cout << "unsafeAppend 跑了 " << rounds << " 輪（上限應為 10）" << std::endl;
        std::cout << "是否曾觀察到超過上限: "
                  << (violations > 0 ? "是 ← 不變量被打破了！" : "否（本次排程幸運地沒撞上）")
                  << std::endl;
        std::cout << "註: 超限的輪數與最大筆數每次執行都不同，故不列出數字。" << std::endl;
        std::cout << "    這【不是 data race】——每次存取都有鎖，"
                     "ThreadSanitizer 掃不出來。" << std::endl;

        std::size_t maxSafe = 0;
        for (int r = 0; r < rounds; ++r) {
            const std::size_t n = runWith(safeAppend);
            if (n > maxSafe) maxSafe = n;
        }
        std::cout << "safeAppend 跑了 " << rounds << " 輪，最大筆數: " << maxSafe
                  << "  (必須恰好是 10，永不超過)" << std::endl;
    }

    std::cout << "\n=== 日常實務: 秒殺活動的庫存超賣 ===" << std::endl;
    {
        // ❌ 錯誤版本
        {
            FlashSale sale(100);
            runFlashSale(sale, false, 32, 20);   // 32 執行緒 × 20 次 = 640 次搶購
            const int finalStock = sale.stock();
            // 註：實際售出數與剩餘庫存每次執行都不同（取決於有多少執行緒
            //     撞進空隙），故不列出數字，只驗證「是否超賣」這個事實。
            std::cout << "[錯誤版] 初始庫存 100，32 執行緒共嘗試搶購 640 次"
                      << std::endl;
            std::cout << "[錯誤版] 庫存是否變成負數（超賣）: "
                      << (finalStock < 0 ? "是 ← 超賣事故！" : "否（本次僥倖）")
                      << std::endl;
            std::cout << "[錯誤版] 實際售出數與剩餘庫存每次執行都不同，故不列出"
                      << std::endl;
        }

        // ✅ 正確版本
        {
            FlashSale sale(100);
            runFlashSale(sale, true, 32, 20);
            const int finalStock = sale.stock();
            const int sold = sale.sold();
            std::cout << "[正確版] 初始庫存 100，售出 " << sold
                      << "，剩餘庫存 " << finalStock << std::endl;
            std::cout << "[正確版] 售出 + 剩餘 = " << (sold + finalStock)
                      << "  (必須恰好是 100)" << std::endl;
            std::cout << "[正確版] 庫存是否為負: "
                      << (finalStock < 0 ? "是" : "否（永遠不會）") << std::endl;
        }
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤12.cpp' -o scope_too_small
// 驗證它不是 data race: g++ -std=c++17 -pthread -g -fsanitize=thread '課程 5.4：互斥鎖的常見錯誤12.cpp' -o scope_tsan
//   （TSan 會回報乾淨——這正是本檔要強調的重點）

// -----------------------------------------------------------------------------
// 【輸出但書】
//   ⚠️ 本檔有【兩行】的結果是機率性的，兩種答案都是「正確的執行結果」：
//     1. 「unsafeAppend 是否曾觀察到超過上限」
//     2. 「[錯誤版] 庫存是否變成負數（超賣）」
//   競態是否被觸發取決於排程，本質上無法保證。
//   本機實測 6 次連續執行【都是「是」】（200 輪的重複與 32 條執行緒
//   同時起跑，大幅提高了撞上空隙的機率），故預期輸出列「是」；
//   但看到「否」【不代表程式有誤】，只代表那次排程沒撞上——
//   這正是這類 bug 難以測試的本質。
//   相對地，safeAppend 的「最大 10」與正確版的「售出 + 剩餘 = 100」
//   是【確定】的，任何一次執行都必然成立。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 保護範圍太小（check-then-act）===
// unsafeAppend 跑了 200 輪（上限應為 10）
// 是否曾觀察到超過上限: 是 ← 不變量被打破了！
// 註: 超限的輪數與最大筆數每次執行都不同，故不列出數字。
//     這【不是 data race】——每次存取都有鎖，ThreadSanitizer 掃不出來。
// safeAppend 跑了 200 輪，最大筆數: 10  (必須恰好是 10，永不超過)
//
// === 日常實務: 秒殺活動的庫存超賣 ===
// [錯誤版] 初始庫存 100，32 執行緒共嘗試搶購 640 次
// [錯誤版] 庫存是否變成負數（超賣）: 是 ← 超賣事故！
// [錯誤版] 實際售出數與剩餘庫存每次執行都不同，故不列出
// [正確版] 初始庫存 100，售出 100，剩餘庫存 0
// [正確版] 售出 + 剩餘 = 100  (必須恰好是 100)
// [正確版] 庫存是否為負: 否（永遠不會）
