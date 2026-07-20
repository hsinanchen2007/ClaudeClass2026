// =============================================================================
//  課程 4.2：不變量與競爭條件3.cpp  —  跨變數不變量：稽核者看到帳目不平
// =============================================================================
//
// 【本檔是「刻意示範錯誤」的範例，不要照抄到實際專案】
//   transfer() 寫入 accountA / accountB 的同時，audit() 正在讀取兩者，
//   彼此之間沒有任何同步 → genuine data race → 【未定義行為 (UB)】。
//   本檔【不會】宣稱「一定會印出警告」或「一定不會印」——UB 沒有固定結果。
//
// 【主題資訊 Information】
//   主題：    橫跨多個變數的不變量，以及「觀察者」看到破壞期的後果
//   語法：    bank.accountA -= amount;  ← 破壞期開始
//             bank.accountB += amount;  ← 破壞期結束
//   標準版本：std::thread 為 C++11
//   標頭檔：  <thread>、<iostream>
//   不變量：  accountA + accountB == 2000（總額守恆）
//   偵測工具：g++ -fsanitize=thread -g -pthread
//
// 【詳細解釋 Explanation】
//
// 【1. 這個不變量的特別之處：它橫跨兩個變數】
//   前兩檔的不變量是「指標互相對應」，看得見摸得著。
//   本檔的不變量更抽象，卻更貼近真實系統：
//       accountA + accountB == 2000
//   它不存在於任何單一變數裡，而是【兩個變數之間的關係】。
//   這帶來一個關鍵推論：
//   → 無論你把 accountA 和 accountB 各自保護得多好（例如各自變成 atomic），
//     都無法保護「它們之間的關係」。
//   要保護一個橫跨 N 個變數的不變量，臨界區段就必須同時涵蓋這 N 個變數。
//
// 【2. 為什麼 audit() 幾乎抓不到，卻仍然是嚴重的錯誤】
//   transfer() 的破壞期只有一道指令那麼寬：
//       bank.accountA -= amount;    // ← 破壞期開始（錢已扣、還沒入帳）
//       // ★ 只有在這個瞬間 audit() 讀取，才會看到 1999
//       bank.accountB += amount;    // ← 破壞期結束
//   而 audit() 自己也要做兩次讀取加一次加法。兩邊要剛好對齊，機率極低。
//   本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）實測，
//   多數執行完全印不出任何警告 —— 但這【不代表】不變量沒有被破壞，
//   只代表沒有被抓到。真實金融系統跑一整年、每秒數萬筆，
//   「機率極低」乘上「次數極大」就變成「每週都發生」。
//
// 【3. 更危險的是：audit 看到的值不只可能是 1999】
//   直覺會認為「最壞就是看到 1999 或 2001，反正誤差是 amount」。
//   這個推理隱含「每個 int 的讀寫都是原子的、且不會被重排」的假設，
//   而 C++ 標準對非 atomic 物件【沒有】這個保證。
//   一旦構成 data race 就是 UB：編譯器可以把 accountA 快取在暫存器、
//   可以重排兩次寫入、可以把整個 audit 迴圈優化掉。
//   所以正確的說法是「audit 可能印出任何東西，也可能什麼都不印」。
//
// 【4. 「觀察者」在真實系統中的樣子】
//   audit() 看似是課本上的玩具，實際上它無所不在：
//     * 監控 / metrics 匯出（Prometheus 定期抓取記憶體中的計數器）
//     * 健康檢查端點（/healthz 讀取多個狀態欄位組成回應）
//     * 對帳批次作業（每小時掃描一次餘額表）
//     * debug 用的 dump / 日誌快照
//   這些「只是讀一下、應該沒差」的程式碼，正是最容易忘記加鎖的地方，
//   也最容易產生「線上偶爾報警、重跑就正常」的鬼故事。
//
// 【5. 正確作法】
//   transfer() 與 audit() 必須用【同一把鎖】：
//       std::lock_guard<std::mutex> lock(bank.mtx);
//   只鎖 transfer 而不鎖 audit 是完全無效的 ——
//   鎖的作用來自「所有參與者都遵守同一個協定」，
//   任何一方不加鎖，互斥就不存在。
//   （下一檔「不變量與競爭條件4.cpp」就是加上鎖之後的正確版本。）
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼把 int 換成 std::atomic<int> 修不好這個 bug
//   把 accountA / accountB 都換成 atomic 之後：
//     * data race 消失了（atomic 的並行存取有定義）
//     * 但 race condition 還在：audit 仍然是「先讀 A、再讀 B」兩次獨立的原子讀，
//       中間 transfer 可以整個跑完。audit 讀到的 A 與 B 來自不同時刻，
//       兩個都是「合法的值」，加起來卻是不合法的組合。
//   → 這是 atomic 最常見的誤用：原子化每個零件，並不會讓組裝過程變原子。
//     要讓「讀 A + 讀 B」成為一個不可分割的觀察，必須用鎖，
//     或把兩個值塞進同一個原子物件（例如 atomic<struct{int a,b;}>，
//     但這需要該型別 is_lock_free，否則實作內部仍是用鎖）。
//
// (B) 破壞期的寬度 = 撞見的機率
//   破壞期越寬（例如中間有 I/O、有 sleep、有耗時計算），
//   被觀察到的機率越高。這也是為什麼「臨界區段內不要做慢動作」
//   除了效能考量之外，也有正確性上的意義：
//   把不變量破壞的時間拉長，等於把地雷埋得更大顆。
//
// (C) 為什麼本檔沒有用 sleep 來「幫忙」重現
//   加 sleep 確實能讓警告穩定出現，但那會讓學習者誤以為
//   「沒有 sleep 就不會發生」。真實的競爭不需要 sleep 也會發生，
//   只是需要運氣或壓力。本檔選擇誠實呈現「多數時候什麼都看不到」，
//   因為這才是這類 bug 真正的面貌 —— 也是它難以被發現的原因。
//
// 【注意事項 Pay Attention】
// 1. 本檔是 UB 示範：不可說「一定會印出警告」，也不可說「一定不會」。
// 2. 什麼都沒印出來，不代表程式正確；請用 ThreadSanitizer 判定。
// 3. 保護橫跨 N 個變數的不變量，臨界區段必須同時涵蓋這 N 個變數。
// 4. 讀取端也必須加鎖。只鎖寫入端等於完全沒鎖。
// 5. 把每個變數換成 atomic 消除 data race，但無法消除跨變數的 race condition。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔的核心是「跨變數不變量在多執行緒下被觀察」，
//   LeetCode 的設計題（146 / 155 / 705 / 707 / 1603）都在單執行緒判題環境下執行，
//   並行題（1114～1117 / 1195）則是在講執行緒間的「順序」而非「跨變數不變量」，
//   兩者都無法誠實對應本檔主題。與其硬湊一題不相關的，
//   不如用下面兩個真實系統中的實例（監控匯出、雙分錄記帳）來說明。
//   （跨變數不變量的 LeetCode 對應，請見「不變量與競爭條件4.cpp」的 155. Min Stack。）
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】跨變數不變量
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 把 accountA 和 accountB 都改成 std::atomic<int>，
//        audit() 還會看到總額不是 2000 嗎?
//     答：會。atomic 消除了 data race（UB 沒了），但 audit 仍是
//         「先原子讀 A、再原子讀 B」兩次獨立觀察，中間 transfer 可以整個完成。
//         兩個值各自合法，組合起來卻不合法。
//         原子化每個零件，不會讓組裝過程變原子。
//     追問：那要怎麼修?
//         → 用同一把 mutex 把「讀 A + 讀 B」包成一次不可分割的觀察；
//           或把兩個值放進同一個原子物件（需 is_lock_free，否則實作內部仍用鎖）。
//
// 🔥 Q2. 只在 transfer() 加鎖、audit() 不加鎖，可以嗎?
//     答：不可以，而且等於完全沒鎖。互斥來自「所有參與者遵守同一協定」，
//         沒加鎖的 audit 照樣可以在 transfer 持鎖期間讀到破壞期。
//         而且 audit 與 transfer 之間仍構成 data race → 依然是 UB。
//     追問：如果 audit 只讀不寫，難道也算 data race?
//         → 算。data race 的定義是「至少一方寫入」，不是「雙方都寫入」。
//           一寫一讀而無同步，就已經成立。
//
// ⚠️ 陷阱. 「我跑了很多次，audit 從來沒印出警告，所以這段程式沒問題」——錯在哪?
//     答：破壞期只有一道指令那麼寬，要剛好被撞見機率極低。
//         測不到只代表沒抓到，不代表沒有。真實系統每秒數萬筆、
//         跑一整年，「極低機率」會變成「每週發生」。
//     為什麼會錯：把並行 bug 當成「有機率出現的錯誤答案」，
//         用抽樣來否證。但 UB 的定義是「標準不再對任何行為負責」，
//         包含「看起來完全正常」。判定要用 ThreadSanitizer 這種
//         基於 happens-before 的形式化工具，不能靠統計。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <map>

