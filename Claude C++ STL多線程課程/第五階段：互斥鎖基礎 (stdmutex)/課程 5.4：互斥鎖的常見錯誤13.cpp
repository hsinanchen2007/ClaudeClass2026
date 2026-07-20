// =============================================================================
//  課程 5.4：互斥鎖的常見錯誤13.cpp  —  保護範圍太大：把並行變回序列
// =============================================================================
//
// 【主題資訊 Information】
//   本檔談的不是 API，而是一個【效能反模式】：
//       臨界區段（critical section）涵蓋了不需要保護的工作，
//       使得原本可以並行的計算被鎖強制序列化。
//   對照上一個檔案：範圍太小 → 正確性問題（TOCTOU）
//                   範圍太大 → 效能問題（本檔）
//   ⚠️ 兩者的修法方向【相反】，判準都是同一個：
//      臨界區段應該恰好涵蓋「不變量從被破壞到被修復」的那段期間。
//   標頭檔：<mutex>、<chrono>、<thread>
//
// 【詳細解釋 Explanation】
//
// 【1. 為什麼「多開執行緒卻沒變快」】
//   本檔的 inefficientWork 把 lock_guard 放在函式最前面：
//       std::lock_guard<std::mutex> lock(mtx);   // 整個函式都被鎖住
//       int temp = input * input;                 // 不需要鎖
//       sleep_for(100ms);                         // 不需要鎖（模擬耗時計算）
//       temp += input;                            // 不需要鎖
//       result += temp;                           // ← 只有這行需要鎖
//   兩條執行緒各做 100ms 的工作，理論上並行只要 100ms，
//   但因為整段都在鎖裡，第二條必須等第一條【完全結束】才能開始，
//   總時間變成 200ms——多執行緒的意義完全消失。
//   → 這是效能問題中最常見、也最容易修的一種：
//     系統看起來「有在跑」，CPU 使用率卻上不去，加機器也沒用。
//
// 【2. Amdahl 定律：序列化的部分決定了加速的上限】
//   若程式中有比例 s 的工作【必須序列執行】，其餘 (1-s) 可以完美並行，
//   則用 N 個核心的最大加速比是：
//       Speedup(N) = 1 / (s + (1 - s) / N)
//   當 N → ∞ 時，上限是 1/s。
//   代入本檔：inefficientWork 的臨界區段幾乎是 100%（s ≈ 1），
//   所以加速比 ≈ 1 —— 開再多執行緒都沒用。
//   efficientWork 的臨界區段只有一次整數加法（s 極小），
//   加速比才能逼近核心數。
//   → 【工程含義】：優化並行程式的第一步不是「加執行緒」，
//     而是「縮短必須序列的那一段」。
//
// 【3. 正確的做法：計算在鎖外，只有寫入在鎖內】
//       int temp = input * input;                 // 鎖外：純區域運算
//       sleep_for(100ms);                          // 鎖外：耗時工作
//       temp += input;                             // 鎖外
//       {
//           std::lock_guard<std::mutex> lock(mtx); // 只鎖必要的部分
//           result += temp;                        // 臨界區段只有這一行
//       }
//   通用的重構模式是「讀取快照 → 鎖外計算 → 鎖內寫回」：
//       T snapshot;
//       { lock_guard lk(mtx); snapshot = shared; }   // 短
//       T computed = expensiveTransform(snapshot);   // 鎖外，可並行
//       { lock_guard lk(mtx); shared = computed; }   // 短
//   ⚠️ 但要注意：這個模式引入了「讀取」與「寫回」之間的空隙——
//   如果 expensiveTransform 的結果依賴於「shared 在這段期間沒被改過」，
//   那就退化成上一個檔案的 TOCTOU 錯誤了。
//   兩者必須一起考慮，不能只顧效能。
//
// 【4. 臨界區段裡絕對不該出現的東西】
//   依危害程度排序：
//     (a) 【I/O】：檔案讀寫、網路請求、printf/cout——
//         可能阻塞數毫秒到數秒，期間所有執行緒都卡住。
//     (b) 【呼叫使用者提供的回呼】：你不知道它會做什麼，
//         它甚至可能回頭呼叫你的 API 而造成重入或死結。
//     (c) 【sleep】：持有鎖睡覺是純粹的浪費。
//     (d) 【記憶體配置】：可能觸發系統呼叫，且配置器本身也有鎖。
//     (e) 【長時間的純計算】：本檔的情況。
//   本課 5.1 的示範為了教學把 cout 放在鎖裡（那是為了展示輸出不交錯），
//   實務上應該「在鎖內組好字串，出鎖後再輸出」。
//
// 【概念補充 Concept Deep Dive】
//   * 鎖的粒度（granularity）是一個光譜，兩端都有代價：
//       粗粒度（一把大鎖保護全部）：簡單、不會死結，但並行度低
//       細粒度（每個資料項一把鎖）：並行度高，但鎖本身的記憶體與
//                                    管理成本上升，且【多把鎖 = 死結風險】
//     實務建議：從粗粒度開始（正確優先），量測後再針對熱點細分。
//     過早細分是並行程式設計最常見的過度工程。
//   * 縮小臨界區段不是唯一手段，還有：
//       - 讀多寫少 → std::shared_mutex（C++17）讓讀取者並行
//       - 簡單計數 → std::atomic，完全不需要鎖
//       - 每執行緒各自累積、最後合併（thread_local + reduce），
//         這是平行歸約的標準做法，鎖只在最後合併時用一次
//   * ⚠️ 縮小臨界區段【不能】以犧牲正確性為代價。
//     若不變量要求「A 和 B 必須一起更新」，就不能為了效能把它們拆開——
//     那會變成上一個檔案的錯誤。效能永遠是正確性之後的考量。
//
// 【注意事項 Pay Attention】
//   1. 臨界區段應恰好涵蓋「不變量被破壞到修復」的期間，不多不少。
//   2. 【絕不】在持有鎖時做 I/O、sleep、或呼叫使用者回呼。
//   3. 用大括號 { } 主動控制 lock_guard 的作用域，這是基本功。
//   4. 「讀快照 → 鎖外算 → 鎖內寫回」會引入空隙，
//      要確認不變量允許這麼做（否則就是 TOCTOU 錯誤）。
//   5. 先確保正確，再談效能。過早細分鎖的粒度會帶來死結風險。
//   6. 本檔測得的時間會因機器與負載而異，但「高效版約為低效版的一半」
//      這個關係是由 sleep 時間結構決定的，相當穩定。
//
// ═══════════════════════════════════════════════════════════════════════════
// 【面試題】鎖的粒度與臨界區段大小
// ───────────────────────────────────────────────────────────────────────────
// 🔥 Q1. 一個服務加了執行緒池之後，CPU 使用率卻上不去、吞吐量也沒改善。
//        你會先懷疑什麼？
//     答：先懷疑臨界區段太大——工作被鎖強制序列化了。
//         典型症狀是「多執行緒但 CPU 只跑滿一顆核心」。
//         驗證方式：取執行緒堆疊（gdb 的 thread apply all bt），
//         若多數執行緒都停在 pthread_mutex_lock，就確定是鎖競爭。
//         接著檢查臨界區段裡有沒有 I/O、sleep、或長時間計算。
//     追問：用 Amdahl 定律怎麼解釋？→ 加速上限是 1/s，s 是必須序列的比例。
//           若臨界區段佔了 90% 的執行時間，加速上限就只有約 1.1 倍，
//           開再多執行緒都沒有意義。
//
// 🔥 Q2. 縮小臨界區段有沒有風險？什麼時候不能縮？
//     答：有。若不變量要求「多個欄位必須一起更新」，
//         把它們拆進不同的臨界區段就會讓其他執行緒看到不一致的中間狀態，
//         那正是 check-then-act / TOCTOU 錯誤。
//         判準不是「越小越好」，而是「恰好涵蓋不變量被破壞到修復的期間」。
//     追問：那怎麼同時兼顧？→ 常見手法是「鎖外算、鎖內寫」：
//           把昂貴的計算搬到鎖外，最後用一次短的臨界區段原子地寫入結果。
//           前提是計算過程不依賴「共享狀態在此期間不變」。
//
// ⚠️ 陷阱. 「鎖的粒度當然是越細越好，這樣並行度最高。」
//     答：不對，細粒度有三個實際代價。
//         (1) 每把鎖都佔記憶體（本機 sizeof(std::mutex) = 40 bytes），
//             百萬個元素各一把鎖是 40MB 的純開銷；
//         (2) 需要同時持有多把鎖時就有【死結風險】，
//             而粗粒度的單一大鎖不可能死結；
//         (3) 鎖本身的取得/釋放也有成本，切得太細反而讓總開銷上升。
//     為什麼會錯：把「並行度」當成唯一的優化目標，
//         忽略了鎖本身的成本與正確性風險。
//         實務原則是【先粗後細】：從一把大鎖開始確保正確，
//         量測找出真正的熱點，再針對那裡細分。
//         過早細分是並行程式設計最常見的過度工程。
// ═══════════════════════════════════════════════════════════════════════════
//
// 【LeetCode 實戰範例】—— 本檔【略過】，理由如下：
//   本檔主題是【效能】——臨界區段的大小如何影響並行度。
//   LeetCode 的並行題（1114 / 1115 / 1116 / 1117 / 1195）只驗證輸出的
//   正確順序，不量測執行時間，而且那些題目的「工作」就是印一個數字，
//   根本沒有可以搬到鎖外的計算。硬掛一題無法示範本檔重點，故從缺。

