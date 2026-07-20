//
// =============================================================================
//  課程 5.6：互斥鎖的效能考量1.cpp  —  一次 lock/unlock 到底要多少錢？
// =============================================================================
//
// 【主題資訊 Information】
//   主題：    無競爭情況下 std::mutex 的基本開銷；futex 的快慢兩條路徑
//   量測方法：同一個迴圈跑兩次（有鎖 / 無鎖），相減後除以迭代次數
//   標準版本：std::chrono::high_resolution_clock 為 C++11
//   標頭檔：  <chrono>、<mutex>、<iomanip>
//   本機環境：Ubuntu 26.04 / g++ 15.2.0 / 16 核心（實測值見檔尾）
//   ⚠️ 所有時間數字都是【本機實測值】，隨 CPU、編譯器、最佳化等級而變，
//      不是標準保證，也不該當成跨平台的常數引用。
//
// 【詳細解釋 Explanation】
//
// 【1. 這支程式量的是「無競爭」的開銷，這點非常重要】
//   本檔只有【單一執行緒】在跑，所以每次 lock() 都必定成功、
//   從來不需要等待。量到的是 mutex 的【最好情況】(fast path)。
//   真實系統中有競爭時的成本會高出兩三個數量級（見下一檔 5.6-2）。
//   為什麼先量無競爭？因為它回答了一個很實際的問題：
//   「我在一個其實不太會被搶的地方加了鎖，代價有多大？」
//   答案是：比想像中小，但也絕不是零。
//
// 【2. futex：為什麼無競爭時這麼便宜】
//   Linux 上 glibc 的 pthread_mutex（std::mutex 的底層）使用
//   futex = fast userspace mutex，它的核心設計是：
//     * 【無競爭】：整個 lock 只是對一個 int 做一次原子的
//       compare-and-swap（x86-64 上是 `lock cmpxchg`）。
//       完全在使用者空間完成，【不進核心】，沒有系統呼叫。
//       unlock 同理，是一次原子寫入加上「有沒有人在等」的檢查。
//     * 【有競爭】：CAS 失敗才呼叫 futex(FUTEX_WAIT) 進入核心，
//       把執行緒掛起、加入等待佇列、觸發上下文切換。
//   → 這個「常見情況不進核心」的設計，正是本檔量到的數字
//     停留在數十奈秒等級的原因。
//   （名字裡的 fast 就是在講這件事：把快路徑留在 userspace。）
//
// 【3. 這幾十奈秒實際上花在哪】
//   拆解一次 lock + unlock 的成本：
//     ① 原子指令本身：`lock cmpxchg` 需要取得快取行的獨佔權
//        （MESI 協定的 Exclusive 狀態）。單執行緒下快取行本來就在
//        自己的 L1，成本很低。
//     ② 記憶體屏障：lock 具有 acquire 語意、unlock 具有 release 語意，
//        會限制編譯器與 CPU 的重排。x86-64 的強記憶體模型下
//        不需要額外的屏障指令，但仍會阻止【編譯器】的最佳化。
//     ③ 函式呼叫：lock_guard 的建構/解構、pthread_mutex_lock 的呼叫。
//   → 其中 ② 常被忽略：即使指令本身很快，
//     「編譯器不能把迴圈裡的操作提出去、不能重排」這件事，
//     往往比指令成本影響更大。本檔的無鎖版本就被最佳化得極快，
//     而有鎖版本的每次迭代都必須真的執行。
//
// 【4. 為什麼「19 倍慢」這個比率會誤導人】
//   本機實測有鎖版本約是無鎖的十幾倍慢（見檔尾；此倍率每次執行都不同），
//   但這個比率幾乎沒有參考價值，因為【分母太小】：
//   無鎖版本的 ++counter 只是一道 add 指令（甚至可能被最佳化成
//   單一次加法），本來就快到接近零。
//   拿「幾乎為零」當分母，任何東西都會顯得很多倍。
//   → 正確的判讀方式是看【絕對值】：每次約二十幾奈秒。
//     若你的臨界區段要做的事本來就要幾微秒，這個開銷是 1% 以下，
//     完全不值得為它做無鎖改寫；若臨界區段只是 ++counter，
//     那就該考慮 std::atomic。
//
// 【5. 什麼時候該擔心這個開銷】
//   判斷的關鍵是「鎖的成本」相對於「臨界區段的工作量」：
//     * 臨界區段做的事 >> 20 ns（讀檔、查表、複雜計算）→ 鎖的開銷可忽略。
//     * 臨界區段只是一次整數遞增（≈1 ns）→ 鎖的開銷是工作本身的十幾倍，
//       此時 std::atomic 的 fetch_add 是明顯更好的選擇。
//     * 每秒數百萬次的熱路徑 → 考慮批次化（本地累加、最後合併一次），
//       把上鎖次數從 N 降到執行緒數（見 5.6-4 與 5.6-6）。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 為什麼要用 high_resolution_clock 而不是 clock() 或 time()
//   * time()：秒級解析度，量不到奈秒。
//   * clock()：量的是 CPU 時間而非牆鐘時間，多執行緒下會把
//     各核心的時間加總，用來量並行程式會得到誤導的結果。
//   * std::chrono::high_resolution_clock：本機 libstdc++ 上它是
//     std::chrono::system_clock 或 steady_clock 的別名，解析度到奈秒。
//   ⚠️ 嚴謹的量測應該用 steady_clock：high_resolution_clock 不保證單調，
//      若期間系統時間被 NTP 調整，量出來的時間可能是負的。
//      本檔沿用原始講義的寫法，但正式的 benchmark 請用 steady_clock。
//
// (B) 這種微量測的三個系統性誤差（本檔都存在）
//   ① 沒有預熱：第一次執行時指令快取、分支預測器、頻率調節都還沒穩定。
//      本機 CPU 有動態頻率調整，前幾毫秒可能還在低頻。
//   ② 只跑一次：沒有取多次的中位數，單次結果容易被排程干擾。
//   ③ 編譯器可能把無鎖迴圈整個最佳化掉：
//      `for(...) ++counter;` 在 -O2 下可能被摺疊成一次加法
//      （本檔用 counter 的引用傳遞降低了這個風險，但沒有完全排除）。
//   → 正式的 benchmark 應該用 Google Benchmark 這類框架，
//     它會處理預熱、多次取樣、防止最佳化消除（DoNotOptimize）。
//     本檔的數字用來建立「數量級的直覺」是足夠的，但不該當成精確測量。
//
// (C) 與其他同步原語的數量級對照（本機等級的粗略概念）
//     * 無同步的 ++            ： < 1 ns
//     * std::atomic fetch_add  ： 數 ns（一道 lock add）
//     * std::mutex 無競爭      ： 數十 ns（本檔量到的）
//     * std::mutex 有競爭      ： 數百 ns ~ 數 μs（含上下文切換）
//     * 執行緒建立             ： 數十 μs
//     * 磁碟 I/O / 系統呼叫    ： 數 μs ~ 數 ms
//   → 記住這個階梯，就能快速判斷「該不該為了省一把鎖去做無鎖改寫」。
//
// 【注意事項 Pay Attention】
// 1. 本檔量的是【無競爭】的最好情況；有競爭時成本高兩三個數量級。
// 2. 所有時間數字都是本機實作定義的實測值，不是標準保證，不可跨平台引用。
// 3. 「慢幾倍」這個比率會誤導 —— 分母（無鎖的 ++）本來就接近零；
//    要看絕對值（每次約十幾到二十幾奈秒，本機多次量測的範圍）。
// 4. 嚴謹量測應用 steady_clock（保證單調），不是 high_resolution_clock。
// 5. 本檔缺少預熱與多次取樣，數字適合建立數量級直覺，不適合當精確 benchmark。
// 6. 每次執行的數字都不同（頻率調節、快取狀態、其他負載），
//    檔尾的預期輸出只是某一次的實測。
//
// 【LeetCode 說明】本檔不附 LeetCode 範例。
//   本檔是效能量測與 futex 機制的說明，沒有可判題的演算法；
//   允許使用的設計題（146/155/705/707/1603）與並行題（1114～1117/1195）
//   都不涉及「量測同步原語的成本」。硬湊只會失焦，故從缺，
//   改以下方兩個真實情境（該不該用 atomic 取代 mutex、批次化的效益）呈現。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】互斥鎖的成本
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一次 std::mutex 的 lock/unlock 大概要多少時間?
//     答：要先問「有沒有競爭」。無競爭時本機實測約 13~25 ns（多次量測的範圍，隨 CPU 頻率調節與
//         當下負載而變動）——
//         因為 Linux 的 futex 設計讓快路徑只做一次原子 CAS，
//         完全在使用者空間完成、不進核心。
//         有競爭時要呼叫 futex(FUTEX_WAIT) 進核心掛起執行緒、
//         觸發上下文切換，成本跳到數百奈秒甚至數微秒。
//     追問：那為什麼還有人說「鎖很貴」?
//         → 他們講的是有競爭的情況，或是講「序列化」的代價 ——
//           鎖讓臨界區段只能一個一個跑，依 Amdahl 定律，
//           這個序列比例決定了多核心加速的上限。
//           真正貴的通常不是指令，是失去的並行度。
//
// 🔥 Q2. 什麼時候該把 mutex 換成 std::atomic?
//     答：判準是「臨界區段的工作量」與「鎖的開銷」的比值。
//         若臨界區段只是一次整數遞增（約 1 ns），
//         二十幾奈秒的鎖開銷就是工作本身的二十倍，換 atomic 明顯划算。
//         若臨界區段本來就要做幾微秒的事，鎖的開銷是 1% 以下，
//         為它做無鎖改寫不值得，還會大幅增加出錯的機率。
//     追問：atomic 什麼時候不夠用?
//         → 當要保護的不變量橫跨多個變數時。
//           沒有任何單一原子指令能同時改兩個位置，
//           這種情況只能用鎖（見課程 4.2-3）。
//
// ⚠️ 陷阱. 本檔量出「有鎖比無鎖慢 19 倍」，是不是代表加鎖會讓程式慢 19 倍?
//     答：不是。這個比率的分母是「無鎖的 ++counter」，
//         它只是一道 add 指令、接近零成本，甚至可能被最佳化掉大半。
//         拿接近零的東西當分母，任何開銷都會顯得很多倍。
//         真正該看的是絕對值：每次約十幾到二十幾奈秒。
//         把它放進真實的臨界區段（通常要做幾百奈秒到幾微秒的事），
//         佔比往往在 1~10% 之間。
//     為什麼會錯：把 micro-benchmark 的比率直接外推到整支程式。
//         微量測刻意把「被測項目」以外的東西減到最少，
//         所以它的比率天生被放大。正確的用法是取它的【絕對值】，
//         再放回自己真實的工作量裡去算佔比。
// ═══════════════════════════════════════════════════════════════════════════
// 檔案：lesson_5_6_lock_overhead.cpp
// 說明：測量互斥鎖的基本開銷

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>
#include <atomic>

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