// -----------------------------------------------------------------------------
// 【錯誤示範】無同步的轉帳 + 稽核 —— 本課主角
// -----------------------------------------------------------------------------
struct Bank {
    int accountA = 1000;
    int accountB = 1000;
    // 不變量：accountA + accountB == 2000
};

Bank bank;
int auditViolations = 0;   // 只在主執行緒 join 後讀取

void transfer(int amount) {
    // 不變量暫時破壞
    bank.accountA -= amount;
    // ← 此刻總額不是 2000！
    bank.accountB += amount;
    // 不變量恢復
}

void audit() {
    int total = bank.accountA + bank.accountB;
    if (total != 2000) {
        ++auditViolations;
    }
}

// -----------------------------------------------------------------------------
// 【對照組】把兩個欄位都換成 atomic：data race 消失，race condition 仍在
//   這一段用來證明「原子化每個零件 ≠ 組裝過程變原子」。
//   因為沒有 UB，這裡的觀察是有意義的：它會【真的】抓到不一致。
// -----------------------------------------------------------------------------
struct AtomicBank {
    std::atomic<int> a{1000};
    std::atomic<int> b{1000};

    void transfer(int amount) {
        a.fetch_sub(amount, std::memory_order_relaxed);   // 原子，但只是「一半」
        b.fetch_add(amount, std::memory_order_relaxed);   // 中間別人可以插進來
    }

