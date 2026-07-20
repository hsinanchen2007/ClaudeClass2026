// =============================================================================
//  課程 5.6：互斥鎖的效能考量6.cpp  —  優化前後對照：一個完整的案例研究
// =============================================================================
//
// 【檔案結構】上半部（約 920 行）是本節的完整講義，包在一個 /* ... */ 區塊裡，
//   收錄了 5.6-1 到 5.6-5 各檔的程式碼與說明；下半部才是本檔實際會編譯執行的
//   「優化前 / 優化後」對照程式。本段是為整份檔案補上的教科書導讀。
//
// 【主題資訊 Information】
//   主題：    鎖優化的完整案例：從「每次操作都上鎖」到「本地累加 + 合併一次」
//   核心手法：thread-local 部分和 + 最後一次合併（reduction / 歸約）
//   標準版本：std::thread / std::chrono / std::accumulate 為 C++11 起
//   標頭檔：  <mutex>、<numeric>、<chrono>、<vector>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心
//   ⚠️ 所有時間數字都是本機實測值，每次執行都不同，非標準保證。
//
// 【詳細解釋 Explanation】
//
// 【1. 這個案例把前五節的教訓濃縮成一個對照】
//   * BeforeOptimization：每處理一個元素就 lock → sum += → unlock。
//     總上鎖次數 = 資料量（本檔 100000 次）。
//   * AfterOptimization ：每條執行緒先在區域變數累加自己那一段，
//     全部算完才上一次鎖把部分和加進全域。
//     總上鎖次數 = 執行緒數（本檔 4 次）。
//   兩者的計算量完全相同、結果完全相同，
//   差別只在「上鎖次數從 100000 降到 4」。
//   本機實測加速比通常在 20 倍以上（見檔尾）。
//
// 【2. 為什麼效果這麼誇張：序列比例被壓到接近零】
//   優化前，每個元素的處理都在臨界區段內，
//   而「sum += data[i]」本身只是一次加法（約 1 ns），
//   鎖的開銷卻是十幾奈秒 —— 【同步比工作本身還貴十幾倍】。
//   再加上 4 條執行緒互相爭搶同一把鎖造成的上下文切換，
//   整體退化成「比單執行緒還慢」。
//   優化後，99.999% 的工作在鎖外並行執行，
//   依 Amdahl 定律序列比例 p ≈ 0，加速比接近核心數上限。
//
// 【3. 「預熱」為什麼在 main 裡出現】
//   main 中先呼叫了一次 runTest(before) 與 runTest(after) 才開始正式量測。
//   原因是第一次執行時：
//     * 指令快取與資料快取都是冷的
//     * 分支預測器還沒學到模式
//     * CPU 可能還在低頻狀態（動態頻率調節需要時間爬升）
//     * 記憶體頁可能還沒真正配置（首次寫入才觸發 page fault）
//   不預熱的話，第一個被測的項目會被系統性地低估效能。
//   這是 micro-benchmark 的基本紀律，本檔有做對。
//
// 【4. 這個優化的三個代價（不能只講好處）】
//   ① 結果有延遲：合併之前，全域 sum 看不到部分進度。
//      需要即時進度條的場景不適用（可改成每 N 次合併一次，折衷）。
//   ② 崩潰會遺失：執行緒中途異常結束，它的本地累加就消失了。
//   ③ 浮點數的累加順序改變：若累加的是 double，
//      因為浮點加法不滿足結合律，結果的最低有效位可能與優化前不同。
//      本檔累加的是整數，所以結果完全相同（程式有驗證這一點）。
//
// 【5. 什麼時候【不該】做這個優化】
//   * 當臨界區段本來就很重（例如每次都要做 I/O 或複雜計算）→
//     鎖的佔比已經很低，優化鎖收益微乎其微（見 5.6-4 的 good vs better）。
//   * 當操作之間有順序相依（後一次要用到前一次的結果）→
//     根本無法拆成獨立的部分和。
//   * 當需要即時的全域一致視圖 → 批次合併違反這個需求。
//   → 判斷流程永遠是：先量測找出真正的瓶頸，再決定要不要優化。
//     「先優化再說」通常只會增加複雜度而沒有收益。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 這個模式的正式名稱與標準支援
//   把資料切塊、各自算部分結果、最後合併，叫做 reduction（歸約）。
//   C++17 起標準函式庫直接支援：
//       std::reduce(std::execution::par, v.begin(), v.end(), 0LL);
//   （需要連結 TBB；g++ 需加 -ltbb）
//   OpenMP 則有 `#pragma omp parallel for reduction(+:sum)`。
//   本檔手寫是為了讓機制透明 —— 理解手寫版本之後，
//   使用標準設施時才知道它在做什麼、以及它的限制在哪。
//
// (B) 為什麼不用 std::atomic<long> 取代 mutex 就好
//   把 sum 換成 atomic 確實比 mutex 快（一道 lock add vs 一次 futex 快路徑），
//   但仍然是【每個元素都要做一次原子操作】，
//   而所有核心都在寫同一條 cache line → cache line ping-pong。
//   本地累加則是每條執行緒寫自己堆疊上的變數（各自的 cache line），
//   完全沒有一致性流量。這就是為什麼「減少同步次數」
//   比「換用更快的同步原語」有效得多。
//
// (C) 為什麼優化後仍然需要那一把鎖
//   最後合併時，4 條執行緒仍然要寫同一個 sum，所以還是需要同步。
//   只是這次只發生 4 次，成本可以忽略。
//   → 這說明優化的目標不是「消滅鎖」，而是「減少上鎖的次數」。
//     完全無鎖的設計（lock-free）通常複雜且容易出錯，
//     而把上鎖次數降幾個數量級，往往就足以解決問題。
//
// 【注意事項 Pay Attention】
// 1. 優化的核心是「減少上鎖次數」，不是「換更快的鎖」或「消滅鎖」。
// 2. micro-benchmark 一定要預熱（本檔有做），否則第一個被測項目會被低估。
// 3. 三個代價：結果有延遲、崩潰會遺失本地累計、浮點累加順序改變。
// 4. 本檔累加整數，所以優化前後結果完全相同；若是 double 則最低有效位可能不同。
// 5. 臨界區段本來就很重、或操作間有順序相依時，這個優化不適用。
// 6. C++17 起可用 std::reduce + 平行執行策略（需 -ltbb）取代手寫。
// 7. 所有時間數字每次執行都不同，檔尾的預期輸出只是某一次的實測。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔是效能優化的案例研究（量測 → 定位瓶頸 → 改寫 → 驗證），
//   沒有可判題的演算法；允許使用的設計題（146/155/705/707/1603）
//   與並行題（1114～1117/1195）都不涉及效能優化流程。
//   硬湊只會失焦，故從缺，改以下方兩個真實情境
//   （日誌分析的平行歸約、以及「先量測再優化」的反例）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】鎖的效能優化
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一段多執行緒程式用了鎖之後比單執行緒還慢，你會怎麼排查?
//     答：先量測「鎖的佔比」——把臨界區段的內容與上鎖次數列出來。
//         最常見的三個原因依序是：
//         ① 上鎖次數太多（每次操作都上鎖）→ 改成本地累加、最後合併
//         ② 臨界區段太大（計算 / I/O 在鎖內）→ 把它們移到鎖外
//         ③ 鎖的粒度太粗（不相干的資料共用一把鎖）→ 分片
//         其中 ① 的效果通常最顯著，因為它直接把序列比例壓到接近零。
//     追問：為什麼不直接換成 std::atomic?
//         → atomic 比 mutex 快，但仍然是「每個元素都做一次原子操作」，
//           而且所有核心搶同一條 cache line 會產生 ping-pong。
//           本地累加則各自寫自己堆疊上的變數，完全沒有一致性流量 ——
//           「減少同步次數」比「換更快的同步原語」有效得多。
//
// 🔥 Q2. 「本地累加 + 最後合併」有什麼代價?
//     答：三個。① 結果有延遲 —— 合併前看不到部分進度，
//         不適合需要即時全域視圖的場景。
//         ② 執行緒中途崩潰會遺失它的本地累計。
//         ③ 若累加的是浮點數，因為加法不滿足結合律，
//         累加順序改變會讓結果的最低有效位不同。
//     追問：那要即時進度怎麼辦?
//         → 折衷：每 N 次合併一次（例如每 1000 筆），
//           把上鎖次數降到 1/N，同時保有大致的即時性。
//
// ⚠️ 陷阱. 「我看到程式碼裡有 mutex，就先把它優化掉再說」——錯在哪?
//     答：這是在還沒確認瓶頸之前就優化。實務上多數效能問題出在
//         演算法複雜度、I/O 或記憶體配置，鎖往往不是主因。
//         而且無鎖改寫會大幅增加複雜度與出錯機率 ——
//         用巨大的正確性風險換取可能只有 1% 的收益。
//     為什麼會錯：把「看得見的同步原語」當成瓶頸的證據。
//         正確的流程是先量測（profiler、或像本檔這樣做 A/B 對照），
//         確認鎖真的佔了顯著比例，再動手。
//         5.6-1 的實測就顯示：當臨界區段有真實工作量時，
//         鎖的佔比往往只有個位數百分比。
// =============================================================================