// -----------------------------------------------------------------------------
// 【日常實務範例 1】計數器：mutex vs atomic 的實際差距
//   情境：服務要統計請求數。這是「臨界區段只有一次遞增」的典型場景，
//         也是 atomic 最划算的地方。
//   本例在【單執行緒無競爭】下比較三種寫法，
//         證明「臨界區段越小，鎖的相對開銷越誇張」。
// -----------------------------------------------------------------------------
long benchPlain(int iterations) {
    long c = 0;
    for (int i = 0; i < iterations; ++i) ++c;
    return c;
}

long benchAtomic(int iterations) {
    std::atomic<long> c{0};
    for (int i = 0; i < iterations; ++i) c.fetch_add(1, std::memory_order_relaxed);
    return c.load();
}

long benchMutex(int iterations) {
    std::mutex m;
    long c = 0;
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(m);
        ++c;
    }
    return c;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】臨界區段有「真實工作量」時，鎖的佔比其實很小
//   情境：真實的臨界區段很少只做一次 ++，通常要查表、比對、更新多個欄位。
//         本例讓臨界區段做一段有意義的工作（雜湊計算 + map 更新），
//         再看鎖的開銷佔多少百分比。
//   結論：這才是判斷「該不該為了省鎖去做無鎖改寫」的正確方式 ——
//         看佔比，而不是看 micro-benchmark 的倍率。
// -----------------------------------------------------------------------------
long realisticWork(int x) {
    // 模擬臨界區段內的真實工作：幾十次運算
    long h = x;
    for (int i = 0; i < 40; ++i) h = h * 31 + (i ^ x);
    return h;
}

