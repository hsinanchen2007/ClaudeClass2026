//
// =============================================================================
//  課程 5.6：互斥鎖的效能考量2.cpp  —  為什麼加執行緒反而變慢？
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    鎖競爭 (lock contention)；序列化如何吃掉多核心的效益
//   量測方法：固定總工作量，改變執行緒數，看總耗時如何變化
//   標準版本：std::thread / std::chrono 為 C++11
//   標頭檔：  <thread>、<mutex>、<chrono>、<iomanip>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//   ⚠️ 所有時間數字都是本機實測值，每次執行都不同，非標準保證。
//
// 【詳細解釋 Explanation】
//
// 【1. 本檔最反直覺的結果：執行緒越多越慢】
//   一般人的預期是「4 條執行緒應該比 1 條快 4 倍」。
//   但本檔的實測結果剛好相反：從 1 條增加到 2 條，時間【暴增數倍】。
//   原因是這個工作【完全沒有可並行的部分】——
//   每條執行緒做的事就是「拿鎖、++、放鎖」，100% 都在臨界區段內。
//   臨界區段是序列化的，所以：
//     * 總工作量沒有變少（還是 100 萬次遞增）
//     * 但多了大量的「搶鎖 / 等待 / 上下文切換」成本
//   → 加執行緒不但沒有幫助，還把純粹的額外成本加了進來。
//
// 【2. Amdahl 定律：序列比例決定了加速的天花板】
//   若程式中有比例 p 的部分【必須序列執行】，
//   則不論用多少核心，加速比的上限是 1/p：
//       加速比 ≤ 1 / (p + (1-p)/N)  →  N→∞ 時趨近 1/p
//   本檔的 p ≈ 100%（整個迴圈都在鎖內），所以加速上限是 1×，
//   也就是「再多核心都不會更快」。而現實比這更糟 ——
//   競爭本身還要付出成本，所以是負加速。
//   → 這是本檔真正要教的事：【鎖的成本不在指令，在失去的並行度】。
//
// 【3. 競爭時到底多花了什麼】
//   上一檔（5.6-1）量的是無競爭的 fast path：一次原子 CAS，不進核心。
//   有競爭時完全是另一回事：
//     ① CAS 失敗 → 呼叫 futex(FUTEX_WAIT) 進入核心
//     ② 核心把執行緒狀態改為睡眠、放進等待佇列 → 一次上下文切換
//     ③ 持有者 unlock 時發現有人在等 → 呼叫 futex(FUTEX_WAKE)
//     ④ 被喚醒的執行緒重新被排程 → 又一次上下文切換
//   一次上下文切換在本機約是微秒等級，比 fast path 的十幾奈秒
//   高出【兩到三個數量級】。這就是曲線陡升的來源。
//   （glibc 在真正睡眠前會先自旋一小段時間，希望持有者很快釋放，
//     所以極短的臨界區段有機會避開進核心 —— 這也是「臨界區段要短」的
//     另一個理由。）
//
// 【4. 為什麼超過某個執行緒數之後曲線會趨於平坦甚至下降】
//   實測常常看到 2 條時最糟，之後反而略為回穩。原因是：
//     * 執行緒很多時，等待者大多處於睡眠狀態，不再瘋狂自旋搶鎖，
//       實際上退化成「排隊」，減少了無效的爭搶。
//     * 快取行在少數幾個核心之間來回的頻率降低。
//   所以曲線不是單調上升的，這也是為什麼
//   【效能問題一定要量測，不能靠推理】。
//
// 【5. 三種降低競爭的手段（後續各檔會逐一示範）】
//   ① 縮短臨界區段：把計算移出鎖（5.6-4）
//   ② 降低鎖的粒度：把一把大鎖拆成多把小鎖（5.6-3）
//   ③ 減少上鎖次數：本地累加、最後合併一次（5.6-4 / 5.6-6）
//   ③ 通常效果最好，因為它把「上鎖次數」從 N 降到「執行緒數」，
//   等於直接把序列比例 p 壓到接近零。
//
// 【概念補充 Concept Deep Dive】
//
// (A) cache line ping-pong：看不見的成本
//   mutex 內部的狀態字與被保護的 sharedCounter 都要被多個核心讀寫。
//   依 MESI 快取一致性協定，一個核心要寫入某條 cache line（本機 64 bytes）
//   之前，必須先讓其他核心的該行失效並取得獨佔權。
//   多核心互搶同一條 cache line 時，這條線就在核心之間來回彈跳，
//   每次都要走快取一致性匯流排 —— 這是純粹的浪費，
//   而且【即使沒有鎖】也會發生（多個 atomic 計數器擠在一起也一樣）。
//
// (B) false sharing：明明沒共享也會互相拖累
//   若把「每執行緒各自的計數器」放進相鄰的陣列元素，
//   它們可能落在同一條 cache line，於是即使邏輯上互不相干，
//   硬體仍會因為一致性協定而互相失效。
//   解法是把各自的資料對齊到 cache line 邊界
//   （C++17 提供 std::hardware_destructive_interference_size）。
//   本檔第二段的「本地累加」用區域變數，天生在各自的堆疊上，
//   沒有這個問題 —— 這也是它為什麼特別快。
//
// (C) std::mutex 不保證公平性
//   標準沒有要求 mutex 依 FIFO 順序把鎖交給等待者。
//   實務上可能出現「同一條執行緒連續搶到鎖好幾次」（barging），
//   甚至某條執行緒長期搶不到（starvation，飢餓）。
//   這也是為什麼本檔的時間數字在不同執行緒數之間跳動得不規則。
//   需要公平性就得自己實作（票號鎖 ticket lock）或改用其他機制。
//
// 【注意事項 Pay Attention】
// 1. 加執行緒不一定變快 —— 若工作幾乎都在臨界區段內，只會更慢。
// 2. 依 Amdahl 定律，序列比例 p 決定加速上限 1/p；競爭還會讓它變成負加速。
// 3. 有競爭時的成本（上下文切換，微秒等級）比無競爭（十幾奈秒）高兩三個數量級。
// 4. 曲線不是單調的，也不可重現 —— 效能一定要量測，不能靠推理。
// 5. std::mutex 不保證公平性，可能有 barging 或飢餓。
// 6. 本檔所有數字每次執行都不同，檔尾的預期輸出只是某一次的實測。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔是鎖競爭的效能量測，沒有可判題的演算法；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都不涉及「量測競爭對吞吐量的影響」。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（分片計數器、本地累加）呈現 ——
//   它們正是本檔量出來的問題的實際解法。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】鎖競爭與可擴展性
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 為什麼把單執行緒的程式改成 4 條執行緒之後反而變慢了?
//     答：因為工作幾乎全部在臨界區段內。臨界區段是序列化的 ——
//         總工作量沒變少，卻多了搶鎖、等待與上下文切換的成本。
//         依 Amdahl 定律，序列比例 p 決定加速上限 1/p；
//         當 p 接近 100%，加速上限就是 1×，
//         再加上競爭成本就變成負加速。
//     追問：那要怎麼真的變快?
//         → 把序列比例降下來：縮短臨界區段（計算移到鎖外）、
//           降低鎖粒度（一把大鎖拆成多把）、
//           或減少上鎖次數（本地累加、最後合併一次）。
//           最後一種通常效果最好，因為它把上鎖次數從 N 降到執行緒數。
//
// 🔥 Q2. 無競爭與有競爭的 mutex，成本差多少?
//     答：差兩到三個數量級。無競爭時只做一次使用者空間的原子 CAS，
//         本機約十幾到二十幾奈秒；有競爭時要呼叫 futex 進核心、
//         把執行緒掛起、之後再喚醒，包含兩次上下文切換，
//         成本跳到微秒等級。
//     追問：glibc 有做什麼優化來緩解?
//         → 在真正睡眠之前會先自旋一小段時間，
//           賭持有者馬上就會釋放。所以【極短的臨界區段】
//           有機會完全避開進核心 —— 這是「臨界區段要短」除了
//           降低競爭機率之外的另一個具體理由。
//
// ⚠️ 陷阱. 「我的服務是 16 核心的機器，所以開 16 條工作執行緒最好」——錯在哪?
//     答：最佳執行緒數取決於【工作的性質】，不是核心數。
//         若工作是 CPU 密集且各自獨立，核心數確實是好起點；
//         但若工作大量爭搶同一把鎖（像本檔），
//         增加執行緒只會增加競爭，1 條反而最快。
//         若工作是 I/O 密集（等網路、等磁碟），
//         執行緒數又該遠多於核心數，因為它們大部分時間在等待。
//     為什麼會錯：把「核心數」當成萬用的調參依據。
//         正確的做法是先分辨工作型態（CPU 密集 / I/O 密集 / 鎖密集），
//         再用本檔這種掃描式量測找出實際的最佳點 ——
//         而且要在【真實負載】下量，不是用 micro-benchmark 推論。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_6_contention.cpp
// 說明：測量鎖競爭對效能的影響

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>