    // 兩次獨立的原子讀 → 讀到的 a 與 b 來自不同時刻
    int auditTotal() const {
        int x = a.load(std::memory_order_relaxed);
        int y = b.load(std::memory_order_relaxed);
        return x + y;
    }
};

// -----------------------------------------------------------------------------
// 【正確版】同一把鎖同時涵蓋兩個變數
// -----------------------------------------------------------------------------
struct SafeBank {
    mutable std::mutex mtx;
    int a = 1000;
    int b = 1000;

    void transfer(int amount) {
        std::lock_guard<std::mutex> lock(mtx);
        a -= amount;
        b += amount;          // 破壞期整段在鎖內 → 不可觀察
    }

    int auditTotal() const {
        std::lock_guard<std::mutex> lock(mtx);   // 讀取端也必須用同一把鎖
        return a + b;
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 1】監控 metrics 匯出：requests 與 errors 必須同時取樣
//   情境：服務對外暴露 /metrics，Prometheus 每 15 秒抓一次。
//         處理請求的執行緒會同時更新 total 與 failed 兩個計數器。
//         若匯出端分兩次讀取，可能拿到「failed 已更新、total 尚未更新」的組合，
//         算出 failed/total > 1 的錯誤率 → 觸發假警報，
//         值班工程師半夜被叫起來查一個不存在的故障。
//   修法：匯出時用同一把鎖一次取得整組快照。
// -----------------------------------------------------------------------------
struct MetricsSnapshot {
    long total;
    long failed;
};

class Metrics {
private:
    mutable std::mutex mtx;
    long total = 0;
    long failed = 0;

public:
    void record(bool ok) {
        std::lock_guard<std::mutex> lock(mtx);
        ++total;                    // 這兩行之間不變量（failed <= total）不成立
        if (!ok) ++failed;          // 但鎖讓它不可觀察
    }

    MetricsSnapshot snapshot() const {
        std::lock_guard<std::mutex> lock(mtx);
        return MetricsSnapshot{total, failed};   // 一次取得整組，保證一致
    }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】雙分錄記帳：每筆交易必須同時寫入借方與貸方
//   情境：會計系統的鐵律是「借貸必相等」（複式簿記）。
//         每筆交易要同時在兩個科目上記帳，這兩次寫入之間帳是不平的。
//         若對帳程式在此時掃描，會報出根本不存在的差額。
//   修法：一筆交易 = 一個臨界區段；對帳掃描也用同一把鎖。
// -----------------------------------------------------------------------------
class Ledger {
private:
    mutable std::mutex mtx;
    std::map<std::string, long> accounts;

public:
    Ledger() {
        accounts["cash"]    = 100000;
        accounts["revenue"] = 0;
        accounts["expense"] = 0;
    }

    // 一筆交易：從 debit 科目移動金額到 credit 科目
    void post(const std::string& debit, const std::string& credit, long amount) {
        std::lock_guard<std::mutex> lock(mtx);
        accounts[debit]  -= amount;      // ← 帳不平的破壞期開始
        accounts[credit] += amount;      // ← 破壞期結束
    }

    // 對帳：所有科目加總必須恆為初始總額
    long totalBalance() const {
        std::lock_guard<std::mutex> lock(mtx);
        long sum = 0;
        for (const auto& kv : accounts) sum += kv.second;
        return sum;
    }
};

int main() {
    std::cout << "=== 錯誤示範：無同步的轉帳 + 稽核（data race / UB）===\n";
    {
        std::thread t1([]() {
            for (int i = 0; i < 1000; ++i) transfer(1);
        });

        std::thread t2([]() {
            for (int i = 0; i < 1000; ++i) audit();
        });

        t1.join();
        t2.join();

        std::cout << "稽核 1000 次，抓到總額異常的次數: " << auditViolations << "\n";
        std::cout << "（此數字每次執行都可能不同；為 0 只代表沒抓到，不代表沒發生）\n";
    }

    std::cout << "\n=== 對照組：兩個欄位都是 atomic（沒有 UB，但仍不一致）===\n";
    {
        AtomicBank ab;
        std::atomic<int> violations{0};

        std::thread w([&] {
            for (int i = 0; i < 200000; ++i) {
                ab.transfer(1);
                ab.transfer(-1);   // 轉回來，讓總額長期維持 2000
            }
        });
        std::thread r([&] {
            for (int i = 0; i < 200000; ++i) {
                if (ab.auditTotal() != 2000) violations.fetch_add(1);
            }
        });
        w.join();
        r.join();

        std::cout << "稽核 200000 次，讀到總額 != 2000 的次數: " << violations.load() << "\n";
        std::cout << "→ 次數每次執行都不同，但通常【大於 0】。\n";
        std::cout << "  這證明：atomic 消除了 UB，卻沒有消除跨變數的競爭條件。\n";
    }

    std::cout << "\n=== 正確版：同一把鎖涵蓋兩個變數 ===\n";
    {
        SafeBank sb;
        std::atomic<int> violations{0};

        std::thread w([&] {
            for (int i = 0; i < 200000; ++i) { sb.transfer(1); sb.transfer(-1); }
        });
        std::thread r([&] {
            for (int i = 0; i < 200000; ++i) {
                if (sb.auditTotal() != 2000) violations.fetch_add(1);
            }
        });
        w.join();
        r.join();

        std::cout << "稽核 200000 次，讀到總額 != 2000 的次數: " << violations.load()
                  << " (必定為 0)\n";
    }

    std::cout << "\n=== 日常實務 1：監控 metrics 的一致性快照 ===\n";
    {
        Metrics m;
        std::vector<std::thread> workers;
        std::atomic<int> badSamples{0};

        for (int i = 0; i < 4; ++i) {
            workers.emplace_back([&, i] {
                for (int k = 0; k < 20000; ++k) m.record((k + i) % 7 != 0);
            });
        }
        std::thread exporter([&] {
            for (int k = 0; k < 20000; ++k) {
                MetricsSnapshot s = m.snapshot();
                // 不變量：failed 不可能超過 total
                if (s.failed > s.total) badSamples.fetch_add(1);
            }
        });

        for (auto& t : workers) t.join();
        exporter.join();

        MetricsSnapshot fin = m.snapshot();
        std::cout << "總請求數: " << fin.total << ", 失敗數: " << fin.failed << "\n";
        std::cout << "匯出 20000 次快照，出現 failed > total 的次數: "
                  << badSamples.load() << " (必定為 0)\n";
    }

    std::cout << "\n=== 日常實務 2：雙分錄記帳的借貸恆等 ===\n";
    {
        Ledger ledger;
        std::atomic<int> unbalanced{0};

        std::thread poster([&] {
            for (int i = 0; i < 50000; ++i) {
                ledger.post("cash", "revenue", 10);
                ledger.post("expense", "cash", 3);
            }
        });
        std::thread auditor([&] {
            for (int i = 0; i < 50000; ++i) {
                if (ledger.totalBalance() != 100000) unbalanced.fetch_add(1);
            }
        });
        poster.join();
        auditor.join();

        std::cout << "對帳 50000 次，帳不平的次數: " << unbalanced.load()
                  << " (必定為 0)\n";
        std::cout << "最終科目總額: " << ledger.totalBalance() << " (恆為 100000)\n";
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 4.2：不變量與競爭條件3.cpp' -o invariant3
//
// 偵測資料競爭（第一段是 UB，強烈建議跑一次）:
//   g++ -std=c++17 -fsanitize=thread -g -pthread '課程 4.2：不變量與競爭條件3.cpp' -o invariant3_tsan
//   ./invariant3_tsan  → 會明確報出 transfer() 與 audit() 之間的 data race

// ⚠️ 兩個數字【每次執行都不同】，下面只是本機某一次的真實實測：
//   (1) 第一段「抓到總額異常的次數」——該段是 genuine data race → UB，
//       本機多數執行為 0（破壞期只有一道指令寬，極難撞見）。
//       為 0 只代表沒抓到，不代表沒發生，更不是標準保證值。
//   (2) 對照組「讀到總額 != 2000 的次數」——該段用 atomic 故無 UB，
//       這個觀察是有意義的，但數值隨排程浮動；本機連續三次實測為
//       91212 / 81732 / 73222，重點是它【穩定大於 0】。
// 其餘各段（正確版、metrics、雙分錄）都有鎖保護，結果為確定值。

// === 預期輸出 ===
// === 錯誤示範：無同步的轉帳 + 稽核（data race / UB）===
// 稽核 1000 次，抓到總額異常的次數: 0
// （此數字每次執行都可能不同；為 0 只代表沒抓到，不代表沒發生）
//
// === 對照組：兩個欄位都是 atomic（沒有 UB，但仍不一致）===
// 稽核 200000 次，讀到總額 != 2000 的次數: 91490
// → 次數每次執行都不同，但通常【大於 0】。
//   這證明：atomic 消除了 UB，卻沒有消除跨變數的競爭條件。
//
// === 正確版：同一把鎖涵蓋兩個變數 ===
// 稽核 200000 次，讀到總額 != 2000 的次數: 0 (必定為 0)
//
// === 日常實務 1：監控 metrics 的一致性快照 ===
// 總請求數: 80000, 失敗數: 11429
// 匯出 20000 次快照，出現 failed > total 的次數: 0 (必定為 0)
//
// === 日常實務 2：雙分錄記帳的借貸恆等 ===
// 對帳 50000 次，帳不平的次數: 0 (必定為 0)
// 最終科目總額: 100000 (恆為 100000)