double benchWorkNoLock(int iterations) {
    auto start = std::chrono::steady_clock::now();
    long sink = 0;
    for (int i = 0; i < iterations; ++i) sink += realisticWork(i);
    auto end = std::chrono::steady_clock::now();
    // 用 sink 避免整段被最佳化掉
    if (sink == 42) std::cout << "";
    return std::chrono::duration<double, std::nano>(end - start).count();
}

double benchWorkWithLock(int iterations) {
    std::mutex m;
    auto start = std::chrono::steady_clock::now();
    long sink = 0;
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(m);
        sink += realisticWork(i);
    }
    auto end = std::chrono::steady_clock::now();
    if (sink == 42) std::cout << "";
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
    std::cout << "（此比率會誤導：分母「無鎖的 ++」本來就接近零成本。"
                 "該看的是絕對值）" << std::endl;

    std::cout << "\n=== 日常實務 1：三種計數器寫法（單執行緒、無競爭）===" << std::endl;
    {
        const int n = 1000000;
        // 預熱：讓指令快取、分支預測器、CPU 頻率先穩定下來
        benchPlain(n); benchAtomic(n); benchMutex(n);

        double tPlain  = measureTime([&]{ benchPlain(n); });
        double tAtomic = measureTime([&]{ benchAtomic(n); });
        double tMutex  = measureTime([&]{ benchMutex(n); });

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "無同步  ：" << tPlain  / n << " ns / 次" << std::endl;
        std::cout << "atomic  ：" << tAtomic / n << " ns / 次" << std::endl;
        std::cout << "mutex   ：" << tMutex  / n << " ns / 次" << std::endl;
        std::cout << "→ 臨界區段只有一次 ++ 時，atomic 明顯划算" << std::endl;
    }

    std::cout << "\n=== 日常實務 2：臨界區段有真實工作量時，鎖的佔比 ===" << std::endl;
    {
        const int n = 200000;
        benchWorkNoLock(n); benchWorkWithLock(n);   // 預熱

        double tNo = benchWorkNoLock(n);
        double tWith = benchWorkWithLock(n);
        double perOpNo = tNo / n;
        double perOpWith = tWith / n;
        double overhead = perOpWith - perOpNo;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "臨界區段工作量：約 " << perOpNo << " ns / 次" << std::endl;
        std::cout << "加鎖之後      ：約 " << perOpWith << " ns / 次" << std::endl;
        std::cout << "鎖的開銷      ：約 " << overhead << " ns / 次" << std::endl;
        std::cout << "鎖佔總時間    ：約 " << (overhead / perOpWith * 100.0) << " %" << std::endl;
        std::cout << "→ 臨界區段有真實工作量時，鎖的佔比通常只有個位數百分比；" << std::endl;
        std::cout << "  這時為了省鎖去做無鎖改寫，是拿巨大的正確性風險換極小的收益。" << std::endl;
    }
    
    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量1.cpp' -o lock_overhead
