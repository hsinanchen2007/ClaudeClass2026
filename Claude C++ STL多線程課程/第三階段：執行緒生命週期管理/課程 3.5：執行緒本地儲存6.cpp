// =============================================================================
//  課程 3.5：執行緒本地儲存6.cpp  —  thread_local 亂數引擎:每執行緒一條亂數流
// =============================================================================
//
// 【主題資訊 Information】
//   語法      : thread_local std::mt19937 rng{std::random_device{}()};
//   標準版本  : C++11(thread_local 關鍵字、<random> 函式庫)
//   標頭檔    : <random>(引擎與分布);thread_local 本身不需標頭檔
//   狀態大小  : 本機 sizeof(std::mt19937) 實測 5000 bytes(實作定義,見【概念補充】A)
//   執行緒安全: 引擎「本身」不是執行緒安全的;thread_local 讓它天生不被共享
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼亂數引擎是 thread_local 的教科書級案例】
// std::mt19937 是一個「有狀態」的物件:每次取值都會前進它內部 624 個字的狀態。
// 也就是說,operator() 既讀又寫 —— 兩個執行緒同時呼叫同一個引擎,就是
// 標準定義的 data race,是未定義行為。
//
// 可能的三種解法:
//   (a) 全域引擎 + mutex  → 正確,但亂數變成全域瓶頸,每取一個數就搶一次鎖
//   (b) 每次用時才建一個  → 正確,但建構 mt19937 要初始化 5000 bytes 的狀態,
//                            而且用 random_device 重新播種非常慢
//   (c) thread_local 引擎 → 每執行緒一份,免鎖、只播種一次      ← 本檔做法
// 幾乎所有需要並行亂數的程式(Monte Carlo、模擬、負載測試、遊戲 AI)
// 都採用 (c)。
//
// 【2. 為什麼不能只用 std::rand()】
// C 的 rand() 有兩個致命問題:
//   * 它有隱藏的全域狀態,多執行緒呼叫同樣是資料競爭(POSIX 的 rand_r
//     才是可重入版本)。
//   * 品質差:週期短、低位元的隨機性尤其糟,常見的 rand() % n 還會引入
//     模數偏差(modulo bias)——當 n 不整除 RAND_MAX+1 時,小的數字會被抽中得更頻繁。
// <random> 的設計正是為了解決這些問題:引擎(產生位元流)和分布
// (把位元流塑形成想要的機率分布)分離,std::uniform_int_distribution
// 內部已正確處理了模數偏差。
//
// 【3. 播種:random_device 的角色與限制】
// std::random_device 是「非確定性」的亂數來源,在 Linux 上通常來自作業系統的
// 熵池。它很慢,適合用來「播種」而不是「持續產生亂數」——
// 所以本檔的寫法是「用 random_device 取一個種子,交給 mt19937 之後全用它」。
//
// 重要但常被忽略的一點:標準「允許」random_device 是確定性的實作
// (若平台沒有真亂數來源,它可以退化成偽亂數,只要行為合法即可)。
// 主流平台上不是問題,但寫跨平台的密碼學相關程式時不可依賴它的不可預測性。
//
// 【4. 可重現性:研究與除錯的剛性需求】
// 用 random_device 播種代表「每次執行結果都不同」,這對正式執行是好事,
// 對除錯卻是災難 —— 出錯的那次再也重現不了。
// 實務上的標準做法是「種子可設定」:預設用 random_device,但允許用參數
// 或環境變數指定固定種子,重跑時就能完全重現。本檔【實務範例 2】示範這個模式。
//
// 【概念補充 Concept Deep Dive】
//
// (A) 記憶體成本要乘以執行緒數
//   本機實測 sizeof(std::mt19937) = 5000 bytes。這個數字值得解釋一下:
//   mt19937 的狀態是 624 個字,直覺會以為是 624 x 4 = 2496 bytes,
//   但 mersenne_twister_engine 的狀態型別是 uint_fast32_t ——
//   在 x86-64 Linux(LP64)上 uint_fast32_t 實際是 8 bytes,
//   所以是 624 x 8 = 4992,再加上索引欄位,共 5000 bytes。
//   這是實作定義的數值:同一份程式碼在 32 位元平台或 MSVC 上會不一樣。
//   作為 thread_local,本機 16 條執行緒就是約 78 KB —— 仍可接受。
//   但這正好示範了 thread_local 的通用取捨:任何放進 thread_local 的東西,
//   實際記憶體用量都要乘以執行緒數。若換成一個 1 MB 的緩衝區、開 200 條
//   執行緒,就是 200 MB。
//
// (B) 為什麼 thread_local 引擎也順便消滅了 false sharing
//   就算你替全域引擎加了鎖讓它「正確」,多核心下的效能仍然很差:
//   引擎狀態這條 cache line 會不斷在核心之間搬移(cache line ping-pong)。
//   thread_local 讓每條執行緒的引擎待在自己的 TLS 區塊,各自留在自己的
//   核心快取裡,這是「消除共享」相對「保護共享」的效能優勢來源。
//
// (C) 分布物件為什麼可以放在函式裡每次重建
//   本檔的 randomInt() 每次呼叫都新建一個 uniform_int_distribution。
//   這是安全的:分布物件很輕(通常只存上下界),而且它不持有跨呼叫的
//   關鍵狀態。真正必須長存的是「引擎」。
//   (嚴格說,分布「允許」有內部狀態 —— 例如 normal_distribution 常快取
//    Box-Muller 產生的第二個值 —— 每次重建會丟掉它,略有浪費但不算錯誤。)
//
// 【注意事項 Pay Attention】
// 1. 絕對不要跨執行緒共用同一個引擎物件而不加鎖 —— 那是 data race,是 UB,
//    症狀還特別隱晦(通常不會崩潰,只是亂數品質默默變差、序列出現相關性)。
// 2. 不要每次要亂數就 std::random_device{}() 重新播種:非常慢,而且
//    在某些平台上短時間內連續取值可能得到相關性高的種子。
// 3. rand() % n 有模數偏差;請用 std::uniform_int_distribution。
// 4. 每次執行結果都不同是刻意的;需要重現時請提供固定種子的路徑。
// 5. thread_local 引擎的記憶體成本 = sizeof(engine) × 執行緒數。
// 6. std::cout 並行輸出不保證整行不被切開,本檔用 mutex 保護輸出,
//    否則會看到兩個執行緒的數字互相穿插。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】thread_local 與亂數引擎
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 多執行緒程式裡,為什麼不能讓所有執行緒共用一個 std::mt19937?
//     答：因為引擎是有狀態的 —— 每次取值都會改寫它內部的狀態。
//         多個執行緒同時呼叫就是同時讀寫同一個物件,構成 data race,
//         屬於未定義行為。解法是每執行緒一份(thread_local),
//         免鎖又免除 cache line 在核心間彈跳。
//     追問：加個 mutex 不就好了?
//         → 正確性可以解決,但亂數通常在最內層迴圈被呼叫,
//           等於把整個程式序列化,而且引擎那條 cache line 會一直在核心間搬。
//           thread_local 是「消除共享」,效能上直接勝出。
//
// 🔥 Q2. 為什麼是「用 random_device 播種 mt19937」,而不是直接一直用
//        random_device 產生亂數?
//     答：random_device 是非確定性來源(通常取自作業系統熵池),很慢;
//         mt19937 是快速的偽亂數引擎。標準做法是讓慢的那個只跑一次
//         提供種子,之後全靠快的那個產生序列。
//     追問：random_device 一定是真亂數嗎?
//         → 不一定。標準允許它以確定性方式實作。主流平台上沒問題,
//           但密碼學用途不該僅依賴它,應使用專門的密碼學安全來源。
//
// ⚠️ 陷阱. 「兩條執行緒各自用 std::random_device{}() 播種,種子一定不同,
//         所以亂數序列一定不會重複。」哪裡錯了?
//     答：這是「機率上幾乎不會」,不是「保證不會」。標準完全沒有保證
//         random_device 兩次呼叫的結果相異;更實際的風險是,有些平台/舊實作
//         的 random_device 品質不佳,或程式改用 time(nullptr) 之類的播種 ——
//         同一秒啟動的多條執行緒就會拿到完全相同的種子,產生一模一樣的序列。
//         這種 bug 在測試時看起來一切正常,只有在正式環境高併發時才浮現。
//     為什麼會錯：把「實務上碰不到」當成「標準有保證」。需要保證互不重疊時,
//         正確做法是用一個確定的方式分配種子(例如 seed_seq,或
//         base_seed + thread_index),而不是祈禱它們碰巧不同。
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