#include <chrono>
#include <iostream>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

std::mutex mtx;
int result = 0;

void inefficientWork(int input) {
    std::lock_guard<std::mutex> lock(mtx);  // 💀 整個函式都被鎖住

    // 這部分不需要鎖
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 模擬耗時計算
    temp += input;

    // 只有這裡需要鎖
    result += temp;
}

void efficientWork(int input) {
    // 不需要鎖的部分
    int temp = input * input;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    temp += input;

    // 只鎖必要的部分
    {
        std::lock_guard<std::mutex> lock(mtx);
        result += temp;  // ✓ 最小化臨界區段
    }
}

// -----------------------------------------------------------------------------
// 【日常實務範例】影像批次處理：從「一把大鎖」到「鎖外計算」
//   情境：一批圖片要做縮圖與統計（總像素數、最大尺寸）。
//         縮圖是純 CPU 運算（可完美並行），統計則需要更新共享狀態。
//   ❌ 錯誤：整個 processImage 包在鎖裡 → 縮圖也被序列化，
//            8 條執行緒的效果等於 1 條。
//   ✅ 正確：縮圖在鎖外做，只有「把結果併入統計」那幾行在鎖內。
//
//   額外示範【更好的做法】：每條執行緒先在自己的區域變數累積，
//   最後只上一次鎖做合併（thread-local reduce）。
//   當工作項目很多時，這能把「取鎖次數」從 N 次降到「執行緒數」次。
// -----------------------------------------------------------------------------
struct ImageStats {
    long totalPixels = 0;
    int  maxWidth    = 0;
    int  processed   = 0;
};