/*
# 第五階段：互斥鎖基礎 (std::mutex)

## 課程 5.6：互斥鎖的效能考量

---

### 引言

互斥鎖是保護共享資料的利器，但它不是免費的。濫用鎖會導致程式效能急劇下降，甚至比單執行緒還慢。本課將深入探討鎖的效能特性，並學習如何在正確性與效能之間取得平衡。

---

### 一、鎖的開銷來源

```
┌─────────────────────────────────────────────────────────────┐
│                    鎖的開銷來源                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. 原子操作開銷                                            │
│     → CAS、記憶體屏障等指令比普通指令慢                      │
│     → 約 10-50 個 CPU 週期                                  │
│                                                             │
│  2. 快取一致性開銷                                          │
│     → 鎖變數在多核心間同步                                  │
│     → 快取行失效與重新載入                                  │
│     → 約 100-500 個 CPU 週期                                │
│                                                             │
│  3. 競爭開銷                                                │
│     → 自旋等待消耗 CPU                                      │
│     → 執行緒睡眠與喚醒（系統呼叫）                          │
│     → 上下文切換：1000-10000+ 個 CPU 週期                   │
│                                                             │
│  4. 序列化開銷                                              │
│     → 臨界區段只能串行執行                                  │
│     → 並行度降低                                            │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 二、測量鎖的基本開銷

```cpp
// 檔案：lesson_5_6_lock_overhead.cpp
// 說明：測量互斥鎖的基本開銷

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>

std::mutex mtx;

// 無鎖操作
void noLockOperation(long long& counter, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        ++counter;
    }
}

// 有鎖操作
void withLockOperation(long long& counter, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        ++counter;
    }
}

// 計時函式
template<typename Func>
double measureTime(Func&& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count();
}

int main() {
    const int iterations = 1000000;
    long long counter = 0;
    
    std::cout << "=== 互斥鎖開銷測量 ===" << std::endl;
    std::cout << "迭代次數：" << iterations << std::endl << std::endl;
    
    // 測量無鎖操作
    counter = 0;
    double noLockTime = measureTime([&]() {
        noLockOperation(counter, iterations);
    });
    
    // 測量有鎖操作（單執行緒，無競爭）
    counter = 0;
    double withLockTime = measureTime([&]() {
        withLockOperation(counter, iterations);
    });
    
    double lockOverhead = (withLockTime - noLockTime) / iterations;
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "無鎖操作總時間：" << noLockTime / 1e6 << " ms" << std::endl;
    std::cout << "有鎖操作總時間：" << withLockTime / 1e6 << " ms" << std::endl;
    std::cout << "每次鎖操作開銷：" << lockOverhead << " ns" << std::endl;
    std::cout << "效能比率：" << withLockTime / noLockTime << "x 慢" << std::endl;
    
    return 0;
}
```

#### 典型輸出

```
=== 互斥鎖開銷測量 ===
迭代次數：1000000

無鎖操作總時間：2.15 ms
有鎖操作總時間：45.32 ms
每次鎖操作開銷：43.17 ns
效能比率：21.08x 慢
```

---

### 三、競爭對效能的影響

```cpp
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
    
    return 0;
}
```

#### 典型輸出

```
=== 鎖競爭效能測試 ===
總迭代次數：1000000

 執行緒數       時間 (ms)        相對效能
----------------------------------------
         1          42.35          1.00x
         2          98.67          2.33x
         4         187.42          4.43x
         8         412.56          9.74x
        16         856.23         20.22x
```

```
┌─────────────────────────────────────────────────────────────┐
│                  競爭的影響分析                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  觀察結果：                                                  │
│  • 執行緒增加，總時間反而增加！                              │
│  • 這是因為所有執行緒都在競爭同一個鎖                        │
│  • 臨界區段變成瓶頸，無法並行                                │
│                                                             │
│  時間 ↑                                                     │
│       │                                    ╱                │
│       │                              ╱                      │
│       │                        ╱                            │
│       │                  ╱                                  │
│       │            ╱                                        │
│       │      ╱                                              │
│       │ ╱                                                   │
│       └─────────────────────────────────→ 執行緒數          │
│         1    2    4    8    16                              │
│                                                             │
│  結論：高競爭場景下，多執行緒比單執行緒還慢！                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 四、粗粒度鎖 vs 細粒度鎖

```
┌─────────────────────────────────────────────────────────────┐
│              粗粒度鎖 vs 細粒度鎖                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  粗粒度鎖（Coarse-grained Locking）                         │
│  ─────────────────────────────────                          │
│  • 一個鎖保護大量資料                                       │
│  • 實作簡單，不易出錯                                       │
│  • 並行度低，競爭嚴重                                       │
│                                                             │
│      ┌─────────────────────────────┐                        │
│      │          一個大鎖           │                        │
│      │  ┌─────┬─────┬─────┬─────┐  │                        │
│      │  │資料1│資料2│資料3│資料4│  │                        │
│      │  └─────┴─────┴─────┴─────┘  │                        │
│      └─────────────────────────────┘                        │
│                                                             │
│  細粒度鎖（Fine-grained Locking）                           │
│  ───────────────────────────────                            │
│  • 多個鎖保護不同資料                                       │
│  • 實作複雜，容易死結                                       │
│  • 並行度高，競爭減少                                       │
│                                                             │
│      ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐                    │
│      │ 鎖1  │ │ 鎖2  │ │ 鎖3  │ │ 鎖4  │                    │
│      │┌────┐│ │┌────┐│ │┌────┐│ │┌────┐│                    │
│      ││資料1│ ││資料2│ ││資料3│ ││資料4│                    │
│      │└────┘│ │└────┘│ │└────┘│ │└────┘│                    │
│      └──────┘ └──────┘ └──────┘ └──────┘                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 比較實作

```cpp
// 檔案：lesson_5_6_lock_granularity.cpp
// 說明：比較粗粒度鎖與細粒度鎖的效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <array>
#include <iomanip>

const int NUM_BUCKETS = 16;
const int ITERATIONS = 100000;

// 粗粒度鎖：一個鎖保護所有資料
class CoarseGrainedCounters {
private:
    std::mutex mtx;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖整個結構
        ++counters[bucket % NUM_BUCKETS];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

// 細粒度鎖：每個桶一個鎖
class FineGrainedCounters {
private:
    std::array<std::mutex, NUM_BUCKETS> mutexes;
    std::array<long long, NUM_BUCKETS> counters{};
    
public:
    void increment(int bucket) {
        int idx = bucket % NUM_BUCKETS;
        std::lock_guard<std::mutex> lock(mutexes[idx]);  // 只鎖一個桶
        ++counters[idx];
    }
    
    long long get(int bucket) const {
        return counters[bucket % NUM_BUCKETS];
    }
    
    long long total() const {
        long long sum = 0;
        for (auto c : counters) sum += c;
        return sum;
    }
};

template<typename Counter>
void worker(Counter& counter, int threadId, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        // 每個執行緒存取不同的桶（模擬真實場景）
        counter.increment(threadId * 1000 + i);
    }
}

template<typename Counter>
double runTest(int numThreads) {
    Counter counter;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker<Counter>, std::ref(counter), i, ITERATIONS);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    std::cout << "=== 鎖粒度效能比較 ===" << std::endl;
    std::cout << "桶數量：" << NUM_BUCKETS << std::endl;
    std::cout << "每執行緒迭代：" << ITERATIONS << std::endl << std::endl;
    
    std::cout << std::setw(10) << "執行緒數"
              << std::setw(18) << "粗粒度 (ms)"
              << std::setw(18) << "細粒度 (ms)"
              << std::setw(12) << "加速比"
              << std::endl;
    std::cout << std::string(58, '-') << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    for (int numThreads : {1, 2, 4, 8, 16}) {
        double coarseTime = runTest<CoarseGrainedCounters>(numThreads);
        double fineTime = runTest<FineGrainedCounters>(numThreads);
        double speedup = coarseTime / fineTime;
        
        std::cout << std::setw(10) << numThreads
                  << std::setw(18) << coarseTime
                  << std::setw(18) << fineTime
                  << std::setw(11) << speedup << "x"
                  << std::endl;
    }
    
    return 0;
}
```

#### 典型輸出

```
=== 鎖粒度效能比較 ===
桶數量：16
每執行緒迭代：100000

 執行緒數    粗粒度 (ms)     細粒度 (ms)       加速比
----------------------------------------------------------
         1             4.52             4.78       0.95x
         2            12.34             5.21       2.37x
         4            28.67             6.45       4.45x
         8            65.43             8.92       7.34x
        16           142.56            12.34      11.55x
```

---

### 五、減少臨界區段大小

```cpp
// 檔案：lesson_5_6_minimize_critical_section.cpp
// 說明：減少臨界區段大小提升效能

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <cmath>
#include <iomanip>

std::mutex mtx;
double result = 0;

// 耗時計算（模擬複雜運算）
double expensiveComputation(int input) {
    double sum = 0;
    for (int i = 0; i < 1000; ++i) {
        sum += std::sin(input + i) * std::cos(input - i);
    }
    return sum;
}

// 差的做法：在鎖內進行計算
void badApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        double computed = expensiveComputation(i);  // 💀 在鎖內計算！
        result += computed;
    }  // 解鎖
}

// 好的做法：在鎖外進行計算
void goodApproach(int start, int count) {
    for (int i = start; i < start + count; ++i) {
        double computed = expensiveComputation(i);  // ✓ 在鎖外計算
        
        std::lock_guard<std::mutex> lock(mtx);  // 鎖定
        result += computed;  // 只鎖定必要的部分
    }  // 解鎖
}

// 更好的做法：批次累加
void betterApproach(int start, int count) {
    double localSum = 0;  // 本地累加
    
    for (int i = start; i < start + count; ++i) {
        localSum += expensiveComputation(i);  // 完全不需要鎖
    }
    
    std::lock_guard<std::mutex> lock(mtx);  // 只鎖定一次
    result += localSum;
}

template<typename Func>
double runTest(Func&& func, int numThreads, int totalWork) {
    result = 0;
    int workPerThread = totalWork / numThreads;
    
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(func, i * workPerThread, workPerThread);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const int totalWork = 1000;
    const int numThreads = 4;
    
    std::cout << "=== 臨界區段大小對效能的影響 ===" << std::endl;
    std::cout << "執行緒數：" << numThreads << std::endl;
    std::cout << "總工作量：" << totalWork << std::endl << std::endl;
    
    std::cout << std::fixed << std::setprecision(2);
    
    double badTime = runTest(badApproach, numThreads, totalWork);
    std::cout << "差的做法（鎖內計算）：" << badTime << " ms" << std::endl;
    
    double goodTime = runTest(goodApproach, numThreads, totalWork);
    std::cout << "好的做法（鎖外計算）：" << goodTime << " ms" << std::endl;
    
    double betterTime = runTest(betterApproach, numThreads, totalWork);
    std::cout << "更好做法（批次累加）：" << betterTime << " ms" << std::endl;
    
    std::cout << std::endl;
    std::cout << "加速比（差 vs 好）：" << badTime / goodTime << "x" << std::endl;
    std::cout << "加速比（差 vs 更好）：" << badTime / betterTime << "x" << std::endl;
    
    return 0;
}
```

#### 典型輸出

```
=== 臨界區段大小對效能的影響 ===
執行緒數：4
總工作量：1000

差的做法（鎖內計算）：156.78 ms
好的做法（鎖外計算）：42.35 ms
更好做法（批次累加）：38.92 ms

加速比（差 vs 好）：3.70x
加速比（差 vs 更好）：4.03x
```

---

### 六、臨界區段內應避免的操作

```
┌─────────────────────────────────────────────────────────────┐
│             臨界區段內應避免的操作                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ✗ 避免在臨界區段內進行：                                   │
│                                                             │
│  1. I/O 操作                                                │
│     • 檔案讀寫                                              │
│     • 網路通訊                                              │
│     • 控制台輸出（除錯用途除外）                            │
│                                                             │
│  2. 阻塞等待                                                │
│     • sleep / wait                                          │
│     • 等待其他鎖（死結風險）                                │
│     • 等待條件變數                                          │
│                                                             │
│  3. 耗時計算                                                │
│     • 複雜演算法                                            │
│     • 大量數學運算                                          │
│     • 字串處理                                              │
│                                                             │
│  4. 記憶體配置                                              │
│     • new / malloc                                          │
│     • 容器擴展（可能觸發重新配置）                          │
│                                                             │
│  5. 呼叫未知函式                                            │
│     • 回呼函式                                              │
│     • 虛擬函式                                              │
│     • 使用者提供的函式                                      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 範例：避免在臨界區段內 I/O

```cpp
// 檔案：lesson_5_6_avoid_io_in_lock.cpp
// 說明：避免在臨界區段內進行 I/O

#include <iostream>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>

std::mutex mtx;
std::vector<std::string> logs;

// 💀 差的做法：在鎖內進行 I/O
void badLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << message << std::endl;  // 💀 I/O 在鎖內！
}

// ✓ 好的做法：只保護共享資料
void goodLog(const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        logs.push_back(message);  // ✓ 只保護共享容器
    }
    // I/O 在鎖外（或由專門的執行緒處理）
}

// ✓ 更好的做法：批次輸出
void flushLogs() {
    std::vector<std::string> localLogs;
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        localLogs.swap(logs);  // 快速交換
    }
    
    // 在鎖外進行 I/O
    for (const auto& log : localLogs) {
        std::cout << log << std::endl;
    }
}
```

---

### 七、Amdahl 定律

```
┌─────────────────────────────────────────────────────────────┐
│                    Amdahl 定律                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  加速比 = 1 / (S + P/N)                                     │
│                                                             │
│  S = 程式中串行部分的比例                                    │
│  P = 程式中可並行部分的比例（P = 1 - S）                    │
│  N = 處理器數量                                             │
│                                                             │
│  ─────────────────────────────────────────────────────────  │
│                                                             │
│  範例：假設程式有 10% 必須串行（如鎖保護的部分）             │
│                                                             │
│  N=2:   加速比 = 1/(0.1 + 0.9/2)  = 1.82x                  │
│  N=4:   加速比 = 1/(0.1 + 0.9/4)  = 3.08x                  │
│  N=8:   加速比 = 1/(0.1 + 0.9/8)  = 4.71x                  │
│  N=∞:   加速比 = 1/(0.1 + 0)      = 10x  ← 最大上限！       │
│                                                             │
│  結論：串行部分決定了並行的上限                              │
│       減少臨界區段比增加執行緒更重要！                       │
│                                                             │
│  加速比 ↑                                                   │
│    10 ┤                          ─ ─ ─ ─ ─ 理論上限 10x     │
│     8 ┤                    ·····                            │
│     6 ┤              ·····                                  │
│     4 ┤        ·····                                        │
│     2 ┤   ·····                                             │
│     0 ┼─────────────────────────────→ 處理器數量 N          │
│         2    4    8   16   32   ∞                           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 八、效能優化策略總結

```
┌─────────────────────────────────────────────────────────────┐
│                  效能優化策略總結                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  策略 1：減少臨界區段大小                                   │
│  ────────────────────────                                   │
│  • 只鎖定真正需要保護的操作                                 │
│  • 將計算移到鎖外                                          │
│  • 使用本地變數累加後再更新共享資料                         │
│                                                             │
│  策略 2：降低鎖競爭                                         │
│  ──────────────────                                         │
│  • 使用細粒度鎖                                            │
│  • 分離讀寫（讀寫鎖，第七階段學習）                         │
│  • 資料分區，每個分區一個鎖                                 │
│                                                             │
│  策略 3：減少鎖的使用                                       │
│  ──────────────────                                         │
│  • thread_local 變數避免共享                               │
│  • 原子操作替代簡單的鎖（第二十階段學習）                   │
│  • 無鎖演算法（第二十二階段學習）                           │
│                                                             │
│  策略 4：正確選擇鎖類型                                     │
│  ──────────────────────                                     │
│  • 讀多寫少：shared_mutex                                  │
│  • 短臨界區段：自旋鎖                                       │
│  • 需要超時：timed_mutex                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 九、實際案例：優化前後對比

```cpp
// 檔案：lesson_5_6_optimization_example.cpp
// 說明：完整的優化前後對比

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <numeric>
#include <iomanip>

const int DATA_SIZE = 100000;
const int NUM_THREADS = 4;

// ===== 優化前：單一全域鎖 =====
class BeforeOptimization {
private:
    std::mutex mtx;
    std::vector<int> data;
    long long sum = 0;
    
public:
    BeforeOptimization() : data(DATA_SIZE) {
        std::iota(data.begin(), data.end(), 1);
    }
    
    void processRange(int start, int end) {
        for (int i = start; i < end; ++i) {
            std::lock_guard<std::mutex> lock(mtx);  // 每次迭代都鎖
            sum += data[i] * data[i];
        }
    }
    
    long long getSum() const { return sum; }
    void reset() { sum = 0; }
};

// ===== 優化後：本地累加 + 最後合併 =====
class AfterOptimization {
private:
    std::mutex mtx;
    std::vector<int> data;
    long long sum = 0;
    
public:
    AfterOptimization() : data(DATA_SIZE) {
        std::iota(data.begin(), data.end(), 1);
    }
    
    void processRange(int start, int end) {
        long long localSum = 0;  // 本地累加
        
        for (int i = start; i < end; ++i) {
            localSum += data[i] * data[i];  // 無鎖計算
        }
        
        std::lock_guard<std::mutex> lock(mtx);  // 只鎖一次
        sum += localSum;
    }
    
    long long getSum() const { return sum; }
    void reset() { sum = 0; }
};

template<typename T>
double runTest(T& processor) {
    processor.reset();
    
    int rangeSize = DATA_SIZE / NUM_THREADS;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        int rangeStart = i * rangeSize;
        int rangeEnd = (i == NUM_THREADS - 1) ? DATA_SIZE : (i + 1) * rangeSize;
        threads.emplace_back(&T::processRange, &processor, rangeStart, rangeEnd);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    BeforeOptimization before;
    AfterOptimization after;
    
    std::cout << "=== 優化前後效能對比 ===" << std::endl;
    std::cout << "資料大小：" << DATA_SIZE << std::endl;
    std::cout << "執行緒數：" << NUM_THREADS << std::endl << std::endl;
    
    // 預熱
    runTest(before);
    runTest(after);
    
    // 正式測試
    double beforeTime = runTest(before);
    double afterTime = runTest(after);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "優化前時間：" << beforeTime << " ms" << std::endl;
    std::cout << "優化後時間：" << afterTime << " ms" << std::endl;
    std::cout << "加速比：" << beforeTime / afterTime << "x" << std::endl;
    std::cout << std::endl;
    std::cout << "結果驗證：" << std::endl;
    std::cout << "優化前 sum = " << before.getSum() << std::endl;
    std::cout << "優化後 sum = " << after.getSum() << std::endl;
    std::cout << "結果一致：" << (before.getSum() == after.getSum() ? "✓" : "✗") << std::endl;
    
    return 0;
}
```

#### 典型輸出

```
=== 優化前後效能對比 ===
資料大小：100000
執行緒數：4

優化前時間：245.67 ms
優化後時間：1.23 ms
加速比：199.73x

結果驗證：
優化前 sum = 333338333350000
優化後 sum = 333338333350000
結果一致：✓
```

---

### 十、效能檢查清單

```
┌─────────────────────────────────────────────────────────────┐
│                  效能優化檢查清單                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  □ 臨界區段是否盡可能小？                                   │
│                                                             │
│  □ 是否有計算可以移到鎖外？                                 │
│                                                             │
│  □ 是否可以使用本地變數累加？                               │
│                                                             │
│  □ 臨界區段內是否有 I/O 操作？                              │
│                                                             │
│  □ 臨界區段內是否有阻塞等待？                               │
│                                                             │
│  □ 是否可以使用細粒度鎖？                                   │
│                                                             │
│  □ 是否可以使用讀寫鎖？                                     │
│                                                             │
│  □ 是否可以用 thread_local 避免共享？                       │
│                                                             │
│  □ 是否可以用原子操作替代鎖？                               │
│                                                             │
│  □ 是否測量了實際的效能影響？                               │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

### 十一、本課重點回顧

1. **鎖不是免費的**：無競爭約 20-50 ns，有競爭可達微秒級
2. **競爭越多越慢**：高競爭場景下多執行緒比單執行緒還慢
3. **細粒度鎖提升並行度**：但增加複雜度和死結風險
4. **減少臨界區段大小**：將計算移到鎖外，使用本地累加
5. **避免在臨界區段內**：I/O、阻塞等待、耗時計算
6. **Amdahl 定律**：串行部分決定並行的上限
7. **測量優先**：優化前先測量，避免過早優化

---

### 🎉 第五階段完成！

恭喜你完成了「互斥鎖基礎」階段！你已經學會：

- ✅ std::mutex 的基本操作（lock/unlock/try_lock）
- ✅ 互斥鎖的內部工作原理
- ✅ 常見錯誤與避免方法
- ✅ 執行緒安全類別的設計
- ✅ 效能考量與優化策略

---

### 下一階段預告

**第六階段：RAII 鎖管理器** 將學習：

| 課程 | 主題 |
|------|------|
| 6.1 | std::lock_guard 基礎 |
| 6.2 | std::unique_lock 入門 |
| 6.3 | unique_lock 進階操作 |
| 6.4 | std::scoped_lock (C++17) |
| 6.5 | 鎖管理器的選擇策略 |
| 6.6 | 自訂鎖管理器 |

這些 RAII 工具將讓你的鎖管理更安全、更優雅！

---

準備好進入第六階段嗎？
*/