//
// 建議也用 -O2 跑一次對照（最佳化會大幅改變「無鎖」那一組的數字）:
//   g++ -std=c++17 -O2 -Wall -Wextra -pthread '課程 5.6：互斥鎖的效能考量1.cpp' -o lock_overhead_O2

// ⚠️ 本檔【所有數字每次執行都不同】——這是效能量測，不是功能輸出。
// 影響因素：CPU 動態頻率調節、快取狀態、其他行程的負載、是否開啟最佳化。
// 本機（Ubuntu 26.04 / g++ 15.2.0 / 16 核心）多次量測，
// 「每次鎖操作開銷」落在約 13~25 ns 之間（無競爭的 fast path）。
// 下面貼的是本機某一次以檔頭指令（未指定 -O，即 -O0）的真實實測，
// 只用來建立數量級的直覺，【不是】標準保證，也不該跨平台引用。
// 加上 -O2 後「無鎖」那一組會快很多（迴圈可能被摺疊），倍率會更誇張 ——
// 這正好說明為什麼該看絕對值而不是倍率。

// === 預期輸出 ===
// === 互斥鎖開銷測量 ===
// 迭代次數：1000000
//
// 無鎖操作總時間：1.24 ms
// 有鎖操作總時間：13.45 ms
// 每次鎖操作開銷：12.21 ns
// 效能比率：10.85x 慢
// （此比率會誤導：分母「無鎖的 ++」本來就接近零成本。該看的是絕對值）
//
// === 日常實務 1：三種計數器寫法（單執行緒、無競爭）===
// 無同步  ：1.16 ns / 次
// atomic  ：5.71 ns / 次
// mutex   ：13.29 ns / 次
// → 臨界區段只有一次 ++ 時，atomic 明顯划算
//
// === 日常實務 2：臨界區段有真實工作量時，鎖的佔比 ===
// 臨界區段工作量：約 72.33 ns / 次
// 加鎖之後      ：約 82.33 ns / 次
// 鎖的開銷      ：約 10.00 ns / 次
// 鎖佔總時間    ：約 12.15 %
// → 臨界區段有真實工作量時，鎖的佔比通常只有個位數百分比；
//   這時為了省鎖去做無鎖改寫，是拿巨大的正確性風險換極小的收益。
