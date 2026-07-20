// =============================================================================
//  課程 1.2：為什麼需要多執行緒 — 第 1 部分：效能提升（運算密集型任務）
// =============================================================================
//
// 【主題資訊 Information】
//   標頭檔：<thread>（std::thread / hardware_concurrency）、<chrono>（計時）、
//           <vector>、<iomanip>
//   標準版本：C++11（std::thread 與 <chrono> 皆為 C++11 引入）
//
//     unsigned int std::thread::hardware_concurrency() noexcept;  // 核心數【提示】
//     std::thread t(func, args...);                               // 建立並立即啟動
//     t.join();                                                   // 等待結束
//     std::ref(x)                                                 // 以參考傳遞引數
//
//   複雜度：把 N 個獨立工作分給 P 條執行緒，理想加速比為 P，
//           實際受 Amdahl 定律、負載不均、記憶體頻寬限制。
//
// 【詳細解釋 Explanation】
//
// 【1. 什麼樣的任務才「值得」多執行緒化】
//   多執行緒不是免費的加速器。建立一條執行緒在 Linux 上約需數十微秒
//   （clone(2) + 堆疊配置），還要付出快取污染與同步的代價。
//   真正划算的前提有三個：
//     (a) 工作量夠大 —— 大到足以攤平執行緒建立成本；
//     (b) 工作可以切開 —— 子任務之間沒有相依性；
//     (c) 子任務不需要頻繁同步 —— 否則同步成本會吃掉所有收益。
//   本檔的質數計數三個條件都滿足：純 CPU 運算、每個數字獨立、
//   各執行緒只寫自己的結果格，全程零同步。這是所謂
//   embarrassingly parallel（易平行）問題，是多執行緒的最佳情況。
//
// 【2. 為什麼要用 std::ref】
//   std::thread 的建構子會把所有引數【複製（decay-copy）】到執行緒自己的
//   儲存空間 —— 這是刻意的設計，避免新執行緒引用呼叫端可能已消失的區域變數。
//   所以直接寫 std::thread(f, result) 傳的是 result 的【副本】，
//   worker 改的是副本，呼叫端的 result 永遠不變（而且不會有任何警告）。
//   要真正傳參考必須用 std::ref(result) 包起來，它產生一個
//   std::reference_wrapper，被複製的是這個輕量包裝，指向的仍是原物件。
//   本檔的 countPrimesWorker(long long, long long, long long&) 就是靠
//   std::ref(results[i]) 才能把結果寫回 main 的 vector。
//
//   ⚠️ 相對地，這也代表你必須自己保證被引用的物件活得夠久 ——
//   results 這個 vector 一定要在所有 join() 之後才能銷毀。
//
// 【3. 為什麼寫 results[i] 不需要 mutex】
//   多條執行緒同時寫同一個 std::vector 的【不同元素】不是 data race。
//   理由是 C++ memory model 以「記憶體位置（memory location）」為單位判定，
//   不同的 vector 元素是不同的物件、佔用不同位址，彼此獨立。
//   標準明文保證（[res.on.data.races]）：對不同元素的並行存取是安全的。
//   ⚠️ 唯一的著名例外是 std::vector<bool> —— 它是位元打包的特化，
//      多個 bool 擠在同一個 byte 裡，並行寫入【就是】data race。
//   ⚠️ 另外要注意：不是 race 不代表沒有效能問題。若相鄰元素落在同一條
//      cache line（64 bytes）上，會發生 false sharing，效能反而慘跌。
//      本檔每個 long long 佔 8 bytes，8 個元素才共用一條 cache line，
//      但因為每條執行緒只在【最後】寫一次結果，寫入頻率極低，影響可忽略。
//      若是在迴圈中頻繁累加到共享陣列，就必須用 padding 或區域變數累加。
//
// 【4. 為什麼加速比達不到核心數 —— Amdahl 定律】
//   Amdahl 定律：若程式中有比例 p 的部分可平行、(1-p) 必須序列，
//   用 N 個核心的理論最大加速比是
//       S(N) = 1 / ((1-p) + p/N)
//   本檔中無法平行的部分包括：建立執行緒、切分範圍、彙總結果、輸出。
//   但更關鍵的限制是【負載不均】：
//     isPrime(n) 的成本是 O(√n)，所以「1 到 62500」比「937500 到 1000000」
//     便宜得多。把 1..1000000 平均切成 16 段，最後一段的每個數字都要試除到
//     √1000000 = 1000，第一段只要試除到 250。
//     結果是先做完的執行緒閒著等最慢的那條 —— 這就是負載不均。
//   本檔會實際印出每條執行緒找到的質數數量，你會看到分佈明顯不均
//   （質數密度依 1/ln(n) 遞減，但單次判定成本隨 √n 遞增）。
//   改善方式是動態排程（work stealing）或更細的切分（chunk 遠多於執行緒數）。
//
// 【5. hardware_concurrency() 的正確用法】
//   它回傳的是【提示】而非保證，標準允許在資訊不可得時回傳 0。
//   正確寫法一定要有後備值：
//       unsigned n = std::thread::hardware_concurrency();
//       if (n == 0) n = 4;
//   ⚠️ 另一個實務陷阱：在容器（Docker/K8s）中它回報【宿主機】的邏輯核心數，
//      不反映 cgroup 的 CPU 配額。只分到 2 核的容器照樣回報 64，
//      據此開 64 條執行緒會嚴重過度訂閱、效能反而下降。
//
// 【概念補充 Concept Deep Dive】
//   * 為什麼用 steady_clock 而不是 system_clock：
//     system_clock 是「牆上時鐘」，可能被 NTP 校時或使用者調整而【往回跳】，
//     拿它量時間間隔可能得到負數。steady_clock 保證單調遞增，
//     是量測經過時間的唯一正確選擇。high_resolution_clock 在多數實作上
//     只是這兩者之一的別名，語意不明確，不建議用。
//
//   * 邏輯核心 vs 實體核心：hardware_concurrency() 回報的是【邏輯】核心數。
//     本機為 16，若啟用了 SMT/Hyper-Threading，實體核心可能只有 8。
//     兩個邏輯核心共用同一組執行單元，對純運算密集任務的加速遠低於 2 倍
//     （典型是 1.2~1.3 倍）。這是加速比達不到 16 的另一個結構性原因。
//
//   * 為什麼 isPrime 的迴圈用 i*i <= n 而不是 i <= sqrt(n)：
//     避免每圈呼叫浮點 sqrt 並承擔精度風險。整數乘法比浮點開方快，
//     且完全沒有邊界值的舍入問題。⚠️ 但 i*i 對極大的 n 有溢位風險，
//     本檔上限 1000000 遠低於 long long 的範圍，安全無虞。
//
//   * 執行緒建立成本的量級：Linux + glibc 上約 20–80 微秒。
//     所以「每個小任務開一條執行緒」是反模式 —— 若單一任務只跑 10 微秒，
//     建立成本就是工作本身的數倍。正確做法是執行緒池（見課程 3.3 第 7 部分）。
//
// 【注意事項 Pay Attention】
//   1. 執行時間、加速比、並行效率【每次執行都不同】，且高度依賴機器與負載。
//      本檔輸出的數字僅供參考，不可當成固定答案。
//   2. hardware_concurrency() 可能回傳 0，且在容器中不反映 cgroup 限制。
//   3. 傳參考給 std::thread 必須用 std::ref，否則傳的是副本且【沒有警告】。
//   4. 並行寫 vector 的不同元素是安全的，但 std::vector<bool> 例外；
//      且要留意 false sharing 造成的效能損失。
//   5. 平均切分範圍在「單位成本不均」的問題上會造成負載不均，
//      加速比因此低於核心數。這不是 bug，是切分策略的固有限制。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】多執行緒的效能提升
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 開了 16 條執行緒，為什麼加速比通常遠小於 16？
//     答：至少四個原因。(1) Amdahl 定律：建立執行緒、切分、彙總、輸出
//         這些序列部分無法平行；(2) 負載不均：本檔中 isPrime 成本是 O(√n)，
//         平均切分範圍會讓處理大數字的執行緒慢得多，先完成的只能空等；
//         (3) hardware_concurrency 回報的是【邏輯】核心，SMT 下兩個邏輯核心
//         共用執行單元，對純運算任務加速遠低於 2 倍；(4) 記憶體頻寬與
//         快取競爭。
//     追問：負載不均要怎麼改善？
//         → 把工作切成遠多於執行緒數的小塊，由執行緒動態領取
//           （work stealing / dynamic scheduling），先做完的自己去拿下一塊，
//           而不是一開始就靜態分配。
//
// 🔥 Q2. 多條執行緒同時寫同一個 std::vector 的不同元素，需要加鎖嗎？
//     答：不需要。C++ memory model 以「記憶體位置」為判定單位，
//         不同的 vector 元素是不同物件、位址不同，標準明文保證
//         並行存取不同元素是安全的。但有兩個重要例外／注意事項：
//         (1) std::vector<bool> 是位元打包特化，多個 bool 共用同一 byte，
//             並行寫入【就是】data race；
//         (2) 不是 race 不代表沒有效能問題 —— 相鄰元素若落在同一條
//             cache line 上會發生 false sharing，效能可能不升反降。
//     追問：那 push_back 呢？
//         → 完全不同。push_back 可能觸發重新配置並改動 size，
//           是對容器本身的修改，多執行緒並行 push_back 必然是 data race。
//
// ⚠️ 陷阱. 這樣建立執行緒，為什麼 result 永遠是 0？
//         long long result = 0;
//         std::thread t(countPrimesWorker, 1, 1000, result);   // 少了 std::ref
//         t.join();
//         std::cout << result;      // → 0
//     答：std::thread 的建構子會把所有引數【複製】到執行緒自己的儲存空間，
//         worker 收到的是 result 的副本，改的也是副本，原本的 result 從未被動過。
//         必須寫成 std::ref(result) 才會真正傳參考。
//     為什麼會錯：以為函式參數宣告成 long long& 就一定會綁到原物件。
//         但 std::thread 在「把引數搬進執行緒」這一步就已經複製了，
//         綁定發生在複製【之後】—— 綁到的是那份副本。
//         最陰險的是這個錯誤在很多情況下【編譯得過也不會警告】，
//         只會安靜地得到錯誤結果。
//         （附帶一提：若參數是非 const 的 long long&，較新的 libstdc++
//           其實會在編譯期報錯；但參數若是 const& 或指標型別就完全不會擋，
//           所以不能依賴編譯器幫你抓。）
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔從缺
//   本檔主題是「量測多執行緒帶來的效能提升」，核心是計時、切分與加速比分析。
//   LeetCode 的並行題（1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded）
//   考的全是「用同步原語強制執行順序」，刻意讓執行緒【互相等待】，
//   與本檔追求的「讓執行緒互不干擾地全速跑」正好相反，
//   而且那些題目完全不涉及效能量測。硬套一題會誤導讀者，故從缺。

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