// 保護 std::cout,避免多執行緒輸出互相切開
std::mutex g_ioMutex;

// -----------------------------------------------------------------------------
// 原始示範:每執行緒一條獨立的亂數流
// -----------------------------------------------------------------------------
thread_local std::mt19937 rng{std::random_device{}()};

int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void worker(int id) {
    int a = randomInt(1, 100);
    int b = randomInt(1, 100);
    int c = randomInt(1, 100);

    std::lock_guard<std::mutex> lock(g_ioMutex);
    std::cout << "  Thread " << id << ": " << a << ", " << b << ", " << c << std::endl;
}

// -----------------------------------------------------------------------------
// 【日常實務範例 1】並行 Monte Carlo:多執行緒估算圓周率
//   情境: Monte Carlo 是亂數最典型的工程用途 —— 在單位正方形裡隨機灑點,
//         落在四分之一圓內的比例 × 4 就是 π 的估計值。
//         這種計算天生可平行,而且每條執行緒都需要自己的亂數流。
//   為什麼用本主題: 若共用一個引擎,不只是 data race,連「各執行緒的取樣
//                   應該互相獨立」這個統計前提都會被破壞。
// -----------------------------------------------------------------------------
long countInsideCircle(long samples, unsigned seed) {
    // 這裡刻意用「明確給定的種子」而不是 thread_local 的 rng,
    // 讓結果可重現(見【詳細解釋 4】)。
    std::mt19937 local(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    long inside = 0;
    for (long i = 0; i < samples; ++i) {
        double x = dist(local);
        double y = dist(local);
        if (x * x + y * y <= 1.0) ++inside;
    }
    return inside;
}

double estimatePi(long samplesPerThread, int threadCount, unsigned baseSeed) {
    std::vector<long> results(static_cast<std::size_t>(threadCount), 0);
    std::vector<std::thread> pool;

    for (int i = 0; i < threadCount; ++i) {
        // 關鍵:用 baseSeed + i 明確分配種子,而不是「祈禱它們碰巧不同」
        pool.emplace_back([&results, i, samplesPerThread, baseSeed]() {
            results[static_cast<std::size_t>(i)] =
                countInsideCircle(samplesPerThread, baseSeed + static_cast<unsigned>(i));
        });
    }
    for (std::thread& t : pool) t.join();

    long inside = 0;
    for (long r : results) inside += r;
    return 4.0 * static_cast<double>(inside) /
           static_cast<double>(samplesPerThread * threadCount);
}

// -----------------------------------------------------------------------------
// 【日常實務範例 2】可重現的隨機:固定種子 → 每次跑出一模一樣的序列
//   情境: 壓力測試工具要產生「隨機」的請求順序,但當某次測試炸掉時,
//         必須能用同一個種子重跑、完整重現那次的請求序列來除錯。
//         這就是「預設隨機、可指定種子」模式。
//   為什麼用本主題: 對照上面的 thread_local + random_device(不可重現),
//                   說明兩種需求各自的正確做法。
// -----------------------------------------------------------------------------
std::string generateRequestSequence(unsigned seed, int n) {
    std::mt19937 engine(seed);
    std::uniform_int_distribution<int> pick(0, 3);
    const char* verbs[] = {"GET", "POST", "PUT", "DELETE"};

    std::string out;
    for (int i = 0; i < n; ++i) {
        if (i) out += ' ';
        out += verbs[pick(engine)];
    }
    return out;
}

// -----------------------------------------------------------------------------
// 【LeetCode 實戰範例】—— 本檔從缺,理由如下
//   LeetCode 的五題並行題(1114 Print in Order、1115 Print FooBar Alternately、
//   1116 Print Zero Even Odd、1117 Building H2O、1195 Fizz Buzz Multithreaded)
//   要求的都是「完全確定的輸出順序」,靠執行緒間同步達成;
//   而亂數與 thread_local 的用途是「讓各執行緒獨立且不確定」,方向相反。
//   LeetCode 上與亂數有關的題目(如 470、478)則是單執行緒的機率設計題,
//   和執行緒儲存期無關。硬套任一題都會失焦,因此誠實從缺。
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=== 原始示範:每執行緒一條獨立亂數流 ===" << std::endl;
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    t1.join();
    t2.join();

    std::cout << "\n=== mt19937 的狀態大小(實作定義) ===" << std::endl;
    std::cout << "  sizeof(std::mt19937) = " << sizeof(std::mt19937)
              << " bytes;每條執行緒各一份" << std::endl;

    std::cout << "\n=== 實務 1:並行 Monte Carlo 估算 π(種子固定 → 可重現) ===" << std::endl;
    double pi1 = estimatePi(200000, 4, 12345);
    double pi2 = estimatePi(200000, 4, 12345);
    std::cout << "  第一次估算: " << pi1 << std::endl;
    std::cout << "  相同種子再算一次: " << pi2 << std::endl;
    std::cout << "  兩次結果相同? " << std::boolalpha << (pi1 == pi2)
              << "(可重現性正是固定種子的目的)" << std::endl;

    std::cout << "\n=== 實務 2:壓力測試的可重現請求序列 ===" << std::endl;
    std::cout << "  seed=42  : " << generateRequestSequence(42, 8) << std::endl;
    std::cout << "  seed=42  : " << generateRequestSequence(42, 8)
              << "  ← 完全相同" << std::endl;
    std::cout << "  seed=999 : " << generateRequestSequence(999, 8)
              << "  ← 換種子就不同" << std::endl;

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread "課程 3.5：執行緒本地儲存6.cpp" -o tls_rng

// 注意:以下為某一次實際執行的結果,其中
//   * 第一段「原始示範」的六個數字,以及兩條執行緒的先後順序,
//     每次執行都不同 —— 因為 rng 是用 std::random_device 播種的。
//   * sizeof(std::mt19937) = 5000 是本機 libstdc++ 的實作定義值,
//     換編譯器/標準函式庫可能不同。
//   * 後面兩段刻意使用固定種子,所以每次執行都會得到完全相同的結果。

// === 預期輸出 ===
// === 原始示範:每執行緒一條獨立亂數流 ===
//   Thread 1: 21, 24, 9
//   Thread 2: 52, 90, 96
//
// === mt19937 的狀態大小(實作定義) ===
//   sizeof(std::mt19937) = 5000 bytes;每條執行緒各一份
//
// === 實務 1:並行 Monte Carlo 估算 π(種子固定 → 可重現) ===
//   第一次估算: 3.14079
//   相同種子再算一次: 3.14079
//   兩次結果相同? true(可重現性正是固定種子的目的)
//
// === 實務 2:壓力測試的可重現請求序列 ===
//   seed=42  : POST DELETE DELETE GET PUT DELETE PUT PUT
//   seed=42  : POST DELETE DELETE GET PUT DELETE PUT PUT  ← 完全相同
//   seed=999 : DELETE GET PUT DELETE GET GET PUT GET  ← 換種子就不同
