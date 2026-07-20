// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤1.cpp  —  忘記解鎖：一次把四種誤用分類清楚
// =============================================================================
//
// 【主題資訊 Information】
//   本檔示範：忘記 unlock() → 另一條執行緒的 lock() 永久阻塞
//   標頭檔：<mutex>、<thread>、<chrono>
//
//   ★ 本課四種誤用的【精確分類】（這是本檔最重要的內容）：
//   ┌────────────────────────────────┬──────────────────────┬──────────────┐
//   │ 誤用方式                       │ 標準怎麼說           │ 是不是 UB？  │
//   ├────────────────────────────────┼──────────────────────┼──────────────┤
//   │ A 忘記 unlock，【別的】執行緒  │ lock() 阻塞等待      │ 【不是】UB   │
//   │   再 lock（本檔）              │ → 永久阻塞（確定）   │ 定義明確     │
//   │ B 【同一條】執行緒對已持有的   │ 明文規定為          │ 【是】UB     │
//   │   std::mutex 再 lock（檔 2、5）│ undefined behavior   │              │
//   │ C 對【未持有】的 mutex unlock  │ 明文規定為 UB        │ 【是】UB     │
//   │ D 兩條執行緒以相反順序取兩把   │ 每個 lock 都合法，   │ 【不是】UB   │
//   │   鎖（檔 11，AB-BA）           │ 但可能循環等待       │ 且【不保證】 │
//   │                                │                      │ 一定發生     │
//   └────────────────────────────────┴──────────────────────┴──────────────┘
//   ⚠️ A、B、D 的【症狀】看起來都是「程式停住了」，但性質完全不同：
//      A 是標準保證會卡住；B 標準什麼都不保證（換平台可能是別的症狀）；
//      D 是機率事件，同一份程式碼可能今天正常、明天卡死。
//      面試時能把這三者分清楚，遠比背出「會死結」有價值。
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔屬於 A 類：標準保證的永久阻塞，不是 UB】
//   情境是：t1 取得 mtx → 沒有 unlock → t1 結束 → t2 呼叫 mtx.lock()。
//   對一把【已被其他執行緒持有】的 mutex 呼叫 lock()，
//   標準的規定就是「阻塞直到取得」——這是完全良好定義的行為。
//   問題只在於這把鎖永遠不會被釋放，所以 t2 永遠等不到。
//   → 說「這是未定義行為」是錯的；說「這一定會卡住」才對。
//
// 【2. 為什麼執行緒結束不會自動釋放它持有的鎖】
//   std::mutex 只是一個普通物件，生命週期跟著它所在的儲存區
//   （本檔是全域變數），與「誰鎖了它」毫無關係。
//   標準只要求 unlock() 必須由目前持有它的執行緒呼叫，
//   從未承諾「執行緒結束時幫你解開」。
//   作業系統層面也是如此：pthread_mutex 預設不是 robust mutex。
//   Linux 有 PTHREAD_MUTEX_ROBUST 可讓下一個等待者收到 EOWNERDEAD
//   並自行修復，但 std::mutex 不暴露這個能力。
//
// 【3. 這種 bug 在生產環境的樣貌與診斷】
//   徵狀非常特別：程式沒有 crash、沒有錯誤訊息、沒有任何日誌，
//   就只是【不動了】，而且 **CPU 使用率接近 0%**——
//   因為所有執行緒都阻塞在鎖上，沒有人在燒 CPU。
//   ⚠️ 這與「無窮迴圈導致 CPU 100%」是完全相反的徵狀，
//      光看這一點就能大幅縮小排查方向。
//   診斷手段（由快到慢）：
//     1. top / htop 看 CPU 使用率 → 接近 0 就往「全體阻塞」查
//     2. eu-stack -p <pid>  或  gdb -p <pid> 後 thread apply all bt
//        → 若多數執行緒停在 __lll_lock_wait / pthread_mutex_lock，確診
//     3. 編譯時加 -fsanitize=thread，測試環境跑，讓 TSan 提早抓出來
//
// 【4. 唯一的根治方式：不要手寫 lock/unlock】
//   人類靠紀律記得解鎖是不可靠的——尤其當函式有多條 return、
//   或某個呼叫可能拋出例外時（例外是原始碼裡看不見的分支）。
//   RAII 把解鎖綁在物件生命週期上，由編譯器保證所有離開路徑都執行：
//       std::lock_guard<std::mutex> lock(mtx);   // 就這樣，不會忘
//   本課「常見錯誤3.cpp」有完整的 RAII 示範。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼本檔的示範要用 -DDEMONSTRATE_UB 關起來：
//     開啟後程式會【永遠不結束】，整份課程的批次編譯執行會被卡死。
//     （沿用課程既有的巨集名稱；嚴格說本檔示範的是「永久阻塞」而非 UB，
//       這一點已在上面的分類表說明。）
//   * 觀察方式一定要配 timeout，否則你的終端機也會被卡住：
//       timeout 5 ./forget_unlock ; echo "exit=$?"     # 預期 exit=124
//     124 是 timeout 指令用來表示「逾時而被終止」的退出碼。
//   * 一個常被忽略的細節：即使 t1 已經結束，鎖仍然「屬於」一個
//     已經不存在的執行緒。這也是為什麼 unlock() 不能由 t2 代勞——
//     由非持有者呼叫 unlock() 是 C 類 UB。
//
// 【注意事項 Pay Attention】
//   1. 本檔的永久阻塞是【標準保證】的行為，【不是】UB——別混為一談。
//   2. 執行緒結束【不會】自動釋放它持有的 std::mutex。
//   3. unlock() 只能由持有者呼叫；由別條執行緒代為解鎖是 UB。
//   4. 症狀是「程式停住且 CPU ≈ 0%」，與忙等迴圈（CPU 100%）完全相反。
//   5. 正式程式一律用 lock_guard / unique_lock / scoped_lock。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】忘記解鎖與誤用分類
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 執行緒 A 鎖住 mutex 後就結束了（沒 unlock），
//        執行緒 B 再 lock() 這把鎖。這是未定義行為嗎？
//     答：不是。對一把【已被其他執行緒持有】的 mutex 呼叫 lock()，
//         標準規定就是「阻塞等待」——行為定義得很清楚。
//         只是這把鎖永遠不會被釋放，所以 B 會【確定地、永久地】卡住。
//         std::mutex 不會因為持有者執行緒結束而自動解鎖。
//     追問：那哪些情況才是真正的 UB？
//           → (1) 同一條執行緒對已持有的 std::mutex 再次 lock()（不可重入）；
//             (2) 對未鎖定的 mutex 呼叫 unlock()；
//             (3) 由非持有者呼叫 unlock()。這三種標準都明文規定為 UB。
//
// 🔥 Q2. 線上服務「沒回應」了，你怎麼快速判斷是死結還是無窮迴圈？
//     答：先看 CPU 使用率。接近 0% → 所有執行緒都在【阻塞】，
//         往死結／鎖洩漏／等不到的 I/O 方向查；
//         接近 100% → 執行緒都是 runnable，往無窮迴圈／活鎖／
//         忙等迴圈方向查。
//         確認方式是取堆疊：eu-stack -p <pid> 或
//         gdb -p <pid> 後 thread apply all bt，
//         看是不是多數執行緒都停在 pthread_mutex_lock。
//     追問：怎麼在上線前就抓到？→ 測試環境用 -fsanitize=thread 編譯，
//           ThreadSanitizer 對鎖的誤用與 data race 都有偵測能力。
//
// ⚠️ 陷阱. 「忘記 unlock 會造成死結（deadlock）。」這句話對嗎？
//     答：不精確。死結的定義是【兩個以上】的執行緒【互相】等待對方持有的
//         資源，形成循環等待。本檔的情況沒有循環——
//         只有一把鎖，而且持有者已經不存在了。
//         精確的說法是【鎖洩漏（lock leak）導致的永久阻塞】。
//     為什麼會錯：把所有「程式卡住」的現象都叫死結。
//         這在面試中會直接暴露對概念的理解深度：
//         死結（循環等待）、活鎖（互相禮讓）、飢餓（長期搶不到）、
//         鎖洩漏（沒人釋放）——四種現象、四種成因、四種解法，
//         症狀相似但完全不是同一回事。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔示範的是「鎖洩漏導致永久阻塞」這個【錯誤模式】與它的分類。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）全部是
//   「寫出正確同步」的題目，若真的漏了解鎖，評測系統只會回報逾時，
//   沒有任何一題在考「忘記解鎖會怎樣」。硬掛一題會模糊本檔重點，故從缺。
//   （lock_guard 的正面應用見同課「常見錯誤3.cpp」的 LeetCode 155。）
//
// -----------------------------------------------------------------------------
// 【本檔是「刻意示範錯誤」的範例，開啟示範後程式會永遠停住，不會自己結束】
//
// ⚠️ 這【不是】未定義行為：mtx 被 t1 鎖住後從未解鎖，
//    t2 在另一個執行緒對已鎖住的 mutex 呼叫 lock()，
//    依標準就是【阻塞等待】—— 而鎖永遠不會被釋放，
//    所以 t2 會【確定地、永久地】卡在 lock()，程式不會自行終止。
//    （t1 已結束，但 std::mutex 不會因為持有者執行緒結束而自動解鎖。）
//
// 為了不讓整份課程的批次執行卡死，這個示範預設【關閉】。
// 要親眼觀察永久停住，請自行加上 -DDEMONSTRATE_UB：
//    g++ -std=c++17 -pthread -DDEMONSTRATE_UB -o forget_unlock '課程 5.4：互斥鎖的常見錯誤1.cpp'
//    timeout 5 ./forget_unlock ; echo "exit=$?"   # 預期 exit=124（逾時）
//
// ✅ 正確作法：用 std::lock_guard / std::unique_lock（RAII），
//    讓解鎖由解構函式保證執行，就不可能忘記。
// -----------------------------------------------------------------------------

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;
int counter = 0;