// 模擬一個運算密集型的任務：計算範圍內所有質數的數量
bool isPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2) {   // i*i 避免浮點 sqrt 的精度問題
        if (n % i == 0) return false;
    }
    return true;
}

// 計算指定範圍內的質數數量（單執行緒版本）
long long countPrimesInRange(long long start, long long end) {
    long long count = 0;
    for (long long i = start; i <= end; ++i) {
        if (isPrime(i)) {
            ++count;
        }
    }
    return count;
}

// 工作執行緒函式：計算指定範圍內的質數數量，並將結果存入指定位置
// ⚠️ 第三個參數是 long long&，呼叫端【必須】用 std::ref 傳入，否則只會改到副本
void countPrimesWorker(long long start, long long end, long long& result) {
    result = countPrimesInRange(start, end);
}

// -----------------------------------------------------------------------------
// 【對照示範】忘記 std::ref 的後果
//   這是初學多執行緒最常見、也最難察覺的錯誤之一。
// -----------------------------------------------------------------------------
void demoMissingRef() {
    long long withRef = 0;
    long long withoutRef = 0;

    // ✓ 正確：用 std::ref 包起來，worker 寫回原物件
    std::thread t1(countPrimesWorker, 1, 10000, std::ref(withRef));
    t1.join();

    // ✗ 錯誤示範：不用 std::ref。
    //   註：countPrimesWorker 的參數是非 const 的 long long&，
    //   較新的 libstdc++ 會在編譯期就擋下來，所以這裡改用 lambda 按值捕獲
    //   來重現「引數被複製、寫回的是副本」這個真正的語意問題。
    std::thread t2([withoutRef]() mutable {
        countPrimesWorker(1, 10000, withoutRef);   // 改的是 lambda 內的副本
    });
    t2.join();

    std::cout << "  用 std::ref 傳遞  → " << withRef << " 個質數（正確）" << std::endl;
    std::cout << "  按值複製後傳遞    → " << withoutRef
              << "（原變數從未被改動，且沒有任何警告）" << std::endl;
}