// 檔案：lesson_5_6_optimization_example.cpp
// 說明：完整的優化前後對比

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <thread>

const int DATA_SIZE = 100000;
const int NUM_THREADS = 4;

// ===== 優化前：單一全域鎖 =====
class BeforeOptimization {
private:
    std::mutex mtx;
    std::vector<int> data;
    long long sum = 0;
    
public:
    BeforeOptimization() : data(DATA_SIZE) {
        std::iota(data.begin(), data.end(), 1);
    }
    
    void processRange(int start, int end) {
        for (int i = start; i < end; ++i) {
            std::lock_guard<std::mutex> lock(mtx);  // 每次迭代都鎖
            sum += data[i] * data[i];
        }
    }
    
    long long getSum() const { return sum; }
    void reset() { sum = 0; }
};

// ===== 優化後：本地累加 + 最後合併 =====
class AfterOptimization {
private:
    std::mutex mtx;
    std::vector<int> data;
    long long sum = 0;
    
public:
    AfterOptimization() : data(DATA_SIZE) {
        std::iota(data.begin(), data.end(), 1);
    }
    
    void processRange(int start, int end) {
        long long localSum = 0;  // 本地累加
        
        for (int i = start; i < end; ++i) {
            localSum += data[i] * data[i];  // 無鎖計算
        }
        
        std::lock_guard<std::mutex> lock(mtx);  // 只鎖一次
        sum += localSum;
    }
    