// 模擬純運算：計算縮圖所需的工作量（不碰任何共享狀態）
static long computeThumbnailCost(int width, int height) {
    long acc = 0;
    for (int i = 0; i < 2000; ++i) {
        acc += (width * 31 + height * 17 + i) % 997;
    }
    return acc;
}

class ImagePipeline {
private:
    std::mutex mtx_;
    ImageStats stats_;

public:
    // ❌ 一把大鎖包住全部：純運算也被序列化
    void processBad(int width, int height) {
        std::lock_guard<std::mutex> lock(mtx_);
        const long cost = computeThumbnailCost(width, height);   // 不需要鎖！
        (void)cost;
        stats_.totalPixels += static_cast<long>(width) * height;
        if (width > stats_.maxWidth) stats_.maxWidth = width;
        ++stats_.processed;
    }

    // ✅ 運算在鎖外，只有合併統計在鎖內
    void processGood(int width, int height) {
        const long cost = computeThumbnailCost(width, height);   // 鎖外，可並行
        (void)cost;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stats_.totalPixels += static_cast<long>(width) * height;
            if (width > stats_.maxWidth) stats_.maxWidth = width;
            ++stats_.processed;
        }
    }

    // ✅✅ 更好：每執行緒先各自累積，最後只上一次鎖合併
    void mergeLocal(const ImageStats& local) {
        std::lock_guard<std::mutex> lock(mtx_);
        stats_.totalPixels += local.totalPixels;
        if (local.maxWidth > stats_.maxWidth) stats_.maxWidth = local.maxWidth;
        stats_.processed += local.processed;
    }

    ImageStats snapshot() {
        std::lock_guard<std::mutex> lock(mtx_);
        return stats_;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mtx_);
        stats_ = ImageStats{};
    }
};