// -----------------------------------------------------------------------------
// 【對照示範】負載不均：平均切分為什麼不等於平均耗時
//   isPrime 的成本是 O(√n)，所以處理大數字的那段慢得多。
// -----------------------------------------------------------------------------
void demoLoadImbalance() {
    const long long total = 1000000;
    const int chunks = 4;
    const long long size = total / chunks;

    std::cout << "  把 1..1000000 平均切成 4 段，量測各段【單獨】耗時：" << std::endl;
    for (int i = 0; i < chunks; ++i) {
        long long s = 1 + i * size;
        long long e = (i == chunks - 1) ? total : s + size - 1;

        auto t0 = std::chrono::steady_clock::now();
        long long c = countPrimesInRange(s, e);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0).count();

        std::cout << "    段 " << i << " [" << std::setw(7) << s << ", "
                  << std::setw(7) << e << "] → " << std::setw(5) << c
                  << " 個質數，耗時 " << std::setw(4) << ms << " ms" << std::endl;
    }
    std::cout << "  ← 元素個數相同，但耗時明顯遞增：這就是負載不均的來源"
              << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例】平行掃描伺服器日誌，統計各等級的出現次數
//   情境：維運要從一批 access/error log 中快速統計 ERROR / WARN / INFO 的數量，
//         用來判斷某次發版後錯誤率是否上升。單機日誌動輒數百萬行，
//         序列掃描要好幾秒，是典型的「工作量大、可切分、無相依」場景。
//   為何適合多執行緒：每一行的解析完全獨立，執行緒之間不需要任何同步。
//         關鍵設計是【每條執行緒先在自己的區域變數累加】，
//         全部跑完後才由主執行緒彙總 —— 這樣既避免了鎖，
//         也避免了對共享計數器頻繁寫入造成的 false sharing。
//         這是平行歸約（parallel reduction）的標準寫法。
//   對照：若每條執行緒直接對共享的 counts[level] 做 ++，
//         不但需要 atomic 或鎖，還會因為多核搶同一條 cache line 而
//         比單執行緒【更慢】。
// -----------------------------------------------------------------------------
struct LogStats {
    long long error = 0;
    long long warn = 0;
    long long info = 0;