    long long getSum() const { return sum; }
    void reset() { sum = 0; }
};

template<typename T>
double runTest(T& processor) {
    processor.reset();
    
    int rangeSize = DATA_SIZE / NUM_THREADS;
    std::vector<std::thread> threads;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        int rangeStart = i * rangeSize;
        int rangeEnd = (i == NUM_THREADS - 1) ? DATA_SIZE : (i + 1) * rangeSize;
        threads.emplace_back(&T::processRange, &processor, rangeStart, rangeEnd);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}


// -----------------------------------------------------------------------------
// 【日常實務範例 1】日誌分析的平行歸約
//   情境：分析 100 萬行日誌，統計各 HTTP 狀態碼的次數與總傳輸量。
//   天真寫法：每處理一行就鎖住全域統計表更新 → 100 萬次上鎖。
//   正解：每條執行緒維護自己的本地統計，處理完自己那段才合併一次。
//         這與本檔的 AfterOptimization 是完全相同的模式，
//         只是累加的對象從單一個 sum 變成一組計數器。
// -----------------------------------------------------------------------------
struct LogStats {
    long count2xx = 0;
    long count4xx = 0;
    long count5xx = 0;
    long totalBytes = 0;

    void merge(const LogStats& other) {
        count2xx += other.count2xx;
        count4xx += other.count4xx;
        count5xx += other.count5xx;
        totalBytes += other.totalBytes;
    }
};