void buggyIncrement() {
    mtx.lock();
    ++counter;
    std::cout << "Counter = " << counter << std::endl;

    // 💀 忘記 unlock()！
    // mtx.unlock();
}

// -----------------------------------------------------------------------------
// 【正確對照組】同樣的邏輯，用 RAII 就不可能漏解鎖
// -----------------------------------------------------------------------------
std::mutex safeMtx;
int safeCounter = 0;

void safeIncrement() {
    std::lock_guard<std::mutex> lock(safeMtx);   // 建構即 lock
    ++safeCounter;
}                                                // 解構即 unlock

// -----------------------------------------------------------------------------
// 【日常實務範例】用「看門狗」偵測卡住的工作執行緒
//   情境：服務裡有一組背景 worker。若某條 worker 因為鎖洩漏而永久阻塞，
//         它不會 crash、不會留下任何日誌，只會安靜地消失在監控之外——
//         直到有人發現「這個佇列怎麼一直沒被消化」。
//   對策：每條 worker 在每輪迴圈開頭更新自己的「心跳時間戳」；
//         監控執行緒定期檢查，超過門檻沒更新就判定為卡住並告警。
//   ⚠️ 這是【偵測】不是【修復】——鎖洩漏無法在程式內自我修復
//      （你不能替別條執行緒 unlock，那是 UB）。
//      看門狗的價值在於把「安靜的永久阻塞」變成「明確的告警」，
//      讓維運能及時重啟服務並取堆疊分析。
// -----------------------------------------------------------------------------
class WorkerWatchdog {
private:
    std::mutex mtx_;
    std::vector<long> lastHeartbeatMs_;   // 每條 worker 的最後心跳時間

public:
    explicit WorkerWatchdog(std::size_t workers)
        : lastHeartbeatMs_(workers, 0) {}