    void accumulate(const LogStats& other) {
        error += other.error;
        warn += other.warn;
        info += other.info;
    }
};

// 產生模擬日誌（實務上會是讀檔）
std::vector<std::string> makeFakeLog(std::size_t lines) {
    static const char* levels[] = {"INFO", "INFO", "INFO", "WARN", "ERROR"};
    std::vector<std::string> out;
    out.reserve(lines);
    for (std::size_t i = 0; i < lines; ++i) {
        out.push_back(std::string("2026-07-20 10:00:00 [") + levels[i % 5] +
                      "] request id=" + std::to_string(i));
    }
    return out;
}

void scanChunk(const std::vector<std::string>& log, std::size_t begin,
               std::size_t end, LogStats& out) {
    LogStats local;   // ← 先在區域變數累加，避免 false sharing
    for (std::size_t i = begin; i < end; ++i) {
        const std::string& line = log[i];
        if (line.find("[ERROR]") != std::string::npos)      ++local.error;
        else if (line.find("[WARN]") != std::string::npos)  ++local.warn;
        else if (line.find("[INFO]") != std::string::npos)  ++local.info;
    }
    out = local;      // 全部算完才寫一次共享位置
}

void demoParallelLogScan() {
    const std::size_t LINES = 400000;
    auto log = makeFakeLog(LINES);

    unsigned n = std::thread::hardware_concurrency();
    if (n == 0) n = 4;                  // 標準允許回 0，必須有後備值
    if (n > 8) n = 8;                   // 避免過度訂閱

    // ---- 序列版 ----
    auto t0 = std::chrono::steady_clock::now();
    LogStats seq;
    scanChunk(log, 0, log.size(), seq);
    auto seqMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - t0).count();

    // ---- 平行版 ----
    t0 = std::chrono::steady_clock::now();
    std::vector<LogStats> partial(n);          // 每條執行緒一格，互不干擾
    std::vector<std::thread> threads;
    std::size_t chunk = log.size() / n;
    for (unsigned i = 0; i < n; ++i) {
        std::size_t b = i * chunk;
        std::size_t e = (i == n - 1) ? log.size() : b + chunk;
        threads.emplace_back(scanChunk, std::cref(log), b, e, std::ref(partial[i]));
    }
    for (auto& t : threads) t.join();

    LogStats par;
    for (const auto& p : partial) par.accumulate(p);   // 主執行緒彙總
    auto parMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - t0).count();

    std::cout << "  掃描 " << LINES << " 行日誌，使用 " << n << " 條執行緒"
              << std::endl;
    std::cout << "  統計結果：ERROR=" << par.error << " WARN=" << par.warn
              << " INFO=" << par.info << std::endl;
    std::cout << "  序列耗時: " << seqMs << " ms，平行耗時: " << parMs << " ms"
              << std::endl;
    if (parMs > 0) {
        std::cout << std::fixed << std::setprecision(2)
                  << "  加速比: " << static_cast<double>(seqMs) / parMs << " 倍"
                  << std::endl;
    }
    std::cout << "  結果一致性檢查: "
              << ((seq.error == par.error && seq.warn == par.warn &&
                   seq.info == par.info)
                      ? "✓ 序列與平行結果相同"
                      : "✗ 不一致！")
              << std::endl;
}