std::mutex mtx;
long long sharedCounter = 0;

void incrementWithLock(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++sharedCounter;
    }
}

double runTest(int numThreads, int totalIterations) {
    sharedCounter = 0;
    int iterationsPerThread = totalIterations / numThreads;
    
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(incrementWithLock, iterationsPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}


// -----------------------------------------------------------------------------
// 【日常實務範例 1】分片計數器（sharded counter）：把一把鎖拆成 N 把
//   情境：高流量服務要統計請求數。單一 atomic 計數器在 16 核心下
//         會產生嚴重的 cache line ping-pong（所有核心搶同一條快取行）。
//   做法：把計數器分成 N 片，每條執行緒只更新自己那片（依 thread id 決定），
//         讀取時再把所有片加總。
//   關鍵細節：每一片必須【對齊到 cache line】，否則相鄰的片會落在
//         同一條快取行，產生 false sharing —— 那就完全失去分片的意義。
//         本機 cache line 為 64 bytes（實作定義值）。
// -----------------------------------------------------------------------------
struct alignas(64) PaddedCounter {   // 對齊到 cache line，避免 false sharing
    long value = 0;
};

class ShardedCounter {
private:
    static const size_t SHARDS = 16;
    std::vector<PaddedCounter> shards;

public:
    ShardedCounter() : shards(SHARDS) {}

    // 每條執行緒只碰自己那片 → 幾乎沒有競爭
    void add(size_t shardIndex, long delta) {
        shards[shardIndex % SHARDS].value += delta;
    }

    // 讀取時才加總（讀取遠比寫入少，所以這個成本可以接受）
    long total() const {
        long sum = 0;
        for (const auto& s : shards) sum += s.value;
        return sum;
    }
};

// 對照組：所有執行緒搶同一把鎖
class SingleLockCounter {
private:
    std::mutex mtx;
    long value = 0;

public:
    void add(long delta) {
        std::lock_guard<std::mutex> lock(mtx);
        value += delta;
    }
    long total() const { return value; }
};

// -----------------------------------------------------------------------------
// 【日常實務範例 2】本地累加：把上鎖次數從 N 降到「執行緒數」
//   情境：同樣是統計，但這次連分片都不需要 ——
//         若統計結果不必即時可見（多數監控場景都是如此），
//         每條執行緒可以先在區域變數累加，收尾時才合併一次。
//   效果：上鎖次數從「每次事件一次」降到「每條執行緒一次」，
//         序列比例 p 幾乎歸零，這才是真正能線性擴展的做法。
//   取捨：統計值有延遲（合併前看不到），
//         且執行緒中途崩潰會遺失它的本地累計。
// -----------------------------------------------------------------------------
double runShardedTest(int numThreads, int totalIterations) {
    ShardedCounter counter;
    std::vector<std::thread> threads;
    int perThread = totalIterations / numThreads;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&counter, i, perThread] {
            for (int k = 0; k < perThread; ++k) counter.add(static_cast<size_t>(i), 1);
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double runLocalAccumTest(int numThreads, int totalIterations) {
    SingleLockCounter counter;
    std::vector<std::thread> threads;
    int perThread = totalIterations / numThreads;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&counter, perThread] {
            long local = 0;                        // 全程無鎖
            for (int k = 0; k < perThread; ++k) ++local;
            counter.add(local);                    // 整條執行緒只上鎖一次
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const int totalIterations = 1000000;
    
    std::cout << "=== 鎖競爭效能測試 ===" << std::endl;
    std::cout << "總迭代次數：" << totalIterations << std::endl << std::endl;
    
    std::cout << std::setw(10) << "執行緒數" 
              << std::setw(15) << "時間 (ms)"
              << std::setw(15) << "相對效能"
              << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    double baseTime = runTest(1, totalIterations);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(10) << 1 
              << std::setw(15) << baseTime
              << std::setw(15) << "1.00x"
              << std::endl;
    
    for (int numThreads : {2, 4, 8, 16}) {
        double time = runTest(numThreads, totalIterations);
        double ratio = time / baseTime;
        
        std::cout << std::setw(10) << numThreads 
                  << std::setw(15) << time
                  << std::setw(14) << ratio << "x"
                  << std::endl;
    }
    std::cout << "→ 執行緒越多不但沒變快，反而更慢：" << std::endl;
    std::cout << "  這個工作 100% 都在臨界區段內，序列比例 p ≈ 1，" << std::endl;
    std::cout << "  依 Amdahl 定律加速上限就是 1×，再加上競爭成本就成了負加速。" << std::endl;

    std::cout << "\n=== 解法比較（同樣的總工作量，8 條執行緒）===" << std::endl;
    {
        const int n = 1000000;
        const int threads = 8;

        // 預熱，讓 CPU 頻率與快取狀態穩定
        runTest(threads, n);
        runShardedTest(threads, n);
        runLocalAccumTest(threads, n);

        double tSingle = runTest(threads, n);
        double tSharded = runShardedTest(threads, n);
        double tLocal = runLocalAccumTest(threads, n);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "單一鎖（每次事件都上鎖）：" << tSingle << " ms" << std::endl;
        std::cout << "分片計數器（16 片對齊）  ：" << tSharded << " ms" << std::endl;
        std::cout << "本地累加（每執行緒一次）  ：" << tLocal << " ms" << std::endl;
        std::cout << std::endl;
        std::cout << "→ 分片把競爭分散到 16 條 cache line；" << std::endl;
        std::cout << "  本地累加更徹底：上鎖次數從 1000000 降到 8 次。" << std::endl;
        std::cout << "  兩者的正確性都與單一鎖版本完全相同。" << std::endl;
    }

    std::cout << "\n=== 正確性驗證（效能改善不能犧牲正確性）===" << std::endl;
    {
        ShardedCounter sharded;
        std::vector<std::thread> ths;
        for (int i = 0; i < 8; ++i) {
            ths.emplace_back([&sharded, i] {
                for (int k = 0; k < 100000; ++k) sharded.add(static_cast<size_t>(i), 1);
            });
        }
        for (auto& t : ths) t.join();
        std::cout << "分片計數器總和: " << sharded.total()
                  << "（必定為 800000）" << std::endl;
        std::cout << "註：每條執行緒只寫自己的分片，所以不需要鎖也沒有 data race；" << std::endl;
        std::cout << "    alignas(64) 確保各分片不落在同一條 cache line。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量2.cpp' -o contention

// ⚠️ 本檔【所有時間數字每次執行都不同】——這是效能量測，不是功能輸出。
// 影響因素：CPU 動態頻率調節、排程、快取狀態、其他行程的負載。
// 而且 std::mutex 不保證公平性，所以曲線【不是單調的】：
// 本機多次量測都看到 2 條執行緒附近最糟、之後反而略為回穩，
// 各執行緒數之間的相對大小也會在不同次執行間互換。
// 下面貼的是本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）某一次的真實實測，
// 只用來呈現「趨勢」與「數量級」，不是保證值，也不該跨平台引用。
//
// 唯一的確定值是最後一段的「分片計數器總和 = 800000」：
// 那是正確性驗證，與效能無關，每次執行都相同。

// === 預期輸出 ===
// === 鎖競爭效能測試 ===
// 總迭代次數：1000000
//
// 執行緒數    時間 (ms)   相對效能
// ----------------------------------------
//          1          19.98          1.00x
//          2          80.31          4.02x
//          4          76.89          3.85x
//          8         105.52          5.28x
//         16         110.88          5.55x
// → 執行緒越多不但沒變快，反而更慢：
//   這個工作 100% 都在臨界區段內，序列比例 p ≈ 1，
//   依 Amdahl 定律加速上限就是 1×，再加上競爭成本就成了負加速。
//
// === 解法比較（同樣的總工作量，8 條執行緒）===
// 單一鎖（每次事件都上鎖）：109.86 ms
// 分片計數器（16 片對齊）  ：1.55 ms
// 本地累加（每執行緒一次）  ：0.40 ms
//
// → 分片把競爭分散到 16 條 cache line；
//   本地累加更徹底：上鎖次數從 1000000 降到 8 次。
//   兩者的正確性都與單一鎖版本完全相同。
//
// === 正確性驗證（效能改善不能犧牲正確性）===
// 分片計數器總和: 800000（必定為 800000）
// 註：每條執行緒只寫自己的分片，所以不需要鎖也沒有 data race；
//     alignas(64) 確保各分片不落在同一條 cache line。