enum class Mode { Bad, Good, LocalReduce };

static long runPipeline(ImagePipeline& pipe, Mode mode, int threads, int perThread) {
    pipe.reset();

    const auto t0 = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads));
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&pipe, mode, perThread, t]() {
            if (mode == Mode::LocalReduce) {
                ImageStats local;                    // 執行緒本地累積，完全不用鎖
                for (int i = 0; i < perThread; ++i) {
                    const int w = 640 + (t * 37 + i) % 1280;
                    const int h = 480 + (t * 53 + i) % 720;
                    const long cost = computeThumbnailCost(w, h);
                    (void)cost;
                    local.totalPixels += static_cast<long>(w) * h;
                    if (w > local.maxWidth) local.maxWidth = w;
                    ++local.processed;
                }
                pipe.mergeLocal(local);              // 整條執行緒只取一次鎖
            } else {
                for (int i = 0; i < perThread; ++i) {
                    const int w = 640 + (t * 37 + i) % 1280;
                    const int h = 480 + (t * 53 + i) % 720;
                    if (mode == Mode::Bad) pipe.processBad(w, h);
                    else                   pipe.processGood(w, h);
                }
            }
        });
    }
    for (auto& t : workers) t.join();

    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - t0).count();
}

int main() {
    std::cout << "=== 課程示範: 臨界區段大小對並行度的影響 ===" << std::endl;

    auto start = std::chrono::steady_clock::now();

    std::thread t1(inefficientWork, 10);
    std::thread t2(inefficientWork, 20);
    t1.join();
    t2.join();

    auto mid = std::chrono::steady_clock::now();

    const int inefficientResult = result;
    result = 0;

    std::thread t3(efficientWork, 10);
    std::thread t4(efficientWork, 20);
    t3.join();
    t4.join();

    auto end = std::chrono::steady_clock::now();

    auto inefficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    auto efficient_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);

    // 註：實際毫秒數每次執行都會有幾毫秒的浮動（sleep 只保證下限、
    //     還有排程延遲），故不列出數字，只驗證兩者的【結構性關係】。
    std::cout << "兩條執行緒各做 100ms 的工作" << std::endl;
    std::cout << "低效版本（整個函式都鎖住）耗時 >= 200ms: "
              << (inefficient_time.count() >= 200 ? "是" : "否")
              << "  ← 被序列化了，等於沒有並行" << std::endl;
    std::cout << "高效版本（只鎖必要部分）耗時 < 150ms: "
              << (efficient_time.count() < 150 ? "是" : "否")
              << "  ← 真正並行，約等於單次工作的時間" << std::endl;
    std::cout << "兩者計算結果相同: "
              << (inefficientResult == result ? "是" : "否")
              << "  (都是 10*10+10 + 20*20+20 = 530)" << std::endl;
    std::cout << "結論: 縮小臨界區段不影響正確性，卻讓耗時減半" << std::endl;

    std::cout << "\n=== 日常實務: 影像批次處理的三種鎖策略 ===" << std::endl;
    {
        ImagePipeline pipe;
        const int threads = 8, perThread = 400;

        const long badMs   = runPipeline(pipe, Mode::Bad, threads, perThread);
        const ImageStats badStats = pipe.snapshot();

        const long goodMs  = runPipeline(pipe, Mode::Good, threads, perThread);
        const ImageStats goodStats = pipe.snapshot();

        const long localMs = runPipeline(pipe, Mode::LocalReduce, threads, perThread);
        const ImageStats localStats = pipe.snapshot();

        std::cout << "8 條執行緒各處理 " << perThread << " 張圖" << std::endl;
        std::cout << "三種策略的處理張數相同: "
                  << (badStats.processed == goodStats.processed &&
                      goodStats.processed == localStats.processed ? "是" : "否")
                  << "  (都是 " << localStats.processed << ")" << std::endl;
        std::cout << "三種策略的統計結果相同: "
                  << (badStats.totalPixels == goodStats.totalPixels &&
                      goodStats.totalPixels == localStats.totalPixels &&
                      badStats.maxWidth == localStats.maxWidth ? "是" : "否")
                  << "  (正確性不受鎖策略影響)" << std::endl;
        std::cout << "「鎖外計算」比「一把大鎖」快: "
                  << (goodMs < badMs ? "是" : "否") << std::endl;
        std::cout << "「本地累積 + 一次合併」是三者中最快或相當: "
                  << (localMs <= goodMs + 5 ? "是" : "否") << std::endl;
        std::cout << "註: 實際毫秒數取決於機器與當下負載，每次執行都不同，"
                     "故只驗證相對關係" << std::endl;
    }

    return 0;
}