int main() {
    const long long RANGE_START = 1;
    const long long RANGE_END = 1000000;  // 計算 1 到 100 萬之間的質數

    std::cout << "========================================" << std::endl;
    std::cout << "    多執行緒效能提升示範" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "計算範圍: " << RANGE_START << " 到 " << RANGE_END << std::endl;

    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0) numCores = 4;   // 標準允許回 0
    std::cout << "可用 CPU 核心數: " << numCores << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // ========== 方式一：單執行緒計算 ==========
    std::cout << "\n【方式一】單執行緒計算：" << std::endl;

    auto singleStart = std::chrono::steady_clock::now();
    long long singleResult = countPrimesInRange(RANGE_START, RANGE_END);
    auto singleEnd = std::chrono::steady_clock::now();

    auto singleDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        singleEnd - singleStart).count();

    std::cout << "找到 " << singleResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << singleDuration << " 毫秒" << std::endl;

    // ========== 方式二：多執行緒計算 ==========
    std::cout << "\n【方式二】多執行緒計算（" << numCores << " 個執行緒）：" << std::endl;

    auto multiStart = std::chrono::steady_clock::now();

    // 準備儲存每個執行緒結果的容器
    std::vector<std::thread> threads;
    std::vector<long long> results(numCores, 0);

    // 計算每個執行緒負責的範圍
    long long rangeSize = (RANGE_END - RANGE_START + 1) / numCores;

    // 建立並啟動所有執行緒
    for (unsigned int i = 0; i < numCores; ++i) {
        long long threadStart = RANGE_START + i * rangeSize;
        long long threadEnd;

        // 最後一個執行緒處理剩餘的所有數字
        if (i == numCores - 1) {
            threadEnd = RANGE_END;
        } else {
            threadEnd = threadStart + rangeSize - 1;
        }

        std::cout << "  執行緒 " << i << ": 處理範圍 ["
                  << threadStart << ", " << threadEnd << "]" << std::endl;

        // 建立執行緒，使用 std::ref 傳遞引用（少了 std::ref 就只會改到副本）
        threads.emplace_back(countPrimesWorker, threadStart, threadEnd,
                            std::ref(results[i]));
    }

    // 等待所有執行緒完成
    for (auto& t : threads) {
        t.join();
    }

    // 彙總所有執行緒的結果
    long long multiResult = 0;
    for (unsigned int i = 0; i < numCores; ++i) {
        std::cout << "  執行緒 " << i << " 找到: " << results[i] << " 個質數" << std::endl;
        multiResult += results[i];
    }

    auto multiEnd = std::chrono::steady_clock::now();
    auto multiDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        multiEnd - multiStart).count();

    std::cout << "總共找到 " << multiResult << " 個質數" << std::endl;
    std::cout << "執行時間: " << multiDuration << " 毫秒" << std::endl;

    // ========== 效能比較 ==========
    std::cout << "\n========================================" << std::endl;
    std::cout << "效能比較結果：" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "單執行緒時間: " << singleDuration << " 毫秒" << std::endl;
    std::cout << "多執行緒時間: " << multiDuration << " 毫秒" << std::endl;

    double speedup = static_cast<double>(singleDuration) / multiDuration;
    double efficiency = speedup / numCores * 100;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "加速比 (Speedup): " << speedup << " 倍" << std::endl;
    std::cout << "並行效率: " << efficiency << "%" << std::endl;
    std::cout << "（並行效率低於 100% 是正常的，原因見檔頭【4.】負載不均與 Amdahl 定律）"
              << std::endl;
    std::cout << "========================================" << std::endl;

    // 驗證結果一致性
    if (singleResult == multiResult) {
        std::cout << "✓ 結果驗證通過：兩種方式得到相同結果" << std::endl;
    } else {
        std::cout << "✗ 結果驗證失敗：結果不一致！" << std::endl;
    }

    std::cout << "\n=== 陷阱示範：忘記 std::ref ===" << std::endl;
    demoMissingRef();

    std::cout << "\n=== 為什麼加速比達不到核心數：負載不均 ===" << std::endl;
    demoLoadImbalance();

    std::cout << "\n=== 日常實務：平行掃描伺服器日誌 ===" << std::endl;
    demoParallelLogScan();

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 1.2：為什麼需要多執行緒1.cpp" -o perf1
//   本檔只用到 C++11 起就有的功能，以 -std=c++17 -pedantic-errors 驗證通過。
//   建議加 -O2 觀察最佳化後的真實效能（未最佳化的計時參考價值較低）。