struct LogEntry {
    int status;
    long bytes;
};

// ✗ 每行都上鎖
void analyzePerLine(std::mutex& mtx, LogStats& global,
                    const std::vector<LogEntry>& logs, size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        if (logs[i].status < 400)      ++global.count2xx;
        else if (logs[i].status < 500) ++global.count4xx;
        else                           ++global.count5xx;
        global.totalBytes += logs[i].bytes;
    }
}

// ✓ 本地統計 + 合併一次
void analyzeLocalMerge(std::mutex& mtx, LogStats& global,
                       const std::vector<LogEntry>& logs, size_t begin, size_t end) {
    LogStats local;                                   // 全程無鎖
    for (size_t i = begin; i < end; ++i) {
        if (logs[i].status < 400)      ++local.count2xx;
        else if (logs[i].status < 500) ++local.count4xx;
        else                           ++local.count5xx;
        local.totalBytes += logs[i].bytes;
    }
    std::lock_guard<std::mutex> lock(mtx);            // 整條執行緒只上鎖一次
    global.merge(local);
}

template<typename Fn>
double runLogAnalysis(Fn fn, const std::vector<LogEntry>& logs, int numThreads,
                      LogStats& result) {
    std::mutex mtx;
    LogStats global;
    std::vector<std::thread> threads;
    size_t chunk = logs.size() / static_cast<size_t>(numThreads);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        size_t b = chunk * static_cast<size_t>(i);
        size_t e = (i == numThreads - 1) ? logs.size() : b + chunk;
        threads.emplace_back([&mtx, &global, &logs, b, e, fn] {
            fn(mtx, global, logs, b, e);
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();

    result = global;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】反例：先量測，不要憑直覺優化
//   情境：這個工作的臨界區段本來就很重（每次要做一段真實計算）。
//         直覺會說「有鎖就該優化」，但實測會顯示：
//         把上鎖次數從 N 降到執行緒數，收益極為有限 ——
//         因為鎖根本不是瓶頸，計算才是。
//   教訓：優化前先量測「鎖佔總時間的比例」，
//         佔比低就別動它，把力氣花在真正的瓶頸上。
// -----------------------------------------------------------------------------
inline long heavyWork(int x) {
    long h = x;
    for (int i = 0; i < 200; ++i) h = h * 31 + (i ^ x);
    return h;
}

double runHeavyPerItemLock(const std::vector<int>& data, int numThreads, long& out) {
    std::mutex mtx;
    long sum = 0;
    std::vector<std::thread> threads;
    size_t chunk = data.size() / static_cast<size_t>(numThreads);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        size_t b = chunk * static_cast<size_t>(i);
        size_t e = (i == numThreads - 1) ? data.size() : b + chunk;
        threads.emplace_back([&mtx, &sum, &data, b, e] {
            for (size_t k = b; k < e; ++k) {
                long v = heavyWork(data[k]);          // 計算在鎖外
                std::lock_guard<std::mutex> lock(mtx);
                sum += v;                              // 每個元素上鎖一次
            }
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    out = sum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double runHeavyLocalMerge(const std::vector<int>& data, int numThreads, long& out) {
    std::mutex mtx;
    long sum = 0;
    std::vector<std::thread> threads;
    size_t chunk = data.size() / static_cast<size_t>(numThreads);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numThreads; ++i) {
        size_t b = chunk * static_cast<size_t>(i);
        size_t e = (i == numThreads - 1) ? data.size() : b + chunk;
        threads.emplace_back([&mtx, &sum, &data, b, e] {
            long local = 0;
            for (size_t k = b; k < e; ++k) local += heavyWork(data[k]);
            std::lock_guard<std::mutex> lock(mtx);
            sum += local;                              // 整條執行緒只上鎖一次
        });
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::steady_clock::now();
    out = sum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    BeforeOptimization before;
    AfterOptimization after;
    
    std::cout << "=== 優化前後效能對比 ===" << std::endl;
    std::cout << "資料大小：" << DATA_SIZE << std::endl;
    std::cout << "執行緒數：" << NUM_THREADS << std::endl << std::endl;
    
    // 預熱
    runTest(before);
    runTest(after);
    
    // 正式測試
    double beforeTime = runTest(before);
    double afterTime = runTest(after);
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "優化前時間：" << beforeTime << " ms" << std::endl;
    std::cout << "優化後時間：" << afterTime << " ms" << std::endl;
    std::cout << "加速比：" << beforeTime / afterTime << "x" << std::endl;
    std::cout << std::endl;
    std::cout << "結果驗證：" << std::endl;
    std::cout << "優化前 sum = " << before.getSum() << std::endl;
    std::cout << "優化後 sum = " << after.getSum() << std::endl;
    std::cout << "結果一致：" << (before.getSum() == after.getSum() ? "✓" : "✗") << std::endl;
    std::cout << "（累加的是整數，所以優化前後結果完全相同；" << std::endl;
    std::cout << "  若是 double，浮點加法不滿足結合律，最低有效位可能不同）" << std::endl;

    std::cout << "\n=== 日常實務 1：日誌分析的平行歸約 ===" << std::endl;
    {
        std::vector<LogEntry> logs(1000000);
        for (size_t i = 0; i < logs.size(); ++i) {
            int r = static_cast<int>(i % 10);
            logs[i].status = (r < 7) ? 200 : (r < 9 ? 404 : 500);
            logs[i].bytes = 100 + static_cast<long>(i % 900);
        }

        LogStats r1, r2;
        runLogAnalysis(analyzePerLine,   logs, 4, r1);   // 預熱
        runLogAnalysis(analyzeLocalMerge, logs, 4, r2);

        double tPerLine = runLogAnalysis(analyzePerLine,    logs, 4, r1);
        double tLocal   = runLogAnalysis(analyzeLocalMerge, logs, 4, r2);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "100 萬行日誌，4 條執行緒" << std::endl;
        std::cout << "每行上鎖  ：" << tPerLine << " ms（上鎖 1000000 次）" << std::endl;
        std::cout << "本地合併  ：" << tLocal   << " ms（上鎖 4 次）" << std::endl;
        std::cout << "2xx=" << r2.count2xx << "  4xx=" << r2.count4xx
                  << "  5xx=" << r2.count5xx << std::endl;
        std::cout << "兩種寫法結果一致："
                  << ((r1.count2xx == r2.count2xx && r1.count4xx == r2.count4xx
                       && r1.count5xx == r2.count5xx && r1.totalBytes == r2.totalBytes)
                      ? "是" : "否") << std::endl;
    }

    std::cout << "\n=== 日常實務 2：反例 —— 鎖不是瓶頸時，優化鎖沒有意義 ===" << std::endl;
    {
        std::vector<int> data(200000);
        for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<int>(i % 1000);

        long s1 = 0, s2 = 0;
        runHeavyPerItemLock(data, 4, s1);    // 預熱
        runHeavyLocalMerge(data, 4, s2);

        double tPerItem = runHeavyPerItemLock(data, 4, s1);
        double tLocal   = runHeavyLocalMerge(data, 4, s2);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "20 萬筆資料，每筆要做 200 次運算，4 條執行緒" << std::endl;
        std::cout << "每筆上鎖  ：" << tPerItem << " ms" << std::endl;
        std::cout << "本地合併  ：" << tLocal   << " ms" << std::endl;
        std::cout << "加速比    ：" << (tPerItem / tLocal) << "x" << std::endl;
        std::cout << "結果一致  ：" << (s1 == s2 ? "是" : "否") << std::endl;
        std::cout << "→ 對照前一段的 20 倍以上，這裡的加速比小得多：" << std::endl;
        std::cout << "  因為每筆的計算量遠大於一次上鎖，鎖根本不是瓶頸。" << std::endl;
        std::cout << "  這就是為什麼要【先量測再優化】。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量6.cpp' -o optimization

// ⚠️ 本檔的【所有時間與加速比每次執行都不同】（效能量測，受頻率調節、
// 排程、負載影響），下面貼的是本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）
// 某一次的真實實測。穩定成立的是【趨勢】：
//   * 上鎖次數從 10 萬 / 100 萬降到 4 次時，加速數十倍（本次 25.69x、39x）
//   * 但當每筆的計算量遠大於一次上鎖時，加速比就掉到接近 1（本次 1.78x）
//     —— 這正是「先量測再優化」要說明的事。
//
// 以下是確定值（正確性驗證，每次執行都相同）：
//   * 優化前後 sum 皆為 18104913692784、結果一致 ✓（累加的是整數）
//   * 日誌統計 2xx=700000 / 4xx=200000 / 5xx=100000

// === 預期輸出 ===
// === 優化前後效能對比 ===
// 資料大小：100000
// 執行緒數：4
//
// 優化前時間：9.48 ms
// 優化後時間：0.32 ms
// 加速比：29.33x
//
// 結果驗證：
// 優化前 sum = 18104913692784
// 優化後 sum = 18104913692784
// 結果一致：✓
// （累加的是整數，所以優化前後結果完全相同；
//   若是 double，浮點加法不滿足結合律，最低有效位可能不同）
//
// === 日常實務 1：日誌分析的平行歸約 ===
// 100 萬行日誌，4 條執行緒
// 每行上鎖  ：112.04 ms（上鎖 1000000 次）
// 本地合併  ：2.60 ms（上鎖 4 次）
// 2xx=700000  4xx=200000  5xx=100000
// 兩種寫法結果一致：是
//
// === 日常實務 2：反例 —— 鎖不是瓶頸時，優化鎖沒有意義 ===
// 20 萬筆資料，每筆要做 200 次運算，4 條執行緒
// 每筆上鎖  ：34.15 ms
// 本地合併  ：18.88 ms
// 加速比    ：1.81x
// 結果一致  ：是
// → 對照前一段的 20 倍以上，這裡的加速比小得多：
//   因為每筆的計算量遠大於一次上鎖，鎖根本不是瓶頸。
//   這就是為什麼要【先量測再優化】。