    static long nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void heartbeat(std::size_t workerId) {
        std::lock_guard<std::mutex> lock(mtx_);
        lastHeartbeatMs_[workerId] = nowMs();
    }

    // 回傳「疑似卡住」的 worker 數量
    int checkStalled(long thresholdMs) {
        std::lock_guard<std::mutex> lock(mtx_);
        const long now = nowMs();
        int stalled = 0;
        for (long last : lastHeartbeatMs_) {
            if (last != 0 && now - last > thresholdMs) ++stalled;
        }
        return stalled;
    }
};

int main() {
    std::thread t1(buggyIncrement);
    t1.join();

    std::cout << "執行緒 1 完成" << std::endl;

#ifdef DEMONSTRATE_UB
    std::thread t2(buggyIncrement);  // 💀 t2 會永遠卡在 mtx.lock()

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 注意：這一行【會】被印出來 —— 卡住的是 t2，不是 main。
    // （原版註解寫「這行可能不會執行」是錯的，實測一定會印出。）
    std::cout << "已等待 2 秒：t2 仍卡在 lock()，counter 停在 "
              << counter << std::endl;

    t2.join();  // 💀 永遠不會返回，程式就停在這裡

    std::cout << "（這一行才是真正永遠不會執行的）" << std::endl;
#else
    std::cout << "已略過會永久卡住的示範（預設關閉）。" << std::endl;
    std::cout << "要重現請加上 -DDEMONSTRATE_UB 重新編譯，"
                 "並用 timeout 觀察它不會自行結束。" << std::endl;
#endif

    std::cout << "\n=== 正確對照組: RAII 不可能漏解鎖 ===" << std::endl;
    {
        std::vector<std::thread> workers;
        for (int i = 0; i < 8; ++i) {
            workers.emplace_back([]() {
                for (int k = 0; k < 10000; ++k) safeIncrement();
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "8 執行緒各遞增 10000 次，結果: " << safeCounter
                  << "  (必須是 80000)" << std::endl;
        // 若任何一次漏了解鎖，上面的迴圈早就永久卡住了
        std::cout << "全部完成且鎖仍可正常取得: "
                  << (safeMtx.try_lock() ? "是" : "否") << std::endl;
        safeMtx.unlock();
    }

    std::cout << "\n=== 四種誤用的分類（本課核心）===" << std::endl;
    std::cout << "A 忘記 unlock，別的執行緒再 lock  → 永久阻塞【不是 UB】" << std::endl;
    std::cout << "B 同一執行緒重複 lock            → 【是 UB】" << std::endl;
    std::cout << "C 對未持有的 mutex unlock        → 【是 UB】" << std::endl;
    std::cout << "D 兩執行緒相反順序取兩把鎖       → AB-BA 死結，"
                 "【不是 UB】且【不保證】發生" << std::endl;

    std::cout << "\n=== 日常實務: 用看門狗偵測卡住的 worker ===" << std::endl;
    {
        WorkerWatchdog watchdog(4);

        // 4 條 worker 正常運作，每輪更新心跳
        std::vector<std::thread> workers;
        for (std::size_t i = 0; i < 4; ++i) {
            workers.emplace_back([&watchdog, i]() {
                for (int round = 0; round < 5; ++round) {
                    watchdog.heartbeat(i);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
        }
        for (auto& t : workers) t.join();

        std::cout << "全部 worker 正常運作，疑似卡住數: "
                  << watchdog.checkStalled(1000) << "  (預期 0)" << std::endl;

        // 模擬「有 worker 卡住了」：等待超過門檻後再檢查
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        std::cout << "停止心跳 120ms 後，以 100ms 為門檻檢查: "
                  << watchdog.checkStalled(100)
                  << " 條疑似卡住  (預期 4——全部都停止更新了)" << std::endl;
        std::cout << "價值: 把「安靜的永久阻塞」變成「明確的告警」，"
                     "讓維運能及時取堆疊分析" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤1.cpp' -o forget_unlock
// 觀察永久阻塞（會卡住，務必加 timeout）:
//   g++ -std=c++17 -Wall -Wextra -pthread -DDEMONSTRATE_UB '課程 5.4：互斥鎖的常見錯誤1.cpp' -o forget_unlock_ub
//   timeout 5 ./forget_unlock_ub ; echo "exit=$?"    # 本機實測 exit=124（逾時）

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 以下是【預設編譯】（不加 -DDEMONSTRATE_UB）的輸出。
//      加上 -DDEMONSTRATE_UB 後程式會停在 t2.join() 永不返回，
//      本機實測 timeout 5 觀察到 exit=124。
//   2. 看門狗的心跳時間戳是實際時間，但輸出只有「疑似卡住的數量」，
//      那是由 sleep 的時間結構決定的，穩定可驗證。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// Counter = 1
// 執行緒 1 完成
// 已略過會永久卡住的示範（預設關閉）。
// 要重現請加上 -DDEMONSTRATE_UB 重新編譯，並用 timeout 觀察它不會自行結束。
//
// === 正確對照組: RAII 不可能漏解鎖 ===
// 8 執行緒各遞增 10000 次，結果: 80000  (必須是 80000)
// 全部完成且鎖仍可正常取得: 是
//
// === 四種誤用的分類（本課核心）===
// A 忘記 unlock，別的執行緒再 lock  → 永久阻塞【不是 UB】
// B 同一執行緒重複 lock            → 【是 UB】
// C 對未持有的 mutex unlock        → 【是 UB】
// D 兩執行緒相反順序取兩把鎖       → AB-BA 死結，【不是 UB】且【不保證】發生
//
// === 日常實務: 用看門狗偵測卡住的 worker ===
// 全部 worker 正常運作，疑似卡住數: 0  (預期 0)
// 停止心跳 120ms 後，以 100ms 為門檻檢查: 4 條疑似卡住  (預期 4——全部都停止更新了)
// 價值: 把「安靜的永久阻塞」變成「明確的告警」，讓維運能及時取堆疊分析