// 註:
//   ⚠️ 所有【毫秒數、加速比、並行效率】都取決於機器、核心數與當下負載，
//   【每次執行都不同】。質數的個數則是確定的（1..1000000 共 78498 個）。
//   以下為本機（16 邏輯核心、未加 -O2）某一次的實際執行結果。

// === 預期輸出 ===
// ========================================
//     多執行緒效能提升示範
// ========================================
// 計算範圍: 1 到 1000000
// 可用 CPU 核心數: 16
// ----------------------------------------
//
// 【方式一】單執行緒計算：
// 找到 78498 個質數
// 執行時間: 218 毫秒
//
// 【方式二】多執行緒計算（16 個執行緒）：
//   執行緒 0: 處理範圍 [1, 62500]
//   執行緒 1: 處理範圍 [62501, 125000]
//   執行緒 2: 處理範圍 [125001, 187500]
//   執行緒 3: 處理範圍 [187501, 250000]
//   執行緒 4: 處理範圍 [250001, 312500]
//   執行緒 5: 處理範圍 [312501, 375000]
//   執行緒 6: 處理範圍 [375001, 437500]
//   執行緒 7: 處理範圍 [437501, 500000]
//   執行緒 8: 處理範圍 [500001, 562500]
//   執行緒 9: 處理範圍 [562501, 625000]
//   執行緒 10: 處理範圍 [625001, 687500]
//   執行緒 11: 處理範圍 [687501, 750000]
//   執行緒 12: 處理範圍 [750001, 812500]
//   執行緒 13: 處理範圍 [812501, 875000]
//   執行緒 14: 處理範圍 [875001, 937500]
//   執行緒 15: 處理範圍 [937501, 1000000]
//   執行緒 0 找到: 6275 個質數
//   執行緒 1 找到: 5459 個質數
//   執行緒 2 找到: 5230 個質數
//   執行緒 3 找到: 5080 個質數
//   執行緒 4 找到: 4948 個質數
//   執行緒 5 找到: 4912 個質數
//   執行緒 6 找到: 4852 個質數
//   執行緒 7 找到: 4782 個質數
//   執行緒 8 找到: 4719 個質數
//   執行緒 9 找到: 4729 個質數
//   執行緒 10 找到: 4640 個質數
//   執行緒 11 找到: 4612 個質數
//   執行緒 12 找到: 4635 個質數
//   執行緒 13 找到: 4575 個質數
//   執行緒 14 找到: 4558 個質數
//   執行緒 15 找到: 4492 個質數
// 總共找到 78498 個質數
// 執行時間: 30 毫秒
//
// ========================================
// 效能比較結果：
// ----------------------------------------
// 單執行緒時間: 218 毫秒
// 多執行緒時間: 30 毫秒
// 加速比 (Speedup): 7.27 倍
// 並行效率: 45.42%
// （並行效率低於 100% 是正常的，原因見檔頭【4.】負載不均與 Amdahl 定律）
// ========================================
// ✓ 結果驗證通過：兩種方式得到相同結果
//
// === 陷阱示範：忘記 std::ref ===
//   用 std::ref 傳遞  → 1229 個質數（正確）
//   按值複製後傳遞    → 0（原變數從未被改動，且沒有任何警告）
//
// === 為什麼加速比達不到核心數：負載不均 ===
//   把 1..1000000 平均切成 4 段，量測各段【單獨】耗時：
//     段 0 [      1,  250000] → 22044 個質數，耗時   33 ms
//     段 1 [ 250001,  500000] → 19494 個質數，耗時   51 ms
//     段 2 [ 500001,  750000] → 18700 個質數，耗時   65 ms
//     段 3 [ 750001, 1000000] → 18260 個質數，耗時   72 ms
//   ← 元素個數相同，但耗時明顯遞增：這就是負載不均的來源
//
// === 日常實務：平行掃描伺服器日誌 ===
//   掃描 400000 行日誌，使用 8 條執行緒
//   統計結果：ERROR=80000 WARN=80000 INFO=240000
//   序列耗時: 34 ms，平行耗時: 5 ms
//   加速比: 6.80 倍
//   結果一致性檢查: ✓ 序列與平行結果相同