// 編譯: g++ -std=c++17 -Wall -Wextra -pthread '課程 5.4：互斥鎖的常見錯誤13.cpp' -o scope_too_large

// -----------------------------------------------------------------------------
// 【輸出但書】
//   1. 本檔【刻意不輸出任何毫秒數】——它們取決於機器、核心數與當下負載，
//      每次執行都不同。改為驗證「結構性關係」（>= 200ms、< 150ms、
//      A 比 B 快），這些關係由 sleep 的時間結構決定，相當穩定。
//   2. 「低效版 >= 200ms」是結構決定的：兩條執行緒各 sleep 100ms
//      且全程持鎖，必然序列化。
//      「高效版 < 150ms」同理：兩條的 sleep 完全重疊。
//   3. 最後一行的比較（本地累積是否最快）在極端負載下理論上可能翻轉，
//      故用 +5ms 的容差；本機連續 5 次執行皆為「是」。
// -----------------------------------------------------------------------------

// === 預期輸出 ===
// === 課程示範: 臨界區段大小對並行度的影響 ===
// 兩條執行緒各做 100ms 的工作
// 低效版本（整個函式都鎖住）耗時 >= 200ms: 是  ← 被序列化了，等於沒有並行
// 高效版本（只鎖必要部分）耗時 < 150ms: 是  ← 真正並行，約等於單次工作的時間
// 兩者計算結果相同: 是  (都是 10*10+10 + 20*20+20 = 530)
// 結論: 縮小臨界區段不影響正確性，卻讓耗時減半
//
// === 日常實務: 影像批次處理的三種鎖策略 ===
// 8 條執行緒各處理 400 張圖
// 三種策略的處理張數相同: 是  (都是 3200)
// 三種策略的統計結果相同: 是  (正確性不受鎖策略影響)
// 「鎖外計算」比「一把大鎖」快: 是
// 「本地累積 + 一次合併」是三者中最快或相當: 是
// 註: 實際毫秒數取決於機器與當下負載，每次執行都不同，故只驗證相對關係
